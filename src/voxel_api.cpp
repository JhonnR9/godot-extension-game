#include "voxel_api.h"
#include "terrain_sampler.h"
#include "ChunkDiskRepository.h"
#include "chunk_model.h"
#include "chunk_pool.h"
#include "save_service.h"
#include "utils.h"
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/classes/world_environment.hpp>
#include <vector>

namespace godot {

void VoxelAPI::_ready() {
	_last_focos_position = Vector3i();

	_chunk_pool.instantiate();
	_chunk_repository.instantiate();
	_chunk_stream_manager.instantiate();
	_model_generator.instantiate();
	_mesh_generator.instantiate();
	_disk_repository.instantiate();
	_region_loader.instantiate();
	_apply_render_settings_fields(get_default_render_settings(), false);
	_apply_vsync_setting(get_default_render_settings());

	_region_loader->set_repository(_disk_repository);

	StreamSettings stream_settings{};
	stream_settings.cache_radius = _cache_radius;
	stream_settings.world_height = _world_height;
	stream_settings.world_radius = _world_radius;

	_chunk_stream_manager->set_stream_settings(stream_settings);
	_chunk_pool->set_owner(this);
	_chunk_pool->set_prewarm(_prewarm_chunk_pool);

	set_process(true);
	_biome_registry = BiomeRegistry::load(_biome_registry_path);
	_terrain_base_height = _biome_registry->world.base_height;
	_terrain_amplitude = _biome_registry->world.amplitude;
	_water_level = _biome_registry->world.sea_level;
	_setup_noises();

}

void VoxelAPI::_setup_noises() {
    const auto &w=_biome_registry->world;
    auto setup=[](Ref<FastNoiseLite> &noise,const TerrainNoiseProfile &profile,FastNoiseLite::NoiseType type) {
        noise.instantiate(); noise->set_noise_type(type);
        noise->set_frequency(profile.frequency); noise->set_fractal_octaves(profile.octaves);
    };
    setup(_terrain_noise,w.terrain,FastNoiseLite::TYPE_PERLIN);
    setup(_biome_noise,w.climate,FastNoiseLite::TYPE_SIMPLEX);
    setup(_dune_noise,w.dune,FastNoiseLite::TYPE_SIMPLEX);
    setup(_mountain_noise,w.mountain,FastNoiseLite::TYPE_SIMPLEX);
    setup(_ocean_noise,w.ocean,FastNoiseLite::TYPE_SIMPLEX);
    setup(_river_noise,w.river,FastNoiseLite::TYPE_SIMPLEX);
}
void VoxelAPI::set_biome_registry_path(const String &path) {
    ERR_FAIL_COND_MSG(is_node_ready(), "Set biome_registry_path before adding VoxelAPI to the scene tree.");
    _biome_registry_path=path;
}
Dictionary VoxelAPI::validate_biome_registry(const Dictionary &data) {
    String error; const auto registry=BiomeRegistry::from_dictionary(data,error);
    Dictionary result; result["valid"]=bool(registry); result["error"]=error; return result;
}
Dictionary VoxelAPI::sample_terrain_column(const Vector2i &position) const {
    ERR_FAIL_COND_V_MSG(!_biome_registry, Dictionary(), "VoxelAPI must be ready before sampling terrain.");
    const auto c=TerrainSampler::sample(_make_terrain_settings(),position.x,position.y);
    Dictionary result;
    result["height"]=c.surface_height; result["biome_id"]=c.biome_id;
    result["biome_name"]=String(c.definition->name.c_str()); result["climate"]=c.climate_weight;
    result["surface_block"]=voxel::type(c.surface_block); result["soil_block"]=voxel::type(c.subsurface_block);
    result["rock_block"]=voxel::type(c.stone_block); result["deep_rock_block"]=voxel::type(c.deep_stone_block);
    result["soil_depth"]=c.subsurface_depth; result["water_level"]=c.water_level;
    result["trees_allowed"]=c.trees_allowed; result["surface_water"]=c.surface_water;
    return result;
}

void VoxelAPI::_init_chunks() {
	ERR_FAIL_COND(_chunk_pool.is_null());

	_is_initializing = true;

	_last_focos_position = voxel::block_to_chunk_coords(_get_current_focus_position());

	_previous_player_chunk_pos		= _last_focos_position;
	const Vector3i current_position = voxel::block_to_chunk_coords(_get_current_focus_position());
	_chunk_stream_manager->shift_chunks(current_position);

	// grid regions 3 x 3 load
	const Vector3i current_region = voxel::chunk_to_region_coords(_last_focos_position);
	for (int x = -1; x <= 1; ++x) {
		for (int z = -1; z <= 1; ++z) {
			_queue_region_load(current_region + Vector3i(x, 0, z));
		}
	}

	ERR_FAIL_COND(_chunk_stream_manager.is_null());
	_chunk_stream_manager->rebuild_all_chunks(_last_focos_position);
	_initial_loading_chunks.clear();
	for (int x = -1; x <= 1; ++x) {
		for (int z = -1; z <= 1; ++z) {
			for (int y = -1; y <= 1; ++y) {
				const Vector3i chunk_pos = _last_focos_position + Vector3i(x, y, z);
				if (chunk_pos.y >= WORLD_MIN_CHUNK_Y && chunk_pos.y <= WORLD_MAX_CHUNK_Y) {
					_initial_loading_chunks.insert(chunk_pos);
				}
			}
		}
	}
}

void VoxelAPI::_remove_chunk(ChunkNode *p_chunk_node) {
	ERR_FAIL_NULL(p_chunk_node);

	const Vector3i pos = voxel::block_to_chunk_coords(p_chunk_node->get_global_position());
	_rendered_chunks.erase(pos);
	_chunk_pool->release(p_chunk_node);

	// The chunk data still exists (LOADED); it has simply ceased to be
	// drawn on the screen. If it has been removed from the repository for another
	// reason (e.g., it was already removed in _cleanup_far_chunks), get_chunk returns
	// null and we do nothing.
	if (std::shared_ptr<Chunk> chunk = _chunk_repository->get_chunk(pos)) {
		if (chunk->stage == ChunkStage::RENDERED) {
			chunk->stage = ChunkStage::LOADED;
		}
	}
}

void VoxelAPI::_update_visible_chunks() {
	if (_is_initializing && !_pending_region_loads.is_empty()) {
		return;
	}

	for (const Vector3i &pos : _chunk_stream_manager->pop_queue_free_chunks()) {
		if (_rendered_chunks.has(pos)) {
			_remove_chunk(_rendered_chunks[pos]);
		}
	}

	for (const Vector3i &pos : _chunk_stream_manager->get_active_chunks_snapshot()) {
		std::shared_ptr<Chunk> chunk = _chunk_repository->get_chunk(pos);

		if (!chunk) {
			chunk = _model_generator->get_loading_chunk(pos);
		}

		if (chunk) {
			switch (chunk->stage) {
				case ChunkStage::LOADED:
				case ChunkStage::WAITING_NEIGHBORS:
					if (!_rendered_chunks.has(pos)) {
						_try_build_mesh_with_neighbors(pos);
					}
					break;
				default:
					break;
			}
			continue;
		}

		_queue_async_generate_chunk(pos);
	}
}

// This routine repeats every 2 seconds.
void VoxelAPI::_cleanup_far_chunks() const {
	std::vector<Vector3i> to_remove;

	const int cache_radius_sq = _cache_radius * _cache_radius;

	for (const auto pos : _chunk_repository->get_keys_snapshot()) {
		const int dx = ABS(pos.x - _last_focos_position.x);
		const int dy = ABS(pos.y - _last_focos_position.y);
		const int dz = ABS(pos.z - _last_focos_position.z);

		bool out_of_vertical_bounds	  = dy > (_world_height + 2);
		out_of_vertical_bounds = out_of_vertical_bounds || pos.y < WORLD_MIN_CHUNK_Y || pos.y > WORLD_MAX_CHUNK_Y;
		bool out_of_horizontal_bounds = (dx * dx + dz * dz) > cache_radius_sq;

		if (out_of_vertical_bounds || out_of_horizontal_bounds) {
			// Chunks still being generated (QUEUED_GENERATION/GENERATING) do not
			// yet exist in the repository (only in the model_generator's
			// placeholder), so get_keys_snapshot() never returns them
			// here—we only need to protect those that already have
			// mesh generation in progress.
			const std::shared_ptr<Chunk> chunk = _chunk_repository->get_chunk(pos);

			const bool busy_with_mesh = chunk &&
					(chunk->stage == ChunkStage::QUEUED_MESH || chunk->stage == ChunkStage::GENERATING_MESH);

			if (!busy_with_mesh) {
				to_remove.push_back(pos);
			}
		}
	}

	for (const Vector3i &pos : to_remove) {
		if (std::shared_ptr<Chunk> chunk = _chunk_repository->get_chunk(pos)) {
			chunk->stage = ChunkStage::UNLOADING;
		}
		_chunk_repository->remove_chunk(pos);
	}
}

void VoxelAPI::_process(double delta) {
	const Vector3i current_position = voxel::block_to_chunk_coords(_get_current_focus_position());

	if (current_position != _last_focos_position) {
		_chunk_stream_manager->shift_chunks(current_position);
		_update_region_streaming(current_position, _last_focos_position);
		_previous_player_chunk_pos = _last_focos_position;
		_last_focos_position	   = current_position;
	}

	_process_loaded_regions();
	_update_visible_chunks();

	const float frame_ms   = static_cast<float>(delta) * 1000.0f;
	size_t mesh_queue_size = _mesh_generator->get_queue_size();

	if (mesh_queue_size > 200)
		_current_chunks_finalize_in_frame = 100;
	else if (frame_ms > 16.0f)
		_current_chunks_finalize_in_frame = 10;
	else
		_current_chunks_finalize_in_frame = 25;

	_process_models();
	_process_meshes(current_position);
	_update_initial_loading_status();

	HashSet<Vector3i> dirty = _chunk_repository->consume_dirty_chunks();

	for (const Vector3i &pos : dirty) {
		_rebuild_chunk(pos);
	}

	static double cleanup_timer = 0.0;
	cleanup_timer += delta;

	if (cleanup_timer > 2.0) {
		_cleanup_far_chunks();
		cleanup_timer = 0.0;
	}
}

void VoxelAPI::_exit_tree() {
	save_world_final();
}

void VoxelAPI::break_block(const Vector3 &world_pos) {
	const Vector3i block_pos = voxel::world_to_block(world_pos);
	if (block_pos.y <= WORLD_BEDROCK_Y) {
		return;
	}
	_chunk_repository->set_block(block_pos, 0);
	_flow_water_into(block_pos);
}

void VoxelAPI::_flow_water_into(const Vector3i &p_target) const {
	if (_chunk_repository.is_null() || p_target.y >= _water_level) {
		return;
	}

	const Vector3i target_chunk_pos = voxel::block_to_chunk_coords(p_target);
	const std::shared_ptr<Chunk> target_chunk = _chunk_repository->get_chunk(target_chunk_pos);
	if (!target_chunk) {
		return;
	}
	const Vector3i target_local = voxel::block_to_chunk_local_block(p_target);
	if (!voxel::is_air(target_chunk->get_block(target_local.x, target_local.y, target_local.z))) {
		return;
	}

	const Vector3i directions[] = {
		Vector3i(1, 0, 0), Vector3i(-1, 0, 0), Vector3i(0, 0, 1),
		Vector3i(0, 0, -1), Vector3i(0, 1, 0), Vector3i(0, -1, 0)
	};
	for (const Vector3i &direction : directions) {
		const Vector3i source_pos = p_target + direction;
		// Water spreads sideways and down, never uphill or above sea level.
		if (source_pos.y < p_target.y || source_pos.y >= _water_level) {
			continue;
		}
		const Vector3i source_chunk_pos = voxel::block_to_chunk_coords(source_pos);
		const std::shared_ptr<Chunk> source_chunk = _chunk_repository->get_chunk(source_chunk_pos);
		if (!source_chunk) {
			continue;
		}
		const Vector3i source_local = voxel::block_to_chunk_local_block(source_pos);
		const voxel::Block source = source_chunk->get_block(source_local.x, source_local.y, source_local.z);
		if (voxel::type(source) != voxel::block_ids::water) {
			continue;
		}
		voxel::Block flags = voxel::BLOCK_FLAG_TRANSPARENT;
		if (voxel::has_flag(source, voxel::BLOCK_FLAG_OCEAN)) {
			flags |= voxel::BLOCK_FLAG_OCEAN;
		}
		_chunk_repository->set_block(p_target,
				voxel::make_block(voxel::block_ids::water, flags));
		return;
	}
}

void VoxelAPI::set_block(const Vector3 &p_world_pos, const voxel::Block &p_block) const {
	Vector3i block_pos = voxel::world_to_block(p_world_pos);
	if (block_pos.y <= WORLD_BEDROCK_Y) {
		return;
	}
	voxel::Block block = p_block;
	if (voxel::type(block) == voxel::block_ids::water) {
		block = voxel::make_block(voxel::block_ids::water, voxel::BLOCK_FLAG_TRANSPARENT);
	} else if (!voxel::is_air(block) && (block & (voxel::BLOCK_FLAG_SOLID | voxel::BLOCK_FLAG_TRANSPARENT)) == 0) {
		block = voxel::make_block(voxel::type(block));
	}
	_chunk_repository->set_block(block_pos, block);
}

bool VoxelAPI::is_water_at(const Vector3 &p_world_pos) const {
	if (_chunk_repository.is_null()) {
		return false;
	}

	const Vector3i block_pos = voxel::world_to_block(p_world_pos);
	const Vector3i chunk_pos = voxel::block_to_chunk_coords(block_pos);
	const std::shared_ptr<Chunk> chunk = _chunk_repository->get_chunk(chunk_pos);
	if (!chunk) {
		return false;
	}

	const Vector3i local_pos = voxel::block_to_chunk_local_block(block_pos);
	return voxel::type(chunk->get_block(local_pos.x, local_pos.y, local_pos.z)) == voxel::block_ids::water;
}

bool VoxelAPI::is_ocean_at(const Vector3 &p_world_pos) const {
	if (_chunk_repository.is_null()) {
		return false;
	}

	const Vector3i block_pos = voxel::world_to_block(p_world_pos);
	const Vector3i chunk_pos = voxel::block_to_chunk_coords(block_pos);
	const std::shared_ptr<Chunk> chunk = _chunk_repository->get_chunk(chunk_pos);
	if (!chunk) {
		return false;
	}

	const Vector3i local_pos = voxel::block_to_chunk_local_block(block_pos);
	const voxel::Block block = chunk->get_block(local_pos.x, local_pos.y, local_pos.z);
	return voxel::type(block) == voxel::block_ids::water && voxel::has_flag(block, voxel::BLOCK_FLAG_OCEAN);
}

int32_t VoxelAPI::get_block_type_at(const Vector3 &p_world_pos) const {
	if (_chunk_repository.is_null()) {
		return -1;
	}

	const Vector3i block_pos = voxel::world_to_block(p_world_pos);
	const Vector3i chunk_pos = voxel::block_to_chunk_coords(block_pos);
	const std::shared_ptr<Chunk> chunk = _chunk_repository->get_chunk(chunk_pos);
	if (!chunk) {
		return -1;
	}

	const Vector3i local_pos = voxel::block_to_chunk_local_block(block_pos);
	return static_cast<int32_t>(voxel::type(chunk->get_block(local_pos.x, local_pos.y, local_pos.z)));
}

void VoxelAPI::set_focus_node(Node3D *p_node) {
	_focus_node		= p_node;
	_use_manual_pos = false;
}

void VoxelAPI::set_focus_position(Vector3 p_pos) {
	_focus_manual_pos = p_pos;
	_use_manual_pos	  = true;
}

void VoxelAPI::create_new_world(const int32_t p_seed, const String &p_name) {
	if (_disk_repository.is_valid() && _disk_repository->get_current_world_id() != 0) {
		save_world_final();
	}
	_clear_world();

	const int64_t id			 = SaveService::get_singleton()->create_world(p_seed, p_name);
	const WorldModel world_model = SaveService::get_singleton()->load_world_model(id);

	_chunk_repository->set_world_model(world_model);
	_disk_repository->set_current_world(world_model.id);

	_terrain_noise->set_seed(world_model.seed);
	_biome_noise->set_seed(world_model.seed + 2);
	_dune_noise->set_seed(world_model.seed + 3);
	_mountain_noise->set_seed(world_model.seed + 4);
	_ocean_noise->set_seed(world_model.seed + 5);
	_river_noise->set_seed(world_model.seed + 6);

	_setup_generation_pipeline(p_seed);

	_init_chunks();
}

void VoxelAPI::start_world(int64_t p_id) {
	if (_disk_repository.is_valid() && _disk_repository->get_current_world_id() != 0) {
		save_world_final();
	}
	_clear_world();

	_disk_repository->set_current_world(p_id);
	const WorldModel world_model = SaveService::get_singleton()->load_world_model(p_id);

	_chunk_repository->set_world_model(world_model);
	_terrain_noise->set_seed(world_model.seed);
	_biome_noise->set_seed(world_model.seed + 2);
	_dune_noise->set_seed(world_model.seed + 3);
	_mountain_noise->set_seed(world_model.seed + 4);
	_ocean_noise->set_seed(world_model.seed + 5);
	_river_noise->set_seed(world_model.seed + 6);
	_setup_generation_pipeline(world_model.seed);


	if (_focus_node) {
		const Dictionary player_data = SaveService::get_singleton()->load_world_section(p_id, "player");
		const Array saved_position = player_data.get("position", Array());
		if (saved_position.size() >= 3) {
			_focus_node->set_global_position(Vector3(
					double(saved_position[0]),
					double(saved_position[1]),
					double(saved_position[2])));
		} else {
			// Use the same seeded surface calculation as generation, including
			// custom biome relief and an independently configured sea level.
			Vector3 spawn = _focus_node->get_global_position();
			const Vector3i block = voxel::world_to_block(spawn);
			const auto column = TerrainSampler::sample(_make_terrain_settings(), block.x, block.z);
			spawn.y = MAX(column.surface_height + 1, column.water_level) + 5.0;
			_focus_node->set_global_position(spawn);
		}
		if (player_data.has("yaw") && player_data.has("pitch") && _focus_node->has_method("restore_rotation")) {
			_focus_node->call("restore_rotation", double(player_data["yaw"]), double(player_data["pitch"]));
		}
	}


	_init_chunks();
}

Dictionary VoxelAPI::_normalize_render_settings(const Dictionary &p_settings) {
	const auto read_int = [&p_settings](const String &key, int fallback, int min_value, int max_value) {
		const Variant value = p_settings.get(key, fallback);
		int parsed = fallback;
		if (value.get_type() == Variant::INT || value.get_type() == Variant::FLOAT) {
			parsed = static_cast<int>(double(value));
		}
		return CLAMP(parsed, min_value, max_value);
	};

	Dictionary normalized;
	normalized["render_distance"] = read_int("render_distance", 4, 4, 30);
	normalized["vertical_render_distance"] = read_int("vertical_render_distance", 3, 2, 15);
	normalized["distance_fog_enabled"] = bool(p_settings.get("distance_fog_enabled", true));
	normalized["distance_fog_start_percent"] = read_int("distance_fog_start_percent", 65, 45, 75);
	normalized["vsync"] = bool(p_settings.get("vsync", true));
	return normalized;
}

Dictionary VoxelAPI::get_default_render_settings() {
	Dictionary saved;
	if (SaveService *service = SaveService::get_singleton()) {
		saved = service->load_user_settings("render");
	}
	return _normalize_render_settings(saved);
}

void VoxelAPI::_apply_vsync_setting(const Dictionary &p_settings) {
	if (DisplayServer *display = DisplayServer::get_singleton()) {
		const bool enabled = bool(p_settings.get("vsync", true));
		display->window_set_vsync_mode(enabled ? DisplayServer::VSYNC_ENABLED : DisplayServer::VSYNC_DISABLED);
	}
}

void VoxelAPI::set_default_render_settings(const Dictionary &p_settings) {
	const Dictionary normalized = _normalize_render_settings(p_settings);
	_apply_vsync_setting(normalized);
	if (SaveService *service = SaveService::get_singleton()) {
		if (!service->save_user_settings("render", normalized)) {
			WARN_PRINT("Could not save render settings.");
		}
	}
}

Dictionary VoxelAPI::get_render_settings() const {
	Dictionary settings;
	settings["render_distance"] = _world_radius;
	settings["vertical_render_distance"] = _world_height;
	settings["distance_fog_enabled"] = _distance_fog_enabled;
	settings["distance_fog_start_percent"] = _distance_fog_start_percent;
	if (DisplayServer *display = DisplayServer::get_singleton()) {
		settings["vsync"] = display->window_get_vsync_mode() != DisplayServer::VSYNC_DISABLED;
	} else {
		settings["vsync"] = true;
	}
	return settings;
}

void VoxelAPI::_apply_render_settings_fields(const Dictionary &p_settings, const bool p_refresh_active_chunks) {
	const Dictionary settings = _normalize_render_settings(p_settings);
	_world_radius = settings["render_distance"];
	_world_height = settings["vertical_render_distance"];
	_distance_fog_enabled = settings["distance_fog_enabled"];
	_distance_fog_start_percent = settings["distance_fog_start_percent"];
	_cache_radius = _world_radius + 3;
	_diameter = (_cache_radius * 2) + 1;
	int horizontal_chunk_count = 0;
	for (int x = -_world_radius; x <= _world_radius; ++x) {
		for (int z = -_world_radius; z <= _world_radius; ++z) {
			if (x * x + z * z <= _world_radius * _world_radius) ++horizontal_chunk_count;
		}
	}
	const int active_chunk_estimate = horizontal_chunk_count * (_world_height * 2 + 1);
	_prewarm_chunk_pool = MIN(active_chunk_estimate, 1024);
	if (_chunk_pool.is_valid()) {
		// The pool tracks total capacity (active and idle nodes) and grows when
		// render settings increase; startup prewarm remains capped for load time.
		_chunk_pool->set_prewarm(MAX(_prewarm_chunk_pool, active_chunk_estimate));
	}
	_apply_distance_fog();

	if (p_refresh_active_chunks && _chunk_stream_manager.is_valid()) {
		StreamSettings stream_settings{};
		stream_settings.cache_radius = _cache_radius;
		stream_settings.world_height = _world_height;
		stream_settings.world_radius = _world_radius;
        
		_chunk_stream_manager->set_stream_settings(stream_settings);
		_chunk_stream_manager->shift_chunks(_last_focos_position);
	}
}

void VoxelAPI::_apply_distance_fog() const {
	Node *parent = get_parent();
	if (parent == nullptr) return;
	WorldEnvironment *world_environment = Object::cast_to<WorldEnvironment>(parent->get_node_or_null("WorldEnvironment"));
	if (world_environment == nullptr) return;
	Ref<Environment> environment = world_environment->get_environment();
	if (environment.is_null()) return;

	environment->set_fog_enabled(_distance_fog_enabled);
	if (!_distance_fog_enabled) return;
	environment->set_fog_mode(Environment::FOG_MODE_DEPTH);
	environment->set_fog_light_color(Color(0.47f, 0.54f, 0.65f, 1.0f));
	environment->set_fog_sky_affect(1.0f);
	const float render_distance = static_cast<float>(_world_radius * Chunk::SIZE_X);
	const float fog_begin = MAX(8.0f, render_distance * static_cast<float>(_distance_fog_start_percent) / 100.0f);
	const float fog_end = MAX(fog_begin + 8.0f, render_distance * 1.02f);
	environment->set_fog_depth_begin(fog_begin);
	environment->set_fog_depth_end(fog_end);
	environment->set_fog_depth_curve(1.0f);
}

void VoxelAPI::set_render_settings(const Dictionary &p_settings) {
	const Dictionary normalized = _normalize_render_settings(p_settings);
	set_default_render_settings(normalized);
	_apply_render_settings_fields(normalized, true);
}

Vector3 VoxelAPI::_get_current_focus_position() const {
	if (!_use_manual_pos && _focus_node) {
		return _focus_node->get_global_position();
	}
	return _focus_manual_pos;
}

void VoxelAPI::_process_models() {
	HashMap<Vector3i, std::shared_ptr<Chunk>> ready_models = _model_generator->consume_generated_results();

	if (ready_models.is_empty()) {
		return;
	}

	for (auto &ready_model : ready_models) {
		//  Chunk start with LOADED.
		_queue_region_load(voxel::chunk_to_region_coords(ready_model.key));
		_chunk_repository->add_chunk(ready_model.key, ready_model.value);
	}
}

void VoxelAPI::_ensure_region_loaded_for_chunk(const Vector3i &chunk_pos) {
	if (_disk_repository.is_null()) {
		return;
	}

	Vector3i region_pos = voxel::chunk_to_region_coords(chunk_pos);
	_queue_region_load(region_pos);
}

void VoxelAPI::_queue_region_load(const Vector3i &region_pos) {
	if (_region_cache.has(region_pos) || _pending_region_loads.has(region_pos)) {
		return;
	}

	_pending_region_loads.insert(region_pos);
	_region_loader->queue_async_load_region(region_pos);
}

void VoxelAPI::_process_loaded_regions() {
	Vector<Vector3i> ready_regions;

	for (const Vector3i &region_pos : _pending_region_loads) {
		if (_region_loader->has_loaded_region(region_pos)) {
			ready_regions.push_back(region_pos);
		}
	}

	for (const Vector3i &region_pos : ready_regions) {
		voxel::Region region = _region_loader->consume_loaded_region(region_pos);
		_pending_region_loads.erase(region_pos);
		_region_cache.insert(region_pos, region);
		if (!region.edited_chunks.is_empty()) {
			_chunk_repository->merge_region_edits(region);
		}
	}
}

void VoxelAPI::_update_region_streaming(const Vector3i &current_chunk_pos, const Vector3i &previous_chunk_pos) {
	Vector3i current_region	 = voxel::chunk_to_region_coords(current_chunk_pos);
	Vector3i previous_region = voxel::chunk_to_region_coords(previous_chunk_pos);
	Vector3i region_delta	 = current_region - previous_region;
	auto sign				 = [](int32_t value) {
		   return value > 0 ? 1 : (value < 0 ? -1 : 0);
	};

	Vector3i primary{ sign(region_delta.x), sign(region_delta.y), sign(region_delta.z) };
	if (primary == Vector3i()) {
		primary = Vector3i(1, 0, 0);
	}

	Vector3i secondary;
	if (ABS(region_delta.x) >= ABS(region_delta.z)) {
		secondary = Vector3i(0, 0, 1);
	} else {
		secondary = Vector3i(1, 0, 0);
	}

	HashSet<Vector3i> desired_regions;
	desired_regions.insert(current_region);
	desired_regions.insert(current_region + primary);
	desired_regions.insert(current_region + secondary);

	for (const Vector3i &region_pos : desired_regions) {
		_queue_region_load(region_pos);
	}

	Vector<Vector3i> to_unload;
	for (const auto &entry : _region_cache) {
		if (!desired_regions.has(entry.key)) {
			to_unload.push_back(entry.key);
		}
	}

	for (const Vector3i &region_pos : to_unload) {
		_unload_region(region_pos);
	}
}

void VoxelAPI::_unload_region(const Vector3i &region_pos) {
	if (!_region_cache.has(region_pos)) {
		return;
	}

	voxel::Region edits = _chunk_repository->take_region_edits(region_pos);
	if (!edits.edited_chunks.is_empty()) {
		_disk_repository->save_region_async(region_pos, edits);
	}

	_region_cache.erase(region_pos);
}
void VoxelAPI::_setup_generation_pipeline(int64_t p_seed) {
	_world_seed = p_seed;
	auto pipeline = std::make_shared<ChunkGenerationPipeline>();
	pipeline->add_pass(std::make_shared<BiomeSelectionPass>());
	pipeline->add_pass(std::make_shared<TerrainSurfacePass>());
	pipeline->add_pass(std::make_shared<CaveCarvingPass>());
	pipeline->add_pass(std::make_shared<OreGenerationPass>());
	pipeline->add_pass(std::make_shared<WaterFillPass>());
	pipeline->add_pass(std::make_shared<TreeGenerationPass>(p_seed));
	pipeline->add_pass(std::make_shared<VegetationGenerationPass>(p_seed));
	_generation_pipeline = std::move(pipeline);
}

void VoxelAPI::_process_meshes(const Vector3i &p_pos) {
	const MeshResultHashSet ready_meshes = _mesh_generator->consume_generated_meshes(_current_chunks_finalize_in_frame);

	for (const MeshResult &result : ready_meshes) {
		const int dist_x = ABS(result.pos.x - p_pos.x);
		const int dist_y = ABS(result.pos.y - p_pos.y);
		const int dist_z = ABS(result.pos.z - p_pos.z);

		if (dist_x > _world_radius + 1 || dist_y > _world_height + 1 || dist_z > _world_radius + 1) {
			continue;
		}

		_finalize_chunk(result);
	}
}

int VoxelAPI::get_initial_loading_ready_chunks() const {
	int ready = 0;
	for (const Vector3i &pos : _initial_loading_chunks) {
		const std::shared_ptr<Chunk> chunk = _chunk_repository->get_chunk(pos);
		if (chunk && chunk->stage == ChunkStage::RENDERED) {
			++ready;
		}
	}
	return ready;
}

float VoxelAPI::get_initial_loading_progress() const {
	const int total = get_initial_loading_total_chunks();
	if (total <= 0) {
		return _is_initializing ? 0.0f : 1.0f;
	}
	return static_cast<float>(get_initial_loading_ready_chunks()) / static_cast<float>(total);
}

void VoxelAPI::_update_initial_loading_status() {
	if (_is_initializing && _pending_region_loads.is_empty() &&
			get_initial_loading_ready_chunks() >= get_initial_loading_total_chunks() &&
			get_initial_loading_total_chunks() > 0) {
		_is_initializing = false;
	}
}

void VoxelAPI::_rebuild_chunk(const Vector3i &pos) const {
	const ChunkNeighbors neighbors = _get_neighbors_for(pos);

	if (!neighbors.center) {
		return;
	}

	const uint64_t version	 = _chunk_repository->get_chunk_version(pos);
	const bool dirty		 = neighbors.center->has_flag(ChunkFlag::DIRTY);
	const bool high_priority = _is_high_priority(pos, dirty);

	neighbors.center->stage = ChunkStage::QUEUED_MESH;
	neighbors.center->remove_flag(ChunkFlag::MESH_DIRTY);

	_mesh_generator->queue_async_generate_mesh(pos, neighbors, version, high_priority);
}

bool VoxelAPI::_is_high_priority(const Vector3i &pos, bool dirty) const {
	const Vector3i player = _last_focos_position;

	const int dx = ABS(pos.x - player.x);
	const int dy = ABS(pos.y - player.y);
	const int dz = ABS(pos.z - player.z);

	const int dist = dx + dy + dz;

	if (dirty)
		return true;
	if (dist < _cache_radius * 1.5)
		return true;

	return false;
}

void VoxelAPI::_clear_world() {
}
void VoxelAPI::_finalize_chunk(const MeshResult &res) {
	std::shared_ptr<Chunk> chunk = _chunk_repository->get_chunk(res.pos);

	// The chunk was unloaded while the mesh was being generated.
	if (!chunk) {
		return;
	}

	// Someone edited the chunk after this mesh was queued;
	// a new mesh generation must have already been (or will be) triggered
	// by _rebuild_chunk. We discard this outdated result.
	if (const uint64_t current_version = _chunk_repository->get_chunk_version(res.pos); current_version != res.version) {
		return;
	}

	if (!_chunk_stream_manager->is_chunk_active(res.pos)) {
		return;
	}

	// Empty chunks have no mesh or collision to install, and are ready for
	// streaming once their empty result has been processed on the main thread.
	if (res.mesh.is_null()) {
		chunk->stage = ChunkStage::RENDERED;
		return;
	}

	if (_rendered_chunks.has(res.pos)) {
		_remove_chunk(_rendered_chunks[res.pos]);
	}

	ChunkNode *chunk_node = _chunk_pool->acquire();

	if (chunk_node == nullptr) {
		WARN_PRINT("ChunkPool is overflow! Increase the prewarm size or check the cleanup.");
		return;
	}

	_rendered_chunks[res.pos] = chunk_node;

	chunk_node->set_mesh(res.mesh);
	chunk_node->set_material_override(Ref<Material>());
	for (int surface = 0; surface < res.mesh->get_surface_count(); ++surface) {
		Array arrays = res.mesh->surface_get_arrays(surface);
		PackedFloat32Array layers = arrays[Mesh::ARRAY_CUSTOM0];
		const bool is_water_surface = !layers.is_empty() && layers[0] >= voxel::WATER_TEXTURE_LAYER;
		chunk_node->set_surface_override_material(surface,
				is_water_surface ? chunk_node->get_water_material() : chunk_node->get_material());
	}
	chunk_node->set_collision_faces(res.collision_faces);
	chunk_node->set_global_position(voxel::chunk_coords_to_world(res.pos));

	chunk->stage = ChunkStage::RENDERED;
}

TerrainSettings VoxelAPI::_make_terrain_settings() const {
    TerrainSettings settings;
    settings.terrain_base_height=_terrain_base_height; settings.terrain_amplitude=_terrain_amplitude;
    settings.water_level=_water_level; settings.world_seed=_world_seed; settings.biome_registry=_biome_registry;
    settings.terrain_noise=_terrain_noise; settings.biome_noise=_biome_noise; settings.dune_noise=_dune_noise;
    settings.mountain_noise=_mountain_noise; settings.ocean_noise=_ocean_noise; settings.river_noise=_river_noise;
    return settings;
}
void VoxelAPI::_queue_async_generate_chunk(const Vector3i p_pos) const {
    const TerrainSettings settings=_make_terrain_settings();

	constexpr bool dirty	 = false;
	const bool high_priority = _is_high_priority(p_pos, dirty);

	_model_generator->_queue_async_generate_chunk_model(p_pos, settings, _generation_pipeline, high_priority);
}

void VoxelAPI::save_world_final() const {
	if (_disk_repository.is_null() || _disk_repository->get_current_world_id() == 0)
		return;

	if (_focus_node && SaveService::get_singleton()) {
		const Vector3 position = _focus_node->get_global_position();
		Array serialized_position;
		serialized_position.push_back(position.x);
		serialized_position.push_back(position.y);
		serialized_position.push_back(position.z);
		Dictionary player_data;
		player_data["position"] = serialized_position;
		player_data["yaw"] = _focus_node->get_rotation().y;
		if (Node *head_node = _focus_node->get_node_or_null("Head")) {
			if (Node3D *head = Object::cast_to<Node3D>(head_node)) {
				player_data["pitch"] = head->get_rotation().x;
			}
		}
		if (!SaveService::get_singleton()->save_world_section(
				_disk_repository->get_current_world_id(), "player", player_data)) {
			WARN_PRINT("Could not save player position.");
		}
	}

	if (_chunk_repository.is_null())
		return;
	HashMap<Vector3i, voxel::Region> all_edits = _chunk_repository->get_all_edited_regions();

	for (const auto &E : all_edits) {
		_disk_repository->save_region(E.key, E.value);
	}
}

ChunkNeighbors VoxelAPI::_get_neighbors_for(const Vector3i p_pos) const {
	ChunkNeighbors n;
	n.center = _chunk_repository->get_chunk(p_pos);
	n.right	 = _chunk_repository->get_chunk(p_pos + voxel::DIR_RIGHT);
	n.left	 = _chunk_repository->get_chunk(p_pos + voxel::DIR_LEFT);
	n.top	 = _chunk_repository->get_chunk(p_pos + voxel::DIR_UP);
	n.bottom = _chunk_repository->get_chunk(p_pos + voxel::DIR_DOWN);
	n.front	 = _chunk_repository->get_chunk(p_pos + voxel::DIR_FRONT);
	n.back	 = _chunk_repository->get_chunk(p_pos + voxel::DIR_BACK);
	return n;
}

void VoxelAPI::_try_build_mesh_with_neighbors(const Vector3i p_pos) const {
	std::shared_ptr<Chunk> chunk = _chunk_repository->get_chunk(p_pos);

	if (!chunk) {
		return;
	}

	if (chunk->stage == ChunkStage::QUEUED_MESH || chunk->stage == ChunkStage::GENERATING_MESH) {
		return;
	}

	const ChunkNeighbors neighbors = _get_neighbors_for(p_pos);

	if (!neighbors.center) {
		return;
	}

	// Outside the finite vertical world there is air, not a pending neighbour.
	const bool missing_top = !neighbors.top && p_pos.y < WORLD_MAX_CHUNK_Y;
	const bool missing_bottom = !neighbors.bottom && p_pos.y > WORLD_MIN_CHUNK_Y;
	if (!neighbors.right || !neighbors.left || missing_top || missing_bottom || !neighbors.front || !neighbors.back) {
		chunk->stage = ChunkStage::WAITING_NEIGHBORS;
		return;
	}

	chunk->stage = ChunkStage::QUEUED_MESH;
	_mesh_generator->queue_async_generate_mesh(p_pos, neighbors, _chunk_repository->get_chunk_version(p_pos));
}

void VoxelAPI::save_world() const {
	save_world_final();
}

void VoxelAPI::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_biome_registry_path", "path"), &VoxelAPI::set_biome_registry_path);
    ClassDB::bind_method(D_METHOD("get_biome_registry_path"), &VoxelAPI::get_biome_registry_path);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "biome_registry_path", PROPERTY_HINT_FILE, "*.json"), "set_biome_registry_path", "get_biome_registry_path");
    ClassDB::bind_method(D_METHOD("sample_terrain_column", "position"), &VoxelAPI::sample_terrain_column);
    ClassDB::bind_static_method("VoxelAPI", D_METHOD("validate_biome_registry", "data"), &VoxelAPI::validate_biome_registry);
	ClassDB::bind_method(D_METHOD("set_focus_node", "node"), &VoxelAPI::set_focus_node);
	ClassDB::bind_method(D_METHOD("set_focus_position", "pos"), &VoxelAPI::set_focus_position);
	ClassDB::bind_method(D_METHOD("break_block", "world_pos"), &VoxelAPI::break_block);
	ClassDB::bind_method(D_METHOD("set_block", "world_pos", "block"), &VoxelAPI::set_block);
	ClassDB::bind_method(D_METHOD("is_water_at", "world_pos"), &VoxelAPI::is_water_at);
	ClassDB::bind_method(D_METHOD("is_ocean_at", "world_pos"), &VoxelAPI::is_ocean_at);
	ClassDB::bind_method(D_METHOD("get_block_type_at", "world_pos"), &VoxelAPI::get_block_type_at);
	ClassDB::bind_method(D_METHOD("save_world"), &VoxelAPI::save_world);
	ClassDB::bind_method(D_METHOD("start_world", "id"), &VoxelAPI::start_world);
	ClassDB::bind_method(D_METHOD("is_initial_loading"), &VoxelAPI::is_initial_loading);
	ClassDB::bind_method(D_METHOD("get_initial_loading_total_chunks"), &VoxelAPI::get_initial_loading_total_chunks);
	ClassDB::bind_method(D_METHOD("get_initial_loading_ready_chunks"), &VoxelAPI::get_initial_loading_ready_chunks);
	ClassDB::bind_method(D_METHOD("get_initial_loading_progress"), &VoxelAPI::get_initial_loading_progress);
	ClassDB::bind_method(D_METHOD("set_render_settings", "settings"), &VoxelAPI::set_render_settings);
	ClassDB::bind_method(D_METHOD("get_render_settings"), &VoxelAPI::get_render_settings);
	ClassDB::bind_static_method(VoxelAPI::get_class_static(), D_METHOD("get_default_render_settings"), &VoxelAPI::get_default_render_settings);
	ClassDB::bind_static_method(VoxelAPI::get_class_static(), D_METHOD("set_default_render_settings", "settings"), &VoxelAPI::set_default_render_settings);
}
} // namespace godot

#include "world.h"
#include <godot_cpp/classes/resource_uid.hpp>
#include "ChunkDiskRepository.h"
#include "chunk_pool.h"
#include "utils.h"
#include <vector>

namespace godot {
void World::_ready() {
	_last_focos_position = Vector3i();

	_chunk_pool.instantiate();
	_chunk_repository.instantiate();
	_chunk_stream_manager.instantiate();
	_model_generator.instantiate();
	_mesh_generator.instantiate();
	_disk_repository.instantiate();
	_region_loader.instantiate();

	_region_loader->set_repository(_disk_repository);

	StreamSettings stream_settings{};
	stream_settings.cache_radius = _cache_radius;
	stream_settings.world_height = _world_height;
	stream_settings.world_radius = _world_radius;

	_chunk_stream_manager->set_stream_settings(stream_settings);
	_chunk_pool->set_owner(this);
	_chunk_pool->set_prewarm(_prewarm_chunk_pool);

	set_process(true);
	_setup_noises();
}

void World::_setup_noises() {
	_terrain_noise.instantiate();
	_terrain_noise->set_noise_type(FastNoiseLite::TYPE_PERLIN);
	_terrain_noise->set_frequency(0.035);
	_terrain_noise->set_fractal_octaves(5);

	_cave_noise.instantiate();
	_cave_noise->set_noise_type(FastNoiseLite::TYPE_PERLIN);
	_cave_noise->set_frequency(0.02);
}

void World::_init_chunks() {
	ERR_FAIL_COND(_chunk_pool.is_null());

	_is_initializing = true;

	_chunk_pool->set_prewarm(_prewarm_chunk_pool);

	if (!_focus_node) {
		_last_focos_position = voxel::block_to_chunk_coords(Vector3());
	} else {
		_last_focos_position = voxel::block_to_chunk_coords(_get_current_focus_position());
	}

	_previous_player_chunk_pos = _last_focos_position;
	_chunk_stream_manager->rebuild_all_chunks(_last_focos_position);

	Vector3i current_region = voxel::chunk_to_region_coords(_last_focos_position);
	for (int x = -1; x <= 1; ++x) {
		for (int z = -1; z <= 1; ++z) {
			_queue_region_load(current_region + Vector3i(x, 0, z));
		}
	}

	Vector<Vector3i> saved_regions = _disk_repository->get_all_saved_regions();
	for (const Vector3i &region_pos : saved_regions) {
		_queue_region_load(region_pos);
	}

}
void World::_remove_chunk(ChunkNode *p_chunk_node) {
	ERR_FAIL_NULL(p_chunk_node);

	const Vector3i pos = voxel::block_to_chunk_coords(p_chunk_node->get_global_position());
	_rendered_chunks.erase(pos);
	_chunk_pool->release(p_chunk_node);
}

void World::_update_visible_chunks() {
	if (_is_initializing) {

		if (_pending_region_loads.is_empty()) {
			_is_initializing = false;
		} else {
			return;
		}
	}
	for (const Vector3i &pos : _chunk_stream_manager->pop_queue_free_chunks()) {
		if (_rendered_chunks.has(pos)) {
			_remove_chunk(_rendered_chunks[pos]);
		}
	}

	for (const Vector3i &pos : _chunk_stream_manager->get_active_chunks_snapshot()) {
		if (_chunk_repository->contains_chunk(pos) || _model_generator->is_loading_chunk(pos)) {
			if (_chunk_repository->contains_chunk(pos) && !_rendered_chunks.has(pos)) {
				_try_build_mesh_with_neighbors(pos);
			}
			continue;
		}

		_queue_async_generate_chunk(pos);
	}
}

void World::_cleanup_far_chunks() {
	std::vector<Vector3i> to_remove;

	const int cache_radius_sq = _cache_radius * _cache_radius;

	for (const auto pos : _chunk_repository->get_keys_snapshot()) {
		const int dx = ABS(pos.x - _last_focos_position.x);
		const int dy = ABS(pos.y - _last_focos_position.y);
		const int dz = ABS(pos.z - _last_focos_position.z);

		bool out_of_vertical_bounds   = dy > (_world_height + 2);
		bool out_of_horizontal_bounds = (dx * dx + dz * dz) > cache_radius_sq;

		if (out_of_vertical_bounds || out_of_horizontal_bounds) {
			if (!_mesh_generator->is_queued_mesh(pos) && !_model_generator->is_loading_chunk(pos)) {
				to_remove.push_back(pos);
			}
		}
	}

	for (const Vector3i &pos : to_remove) {
		_chunk_repository->remove_chunk(pos);
	}

}

void World::_process(double delta) {
	const Vector3i current_position = voxel::block_to_chunk_coords(_get_current_focus_position());

	if (current_position != _last_focos_position) {
		_chunk_stream_manager->shift_chunks(current_position);
		_update_region_streaming(current_position, _last_focos_position);
		_previous_player_chunk_pos = _last_focos_position;
		_last_focos_position = current_position;
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

void World::_exit_tree() {
	save_world_final();
}

void World::break_block(const Vector3 &world_pos) {
	Vector3i block_pos = voxel::world_to_block(world_pos);
	_chunk_repository->set_block(block_pos, 0);
}

void World::set_block(const Vector3 &p_world_pos, const voxel::Block &p_block) const {
	Vector3i block_pos = voxel::world_to_block(p_world_pos);
	_chunk_repository->set_block(block_pos, p_block);
}

void World::set_focus_node(Node3D *p_node) {
	_focus_node     = p_node;
	_use_manual_pos = false;
}

void World::set_focus_position(Vector3 p_pos) {
	_focus_manual_pos = p_pos;
	_use_manual_pos   = true;
}

void World::create_new_world(int32_t p_seed, const String &p_name) {
	_chunk_repository->clear_all();
	_chunk_pool->clear();
	_rendered_chunks.clear();
	_region_cache.clear();
	_pending_region_loads.clear();

	const voxel::WorldModel world_model{
		.seed = p_seed,
		.name = p_name,
		.id = ResourceUID::get_singleton()->create_id()
	};

	_chunk_repository->set_world_model(world_model);

	_disk_repository->set_current_world(world_model.id);
	_disk_repository->save_world_model(world_model);

	_terrain_noise->set_seed(world_model.seed);
	_cave_noise->set_seed(world_model.seed + 1);

	_init_chunks();
}

void World::load_world(uint64_t p_id) {
	_chunk_repository->clear_all();
	_chunk_pool->clear();
	_rendered_chunks.clear();
	_region_cache.clear();
	_pending_region_loads.clear();

	_disk_repository->set_current_world(p_id);
	const voxel::WorldModel world_model = _disk_repository->load_world_model(p_id);

	_chunk_repository->set_world_model(world_model);

	_terrain_noise->set_seed(world_model.seed);
	_cave_noise->set_seed(world_model.seed + 1);

	_init_chunks();
}

void World::save_world() {
	if (_disk_repository.is_valid() && _disk_repository->get_current_world_id() != 0) {
		_chunk_repository->save_edited_chunks_to_disk(_disk_repository);
	}
}

void World::delete_world(uint64_t p_id) {
	if (_disk_repository.is_valid()) {
		_disk_repository->delete_world(p_id);
	}
}

PackedInt64Array World::get_saved_worlds() const {
	PackedInt64Array result;

	if (_disk_repository.is_valid()) {
		HashSet<int64_t> worlds = _disk_repository->get_saved_worlds();
		for (const int64_t &id : worlds) {
			result.append(id);
		}
	}

	return result;
}

Vector3 World::_get_current_focus_position() const {
	if (!_use_manual_pos && _focus_node) {
		return _focus_node->get_global_position();
	}
	return _focus_manual_pos;
}

void World::_process_models() {
	HashMap<Vector3i, std::shared_ptr<ChunkModel>> ready_models = _model_generator->consume_generated_results();

	if (ready_models.is_empty()) {
		return;
	}

	for (auto &ready_model : ready_models) {
		_queue_region_load(voxel::chunk_to_region_coords(ready_model.key));
		_chunk_repository->add_chunk(ready_model.key, ready_model.value);
	}
}

void World::_ensure_region_loaded_for_chunk(const Vector3i &chunk_pos) {
	if (_disk_repository.is_null()) {
		return;
	}

	Vector3i region_pos = voxel::chunk_to_region_coords(chunk_pos);
	_queue_region_load(region_pos);
}

void World::_queue_region_load(const Vector3i &region_pos) {
	if (_region_cache.has(region_pos) || _pending_region_loads.has(region_pos)) {
		return;
	}

	_pending_region_loads.insert(region_pos);
	_region_loader->queue_async_load_region(region_pos);
}

void World::_process_loaded_regions() {
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

void World::_update_region_streaming(const Vector3i &current_chunk_pos, const Vector3i &previous_chunk_pos) {
	Vector3i current_region = voxel::chunk_to_region_coords(current_chunk_pos);
	Vector3i previous_region = voxel::chunk_to_region_coords(previous_chunk_pos);
	Vector3i region_delta = current_region - previous_region;
	auto sign = [](int32_t value) {
		return value > 0 ? 1 : (value < 0 ? -1 : 0);
	};

	Vector3i primary{sign(region_delta.x), sign(region_delta.y), sign(region_delta.z)};
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

void World::_unload_region(const Vector3i &region_pos) {
	if (!_region_cache.has(region_pos)) {
		return;
	}

	voxel::Region edits = _chunk_repository->take_region_edits(region_pos);
	if (!edits.edited_chunks.is_empty()) {
		_disk_repository->save_region_async(region_pos, edits);
	}

	_region_cache.erase(region_pos);
}

void World::_process_meshes(const Vector3i &p_pos) {
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

void World::_rebuild_chunk(const Vector3i &pos) const {
	const ChunkNeighbors neighbors = _get_neighbors_for(pos);

	if (!neighbors.center) {
		return;
	}

	const uint64_t version   = _chunk_repository->get_chunk_version(pos);
	const bool dirty         = _chunk_repository->is_chunk_dirty(pos);
	const bool high_priority = _is_high_priority(pos, dirty);

	_mesh_generator->queue_async_generate_mesh(pos, neighbors, version, high_priority);
}

bool World::_is_high_priority(const Vector3i &pos, bool dirty) const {
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

void World::_finalize_chunk(const MeshResult &res) {
	if (res.mesh.is_null()) {
		return;
	}

	uint64_t current_version = _chunk_repository->get_chunk_version(res.pos);

	if (current_version != res.version) {
		return;
	}

	if (!_chunk_stream_manager->is_chunk_active(res.pos)) {
		return;
	}

	if (_rendered_chunks.has(res.pos)) {
		_remove_chunk(_rendered_chunks[res.pos]);
	}

	ChunkNode *chunk = _chunk_pool->acquire();

	if (chunk == nullptr) {
		WARN_PRINT("ChunkPool is overflow! Increase the prewarm size or check the cleanup.");
		return;
	}

	_rendered_chunks[res.pos] = chunk;

	chunk->set_mesh(res.mesh);
	chunk->set_surface_override_material(0, chunk->get_material());
	chunk->set_collision_faces(res.collision_faces);
	chunk->set_global_position(voxel::chunk_coords_to_world(res.pos));
}

void World::_queue_async_generate_chunk(const Vector3i p_pos) const {
	TerrainSettings settings;
	settings.terrain_base_height     = _terrain_base_height;
	settings.terrain_amplitude       = _terrain_amplitude;
	settings.cave_threshold          = 0.1f;
	settings.noise_set.terrain_noise = _terrain_noise;
	settings.noise_set.cave_noise    = _cave_noise;

	constexpr bool dirty     = false;
	const bool high_priority = _is_high_priority(p_pos, dirty);

	_model_generator->_queue_async_generate_chunk_model(p_pos, settings, high_priority);
}

void World::save_world_final() {
	if (_disk_repository.is_null()) return;

	HashMap<Vector3i, voxel::Region> all_edits = _chunk_repository->get_all_edited_regions();

	for (const auto &E : all_edits) {
		_disk_repository->save_region(E.key, E.value);
	}
}

ChunkNeighbors World::_get_neighbors_for(const Vector3i p_pos) const {
	ChunkNeighbors n;
	n.center = _chunk_repository->get_chunk(p_pos);
	n.right  = _chunk_repository->get_chunk(p_pos + voxel::DIR_RIGHT);
	n.left   = _chunk_repository->get_chunk(p_pos + voxel::DIR_LEFT);
	n.top    = _chunk_repository->get_chunk(p_pos + voxel::DIR_UP);
	n.bottom = _chunk_repository->get_chunk(p_pos + voxel::DIR_DOWN);
	n.front  = _chunk_repository->get_chunk(p_pos + voxel::DIR_FRONT);
	n.back   = _chunk_repository->get_chunk(p_pos + voxel::DIR_BACK);
	return n;
}

void World::_try_build_mesh_with_neighbors(const Vector3i p_pos) const {
	if (_mesh_generator->is_queued_mesh(p_pos)) {
		return;
	}

	const ChunkNeighbors neighbors = _get_neighbors_for(p_pos);

	if (!neighbors.center) {
		return;
	}

	if (!neighbors.right || !neighbors.left || !neighbors.top || !neighbors.bottom || !neighbors.front || !neighbors.back) {
		return;
	}

	_mesh_generator->queue_async_generate_mesh(p_pos, neighbors, _chunk_repository->get_chunk_version(p_pos));
}

void World::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_focus_node", "node"), &World::set_focus_node);
	ClassDB::bind_method(D_METHOD("set_focus_position", "pos"), &World::set_focus_position);
	ClassDB::bind_method(D_METHOD("break_block", "world_pos"), &World::break_block);
	ClassDB::bind_method(D_METHOD("set_block", "world_pos", "block"), &World::set_block);

	ClassDB::bind_method(D_METHOD("create_new_world", "seed", "name"), &World::create_new_world);
	ClassDB::bind_method(D_METHOD("load_world", "id"), &World::load_world);
	ClassDB::bind_method(D_METHOD("save_world"), &World::save_world);
	ClassDB::bind_method(D_METHOD("delete_world", "id"), &World::delete_world);
	ClassDB::bind_method(D_METHOD("get_saved_worlds"), &World::get_saved_worlds);
}
} // namespace godot
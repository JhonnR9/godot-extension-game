#include "world.h"
#include <godot_cpp/classes/resource_uid.hpp>
#include "ChunkDiskRepository.h"
#include "chunk_pool.h"
#include "utils.h"

namespace godot {
void World::_ready() {
	_last_focos_position = Vector3i();

	_chunk_pool.instantiate();
	_chunk_repository.instantiate();
	_chunk_stream_manager.instantiate();
	_model_generator.instantiate();
	_mesh_generator.instantiate();

	StreamSettings stream_settings{};
	stream_settings.cache_radius = _cache_radius;
	stream_settings.world_height = _world_height;
	stream_settings.world_radius = _world_radius;

	_chunk_stream_manager->set_stream_settings(stream_settings);
	_chunk_pool->set_owner(this);

	set_process(true);
	_setup_noises();

	_init_chunks();

	Ref<ChunkDiskRepository> chunk_disk;
	chunk_disk.instantiate();

	chunk_disk->save_test();
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

	_chunk_pool->set_prewarm(_prewarm_chunk_pool);

	if (!_focus_node) {
		_last_focos_position = voxel::block_to_chunk_coords(Vector3()); // fallback
		_chunk_stream_manager->rebuild_all_chunks(_last_focos_position);
		return;
	}

	_last_focos_position       = voxel::block_to_chunk_coords(_get_current_focus_position());
	_previous_player_chunk_pos = _last_focos_position;
	_chunk_stream_manager->rebuild_all_chunks(_last_focos_position);
}

void World::_remove_chunk(ChunkNode *p_chunk_node) {
	ERR_FAIL_NULL(p_chunk_node);

	const Vector3i pos = voxel::block_to_chunk_coords(p_chunk_node->get_global_position());
	_rendered_chunks.erase(pos);
	_chunk_pool->release(p_chunk_node);
}

void World::_update_visible_chunks() {
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

void World::_cleanup_far_chunks() const {
	std::vector<Vector3i> to_remove;

	const int cache_radius_sq = _cache_radius * _cache_radius;

	for (const auto pos : _chunk_repository->get_keys_snapshot()) {
		const int dx = ABS(pos.x - _last_focos_position.x);
		const int dy = ABS(pos.y - _last_focos_position.y);
		const int dz = ABS(pos.z - _last_focos_position.z);

		bool out_of_vertical_bounds = dy > (_world_height + 2);
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
		_last_focos_position       = current_position;
		_previous_player_chunk_pos = current_position;

		_chunk_stream_manager->shift_chunks(current_position);
		_update_visible_chunks();
	}

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

void World::break_block(const Vector3 &world_pos) const {
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

	const WorldModel world_model{
		.id =  ResourceUID::get_singleton()->create_id(),
		.name = p_name,
		.seed = p_seed

	};

	_terrain_noise->set_seed(world_model.seed);
	_cave_noise->set_seed(world_model.seed + 1);

	_init_chunks();
}

void World::load_world(uint64_t p_id) {
	_chunk_repository->clear_all();
	_chunk_pool->clear();
	_rendered_chunks.clear();

	const WorldModel world_model = _chunk_repository->get_world_model(p_id);

	_terrain_noise->set_seed(world_model.seed);
	_cave_noise->set_seed(world_model.seed + 1);

	_init_chunks();
}

HashSet<int64_t> World::get_saved_worlds() const {
	return  _chunk_repository->get_saved_worlds();
}

Vector3 World::_get_current_focus_position() const {
	if (!_use_manual_pos && _focus_node) {
		return _focus_node->get_global_position();
	}
	return _focus_manual_pos;
}

void World::_process_models() const {
	HashMap<Vector3i, std::shared_ptr<ChunkModel>> ready_models = _model_generator->consume_generated_results();

	if (ready_models.is_empty()) {
		return;
	}

	for (auto &ready_model : ready_models) {
		_chunk_repository->add_chunk(ready_model.key, ready_model.value);
	}
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
}
} // namespace godot
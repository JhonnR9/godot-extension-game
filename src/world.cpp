#include "world.h"

#include "chunk_pool.h"


namespace godot {
void World::_ready() {
	_last_player_chunk_pos = Vector3i(INT_MAX, INT_MAX, INT_MAX);

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

	_player_node = get_node<Node3D>("../Player");
	if (!_player_node) {
		_last_player_chunk_pos     = _world_to_chunk_pos(Vector3()); // fallback
		_chunk_stream_manager->rebuild_all_chunks(_last_player_chunk_pos);
		return;
	}


	_last_player_chunk_pos     = _world_to_chunk_pos(_player_node->get_global_position());
	_previous_player_chunk_pos = _last_player_chunk_pos;
	_chunk_stream_manager->rebuild_all_chunks(_last_player_chunk_pos);
}

bool World::_did_player_change_chunk() const {
	if (!_player_node) {
		return false;
	}
	const Vector3i current_chunk_position = _world_to_chunk_pos(_player_node->get_global_position());
	return current_chunk_position != _last_player_chunk_pos;
}

void World::_remove_chunk(ChunkNode *p_chunk_node) {
	ERR_FAIL_NULL(p_chunk_node);

	const Vector3i pos = _world_to_chunk_pos(p_chunk_node->get_global_position());
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

	for (const auto pos : _chunk_repository->get_keys_snapshot()) {
		const int dx = ABS(pos.x - _last_player_chunk_pos.x);
		const int dy = ABS(pos.y - _last_player_chunk_pos.y);
		const int dz = ABS(pos.z - _last_player_chunk_pos.z);

		if (dx > _cache_radius || dy > _world_height + 2 || dz > _cache_radius) {
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
	Vector3i current_position = Vector3();
	if (_player_node) {
		current_position = _world_to_chunk_pos(_player_node->get_global_position());
		if (_did_player_change_chunk()) {
			_last_player_chunk_pos     = current_position;
			_previous_player_chunk_pos = current_position;

			_chunk_stream_manager->shift_chunks(current_position);

			_update_visible_chunks();
		}
	}


	float frame_ms = delta * 1000.0f;
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

void World::break_block(const Vector3 &world_pos) {
	Vector3i block_pos = Vector3i(
		Math::floor(world_pos.x),
		Math::floor(world_pos.y),
		Math::floor(world_pos.z)
	);
	_chunk_repository->set_block(block_pos, 0);
}

void World::set_block(const Vector3 &p_world_pos, const block::Block &p_block) {
	Vector3i block_pos = Vector3i(
		Math::floor(p_world_pos.x),
		Math::floor(p_world_pos.y),
		Math::floor(p_world_pos.z)
	);
	_chunk_repository->set_block(block_pos, p_block);
}

void World::_process_models() {
	HashMap<Vector3i, std::shared_ptr<ChunkModel>> ready_models = _model_generator->consume_generated_results();

	if (ready_models.is_empty()) {
		return;
	}

	for (auto &ready_model : ready_models) {
		_chunk_repository->add_chunk(ready_model.key, ready_model.value);
	}

	for (auto it = ready_models.begin(); it != ready_models.end(); ++it) {
		Vector3i pos = it->key;

		_try_build_mesh_with_neighbors(pos);

		Vector3i dirs[] = {
			{ 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 }
		};

		for (const Vector3i &dir : dirs) {
			Vector3i neighbor_pos = pos + dir;

			if (_chunk_repository->contains_chunk(neighbor_pos)) {
				_try_build_mesh_with_neighbors(neighbor_pos);
			}
		}
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

void World::_rebuild_chunk(const Vector3i &pos) {
	const ChunkNeighbors neighbors = _get_neighbors_for(pos);

	if (!neighbors.center) {
		return;
	}
	uint64_t version = _chunk_repository->get_chunk_version(pos);

	bool dirty = _chunk_repository->is_chunk_dirty(pos);

	bool high_priority = _is_high_priority(pos, dirty);

	_mesh_generator->queue_async_generate_mesh(
		pos,
		neighbors,
		version,
		high_priority
	);
}

bool World::_is_high_priority(const Vector3i &pos, bool dirty) {
	const Vector3i player = _last_player_chunk_pos;

	int dx = ABS(pos.x - player.x);
	int dy = ABS(pos.y - player.y);
	int dz = ABS(pos.z - player.z);

	int dist = dx + dy + dz;

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

	ChunkNode *chunk          = _chunk_pool->acquire();
	_rendered_chunks[res.pos] = chunk;

	chunk->set_mesh(res.mesh);
	chunk->set_surface_override_material(0, chunk->get_material());
	chunk->set_collision_faces(res.collision_faces);
	chunk->set_global_position(Vector3(res.pos.x * ChunkModel::SIZE_X, res.pos.y * ChunkModel::SIZE_Y, res.pos.z * ChunkModel::SIZE_Z));
	chunk->set_visible(true);
}

Vector3i World::_world_to_chunk_pos(const Vector3 p_pos) {
	return Vector3i(
			Math::floor(p_pos.x / ChunkModel::SIZE_X),
			Math::floor(p_pos.y / ChunkModel::SIZE_Y),
			Math::floor(p_pos.z / ChunkModel::SIZE_Z)
			);
}

void World::_queue_async_generate_chunk(const Vector3i p_pos) {
	TerrainSettings settings;
	settings.terrain_base_height     = _terrain_base_height;
	settings.terrain_amplitude       = _terrain_amplitude;
	settings.cave_threshold          = 0.1f;
	settings.noise_set.terrain_noise = _terrain_noise;
	settings.noise_set.cave_noise    = _cave_noise;

	bool dirty = false;

	bool high_priority = _is_high_priority(p_pos, dirty);

	_model_generator->_queue_async_generate_chunk_model(p_pos, settings, high_priority);
}

ChunkNeighbors World::_get_neighbors_for(const Vector3i p_pos) {
	ChunkNeighbors n;
	n.center = _chunk_repository->get_chunk(p_pos);
	n.right  = _chunk_repository->get_chunk(p_pos + Vector3i(1, 0, 0));
	n.left   = _chunk_repository->get_chunk(p_pos + Vector3i(-1, 0, 0));
	n.top    = _chunk_repository->get_chunk(p_pos + Vector3i(0, 1, 0));
	n.bottom = _chunk_repository->get_chunk(p_pos + Vector3i(0, -1, 0));
	n.front  = _chunk_repository->get_chunk(p_pos + Vector3i(0, 0, 1));
	n.back   = _chunk_repository->get_chunk(p_pos + Vector3i(0, 0, -1));
	return n;
}

void World::_try_build_mesh_with_neighbors(const Vector3i p_pos) {
	if (_mesh_generator->is_queued_mesh(p_pos)) {
		return;
	}

	const ChunkNeighbors neighbors = _get_neighbors_for(p_pos);

	if (!neighbors.center) {
		return;
	}

	if (!neighbors.center || !neighbors.right || !neighbors.left ||
		!neighbors.top || !neighbors.bottom || !neighbors.front || !neighbors.back) {
		return;
	}

	_mesh_generator->queue_async_generate_mesh(p_pos, neighbors, _chunk_repository->get_chunk_version(p_pos));
}

void World::_bind_methods() {
}
} // namespace godot
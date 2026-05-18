#include "world.h"

#include "atlas_loader.h"
#include "chunk_pool.h"

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

void World::_ready() {
	_last_player_chunk_pos = Vector3i(INT_MAX, INT_MAX, INT_MAX);
	load_atlas("res://sprites/atlas.json");
	_chunk_pool.instantiate();
	_chunk_repository.instantiate();

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

	ERR_FAIL_NULL(_player_node);
	_last_player_chunk_pos = _world_to_chunk_pos(_player_node->get_global_position());
	_previous_player_chunk_pos = _last_player_chunk_pos;
	_rebuild_all_chunks();
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

	const Vector3i pos = _world_to_chunk_pos(
			p_chunk_node->get_global_position());

	_last_active_node_chunks.erase(pos);

	p_chunk_node->disable();


	_chunk_pool->release(p_chunk_node);


}

void World::_shift_chunks() {
	std::lock_guard<std::mutex> lock(_active_chunks_mutex);

	const int radius_xz = _world_radius;
	const int radius_y = _world_height;

	HashSet<Vector3i> new_active_chunks;
	const int estimated_size = (2 * radius_xz + 1) * (2 * radius_y + 1) * (2 * radius_xz + 1);
	new_active_chunks.reserve(estimated_size);

	for (int x = -radius_xz; x <= radius_xz; x++) {
		for (int y = -radius_y; y <= radius_y; y++) {
			for (int z = -radius_xz; z <= radius_xz; z++) {
				Vector3i pos{
					_last_player_chunk_pos.x + x,
					_last_player_chunk_pos.y + y,
					_last_player_chunk_pos.z + z
				};
				new_active_chunks.insert(pos);
			}
		}
	}

	std::vector<Vector3i> chunks_to_remove;
	for (const Vector3i &pos : _active_chunks) {
		if (!new_active_chunks.has(pos)) {
			chunks_to_remove.push_back(pos);
		}
	}

	for (const Vector3i &pos : chunks_to_remove) {
		_active_chunks.erase(pos);
		if (_last_active_node_chunks.has(pos)) {
			_remove_chunk(_last_active_node_chunks[pos]);
		}
	}

	_active_chunks = std::move(new_active_chunks);

	HashSet<Vector3i> loading_chunks_copy;
	{
		std::lock_guard<std::mutex> lock2(_loading_chunks_mutex);
		loading_chunks_copy = _loading_chunks;
	}

	for (const Vector3i &pos : _active_chunks) {
		if (!_last_active_node_chunks.has(pos) && !loading_chunks_copy.has(pos)) {
			_queue_async_generate_chunk(pos);
		}
	}
}

void World::_cleanup_far_chunks() {
	std::vector<Vector3i> to_remove;

	for (const auto pos : _chunk_repository->get_keys_snapshot()){

		const int dx = ABS(pos.x - _last_player_chunk_pos.x);
		const int dy = ABS(pos.y - _last_player_chunk_pos.y);
		const int dz = ABS(pos.z - _last_player_chunk_pos.z);

		if (dx > _cache_radius ||dy > _world_height + 2 ||dz > _cache_radius) {
			to_remove.push_back(pos);
		}
	}

	for (const Vector3i& pos : to_remove) {
		_chunk_repository->remove_chunk(pos);
	}
}

void World::_rebuild_all_chunks() {
	const int size = (2 * _world_radius + 1) * (2 * _world_height + 1) * (2 * _world_radius + 1);
	_active_chunks.reserve(size);

	for (int x = -_world_radius; x <= _world_radius; x++) {
		for (int y = -_world_height; y <= _world_height; y++) {
			for (int z = -_world_radius; z <= _world_radius; z++) {
				Vector3i pos{
					_last_player_chunk_pos.x + x,
					_last_player_chunk_pos.y + y,
					_last_player_chunk_pos.z + z
				};

				_active_chunks.insert(pos);

				_queue_async_generate_chunk(pos);
			}
		}
	}
}

bool World::_is_chunk_active(const Vector3i &pos) {
	std::lock_guard<std::mutex> lock(_active_chunks_mutex);
	return _active_chunks.has(pos);
}

void World::_process(double delta) {
	if (!_player_node) {
		return;
	}




	if (_did_player_change_chunk()) {
		Vector3i current = _world_to_chunk_pos(_player_node->get_global_position());

		_last_player_chunk_pos = current;
		_previous_player_chunk_pos = current;

		_shift_chunks();
	}

	const Vector3i player_chunk =
			_world_to_chunk_pos(_player_node->get_global_position());

	std::vector<ChunkGenerationResult> pending_results_to_process;

	{
		std::lock_guard<std::mutex> lock(_pending_results_mutex);

		const int max_chunks_per_frame = _max_chunk_finalize_per_frame;

		const int amount = MIN(
				max_chunks_per_frame,
				static_cast<int>(_pending_results.size()));

		pending_results_to_process.reserve(amount);

		for (int i = 0; i < amount; i++) {
			pending_results_to_process.push_back(
					std::move(_pending_results[i]));
		}

		_pending_results.erase(
				_pending_results.begin(),
				_pending_results.begin() + amount);
	}

	for (const ChunkGenerationResult &result : pending_results_to_process) {
		const int dist_x = ABS(result.pos.x - player_chunk.x);
		const int dist_y = ABS(result.pos.y - player_chunk.y);
		const int dist_z = ABS(result.pos.z - player_chunk.z);

		if (
				dist_x > _world_radius + 1 ||
				dist_y > _world_height + 1 ||
				dist_z > _world_radius + 1) {
			std::lock_guard<std::mutex> lock(_loading_chunks_mutex);
			_loading_chunks.erase(result.pos);
			continue;
		}

		if (!_is_chunk_active(result.pos)) {
			std::lock_guard<std::mutex> lock(_loading_chunks_mutex);
			_loading_chunks.erase(result.pos);
			continue;
		}

		_finalize_chunk(result);

		{
			std::lock_guard<std::mutex> lock(_loading_chunks_mutex);
			_loading_chunks.erase(result.pos);
		}
	}

	// Cleanup
	static double cleanup_timer = 0.0;
	cleanup_timer += delta;

	if (cleanup_timer > 2.0) {
		_cleanup_far_chunks();
		cleanup_timer = 0.0;
	}
}

void World::_finalize_chunk(const ChunkGenerationResult &res) {
	if (res.mesh.is_null()) {
		return;
	}

	if (!_is_chunk_active(res.pos)) {
		return;
	}

	if (_last_active_node_chunks.has(res.pos)) {
		_remove_chunk(_last_active_node_chunks[res.pos]);
	}

	ChunkNode *chunk = _chunk_pool->acquire();

	_last_active_node_chunks[res.pos] = chunk;

	chunk->set_mesh(res.mesh);

	chunk->set_surface_override_material(
			0,
			chunk->get_material());

	chunk->set_collision_faces(res.collision_faces);

	chunk->set_global_position(
			Vector3(
					res.pos.x * ChunkModel::SIZE,
					res.pos.y * ChunkModel::SIZE,
					res.pos.z * ChunkModel::SIZE));

	chunk->set_visible(true);
}

Vector3i World::_world_to_chunk_pos(Vector3 p_pos) {
	return Vector3i(
			Math::floor(p_pos.x / ChunkModel::SIZE),
			Math::floor(p_pos.y / ChunkModel::SIZE),
			Math::floor(p_pos.z / ChunkModel::SIZE));
}

void World::_queue_async_generate_chunk(const Vector3i p_pos) {
	{
		std::lock_guard<std::mutex> lock(_loading_chunks_mutex);
		_loading_chunks.insert(p_pos);
	}

	const Callable action = Callable(this, "_thread_work").bind(p_pos);
	WorkerThreadPool::get_singleton()->add_task(action);
}

ChunkNeighbors World::_get_neighbors_for(Vector3i p_pos) {
	ChunkNeighbors n;

	n.center = _chunk_repository->get_chunk(p_pos);

	n.right = _chunk_repository->get_chunk(p_pos + Vector3i(1, 0, 0));
	n.left = _chunk_repository->get_chunk(p_pos + Vector3i(-1, 0, 0));

	n.top = _chunk_repository->get_chunk(p_pos + Vector3i(0, 1, 0));
	n.bottom = _chunk_repository->get_chunk(p_pos + Vector3i(0, -1, 0));

	n.front = _chunk_repository->get_chunk(p_pos + Vector3i(0, 0, 1));
	n.back = _chunk_repository->get_chunk(p_pos + Vector3i(0, 0, -1));

	return n;
}

void World::_thread_work(Vector3i p_pos) {
	TerrainSettings settings;
	settings.terrain_base_height = _terrain_base_height;
	settings.terrain_amplitude = _terrain_amplitude;
	settings.cave_threshold = 0.1f;

	settings.noise_set.terrain_noise = _terrain_noise;
	settings.noise_set.cave_noise = _cave_noise;

	{
		const auto model = std::make_shared<ChunkModel>(ChunkGenerator::generate(p_pos, settings));
		_chunk_repository->add_chunk(p_pos, model);
	}

	_try_build_mesh_with_neighbors(p_pos);

	Vector3i dirs[] = {
		{ 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 }
	};

	for (const Vector3i &dir : dirs) {
		_try_build_mesh_with_neighbors(p_pos + dir);
	}
}

void World::_try_build_mesh_with_neighbors(Vector3i p_pos) {
	const ChunkNeighbors neighbors = _get_neighbors_for(p_pos);

	if (!neighbors.center)
		return;

	if (const bool ready = neighbors.left && neighbors.right && neighbors.top && neighbors.bottom && neighbors.front && neighbors.back; !ready)
		return;

	ChunkMeshBuilder mesh_builder;
	const Ref<ArrayMesh> mesh = mesh_builder.build(neighbors);

	if (mesh.is_null())
		return;

	ChunkGenerationResult result;
	result.pos = p_pos;
	result.mesh = mesh;
	result.collision_faces = mesh_builder.get_last_collision_faces();

	{
		std::lock_guard<std::mutex> lock(_pending_results_mutex);
		_pending_results.push_back(result);
	}
}

void World::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_thread_work", "pos"), &World::_thread_work);
}

} // namespace godot
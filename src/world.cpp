#include "world.h"
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/classes/object.hpp>

namespace godot {

void World::_ready() {
	_last_player_chunk_pos = Vector3i(INT_MAX, INT_MAX, INT_MAX);
	set_process(true);

	_terrain_noise.instantiate();
	_terrain_noise->set_noise_type(FastNoiseLite::TYPE_PERLIN);
	_terrain_noise->set_frequency(0.01);
	_terrain_noise->set_fractal_octaves(3);

	_cave_noise.instantiate();
	_cave_noise->set_noise_type(FastNoiseLite::TYPE_PERLIN);
	_cave_noise->set_frequency(0.02);
}


void World::_process(double delta) {
	if (!_player_node) {
		_player_node = get_node<Node3D>("../Player");
		if (!_player_node) return;

		_last_player_chunk_pos = _world_to_chunk_pos(_player_node->get_global_position());
		_update_chunks();
		return;
	}

	Vector3i current_chunk_p = _world_to_chunk_pos(_player_node->get_global_position());

	if (current_chunk_p != _last_player_chunk_pos) {
		_last_player_chunk_pos = current_chunk_p;
		_update_chunks();
	}

	_chunks_mutex.lock();
	std::vector<ChunkGenerationResult> to_process = std::move(_pending_results);
	_pending_results.clear();
	_chunks_mutex.unlock();

	for (const auto& result : to_process) {
		Vector3i player_chunk = _world_to_chunk_pos(_player_node->get_global_position());

		int dist_x = ABS(result.pos.x - player_chunk.x);
		int dist_y = ABS(result.pos.y - player_chunk.y);
		int dist_z = ABS(result.pos.z - player_chunk.z);

		if (dist_x > _world_radius + 1 || dist_y > _world_height + 1 || dist_z > _world_radius + 1) {
			_loading_chunks.erase(result.pos);
			continue;
		}

		_finalize_chunk(result);
		_loading_chunks.erase(result.pos);
	}
}

void World::_finalize_chunk(const ChunkGenerationResult& res) {
	if (res.mesh.is_null()) {
		return;
	}
	ChunkNode* chunk = nullptr;

	if (!_chunk_pool.is_empty()) {
		chunk = Object::cast_to<ChunkNode>(_chunk_pool.pop_back());
	} else {
		chunk = memnew(ChunkNode);
		add_child(chunk);
	}

	_chunks[res.pos] = chunk;
	chunk->set_mesh(res.mesh);

	chunk->set_surface_override_material(0, chunk->get_material());
	chunk->set_collision_faces(res.collision_faces);
	chunk->set_global_position(Vector3(res.pos.x * ChunkModel::SIZE, res.pos.y * ChunkModel::SIZE, res.pos.z * ChunkModel::SIZE));
	chunk->set_visible(true);

}

Vector3i World::_world_to_chunk_pos(Vector3 p_pos) {
    return Vector3i(
        Math::floor(p_pos.x / ChunkModel::SIZE),
        Math::floor(p_pos.y / ChunkModel::SIZE),
        Math::floor(p_pos.z / ChunkModel::SIZE)
    );
}

void World::_update_chunks() {
	_active_chunks_set.clear();

	for (int x = -_world_radius; x <= _world_radius; x++) {
		for (int y = -_world_height; y <= _world_height; y++) {
			for (int z = -_world_radius; z <= _world_radius; z++) {
				_active_chunks_set.insert(_last_player_chunk_pos + Vector3i(x, y, z));
			}
		}
	}

	Array to_remove;
	for (const KeyValue<Vector3i, ChunkNode*> &E : _chunks) {
		if (!_active_chunks_set.has(E.key)) {
			to_remove.push_back(E.key);
		}
	}

	for (int i = 0; i < to_remove.size(); i++) {
		Vector3i pos = to_remove[i];
		ChunkNode *node = _chunks[pos];
		node->set_visible(false);
		_chunk_pool.push_back(node);
		_chunks.erase(pos);

		std::lock_guard<std::mutex> lock(_data_mutex);
		_chunk_data.erase(pos);
	}

	for (const Vector3i &pos : _active_chunks_set) {
		if (_chunks.has(pos) || _loading_chunks.has(pos)) continue;
		_async_generate_chunk(pos);
	}
}

void World::_async_generate_chunk(Vector3i p_pos) {
	_loading_chunks.insert(p_pos);

	TerrainSettings settings;
	settings.noise_set.terrain_noise = _terrain_noise;
	settings.noise_set.cave_noise = _cave_noise;
	settings.terrain_base_height = _terrain_base_height;
	settings.terrain_amplitude = _terrain_amplitude;
	settings.cave_threshold = _cave_threshold;


	Callable action = Callable(this, "_thread_work").bind(p_pos);

	WorkerThreadPool::get_singleton()->add_task(action);
}
ChunkNeighbors World::_get_neighbors_for(Vector3i p_pos) {
	ChunkNeighbors n;

	std::lock_guard<std::mutex> lock(_data_mutex);

	auto get_model_ptr = [&](Vector3i pos) -> const ChunkModel* {
		if (_chunk_data.has(pos)) {
			return &_chunk_data[pos];
		}
		return nullptr;
	};

	n.center = get_model_ptr(p_pos);
	n.right  = get_model_ptr(p_pos + Vector3i(1, 0, 0));
	n.left   = get_model_ptr(p_pos + Vector3i(-1, 0, 0));
	n.top    = get_model_ptr(p_pos + Vector3i(0, 1, 0));
	n.bottom = get_model_ptr(p_pos + Vector3i(0, -1, 0));
	n.front  = get_model_ptr(p_pos + Vector3i(0, 0, 1));
	n.back   = get_model_ptr(p_pos + Vector3i(0, 0, -1));

	return n;
}
void World::_thread_work(Vector3i p_pos) {
	TerrainSettings settings;
	settings.terrain_base_height = _terrain_base_height;
	settings.terrain_amplitude = _terrain_amplitude;
	settings.cave_threshold = 0.1f;
	settings.noise_set.terrain_noise = _terrain_noise;
	settings.noise_set.cave_noise = _cave_noise;

	ChunkModel model = ChunkGenerator::generate(p_pos, settings);

	_data_mutex.lock();
	_chunk_data[p_pos] = model;
	_data_mutex.unlock();

	_try_build_mesh_with_neighbors(p_pos);

	Vector3i dirs[] = {
		{1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
	};

	for(const Vector3i& dir : dirs) {
		_try_build_mesh_with_neighbors(p_pos + dir);
	}
}

void World::_try_build_mesh_with_neighbors(Vector3i p_pos) {

	ChunkNeighbors neighbors = _get_neighbors_for(p_pos);

	if (!neighbors.center) return;

	bool ready = neighbors.left && neighbors.right && neighbors.top &&
				 neighbors.bottom && neighbors.front && neighbors.back;

	if (!ready) return;


	ChunkMeshBuilder mesh_builder;
	Ref<ArrayMesh> mesh = mesh_builder.build(neighbors);

	if (mesh.is_null()) return;

	ChunkGenerationResult result;
	result.pos = p_pos;
	result.mesh = mesh;
	result.collision_faces = mesh_builder.get_last_collision_faces();

	std::lock_guard<std::mutex> lock(_chunks_mutex);

	_pending_results.push_back(result);
}

void World::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_thread_work", "pos"), &World::_thread_work);
}

} // namespace godot
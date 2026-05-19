#include "world.h"

#include "atlas_loader.h"
#include "chunk_pool.h"


namespace godot {

void World::_ready() {
    _last_player_chunk_pos = Vector3i(INT_MAX, INT_MAX, INT_MAX);
    load_atlas("res://sprites/atlas.json");

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

    ERR_FAIL_NULL(_player_node);
    _last_player_chunk_pos = _world_to_chunk_pos(_player_node->get_global_position());
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
    _last_active_node_chunks.erase(pos);
    _chunk_pool->release(p_chunk_node);
}

void World::_shift_chunks() {
	for (const Vector3i &pos : _chunk_stream_manager->pop_queue_free_chunks()) {
		if (_last_active_node_chunks.has(pos)) {
			_remove_chunk(_last_active_node_chunks[pos]);
		}
	}

	for (const Vector3i &pos : _chunk_stream_manager->get_active_chunks_snapshot()) {

		if (_chunk_repository->contains_chunk(pos) || _model_generator->is_loading_chunk(pos)) {
			if (_chunk_repository->contains_chunk(pos) && !_last_active_node_chunks.has(pos)) {
				_try_build_mesh_with_neighbors(pos);
			}
			continue;
		}

		_queue_async_generate_chunk(pos);
	}
}

void World::_cleanup_far_chunks() {
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
    if (!_player_node) {
       return;
    }

    if (_did_player_change_chunk()) {
    	const Vector3i current = _world_to_chunk_pos(_player_node->get_global_position());

    	_last_player_chunk_pos = current;
    	_previous_player_chunk_pos = current;

    	_chunk_stream_manager->shift_chunks(current);

    	_shift_chunks();
    }

    const Vector3i player_chunk = _world_to_chunk_pos(_player_node->get_global_position());

    _process_generated_models();


    _process_generated_meshes(player_chunk);

    static double cleanup_timer = 0.0;
    cleanup_timer += delta;

    if (cleanup_timer > 2.0) {
       _cleanup_far_chunks();
       cleanup_timer = 0.0;
    }
}

void World::_process_generated_models() {
	HashMap<Vector3i, std::shared_ptr<ChunkModel>> ready_models = _model_generator->consume_generated_results();

	if (ready_models.is_empty()) {
		return;
	}


	for (auto it = ready_models.begin(); it != ready_models.end(); ++it) {
		_chunk_repository->add_chunk(it->key, it->value);
	}

	for (auto it = ready_models.begin(); it != ready_models.end(); ++it) {
		Vector3i pos = it->key;
		_try_build_mesh_with_neighbors(pos);

		Vector3i dirs[] = {
			{ 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 }
		};
		for (const Vector3i &dir : dirs) {
			_try_build_mesh_with_neighbors(pos + dir);
		}
	}
}

void World::_process_generated_meshes(const Vector3i &player_chunk) {
	MeshResultHashSet ready_meshes = _mesh_generator->consume_generated_meshes(_max_chunk_finalize_per_frame);

	for (const MeshResult &result : ready_meshes) {
		const int dist_x = ABS(result.pos.x - player_chunk.x);
		const int dist_y = ABS(result.pos.y - player_chunk.y);
		const int dist_z = ABS(result.pos.z - player_chunk.z);

		if (dist_x > _world_radius + 1 || dist_y > _world_height + 1 || dist_z > _world_radius + 1) {
			continue;
		}

		if (!_chunk_stream_manager->is_chunk_active(result.pos)) {
			continue;
		}

		_finalize_chunk(result);
	}
}
void World::_TEST_synchronous_model_generation(Vector3i p_pos) {
	UtilityFunctions::print("TEST: Iniciando geração de modelo síncrono em: ", p_pos);

	TerrainSettings settings;
	settings.terrain_base_height = _terrain_base_height;
	settings.terrain_amplitude = _terrain_amplitude;
	settings.cave_threshold = 0.1f;
	settings.noise_set.terrain_noise = _terrain_noise;
	settings.noise_set.cave_noise = _cave_noise;

	// 1. Chamar o gerador diretamente sem passar pelo WorkerThreadPool
	ChunkModel raw_model = ChunkGenerator::generate(p_pos, settings);
	auto model_ptr = std::make_shared<ChunkModel>(raw_model);

	// 2. Inserir direto no repositório
	_chunk_repository->add_chunk(p_pos, model_ptr);

	UtilityFunctions::print("TEST: Modelo gerado com sucesso para: ", p_pos);
}
void World::_TEST_synchronous_mesh_generation(Vector3i p_pos) {
	ChunkNeighbors neighbors = _get_neighbors_for(p_pos);

	if (!neighbors.center) return;

	// Verifica se todos os vizinhos existem no repositório para o culling funcionar
	bool ready = neighbors.left && neighbors.right && neighbors.top && neighbors.bottom && neighbors.front && neighbors.back;
	if (!ready) return;

	UtilityFunctions::print("TEST: Construindo Mesh síncrona para: ", p_pos);

	// Executa o builder diretamente na Main Thread
	ChunkMeshBuilder builder;
	Ref<ArrayMesh> mesh = builder.build(neighbors);

	if (mesh.is_valid()) {
		UtilityFunctions::print("TEST: Mesh criada com sucesso! Vértices gerados.");

		// Simula o finalizador estruturando o nó visual
		MeshResult mock_res{
			mesh,
			builder.get_last_collision_faces(),
			p_pos
		};
		_finalize_chunk(mock_res);
	}
}

void World::_finalize_chunk(const MeshResult &res) {
    if (res.mesh.is_null()) {
       return;
    }

    if (!_chunk_stream_manager->is_chunk_active(res.pos)) {
       return;
    }

    if (_last_active_node_chunks.has(res.pos)) {
       _remove_chunk(_last_active_node_chunks[res.pos]);
    }

    ChunkNode *chunk = _chunk_pool->acquire();
    _last_active_node_chunks[res.pos] = chunk;

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
          Math::floor(p_pos.z / ChunkModel::SIZE));
}

void World::_queue_async_generate_chunk(const Vector3i p_pos) {
    TerrainSettings settings;
    settings.terrain_base_height = _terrain_base_height;
    settings.terrain_amplitude = _terrain_amplitude;
    settings.cave_threshold = 0.1f;
    settings.noise_set.terrain_noise = _terrain_noise;
    settings.noise_set.cave_noise = _cave_noise;

	_model_generator->_queue_async_generate_chunk_model(p_pos, settings);
}

ChunkNeighbors World::_get_neighbors_for(Vector3i p_pos) {
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

void World::_try_build_mesh_with_neighbors(Vector3i p_pos) {
	if (_mesh_generator->is_queued_mesh(p_pos)) {
		return;
	}

	ChunkNeighbors neighbors = _get_neighbors_for(p_pos);

	if (!neighbors.center) {
		return;
	}

	bool ready = neighbors.left && neighbors.right && neighbors.top && neighbors.bottom && neighbors.front && neighbors.back;
	if (!ready) {
		return;
	}

	_mesh_generator->queue_async_generate_mesh(p_pos, neighbors);
}

void World::_bind_methods() {

}

} // namespace godot
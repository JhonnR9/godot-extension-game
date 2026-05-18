#ifndef WORLD_H
#define WORLD_H

#include "ChunkNode.h"
#include "chunk_generator.h"
#include "chunk_pool.h"
#include <godot_cpp/classes/fast_noise_lite.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/worker_thread_pool.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/vector3i.hpp>
#include <memory>

#include <mutex>
#include <vector>

namespace godot {

struct ChunkGenerationResult {
	Vector3i pos;
	ChunkModel model;
	Ref<ArrayMesh> mesh;
	PackedVector3Array collision_faces;
};


class World : public Node3D {
	GDCLASS(World, Node3D)

public:
	// Godot lifecycle
	void _ready() override;
	void _process(double delta) override;

protected:
	static void _bind_methods();

private:
	Ref<ChunkPool> _chunk_pool;

	// Optimization
	int _world_radius = 16;
	int _cache_radius = _world_radius + (_world_radius * .5f);
	int _world_height = 5;
	int _prewarm_chunk_pool = 8192 * 2;
	int _max_chunk_finalize_per_frame = 12;
	// Terrain settings
	uint64_t _seed = 546546;
	int _terrain_base_height = 24;
	float _terrain_amplitude = 32.0f;
	int _dirt_layer_depth = 12;
	float _cave_threshold = 0.02f;

	Ref<FastNoiseLite> _terrain_noise;
	Ref<FastNoiseLite> _cave_noise;
	// Player position control
	Vector3i _last_player_chunk_pos;
	Node3D *_player_node = nullptr;

	// World management
	void _queue_async_generate_chunk(Vector3i p_pos);
	ChunkNeighbors _get_neighbors_for(Vector3i p_pos);
	void _setup_noises();
	void _init_chunks();
	bool _did_player_change_chunk() const;
	void _remove_chunk(ChunkNode *p_chunk_node);
	void _rebuild_all_chunks();
	bool _is_chunk_active(const Vector3i &pos);
	void _shift_chunks();
	void _cleanup_far_chunks();

	static Vector3i _world_to_chunk_pos(Vector3 p_pos);
	void _finalize_chunk(const ChunkGenerationResult &res);
	void _try_build_mesh_with_neighbors(Vector3i p_pos);

	// Async functions
	void _thread_work(Vector3i p_pos);

	// Async state
	std::mutex _chunk_data_mutex;
	HashMap<Vector3i, std::shared_ptr<ChunkModel>> _chunk_data;

	std::mutex _active_chunks_mutex;
	HashSet<Vector3i> _active_chunks;

	HashMap<Vector3i, ChunkNode *> _last_active_node_chunks;

	std::mutex _pending_results_mutex;
	std::vector<ChunkGenerationResult> _pending_results;

	std::mutex _loading_chunks_mutex;
	HashSet<Vector3i> _loading_chunks;

	Vector3i _previous_player_chunk_pos;
};

} // namespace godot

#endif
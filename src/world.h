#ifndef WORLD_H
#define WORLD_H

#include "chunk_generator.h"
#include "chunk_mesh_async_generator.h"
#include "chunk_model_generator.h"
#include "chunk_node.h"
#include "chunk_pool.h"
#include "chunk_repository.h"
#include "chunk_streaming_manager.h"

#include <godot_cpp/classes/fast_noise_lite.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/vector3i.hpp>
#include <memory>

namespace godot {

class World : public Node3D {
	GDCLASS(World, Node3D)

public:
	// Godot lifecycle
	void _ready() override;
	void _process(double delta) override;

	void break_block(const Vector3 &world_pos);
	void set_block(const Vector3& p_world_pos, const block::Block& p_block);

protected:
	static void _bind_methods();

private:
	Ref<ChunkPool> _chunk_pool;
	Ref<ChunkRepository> _chunk_repository;
	Ref<ChunkStreamingManager> _chunk_stream_manager;
	Ref<ChunkModelGenerator> _model_generator;
	Ref<ChunkMeshAsyncGenerator> _mesh_generator;

	// Optimization
	int _world_radius = 8;
	int _cache_radius = _world_radius + (_world_radius  / 2);
	int _world_height = 4;
	int _prewarm_chunk_pool = 8192;
	int _current_chunks_finalize_in_frame = 100;

	// Terrain settings
	uint64_t _seed = 546546;
	int _terrain_base_height = 24;
	float _terrain_amplitude = 5.0f;
	int _dirt_layer_depth = 20;
	float _cave_threshold = 0.2f;

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
	void _update_visible_chunks();
	void _cleanup_far_chunks() const;
	float _get_current_chunks_finalize_amount(float delta);

	static Vector3i _world_to_chunk_pos(Vector3 p_pos);
	void _finalize_chunk(const MeshResult &res);
	void _try_build_mesh_with_neighbors(Vector3i p_pos);

	void _process_models();
	void _process_meshes(const Vector3i &p_pos);
	void _rebuild_chunk(const Vector3i &pos);
	bool _is_high_priority(const Vector3i &pos, bool dirty);

	// World state
	HashMap<Vector3i, ChunkNode *> _rendered_chunks;
	Vector3i _previous_player_chunk_pos;
};

} // namespace godot

#endif // WORLD_H
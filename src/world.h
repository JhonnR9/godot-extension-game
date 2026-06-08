#ifndef WORLD_H
#define WORLD_H

#include "chunk_generator.h"
#include "chunk_mesh_async_generator.h"
#include "chunk_model_generator.h"
#include "chunk_node.h"
#include "chunk_pool.h"
#include "chunk_region_async_loader.h"
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
	void _exit_tree() override;

	void break_block(const Vector3 &world_pos);
	void set_block(const Vector3 &p_world_pos, const voxel::Block &p_block) const;
	void set_focus_node(Node3D *p_node);
	void set_focus_position(Vector3 p_pos);
	void create_new_world(int32_t p_seed, const String &p_name);
	void load_world(uint64_t p_id);
	PackedInt64Array get_saved_worlds() const;
	void save_world();
	void delete_world(uint64_t p_id);
protected:
	static void _bind_methods();

private:
	Ref<ChunkPool> _chunk_pool;
	Ref<ChunkRepository> _chunk_repository;
	Ref<ChunkStreamingManager> _chunk_stream_manager;
	Ref<ChunkModelGenerator> _model_generator;
	Ref<ChunkMeshAsyncGenerator> _mesh_generator;
	Ref<ChunkDiskRepository> _disk_repository;
	Ref<ChunkRegionAsyncLoader> _region_loader;
	void save_world_final();

	// Optimization
	int _world_radius = 6;
	int _world_height = 4;
	int _cache_radius = _world_radius + 2;
	int _diameter     = (_cache_radius * 2) + 1; //  (2 * R + 1).
	int _prewarm_chunk_pool = (_diameter * _diameter) * _world_height;
	int _current_chunks_finalize_in_frame = 100;

	// Terrain settings
	int _terrain_base_height = 24;
	float _terrain_amplitude = 8.0f;
	int _dirt_layer_depth    = 20;
	float _cave_threshold    = 0.2f;

	Ref<FastNoiseLite> _terrain_noise;
	Ref<FastNoiseLite> _cave_noise;

	// Player position control
	Vector3i _last_focos_position;

	// World management
	void _queue_async_generate_chunk(Vector3i p_pos) const;
	ChunkNeighbors _get_neighbors_for(Vector3i p_pos) const;
	void _setup_noises();
	void _init_chunks();
	void _remove_chunk(ChunkNode *p_chunk_node);
	void _update_visible_chunks();
	void _cleanup_far_chunks();
	float _get_current_chunks_finalize_amount(float delta);

	void _finalize_chunk(const MeshResult &res);
	void _try_build_mesh_with_neighbors(Vector3i p_pos) const;

	void _process_models();
	void _process_meshes(const Vector3i &p_pos);
	void _rebuild_chunk(const Vector3i &pos) const;
	bool _is_high_priority(const Vector3i &pos, bool dirty) const;
	void _ensure_region_loaded_for_chunk(const Vector3i &chunk_pos);

	Node3D *_focus_node       = nullptr;
	Vector3 _focus_manual_pos = Vector3(0, 0, 0);
	bool _use_manual_pos      = true;
	Vector3 _get_current_focus_position() const;

	// World state
	HashMap<Vector3i, ChunkNode *> _rendered_chunks;
	HashMap<Vector3i, voxel::Region> _region_cache;
	HashSet<Vector3i> _pending_region_loads;
	Vector3i _previous_player_chunk_pos;

	void _queue_region_load(const Vector3i &region_pos);
	void _update_region_streaming(const Vector3i &current_chunk_pos, const Vector3i &previous_chunk_pos);
	void _process_loaded_regions();
	void _unload_region(const Vector3i &region_pos);

	bool _is_initializing{false};
};
} // namespace godot

#endif // WORLD_H
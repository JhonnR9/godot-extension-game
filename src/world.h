#ifndef WORLD_H
#define WORLD_H

#include "ChunkNode.h"
#include "chunk_generator.h"

#include <godot_cpp/classes/fast_noise_lite.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/vector3i.hpp>
#include <godot_cpp/classes/worker_thread_pool.hpp>
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
	void _ready() override;
	void _process(double delta) override;

	void _thread_work(Vector3i p_pos);
protected:
	static void _bind_methods();

private:
	uint64_t _seed = 12345;

	int _world_radius = 8;
	int _world_height = 8;

	int _terrain_base_height = 32;
	float _terrain_amplitude = 16.0f;
	int _dirt_layer_depth = 12;
	float cave_threshold = 0.7f;

	Ref<FastNoiseLite> _terrain_noise;
	Ref<FastNoiseLite> _cave_noise;

	HashMap<Vector3i, ChunkNode *> _chunks;

	Vector3i _last_player_chunk_pos;
	Node3D *_player_node = nullptr;

	TypedArray<ChunkNode> _chunk_pool;

	void _update_chunks();
	static Vector3i _world_to_chunk_pos(Vector3 p_pos);
	void _finalize_chunk(const ChunkGenerationResult& res);
	void _try_build_mesh_with_neighbors(Vector3i p_pos);

private:
	std::mutex _chunks_mutex;
	std::vector<ChunkGenerationResult> _pending_results;
	HashSet<Vector3i> _loading_chunks;

	void _async_generate_chunk(Vector3i p_pos);
	ChunkNeighbors _get_neighbors_for(Vector3i p_pos);
	std::mutex _data_mutex;
	HashMap<Vector3i, ChunkModel> _chunk_data;



};

} // namespace godot

#endif
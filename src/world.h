#ifndef WORLD_H
#define WORLD_H

#include "ChunkNode.h"
#include "chunk_generator.h"

#include <godot_cpp/classes/fast_noise_lite.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/vector3i.hpp>

namespace godot {

struct ChunkHandler {
	ChunkModel model;
	ChunkMeshBuilder builder;
	ChunkGenerator generator;
};

class World : public Node3D {
	GDCLASS(World, Node3D)

public:
	void _ready() override;
	void generate_world(int radius, int height);
	void _process(double delta) override;
	void _regenerate_world();

protected:
	static void _bind_methods();

private:
	uint64_t _seed = 12345;

	int _world_radius = 4;
	int _world_height = 4;

	int _terrain_base_height = 16;
	float _terrain_amplitude = 16.0f;
	int _dirt_layer_depth = 4;

	Ref<FastNoiseLite> _terrain_noise;
	Ref<FastNoiseLite> _cave_noise;

	HashMap<Vector3i, ChunkNode *> _chunks;
	void _create_chunk(Vector3i chunk_pos);

	Vector3i _last_player_chunk_pos;
	Node3D *_player_node = nullptr;
	TypedArray<ChunkNode> _chunk_pool;

	void _update_chunks();
	static Vector3i _world_to_chunk_pos(Vector3 p_pos);
};

} // namespace godot

#endif
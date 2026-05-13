#ifndef CHUNK_H
#define CHUNK_H

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/fast_noise_lite.hpp>
#include <godot_cpp/classes/static_body3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/concave_polygon_shape3d.hpp>
#include "block.h"
#include "voxel_mesher.h"

namespace godot {
class World;

class Chunk : public MeshInstance3D {
	GDCLASS(Chunk, MeshInstance3D)
	World *_world = nullptr;

public:
	Chunk() = default;
	static constexpr int SIZE = 16;
	void _ready() override;

	void rebuild_mesh();

	void generate(
	   uint64_t seed,
	   Vector3i chunk_pos,
	   Ref<FastNoiseLite> terrain_noise,
	   Ref<FastNoiseLite> cave_noise,
	   int terrain_base_height,
	   float terrain_amplitude);
	void set_world(World *p_world) {
		_world = p_world;
	}

	const Block &get_block(int x, int y, int z) const {
		return _blocks[x][y][z];
	}
	void  _enter_tree() override;

protected:
	static void _bind_methods();

private:
	bool _is_air(int x, int y, int z) const;

	Block _blocks[SIZE][SIZE][SIZE];

	VoxelMesher _voxel_mesher;
	StaticBody3D *_static_body = nullptr;
	CollisionShape3D *_collision_shape = nullptr;
	Ref<ArrayMesh> _mesh;
	Ref<ConcavePolygonShape3D> _shape;
	Ref<StandardMaterial3D> _material;

	bool _active = false;

	void _generate_faces();
	void _setup_material();
};

}

#endif
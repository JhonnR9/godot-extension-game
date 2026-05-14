#ifndef CHUNK_H
#define CHUNK_H

#include "chunk_mesh_builder.h"

#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/concave_polygon_shape3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/static_body3d.hpp>

namespace godot {
class World;

class ChunkNode : public MeshInstance3D {
	GDCLASS(ChunkNode, MeshInstance3D)

public:
	~ChunkNode() override;
	void  _enter_tree() override;

protected:
	static void _bind_methods();

private:
	StaticBody3D *_static_body = nullptr;
	CollisionShape3D *_collision_shape = nullptr;

	Ref<ConcavePolygonShape3D> _shape;
	Ref<StandardMaterial3D> _material;

	void _setup();
	void _setup_material();

public:
	void set_collision_faces(const PackedVector3Array&collision_faces);
	Ref<StandardMaterial3D> get_material() {
		return _material;
	}
};

}

#endif
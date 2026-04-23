//
// Created by jhone on 21/04/2026.
//

#ifndef CUBE_BASE_H
#define CUBE_BASE_H

#include <godot_cpp/classes/mesh_instance3d.hpp>

namespace godot {

class CubeBase final : public MeshInstance3D {
	GDCLASS(CubeBase, MeshInstance3D)

public:
	CubeBase() = default;
	void _ready() override;

protected:
	static void _bind_methods();

private:
	Ref<ArrayMesh> _mesh;
	PackedVector3Array _vertices;
	PackedVector3Array _normals;
	PackedVector2Array _uvs;
	PackedInt32Array _indices;

	void _add_face(Vector3 v0, Vector3 v1, Vector3 v2, Vector3 v3, Vector3 normal);
};

} //namespace godot

#endif //CUBE_BASE_H

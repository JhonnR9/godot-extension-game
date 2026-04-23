//
// Created by jhone on 21/04/2026.
//

#include "cube_base.h"

namespace godot {

void CubeBase::_ready() {
	MeshInstance3D::_ready();

	_mesh.instantiate();

	constexpr real_t s = 0.5;

	// FRONT (+Z)
	_add_face(
		Vector3(-s, -s,  s),
		Vector3( s, -s,  s),
		Vector3( s,  s,  s),
		Vector3(-s,  s,  s),
		Vector3(0, 0, 1)
	);

	// BACK (-Z)
	_add_face(
		Vector3( s, -s, -s),
		Vector3(-s, -s, -s),
		Vector3(-s,  s, -s),
		Vector3( s,  s, -s),
		Vector3(0, 0, -1)
	);

	// LEFT (-X)
	_add_face(
		Vector3(-s, -s, -s),
		Vector3(-s, -s,  s),
		Vector3(-s,  s,  s),
		Vector3(-s,  s, -s),
		Vector3(-1, 0, 0)
	);

	// RIGHT (+X)
	_add_face(
		Vector3( s, -s,  s),
		Vector3( s, -s, -s),
		Vector3( s,  s, -s),
		Vector3( s,  s,  s),
		Vector3(1, 0, 0)
	);

	// TOP (+Y)
	_add_face(
		Vector3(-s,  s,  s),
		Vector3( s,  s,  s),
		Vector3( s,  s, -s),
		Vector3(-s,  s, -s),
		Vector3(0, 1, 0)
	);

	// BOTTOM (-Y)
	_add_face(
		Vector3(-s, -s, -s),
		Vector3( s, -s, -s),
		Vector3( s, -s,  s),
		Vector3(-s, -s,  s),
		Vector3(0, -1, 0)
	);

	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);

	arrays[Mesh::ARRAY_VERTEX] = _vertices;
	arrays[Mesh::ARRAY_NORMAL] = _normals;
	arrays[Mesh::ARRAY_TEX_UV] = _uvs;
	arrays[Mesh::ARRAY_INDEX] = _indices;

	_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

	set_mesh(_mesh);
}

void CubeBase::_bind_methods() {
}

void CubeBase::_add_face(Vector3 v0, Vector3 v1, Vector3 v2, Vector3 v3, Vector3 normal) {
	int start = _vertices.size();

	_vertices.append(v0);
	_vertices.append(v1);
	_vertices.append(v2);
	_vertices.append(v3);

	// Normals (one per vertex)
	_normals.append(normal);
	_normals.append(normal);
	_normals.append(normal);
	_normals.append(normal);

	// Basic UV (each face uses the full quad)
	_uvs.append(Vector2(0, 0));
	_uvs.append(Vector2(1, 0));
	_uvs.append(Vector2(1, 1));
	_uvs.append(Vector2(0, 1));

	// Two triangles
	_indices.append(start);
	_indices.append(start + 2);
	_indices.append(start + 1);

	_indices.append(start);
	_indices.append(start + 3);
	_indices.append(start + 2);

}

} //namespace godot
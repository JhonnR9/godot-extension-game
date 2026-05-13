#include "voxel_mesher.h"

#include <godot_cpp/classes/mesh.hpp>

namespace godot {

void VoxelMesher::clear() {
	_vertices.clear();
	_normals.clear();
	_uvs.clear();
	_colors.clear();
	_indices.clear();
	_collision_faces.clear();
}

void VoxelMesher::add_face(CubeFace face, Vector3 pos, Color color, float size) {

	float s = size * 0.5f;

	Vector3 p = pos;

	switch (face) {

		case CubeFace::F:
			_add_quad(
				p + Vector3(-s,-s, s),
				p + Vector3( s,-s, s),
				p + Vector3( s, s, s),
				p + Vector3(-s, s, s),
				Vector3(0,0,1),
				color
			);
			break;

		case CubeFace::B:
			_add_quad(
				p + Vector3( s,-s,-s),
				p + Vector3(-s,-s,-s),
				p + Vector3(-s, s,-s),
				p + Vector3( s, s,-s),
				Vector3(0,0,-1),
				color
			);
			break;

		case CubeFace::L:
			_add_quad(
				p + Vector3(-s,-s,-s),
				p + Vector3(-s,-s, s),
				p + Vector3(-s, s, s),
				p + Vector3(-s, s,-s),
				Vector3(-1,0,0),
				color
			);
			break;

		case CubeFace::R:
			_add_quad(
				p + Vector3( s,-s, s),
				p + Vector3( s,-s,-s),
				p + Vector3( s, s,-s),
				p + Vector3( s, s, s),
				Vector3(1,0,0),
				color
			);
			break;

		case CubeFace::U:
			_add_quad(
				p + Vector3(-s, s, s),
				p + Vector3( s, s, s),
				p + Vector3( s, s,-s),
				p + Vector3(-s, s,-s),
				Vector3(0,1,0),
				color
			);
			break;

		case CubeFace::D:
			_add_quad(
				p + Vector3(-s,-s,-s),
				p + Vector3( s,-s,-s),
				p + Vector3( s,-s, s),
				p + Vector3(-s,-s, s),
				Vector3(0,-1,0),
				color
			);
			break;
	}
}

void VoxelMesher::_add_quad(
	Vector3 v0,
	Vector3 v1,
	Vector3 v2,
	Vector3 v3,
	Vector3 normal,
	Color color
) {
	int start = _vertices.size();

	_vertices.append(v0);
	_vertices.append(v1);
	_vertices.append(v2);
	_vertices.append(v3);

	for (int i = 0; i < 4; i++) {
		_normals.append(normal);
		_colors.append(color);
	}

	_uvs.append(Vector2(0,0));
	_uvs.append(Vector2(1,0));
	_uvs.append(Vector2(1,1));
	_uvs.append(Vector2(0,1));

	_indices.append(start);
	_indices.append(start + 2);
	_indices.append(start + 1);

	_indices.append(start);
	_indices.append(start + 3);
	_indices.append(start + 2);

	_collision_faces.append(v0);
	_collision_faces.append(v2);
	_collision_faces.append(v1);

	_collision_faces.append(v0);
	_collision_faces.append(v3);
	_collision_faces.append(v2);
}

Array VoxelMesher::build_arrays() const {
	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);

	arrays[Mesh::ARRAY_VERTEX] = _vertices;
	arrays[Mesh::ARRAY_NORMAL] = _normals;
	arrays[Mesh::ARRAY_COLOR] = _colors;
	arrays[Mesh::ARRAY_TEX_UV] = _uvs;
	arrays[Mesh::ARRAY_INDEX] = _indices;

	return arrays;
}
PackedVector3Array VoxelMesher::get_collision_faces() const {
	return _collision_faces;
}

} //namespace godot
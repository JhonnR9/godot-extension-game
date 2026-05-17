#include "voxel_mesher.h"

#include "chunk_mesh_builder.h"
#include "chunk_model.h"

#include <bits/fs_fwd.h>

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

void VoxelMesher::add_face(
	CubeFace face,
	Vector3 pos,
	const AtlasUV &uv,
	Color color,
	float size
) {
	float s = size * 0.5f;

	switch (face) {
		case CubeFace::F:
			_add_quad(
				pos + Vector3(-s, -s, s),
				pos + Vector3(s, -s, s),
				pos + Vector3(s, s, s),
				pos + Vector3(-s, s, s),
				Vector3(0, 0, 1),
				uv,
				color
			);
			break;

		case CubeFace::B:
			_add_quad(
				pos + Vector3(s, -s, -s),
				pos + Vector3(-s, -s, -s),
				pos + Vector3(-s, s, -s),
				pos + Vector3(s, s, -s),
				Vector3(0, 0, -1),
				uv,
				color
			);
			break;

		case CubeFace::L:
			_add_quad(
				pos + Vector3(-s, -s, -s),
				pos + Vector3(-s, -s, s),
				pos + Vector3(-s, s, s),
				pos + Vector3(-s, s, -s),
				Vector3(-1, 0, 0),
				uv,
				color
			);
			break;

		case CubeFace::R:
			_add_quad(
				pos + Vector3(s, -s, s),
				pos + Vector3(s, -s, -s),
				pos + Vector3(s, s, -s),
				pos + Vector3(s, s, s),
				Vector3(1, 0, 0),
				uv,
				color
			);
			break;

		case CubeFace::U:
			_add_quad(
				pos + Vector3(-s, s, s),
				pos + Vector3(s, s, s),
				pos + Vector3(s, s, -s),
				pos + Vector3(-s, s, -s),
				Vector3(0, 1, 0),
				uv,
				color
			);
			break;

		case CubeFace::D:
			_add_quad(
				pos + Vector3(-s, -s, -s),
				pos + Vector3(s, -s, -s),
				pos + Vector3(s, -s, s),
				pos + Vector3(-s, -s, s),
				Vector3(0, -1, 0),
				uv,
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
	const AtlasUV &uv,
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

	_uvs.append(Vector2(uv.min.x, uv.max.y));
	_uvs.append(Vector2(uv.max.x, uv.max.y));
	_uvs.append(Vector2(uv.max.x, uv.min.y));
	_uvs.append(Vector2(uv.min.x, uv.min.y));

	// indices
	_indices.append(start);
	_indices.append(start + 2);
	_indices.append(start + 1);

	_indices.append(start);
	_indices.append(start + 3);
	_indices.append(start + 2);

	// collision
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
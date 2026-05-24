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
	_indices.clear();
	_collision_faces.clear();
	_tex_layer.clear();
}


void VoxelMesher::add_quad(
		const Vector3 &v0,
		const Vector3 &v1,
		const Vector3 &v2,
		const Vector3 &v3,
		const Vector3 &normal,
		const int tex_layer,
		const Vector2 &tile_scale,
		bool swap_uvs
		) {
	const int start = static_cast<int>(_vertices.size());

	_vertices.append(v0);
	_vertices.append(v1);
	_vertices.append(v2);
	_vertices.append(v3);


	for (int i = 0; i < 4; i++) {
		_normals.append(normal);
		_tex_layer.append(static_cast<float>(tex_layer));
	}

	if (swap_uvs) {
		 _uvs.append(Vector2(0.0f, tile_scale.x));
		 _uvs.append(Vector2(0.0f, 0.0f));
		 _uvs.append(Vector2(tile_scale.y, 0.0f));
		 _uvs.append(Vector2(tile_scale.y, tile_scale.x));
	} else {
		_uvs.append(Vector2(0.0f, tile_scale.y));
		_uvs.append(Vector2(tile_scale.x, tile_scale.y));
		_uvs.append(Vector2(tile_scale.x, 0.0f));
		_uvs.append(Vector2(0.0f, 0.0f));
	}

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

	arrays[Mesh::ARRAY_VERTEX]  = _vertices;
	arrays[Mesh::ARRAY_NORMAL]  = _normals;
	arrays[Mesh::ARRAY_TEX_UV]  = _uvs;
	arrays[Mesh::ARRAY_CUSTOM0] = _tex_layer;
	arrays[Mesh::ARRAY_INDEX]   = _indices;

	return arrays;
}

PackedVector3Array VoxelMesher::get_collision_faces() const {
	return _collision_faces;
}
} //namespace godot
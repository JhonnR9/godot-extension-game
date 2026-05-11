#ifndef VOXEL_MESHER_H
#define VOXEL_MESHER_H

#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>

#include "voxel_types.h"

namespace godot {

class VoxelMesher {
public:
	void clear();

	void add_face(
		CubeFace face,
		Vector3 position,
		Color color = Color(1.0f, 1.0f, 1.0f, 1.0f),
		float size = 1.0f
	);

	[[nodiscard]]
	Array build_arrays() const;

private:
	PackedVector3Array _vertices;
	PackedVector3Array _normals;
	PackedVector2Array _uvs;
	PackedColorArray _colors;
	PackedInt32Array _indices;

	void _add_quad(
		Vector3 v0,
		Vector3 v1,
		Vector3 v2,
		Vector3 v3,
		Vector3 normal,
		Color color
	);
};

}

#endif
#ifndef UTILS_H
#define UTILS_H
#include "chunk_model.h"
#include "godot_cpp/variant/vector3i.hpp"


namespace voxel {
using namespace godot;

inline int32_t floor_div(int32_t a, int32_t b) {
	int32_t r = a / b;
	int32_t m = a % b;

	if (m != 0 && ((m < 0) != (b < 0)))
		--r;

	return r;
}

inline int32_t posmod(int32_t a, int32_t b) {
	int32_t m = a % b;

	if (m < 0)
		m += (b < 0 ? -b : b);

	return m;
}

inline int32_t floor_int(float f) {
	int32_t i = static_cast<int32_t>(f);
	if (f < 0 && f != static_cast<float>(i)) {
		return i - 1;
	}
	return i;
}

inline Vector3i world_to_block(const Vector3 &p) {
	return {
		floor_int(p.x),
		floor_int(p.y),
		floor_int(p.z)
	};
}

inline Vector3i block_to_chunk_coords(const Vector3i &p) {
	return {
		floor_div(p.x, ChunkModel::SIZE_X),
		floor_div(p.y, ChunkModel::SIZE_Y),
		floor_div(p.z, ChunkModel::SIZE_Z)
	};
}

inline Vector3i chunk_coords_to_world(const Vector3i &chunk_pos) {
	return {
		chunk_pos.x * ChunkModel::SIZE_X,
		chunk_pos.y * ChunkModel::SIZE_Y,
		chunk_pos.z * ChunkModel::SIZE_Z
	};
}

inline Vector3i block_to_chunk_local_block(const Vector3i &pos) {
	return {
		posmod(pos.x, ChunkModel::SIZE_X),
		posmod(pos.y, ChunkModel::SIZE_Y),
		posmod(pos.z, ChunkModel::SIZE_Z)
	};
}

inline Vector3i world_to_chunk(const Vector3 &p) {
	return block_to_chunk_coords(world_to_block(p));
}

const auto DIR_RIGHT = Vector3i(1, 0, 0);
const auto DIR_LEFT  = Vector3i(-1, 0, 0);
const auto DIR_UP    = Vector3i(0, 1, 0);
const auto DIR_DOWN  = Vector3i(0, -1, 0);
const auto DIR_FRONT = Vector3i(0, 0, 1);
const auto DIR_BACK  = Vector3i(0, 0, -1);
}
#endif //UTILS_H
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

inline Vector3i world_to_chunk_pos(const Vector3i &p) {
	return {
		floor_div(p.x, ChunkModel::SIZE_X),
		floor_div(p.y, ChunkModel::SIZE_Y),
		floor_div(p.z, ChunkModel::SIZE_Z)
	};
}

inline Vector3i world_to_chunk_block_pos(const Vector3i& pos) {
	return {
		posmod(pos.x, ChunkModel::SIZE_X),
		posmod(pos.y, ChunkModel::SIZE_Y),
		posmod(pos.z, ChunkModel::SIZE_Z)
	};
}
}
#endif //UTILS_H
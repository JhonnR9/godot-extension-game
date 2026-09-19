#ifndef UTILS_H
#define UTILS_H
#include <cstdint>
#include "chunk_model.h"
#include "godot_cpp/variant/vector3i.hpp"
#include "godot_cpp/templates/hash_map.hpp"

namespace voxel {
using namespace godot;

struct BlockPos {
	Vector3i v;
	explicit BlockPos(Vector3i p = {}) : v(p) {}
	bool operator==(const BlockPos &o) const { return v == o.v; }
};

struct ChunkPos {
	Vector3i v;
	explicit ChunkPos(Vector3i p = {}) : v(p) {}
	bool operator==(const ChunkPos &o) const { return v == o.v; }
	ChunkPos operator+(const Vector3i &dir) const { return ChunkPos(v + dir); }
	ChunkPos operator-(const Vector3i &dir) const { return ChunkPos(v - dir); }
};

struct RegionPos {
	Vector3i v;
	explicit RegionPos(Vector3i p = {}) : v(p) {}
	bool operator==(const RegionPos &o) const { return v == o.v; }
};

struct ChunkPosHasher {
	static uint32_t hash(const ChunkPos &p) { return HashMapHasherDefault::hash(p.v); }
};

struct RegionPosHasher {
	static uint32_t hash(const RegionPos &p) { return HashMapHasherDefault::hash(p.v); }
};

struct BlockPosHasher{
	static uint32_t hash(const BlockPos &p) { return HashMapHasherDefault::hash(p.v); }
};

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

inline int32_t floor_int(const float f) {
	const auto i = static_cast<int32_t>(f);
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
		floor_div(p.x, Chunk::SIZE_X),
		floor_div(p.y, Chunk::SIZE_Y),
		floor_div(p.z, Chunk::SIZE_Z)
	};
}

inline Vector3i chunk_coords_to_world(const Vector3i &chunk_pos) {
	return {
		chunk_pos.x * Chunk::SIZE_X,
		chunk_pos.y * Chunk::SIZE_Y,
		chunk_pos.z * Chunk::SIZE_Z
	};
}

inline Vector3i block_to_chunk_local_block(const Vector3i &pos) {
	return {
		posmod(pos.x, Chunk::SIZE_X),
		posmod(pos.y, Chunk::SIZE_Y),
		posmod(pos.z, Chunk::SIZE_Z)
	};
}

inline Vector3i world_to_chunk(const Vector3 &p) {
	return block_to_chunk_coords(world_to_block(p));
}

inline bool is_position_in_cylinder(const Vector3i &pos, const Vector3i &center, int radius_xz, int radius_y) {
	int dx = pos.x - center.x;
	int dz = pos.z - center.z;
	int dy = ABS(pos.y - center.y);

	return (dx * dx + dz * dz <= radius_xz * radius_xz) && (dy <= radius_y);
}

struct ChunkDelta {
	HashMap<Vector3i, Block> delta;
};

struct Region {
	HashMap<Vector3i, ChunkDelta> edited_chunks;
};

constexpr int CHUNKS_PER_REGION = 32;

inline Vector3i chunk_to_region_coords(const Vector3i &chunk_pos) {
	return {
		floor_div(chunk_pos.x, CHUNKS_PER_REGION),
		floor_div(chunk_pos.y, CHUNKS_PER_REGION),
		floor_div(chunk_pos.z, CHUNKS_PER_REGION)
	};
}

inline Vector3i chunk_to_region_local_chunk(const Vector3i &chunk_pos) {
	return {
		posmod(chunk_pos.x, CHUNKS_PER_REGION),
		posmod(chunk_pos.y, CHUNKS_PER_REGION),
		posmod(chunk_pos.z, CHUNKS_PER_REGION)
	};
}

const auto DIR_RIGHT = Vector3i(1, 0, 0);
const auto DIR_LEFT  = Vector3i(-1, 0, 0);
const auto DIR_UP    = Vector3i(0, 1, 0);
const auto DIR_DOWN  = Vector3i(0, -1, 0);
const auto DIR_FRONT = Vector3i(0, 0, 1);
const auto DIR_BACK  = Vector3i(0, 0, -1);
} // namespace voxel
#endif //UTILS_H
#ifndef CHUNK_MODEL_H
#define CHUNK_MODEL_H

#include "voxel.h"
#include <cstdint>

namespace godot {

enum class ChunkStage : uint8_t {
	UNLOADED,

	QUEUED_GENERATION,
	GENERATING,

	LOADED,

	WAITING_NEIGHBORS,

	QUEUED_MESH,
	GENERATING_MESH,
	MESH_READY,

	RENDERED,

	UNLOADING,
};

enum class ChunkFlag : uint8_t {
	NONE       = 0,
	DIRTY      = 1 << 0,
	MESH_DIRTY = 1 << 1,
};

struct Chunk {
	static constexpr int SIZE_X = 16;
	static constexpr int SIZE_Y = 16;
	static constexpr int SIZE_Z = 16;

	static constexpr int MAX_X = SIZE_X - 1;
	static constexpr int MAX_Y = SIZE_Y - 1;
	static constexpr int MAX_Z = SIZE_Z - 1;

	static constexpr int MIN_X = 0;
	static constexpr int MIN_Y = 0;
	static constexpr int MIN_Z = 0;

	static constexpr int VOLUME = SIZE_X * SIZE_Y * SIZE_Z;

	ChunkStage stage = ChunkStage::UNLOADED;
	uint8_t flags = 0;

	constexpr voxel::Block get_block(int x, int y, int z) const {
		return _blocks[index(x, y, z)];
	}

	constexpr void set_block(int x, int y, int z, voxel::Block b) {
		_blocks[index(x, y, z)] = b;
	}

	constexpr bool has_flag(ChunkFlag flag) const {
		return flags & static_cast<uint8_t>(flag);
	}

	constexpr void add_flag(ChunkFlag flag) {
		flags |= static_cast<uint8_t>(flag);
	}

	constexpr void remove_flag(ChunkFlag flag) {
		flags &= ~static_cast<uint8_t>(flag);
	}

	constexpr void clear_flags() {
		flags = 0;
	}

private:
	voxel::Block _blocks[VOLUME]{};

	static constexpr int index(int x, int y, int z) {
		return x + y * SIZE_X + z * SIZE_X * SIZE_Y;
	}
};

// 32 vertical chunks centered around world chunk Y = 0.
constexpr int WORLD_MIN_CHUNK_Y = -16;
constexpr int WORLD_MAX_CHUNK_Y = 15;
constexpr int WORLD_BEDROCK_Y = WORLD_MIN_CHUNK_Y * Chunk::SIZE_Y;

} // namespace godot

#endif

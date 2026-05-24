#ifndef CHUNK_MODEL_H
#define CHUNK_MODEL_H

#include "block.h"

namespace godot {

struct ChunkModel {
	static constexpr int SIZE_X = 16;
	static constexpr int SIZE_Y = 16;
	static constexpr int SIZE_Z = 16;

	static constexpr int VOLUME = SIZE_X * SIZE_Y * SIZE_Z;

	constexpr block::Block get_block(int x, int y, int z) const {
		return _blocks[index(x, y, z)];
	}

	constexpr void set_block(int x, int y, int z, block::Block b) {
		_blocks[index(x, y, z)] = b;
	}

private:
	block::Block _blocks[VOLUME] {};

	static constexpr int index(int x, int y, int z) {
		return x + y * SIZE_X + z * SIZE_X * SIZE_Y;
	}
};

}

#endif
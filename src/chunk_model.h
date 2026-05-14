//
// Created by jhone on 13/05/2026.
//

#ifndef CHUNK_MODEL_H
#define CHUNK_MODEL_H
#include "block.h"

namespace godot {
struct  ChunkModel {
	static  constexpr int SIZE = 16;

	const Block &get_block(const int x, const int y, const int z) const {
		return _blocks[x][y][z];
	}

	Block _blocks[SIZE][SIZE][SIZE];
};
}



#endif //CHUNK_MODEL_H

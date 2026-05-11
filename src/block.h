#ifndef BLOCK_H
#define BLOCK_H

#include "voxel_types.h"

namespace godot {

struct Block {
	BlockType type = BlockType::AIR;

	[[nodiscard]]
	bool is_air() const {
		return type == BlockType::AIR;
	}
};

}

#endif
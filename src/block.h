#ifndef BLOCK_H
#define BLOCK_H

#include "voxel_types.h"

namespace block {

using Block = uint32_t;

constexpr uint32_t BLOCK_ID_MASK = 0x3FF; // 10 bits

constexpr uint16_t block_id(Block b) {
	return b & BLOCK_ID_MASK;
}

constexpr godot::BlockType type(Block b) {
	return static_cast<godot::BlockType>(block_id(b));
}

constexpr bool is_air(Block b) {
	return block_id(b) == 0;
}

constexpr Block make_block(godot::BlockType type) {
	return static_cast<Block>(type);
}

} // namespace block

#endif
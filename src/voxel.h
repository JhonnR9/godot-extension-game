#ifndef BLOCK_H
#define BLOCK_H

#include "voxel_types.h"

namespace voxel {

using Block = uint32_t;

constexpr uint32_t BLOCK_ID_MASK = 0x3FF; // 10 bits
constexpr Block BLOCK_FLAG_SOLID       = Block{1u} << 10;
constexpr Block BLOCK_FLAG_TRANSPARENT = Block{1u} << 11;
constexpr Block BLOCK_FLAG_EMISSIVE    = Block{1u} << 12;
constexpr Block BLOCK_FLAG_WATERLOG    = Block{1u} << 13;

constexpr uint16_t block_id(Block b) {
	return b & BLOCK_ID_MASK;
}

constexpr BlockType type(Block b) {
	return static_cast<BlockType>(block_id(b));
}

constexpr bool is_air(Block b) {
	return block_id(b) == 0;
}

constexpr Block make_block(BlockType type) {
	return static_cast<Block>(type);
}

} // namespace voxel

#endif
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
constexpr Block BLOCK_FLAG_CUTOUT      = Block{1u} << 14;
constexpr int WATER_TEXTURE_LAYER = 18;

constexpr uint16_t block_id(Block b) {
	return b & BLOCK_ID_MASK;
}

constexpr BlockType type(Block b) {
	return static_cast<BlockType>(block_id(b));
}

constexpr bool has_flag(Block b, Block flag) {
	return (b & flag) != 0;
}

constexpr bool is_air(Block b) {
	return block_id(b) == 0;
}

constexpr bool is_transparent(Block b) {
	// Cutout blocks (such as leaves) stay in the opaque pass and use alpha scissor.
	// Keep older saved/manual water blocks working even if they lack the flag.
	return has_flag(b, BLOCK_FLAG_TRANSPARENT) || type(b) == BlockType::WATER;
}

constexpr bool is_collidable(Block b) {
	if (is_air(b) || is_transparent(b)) {
		return false;
	}
	const Block behavior_flags = b & (BLOCK_FLAG_SOLID | BLOCK_FLAG_TRANSPARENT);
	return behavior_flags == 0 || has_flag(b, BLOCK_FLAG_SOLID);
}

constexpr Block make_block(BlockType type, Block flags = 0) {
	if (type == BlockType::LEAVES) {
		flags |= BLOCK_FLAG_CUTOUT;
	}
	if (type != BlockType::AIR && !has_flag(flags, BLOCK_FLAG_TRANSPARENT)) {
		flags |= BLOCK_FLAG_SOLID;
	}
	return static_cast<Block>(type) | flags;
}

} // namespace voxel

#endif

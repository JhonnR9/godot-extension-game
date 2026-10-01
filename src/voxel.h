#ifndef BLOCK_H
#define BLOCK_H

#include "voxel_types.h"
#include "../project/generated/block_registry.generated.h"

namespace voxel {

using Block = uint32_t;

constexpr uint32_t BLOCK_ID_MASK = 0x3FF; // 10 bits
constexpr Block BLOCK_FLAG_SOLID       = Block{1u} << 10;
constexpr Block BLOCK_FLAG_TRANSPARENT = Block{1u} << 11;
constexpr Block BLOCK_FLAG_EMISSIVE    = Block{1u} << 12;
constexpr Block BLOCK_FLAG_WATERLOG    = Block{1u} << 13;
constexpr Block BLOCK_FLAG_CUTOUT      = Block{1u} << 14;
constexpr Block BLOCK_FLAG_OCEAN       = Block{1u} << 15;
constexpr Block BLOCK_FLAG_CROSSED     = Block{1u} << 16;

constexpr uint16_t block_id(Block b) {
	return b & BLOCK_ID_MASK;
}

constexpr uint16_t type(Block b) {
	return block_id(b);
}

constexpr bool has_flag(Block b, Block flag) {
	return (b & flag) != 0;
}

constexpr bool is_air(Block b) {
	return block_id(b) == 0;
}

constexpr bool is_transparent(Block b) {
	return has_flag(b, BLOCK_FLAG_TRANSPARENT) ||
			has_flag(default_block_flags(block_id(b)), BLOCK_FLAG_TRANSPARENT);
}

constexpr bool is_cutout(Block b) {
	return has_flag(b, BLOCK_FLAG_CUTOUT) ||
			has_flag(default_block_flags(block_id(b)), BLOCK_FLAG_CUTOUT);
}

constexpr bool is_collidable(Block b) {
	return !is_air(b) && !is_transparent(b) &&
			has_flag(default_block_flags(block_id(b)), BLOCK_FLAG_SOLID);
}

constexpr Block make_block(uint16_t id, Block flags = 0) {
	if (id == block_ids::air || id > BLOCK_ID_MASK) return 0;
	return static_cast<Block>(id) | default_block_flags(id) | flags;
}

} // namespace voxel

#endif

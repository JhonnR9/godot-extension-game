//
// Created by jhone on 10/05/2026.
//

#ifndef VOXEL_TYPES_H
#define VOXEL_TYPES_H

#include <cstdint>
#include <godot_cpp/variant/color.hpp>

namespace godot {

enum class BlockType : uint8_t {
	AIR = 0,
	GRASS,
	DIRT,
	STONE,
	DEEPSLATE,
	IRON_ORE,
	DIAMOND_ORE
};

enum class CubeFace : uint8_t {
	F,
	B,
	L,
	R,
	U,
	D
};

inline Color get_block_color(BlockType type) {
	switch (type) {
		case BlockType::GRASS:
			return Color(0.2, 0.8, 0.2);
		case BlockType::DEEPSLATE:
			return Color(0.2, 0.2, 0.25);
		case BlockType::IRON_ORE:
			return Color(0.7, 0.5, 0.4);
		case BlockType::DIRT:
			return Color(0.588f, 0.447f, 0.31f, 1.0f);
		case BlockType::STONE:
			return Color(0.6f, 0.6f, 0.6f, 1.0f);
		case BlockType::AIR:
		default:
			return Color(1.0f, 1.0f, 1.0f, 1.0f);
	}
}

} //namespace godot

#endif //VOXEL_TYPES_H

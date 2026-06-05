//
// Created by jhone on 10/05/2026.
//

#ifndef VOXEL_TYPES_H
#define VOXEL_TYPES_H

#include "godot_cpp/classes/ref.hpp"

#include <cstdint>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/variant/color.hpp>

namespace godot {

enum class BlockType : uint16_t {
	AIR = 0,
	GRASS,
	DIRT,
	STONE,
	WOOD,
	DEEPSLATE,
	IRON_ORE,
	DIAMOND_ORE,
	LOG,
	LEAVES
};

enum class CubeFace : uint8_t {
	F,
	B,
	L,
	R,
	U,
	D
};

struct WorldModel {
	int32_t seed;
	String name;
	int64_t id;
};

} //namespace godot

#endif //VOXEL_TYPES_H

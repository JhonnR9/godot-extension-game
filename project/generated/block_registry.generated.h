#ifndef BLOCK_REGISTRY_GENERATED_H
#define BLOCK_REGISTRY_GENERATED_H

#include <cstdint>
#include <string_view>

namespace voxel {
namespace block_ids {
inline constexpr std::uint16_t air = 0;
inline constexpr std::uint16_t grass = 1;
inline constexpr std::uint16_t dirt = 2;
inline constexpr std::uint16_t stone = 3;
inline constexpr std::uint16_t wood = 4;
inline constexpr std::uint16_t deepslate = 5;
inline constexpr std::uint16_t iron_ore = 6;
inline constexpr std::uint16_t diamond_ore = 7;
inline constexpr std::uint16_t log = 8;
inline constexpr std::uint16_t leaves = 9;
inline constexpr std::uint16_t water = 10;
inline constexpr std::uint16_t sand = 11;
inline constexpr std::uint16_t sandstone = 12;
inline constexpr std::uint16_t cactus = 13;
inline constexpr std::uint16_t flower = 14;
inline constexpr std::uint16_t tall_grass = 15;
inline constexpr std::uint16_t bedrock = 16;
inline constexpr std::uint16_t daisy = 17;
inline constexpr std::uint16_t cornflower = 18;
inline constexpr std::uint16_t poppy = 19;
inline constexpr std::uint16_t short_grass = 20;
inline constexpr std::uint16_t fern = 21;
inline constexpr std::uint16_t coal_ore = 22;
}

inline constexpr std::uint32_t default_block_flags(std::uint16_t id) {
	switch (id) {
		case block_ids::air: return 0u;
		case block_ids::grass: return 1024u;
		case block_ids::dirt: return 1024u;
		case block_ids::stone: return 1024u;
		case block_ids::wood: return 1024u;
		case block_ids::deepslate: return 1024u;
		case block_ids::iron_ore: return 1024u;
		case block_ids::diamond_ore: return 1024u;
		case block_ids::log: return 1024u;
		case block_ids::leaves: return 17408u;
		case block_ids::water: return 34816u;
		case block_ids::sand: return 1024u;
		case block_ids::sandstone: return 1024u;
		case block_ids::cactus: return 1024u;
		case block_ids::flower: return 81920u;
		case block_ids::tall_grass: return 81920u;
		case block_ids::bedrock: return 1024u;
		case block_ids::daisy: return 81920u;
		case block_ids::cornflower: return 81920u;
		case block_ids::poppy: return 81920u;
		case block_ids::short_grass: return 81920u;
		case block_ids::fern: return 81920u;
		case block_ids::coal_ore: return 1024u;
		default: return 0u;
	}
}

inline constexpr std::uint16_t block_id_from_name(std::string_view name) {
	if (name == "air") return block_ids::air;
	if (name == "grass") return block_ids::grass;
	if (name == "dirt") return block_ids::dirt;
	if (name == "stone") return block_ids::stone;
	if (name == "wood") return block_ids::wood;
	if (name == "deepslate") return block_ids::deepslate;
	if (name == "iron_ore") return block_ids::iron_ore;
	if (name == "diamond_ore") return block_ids::diamond_ore;
	if (name == "log") return block_ids::log;
	if (name == "leaves") return block_ids::leaves;
	if (name == "water") return block_ids::water;
	if (name == "sand") return block_ids::sand;
	if (name == "sandstone") return block_ids::sandstone;
	if (name == "cactus") return block_ids::cactus;
	if (name == "flower") return block_ids::flower;
	if (name == "tall_grass") return block_ids::tall_grass;
	if (name == "bedrock") return block_ids::bedrock;
	if (name == "daisy") return block_ids::daisy;
	if (name == "cornflower") return block_ids::cornflower;
	if (name == "poppy") return block_ids::poppy;
	if (name == "short_grass") return block_ids::short_grass;
	if (name == "fern") return block_ids::fern;
	if (name == "coal_ore") return block_ids::coal_ore;
	return 0xffff;
}

inline constexpr int texture_layer_from_name(std::string_view name) {
	if (name == "cactus_side") return 0;
	if (name == "coal_ore") return 1;
	if (name == "cornflower_side") return 2;
	if (name == "daisy_side") return 3;
	if (name == "diamond_ore") return 4;
	if (name == "dirt_down") return 5;
	if (name == "dirt_side") return 6;
	if (name == "dirt_top") return 7;
	if (name == "fern_side") return 8;
	if (name == "flower_side") return 9;
	if (name == "grass_bottom") return 10;
	if (name == "grass_side") return 11;
	if (name == "grass_top") return 12;
	if (name == "iron_ore") return 13;
	if (name == "leaves_bottom") return 14;
	if (name == "leaves_side") return 15;
	if (name == "leaves_top") return 16;
	if (name == "log_side") return 17;
	if (name == "oak_log_top") return 18;
	if (name == "poppy_side") return 19;
	if (name == "sand_side") return 20;
	if (name == "sandstone_side") return 21;
	if (name == "short_grass_side") return 22;
	if (name == "stone") return 23;
	if (name == "tallgrass_side") return 24;
	if (name == "wood_bottom") return 25;
	if (name == "wood_side") return 26;
	if (name == "wood_top") return 27;
	return -1;
}

inline constexpr int WATER_TEXTURE_LAYER = 28;
}

#endif

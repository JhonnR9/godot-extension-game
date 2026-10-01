#include "chunk_generator.h"

#include <godot_cpp/core/math.hpp>

namespace godot {

void BiomeSelectionPass::apply(ChunkGenerationContext &context) const {
	const TerrainSettings &settings = context.settings;

	for (int z = 0; z < Chunk::SIZE_Z; ++z) {
		for (int x = 0; x < Chunk::SIZE_X; ++x) {
			const int32_t wx = context.world_x(x);
			const int32_t wz = context.world_z(z);
			ColumnGenerationData &column = context.column(x, z);
			const float desert = context.desert_weight_at(wx, wz);
			column.desert_weight = desert;
			column.ocean_weight = settings.ocean_noise.is_valid() ? settings.ocean_noise->get_noise_2d(wx, wz) : 0.0f;
			column.river_weight = settings.river_noise.is_valid() ? settings.river_noise->get_noise_2d(wx, wz) : 1.0f;
			const float terrain = settings.terrain_noise.is_valid() ? settings.terrain_noise->get_noise_2d(wx, wz) : 0.0f;
			const float mountain = settings.mountain_noise.is_valid() ? settings.mountain_noise->get_noise_2d(wx, wz) : terrain;
			const float mountain_ridge = 1.0f - Math::abs(mountain);
			const float dune = settings.dune_noise.is_valid() ? settings.dune_noise->get_noise_2d(wx, wz) : 0.0f;
			const float mountain_offset = Math::pow(mountain_ridge, 2.0f) * 19.0f - 5.0f;
			const float dune_offset = (1.0f - Math::abs(dune)) * 4.0f - 1.5f;
			column.height_offset = Math::round(Math::lerp(mountain_offset, dune_offset, desert));
			column.height_scale = Math::lerp(2.4f, 0.32f, desert);
			// Sea level is a world-wide fixed height. Biomes can change terrain,
			// but must not raise the water surface above this elevation.
			column.water_level = settings.water_level;
			column.trees_allowed = desert < 0.45f;
			if (desert > 0.68f) {
				column.biome_id = static_cast<uint16_t>(BiomeId::DESERT);
				column.surface_block = voxel::make_block(voxel::BlockType::SAND);
				column.subsurface_block = voxel::make_block(voxel::BlockType::SAND);
				column.stone_block = voxel::make_block(voxel::BlockType::SANDSTONE);
				column.subsurface_depth = 4;
			} else if (desert > 0.34f) {
				// A broad scrubby transition belt blends the two palettes.
				column.surface_block = voxel::make_block(voxel::BlockType::DIRT);
				column.subsurface_depth = 7;
			}
		}
	}
}

void VegetationGenerationPass::apply(ChunkGenerationContext &context) const {
	const int32_t origin_x = context.chunk_position.x * Chunk::SIZE_X;
	const int32_t origin_y = context.chunk_position.y * Chunk::SIZE_Y;
	const int32_t origin_z = context.chunk_position.z * Chunk::SIZE_Z;
	auto hash = [this](int32_t x, int32_t z) {
		uint64_t value = static_cast<uint64_t>(_seed) ^ (static_cast<uint64_t>(static_cast<int64_t>(x)) * 0x9E3779B97F4A7C15ULL) ^
				(static_cast<uint64_t>(static_cast<int64_t>(z)) * 0xC2B2AE3D27D4EB4FULL);
		value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
		value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
		return value ^ (value >> 31);
	};
	for (int z = 0; z < Chunk::SIZE_Z; ++z) {
		for (int x = 0; x < Chunk::SIZE_X; ++x) {
			const auto &column = context.column(x, z);
			const int32_t wx = origin_x + x;
			const int32_t wz = origin_z + z;
			const int32_t y = column.surface_height + 1;
			if (y <= column.water_level) continue;
			const uint64_t roll = hash(wx, wz) % 1000;
			if (column.desert_weight > 0.97f && column.biome_id == static_cast<uint16_t>(BiomeId::DESERT) &&
					voxel::type(column.surface_block) == voxel::BlockType::SAND) {
				if (roll >= 2) continue;
				if (y >= origin_y && y < origin_y + Chunk::SIZE_Y) {
					const int ground_y = column.surface_height - origin_y;
					const int plant_y = y - origin_y;
					if (ground_y < 0 || ground_y >= Chunk::SIZE_Y ||
							voxel::type(context.chunk.get_block(x, ground_y, z)) != voxel::BlockType::SAND ||
							!voxel::is_air(context.chunk.get_block(x, plant_y, z))) {
						continue;
					}
				}
				const int height = 1 + static_cast<int>((hash(wx ^ 0x5A5A, wz) >> 8) % 3);
				for (int dy = 0; dy < height && y + dy < origin_y + Chunk::SIZE_Y; ++dy) {
					if (y + dy < origin_y) continue;
					context.write_block(x, y + dy - origin_y, z,
							voxel::make_block(voxel::BlockType::CACTUS), GenerationLayer::VEGETATION);
				}
			} else if (column.desert_weight < 0.34f && roll < 58 && y >= origin_y && y < origin_y + Chunk::SIZE_Y) {
				const voxel::BlockType plant = roll < 28 ? voxel::BlockType::FLOWER : voxel::BlockType::TALL_GRASS;
				if (voxel::is_air(context.chunk.get_block(x, y - origin_y, z))) {
					context.write_block(x, y - origin_y, z,
							voxel::make_block(plant, voxel::BLOCK_FLAG_CUTOUT), GenerationLayer::VEGETATION);
				}
			}
		}
	}
}

void TerrainSurfacePass::apply(ChunkGenerationContext &context) const {
	const TerrainSettings &settings = context.settings;
	for (int z = 0; z < Chunk::SIZE_Z; ++z) {
		for (int x = 0; x < Chunk::SIZE_X; ++x) {
			ColumnGenerationData &column = context.column(x, z);
			const int32_t wx = context.world_x(x);
			const int32_t wz = context.world_z(z);
			const float noise = settings.terrain_noise.is_valid()
					? settings.terrain_noise->get_noise_2d(wx, wz)
					: 0.0f;
			column.surface_height = settings.terrain_base_height + column.height_offset +
					Math::round(noise * settings.terrain_amplitude * column.height_scale);
			const bool is_desert = column.biome_id == static_cast<uint16_t>(BiomeId::DESERT);
			const bool is_macro_ocean = column.ocean_weight < -0.28f;
			if (!is_desert && (is_macro_ocean || column.surface_height <= column.water_level - 4)) {
				if (is_macro_ocean) {
					column.surface_height = MIN(column.surface_height, column.water_level - 6);
				}
				column.biome_id = static_cast<uint16_t>(BiomeId::OCEAN);
				column.surface_block = voxel::make_block(voxel::BlockType::SAND);
				column.subsurface_block = voxel::make_block(voxel::BlockType::SAND);
				column.subsurface_depth = 5;
				column.trees_allowed = false;
			} else if (!is_desert && Math::abs(column.river_weight) < 0.035f &&
					column.surface_height <= column.water_level + 2) {
				column.surface_height = MIN(column.surface_height, column.water_level - 2);
				column.biome_id = static_cast<uint16_t>(BiomeId::RIVER);
				column.surface_block = voxel::make_block(voxel::BlockType::SAND);
				column.subsurface_block = voxel::make_block(voxel::BlockType::SAND);
				column.subsurface_depth = 2;
				column.trees_allowed = false;
			}

			for (int y = 0; y < Chunk::SIZE_Y; ++y) {
				const int32_t wy = context.world_y(y);
				if (wy > column.surface_height) {
					continue;
				}

				const int32_t depth = column.surface_height - wy;
				voxel::Block block = column.stone_block;
				if (depth == 0) {
					block = column.surface_block;
				} else if (depth <= column.subsurface_depth) {
					block = column.subsurface_block;
				} else if (wy < -32) {
					block = column.deep_stone_block;
				}
				context.write_block(x, y, z, block, GenerationLayer::TERRAIN);
			}
		}
	}
}

void CaveCarvingPass::apply(ChunkGenerationContext &context) const {
	if (context.settings.cave_noise.is_null()) {
		return;
	}

	for (int z = 0; z < Chunk::SIZE_Z; ++z) {
		for (int x = 0; x < Chunk::SIZE_X; ++x) {
			const int32_t wx = context.world_x(x);
			const int32_t wz = context.world_z(z);
			const ColumnGenerationData &column = context.column(x, z);
			for (int y = 0; y < Chunk::SIZE_Y; ++y) {
				const int32_t wy = context.world_y(y);
				voxel::Block block = context.chunk.get_block(x, y, z);
				if (voxel::is_air(block)) {
					continue;
				}

				const int32_t depth = column.surface_height - wy;
				const float cave = context.settings.cave_noise->get_noise_3d(wx, wy, wz);
				const float cave_mask = Math::clamp(static_cast<float>(depth) / 6.0f, 0.0f, 1.0f);
				const float threshold = Math::lerp(1.0f, context.settings.cave_threshold, cave_mask);
				if (cave > threshold) {
					context.write_block(x, y, z, 0, GenerationLayer::CARVING);
				}
			}
		}
	}
}

void WaterFillPass::apply(ChunkGenerationContext &context) const {
	for (int z = 0; z < Chunk::SIZE_Z; ++z) {
		for (int x = 0; x < Chunk::SIZE_X; ++x) {
			const ColumnGenerationData &column = context.column(x, z);
			// Water bodies belong to lowland/plains terrain only. Leave the
			// desert dry and ignore shallow one-block dips in the ground.
			if (column.biome_id == static_cast<uint16_t>(BiomeId::DESERT) ||
					column.surface_height >= column.water_level - 1) {
				continue;
			}
			for (int y = 0; y < Chunk::SIZE_Y; ++y) {
				const int32_t wy = context.world_y(y);
				// Water occupies blocks below sea level so its top mesh lands
				// exactly on the sea-level plane instead of one block above it.
				if (wy > column.surface_height && wy < column.water_level &&
						voxel::is_air(context.chunk.get_block(x, y, z))) {
					const voxel::Block water_flags = voxel::BLOCK_FLAG_TRANSPARENT |
							(column.biome_id == static_cast<uint16_t>(BiomeId::OCEAN) ? voxel::BLOCK_FLAG_OCEAN : 0);
					context.write_block(x, y, z,
							voxel::make_block(voxel::BlockType::WATER, water_flags), GenerationLayer::WATER);
				}
			}
		}
	}
}

Chunk ChunkGenerator::generate(const Vector3i &chunk_pos, const TerrainSettings &settings,
		const ChunkGenerationPipeline &pipeline) {
	return pipeline.generate(chunk_pos, settings);
}

} // namespace godot

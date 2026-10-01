#include "chunk_generator.h"

#include <godot_cpp/core/math.hpp>

namespace godot {

void BiomeSelectionPass::apply(ChunkGenerationContext &context) const {
	const TerrainSettings &settings = context.settings;
	if (settings.biome_noise.is_null()) {
		return;
	}

	for (int z = 0; z < Chunk::SIZE_Z; ++z) {
		for (int x = 0; x < Chunk::SIZE_X; ++x) {
			const int32_t wx = context.world_x(x);
			const int32_t wz = context.world_z(z);
			ColumnGenerationData &column = context.column(x, z);
			const float biome_sample = settings.biome_noise->get_noise_2d(wx, wz);
			if (biome_sample < 0.28f) {
				continue;
			}

			column.biome_id = static_cast<uint16_t>(BiomeId::DESERT);
			column.surface_block = voxel::make_block(voxel::BlockType::SAND);
			column.subsurface_block = voxel::make_block(voxel::BlockType::SAND);
			column.stone_block = voxel::make_block(voxel::BlockType::SANDSTONE);
			column.subsurface_depth = 4;
			column.water_level = settings.terrain_base_height - 8;
			column.height_scale = 0.32f;
			column.trees_allowed = false;

			if (settings.dune_noise.is_valid()) {
				const float dune_sample = settings.dune_noise->get_noise_2d(wx, wz);
				const float ridge = 1.0f - Math::abs(dune_sample);
				column.height_offset = Math::round(ridge * 4.0f - 1.5f);
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
				context.chunk.set_block(x, y, z, block);
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
					context.chunk.set_block(x, y, z, 0);
				}
			}
		}
	}
}

void WaterFillPass::apply(ChunkGenerationContext &context) const {
	for (int z = 0; z < Chunk::SIZE_Z; ++z) {
		for (int x = 0; x < Chunk::SIZE_X; ++x) {
			const ColumnGenerationData &column = context.column(x, z);
			for (int y = 0; y < Chunk::SIZE_Y; ++y) {
				const int32_t wy = context.world_y(y);
				if (wy > column.surface_height && wy <= column.water_level &&
						voxel::is_air(context.chunk.get_block(x, y, z))) {
					context.chunk.set_block(x, y, z,
							voxel::make_block(voxel::BlockType::WATER, voxel::BLOCK_FLAG_TRANSPARENT));
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

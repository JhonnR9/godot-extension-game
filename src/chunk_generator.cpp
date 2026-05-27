#include "chunk_generator.h"

namespace godot {

ChunkModel ChunkGenerator::generate(const Vector3i chunk_pos,const TerrainSettings &terrain_settings) {
	ChunkModel chunk{};

	Ref<FastNoiseLite> terrain_noise = terrain_settings.noise_set.terrain_noise;
	Ref<FastNoiseLite> cave_noise = terrain_settings.noise_set.cave_noise;

	for (int z = 0; z < ChunkModel::SIZE_Z; z++) {
		for (int y = 0; y < ChunkModel::SIZE_Y; y++) {
			for (int x = 0; x < ChunkModel::SIZE_X; x++) {
				int world_x = chunk_pos.x * ChunkModel::SIZE_X + x;
				int world_y = chunk_pos.y * ChunkModel::SIZE_Y + y;
				int world_z = chunk_pos.z * ChunkModel::SIZE_Z + z;

				float terrain_n = terrain_noise->get_noise_2d(world_x, world_z);

				int terrain_height = terrain_settings.terrain_base_height + Math::round(terrain_n * terrain_settings.terrain_amplitude);

				voxel::Block block = 0;

				if (world_y <= terrain_height) {
					float cave = cave_noise->get_noise_3d(world_x, world_y, world_z);

					int depth = terrain_height - world_y;

					float cave_mask = Math::clamp(static_cast<float>(depth) / 6.0f, 0.0f, 1.0f);
					float adjusted_threshold = Math::lerp(1.0f, terrain_settings.cave_threshold, cave_mask);

						if (cave <= adjusted_threshold) {
							if (depth == 0) {
								block = static_cast<uint32_t>(BlockType::GRASS);

							} else if (depth <= 15) {
								block = static_cast<uint32_t>(BlockType::DIRT);

							} else {
								block = static_cast<uint32_t>(world_y < -32 ? BlockType::DEEPSLATE : BlockType::STONE);
							}
						}
				}

				chunk.set_block(x, y, z, block);
			}
		}
	}

	return chunk;
}

} //namespace godot
#include "chunk_generator.h"

namespace godot {
ChunkModel ChunkGenerator::generate(const Vector3i chunk_pos, const TerrainSettings &terrain_settings) {
    ChunkModel chunk{};
    Ref<FastNoiseLite> terrain_noise = terrain_settings.noise_set.terrain_noise;
    Ref<FastNoiseLite> cave_noise = terrain_settings.noise_set.cave_noise;

    for (int x = 0; x < ChunkModel::SIZE; x++) {
       for (int z = 0; z < ChunkModel::SIZE; z++) {
          int world_x = chunk_pos.x * ChunkModel::SIZE + x;
          int world_z = chunk_pos.z * ChunkModel::SIZE + z;

          float terrain_n = terrain_noise->get_noise_2d(world_x, world_z);
          int terrain_height = terrain_settings.terrain_base_height + Math::round(terrain_n * terrain_settings.terrain_amplitude);

          for (int y = 0; y < ChunkModel::SIZE; y++) {
             int world_y = chunk_pos.y * ChunkModel::SIZE + y;
             Block &block = chunk._blocks[x][y][z];
             block.type = BlockType::AIR;

             if (world_y > terrain_height) continue;

             float cave_sample = cave_noise->get_noise_3d(world_x, world_y, world_z);

             int depth_from_surface = terrain_height - world_y;
             float cave_mask = Math::clamp((float)depth_from_surface / 6.0f, 0.0f, 1.0f);

             if (cave_sample * cave_mask > terrain_settings.cave_threshold) {
                continue;
             }

             if (depth_from_surface == 0) {
                block.type = BlockType::GRASS;
             } else if (depth_from_surface <= 4) {
                block.type = BlockType::DIRT;
             } else {
                block.type = world_y < -32 ? BlockType::DEEPSLATE : BlockType::STONE;
             }
          }
       }
    }
    return chunk;
}
} //namespace godot
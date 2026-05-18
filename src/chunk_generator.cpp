#include "chunk_generator.h"

namespace godot {

void ChunkGenerator::generate_tree(ChunkModel &chunk, int start_x, int start_y, int start_z) {
	int tree_height = 5;


	for (int i = 0; i < tree_height; i++) {
		int y = start_y + i;
		if (y < ChunkModel::SIZE) {
			chunk._blocks[start_x][y][start_z].type = BlockType::LOG;
		}
	}


	int leaf_top = start_y + tree_height;

	for (int ly = leaf_top - 2; ly <= leaf_top + 1; ly++) {
		if (ly >= ChunkModel::SIZE || ly < 0)
			continue;

		const int radius = (ly >= leaf_top) ? 1 : 2;

		for (int lx = start_x - radius; lx <= start_x + radius; lx++) {
			for (int lz = start_z - radius; lz <= start_z + radius; lz++) {
				if (lx >= 0 && lx < ChunkModel::SIZE && lz >= 0 && lz < ChunkModel::SIZE) {

					if (lx == start_x && lz == start_z && ly < leaf_top) continue;

					if (chunk._blocks[lx][ly][lz].type == BlockType::AIR) {
						chunk._blocks[lx][ly][lz].type = BlockType::LEAVES;
					}
				}
			}
		}
	}
}

ChunkModel ChunkGenerator::generate(const Vector3i chunk_pos, const TerrainSettings &terrain_settings) {
    ChunkModel chunk{};
    Ref<FastNoiseLite> terrain_noise = terrain_settings.noise_set.terrain_noise;
    Ref<FastNoiseLite> cave_noise = terrain_settings.noise_set.cave_noise;

    std::vector<Vector3i> tree_positions;

    for (int x = 0; x < ChunkModel::SIZE; x++) {
       for (int z = 0; z < ChunkModel::SIZE; z++) {
          int world_x = chunk_pos.x * ChunkModel::SIZE + x;
          int world_z = chunk_pos.z * ChunkModel::SIZE + z;

          float terrain_n = terrain_noise->get_noise_2d(world_x, world_z);
          int terrain_height = terrain_settings.terrain_base_height + Math::round(terrain_n * terrain_settings.terrain_amplitude);

          bool local_tree_spawn = false;
          if (x > 2 && x < ChunkModel::SIZE - 3 && z > 2 && z < ChunkModel::SIZE - 3) {

              uint32_t hash = (world_x * 73856093) ^ (world_z * 19349663);
              if (hash % 100 < 2) {
                  local_tree_spawn = true;
              }
          }

          for (int y = 0; y < ChunkModel::SIZE; y++) {
             int world_y = chunk_pos.y * ChunkModel::SIZE + y;
             Block &block = chunk._blocks[x][y][z];
             block.type = BlockType::AIR;

             if (world_y > terrain_height) continue;

             float cave_sample = cave_noise->get_noise_3d(world_x, world_y, world_z);
             int depth_from_surface = terrain_height - world_y;
             float cave_mask = Math::clamp(static_cast<float>(depth_from_surface) / 6.0f, 0.0f, 1.0f);

             if (cave_sample * cave_mask > terrain_settings.cave_threshold) {
                continue;
             }

             if (depth_from_surface == 0) {
                block.type = BlockType::GRASS;
                if (local_tree_spawn) {
                   // tree_positions.push_back(Vector3i(x, y, z));
                }
             }/* else if (depth_from_surface <= 4) {
                block.type = BlockType::DIRT;
             } else {
                block.type = world_y < -32 ? BlockType::DEEPSLATE : BlockType::STONE;
             }*/
          }
       }
    }

   /* for (const Vector3i &tree_pos : tree_positions) {
        generate_tree(chunk, tree_pos.x, tree_pos.y + 1, tree_pos.z);
    }*/

    return chunk;
}
} //namespace godot
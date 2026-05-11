#include "chunk.h"

#include "world.h"

namespace godot {

void Chunk::_ready() {
}

bool Chunk::_is_air(int x, int y, int z) const {
	if (x >= 0 && x < SIZE && y >= 0 && y < SIZE && z >= 0 && z < SIZE) {
		return _blocks[x][y][z].is_air();
	}

	int wx = get_position().x + x;
	int wy = get_position().y + y;
	int wz = get_position().z + z;

	return _world->is_air_global(wx, wy, wz);
}

void Chunk::rebuild_mesh() {
    _mesher.clear();

    for (int x = 0; x < SIZE; x++) {
       for (int y = 0; y < SIZE; y++) {
          for (int z = 0; z < SIZE; z++) {
             const Block &block = _blocks[x][y][z];

             if (block.is_air()) {
                continue;
             }

             Color block_color = get_block_color(block.type);
             Vector3 pos(x, y, z);

             if (_is_air(x, y, z + 1)) _mesher.add_face(CubeFace::F, pos, block_color);
             if (_is_air(x, y, z - 1)) _mesher.add_face(CubeFace::B, pos, block_color);
             if (_is_air(x - 1, y, z)) _mesher.add_face(CubeFace::L, pos, block_color);
             if (_is_air(x + 1, y, z)) _mesher.add_face(CubeFace::R, pos, block_color);
             if (_is_air(x, y + 1, z)) _mesher.add_face(CubeFace::U, pos, block_color);
             if (_is_air(x, y - 1, z)) _mesher.add_face(CubeFace::D, pos, block_color);
          }
       }
    }

    Array arrays = _mesher.build_arrays();

    if (Array(arrays[Mesh::ARRAY_VERTEX]).size() == 0) {
        if (_mesh.is_valid()) {
            _mesh->clear_surfaces();
        }
        set_mesh(nullptr);
        return;
    }

    if (!_mesh.is_valid()) {
       _mesh.instantiate();
    }

    _mesh->clear_surfaces();


    _mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

    set_mesh(_mesh);

    if (!_material.is_valid()) {
       _material.instantiate();
       _material->set_texture_filter(BaseMaterial3D::TEXTURE_FILTER_NEAREST);
       _material->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
       _material->set_shading_mode(BaseMaterial3D::SHADING_MODE_PER_PIXEL);
    }


    set_surface_override_material(0, _material);
}

void Chunk::generate(
	   uint64_t seed,
	   Vector3i chunk_pos,
	   Ref<FastNoiseLite> terrain_noise,
	   Ref<FastNoiseLite> cave_noise,
	   int terrain_base_height,
	   float terrain_amplitude) {


	if (terrain_noise.is_null() || cave_noise.is_null()) {
		return;
	}


	 terrain_noise->set_seed(static_cast<int>(seed));

	for (int x = 0; x < SIZE; x++) {
		for (int z = 0; z < SIZE; z++) {
			int world_x = chunk_pos.x * SIZE + x;
			int world_z = chunk_pos.z * SIZE + z;

			float n = terrain_noise->get_noise_2d(world_x, world_z);
			int terrain_height = terrain_base_height + static_cast<int>(n * terrain_amplitude);

			for (int y = 0; y < SIZE; y++) {
				int world_y = chunk_pos.y * SIZE + y;
				Block &block = _blocks[x][y][z];

				if (world_y <= terrain_height) {
					// Lógica de camadas (Stone, Dirt, etc)
					block.type = (world_y == terrain_height) ? BlockType::GRASS : BlockType::STONE;

					// Lógica de Cavernas usando o recurso do Inspetor
					float c = cave_noise->get_noise_3d(world_x, world_y, world_z);
					if (c > 0.2f) { // O threshold também poderia ser uma propriedade do World
						block.type = BlockType::AIR;
					}
				}
			}
		}
	}
}

void Chunk::_generate_resources(Block &block, int wx, int wy, int wz, uint64_t seed) {
	// Usamos um ruído de frequência muito alta para "pontos" de minério
	// Ou uma função de hash baseada na posição
	float ore_sample = static_cast<float>(Math::fmod(Math::abs(Math::sin(wx * 12.9898 + wy * 78.233 + wz * 45.164) * 43758.5453), 1.0));

	// Camada de Ferro (exemplo: entre altura -10 e 20)
	if (wy > -10 && wy < 20) {
		if (ore_sample < 0.02f) block.type = BlockType::IRON_ORE;
	}

	// Camada de Diamante (exemplo: abaixo de -40)
	if (wy < -40) {
		if (ore_sample < 0.005f) block.type = BlockType::DIAMOND_ORE;
	}
}

void Chunk::_bind_methods() {
}

} //namespace godot
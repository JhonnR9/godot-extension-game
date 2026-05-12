#include "chunk.h"
#include <godot_cpp/classes/static_body3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/concave_polygon_shape3d.hpp>
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

	// 1. Criar ou limpar o StaticBody3D (o "corpo" físico)
	StaticBody3D *static_body = Object::cast_to<StaticBody3D>(get_node_or_null("StaticBody3D"));

	if (Array(arrays[Mesh::ARRAY_VERTEX]).size() == 0) {
		if (static_body) static_body->queue_free();
		return;
	}

	if (!static_body) {
		static_body = memnew(StaticBody3D);
		static_body->set_name("StaticBody3D");
		add_child(static_body);
	}

	CollisionShape3D *collision_shape = Object::cast_to<CollisionShape3D>(static_body->get_node_or_null("CollisionShape3D"));
	if (!collision_shape) {
		collision_shape = memnew(CollisionShape3D);
		collision_shape->set_name("CollisionShape3D");
		static_body->add_child(collision_shape);
	}

	Ref<ConcavePolygonShape3D> shape = memnew(ConcavePolygonShape3D);
	shape->set_faces(_mesh->get_faces());
	collision_shape->set_shape(shape);

    if (!_material.is_valid()) {
       _material.instantiate();
       _material->set_texture_filter(BaseMaterial3D::TEXTURE_FILTER_NEAREST);
       _material->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
       _material->set_shading_mode(BaseMaterial3D::SHADING_MODE_PER_VERTEX);
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
	cave_noise->set_seed(static_cast<int>(seed + 1337));

	for (int x = 0; x < SIZE; x++) {
		for (int z = 0; z < SIZE; z++) {

			int world_x = chunk_pos.x * SIZE + x;
			int world_z = chunk_pos.z * SIZE + z;

			float terrain_n = terrain_noise->get_noise_2d(world_x, world_z);

			int terrain_height =
					terrain_base_height +
					static_cast<int>(terrain_n * terrain_amplitude);

			for (int y = 0; y < SIZE; y++) {

				int world_y = chunk_pos.y * SIZE + y;

				Block &block = _blocks[x][y][z];

				block.type = BlockType::AIR;

				// Acima do terreno
				if (world_y > terrain_height) {
					continue;
				}

				// Cavernas
				float cave = cave_noise->get_noise_3d(
						world_x,
						world_y,
						world_z);

				if (cave > 0.35f) {
					continue;
				}

				int depth = terrain_height - world_y;

				// SUPERFÍCIE
				if (depth == 0) {
					block.type = BlockType::GRASS;
				}

				// SUBSOLO
				else if (depth <= 4) {
					block.type = BlockType::DIRT;
				}

				// CAMADA PROFUNDA
				else {

					// DEEPSLATE
					if (world_y < -32) {
						block.type = BlockType::DEEPSLATE;
					} else {
						block.type = BlockType::STONE;
					}

					// MINÉRIOS
					float ore =
							terrain_noise->get_noise_3d(
									world_x * 0.8f,
									world_y * 0.8f,
									world_z * 0.8f);

					// Ferro
					if (ore > 0.55f && world_y < 40) {
						block.type = BlockType::IRON_ORE;
					}

					// Diamante
					if (ore > 0.72f && world_y < -40) {
						block.type = BlockType::DIAMOND_ORE;
					}
				}
			}
		}
	}
}

void Chunk::_generate_resources(Block &block, int wx, int wy, int wz, uint64_t seed) {
	float ore_sample = static_cast<float>(Math::fmod(Math::abs(Math::sin(wx * 12.9898 + wy * 78.233 + wz * 45.164) * 43758.5453), 1.0));

	if (wy > -10 && wy < 20) {
		if (ore_sample < 0.02f) block.type = BlockType::IRON_ORE;
	}

	if (wy < -40) {
		if (ore_sample < 0.005f) block.type = BlockType::DIAMOND_ORE;
	}
}

void Chunk::_bind_methods() {
}

} //namespace godot
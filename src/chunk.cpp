#include "chunk.h"
#include "world.h"

namespace godot {

void Chunk::_ready() {

}

bool Chunk::_is_air(int x, int y, int z) const {
	if (x >= 0 && x < SIZE && y >= 0 && y < SIZE && z >= 0 && z < SIZE) {
		return _blocks[x][y][z].is_air();
	}

	if (!_world) {
		return false;
	}

	int wx = get_position().x + x;
	int wy = get_position().y + y;
	int wz = get_position().z + z;

	return _world->is_air_global(wx, wy, wz);
}
void Chunk::_generate_faces() {
	for (int x = 0; x < SIZE; x++) {
		for (int y = 0; y < SIZE; y++) {
			for (int z = 0; z < SIZE; z++) {
				const Block &block = _blocks[x][y][z];

				if (block.is_air()) {
					continue;
				}

				Color block_color = get_block_color(block.type);
				Vector3 pos(x, y, z);

				if (_is_air(x, y, z + 1))
					_voxel_mesher.add_face(CubeFace::F, pos, block_color);
				if (_is_air(x, y, z - 1))
					_voxel_mesher.add_face(CubeFace::B, pos, block_color);
				if (_is_air(x - 1, y, z))
					_voxel_mesher.add_face(CubeFace::L, pos, block_color);
				if (_is_air(x + 1, y, z))
					_voxel_mesher.add_face(CubeFace::R, pos, block_color);
				if (_is_air(x, y + 1, z))
					_voxel_mesher.add_face(CubeFace::U, pos, block_color);
				if (_is_air(x, y - 1, z))
					_voxel_mesher.add_face(CubeFace::D, pos, block_color);
			}
		}
	}
}
void Chunk::_setup_material() {
	if (!_material.is_valid()) {
		_material.instantiate();
		_material->set_texture_filter(BaseMaterial3D::TEXTURE_FILTER_NEAREST);
		_material->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
		_material->set_shading_mode(BaseMaterial3D::SHADING_MODE_PER_VERTEX);
	}

}

void Chunk::rebuild_mesh() {
	if (!_mesh.is_valid()) _mesh.instantiate();
	if (!_shape.is_valid()) _shape.instantiate();

	if (_collision_shape == nullptr) {
		return;
	}

	_voxel_mesher.clear();
	_generate_faces();

	Array arrays = _voxel_mesher.build_arrays();
	PackedVector3Array vertices = arrays[Mesh::ARRAY_VERTEX];

	// Empty chunk
	if (vertices.is_empty()) {
		_mesh->clear_surfaces();
		_collision_shape->set_disabled(true);
		set_visible(false);
		return;
	}

	// Render + collision setup
	set_visible(true);
	_collision_shape->set_disabled(false);
	_setup_material();
	_mesh->clear_surfaces();
	_mesh->add_surface_from_arrays( Mesh::PRIMITIVE_TRIANGLES, arrays);
	_mesh->surface_set_material(0, _material);
	set_mesh(_mesh);

	_shape->set_faces(_voxel_mesher.get_collision_faces());
	_collision_shape->set_shape(_shape);


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

			int terrain_height = terrain_base_height + static_cast<int>(terrain_n * terrain_amplitude);

			for (int y = 0; y < SIZE; y++) {
				int world_y = chunk_pos.y * SIZE + y;

				Block &block = _blocks[x][y][z];

				block.type = BlockType::AIR;

				if (world_y > terrain_height) {
					continue;
				}

				constexpr float cave_density_threshold = 0.30f;
				if (const float cave = cave_noise->get_noise_3d(world_x, world_y, world_z); cave > cave_density_threshold) {
					continue;
				}

				int depth = terrain_height - world_y;

				if (depth == 0) {
					block.type = BlockType::GRASS;
				}

				else if (depth <= 4) {
					block.type = BlockType::DIRT;
				}

				else {
					if (world_y < -32) {
						block.type = BlockType::DEEPSLATE;
					} else {
						block.type = BlockType::STONE;
					}

					float ore =
							terrain_noise->get_noise_3d(
									world_x * 0.8f,
									world_y * 0.8f,
									world_z * 0.8f);

					if (ore > 0.55f && world_y < 40) {
						block.type = BlockType::IRON_ORE;
					}

					if (ore > 0.72f && world_y < -40) {
						block.type = BlockType::DIAMOND_ORE;
					}
				}
			}
		}
	}
}

void Chunk::_enter_tree() {
	MeshInstance3D::_enter_tree();
	if (!_mesh.is_valid()) {
		_mesh.instantiate();
		set_mesh(_mesh);
	}

	if (!_material.is_valid()) {
		_material.instantiate();
		_material->set_texture_filter(BaseMaterial3D::TEXTURE_FILTER_NEAREST);
		_material->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
		_material->set_shading_mode(BaseMaterial3D::SHADING_MODE_PER_VERTEX);
	}

	_static_body = memnew(StaticBody3D);
	_static_body->set_name("StaticBody3D");
	add_child(_static_body);

	_collision_shape = memnew(CollisionShape3D);
	_collision_shape->set_name("CollisionShape3D");

	_static_body->add_child(_collision_shape);

	_shape.instantiate();
}
void Chunk::_bind_methods() {
}

} //namespace godot
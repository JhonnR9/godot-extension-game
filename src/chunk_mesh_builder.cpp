#include "chunk_mesh_builder.h"
#include "voxel_mesher.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

namespace godot {
bool ChunkMeshBuilder::_is_air(const ChunkNeighbors &n, int x, int y, int z) {
	return voxel::is_air(_get_block(n, x, y, z));
}

voxel::Block ChunkMeshBuilder::_get_block(const ChunkNeighbors &n, int x, int y, int z) {
	if (x >= Chunk::MIN_X && x <= Chunk::MAX_X &&
		y >= Chunk::MIN_Y && y <= Chunk::MAX_Y &&
		z >= Chunk::MIN_Z && z <= Chunk::MAX_Z) {
		return n.center->get_block(x, y, z);
	}

	if (x < Chunk::MIN_X) {
		return n.left ? n.left->get_block(Chunk::MAX_X, y, z) : voxel::Block{0};
	}
	if (x > Chunk::MAX_X) {
		return n.right ? n.right->get_block(Chunk::MIN_X, y, z) : voxel::Block{0};
	}

	if (y < Chunk::MIN_Y) {
		return n.bottom ? n.bottom->get_block(x, Chunk::MAX_Y, z) : voxel::Block{0};
	}
	if (y > Chunk::MAX_Y) {
		return n.top ? n.top->get_block(x, Chunk::MIN_Y, z) : voxel::Block{0};
	}

	if (z < Chunk::MIN_Z) {
		return n.back ? n.back->get_block(x, y, Chunk::MAX_Z) : voxel::Block{0};
	}

	return n.front ? n.front->get_block(x, y, Chunk::MIN_Z) : voxel::Block{0};
}

bool ChunkMeshBuilder::_is_face_visible(const ChunkNeighbors &n, int x, int y, int z, const voxel::Block current_block) {
	const voxel::Block neighbor = _get_block(n, x, y, z);
	return voxel::is_air(neighbor) ||
			(voxel::is_collidable(current_block) &&
					(voxel::is_transparent(neighbor) || voxel::is_cutout(neighbor)));
}

bool ChunkMeshBuilder::_is_crossed_plant(const voxel::Block block) {
	const voxel::BlockType type = voxel::type(block);
	return type == voxel::BlockType::FLOWER || type == voxel::BlockType::TALL_GRASS;
}

VoxelMesher &ChunkMeshBuilder::_get_mesher(const voxel::Block block) {
	return voxel::is_transparent(block) ? transparent_mesher : opaque_mesher;
}

void ChunkMeshBuilder::_add_right_faces(const ChunkNeighbors &neighbors) {
	const Chunk *center = neighbors.center.get();

	const int SX = Chunk::SIZE_X;
	const int SY = Chunk::SIZE_Y;
	const int SZ = Chunk::SIZE_Z;

	bool mask[SY][SZ];
	bool visited[SY][SZ];

	for (int x = 0; x < SX; x++) {
		for (int y = 0; y < SY; y++)
			for (int z = 0; z < SZ; z++) {
				mask[y][z]    = false;
				visited[y][z] = false;
			}

		// build mask
		for (int y = 0; y < SY; y++) {
			for (int z = 0; z < SZ; z++) {
				const voxel::Block block = center->get_block(x, y, z);
				if (voxel::is_air(block) || _is_crossed_plant(block))
					continue;

				if (_is_face_visible(neighbors, x + 1, y, z, block)) {
					mask[y][z] = true;
				}
			}
		}

		// greedy
		for (int y = 0; y < SY; y++) {
			for (int z = 0; z < SZ; z++) {
				if (!mask[y][z] || visited[y][z])
					continue;

				const voxel::Block block    = center->get_block(x, y, z);
				const voxel::BlockType type = voxel::type(block);

				int quad_h = 1; // Y
				int quad_w = 1; // Z

				// expand Z
				while (z + quad_w < SZ) {
					if (!mask[y][z + quad_w] || visited[y][z + quad_w])
						break;

					const voxel::BlockType other =
							voxel::type(center->get_block(x, y, z + quad_w));

					if (other != type)
						break;

					quad_w++;
				}

				// expand Y
				bool can_expand = true;
				while (y + quad_h < SY && can_expand) {
					for (int k = 0; k < quad_w; k++) {
						if (!mask[y + quad_h][z + k] ||
							visited[y + quad_h][z + k]) {
							can_expand = false;
							break;
						}

						const voxel::BlockType other =
								voxel::type(center->get_block(x, y + quad_h, z + k));

						if (other != type) {
							can_expand = false;
							break;
						}
					}

					if (can_expand)
						quad_h++;
				}

				// mark visited
				for (int dy = 0; dy < quad_h; dy++)
					for (int dz                 = 0; dz < quad_w; dz++)
						visited[y + dy][z + dz] = true;

				// quad geometry
				Vector3 v0(x + 1, y, z);
				Vector3 v1(x + 1, y + quad_h, z);
				Vector3 v2(x + 1, y + quad_h, z + quad_w);
				Vector3 v3(x + 1, y, z + quad_w);

				int tex_layer = _get_tex_layer(CubeFace::R, type);

				_get_mesher(block).add_quad(
						v0, v1, v2, v3,
						Vector3(1, 0, 0),
						tex_layer,
						Vector2(quad_h, quad_w),
						true,
						voxel::is_collidable(block)
						);
			}
		}
	}
}

void ChunkMeshBuilder::_add_up_faces(const ChunkNeighbors &neighbors) {
	const Chunk *center = neighbors.center.get();

	const int SX = Chunk::SIZE_X;
	const int SY = Chunk::SIZE_Y;
	const int SZ = Chunk::SIZE_Z;

	bool mask[SX][SZ];
	bool visited[SX][SZ];

	for (int y = 0; y < SY; y++) {
		// reset
		for (int x = 0; x < SX; x++) {
			for (int z = 0; z < SZ; z++) {
				mask[x][z]    = false;
				visited[x][z] = false;
			}
		}

		// build mask
		for (int x = 0; x < SX; x++) {
			for (int z = 0; z < SZ; z++) {
				const voxel::Block block = center->get_block(x, y, z);
				if (voxel::is_air(block) || _is_crossed_plant(block))
					continue;

				if (_is_face_visible(neighbors, x, y + 1, z, block)) {
					mask[x][z] = true;
				}
			}
		}

		// greedy meshing
		for (int x = 0; x < SX; x++) {
			for (int z = 0; z < SZ; z++) {
				if (!mask[x][z] || visited[x][z])
					continue;

				const voxel::Block block    = center->get_block(x, y, z);
				const voxel::BlockType type = voxel::type(block);

				int quad_w = 1; // Z
				int quad_h = 1; // X

				// expand Z
				while (z + quad_w < SZ) {
					if (!mask[x][z + quad_w] || visited[x][z + quad_w])
						break;

					const voxel::BlockType other =
							voxel::type(center->get_block(x, y, z + quad_w));

					if (other != type)
						break;

					quad_w++;
				}

				// expand X
				bool can_expand = true;
				while (x + quad_h < SX && can_expand) {
					for (int k = 0; k < quad_w; k++) {
						if (!mask[x + quad_h][z + k] ||
							visited[x + quad_h][z + k]) {
							can_expand = false;
							break;
						}

						const voxel::BlockType other =
								voxel::type(center->get_block(x + quad_h, y, z + k));

						if (other != type) {
							can_expand = false;
							break;
						}
					}

					if (can_expand)
						quad_h++;
				}

				// mark visited
				for (int dx = 0; dx < quad_h; dx++) {
					for (int dz = 0; dz < quad_w; dz++) {
						visited[x + dx][z + dz] = true;
					}
				}

				// geometry
				Vector3 v0(x, y + 1, z + quad_w);
				Vector3 v1(x + quad_h, y + 1, z + quad_w);
				Vector3 v2(x + quad_h, y + 1, z);
				Vector3 v3(x, y + 1, z);

				int tex_layer = _get_tex_layer(CubeFace::U, type);

				_get_mesher(block).add_quad(
						v0, v1, v2, v3,
						Vector3(0, 1, 0),
						tex_layer,
						Vector2((float)quad_h, (float)quad_w),
						false,
						voxel::is_collidable(block)
						);
			}
		}
	}
}

void ChunkMeshBuilder::_add_left_faces(const ChunkNeighbors &neighbors) {
	const Chunk *center = neighbors.center.get();

	const int SX = Chunk::SIZE_X;
	const int SY = Chunk::SIZE_Y;
	const int SZ = Chunk::SIZE_Z;

	bool mask[SY][SZ];
	bool visited[SY][SZ];

	for (int x = 0; x < SX; x++) {
		// reset
		for (int y = 0; y < SY; y++) {
			for (int z = 0; z < SZ; z++) {
				mask[y][z]    = false;
				visited[y][z] = false;
			}
		}

		// build mask
		for (int y = 0; y < SY; y++) {
			for (int z = 0; z < SZ; z++) {
				const voxel::Block block = center->get_block(x, y, z);
				if (voxel::is_air(block) || _is_crossed_plant(block))
					continue;

				if (_is_face_visible(neighbors, x - 1, y, z, block)) {
					mask[y][z] = true;
				}
			}
		}

		// greedy
		for (int y = 0; y < SY; y++) {
			for (int z = 0; z < SZ; z++) {
				if (!mask[y][z] || visited[y][z])
					continue;

				const voxel::Block block    = center->get_block(x, y, z);
				const voxel::BlockType type = voxel::type(block);

				int quad_w = 1; // Z
				int quad_h = 1; // Y

				// expand Z
				while (z + quad_w < SZ) {
					if (!mask[y][z + quad_w] || visited[y][z + quad_w])
						break;

					const voxel::BlockType other =
							voxel::type(center->get_block(x, y, z + quad_w));

					if (other != type)
						break;

					quad_w++;
				}

				// expand Y
				bool can_expand = true;
				while (y + quad_h < SY && can_expand) {
					for (int k = 0; k < quad_w; k++) {
						if (!mask[y + quad_h][z + k] ||
							visited[y + quad_h][z + k]) {
							can_expand = false;
							break;
						}

						const voxel::BlockType other =
								voxel::type(center->get_block(x, y + quad_h, z + k));

						if (other != type) {
							can_expand = false;
							break;
						}
					}

					if (can_expand)
						quad_h++;
				}

				// mark visited
				for (int dy = 0; dy < quad_h; dy++) {
					for (int dz = 0; dz < quad_w; dz++) {
						visited[y + dy][z + dz] = true;
					}
				}

				// quad geometry (LEFT face)
				Vector3 v0(x, y, z);
				Vector3 v1(x, y, z + quad_w);
				Vector3 v2(x, y + quad_h, z + quad_w);
				Vector3 v3(x, y + quad_h, z);

				int tex_layer = _get_tex_layer(CubeFace::L, type);

				_get_mesher(block).add_quad(
						v0, v1, v2, v3,
						Vector3(-1, 0, 0),
						tex_layer,
						Vector2((float)quad_w, (float)quad_h),
						false,
						voxel::is_collidable(block)
						);
			}
		}
	}
}

void ChunkMeshBuilder::_add_down_faces(const ChunkNeighbors &neighbors) {
	const Chunk *center = neighbors.center.get();

	const int SX = Chunk::SIZE_X;
	const int SY = Chunk::SIZE_Y;
	const int SZ = Chunk::SIZE_Z;

	bool mask[SX][SZ];
	bool visited[SX][SZ];

	for (int y = 0; y < SY; y++) {
		// reset
		for (int x = 0; x < SX; x++) {
			for (int z = 0; z < SZ; z++) {
				mask[x][z]    = false;
				visited[x][z] = false;
			}
		}

		// build mask
		for (int x = 0; x < SX; x++) {
			for (int z = 0; z < SZ; z++) {
				const voxel::Block block = center->get_block(x, y, z);
				if (voxel::is_air(block) || _is_crossed_plant(block))
					continue;

				if (_is_face_visible(neighbors, x, y - 1, z, block)) {
					mask[x][z] = true;
				}
			}
		}

		// greedy
		for (int x = 0; x < SX; x++) {
			for (int z = 0; z < SZ; z++) {
				if (!mask[x][z] || visited[x][z])
					continue;

				const voxel::Block block    = center->get_block(x, y, z);
				const voxel::BlockType type = voxel::type(block);

				int quad_w = 1; // Z
				int quad_h = 1; // X

				// expand Z
				while (z + quad_w < SZ) {
					if (!mask[x][z + quad_w] || visited[x][z + quad_w])
						break;

					const voxel::BlockType other =
							voxel::type(center->get_block(x, y, z + quad_w));

					if (other != type)
						break;

					quad_w++;
				}

				// expand X
				bool can_expand = true;
				while (x + quad_h < SX && can_expand) {
					for (int k = 0; k < quad_w; k++) {
						if (!mask[x + quad_h][z + k] ||
							visited[x + quad_h][z + k]) {
							can_expand = false;
							break;
						}

						const voxel::BlockType other =
								voxel::type(center->get_block(x + quad_h, y, z + k));

						if (other != type) {
							can_expand = false;
							break;
						}
					}

					if (can_expand)
						quad_h++;
				}

				// mark visited
				for (int dx = 0; dx < quad_h; dx++) {
					for (int dz = 0; dz < quad_w; dz++) {
						visited[x + dx][z + dz] = true;
					}
				}

				// geometry (DOWN face)
				Vector3 v0(x, y, z);
				Vector3 v1(x + quad_h, y, z);
				Vector3 v2(x + quad_h, y, z + quad_w);
				Vector3 v3(x, y, z + quad_w);

				int tex_layer = _get_tex_layer(CubeFace::D, type);

				_get_mesher(block).add_quad(
						v0, v1, v2, v3,
						Vector3(0, -1, 0),
						tex_layer,
						Vector2((float)quad_h, (float)quad_w),
						true,
						voxel::is_collidable(block)
						);
			}
		}
	}
}

void ChunkMeshBuilder::_add_front_faces(const ChunkNeighbors &neighbors) {
	const Chunk *center = neighbors.center.get();

	const int SX = Chunk::SIZE_X;
	const int SY = Chunk::SIZE_Y;
	const int SZ = Chunk::SIZE_Z;

	bool mask[SX][SY];
	bool visited[SX][SY];

	for (int z = 0; z < SZ; z++) {
		// reset
		for (int x = 0; x < SX; x++) {
			for (int y = 0; y < SY; y++) {
				mask[x][y]    = false;
				visited[x][y] = false;
			}
		}

		// build mask
		for (int x = 0; x < SX; x++) {
			for (int y = 0; y < SY; y++) {
				const voxel::Block block = center->get_block(x, y, z);
				if (voxel::is_air(block) || _is_crossed_plant(block))
					continue;

				if (_is_face_visible(neighbors, x, y, z + 1, block)) {
					mask[x][y] = true;
				}
			}
		}

		// greedy
		for (int x = 0; x < SX; x++) {
			for (int y = 0; y < SY; y++) {
				if (!mask[x][y] || visited[x][y])
					continue;

				const voxel::Block block    = center->get_block(x, y, z);
				const voxel::BlockType type = voxel::type(block);

				int quad_w = 1; // X
				int quad_h = 1; // Y

				// expand X
				while (x + quad_w < SX) {
					if (!mask[x + quad_w][y] || visited[x + quad_w][y])
						break;

					const voxel::BlockType other =
							voxel::type(center->get_block(x + quad_w, y, z));

					if (other != type)
						break;

					quad_w++;
				}

				// expand Y
				bool can_expand = true;
				while (y + quad_h < SY && can_expand) {
					for (int k = 0; k < quad_w; k++) {
						if (!mask[x + k][y + quad_h] ||
							visited[x + k][y + quad_h]) {
							can_expand = false;
							break;
						}

						const voxel::BlockType other =
								voxel::type(center->get_block(x + k, y + quad_h, z));

						if (other != type) {
							can_expand = false;
							break;
						}
					}

					if (can_expand)
						quad_h++;
				}

				// mark visited
				for (int dx = 0; dx < quad_w; dx++) {
					for (int dy = 0; dy < quad_h; dy++) {
						visited[x + dx][y + dy] = true;
					}
				}

				// geometry (FRONT face)
				Vector3 v0(x, y, z + 1);
				Vector3 v1(x + quad_w, y, z + 1);
				Vector3 v2(x + quad_w, y + quad_h, z + 1);
				Vector3 v3(x, y + quad_h, z + 1);

				int tex_layer = _get_tex_layer(CubeFace::F, type);

				_get_mesher(block).add_quad(
						v0, v1, v2, v3,
						Vector3(0, 0, 1),
						tex_layer,
						Vector2((float)quad_w, (float)quad_h),
						false,
						voxel::is_collidable(block)
						);
			}
		}
	}
}

void ChunkMeshBuilder::_add_back_faces(const ChunkNeighbors &neighbors) {
	const Chunk *center = neighbors.center.get();

	const int SX = Chunk::SIZE_X;
	const int SY = Chunk::SIZE_Y;
	const int SZ = Chunk::SIZE_Z;

	bool mask[SX][SY];
	bool visited[SX][SY];

	for (int z = 0; z < SZ; z++) {
		// reset
		for (int x = 0; x < SX; x++) {
			for (int y = 0; y < SY; y++) {
				mask[x][y]    = false;
				visited[x][y] = false;
			}
		}

		// build mask
		for (int x = 0; x < SX; x++) {
			for (int y = 0; y < SY; y++) {
				const voxel::Block block = center->get_block(x, y, z);
				if (voxel::is_air(block) || _is_crossed_plant(block))
					continue;

				if (_is_face_visible(neighbors, x, y, z - 1, block)) {
					mask[x][y] = true;
				}
			}
		}

		// greedy
		for (int x = 0; x < SX; x++) {
			for (int y = 0; y < SY; y++) {
				if (!mask[x][y] || visited[x][y])
					continue;

				const voxel::Block block    = center->get_block(x, y, z);
				const voxel::BlockType type = voxel::type(block);

				int quad_w = 1; // X
				int quad_h = 1; // Y

				// expand X
				while (x + quad_w < SX) {
					if (!mask[x + quad_w][y] || visited[x + quad_w][y])
						break;

					const voxel::BlockType other =
							voxel::type(center->get_block(x + quad_w, y, z));

					if (other != type)
						break;

					quad_w++;
				}

				// expand Y
				bool can_expand = true;
				while (y + quad_h < SY && can_expand) {
					for (int k = 0; k < quad_w; k++) {
						if (!mask[x + k][y + quad_h] ||
							visited[x + k][y + quad_h]) {
							can_expand = false;
							break;
						}

						const voxel::BlockType other =
								voxel::type(center->get_block(x + k, y + quad_h, z));

						if (other != type) {
							can_expand = false;
							break;
						}
					}

					if (can_expand)
						quad_h++;
				}

				// mark visited
				for (int dx = 0; dx < quad_w; dx++) {
					for (int dy = 0; dy < quad_h; dy++) {
						visited[x + dx][y + dy] = true;
					}
				}

				// geometry (BACK face)
				Vector3 v0(x + quad_w, y, z);
				Vector3 v1(x, y, z);
				Vector3 v2(x, y + quad_h, z);
				Vector3 v3(x + quad_w, y + quad_h, z);

				int tex_layer = _get_tex_layer(CubeFace::B, type);

				_get_mesher(block).add_quad(
						v0, v1, v2, v3,
						Vector3(0, 0, -1),
						tex_layer,
						Vector2((float)quad_w, (float)quad_h),
						false,
						voxel::is_collidable(block)
						);
			}
		}
	}
}

int ChunkMeshBuilder::_get_tex_layer(const CubeFace &face, const voxel::BlockType &type) {
	if (type == voxel::BlockType::WATER) {
		return voxel::WATER_TEXTURE_LAYER;
	}

	if (TextureKey key = { type, face }; texture_map.has(key)) {
		return texture_map[key];
	}
	// Blocks without a dedicated texture (for example ores and deepslate)
	// inherit stone, never atlas layer zero, which may belong to another block.
	if (TextureKey stone_key = { voxel::BlockType::STONE, face }; texture_map.has(stone_key)) {
		return texture_map[stone_key];
	}

	return texture_map.has({ voxel::BlockType::STONE, CubeFace::F })
			? texture_map[{ voxel::BlockType::STONE, CubeFace::F }]
			: 0;
}

void ChunkMeshBuilder::_load_textures() {
	block_texture_array = ResourceLoader::get_singleton()->load("res://textures/block_array.tres");
}

void ChunkMeshBuilder::_initialize_texture_map() {
	Ref<FileAccess> file = FileAccess::open("res://textures/block_mapping.json", FileAccess::READ);
	if (file.is_null()) {
		ERR_PRINT("Não foi possível carregar o mapeamento de texturas!");
		return;
	}

	String json_text = file->get_as_text();
	Variant data     = JSON::parse_string(json_text);

	if (data.get_type() != Variant::DICTIONARY) {
		ERR_PRINT("Formato de JSON inválido!");
		return;
	}

	Dictionary dict = data;
	Array keys      = dict.keys();

	for (int i = 0; i < keys.size(); i++) {
		String file_name = keys[i];
		int layer_index  = (int)dict[file_name];

		String base_name      = file_name.get_slice("_", 0);
		voxel::BlockType type = map_string_to_type(base_name);

		if (file_name.ends_with("_top")) {
			texture_map[{ type, CubeFace::U }] = layer_index;
		} else if (file_name.ends_with("_bottom")) {
			texture_map[{ type, CubeFace::D }] = layer_index;
		} else if (file_name.ends_with("_side")) {
			texture_map[{ type, CubeFace::F }] = layer_index;
			texture_map[{ type, CubeFace::B }] = layer_index;
			texture_map[{ type, CubeFace::L }] = layer_index;
			texture_map[{ type, CubeFace::R }] = layer_index;
			if (!texture_map.has({ type, CubeFace::U })) {
				texture_map[{ type, CubeFace::U }] = layer_index;
			}
			if (!texture_map.has({ type, CubeFace::D })) {
				texture_map[{ type, CubeFace::D }] = layer_index;
			}
		}
	}
}

voxel::BlockType godot::ChunkMeshBuilder::map_string_to_type(const godot::String &name) {
	if (name.begins_with("dirt")) {
		return voxel::BlockType::DIRT;
	}
	if (name.begins_with("grass")) {
		return voxel::BlockType::GRASS;
	}
	if (name.begins_with("leaves")) {
		return voxel::BlockType::LEAVES;
	}
	if (name.begins_with("stone")) {
		return voxel::BlockType::STONE;
	}
	if (name.begins_with("wood")) {
		return voxel::BlockType::WOOD;
	}
	if (name.begins_with("log")) {
		return voxel::BlockType::LOG;
	}
	if (name.begins_with("sandstone")) {
		return voxel::BlockType::SANDSTONE;
	}
	if (name.begins_with("sand")) {
		return voxel::BlockType::SAND;
	}
	if (name.begins_with("cactus")) {
		return voxel::BlockType::CACTUS;
	}
	if (name.begins_with("flower")) {
		return voxel::BlockType::FLOWER;
	}
	if (name.begins_with("tallgrass")) {
		return voxel::BlockType::TALL_GRASS;
	}

	// Fallback
	return voxel::BlockType::STONE;
}

void ChunkMeshBuilder::_add_crossed_plant_faces(const ChunkNeighbors &neighbors) {
	const Chunk *center = neighbors.center.get();
	for (int x = 0; x < Chunk::SIZE_X; ++x) {
		for (int y = 0; y < Chunk::SIZE_Y; ++y) {
			for (int z = 0; z < Chunk::SIZE_Z; ++z) {
				const voxel::Block block = center->get_block(x, y, z);
				if (!_is_crossed_plant(block)) continue;
				const int layer = _get_tex_layer(CubeFace::F, voxel::type(block));
				const Vector3 a(x + 0.12f, y, z + 0.12f);
				const Vector3 b(x + 0.88f, y, z + 0.88f);
				const Vector3 c(x + 0.88f, y + 1.0f, z + 0.88f);
				const Vector3 d(x + 0.12f, y + 1.0f, z + 0.12f);
				const Vector3 e(x + 0.12f, y, z + 0.88f);
				const Vector3 f(x + 0.88f, y, z + 0.12f);
				const Vector3 g(x + 0.88f, y + 1.0f, z + 0.12f);
				const Vector3 h(x + 0.12f, y + 1.0f, z + 0.88f);
				const Vector2 uv_scale(1.0f, 1.0f);
				const Vector3 normal_a(0.707f, 0.0f, -0.707f);
				const Vector3 normal_b(-0.707f, 0.0f, -0.707f);
				opaque_mesher.add_quad(a, b, c, d, normal_a, layer, uv_scale, false, false);
				opaque_mesher.add_quad(a, d, c, b, -normal_a, layer, uv_scale, true, false);
				opaque_mesher.add_quad(e, f, g, h, normal_b, layer, uv_scale, false, false);
				opaque_mesher.add_quad(e, h, g, f, -normal_b, layer, uv_scale, true, false);
			}
		}
	}
}

ChunkMeshBuilder::ChunkMeshBuilder() {
	_load_textures();
	_initialize_texture_map();
}

Ref<ArrayMesh> ChunkMeshBuilder::build(const ChunkNeighbors &neighbors) {
	opaque_mesher.clear();
	transparent_mesher.clear();

	_add_up_faces(neighbors);
	_add_left_faces(neighbors);
	_add_right_faces(neighbors);
	_add_down_faces(neighbors);
	_add_front_faces(neighbors);
	_add_back_faces(neighbors);
	_add_crossed_plant_faces(neighbors);

	Array opaque_arrays = opaque_mesher.build_arrays();
	Array transparent_arrays = transparent_mesher.build_arrays();
	const bool has_opaque = !PackedVector3Array(opaque_arrays[Mesh::ARRAY_VERTEX]).is_empty();
	const bool has_transparent = !PackedVector3Array(transparent_arrays[Mesh::ARRAY_VERTEX]).is_empty();
	if (!has_opaque && !has_transparent) {
		return Ref<ArrayMesh>();
	}

	Ref<ArrayMesh> mesh;
	mesh.instantiate();

	int64_t format = Mesh::ARRAY_FORMAT_VERTEX |
			Mesh::ARRAY_FORMAT_NORMAL |
			Mesh::ARRAY_FORMAT_TEX_UV |
			Mesh::ARRAY_FORMAT_CUSTOM0 |
			Mesh::ARRAY_FORMAT_INDEX |
			(Mesh::ARRAY_CUSTOM_R_FLOAT << Mesh::ARRAY_FORMAT_CUSTOM0_SHIFT);

	if (has_opaque) {
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, opaque_arrays, Array(), Dictionary(), format);
	}
	if (has_transparent) {
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, transparent_arrays, Array(), Dictionary(), format);
	}

	return mesh;
}
} // namespace godot

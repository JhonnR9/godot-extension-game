#include "chunk_mesh_builder.h"
#include "voxel_mesher.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

namespace godot {
bool ChunkMeshBuilder::_is_air(const ChunkNeighbors &n, int x, int y, int z) {
	if (x >= ChunkModel::MIN_X && x <= ChunkModel::MAX_X &&
		y >= ChunkModel::MIN_Y && y <= ChunkModel::MAX_Y &&
		z >= ChunkModel::MIN_Z && z <= ChunkModel::MAX_Z) {
		return voxel::is_air(n.center->get_block(x, y, z));
	}

	if (x < ChunkModel::MIN_X) {
		return n.left ? voxel::is_air(n.left->get_block(ChunkModel::MAX_X, y, z)) : true;
	}
	if (x > ChunkModel::MAX_X) {
		return n.right ? voxel::is_air(n.right->get_block(ChunkModel::MIN_X, y, z)) : true;
	}

	if (y < ChunkModel::MIN_Y) {
		return n.bottom ? voxel::is_air(n.bottom->get_block(x, ChunkModel::MAX_Y, z)) : true;
	}
	if (y > ChunkModel::MAX_Y) {
		return n.top ? voxel::is_air(n.top->get_block(x, ChunkModel::MIN_Y, z)) : true;
	}

	if (z < ChunkModel::MIN_Z) {
		return n.back ? voxel::is_air(n.back->get_block(x, y, ChunkModel::MAX_Z)) : true;
	}

	return n.front ? voxel::is_air(n.front->get_block(x, y, ChunkModel::MIN_Z)) : true;
}

void ChunkMeshBuilder::_add_right_faces(const ChunkNeighbors &neighbors) {
	const ChunkModel *center = neighbors.center.get();

	const int SX = ChunkModel::SIZE_X;
	const int SY = ChunkModel::SIZE_Y;
	const int SZ = ChunkModel::SIZE_Z;

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
				if (voxel::is_air(block))
					continue;

				if (_is_air(neighbors, x + 1, y, z)) {
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

				mesher.add_quad(
						v0, v1, v2, v3,
						Vector3(1, 0, 0),
						tex_layer,
						Vector2(quad_h, quad_w),
						true
						);
			}
		}
	}
}

void ChunkMeshBuilder::_add_up_faces(const ChunkNeighbors &neighbors) {
	const ChunkModel *center = neighbors.center.get();

	const int SX = ChunkModel::SIZE_X;
	const int SY = ChunkModel::SIZE_Y;
	const int SZ = ChunkModel::SIZE_Z;

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
				if (voxel::is_air(block))
					continue;

				if (_is_air(neighbors, x, y + 1, z)) {
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

				mesher.add_quad(
						v0, v1, v2, v3,
						Vector3(0, 1, 0),
						tex_layer,
						Vector2((float)quad_h, (float)quad_w)
						);
			}
		}
	}
}

void ChunkMeshBuilder::_add_left_faces(const ChunkNeighbors &neighbors) {
	const ChunkModel *center = neighbors.center.get();

	const int SX = ChunkModel::SIZE_X;
	const int SY = ChunkModel::SIZE_Y;
	const int SZ = ChunkModel::SIZE_Z;

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
				if (voxel::is_air(block))
					continue;

				if (_is_air(neighbors, x - 1, y, z)) {
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

				mesher.add_quad(
						v0, v1, v2, v3,
						Vector3(-1, 0, 0),
						tex_layer,
						Vector2((float)quad_w, (float)quad_h)
						);
			}
		}
	}
}

void ChunkMeshBuilder::_add_down_faces(const ChunkNeighbors &neighbors) {
	const ChunkModel *center = neighbors.center.get();

	const int SX = ChunkModel::SIZE_X;
	const int SY = ChunkModel::SIZE_Y;
	const int SZ = ChunkModel::SIZE_Z;

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
				if (voxel::is_air(block))
					continue;

				if (_is_air(neighbors, x, y - 1, z)) {
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

				mesher.add_quad(
						v0, v1, v2, v3,
						Vector3(0, -1, 0),
						tex_layer,
						Vector2((float)quad_h, (float)quad_w),
						true
						);
			}
		}
	}
}

void ChunkMeshBuilder::_add_front_faces(const ChunkNeighbors &neighbors) {
	const ChunkModel *center = neighbors.center.get();

	const int SX = ChunkModel::SIZE_X;
	const int SY = ChunkModel::SIZE_Y;
	const int SZ = ChunkModel::SIZE_Z;

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
				if (voxel::is_air(block))
					continue;

				if (_is_air(neighbors, x, y, z + 1)) {
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

				mesher.add_quad(
						v0, v1, v2, v3,
						Vector3(0, 0, 1),
						tex_layer,
						Vector2((float)quad_w, (float)quad_h)
						);
			}
		}
	}
}

void ChunkMeshBuilder::_add_back_faces(const ChunkNeighbors &neighbors) {
	const ChunkModel *center = neighbors.center.get();

	const int SX = ChunkModel::SIZE_X;
	const int SY = ChunkModel::SIZE_Y;
	const int SZ = ChunkModel::SIZE_Z;

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
				if (voxel::is_air(block))
					continue;

				if (_is_air(neighbors, x, y, z - 1)) {
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

				mesher.add_quad(
						v0, v1, v2, v3,
						Vector3(0, 0, -1),
						tex_layer,
						Vector2((float)quad_w, (float)quad_h)
						);
			}
		}
	}
}

int ChunkMeshBuilder::_get_tex_layer(const CubeFace &face, const voxel::BlockType &type) {
	if (TextureKey key = { type, face }; texture_map.has(key)) {
		return texture_map[key];
	}

	return 0;
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
		}
	}
}

voxel::BlockType godot::ChunkMeshBuilder::map_string_to_type(const godot::String &name) {
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

	// Fallback
	return voxel::BlockType::STONE;
}

ChunkMeshBuilder::ChunkMeshBuilder() {
	_load_textures();
	_initialize_texture_map();
}

Ref<ArrayMesh> ChunkMeshBuilder::build(const ChunkNeighbors &neighbors) {
	mesher.clear();

	_add_up_faces(neighbors);
	_add_left_faces(neighbors);
	_add_right_faces(neighbors);
	_add_down_faces(neighbors);
	_add_front_faces(neighbors);
	_add_back_faces(neighbors);

	Array arrays = mesher.build_arrays();

	if (arrays.is_empty()) {
		return Ref<ArrayMesh>();
	}

	PackedVector3Array verts = arrays[Mesh::ARRAY_VERTEX];

	if (verts.is_empty()) {
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

	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays, Array(), Dictionary(), format);

	return mesh;
}
} // namespace godot
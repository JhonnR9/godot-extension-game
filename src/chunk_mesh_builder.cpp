#include "chunk_mesh_builder.h"
#include "voxel_mesher.h"

namespace godot {
bool ChunkMeshBuilder::_is_air(const ChunkNeighbors &n, int x, int y, int z) {
	if (x >= 0 && x < ChunkModel::SIZE_X && y >= 0 && y < ChunkModel::SIZE_Y && z >= 0 && z < ChunkModel::SIZE_Z) {
		return block::is_air(n.center->get_block(x, y, z));
	}

	if (x < 0) {
		return n.left ? block::is_air(n.left->get_block(ChunkModel::SIZE_X - 1, y, z)) : true;
	}

	if (x >= ChunkModel::SIZE_X) {
		return n.right ? block::is_air(n.right->get_block(0, y, z)) : true;
	}

	if (y < 0) {
		return n.bottom ? block::is_air(n.bottom->get_block(x, ChunkModel::SIZE_Y - 1, z)) : true;
	}

	if (y >= ChunkModel::SIZE_Y) {
		return n.top ? block::is_air(n.top->get_block(x, 0, z)) : true;
	}

	if (z < 0) {
		return n.back ? block::is_air(n.back->get_block(x, y, ChunkModel::SIZE_Z - 1)) : true;
	}

	return n.front ? block::is_air(n.front->get_block(x, y, 0)) : true;
}

static AtlasUV get_uv(BlockType type, CubeFace face) {
	switch (type) {
		case BlockType::GRASS: {
			if (face == CubeFace::U)
				return atlas["grass_top"];
			if (face == CubeFace::D)
				return atlas["grass_bottom"];

			return atlas["grass_side"];
		}

		case BlockType::STONE: {
			if (face == CubeFace::U)
				return atlas["stone_top"];
			if (face == CubeFace::D)
				return atlas["stone_bottom"];

			return atlas["stone_side"];
		}

		case BlockType::LEAVES: {
			if (face == CubeFace::U)
				return atlas["leaves_top"];
			if (face == CubeFace::D)
				return atlas["leaves_bottom"];
			return atlas["leaves_side"];
		}

		case BlockType::DIRT: {
			return atlas["grass_bottom"];
		}

		case BlockType::LOG: {
			if (face == CubeFace::U || face == CubeFace::D)
				return atlas["stone_top"];

			return atlas["stone_side"];
		}

		case BlockType::DEEPSLATE: {
			return atlas["stone_bottom"];
		}

		default:
			return atlas["grass_side"];
	}
}


void ChunkMeshBuilder::_add_right_faces(const ChunkNeighbors &neighbors) {
	const ChunkModel *center = neighbors.center.get();

	const int SX = ChunkModel::SIZE_X;
	const int SY = ChunkModel::SIZE_Y;
	const int SZ = ChunkModel::SIZE_Z;

	bool mask[SY][SZ];
	bool visited[SY][SZ];

	for (int x = 0; x < SX; x++) {
		for (int y = 0; y < SY; y++) {
			for (int z = 0; z < SZ; z++) {
				mask[y][z]    = false;
				visited[y][z] = false;
			}
		}

		for (int y = 0; y < SY; y++) {
			for (int z = 0; z < SZ; z++) {
				const block::Block block = center->get_block(x, y, z);

				if (block::is_air(block)) {
					continue;
				}

				if (_is_air(neighbors, x + 1, y, z)) {
					mask[y][z] = true;
				}
			}
		}

		for (int y = 0; y < SY; y++) {
			for (int z = 0; z < SZ; z++) {
				if (!mask[y][z] || visited[y][z]) {
					continue;
				}

				const block::Block block = center->get_block(x, y, z);
				const BlockType type     = block::type(block);

				int quad_w = 1;
				int quad_h = 1;

				while (z + quad_w < SZ) {
					if (!mask[y][z + quad_w]) {
						break;
					}

					if (visited[y][z + quad_w]) {
						break;
					}

					const block::Block other =
							center->get_block(x, y, z + quad_w);

					if (block::type(other) != type) {
						break;
					}

					quad_w++;
				}

				bool can_expand = true;

				while (y + quad_h < SY && can_expand) {
					for (int k = 0; k < quad_w; k++) {
						if (!mask[y + quad_h][z + k]) {
							can_expand = false;
							break;
						}

						if (visited[y + quad_h][z + k]) {
							can_expand = false;
							break;
						}

						const block::Block other =
								center->get_block(x, y + quad_h, z + k);

						if (block::type(other) != type) {
							can_expand = false;
							break;
						}
					}

					if (can_expand) {
						quad_h++;
					}
				}

				for (int dy = 0; dy < quad_h; dy++) {
					for (int dz = 0; dz < quad_w; dz++) {
						visited[y + dy][z + dz] = true;
					}
				}

				Vector3 v0(x + 1, y, z + quad_w);
				Vector3 v1(x + 1, y, z);
				Vector3 v2(x + 1, y + quad_h, z);
				Vector3 v3(x + 1, y + quad_h, z + quad_w);

				mesher.add_quad(
						v0,
						v1,
						v2,
						v3,
						Vector3(1, 0, 0),
						get_uv(type, CubeFace::R),
						Color(1.0, 1.0, 1.0)
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
		for (int x = 0; x < SX; x++) {
			for (int z = 0; z < SZ; z++) {
				mask[x][z]    = false;
				visited[x][z] = false;
			}
		}

		for (int x = 0; x < SX; x++) {
			for (int z = 0; z < SZ; z++) {
				const block::Block block = center->get_block(x, y, z);

				if (block::is_air(block)) {
					continue;
				}

				if (_is_air(neighbors, x, y + 1, z)) {
					mask[x][z] = true;
				}
			}
		}

		for (int x = 0; x < SX; x++) {
			for (int z = 0; z < SZ; z++) {
				if (!mask[x][z] || visited[x][z]) {
					continue;
				}

				const block::Block block = center->get_block(x, y, z);
				const BlockType type     = block::type(block);

				int quad_w = 1;
				int quad_h = 1;

				while (z + quad_w < SZ) {
					if (!mask[x][z + quad_w] || visited[x][z + quad_w]) {
						break;
					}

					const block::Block other = center->get_block(x, y, z + quad_w);
					if (block::type(other) != type) {
						break;
					}

					quad_w++;
				}

				bool can_expand = true;
				while (x + quad_h < SX && can_expand) {
					for (int k = 0; k < quad_w; k++) {
						if (!mask[x + quad_h][z + k] || visited[x + quad_h][z + k]) {
							can_expand = false;
							break;
						}

						const block::Block other = center->get_block(x + quad_h, y, z + k);
						if (block::type(other) != type) {
							can_expand = false;
							break;
						}
					}

					if (can_expand) {
						quad_h++;
					}
				}

				for (int dx = 0; dx < quad_h; dx++) {
					for (int dz = 0; dz < quad_w; dz++) {
						visited[x + dx][z + dz] = true;
					}
				}

				Vector3 v0(x, y + 1, z + quad_w);
				Vector3 v1(x + quad_h, y + 1, z + quad_w);
				Vector3 v2(x + quad_h, y + 1, z);
				Vector3 v3(x, y + 1, z);

				mesher.add_quad(
						v0,
						v1,
						v2,
						v3,
						Vector3(0, 1, 0),
						get_uv(type, CubeFace::U),
						Color(1.0, 1.0, 1.0)
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
		for (int y = 0; y < SY; y++) {
			for (int z = 0; z < SZ; z++) {
				mask[y][z]    = false;
				visited[y][z] = false;
			}
		}

		for (int y = 0; y < SY; y++) {
			for (int z = 0; z < SZ; z++) {
				const block::Block block = center->get_block(x, y, z);

				if (block::is_air(block)) {
					continue;
				}

				if (_is_air(neighbors, x - 1, y, z)) {
					mask[y][z] = true;
				}
			}
		}

		for (int y = 0; y < SY; y++) {
			for (int z = 0; z < SZ; z++) {
				if (!mask[y][z] || visited[y][z]) {
					continue;
				}

				const block::Block block = center->get_block(x, y, z);
				const BlockType type     = block::type(block);

				int quad_w = 1;
				int quad_h = 1;

				while (z + quad_w < SZ) {
					if (!mask[y][z + quad_w]) {
						break;
					}

					if (visited[y][z + quad_w]) {
						break;
					}

					const block::Block other =
							center->get_block(x, y, z + quad_w);

					if (block::type(other) != type) {
						break;
					}

					quad_w++;
				}

				bool can_expand = true;

				while (y + quad_h < SY && can_expand) {
					for (int k = 0; k < quad_w; k++) {
						if (!mask[y + quad_h][z + k]) {
							can_expand = false;
							break;
						}

						if (visited[y + quad_h][z + k]) {
							can_expand = false;
							break;
						}

						const block::Block other =
								center->get_block(x, y + quad_h, z + k);

						if (block::type(other) != type) {
							can_expand = false;
							break;
						}
					}

					if (can_expand) {
						quad_h++;
					}
				}

				for (int dy = 0; dy < quad_h; dy++) {
					for (int dz = 0; dz < quad_w; dz++) {
						visited[y + dy][z + dz] = true;
					}
				}

				Vector3 v0(x, y, z);
				Vector3 v1(x, y, z + quad_w);
				Vector3 v2(x, y + quad_h, z + quad_w);
				Vector3 v3(x, y + quad_h, z);

				mesher.add_quad(
						v0,
						v1,
						v2,
						v3,
						Vector3(-1, 0, 0),
						get_uv(type, CubeFace::L),
						Color(1.0, 1.0, 1.0)
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
		for (int x = 0; x < SX; x++) {
			for (int z = 0; z < SZ; z++) {
				mask[x][z]    = false;
				visited[x][z] = false;
			}
		}

		for (int x = 0; x < SX; x++) {
			for (int z = 0; z < SZ; z++) {
				const block::Block block = center->get_block(x, y, z);

				if (block::is_air(block)) {
					continue;
				}

				if (_is_air(neighbors, x, y - 1, z)) {
					mask[x][z] = true;
				}
			}
		}

		for (int x = 0; x < SX; x++) {
			for (int z = 0; z < SZ; z++) {
				if (!mask[x][z] || visited[x][z]) {
					continue;
				}

				const block::Block block = center->get_block(x, y, z);
				const BlockType type     = block::type(block);

				int quad_w = 1;
				int quad_h = 1;

				while (z + quad_w < SZ) {
					if (!mask[x][z + quad_w] || visited[x][z + quad_w]) {
						break;
					}

					const block::Block other =
							center->get_block(x, y, z + quad_w);

					if (block::type(other) != type) {
						break;
					}

					quad_w++;
				}

				bool can_expand = true;

				while (x + quad_h < SX && can_expand) {
					for (int k = 0; k < quad_w; k++) {
						if (!mask[x + quad_h][z + k] ||
							visited[x + quad_h][z + k]) {
							can_expand = false;
							break;
						}

						const block::Block other =
								center->get_block(x + quad_h, y, z + k);

						if (block::type(other) != type) {
							can_expand = false;
							break;
						}
					}

					if (can_expand) {
						quad_h++;
					}
				}

				for (int dx = 0; dx < quad_h; dx++) {
					for (int dz = 0; dz < quad_w; dz++) {
						visited[x + dx][z + dz] = true;
					}
				}

				Vector3 v0(x, y, z);
				Vector3 v1(x + quad_h, y, z);
				Vector3 v2(x + quad_h, y, z + quad_w);
				Vector3 v3(x, y, z + quad_w);

				mesher.add_quad(
						v0,
						v1,
						v2,
						v3,
						Vector3(0, -1, 0),
						get_uv(type, CubeFace::D),
						Color(1.0, 1.0, 1.0)
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
		for (int x = 0; x < SX; x++) {
			for (int y = 0; y < SY; y++) {
				mask[x][y]    = false;
				visited[x][y] = false;
			}
		}

		for (int x = 0; x < SX; x++) {
			for (int y = 0; y < SY; y++) {
				const block::Block block = center->get_block(x, y, z);

				if (block::is_air(block)) {
					continue;
				}

				if (_is_air(neighbors, x, y, z + 1)) {
					mask[x][y] = true;
				}
			}
		}

		for (int x = 0; x < SX; x++) {
			for (int y = 0; y < SY; y++) {
				if (!mask[x][y] || visited[x][y]) {
					continue;
				}

				const block::Block block = center->get_block(x, y, z);
				const BlockType type     = block::type(block);

				int quad_w = 1;
				int quad_h = 1;

				while (x + quad_w < SX) {
					if (!mask[x + quad_w][y] ||
						visited[x + quad_w][y]) {
						break;
					}

					const block::Block other =
							center->get_block(x + quad_w, y, z);

					if (block::type(other) != type) {
						break;
					}

					quad_w++;
				}

				bool can_expand = true;

				while (y + quad_h < SY && can_expand) {
					for (int k = 0; k < quad_w; k++) {
						if (!mask[x + k][y + quad_h] ||
							visited[x + k][y + quad_h]) {
							can_expand = false;
							break;
						}

						const block::Block other =
								center->get_block(x + k, y + quad_h, z);

						if (block::type(other) != type) {
							can_expand = false;
							break;
						}
					}

					if (can_expand) {
						quad_h++;
					}
				}

				for (int dx = 0; dx < quad_w; dx++) {
					for (int dy = 0; dy < quad_h; dy++) {
						visited[x + dx][y + dy] = true;
					}
				}

				Vector3 v0(x, y, z + 1);
				Vector3 v1(x + quad_w, y, z + 1);
				Vector3 v2(x + quad_w, y + quad_h, z + 1);
				Vector3 v3(x, y + quad_h, z + 1);

				mesher.add_quad(
						v0,
						v1,
						v2,
						v3,
						Vector3(0, 0, 1),
						get_uv(type, CubeFace::F),
						Color(1.0, 1.0, 1.0)
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
		for (int x = 0; x < SX; x++) {
			for (int y = 0; y < SY; y++) {
				mask[x][y]    = false;
				visited[x][y] = false;
			}
		}

		for (int x = 0; x < SX; x++) {
			for (int y = 0; y < SY; y++) {
				const block::Block block = center->get_block(x, y, z);

				if (block::is_air(block)) {
					continue;
				}

				if (_is_air(neighbors, x, y, z - 1)) {
					mask[x][y] = true;
				}
			}
		}

		for (int x = 0; x < SX; x++) {
			for (int y = 0; y < SY; y++) {
				if (!mask[x][y] || visited[x][y]) {
					continue;
				}

				const block::Block block = center->get_block(x, y, z);
				const BlockType type     = block::type(block);

				int quad_w = 1;
				int quad_h = 1;

				while (x + quad_w < SX) {
					if (!mask[x + quad_w][y] ||
						visited[x + quad_w][y]) {
						break;
					}

					const block::Block other =
							center->get_block(x + quad_w, y, z);

					if (block::type(other) != type) {
						break;
					}

					quad_w++;
				}

				bool can_expand = true;

				while (y + quad_h < SY && can_expand) {
					for (int k = 0; k < quad_w; k++) {
						if (!mask[x + k][y + quad_h] ||
							visited[x + k][y + quad_h]) {
							can_expand = false;
							break;
						}

						const block::Block other =
								center->get_block(x + k, y + quad_h, z);

						if (block::type(other) != type) {
							can_expand = false;
							break;
						}
					}

					if (can_expand) {
						quad_h++;
					}
				}

				for (int dx = 0; dx < quad_w; dx++) {
					for (int dy = 0; dy < quad_h; dy++) {
						visited[x + dx][y + dy] = true;
					}
				}

				Vector3 v0(x + quad_w, y, z);
				Vector3 v1(x, y, z);
				Vector3 v2(x, y + quad_h, z);
				Vector3 v3(x + quad_w, y + quad_h, z);

				mesher.add_quad(
						v0,
						v1,
						v2,
						v3,
						Vector3(0, 0, -1),
						get_uv(type, CubeFace::B),
						Color(1.0, 1.0, 1.0)
						);
			}
		}
	}
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

	mesh->add_surface_from_arrays(
			Mesh::PRIMITIVE_TRIANGLES,
			arrays
			);

	return mesh;
}
} // namespace godot
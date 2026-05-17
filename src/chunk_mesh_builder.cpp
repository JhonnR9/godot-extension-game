#include "chunk_mesh_builder.h"
#include "voxel_mesher.h"

namespace godot {

bool ChunkMeshBuilder::_is_air(const ChunkNeighbors& n, int x, int y, int z) {
	if (x >= 0 && x < ChunkModel::SIZE &&
		y >= 0 && y < ChunkModel::SIZE &&
		z >= 0 && z < ChunkModel::SIZE) {
		return n.center->_blocks[x][y][z].is_air();
	}

	if (x < 0)
		return n.left ? n.left->_blocks[ChunkModel::SIZE - 1][y][z].is_air() : true;
	if (x >= ChunkModel::SIZE)
		return n.right ? n.right->_blocks[0][y][z].is_air() : true;

	if (y < 0)
		return n.bottom ? n.bottom->_blocks[x][ChunkModel::SIZE - 1][z].is_air() : true;
	if (y >= ChunkModel::SIZE)
		return n.top ? n.top->_blocks[x][0][z].is_air() : true;

	if (z < 0)
		return n.back ? n.back->_blocks[x][y][ChunkModel::SIZE - 1].is_air() : true;
	if (z >= ChunkModel::SIZE)
		return n.front ? n.front->_blocks[x][y][0].is_air() : true;

	return true;
}

static AtlasUV get_uv(BlockType type, CubeFace face) {
	switch (type) {
		case BlockType::GRASS:
			if (face == CubeFace::U)
				return atlas["grass_top"];
			else if (face == CubeFace::D)
				return atlas["grass_bottom"];
			else
				return atlas["grass_side"];

		case BlockType::STONE:
			return atlas["stone_side"];

		default:
			return atlas["stone_side"];
	}
}

Ref<ArrayMesh> ChunkMeshBuilder::build(const ChunkNeighbors& neighbors) {
	mesher.clear();

	const ChunkModel* center = neighbors.center.get();

	for (int x = 0; x < ChunkModel::SIZE; x++) {
	for (int y = 0; y < ChunkModel::SIZE; y++) {
	for (int z = 0; z < ChunkModel::SIZE; z++) {

		const Block& block = center->_blocks[x][y][z];

		if (block.is_air())
			continue;

		Vector3i pos(x, y, z);

		if (x < ChunkModel::SIZE - 1) {
			if (center->_blocks[x + 1][y][z].is_air())
				mesher.add_face(CubeFace::R, pos, get_uv(block.type, CubeFace::R));
		} else if (_is_air(neighbors, x + 1, y, z)) {
			mesher.add_face(CubeFace::R, pos, get_uv(block.type, CubeFace::R));
		}

		if (x > 0) {
			if (center->_blocks[x - 1][y][z].is_air())
				mesher.add_face(CubeFace::L, pos, get_uv(block.type, CubeFace::L));
		} else if (_is_air(neighbors, x - 1, y, z)) {
			mesher.add_face(CubeFace::L, pos, get_uv(block.type, CubeFace::L));
		}

		if (y < ChunkModel::SIZE - 1) {
			if (center->_blocks[x][y + 1][z].is_air())
				mesher.add_face(CubeFace::U, pos, get_uv(block.type, CubeFace::U));
		} else if (_is_air(neighbors, x, y + 1, z)) {
			mesher.add_face(CubeFace::U, pos, get_uv(block.type, CubeFace::U));
		}

		if (y > 0) {
			if (center->_blocks[x][y - 1][z].is_air())
				mesher.add_face(CubeFace::D, pos, get_uv(block.type, CubeFace::D));
		} else if (_is_air(neighbors, x, y - 1, z)) {
			mesher.add_face(CubeFace::D, pos, get_uv(block.type, CubeFace::D));
		}

		if (z < ChunkModel::SIZE - 1) {
			if (center->_blocks[x][y][z + 1].is_air())
				mesher.add_face(CubeFace::F, pos, get_uv(block.type, CubeFace::F));
		} else if (_is_air(neighbors, x, y, z + 1)) {
			mesher.add_face(CubeFace::F, pos, get_uv(block.type, CubeFace::F));
		}

		if (z > 0) {
			if (center->_blocks[x][y][z - 1].is_air())
				mesher.add_face(CubeFace::B, pos, get_uv(block.type, CubeFace::B));
		} else if (_is_air(neighbors, x, y, z - 1)) {
			mesher.add_face(CubeFace::B, pos, get_uv(block.type, CubeFace::B));
		}
	}
	}
	}

	Array arrays = mesher.build_arrays();

	if (arrays.is_empty())
		return Ref<ArrayMesh>();

	PackedVector3Array verts = arrays[Mesh::ARRAY_VERTEX];
	if (verts.size() == 0)
		return Ref<ArrayMesh>();

	Ref<ArrayMesh> mesh;
	mesh.instantiate();
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

	return mesh;
}
} // namespace godot
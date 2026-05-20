#include "chunk_mesh_builder.h"
#include "voxel_mesher.h"

namespace godot {

bool ChunkMeshBuilder::_is_air(const ChunkNeighbors &n, int x, int y, int z) {
	if (x >= 0 && x < ChunkModel::SIZE && y >= 0 && y < ChunkModel::SIZE && z >= 0 && z < ChunkModel::SIZE) {
		return block::is_air(n.center->get_block(x, y, z));
	}

	if (x < 0) {
		return n.left ? block::is_air(n.left->get_block(ChunkModel::SIZE - 1, y, z)) : true;
	}

	if (x >= ChunkModel::SIZE) {
		return n.right ? block::is_air(n.right->get_block(0, y, z)) : true;
	}

	if (y < 0) {
		return n.bottom ? block::is_air(n.bottom->get_block(x, ChunkModel::SIZE - 1, z)) : true;
	}

	if (y >= ChunkModel::SIZE) {
		return n.top? block::is_air(n.top->get_block(x, 0, z)): true;
	}

	if (z < 0) {
		return n.back ? block::is_air(n.back->get_block(x, y, ChunkModel::SIZE - 1)) : true;
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

Ref<ArrayMesh> ChunkMeshBuilder::build(const ChunkNeighbors &neighbors) {
	mesher.clear();

	const ChunkModel *center = neighbors.center.get();

	for (int z = 0; z < ChunkModel::SIZE; z++) {
		for (int y = 0; y < ChunkModel::SIZE; y++) {
			for (int x = 0; x < ChunkModel::SIZE; x++) {

				const block::Block block =center->get_block(x, y, z);

				if (block::is_air(block))
					continue;

				const BlockType type =block::type(block);

				Vector3i pos(x, y, z);

				// RIGHT
				if (x < ChunkModel::SIZE - 1) {
					if (block::is_air(center->get_block(x + 1, y, z))) {
						mesher.add_face(CubeFace::R,pos,get_uv(type, CubeFace::R));
					}

				} else if (_is_air(neighbors, x + 1, y, z)) {
					mesher.add_face(CubeFace::R,pos,get_uv(type, CubeFace::R));
				}

				// LEFT
				if (x > 0) {
					if (block::is_air(center->get_block(x - 1, y, z))) {
						mesher.add_face(CubeFace::L,pos,get_uv(type, CubeFace::L));
					}

				} else if (_is_air(neighbors, x - 1, y, z)) {

					mesher.add_face(CubeFace::L,pos,get_uv(type, CubeFace::L));
				}

				// UP
				if (y < ChunkModel::SIZE - 1) {

					if (block::is_air(center->get_block(x, y + 1, z))) {
						mesher.add_face(CubeFace::U,pos,get_uv(type, CubeFace::U));
					}

				} else if (_is_air(neighbors, x, y + 1, z)) {
					mesher.add_face(CubeFace::U,pos,get_uv(type, CubeFace::U));
				}

				// DOWN
				if (y > 0) {
					if (block::is_air(center->get_block(x, y - 1, z))) {
						mesher.add_face(CubeFace::D,pos,get_uv(type, CubeFace::D));
					}

				} else if (_is_air(neighbors, x, y - 1, z)) {
					mesher.add_face(CubeFace::D,pos,get_uv(type, CubeFace::D));
				}

				// FRONT
				if (z < ChunkModel::SIZE - 1) {
					if (block::is_air(center->get_block(x, y, z + 1))) {
						mesher.add_face(CubeFace::F,pos,get_uv(type, CubeFace::F));
					}

				} else if (_is_air(neighbors, x, y, z + 1)) {

					mesher.add_face(CubeFace::F,pos,get_uv(type, CubeFace::F));
				}

				// BACK
				if (z > 0) {
					if (block::is_air(center->get_block(x, y, z - 1))) {
						mesher.add_face(CubeFace::B,pos,get_uv(type, CubeFace::B));
					}

				} else if (_is_air(neighbors, x, y, z - 1)) {
					mesher.add_face(CubeFace::B,pos,get_uv(type, CubeFace::B));
				}
			}
		}
	}

	Array arrays = mesher.build_arrays();

	if (arrays.is_empty())
		return Ref<ArrayMesh>();

	PackedVector3Array verts = arrays[Mesh::ARRAY_VERTEX];

	if (verts.is_empty())
		return Ref<ArrayMesh>();

	Ref<ArrayMesh> mesh;
	mesh.instantiate();

	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES,arrays);

	return mesh;
}
} // namespace godot
//
// Created by jhone on 13/05/2026.
//

#include "chunk_mesh_builder.h"

#include "voxel_mesher.h"

namespace godot {
bool ChunkMeshBuilder::_is_air_local(const ChunkModel &model, int x, int y, int z) {
	if (x >= 0 && x < model.SIZE && y >= 0 && y < model.SIZE && z >= 0 && z < model.SIZE) {
		return model._blocks[x][y][z].is_air();
	}

	return true;
}

Ref<ArrayMesh> ChunkMeshBuilder::build(const ChunkModel &model) {
	for (int x = 0; x < ChunkModel::SIZE; x++) {
		for (int y = 0; y < ChunkModel::SIZE; y++) {
			for (int z = 0; z < ChunkModel::SIZE; z++) {

				const Block& block =
					model.get_block(x, y, z);

				if (block.type == BlockType::AIR)
					continue;

				Vector3 pos(x, y, z);

				if (_is_air_local(model, x + 1, y, z))
					mesher.add_face(CubeFace::R, pos);

				if (_is_air_local(model, x - 1, y, z))
					mesher.add_face(CubeFace::L, pos);

				if (_is_air_local(model, x, y + 1, z))
					mesher.add_face(CubeFace::U, pos);

				if (_is_air_local(model, x, y - 1, z))
					mesher.add_face(CubeFace::D, pos);

				if (_is_air_local(model, x, y, z + 1))
					mesher.add_face(CubeFace::F, pos);

				if (_is_air_local(model, x, y, z - 1))
					mesher.add_face(CubeFace::B, pos);
			}
		}
	}

	Ref<ArrayMesh> mesh;
	mesh.instantiate();

	mesh->add_surface_from_arrays(
		Mesh::PRIMITIVE_TRIANGLES,
		mesher.build_arrays()
	);

	return mesh;
}
} // godot
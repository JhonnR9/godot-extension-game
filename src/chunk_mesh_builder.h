//
// Created by jhone on 13/05/2026.
//

#ifndef CHUNK_MESH_BUILDER_H
#define CHUNK_MESH_BUILDER_H

#include "chunk_model.h"
#include "voxel_mesher.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace godot {
inline std::unordered_map<std::string, AtlasUV> atlas;

struct ChunkNeighbors {
	std::shared_ptr<ChunkModel> center;

	std::shared_ptr<ChunkModel> right;
	std::shared_ptr<ChunkModel> left;

	std::shared_ptr<ChunkModel> top;
	std::shared_ptr<ChunkModel> bottom;

	std::shared_ptr<ChunkModel> front;
	std::shared_ptr<ChunkModel> back;
};

class ChunkMeshBuilder {
	VoxelMesher mesher;

public:
	Ref<ArrayMesh> build(const ChunkNeighbors& neighbors);

	static bool _is_air(const ChunkNeighbors& n, int x, int y, int z);

	PackedVector3Array get_last_collision_faces() const {
		return mesher.get_collision_faces();
	}
};

} // namespace godot

#endif
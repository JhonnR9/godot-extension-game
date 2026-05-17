//
// Created by jhone on 13/05/2026.
//

#ifndef CHUNK_MESH_BUILDER_H
#define CHUNK_MESH_BUILDER_H
#include "chunk_model.h"
#include "voxel_mesher.h"

#include <godot_cpp/classes/array_mesh.hpp>

namespace godot {
struct ChunkNeighbors {
	const ChunkModel* center;
	const ChunkModel* left;   // x - 1
	const ChunkModel* right;  // x + 1
	const ChunkModel* top;    // y + 1
	const ChunkModel* bottom; // y - 1
	const ChunkModel* front;  // z + 1
	const ChunkModel* back;   // z - 1
};

class ChunkMeshBuilder {
	VoxelMesher mesher;
	ChunkNeighbors neighbors = {};

public:
	Ref<ArrayMesh> build(const ChunkNeighbors& neighbors );
	static bool _is_air(const ChunkNeighbors& n, int x, int y, int z) ;
	PackedVector3Array get_last_collision_faces() const {
		return mesher.get_collision_faces();
	}
};

} // godot

#endif //CHUNK_MESH_BUILDER_H

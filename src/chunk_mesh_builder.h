//
// Created by jhone on 13/05/2026.
//

#ifndef CHUNK_MESH_BUILDER_H
#define CHUNK_MESH_BUILDER_H
#include "chunk_model.h"
#include "voxel_mesher.h"

#include <godot_cpp/classes/array_mesh.hpp>

namespace godot {

class ChunkMeshBuilder {
	VoxelMesher mesher;
	static bool _is_air_local(const ChunkModel &model, int x, int y, int z);
public:
	Ref<ArrayMesh> build( const ChunkModel& model);
};

} // godot

#endif //CHUNK_MESH_BUILDER_H

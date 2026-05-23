#ifndef CHUNK_MESH_ASYNC_GENERATOR_H
#define CHUNK_MESH_ASYNC_GENERATOR_H

#include "chunk_mesh_builder.h"
#include "chunk_model.h"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/hash_set.hpp"
#include "godot_cpp/templates/hashfuncs.hpp"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <memory>
#include <mutex>

namespace godot {
struct MeshResult {
	Ref<ArrayMesh> mesh;
	PackedVector3Array collision_faces;
	Vector3i pos;
	uint64_t version{ 0 };

	bool operator==(const MeshResult &p_b) const {
		return pos == p_b.pos;
	}
};


struct MeshResultHasher {
	static _FORCE_INLINE_ uint32_t hash(const MeshResult &p_value) {
		uint32_t h = hash_murmur3_one_32(p_value.pos.x);
		h          = hash_murmur3_one_32(p_value.pos.y, h);
		return hash_murmur3_one_32(p_value.pos.z, h);
	}
};


using MeshResultHashSet = HashSet<MeshResult, MeshResultHasher>;

class ChunkMeshAsyncGenerator;

struct ChunkMeshJob {
	Vector3i pos;
	ChunkNeighbors neighbors;
	ChunkMeshAsyncGenerator *generator;
	uint64_t version;
};

class ChunkMeshAsyncGenerator : public RefCounted {
	GDCLASS(ChunkMeshAsyncGenerator, RefCounted)

protected:
	static void _bind_methods();

private:
	std::mutex _generated_meshes_mutex;
	MeshResultHashSet _generated_meshes;

	std::mutex _generating_meshes_mutex;
	HashSet<Vector3i> _generating_meshes;

public:
	void queue_async_generate_mesh(Vector3i p_pos, ChunkNeighbors p_neighbors, uint64_t p_version, bool p_priority = false);
	bool is_queued_mesh(Vector3i p_pos);

	MeshResultHashSet consume_generated_meshes(int amount = -1);
};
} //namespace godot

#endif //CHUNK_MESH_ASYNC_GENERATOR_H
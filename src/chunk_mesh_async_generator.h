
#ifndef CHUNK_MESH_ASYNC_GENERATOR_H
#define CHUNK_MESH_ASYNC_GENERATOR_H
#include "chunk_mesh_builder.h"
#include "chunk_model.h"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/hash_set.hpp"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <memory>

namespace godot {

struct MeshResult {
	Ref<ArrayMesh> mesh;
	PackedVector3Array collision_faces;
	Vector3i pos;
};

inline bool operator==(const MeshResult &a, const MeshResult &b) {
	return a.pos == b.pos;
}

inline uint32_t hash(const MeshResult &p_value) {
	return hash_murmur3_one_32(
		p_value.pos.x ^
		(p_value.pos.y << 10) ^
		(p_value.pos.z << 20)
	);
}

} //namespace godot

namespace godot {
class ChunkMeshAsyncGenerator;

struct ChunkMeshJob {
	Vector3i pos;
	ChunkNeighbors &neighbors;
	ChunkMeshAsyncGenerator *generator;
};

class ChunkMeshAsyncGenerator : public RefCounted {
	GDCLASS(ChunkMeshAsyncGenerator, RefCounted)

protected:
	static void _bind_methods();

private:
	std::mutex _generated_meshes_mutex;
	HashSet<MeshResult> _generated_meshes;

	std::mutex _generating_meshes_mutex;
	HashSet<Vector3i> _generating_meshes;

public:
	// TODO change this ChunkNeighbors& for share_ptr<ChunkNeighbors>
	void queue_async_generate_mesh(Vector3i p_pos, ChunkNeighbors &p_neighbors);
	bool is_queued_mesh(Vector3i p_pos);

	HashSet<MeshResult> consume_generated_meshes(int amount = -1);
};

} //namespace godot

#endif //CHUNK_MESH_ASYNC_GENERATOR_H

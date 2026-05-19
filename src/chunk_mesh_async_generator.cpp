

#include "chunk_mesh_async_generator.h"

#include "chunk_mesh_builder.h"

#include <godot_cpp/classes/worker_thread_pool.hpp>

namespace godot {
void ChunkMeshAsyncGenerator::_bind_methods() {

}

void ChunkMeshAsyncGenerator::queue_async_generate_mesh(Vector3i p_pos, ChunkNeighbors &p_neighbors) {
	{
		std::lock_guard lock(_generating_meshes_mutex);
		_generating_meshes.insert(p_pos);
	}

	auto *chunk_mesh_job = new ChunkMeshJob{
		p_pos,
		p_neighbors,
		this
	};

	WorkerThreadPool::get_singleton()->add_native_task(
			[](void *data) {
				const auto *chunk_job = static_cast<ChunkMeshJob *>(data);
				{
					ChunkMeshBuilder builder;
					Ref<ArrayMesh> mesh = builder.build(chunk_job->neighbors);

					MeshResult mesh_result{
						mesh,
						builder.get_last_collision_faces(),
						chunk_job->pos
					};

					std::lock_guard lock(chunk_job->generator->_generated_meshes_mutex);
					chunk_job->generator->_generated_meshes.insert(mesh_result);

					{
						std::lock_guard lock2(chunk_job->generator->_generating_meshes_mutex);
						chunk_job->generator->_generating_meshes.erase(chunk_job->pos);
					}

				}

				delete chunk_job;
			},
			chunk_mesh_job,
			false,
			"chunk_mesh_task");
}
bool ChunkMeshAsyncGenerator::is_queued_mesh(const Vector3i p_pos) {
	std::lock_guard lock(_generating_meshes_mutex);
	return _generating_meshes.has(p_pos);
}

HashSet<MeshResult> ChunkMeshAsyncGenerator::consume_generated_meshes(int amount) {
	HashSet<MeshResult> consumed;
	HashSet<MeshResult> remaining;

	std::lock_guard lock(_generated_meshes_mutex);

	if (amount < 0) {
		amount = static_cast<int>(_generated_meshes.size());
	}

	const int consume_count = std::min(amount, static_cast<int>(_generated_meshes.size()));

	consumed.reserve(consume_count);
	remaining.reserve(_generated_meshes.size() - consume_count);

	int count = 0;

	for (auto it = _generated_meshes.begin(); it != _generated_meshes.end(); ++it) {
		if (count < consume_count) {
			consumed.insert(*it);
			count++;
		} else {
			remaining.insert(*it);
		}
	}

	_generated_meshes = std::move(remaining);

	return consumed;
}
} // godot
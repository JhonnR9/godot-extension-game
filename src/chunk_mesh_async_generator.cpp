#include "chunk_mesh_async_generator.h"
#include "chunk_mesh_builder.h"
#include <godot_cpp/classes/worker_thread_pool.hpp>
#include <utility>

namespace godot {

void ChunkMeshAsyncGenerator::_bind_methods() {
}

void ChunkMeshAsyncGenerator::queue_async_generate_mesh(Vector3i p_pos, ChunkNeighbors p_neighbors, uint64_t version, bool p_priority) {
	{
		std::lock_guard lock(_generating_meshes_mutex);
		_generating_meshes.insert(p_pos);
	}

	auto *chunk_mesh_job = new ChunkMeshJob{
		p_pos,
		std::move(p_neighbors),
		this,
		version
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
						chunk_job->pos,
						chunk_job->version
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
			p_priority,
			"chunk_mesh_task");
}

bool ChunkMeshAsyncGenerator::is_queued_mesh(const Vector3i p_pos) {
	std::lock_guard lock(_generating_meshes_mutex);
	return _generating_meshes.has(p_pos);
}

size_t ChunkMeshAsyncGenerator::get_queue_size() {
	std::lock_guard lock(_generated_meshes_mutex);
	return _generated_meshes.size();
}

MeshResultHashSet ChunkMeshAsyncGenerator::consume_generated_meshes(int amount) {
	MeshResultHashSet consumed;
	MeshResultHashSet remaining;

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

} // namespace godot
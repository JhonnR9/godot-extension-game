
#include "chunk_model_generator.h"

#include <godot_cpp/classes/worker_thread_pool.hpp>
#include <memory>

namespace godot {
void ChunkModelGenerator::_bind_methods() {
}

void ChunkModelGenerator::_queue_async_generate_chunk_model(Vector3i p_pos, const TerrainSettings &p_settings) {
	{
		std::lock_guard lock(_loading_chunks_mutex);
		_loading_chunks.insert(p_pos);
	}

	auto job = new ChunkJob{
		p_pos,
		p_settings,
		this
	};

	WorkerThreadPool::get_singleton()->add_native_task(
			[](void *data) {
				const auto *chunk_job = static_cast<ChunkJob *>(data);

				{
					const auto model = std::make_shared<ChunkModel>(ChunkGenerator::generate(chunk_job->pos, chunk_job->settings));
					std::lock_guard free_lock(chunk_job->generator->_generated_results_mutex);
					chunk_job->generator->_generated_results.insert(chunk_job->pos, model);
				}

				delete chunk_job;
			},
			job,
			false,
			"chunk_gen");
}
bool ChunkModelGenerator::is_loading_chunk(const Vector3i &p_pos) {
	std::lock_guard lock(_loading_chunks_mutex);
	return _loading_chunks.has(p_pos);
}
HashMap<Vector3i, std::shared_ptr<ChunkModel>> ChunkModelGenerator::consume_generated_results(int amount) {
	{
		HashMap<Vector3i, std::shared_ptr<ChunkModel>> consumed;
		HashSet<Vector3i> to_remove;
		std::lock_guard lock(_generated_results_mutex);

		if (amount < 0) {
			amount = _generated_results.size();
		} else {
			const int consume_count = std::min(amount, static_cast<int>(_generated_results.size()));
			consumed.reserve(consume_count);
			to_remove.reserve(consume_count);
		}

		int count = 0;
		for (auto it = _generated_results.begin(); it != _generated_results.end(); ++it) {
			if (count >= amount) break;
			consumed.insert(it->key, it->value);
			to_remove.insert(it->key);
			count++;
		}

		if (!to_remove.is_empty()) {
			std::lock_guard loading_lock(_loading_chunks_mutex);
			for (const Vector3i &pos : to_remove) {
				_generated_results.erase(pos);
				_loading_chunks.erase(pos);
			}
		}

		return consumed;
	}
}
} //namespace godot
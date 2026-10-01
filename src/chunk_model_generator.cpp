#include "chunk_model_generator.h"
#include <godot_cpp/classes/worker_thread_pool.hpp>
#include <memory>

namespace godot {
void ChunkModelGenerator::_bind_methods() {
}

void ChunkModelGenerator::_queue_async_generate_chunk_model(Vector3i p_pos, const TerrainSettings &p_settings,
		std::shared_ptr<const ChunkGenerationPipeline> p_pipeline, bool p_priotity) {
	if (!p_pipeline) {
		ERR_PRINT("Cannot generate a chunk without a generation pipeline.");
		return;
	}

	{
		auto placeholder  = std::make_shared<Chunk>();
		placeholder->stage = ChunkStage::QUEUED_GENERATION;

		std::lock_guard lock(_loading_chunks_mutex);
		_loading_chunks.insert(p_pos, placeholder);
	}

	auto job = new ChunkJob{
		p_pos,
		p_settings,
		std::move(p_pipeline),
		this
	};

	WorkerThreadPool::get_singleton()->add_native_task(
			[](void *data) {
				const auto *chunk_job = static_cast<ChunkJob *>(data);

				{
					std::lock_guard loading_lock(chunk_job->generator->_loading_chunks_mutex);
					if (std::shared_ptr<Chunk> *placeholder = chunk_job->generator->_loading_chunks.getptr(chunk_job->pos)) {
						(*placeholder)->stage = ChunkStage::GENERATING;
					}
				}

				const auto model = std::make_shared<Chunk>(ChunkGenerator::generate(
						chunk_job->pos, chunk_job->settings, *chunk_job->pipeline));
				model->stage	 = ChunkStage::LOADED;

				{
					std::lock_guard free_lock(chunk_job->generator->_generated_results_mutex);
					chunk_job->generator->_generated_results.insert(chunk_job->pos, model);
				}

				delete chunk_job;
			},
			job,
			p_priotity,
			"chunk_gen");
}

bool ChunkModelGenerator::is_loading_chunk(const Vector3i &p_pos) {
	std::lock_guard lock(_loading_chunks_mutex);
	return _loading_chunks.has(p_pos);
}

std::shared_ptr<Chunk> ChunkModelGenerator::get_loading_chunk(const Vector3i &p_pos) {
	std::lock_guard lock(_loading_chunks_mutex);
	if (std::shared_ptr<Chunk> *found = _loading_chunks.getptr(p_pos)) {
		return *found;
	}
	return nullptr;
}

HashMap<Vector3i, std::shared_ptr<Chunk>> ChunkModelGenerator::consume_generated_results(int amount) {
	HashMap<Vector3i, std::shared_ptr<Chunk>> consumed;
	HashSet<Vector3i> to_remove;

	{
		std::lock_guard lock(_generated_results_mutex);

		if (amount < 0) {
			amount = _generated_results.size();
		}

		int count = 0;

		std::vector<Vector3i> keys_to_erase;


		for (const auto &E : _generated_results) {
			if (count >= amount) break;


			consumed.insert(E.key, E.value);

			to_remove.insert(E.key);
			keys_to_erase.push_back(E.key);

			count++;
		}

		for (const Vector3i &k : keys_to_erase) {
			_generated_results.erase(k);
		}
	}


	if (!to_remove.is_empty()) {
		std::lock_guard loading_lock(_loading_chunks_mutex);
		for (const Vector3i &pos : to_remove) {
			_loading_chunks.erase(pos);
		}
	}

	return consumed;
}
} //namespace godot

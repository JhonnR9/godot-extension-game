#ifndef CHUNK_MODEL_GENERATOR_H
#define CHUNK_MODEL_GENERATOR_H

#include "chunk_generator.h"
#include "chunk_generation_pipeline.h"
#include "chunk_model.h"
#include "godot_cpp/classes/wrapped.hpp"
#include "godot_cpp/templates/hash_set.hpp"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <memory>

namespace godot {
class ChunkModelGenerator;

struct ChunkJob {
	Vector3i pos;
	TerrainSettings settings;
	std::shared_ptr<const ChunkGenerationPipeline> pipeline;
	ChunkModelGenerator *generator;
};

class ChunkModelGenerator final : public RefCounted {
	GDCLASS(ChunkModelGenerator, RefCounted)

protected:
	static void _bind_methods();

private:
	std::mutex _generated_results_mutex;
	HashMap<Vector3i, std::shared_ptr<Chunk>> _generated_results;

	std::mutex _loading_chunks_mutex;
	HashMap<Vector3i, std::shared_ptr<Chunk>> _loading_chunks;


public:
	HashMap<Vector3i, std::shared_ptr<Chunk>> consume_generated_results(int amount = -1);
	void _queue_async_generate_chunk_model(Vector3i p_pos, const TerrainSettings &p_settings,
			std::shared_ptr<const ChunkGenerationPipeline> p_pipeline, bool p_priority = false);

	bool is_loading_chunk(const Vector3i &p_pos);

	std::shared_ptr<Chunk> get_loading_chunk(const Vector3i &p_pos);
};

} //namespace godot

#endif //CHUNK_MODEL_GENERATOR_H

#ifndef TREE_DECORATOR_H
#define TREE_DECORATOR_H

#include "chunk_generation_pipeline.h"
#include <cstdint>
#include <deque>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace godot {

// 100% deterministic and stateless generation:
// the trees for a chunk column (cx, cz) depend only on (seed, cx, cz) and the terrain height.
// When decorating chunk C, we calculate the trees for the 9 neighboring columns (3x3)
// and write only the blocks that fall within C. This ensures that trees on the boundary
// are complete and reappear identically if the chunk is unloaded and reloaded.
class TreeGenerationPass final : public ChunkGenerationPass {
public:
	explicit TreeGenerationPass(int64_t p_seed, int p_max_trees_per_chunk = 4,
			int p_min_trunk_height = 5, int p_max_trunk_height = 7) :
			_seed(p_seed), _max_trees_per_chunk(p_max_trees_per_chunk),
			_min_trunk_height(p_min_trunk_height), _max_trunk_height(p_max_trunk_height) {}

	void apply(ChunkGenerationContext &context) const override;

private:
	struct ChunkKey {
		int32_t x = 0;
		int32_t z = 0;
		bool operator==(const ChunkKey &other) const { return x == other.x && z == other.z; }
	};
	struct ChunkKeyHash {
		size_t operator()(const ChunkKey &key) const {
			uint64_t value = static_cast<uint32_t>(key.x);
			value = (value << 32) | static_cast<uint32_t>(key.z);
			value ^= value >> 30;
			value *= 0xBF58476D1CE4E5B9ULL;
			value ^= value >> 27;
			return static_cast<size_t>(value);
		}
	};
	struct TreeCandidate {
		int32_t x = 0;
		int32_t z = 0;
		int32_t base_y = 0;
		int trunk_height = 0;
		uint64_t shape_seed = 0;
	};
	using CandidateList = std::vector<TreeCandidate>;
	CandidateList _calculate_chunk_candidates(const ChunkGenerationContext &context, int32_t cx, int32_t cz) const;
	CandidateList _get_chunk_candidates(const ChunkGenerationContext &context, int32_t cx, int32_t cz) const;

	int64_t _seed = 0;
	int _max_trees_per_chunk = 4;
	int _min_trunk_height = 5;
	int _max_trunk_height = 7;
	mutable std::mutex _cache_mutex;
	mutable std::unordered_map<ChunkKey, CandidateList, ChunkKeyHash> _candidate_cache;
	mutable std::deque<ChunkKey> _cache_order;
	static constexpr size_t MAX_CACHED_CHUNKS = 512;
};

} // namespace godot

#endif // TREE_DECORATOR_H

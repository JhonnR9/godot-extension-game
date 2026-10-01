#ifndef TREE_DECORATOR_H
#define TREE_DECORATOR_H

#include "chunk_generation_pipeline.h"
#include <cstdint>

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
	int64_t _seed = 0;
	int _max_trees_per_chunk = 4;
	int _min_trunk_height = 5;
	int _max_trunk_height = 7;
};

} // namespace godot

#endif // TREE_DECORATOR_H

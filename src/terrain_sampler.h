#ifndef TERRAIN_SAMPLER_H
#define TERRAIN_SAMPLER_H
#include "chunk_generation_pipeline.h"
namespace godot {
class TerrainSampler {
public:
	static ColumnGenerationData sample(const TerrainSettings &settings, int32_t x, int32_t z);
	static voxel::Block block_at(const ColumnGenerationData &column, int32_t y);
};
} //namespace godot
#endif

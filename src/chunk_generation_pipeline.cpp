#include "chunk_generation_pipeline.h"
#include "terrain_sampler.h"

#include <godot_cpp/core/math.hpp>

namespace godot {

ChunkGenerationContext::ChunkGenerationContext(const Vector3i &p_chunk_position, Chunk &p_chunk, const TerrainSettings &p_settings) :
		chunk_position(p_chunk_position), chunk(p_chunk), settings(p_settings),
		columns(Chunk::SIZE_X * Chunk::SIZE_Z), block_write_layers(Chunk::VOLUME, 0) {
	for (ColumnGenerationData &data : columns) {
		data.water_level = settings.water_level;
		data.surface_block = voxel::make_block(voxel::block_ids::grass);
		data.subsurface_block = voxel::make_block(voxel::block_ids::dirt);
		data.stone_block = voxel::make_block(voxel::block_ids::stone);
		data.deep_stone_block = voxel::make_block(voxel::block_ids::deepslate);
	}
}

ColumnGenerationData &ChunkGenerationContext::column(const int x, const int z) {
	return columns[static_cast<size_t>(z * Chunk::SIZE_X + x)];
}

const ColumnGenerationData &ChunkGenerationContext::column(const int x, const int z) const {
	return columns[static_cast<size_t>(z * Chunk::SIZE_X + x)];
}

int32_t ChunkGenerationContext::world_x(const int local_x) const {
	return chunk_position.x * Chunk::SIZE_X + local_x;
}

int32_t ChunkGenerationContext::world_y(const int local_y) const {
	return chunk_position.y * Chunk::SIZE_Y + local_y;
}

int32_t ChunkGenerationContext::world_z(const int local_z) const {
	return chunk_position.z * Chunk::SIZE_Z + local_z;
}

bool ChunkGenerationContext::write_block(const int x, const int y, const int z, const voxel::Block block,
		const GenerationLayer layer) {
	if (x < 0 || x >= Chunk::SIZE_X || y < 0 || y >= Chunk::SIZE_Y || z < 0 || z >= Chunk::SIZE_Z) {
		return false;
	}
	const size_t index = static_cast<size_t>(x + y * Chunk::SIZE_X + z * Chunk::SIZE_X * Chunk::SIZE_Y);
	const uint8_t priority = static_cast<uint8_t>(layer);
	if (priority <= block_write_layers[index]) {
		return false;
	}
	chunk.set_block(x, y, z, block);
	block_write_layers[index] = priority;
	return true;
}

ColumnGenerationData ChunkGenerationContext::sample_column_at(int32_t wx, int32_t wz) const {
    const int x = wx - world_x(0), z = wz - world_z(0);
    if (columns_ready && x >= 0 && x < Chunk::SIZE_X && z >= 0 && z < Chunk::SIZE_Z) return column(x,z);
    return TerrainSampler::sample(settings, wx, wz);
}
int32_t ChunkGenerationContext::surface_height_at(int32_t x, int32_t z) const { return sample_column_at(x,z).surface_height; }
int32_t ChunkGenerationContext::water_level_at(int32_t x, int32_t z) const { return sample_column_at(x,z).water_level; }
float ChunkGenerationContext::climate_weight_at(int32_t x, int32_t z) const { return sample_column_at(x,z).climate_weight; }
bool ChunkGenerationContext::trees_allowed_at(int32_t x, int32_t z) const { return sample_column_at(x,z).trees_allowed; }

void ChunkGenerationPipeline::add_pass(std::shared_ptr<const ChunkGenerationPass> pass) {
	if (pass) {
		_passes.push_back(std::move(pass));
	}
}

Chunk ChunkGenerationPipeline::generate(const Vector3i &chunk_position, const TerrainSettings &settings) const {
	Chunk chunk{};
	ChunkGenerationContext context(chunk_position, chunk, settings);
	for (const std::shared_ptr<const ChunkGenerationPass> &pass : _passes) {
		pass->apply(context);
	}
	return chunk;
}

} // namespace godot

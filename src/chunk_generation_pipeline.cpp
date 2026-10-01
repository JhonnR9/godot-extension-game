#include "chunk_generation_pipeline.h"

#include <godot_cpp/core/math.hpp>

namespace godot {

ChunkGenerationContext::ChunkGenerationContext(const Vector3i &p_chunk_position, Chunk &p_chunk, const TerrainSettings &p_settings) :
		chunk_position(p_chunk_position), chunk(p_chunk), settings(p_settings), columns(Chunk::SIZE_X * Chunk::SIZE_Z) {
	for (ColumnGenerationData &data : columns) {
		data.water_level = settings.water_level;
		data.surface_block = voxel::make_block(voxel::BlockType::GRASS);
		data.subsurface_block = voxel::make_block(voxel::BlockType::DIRT);
		data.stone_block = voxel::make_block(voxel::BlockType::STONE);
		data.deep_stone_block = voxel::make_block(voxel::BlockType::DEEPSLATE);
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

int32_t ChunkGenerationContext::surface_height_at(const int32_t p_world_x, const int32_t p_world_z) const {
	const int local_x = p_world_x - chunk_position.x * Chunk::SIZE_X;
	const int local_z = p_world_z - chunk_position.z * Chunk::SIZE_Z;
	if (local_x >= 0 && local_x < Chunk::SIZE_X && local_z >= 0 && local_z < Chunk::SIZE_Z) {
		return column(local_x, local_z).surface_height;
	}
	float height_scale = 1.0f;
	int32_t height_offset = 0;
	if (settings.biome_noise.is_valid() && settings.biome_noise->get_noise_2d(p_world_x, p_world_z) >= 0.28f) {
		height_scale = 0.32f;
		if (settings.dune_noise.is_valid()) {
			const float ridge = 1.0f - Math::abs(settings.dune_noise->get_noise_2d(p_world_x, p_world_z));
			height_offset = static_cast<int32_t>(Math::round(ridge * 4.0f - 1.5f));
		}
	}
	const float noise = settings.terrain_noise.is_valid()
			? settings.terrain_noise->get_noise_2d(p_world_x, p_world_z)
			: 0.0f;
	return settings.terrain_base_height + height_offset +
			static_cast<int32_t>(Math::round(noise * settings.terrain_amplitude * height_scale));
}

int32_t ChunkGenerationContext::water_level_at(const int32_t p_world_x, const int32_t p_world_z) const {
	const int local_x = p_world_x - chunk_position.x * Chunk::SIZE_X;
	const int local_z = p_world_z - chunk_position.z * Chunk::SIZE_Z;
	if (local_x >= 0 && local_x < Chunk::SIZE_X && local_z >= 0 && local_z < Chunk::SIZE_Z) {
		return column(local_x, local_z).water_level;
	}
	if (settings.biome_noise.is_valid() && settings.biome_noise->get_noise_2d(p_world_x, p_world_z) >= 0.28f) {
		return settings.terrain_base_height - 8;
	}
	return settings.water_level;
}

bool ChunkGenerationContext::trees_allowed_at(const int32_t p_world_x, const int32_t p_world_z) const {
	const int local_x = p_world_x - chunk_position.x * Chunk::SIZE_X;
	const int local_z = p_world_z - chunk_position.z * Chunk::SIZE_Z;
	if (local_x >= 0 && local_x < Chunk::SIZE_X && local_z >= 0 && local_z < Chunk::SIZE_Z) {
		return column(local_x, local_z).trees_allowed;
	}
	return settings.biome_noise.is_null() || settings.biome_noise->get_noise_2d(p_world_x, p_world_z) < 0.28f;
}

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

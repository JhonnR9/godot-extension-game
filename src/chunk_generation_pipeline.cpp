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
	const float desert = desert_weight_at(p_world_x, p_world_z);
	const float terrain = settings.terrain_noise.is_valid() ? settings.terrain_noise->get_noise_2d(p_world_x, p_world_z) : 0.0f;
	const float mountain = settings.mountain_noise.is_valid() ? settings.mountain_noise->get_noise_2d(p_world_x, p_world_z) : terrain;
	const float ridge = 1.0f - Math::abs(mountain);
	const float mountain_offset = Math::pow(ridge, 2.0f) * 19.0f - 5.0f;
	const float dune = settings.dune_noise.is_valid() ? settings.dune_noise->get_noise_2d(p_world_x, p_world_z) : 0.0f;
	const float dune_height = (1.0f - Math::abs(dune)) * 4.0f - 1.5f;
	const float height_offset = Math::lerp(mountain_offset, dune_height, desert);
	const float height_scale = Math::lerp(2.4f, 0.32f, desert);
	const float height_delta = terrain * settings.terrain_amplitude * height_scale + height_offset;
	return settings.terrain_base_height + static_cast<int32_t>(Math::round(height_delta));
}

int32_t ChunkGenerationContext::water_level_at(const int32_t p_world_x, const int32_t p_world_z) const {
	const int local_x = p_world_x - chunk_position.x * Chunk::SIZE_X;
	const int local_z = p_world_z - chunk_position.z * Chunk::SIZE_Z;
	if (local_x >= 0 && local_x < Chunk::SIZE_X && local_z >= 0 && local_z < Chunk::SIZE_Z) {
		return column(local_x, local_z).water_level;
	}
	return static_cast<int32_t>(Math::round(Math::lerp(static_cast<float>(settings.water_level),
			static_cast<float>(settings.terrain_base_height - 8), desert_weight_at(p_world_x, p_world_z))));
}

float ChunkGenerationContext::desert_weight_at(const int32_t p_world_x, const int32_t p_world_z) const {
	if (settings.biome_noise.is_null()) {
		return 0.0f;
	}
	const float sample = settings.biome_noise->get_noise_2d(p_world_x, p_world_z);
	const float t = Math::clamp((sample - 0.05f) / 0.30f, 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

bool ChunkGenerationContext::trees_allowed_at(const int32_t p_world_x, const int32_t p_world_z) const {
	const int local_x = p_world_x - chunk_position.x * Chunk::SIZE_X;
	const int local_z = p_world_z - chunk_position.z * Chunk::SIZE_Z;
	if (local_x >= 0 && local_x < Chunk::SIZE_X && local_z >= 0 && local_z < Chunk::SIZE_Z) {
		return column(local_x, local_z).trees_allowed;
	}
	return desert_weight_at(p_world_x, p_world_z) < 0.45f;
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

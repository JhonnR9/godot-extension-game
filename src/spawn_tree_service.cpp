#include "spawn_tree_service.h"

namespace godot {

namespace {

inline uint64_t next_rand(uint64_t &state) {
	state += 0x9E3779B97F4A7C15ULL;
	uint64_t z = state;
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
	return z ^ (z >> 31);
}

inline uint64_t mix_bits(uint64_t value) {
	value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
	value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
	return value ^ (value >> 31);
}

inline uint64_t leaf_hash(uint64_t seed, int32_t x, int32_t y, int32_t z) {
	uint64_t value = seed;
	value ^= static_cast<uint64_t>(static_cast<int64_t>(x)) * 0x9E3779B97F4A7C15ULL;
	value ^= static_cast<uint64_t>(static_cast<int64_t>(y)) * 0xC2B2AE3D27D4EB4FULL;
	value ^= static_cast<uint64_t>(static_cast<int64_t>(z)) * 0x165667B19E3779F9ULL;
	return mix_bits(value);
}

} // namespace

void TreeGenerationPass::apply(ChunkGenerationContext &context) const {
	if (_max_trees_per_chunk <= 0) {
		return;
	}

	const int32_t origin_x = context.chunk_position.x * Chunk::SIZE_X;
	const int32_t origin_y = context.chunk_position.y * Chunk::SIZE_Y;
	const int32_t origin_z = context.chunk_position.z * Chunk::SIZE_Z;


	auto put = [&](int32_t wx, int32_t wy, int32_t wz, voxel::Block block, bool replace_leaves, bool replace_ground = false) {
		const int32_t lx = wx - origin_x;
		const int32_t ly = wy - origin_y;
		const int32_t lz = wz - origin_z;
		if (lx < 0 || lx >= Chunk::SIZE_X || ly < 0 || ly >= Chunk::SIZE_Y || lz < 0 || lz >= Chunk::SIZE_Z) {
			return;
		}
		const voxel::Block existing = context.chunk.get_block(lx, ly, lz);
		if (voxel::is_air(existing) ||
				(replace_leaves && voxel::type(existing) == voxel::BlockType::LEAVES) ||
				(replace_ground && voxel::is_collidable(existing))) {
			context.chunk.set_block(lx, ly, lz, block);
		}
	};

	const voxel::Block log_block = voxel::make_block(voxel::BlockType::LOG);
	const voxel::Block leaves_block = voxel::make_block(voxel::BlockType::LEAVES, voxel::BLOCK_FLAG_CUTOUT);

	for (int dcx = -1; dcx <= 1; ++dcx) {
		for (int dcz = -1; dcz <= 1; ++dcz) {
			const int32_t cx = context.chunk_position.x + dcx;
			const int32_t cz = context.chunk_position.z + dcz;

			uint64_t rng = static_cast<uint64_t>(_seed) ^
					(static_cast<uint64_t>(static_cast<int64_t>(cx)) * 0x9E3779B97F4A7C15ULL) ^
					(static_cast<uint64_t>(static_cast<int64_t>(cz)) * 0xC2B2AE3D27D4EB4FULL);
			next_rand(rng);

			const int tree_count = static_cast<int>(next_rand(rng) % (_max_trees_per_chunk + 1));

			for (int i = 0; i < tree_count; ++i) {
				const int lx = static_cast<int>(next_rand(rng) % Chunk::SIZE_X);
				const int lz = static_cast<int>(next_rand(rng) % Chunk::SIZE_Z);
				const int span = _max_trunk_height - _min_trunk_height + 1;
				const int trunk_h = _min_trunk_height + static_cast<int>(next_rand(rng) % span);

				const int32_t wx = cx * Chunk::SIZE_X + lx;
				const int32_t wz = cz * Chunk::SIZE_Z + lz;
				if (!context.trees_allowed_at(wx, wz)) {
					continue;
				}
			const int32_t surface_y = context.surface_height_at(wx, wz);
			const int32_t base_y = surface_y + 1;
			// The water fill pass runs before trees. Reject the whole feature when
			// its planting cell is at or below the local water surface.
			if (base_y <= context.water_level_at(wx, wz)) {
				continue;
			}
			const uint64_t shape_seed = next_rand(rng);

				if (base_y + trunk_h + 2 < origin_y || base_y - 1 > origin_y + Chunk::MAX_Y) {
					continue;
				}

				if (wx + 2 < origin_x || wx - 2 >= origin_x + Chunk::SIZE_X ||
						wz + 2 < origin_z || wz - 2 >= origin_z + Chunk::SIZE_Z) {
					continue;
				}

				// Five tapered layers make a rounded crown instead of a flat box.
				for (int layer = 0; layer < 5; ++layer) {
					const int dy = trunk_h - 2 + layer;
					const int radius = (layer == 1 || layer == 2) ? 2 : (layer == 4 ? 0 : 1);
					for (int dx = -radius; dx <= radius; ++dx) {
						for (int dz = -radius; dz <= radius; ++dz) {
							const int ax = ABS(dx);
							const int az = ABS(dz);
							if (radius == 2 && ax == 2 && az == 2) {
								continue;
							}
							// Break up the outer silhouette with sparse, repeatable gaps.
							const bool outer_edge = radius == 2 && (ax == 2 || az == 2);
							if (outer_edge && (leaf_hash(shape_seed, wx + dx, base_y + dy, wz + dz) % 7 == 0)) {
								continue;
							}
							put(wx + dx, base_y + dy, wz + dz, leaves_block, false);
						}
					}
					// Add a few leaf tufts around the upper shoulder of the crown.
					if (layer == 3) {
						put(wx - 2, base_y + dy, wz, leaves_block, false);
						put(wx + 2, base_y + dy, wz, leaves_block, false);
						put(wx, base_y + dy, wz - 2, leaves_block, false);
						put(wx, base_y + dy, wz + 2, leaves_block, false);
					}
				}


				// Carry the trunk up through the crown, leaving only the top leaf cap above it.
				for (int dy = -1; dy < trunk_h + 2; ++dy) {
					put(wx, base_y + dy, wz, log_block, true, dy == -1);
				}
			}
		}
	}
}

} // namespace godot

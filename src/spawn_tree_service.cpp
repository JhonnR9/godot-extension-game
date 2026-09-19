#include "spawn_tree_service.h"
#include "voxel_types.h"

namespace godot {

namespace {

inline int32_t floor_div(int32_t a, int32_t b) {
	int32_t q = a / b;
	if ((a % b != 0) && ((a < 0) != (b < 0))) {
		--q;
	}
	return q;
}

inline uint64_t next_rand(uint64_t &state) {
	state += 0x9E3779B97F4A7C15ULL;
	uint64_t z = state;
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
	return z ^ (z >> 31);
}

} // namespace

void TreeDecorator::decorate_chunk(const Vector3i &p_chunk_pos, const std::shared_ptr<Chunk> &p_chunk) const {
	if (!p_chunk || !_settings.surface_height || _settings.max_trees_per_chunk <= 0) {
		return;
	}

	const int32_t origin_x = p_chunk_pos.x * Chunk::SIZE_X;
	const int32_t origin_y = p_chunk_pos.y * Chunk::SIZE_Y;
	const int32_t origin_z = p_chunk_pos.z * Chunk::SIZE_Z;


	auto put = [&](int32_t wx, int32_t wy, int32_t wz, voxel::Block block, bool replace_leaves) {
		const int32_t lx = wx - origin_x;
		const int32_t ly = wy - origin_y;
		const int32_t lz = wz - origin_z;
		if (lx < 0 || lx >= Chunk::SIZE_X || ly < 0 || ly >= Chunk::SIZE_Y || lz < 0 || lz >= Chunk::SIZE_Z) {
			return;
		}
		const voxel::Block existing = p_chunk->get_block(lx, ly, lz);
		if (voxel::is_air(existing) || (replace_leaves && existing == voxel::make_block(voxel::BlockType::LEAVES))) {
			p_chunk->set_block(lx, ly, lz, block);
		}
	};

	const voxel::Block log_block = voxel::make_block(voxel::BlockType::LOG);
	const voxel::Block leaves_block = voxel::make_block(voxel::BlockType::LEAVES);

	for (int dcx = -1; dcx <= 1; ++dcx) {
		for (int dcz = -1; dcz <= 1; ++dcz) {
			const int32_t cx = p_chunk_pos.x + dcx;
			const int32_t cz = p_chunk_pos.z + dcz;

			uint64_t rng = static_cast<uint64_t>(_settings.seed) ^
					(static_cast<uint64_t>(static_cast<int64_t>(cx)) * 0x9E3779B97F4A7C15ULL) ^
					(static_cast<uint64_t>(static_cast<int64_t>(cz)) * 0xC2B2AE3D27D4EB4FULL);
			next_rand(rng);

			const int tree_count = static_cast<int>(next_rand(rng) % (_settings.max_trees_per_chunk + 1));

			for (int i = 0; i < tree_count; ++i) {
				const int lx = static_cast<int>(next_rand(rng) % Chunk::SIZE_X);
				const int lz = static_cast<int>(next_rand(rng) % Chunk::SIZE_Z);
				const int span = _settings.max_trunk_height - _settings.min_trunk_height + 1;
				const int trunk_h = _settings.min_trunk_height + static_cast<int>(next_rand(rng) % span);

				const int32_t wx = cx * Chunk::SIZE_X + lx;
				const int32_t wz = cz * Chunk::SIZE_Z + lz;
				const int32_t base_y = _settings.surface_height(wx, wz) + 1;

				if (base_y + trunk_h + 1 < origin_y || base_y > origin_y + Chunk::MAX_Y) {
					continue;
				}

				if (wx + 2 < origin_x || wx - 2 >= origin_x + Chunk::SIZE_X ||
						wz + 2 < origin_z || wz - 2 >= origin_z + Chunk::SIZE_Z) {
					continue;
				}

				for (int dy = trunk_h - 2; dy <= trunk_h + 1; ++dy) {
					const int radius = (dy >= trunk_h) ? 1 : 2;
					for (int dx = -radius; dx <= radius; ++dx) {
						for (int dz = -radius; dz <= radius; ++dz) {
							if (radius == 2 && ABS(dx) == 2 && ABS(dz) == 2) {
								continue;
							}
							put(wx + dx, base_y + dy, wz + dz, leaves_block, false);
						}
					}
				}


				for (int dy = 0; dy < trunk_h; ++dy) {
					put(wx, base_y + dy, wz, log_block, true);
				}
			}
		}
	}
}

} // namespace godot
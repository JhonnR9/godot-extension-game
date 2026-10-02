#ifndef SURFACE_VEGETATION_H
#define SURFACE_VEGETATION_H

#include "../project/generated/block_registry.generated.h"
#include <cstdint>

namespace voxel {
inline uint64_t vegetation_hash(int64_t seed, int32_t x, int32_t z) {
	uint64_t value = static_cast<uint64_t>(seed) ^
			(static_cast<uint64_t>(static_cast<int64_t>(x)) * UINT64_C(0x9E3779B97F4A7C15)) ^
			(static_cast<uint64_t>(static_cast<int64_t>(z)) * UINT64_C(0xC2B2AE3D27D4EB4F));
	value = (value ^ (value >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
	value = (value ^ (value >> 27)) * UINT64_C(0x94D049BB133111EB);
	return value ^ (value >> 31);
}

struct SurfacePlant {
	uint16_t block = 0;
	int height	   = 0;
};

// Profile values have been validated before worker jobs start. Keeping this
// helper independent of Godot makes the actual placement policy testable.
template <class Profile>
SurfacePlant sample_surface_plant(const Profile &p, int64_t seed, int32_t x, int32_t z) {
	if (p.coverage_max == 0)
		return {};
	const int size = p.patch_size;
	const int px = x >= 0 ? x / size : (x - size + 1) / size, pz = z >= 0 ? z / size : (z - size + 1) / size;
	const uint64_t patch = vegetation_hash(seed ^ 0x3A71, px, pz), sample = vegetation_hash(seed, x, z);
	const int roll = sample % 1000;
	if (roll >= p.coverage_min + int(patch % (p.coverage_max - p.coverage_min + 1)))
		return {};
	const bool flower	= roll < p.flowers_min + int((patch >> 16) % (p.flowers_max - p.flowers_min + 1));
	const auto &choices = flower ? p.flowers : p.plants;
	if (choices.empty())
		return {};
	int total = 0;
	for (const auto &v : choices)
		total += v.weight;
	int pick		  = (flower ? (patch >> 24) + (sample >> 32) % 3 : sample >> 24) % total;
	const auto *plant = &choices.back();
	for (const auto &v : choices) {
		if (pick < v.weight) {
			plant = &v;
			break;
		}
		pick -= v.weight;
	}
	const int height = plant->min_height + int((vegetation_hash(seed, x ^ 0x5A5A, z) >> 8) % (plant->max_height - plant->min_height + 1));
	return { plant->block, height };
}
} // namespace voxel
#endif

#ifndef TERRAIN_TRANSITION_H
#define TERRAIN_TRANSITION_H
#include "surface_vegetation.h"
#include <algorithm>
#include <cmath>

namespace voxel {
inline float transition_smooth(float t) {
	t = std::clamp(t, 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

// Small coherent patches mix palettes without a per-block checkerboard.
// World coordinates keep the pattern continuous across chunk boundaries.
inline float transition_patch(int64_t seed, int32_t x, int32_t z) {
	constexpr double size = 8.0;
	const int32_t px = int32_t(std::floor(x / size)), pz = int32_t(std::floor(z / size));
	const float tx = transition_smooth(float(x / size - px));
	const float tz = transition_smooth(float(z / size - pz));
	auto value = [&](int32_t a, int32_t b) {
		return float(vegetation_hash(seed ^ 0x6B31, a, b) >> 40) / 16777215.0f;
	};
	const float a = value(px, pz) * (1 - tx) + value(px + 1, pz) * tx;
	const float b = value(px, pz + 1) * (1 - tx) + value(px + 1, pz + 1) * tx;
	return a * (1 - tz) + b * tz;
}

// Dry coasts keep a shallow sandy shelf, then join the same ocean floor.
inline float coast_height(float wet, float dry, float dryness, float ocean) {
	const float shelf = dryness * (1 - transition_smooth((ocean - 0.45f) / 0.45f));
	return wet + (dry - wet) * shelf;
}
} // namespace voxel
#endif

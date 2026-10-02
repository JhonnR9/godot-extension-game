#ifndef BIOME_RARITY_H
#define BIOME_RARITY_H
#include <cmath>
#include <cstdint>
namespace voxel {
// Smooth seeded regions, independent of chunks and generation order.
inline float biome_presence(int rarity, int64_t seed, uint16_t biome, int32_t x, int32_t z) {
	if (rarity == 0)
		return 0.0f;
	if (rarity == 1)
		return 1.0f;
	constexpr double size = 256.0;
	const int64_t rx	  = static_cast<int64_t>(std::floor(x / size));
	const int64_t rz	  = static_cast<int64_t>(std::floor(z / size));
	auto hash			  = [&](int64_t a, int64_t b) {
		uint64_t v = uint64_t(seed) ^ (uint64_t(biome) * UINT64_C(0x165667B19E3779F9)) ^
				(uint64_t(a) * UINT64_C(0x9E3779B97F4A7C15)) ^ (uint64_t(b) * UINT64_C(0xC2B2AE3D27D4EB4F));
		v = (v ^ (v >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
		v = (v ^ (v >> 27)) * UINT64_C(0x94D049BB133111EB);
		v ^= v >> 31;
		return double(v >> 11) / 9007199254740992.0;
	};
	auto smooth		= [](double t) { return t * t * (3.0 - 2.0 * t); };
	const double tx = smooth(x / size - rx), tz = smooth(z / size - rz);
	const double a		   = hash(rx, rz) * (1 - tx) + hash(rx + 1, rz) * tx;
	const double b		   = hash(rx, rz + 1) * (1 - tx) + hash(rx + 1, rz + 1) * tx;
	const double value	   = a * (1 - tz) + b * tz;
	const double threshold = 1.0 / rarity;
	// Blend the relief through a narrow band surrounding the selection boundary.
	const double width = threshold * 0.20;
	double t		   = (threshold + width - value) / (2 * width);
	t				   = t < 0 ? 0 : t > 1 ? 1
										   : t;
	return float(smooth(t));
}
} //namespace voxel
#endif

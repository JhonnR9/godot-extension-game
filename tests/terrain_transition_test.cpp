#include "../src/terrain_transition.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
	float low = 1, high = 0;
	for (int z = -64; z <= 64; ++z)
		for (int x = -64; x <= 64; ++x) {
			const float v = voxel::transition_patch(42, x, z);
			assert(v >= 0 && v <= 1);
			assert(v == voxel::transition_patch(42, x, z));
			assert(std::abs(v - voxel::transition_patch(42, x + 1, z)) < 0.19f);
			assert(std::abs(v - voxel::transition_patch(42, x, z + 1)) < 0.19f);
			low = std::min(low, v);
			high = std::max(high, v);
		}
	assert(low < 0.1f && high > 0.9f);
	assert(voxel::transition_patch(42, -16, 16) != voxel::transition_patch(43, -16, 16));
	for (float dry : {0.0f, 0.5f, 1.0f}) {
		float previous = 25;
		for (int i = 0; i <= 100; ++i) {
			float height = voxel::coast_height(16, 25, dry, i / 100.0f);
			assert(height <= previous && height >= 16);
			previous = height;
		}
		assert(previous == 16);
	}
	assert(voxel::coast_height(16, 25, 1, 0.4f) == 25);
	assert(voxel::freezing_weight(0.0f, 0.18f, 0.14f) == 1);
	assert(voxel::freezing_weight(0.4f, 0.18f, 0.14f) == 0);
	assert(std::abs(voxel::freezing_weight(0.18f, 0.18f, 0.14f) - 0.5f) < 0.0001f);
	for (float patch : {0.0f, 0.5f, 1.0f}) {
		assert(voxel::freezing_height(0, patch, 16, 24) == 16);
		assert(voxel::freezing_height(1, patch, 16, 24) == 23);
		int previous = 16;
		for (int i = 0; i <= 100; ++i) {
			const int height = voxel::freezing_height(i / 100.0f, patch, 16, 24);
			assert(height >= previous && height <= previous + 1 && height <= 23);
			previous = height;
		}
	}
	std::cout << "Terrain transition checks passed.\n";
}

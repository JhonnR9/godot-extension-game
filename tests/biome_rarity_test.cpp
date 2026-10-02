#include "../src/biome_rarity.h"
#include <cassert>
#include <iostream>
int main() {
	int counts[4] = {}, changed = 0;
	for (int z = -8192; z < 8192; z += 17)
		for (int x = -8192; x < 8192; x += 17) {
			assert(voxel::biome_presence(0, 42, 1, x, z) == 0);
			assert(voxel::biome_presence(1, 42, 1, x, z) == 1);
			const float p2 = voxel::biome_presence(2, 42, 1, x, z), p4 = voxel::biome_presence(4, 42, 1, x, z), p8 = voxel::biome_presence(8, 42, 1, x, z);
			assert(p2 >= p4 && p4 >= p8);
			assert(p4 == voxel::biome_presence(4, 42, 1, x, z));
			counts[0]++;
			counts[1] += p2 >= 0.5f;
			counts[2] += p4 >= 0.5f;
			counts[3] += p8 >= 0.5f;
			changed += (p4 >= 0.5f) != (voxel::biome_presence(4, 43, 1, x, z) >= 0.5f);
			// No discontinuity at lattice/chunk boundaries, including negatives.
			assert(std::abs(p4 - voxel::biome_presence(4, 42, 1, x + 1, z)) < 0.10f);
		}
	assert(counts[1] > counts[2] && counts[2] > counts[3] && counts[3] > 100);
	assert(counts[1] > counts[0] * 0.4 && counts[1] < counts[0] * 0.6);
	assert(changed > 1000);
	std::cout << "Rarity region populations: " << counts[1] << ", " << counts[2] << ", " << counts[3] << '\n';
}

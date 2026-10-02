#include "../src/surface_vegetation.h"
#include <array>
#include <cassert>
#include <iostream>
#include <vector>

struct Plant { uint16_t block; int weight; int min_height=1,max_height=1; };
struct Profile {
	int patch_chance=1000,cluster_radius=0;
    int patch_size=12,coverage_min=85,coverage_max=159,flowers_min=12,flowers_max=20;
    std::vector<Plant> plants={{20,62},{15,28},{21,10}},flowers={{14,1},{17,1},{18,1},{19,1}};
};
int main() {
    Profile profile;
    Profile empty=profile; empty.coverage_min=empty.coverage_max=0;
    Profile cactus; cactus.coverage_min=cactus.coverage_max=1000;
    cactus.flowers_min=cactus.flowers_max=0; cactus.plants={{voxel::block_ids::cactus,1,1,3}};
    int height_counts[4]={};
    for(int z=-32;z<32;++z) for(int x=-32;x<32;++x) {
        assert(voxel::sample_surface_plant(empty,42,x,z).block==0);
        const auto v=voxel::sample_surface_plant(cactus,42,x,z);
        assert(v.block==voxel::block_ids::cactus && v.height>=1 && v.height<=3);
        ++height_counts[v.height];
    }
    for(int i=1;i<=3;++i) assert(height_counts[i]>500);
	Profile clumps;
	clumps.patch_size=24; clumps.patch_chance=180; clumps.cluster_radius=3;
	clumps.coverage_min=600; clumps.coverage_max=850;
	clumps.flowers_min=clumps.flowers_max=0; clumps.plants=clumps.flowers;
	int clump_flowers=0;
	for(int z=-256;z<256;++z) for(int x=-256;x<256;++x) {
		const auto plant=voxel::sample_surface_plant(clumps,42,x,z).block;
		if(plant) {
			assert(plant==14 || plant==17 || plant==18 || plant==19);
			++clump_flowers;
		}
	}
	assert(clump_flowers>100 && clump_flowers<512*512/30);

    for (int64_t seed : {0,42,-17}) {
        std::array<int, 22> counts{};
        for (int z=-256; z<256; ++z) for (int x=-256; x<256; ++x) {
            const auto plant=voxel::sample_surface_plant(profile,seed,x,z).block;
            assert(plant==voxel::sample_surface_plant(profile,seed,x,z).block);
            assert(plant==voxel::block_ids::air || plant==voxel::block_ids::flower ||
                   plant==voxel::block_ids::daisy || plant==voxel::block_ids::cornflower ||
                   plant==voxel::block_ids::poppy || plant==voxel::block_ids::short_grass ||
                   plant==voxel::block_ids::tall_grass || plant==voxel::block_ids::fern);
            if (plant != voxel::block_ids::air) {
                const auto flags=voxel::default_block_flags(plant);
                assert((flags & (1u << 16)) != 0);
                assert((flags & (1u << 14)) != 0);
                assert((flags & (1u << 10)) == 0);
            }
            ++counts[plant];
        }
        const int total=512*512;
        const double density=1.0-static_cast<double>(counts[0])/total;
        assert(density>0.08 && density<0.17);
        for (int id : {14,15,17,18,19,20,21}) assert(counts[id]>100);
        assert(counts[20] > counts[15]);
        int flowers=counts[14]+counts[17]+counts[18]+counts[19];
        assert(flowers < counts[20]/2);
        std::cout << "seed " << seed << ": " << density*100 << "% plants, " << flowers << " flowers\n";
    }
}

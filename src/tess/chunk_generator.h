#ifndef CHUNK_GENERATOR_H
#define CHUNK_GENERATOR_H

#include "../chunk_model.h"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/fast_noise_lite.hpp"

namespace godot {

struct NoiseSet {
	Ref<FastNoiseLite> terrain_noise;
	Ref<FastNoiseLite> cave_noise;
};

struct TerrainSettings {
	int terrain_base_height;
	float terrain_amplitude;
	float cave_threshold;
	NoiseSet noise_set;
};

class ChunkGenerator {
public:
	static ChunkModel generate(
		Vector3i chunk_pos,
		const TerrainSettings &terrain_settings
	);
};

}

#endif
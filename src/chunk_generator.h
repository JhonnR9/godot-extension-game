

#ifndef CHUNK_GENERATOR_H
#define CHUNK_GENERATOR_H
#include "chunk_model.h"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/variant/vector3i.hpp"

#include <cstdint>
#include <godot_cpp/classes/fast_noise_lite.hpp>

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
	static void generate_tree(ChunkModel &chunk, int start_x, int start_y, int start_z);
public:
	static ChunkModel generate(Vector3i chunk_pos, const TerrainSettings & terrain_settings);
};

} //namespace godot

#endif //CHUNK_GENERATOR_H

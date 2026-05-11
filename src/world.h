#ifndef WORLD_H
#define WORLD_H

#include "chunk.h"
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/vector3i.hpp>
#include <godot_cpp/classes/fast_noise_lite.hpp>

namespace godot {

class World : public Node3D {
    GDCLASS(World, Node3D)

public:
    World() = default;
    ~World() = default;

    void _ready() override;

    bool is_air_global(int wx, int wy, int wz) const;
    void generate_world(int radius, int height);

    void set_seed(uint64_t p_seed);
    uint64_t get_seed() const;

    void set_world_radius(int p_radius);
    int get_world_radius() const;

    void set_world_height(int p_height);
    int get_world_height() const { return _world_height; }

    void set_terrain_base_height(int p_height);
    int get_terrain_base_height() const;

    void set_terrain_amplitude(float p_amplitude);
    float get_terrain_amplitude() const;

    void set_dirt_layer_depth(int p_depth);
    int get_dirt_layer_depth() const { return _dirt_layer_depth; }

    void set_terrain_noise(const Ref<FastNoiseLite> &p_noise);
    Ref<FastNoiseLite> get_terrain_noise() const;

    void set_cave_noise(const Ref<FastNoiseLite> &p_noise);
    Ref<FastNoiseLite> get_cave_noise() const;

    void _regenerate_world();

protected:
    static void _bind_methods();

private:
    uint64_t _seed = 12345;
    bool _is_generating{false};

    int _world_radius = 4;
    int _world_height = 4;

    int _terrain_base_height = 16;
    float _terrain_amplitude = 16.0f;
    int _dirt_layer_depth = 4;

    Ref<FastNoiseLite> _terrain_noise;
    Ref<FastNoiseLite> _cave_noise;


    HashMap<Vector3i, Chunk *> _chunks;
    void _create_chunk(Vector3i chunk_pos);
};

} // namespace godot

#endif
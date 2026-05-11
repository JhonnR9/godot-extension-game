#include "world.h"

#include <godot_cpp/core/class_db.hpp>

namespace godot {

void World::_ready() {
	generate_world(_world_radius, _world_height);
}

void World::generate_world(int radius, int height) {
	for (int x = -radius; x <= radius; x++) {
		for (int y = -height; y <= height; y++) {
			for (int z = -radius; z <= radius; z++) {
				_create_chunk(Vector3i(x, y, z));
			}
		}
	}

	for (const KeyValue<Vector3i, Chunk *> &kv : _chunks) {
		if (kv.value) {
			kv.value->rebuild_mesh();
		}
	}
}

void World::_regenerate_world() {
	if (_terrain_noise.is_null() || _cave_noise.is_null()) {
		return;
	}

	if (_is_generating) return;
	_is_generating = true;

	for (const KeyValue<Vector3i, Chunk *> &kv : _chunks) {
		if (kv.value) {
			remove_child(kv.value);
			kv.value->queue_free();
		}
	}
	_chunks.clear();

	generate_world(_world_radius, _world_height);

	_is_generating = false;
}

void World::set_terrain_noise(const Ref<FastNoiseLite> &p_noise) {
	if (_terrain_noise == p_noise) return;

	if (_terrain_noise.is_valid()) {
		_terrain_noise->disconnect("changed", callable_mp(this, &World::_regenerate_world));
	}

	_terrain_noise = p_noise;

	if (_terrain_noise.is_valid()) {
		_terrain_noise->set_seed(static_cast<int>(_seed));
		_terrain_noise->connect("changed", callable_mp(this, &World::_regenerate_world));
	}

	_regenerate_world();
}

Ref<FastNoiseLite> World::get_terrain_noise() const {
	return _terrain_noise;
}

void World::set_cave_noise(const Ref<FastNoiseLite> &p_noise) {
	if (_cave_noise == p_noise) return;

	if (_cave_noise.is_valid()) {
		_cave_noise->disconnect("changed", callable_mp(this, &World::_regenerate_world));
	}

	_cave_noise = p_noise;

	if (_cave_noise.is_valid()) {
		_cave_noise->set_seed(static_cast<int>(_seed) + 123);
		_cave_noise->connect("changed", callable_mp(this, &World::_regenerate_world));
	}

	_regenerate_world();
}

Ref<FastNoiseLite> World::get_cave_noise() const {
	return _cave_noise;
}
void World::set_seed(uint64_t p_seed) {
	if (_seed == p_seed) return;
	_seed = p_seed;

	if (_terrain_noise.is_valid()) {
		_terrain_noise->set_seed(static_cast<int>(_seed));
	}
	if (_cave_noise.is_valid()) {
		_cave_noise->set_seed(static_cast<int>(_seed) + 123);
	}

	if (is_inside_tree()) {
		_regenerate_world();
	}
}

uint64_t World::get_seed() const {
	return _seed;
}

void World::set_world_radius(int p_radius) {
	if (_world_radius == p_radius) {
		return;
	}

	_world_radius = p_radius;

	if (is_inside_tree()) {
		_regenerate_world();
	}
}

int World::get_world_radius() const {
	return _world_radius;
}

void World::set_terrain_base_height(int p_height) {
	if (_terrain_base_height == p_height) {
		return;
	}

	_terrain_base_height = p_height;

	if (is_inside_tree()) {
		_regenerate_world();
	}
}

int World::get_terrain_base_height() const {
	return _terrain_base_height;
}

void World::set_terrain_amplitude(float p_amplitude) {
	if (_terrain_amplitude == p_amplitude) {
		return;
	}

	_terrain_amplitude = p_amplitude;

	if (is_inside_tree()) {
		_regenerate_world();
	}
}

float World::get_terrain_amplitude() const {
	return _terrain_amplitude;
}

void World::set_dirt_layer_depth(int p_depth) {
	if (_dirt_layer_depth == p_depth) {
		return;
	}

	_dirt_layer_depth = p_depth;

	if (is_inside_tree()) {
		_regenerate_world();
	}
}
void World::set_world_height(int p_height) {
	if (p_height == _world_height) {
		return;
	}
	this->_world_height = p_height;

	if (is_inside_tree()) {
		_regenerate_world();
	}
}

void World::_create_chunk(Vector3i chunk_pos) {

	if (_terrain_noise.is_null() || _cave_noise.is_null()) {
		return;
	}

	Vector3i key = chunk_pos;
	if (_chunks.has(key)) return;

	Chunk *chunk = memnew(Chunk);
	add_child(chunk);
	chunk->set_world(this);
	chunk->set_position(Vector3(chunk_pos.x * Chunk::SIZE, chunk_pos.y * Chunk::SIZE, chunk_pos.z * Chunk::SIZE));

	_chunks.insert(key, chunk);

	chunk->generate(_seed, chunk_pos, _terrain_noise, _cave_noise, _terrain_base_height, _terrain_amplitude);
}

bool World::is_air_global(int wx, int wy, int wz) const {
	int cx = Math::floor((float)wx / Chunk::SIZE);
	int cy = Math::floor((float)wy / Chunk::SIZE);
	int cz = Math::floor((float)wz / Chunk::SIZE);

	int lx = Math::posmod(wx, Chunk::SIZE);
	int ly = Math::posmod(wy, Chunk::SIZE);
	int lz = Math::posmod(wz, Chunk::SIZE);

	Vector3i key(cx, cy, cz);

	if (!_chunks.has(key)) {
		return true;
	}

	Chunk *chunk = _chunks[key];

	return chunk->get_block(lx, ly, lz).is_air();
}

void World::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_seed", "seed"), &World::set_seed);
    ClassDB::bind_method(D_METHOD("get_seed"), &World::get_seed);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "seed"), "set_seed", "get_seed");

    ClassDB::bind_method(D_METHOD("set_world_radius", "radius"), &World::set_world_radius);
    ClassDB::bind_method(D_METHOD("get_world_radius"), &World::get_world_radius);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "world_radius"), "set_world_radius", "get_world_radius");

    ClassDB::bind_method(D_METHOD("set_world_height", "height"), &World::set_world_height);
    ClassDB::bind_method(D_METHOD("get_world_height"), &World::get_world_height);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "world_height"), "set_world_height", "get_world_height");

    ADD_GROUP("Terrain Settings", "terrain_");

    ClassDB::bind_method(D_METHOD("set_terrain_base_height", "height"), &World::set_terrain_base_height);
    ClassDB::bind_method(D_METHOD("get_terrain_base_height"), &World::get_terrain_base_height);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "terrain_base_height"), "set_terrain_base_height", "get_terrain_base_height");

    ClassDB::bind_method(D_METHOD("set_terrain_amplitude", "amplitude"), &World::set_terrain_amplitude);
    ClassDB::bind_method(D_METHOD("get_terrain_amplitude"), &World::get_terrain_amplitude);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "terrain_amplitude"), "set_terrain_amplitude", "get_terrain_amplitude");

    ClassDB::bind_method(D_METHOD("set_dirt_layer_depth", "depth"), &World::set_dirt_layer_depth);
    ClassDB::bind_method(D_METHOD("get_dirt_layer_depth"), &World::get_dirt_layer_depth);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "dirt_layer_depth"), "set_dirt_layer_depth", "get_dirt_layer_depth");

    ADD_GROUP("Noise Resources", "");

    ClassDB::bind_method(D_METHOD("set_terrain_noise", "noise"), &World::set_terrain_noise);
    ClassDB::bind_method(D_METHOD("get_terrain_noise"), &World::get_terrain_noise);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "terrain_noise", PROPERTY_HINT_RESOURCE_TYPE, "FastNoiseLite"), "set_terrain_noise", "get_terrain_noise");

    ClassDB::bind_method(D_METHOD("set_cave_noise", "noise"), &World::set_cave_noise);
    ClassDB::bind_method(D_METHOD("get_cave_noise"), &World::get_cave_noise);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "cave_noise", PROPERTY_HINT_RESOURCE_TYPE, "FastNoiseLite"), "set_cave_noise", "get_cave_noise");


    ClassDB::bind_method(D_METHOD("regenerate_world"), &World::_regenerate_world);
}

} // namespace godot
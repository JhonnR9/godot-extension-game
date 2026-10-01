//
// Created by jhone on 13/05/2026.
//

#ifndef CHUNK_MESH_BUILDER_H
#define CHUNK_MESH_BUILDER_H

#include "chunk_model.h"
#include "voxel_mesher.h"
#include "godot_cpp/templates/hash_map.hpp"

#include <godot_cpp/classes/array_mesh.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <godot_cpp/classes/texture2d_array.hpp>

namespace godot {
struct ChunkNeighbors {
	std::shared_ptr<Chunk> center;

	std::shared_ptr<Chunk> right;
	std::shared_ptr<Chunk> left;

	std::shared_ptr<Chunk> top;
	std::shared_ptr<Chunk> bottom;

	std::shared_ptr<Chunk> front;
	std::shared_ptr<Chunk> back;
};

struct TextureKey {
	voxel::BlockType type;
	CubeFace face;

	bool operator==(const TextureKey &p_other) const {
		return type == p_other.type && face == p_other.face;
	}
};

struct TextureKeyHasher {
	static uint32_t hash(const TextureKey &p_key) {
		uint32_t h = hash_murmur3_buffer(&p_key.type, sizeof(voxel::BlockType));
		h          = hash_murmur3_buffer(&p_key.face, sizeof(CubeFace), h);
		return h;
	}
};

class ChunkMeshBuilder {
	VoxelMesher opaque_mesher;
	VoxelMesher transparent_mesher;
	VoxelMesher &_get_mesher(voxel::Block block);
	void _add_faces(const ChunkNeighbors &neighbors, CubeFace face);
	void _add_crossed_plant_faces(const ChunkNeighbors &neighbors);
	int _get_tex_layer(const CubeFace &face, const voxel::BlockType &type);

	Ref<Texture2DArray> block_texture_array;
	void _load_textures();
	void _initialize_texture_map();
	static voxel::BlockType map_string_to_type(const String &name);

	HashMap<TextureKey, int, TextureKeyHasher> texture_map;
	static voxel::Block _get_block(const ChunkNeighbors &neighbors, int x, int y, int z);
	static bool _is_face_visible(const ChunkNeighbors &neighbors, int x, int y, int z, voxel::Block current_block);
	static bool _is_crossed_plant(voxel::Block block);

public:
	ChunkMeshBuilder();
	Ref<ArrayMesh> build(const ChunkNeighbors &neighbors);

	static bool _is_air(const ChunkNeighbors &n, int x, int y, int z);

	PackedVector3Array get_last_collision_faces() const {
		return opaque_mesher.get_collision_faces();
	}
};
} // namespace godot

#endif

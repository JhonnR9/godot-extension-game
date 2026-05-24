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
	std::shared_ptr<ChunkModel> center;

	std::shared_ptr<ChunkModel> right;
	std::shared_ptr<ChunkModel> left;

	std::shared_ptr<ChunkModel> top;
	std::shared_ptr<ChunkModel> bottom;

	std::shared_ptr<ChunkModel> front;
	std::shared_ptr<ChunkModel> back;
};

struct TextureKey {
	BlockType type;
	CubeFace face;

	bool operator==(const TextureKey &p_other) const {
		return type == p_other.type && face == p_other.face;
	}
};
struct TextureKeyHasher {
	static uint32_t hash(const TextureKey &p_key) {
		uint32_t h = hash_murmur3_buffer(&p_key.type, sizeof(BlockType));
		h = hash_murmur3_buffer(&p_key.face, sizeof(CubeFace), h);
		return h;
	}
};

class ChunkMeshBuilder {
	VoxelMesher mesher;
	void _add_right_faces(const ChunkNeighbors& neighbors);
	void _add_up_faces(const ChunkNeighbors& neighbors);
	void _add_left_faces(const ChunkNeighbors& neighbors);
	void _add_down_faces(const ChunkNeighbors& neighbors);

	void _add_front_faces(const ChunkNeighbors& neighbors);
	void _add_back_faces(const ChunkNeighbors& neighbors);
	int _get_tex_layer(const CubeFace& face,const BlockType& type);

	Ref<Texture2DArray> block_texture_array;
	void _load_textures();
	void _initialize_texture_map();
	static BlockType map_string_to_type(const String &name);

	HashMap<TextureKey, int, TextureKeyHasher> texture_map;
public:
	ChunkMeshBuilder();
	Ref<ArrayMesh> build(const ChunkNeighbors& neighbors);

	static bool _is_air(const ChunkNeighbors& n, int x, int y, int z);

	PackedVector3Array get_last_collision_faces() const {
		return mesher.get_collision_faces();
	}
};

} // namespace godot

#endif
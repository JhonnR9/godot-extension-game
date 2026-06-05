#ifndef CHUNK_REPOSITORY_H
#define CHUNK_REPOSITORY_H
#include "voxel.h"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/hash_set.hpp"

#include <godot_cpp/classes/ref_counted.hpp>
#include <memory>

namespace godot {
struct ChunkModel;


class ChunkRepository : public RefCounted {
	GDCLASS(ChunkRepository, RefCounted)
	std::mutex _mutex;
	HashMap<Vector3i, std::shared_ptr<ChunkModel>> _chunks;
	std::mutex _dirty_chunks_mutex;
	HashSet<Vector3i> _dirty_chunks;

	std::mutex _chunk_versions_mutex;
	HashMap<Vector3i, uint64_t> _chunk_versions;

	std::mutex _edited_blocks_mutex;
	HashMap<Vector3i, HashMap<Vector3i, voxel::Block>> _edited_chunks;

	WorldModel world_model_;

protected:
	static void _bind_methods();

public:
	void add_chunk(const Vector3i &p_pos, const std::shared_ptr<ChunkModel> &p_model);
	std::shared_ptr<ChunkModel> get_chunk(const Vector3i &p_pos);
	bool contains_chunk(const Vector3i &p_pos);
	void remove_chunk(const Vector3i &p_pos);
	Vector<Vector3i> get_keys_snapshot();
	void clear_all();
	void set_block(const Vector3i &world_block_pos, voxel::Block block);
	HashSet<Vector3i> consume_dirty_chunks();
	uint64_t get_chunk_version(const Vector3i &p_pos);
	bool is_chunk_dirty(const Vector3i &p_pos);

	void set_world_model(const WorldModel &p_world);
	WorldModel get_world_model(uint64_t p_id);
	HashSet<int64_t> get_saved_worlds() const;

private:
	void _update_dirty_chunks(const Vector3i &p_local_pos, const Vector3i &p_chunk_pos);
	void _apply_edited_blocks(const Vector3i &p_chunk_pos, const std::shared_ptr<ChunkModel> &p_model);
};
} // godot

#endif //CHUNK_REPOSITORY_H
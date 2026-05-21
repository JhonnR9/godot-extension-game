
#ifndef CHUNK_REPOSITORY_H
#define CHUNK_REPOSITORY_H
#include "block.h"
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

	HashMap<Vector3i, uint64_t> _chunk_versions;
protected:
	static void _bind_methods();
public:
	void add_chunk(const Vector3i &p_pos, const std::shared_ptr<ChunkModel> &p_model);
	std::shared_ptr<ChunkModel> get_chunk(const Vector3i &p_pos);
	bool contains_chunk(const Vector3i &p_pos);
	void remove_chunk(const Vector3i &p_pos);
	Vector<Vector3i> get_keys_snapshot();
	void clear_all();
	void set_block(const Vector3i &world_block_pos,block::Block block);
	HashSet<Vector3i>consume_dirty_chunks();
	uint64_t get_chunk_version(const Vector3i &p_pos);

};


} // godot

#endif //CHUNK_REPOSITORY_H

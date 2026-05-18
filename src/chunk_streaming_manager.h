//
// Created by jhone on 17/05/2026.
//

#ifndef CHUNK_STREAMING_MANAGER_H
#define CHUNK_STREAMING_MANAGER_H
#include "godot_cpp/templates/hash_set.hpp"
#include <godot_cpp/classes/ref_counted.hpp>

namespace godot {

class ChunkNode;

struct StreamSettings {
	int world_radius = 1;
	int cache_radius = 1;
	int world_height = 1;
};

class ChunkStreamingManager final : public RefCounted {
	GDCLASS(ChunkStreamingManager, RefCounted)

protected:
	static void _bind_methods();

private:
	std::mutex _chunks_mutex;
	HashSet<Vector3i> _active_chunks;

	std::mutex _queue_free_chunks_mutex;
	HashSet<Vector3i> _queue_free_chunks;
	StreamSettings _stream_settings;
public:
	void set_stream_settings(StreamSettings _p_settings) {
		_stream_settings = _p_settings;
	}
	void shift_chunks(const Vector3i & p_pos_center);
	void rebuild_all_chunks(const Vector3i & p_pos_center);
	bool is_chunk_active(const Vector3i &p_pos);
	HashSet<Vector3i> pop_queue_free_chunks();
	HashSet<Vector3i> get_active_chunks_snapshot();
};

} //namespace godot

#endif //CHUNK_STREAMING_MANAGER_H

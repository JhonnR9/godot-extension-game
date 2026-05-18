
#include "chunk_streaming_manager.h"

namespace godot {

void ChunkStreamingManager::_bind_methods() {
}

void ChunkStreamingManager::shift_chunks(const Vector3i &p_pos_center) {
	const int radius_xz = _stream_settings.world_radius;
	const int radius_y = _stream_settings.world_height;

	HashSet<Vector3i> new_active_chunks;
	const int estimated_size = (2 * radius_xz + 1) * (2 * radius_y + 1) * (2 * radius_xz + 1);
	new_active_chunks.reserve(estimated_size);
	Vector3i chunk{};
	for (int x = -radius_xz; x <= radius_xz; x++) {
		for (int y = -radius_y; y <= radius_y; y++) {
			for (int z = -radius_xz; z <= radius_xz; z++) {
				chunk.x = p_pos_center.x + x;
				chunk.y = p_pos_center.y + y;
				chunk.z = p_pos_center.z + z;

				new_active_chunks.insert(chunk);
			}
		}
	}
	std::lock_guard lock(_chunks_mutex);
	Vector<Vector3i> chunks_to_remove;

	for (const Vector3i &pos : _active_chunks) {
		if (!new_active_chunks.has(pos)) {
			chunks_to_remove.push_back(pos);
		}
	}

	for (const Vector3i &pos : chunks_to_remove) {
		_active_chunks.erase(pos);
	}

	{
		std::lock_guard free_lock(_queue_free_chunks_mutex);

		for (const Vector3i &pos : chunks_to_remove) {
			_queue_free_chunks.insert(pos);
		}
	}

	_active_chunks = std::move(new_active_chunks);
}

void ChunkStreamingManager::rebuild_all_chunks(const Vector3i &p_pos_center) {
	const int size = (2 * _stream_settings.world_radius + 1) * (2 * _stream_settings.world_height + 1) * (2 * _stream_settings.world_radius + 1);
	_active_chunks.reserve(size);

	for (int x = -_stream_settings.world_radius; x <= _stream_settings.world_radius; x++) {
		for (int y = -_stream_settings.world_height; y <= _stream_settings.world_height; y++) {
			for (int z = -_stream_settings.world_radius; z <= _stream_settings.world_radius; z++) {
				Vector3i pos{
					p_pos_center.x + x,
					p_pos_center.y + y,
					p_pos_center.z + z
				};
				_active_chunks.insert(pos);

			}
		}
	}
}
bool ChunkStreamingManager::is_chunk_active(const Vector3i &p_pos) {
	return _active_chunks.has(p_pos);
}
HashSet<Vector3i> ChunkStreamingManager::pop_queue_free_chunks() {
	std::lock_guard lock(_queue_free_chunks_mutex);
	HashSet<Vector3i> to_remove_chunks = _queue_free_chunks;
	_queue_free_chunks.clear();

	return to_remove_chunks;
}

HashSet<Vector3i> ChunkStreamingManager::get_active_chunks_snapshot() {
	std::lock_guard lock(_chunks_mutex);
	HashSet<Vector3i> chunks_snapshot = _active_chunks;

	return chunks_snapshot;
}
} //namespace godot
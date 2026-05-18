
#include "chunk_repository.h"
#include "chunk_model.h"

namespace godot {
void ChunkRepository::_bind_methods() {
}
void ChunkRepository::add_chunk(const Vector3i &p_pos, const std::shared_ptr<ChunkModel> &p_model) {
	std::lock_guard lock(_mutex);
	_chunks[p_pos] = p_model;
}
std::shared_ptr<ChunkModel> ChunkRepository::get_chunk(const Vector3i &p_pos) {
	std::lock_guard lock(_mutex);
	if (_chunks.has(p_pos)) {
		return _chunks[p_pos];
	}

	return nullptr;
}
bool ChunkRepository::contains_chunk(const Vector3i &p_pos) {
	std::lock_guard lock(_mutex);
	return _chunks.has(p_pos);
}

void ChunkRepository::remove_chunk(const Vector3i &p_pos) {
	std::lock_guard lock(_mutex);
	if (contains_chunk(p_pos))
		_chunks.erase(p_pos);
}

Vector<Vector3i> ChunkRepository::get_keys_snapshot() {
	std::lock_guard lock(_mutex);

	Vector<Vector3i> keys;
	keys.resize(_chunks.size());

	for (const auto &E : _chunks) {
		keys.push_back(E.key);
	}

	return keys;
}

void ChunkRepository::clear_all() {
	std::lock_guard<std::mutex> lock(_mutex);
	_chunks.clear();
}

} //namespace godot
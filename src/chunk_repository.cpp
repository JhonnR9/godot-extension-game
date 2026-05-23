
#include "chunk_repository.h"
#include "chunk_model.h"

namespace godot {
uint64_t ChunkRepository::get_chunk_version(const Vector3i &p_pos) {
	std::lock_guard lock(_mutex);

	if (_chunk_versions.has(p_pos)) {
		return _chunk_versions[p_pos];
	}

	return 0;
}

bool ChunkRepository::is_chunk_dirty(const Vector3i &p_pos) {
	std::lock_guard lock(_dirty_chunks_mutex);
	return _dirty_chunks.has(p_pos);
}

void ChunkRepository::_bind_methods() {
}
void ChunkRepository::add_chunk(const Vector3i &p_pos, const std::shared_ptr<ChunkModel> &p_model) {
	std::lock_guard lock(_mutex);

	_chunks[p_pos] = p_model;

	if (!_chunk_versions.has(p_pos)) {
		_chunk_versions[p_pos] = 1;
	}
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
	if (_chunks.has(p_pos))
		_chunks.erase(p_pos);
}

Vector<Vector3i> ChunkRepository::get_keys_snapshot() {
	std::lock_guard lock(_mutex);

	Vector<Vector3i> keys;

	for (const auto &E : _chunks) {
		keys.push_back(E.key);
	}
	return keys;
}

void ChunkRepository::clear_all() {
	std::lock_guard<std::mutex> lock(_mutex);
	_chunks.clear();
}
void ChunkRepository::set_block(const Vector3i &world_block_pos, block::Block block) {
	Vector3i chunk_pos(
			Math::floor((float)world_block_pos.x / ChunkModel::SIZE_X),
			Math::floor((float)world_block_pos.y / ChunkModel::SIZE_Y),
			Math::floor((float)world_block_pos.z / ChunkModel::SIZE_Z));

	auto chunk = get_chunk(chunk_pos);

	if (!chunk) {
		return;
	}

	int local_x = Math::posmod(world_block_pos.x, ChunkModel::SIZE_X);
	int local_y = Math::posmod(world_block_pos.y, ChunkModel::SIZE_Y);
	int local_z = Math::posmod(world_block_pos.z, ChunkModel::SIZE_Z);

	chunk->set_block(local_x, local_y, local_z, 0);

	{
		std::lock_guard lock(_mutex);

		_dirty_chunks.insert(chunk_pos);

		if (local_x == 0)
			_dirty_chunks.insert(chunk_pos + Vector3i(-1, 0, 0));

		if (local_x == ChunkModel::SIZE_X - 1)
			_dirty_chunks.insert(chunk_pos + Vector3i(1, 0, 0));

		if (local_y == 0)
			_dirty_chunks.insert(chunk_pos + Vector3i(0, -1, 0));

		if (local_y == ChunkModel::SIZE_Y - 1)
			_dirty_chunks.insert(chunk_pos + Vector3i(0, 1, 0));

		if (local_z == 0)
			_dirty_chunks.insert(chunk_pos + Vector3i(0, 0, -1));

		if (local_z == ChunkModel::SIZE_Z - 1)
			_dirty_chunks.insert(chunk_pos + Vector3i(0, 0, 1));
	}
	_chunk_versions[chunk_pos]++;
}
HashSet<Vector3i> ChunkRepository::consume_dirty_chunks() {
	std::lock_guard lock(_dirty_chunks_mutex);
	HashSet<Vector3i> dirty = _dirty_chunks;

	_dirty_chunks.clear();

	return dirty;
}

} //namespace godot
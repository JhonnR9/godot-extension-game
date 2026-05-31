#include "chunk_repository.h"
#include "chunk_model.h"
#include "utils.h"

namespace godot {
uint64_t ChunkRepository::get_chunk_version(const Vector3i &p_pos) {
	std::lock_guard lock(_chunk_versions_mutex);
	if (_chunk_versions.has(p_pos)) {
		return _chunk_versions[p_pos];
	}

	return 0;
}

bool ChunkRepository::is_chunk_dirty(const Vector3i &p_pos) {
	std::lock_guard lock(_dirty_chunks_mutex);
	return _dirty_chunks.has(p_pos);
}

void ChunkRepository::_update_dirty_chunks(const Vector3i &p_local_pos, const Vector3i& p_chunk_pos) {
	std::lock_guard lock(_dirty_chunks_mutex);

	_dirty_chunks.insert(p_chunk_pos);

	if (p_local_pos.x == ChunkModel::MIN_X) _dirty_chunks.insert(p_chunk_pos + voxel::DIR_LEFT);
	if (p_local_pos.x == ChunkModel::MAX_X) _dirty_chunks.insert(p_chunk_pos + voxel::DIR_RIGHT);

	if (p_local_pos.y == ChunkModel::MIN_Y) _dirty_chunks.insert(p_chunk_pos + voxel::DIR_DOWN);
	if (p_local_pos.y == ChunkModel::MAX_Y) _dirty_chunks.insert(p_chunk_pos + voxel::DIR_UP);

	if (p_local_pos.z == ChunkModel::MIN_Z) _dirty_chunks.insert(p_chunk_pos + voxel::DIR_BACK);
	if (p_local_pos.z == ChunkModel::MAX_Z) _dirty_chunks.insert(p_chunk_pos + voxel::DIR_FRONT);
}

void ChunkRepository::_apply_edited_blocks(const Vector3i &p_chunk_pos, const std::shared_ptr<ChunkModel> &p_model) {
	{
		std::lock_guard lock(_edited_blocks_mutex);
		if (_edited_chunks.has(p_chunk_pos)) {
			const HashMap<Vector3i, voxel::Block> &blocks = _edited_chunks[p_chunk_pos];

			for (const auto &E : blocks) {
				const Vector3i &local_pos = E.key;
				const voxel::Block &block = E.value;
				p_model->set_block(local_pos.x, local_pos.y, local_pos.z, block);
			}
		}
	}
}


void ChunkRepository::_bind_methods() {
}

void ChunkRepository::add_chunk(const Vector3i &p_pos, const std::shared_ptr<ChunkModel> &p_model) {
	{
		std::lock_guard lock(_mutex);
		_chunks[p_pos] = p_model;
	}

	_apply_edited_blocks(p_pos, p_model);

	{
		std::lock_guard lock(_chunk_versions_mutex);
		if (!_chunk_versions.has(p_pos)) {
			_chunk_versions[p_pos] = 1;
		}
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
	{
		std::lock_guard lock(_mutex);
		if (_chunks.has(p_pos))
			_chunks.erase(p_pos);

	}
	{
		std::lock_guard lock(_chunk_versions_mutex);
		if (_chunk_versions.has(p_pos)) {
			_chunk_versions.erase(p_pos);
		}
	}
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
	{
		std::lock_guard lock(_mutex);
		_chunks.clear();
	}

	{
		std::lock_guard lock(_chunk_versions_mutex);
		_chunk_versions.clear();
	}

	{
		std::lock_guard lock(_dirty_chunks_mutex);
		_dirty_chunks.clear();
	}

	{
		std::lock_guard lock(_edited_blocks_mutex);
		_edited_chunks.clear();
	}
}

void ChunkRepository::set_block(const Vector3i &world_block_pos, voxel::Block block) {
	const Vector3i chunk_pos = voxel::block_to_chunk_coords(world_block_pos);

	std::shared_ptr<ChunkModel> chunk;
	{
		std::lock_guard lock(_mutex);
		if (_chunks.has(chunk_pos)) {
			chunk = _chunks[chunk_pos];
		}else {
			return;
		}
	}

	const Vector3i local_pos = voxel::block_to_chunk_local_block(world_block_pos);

	chunk->set_block(local_pos.x, local_pos.y, local_pos.z, block);

	{
		std::lock_guard lock(_edited_blocks_mutex);
		_edited_chunks[chunk_pos][local_pos] = block;
	}

	_update_dirty_chunks(local_pos, chunk_pos);

	{
		std::lock_guard lock(_chunk_versions_mutex);
		_chunk_versions[chunk_pos]++;
	}
}

HashSet<Vector3i> ChunkRepository::consume_dirty_chunks() {
	std::lock_guard lock(_dirty_chunks_mutex);
	HashSet<Vector3i> dirty =_dirty_chunks;

	_dirty_chunks.clear();

	return dirty;
}
} //namespace godot
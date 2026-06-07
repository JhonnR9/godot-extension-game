
#include "chunk_pool.h"
#include "chunk_node.h"
#include "godot_cpp/classes/node.hpp"

namespace godot {

void ChunkPool::set_owner(Node *owner) {
	this->_owner_node = owner;
}

void ChunkPool::_bind_methods() {
}
ChunkNode *ChunkPool::acquire() {
	if (!_owner_node) return nullptr;

	if (_pool.empty()) {
		WARN_PRINT("ChunkPool empty, allocating an extra chunk node.");
		ChunkNode *chunk = memnew(ChunkNode);
		_owner_node->add_child(chunk);
		chunk->enable();
		return chunk;
	}
	ChunkNode *chunk = _pool.back();
	_pool.pop_back();
	chunk->enable();
	return chunk;
}

void ChunkPool::release(ChunkNode *chunk) {
	if (!chunk) return;

	chunk->disable();
	_pool.push_back(chunk);
}
void ChunkPool::set_prewarm(const int count) {
	if (!_owner_node) return;
	for (int i = 0; i < count; i++) {
		auto *chunk = memnew(ChunkNode);
		_owner_node->add_child(chunk);
		chunk->disable();
		_pool.push_back(chunk);
	}
}

void ChunkPool::clear() {
	for (ChunkNode *chunk : _pool) {
		if (chunk) {
			if (chunk->get_parent()) {
				chunk->get_parent()->remove_child(chunk);
			}
			chunk->queue_free();
		}
	}

	_pool.clear();
}

} //namespace godot
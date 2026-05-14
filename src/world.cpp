#include "world.h"
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/classes/object.hpp>

namespace godot {

void World::_ready() {
	_last_player_chunk_pos = Vector3i(INT_MAX, INT_MAX, INT_MAX);
	set_process(true);
}


void World::_process(double delta) {
	if (!_player_node) {
		_player_node = get_node<Node3D>("../Player");
		if (!_player_node) return;

		_last_player_chunk_pos = _world_to_chunk_pos(_player_node->get_global_position());
		_update_chunks();
		return;
	}

	Vector3i current_chunk_p = _world_to_chunk_pos(_player_node->get_global_position());

	if (current_chunk_p != _last_player_chunk_pos) {
		_last_player_chunk_pos = current_chunk_p;
		_update_chunks();
	}
}

Vector3i World::_world_to_chunk_pos(Vector3 p_pos) {
    return Vector3i(
        Math::floor(p_pos.x / ChunkModel::SIZE),
        Math::floor(p_pos.y / ChunkModel::SIZE),
        Math::floor(p_pos.z / ChunkModel::SIZE)
    );
}

void World::_update_chunks() {
    Dictionary chunks_to_keep;
    for (int x = -_world_radius; x <= _world_radius; x++) {
        for (int y = -_world_height; y <= _world_height; y++) {
            for (int z = -_world_radius; z <= _world_radius; z++) {
                Vector3i pos = _last_player_chunk_pos + Vector3i(x, y, z);
                chunks_to_keep[pos] = true;
            }
        }
    }

	Array to_remove;
	for (const KeyValue<Vector3i, ChunkNode*> &E : _chunks) {
		if (!chunks_to_keep.has(E.key)) {
			to_remove.push_back(E.key);
		}
	}

	for (int i = 0; i < to_remove.size(); i++) {
		Vector3i pos = to_remove[i];

		if (ChunkNode *value = _chunks[pos]) {
			value->set_visible(false);
			value->set_process(false);
			_chunk_pool.push_back(value);
		}
		_chunks.erase(pos);
	}

    Array needed_keys = chunks_to_keep.keys();
    for (int i = 0; i < needed_keys.size(); i++) {
        Vector3i pos = needed_keys[i];

        if (_chunks.has(pos)) continue;

        ChunkNode* chunk = nullptr;

        if (!_chunk_pool.is_empty()) {
            int last_idx = _chunk_pool.size() - 1;
            chunk = Object::cast_to<ChunkNode>(_chunk_pool[last_idx]);
            _chunk_pool.remove_at(last_idx);
        } else {
            chunk = memnew(ChunkNode);
            add_child(chunk);
        }

        if (chunk) {
            _chunks[pos] = chunk;
            chunk->set_visible(true);
            chunk->set_process(true);
            chunk->set_global_position(Vector3(pos.x * ChunkModel::SIZE, pos.y * ChunkModel::SIZE, pos.z * ChunkModel::SIZE));


        }
    }
}

void World::_bind_methods() {

}

} // namespace godot
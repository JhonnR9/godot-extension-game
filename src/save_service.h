#pragma once

#include "godot_cpp/classes/object.hpp"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/hash_set.hpp"

#include "utils.h"

namespace godot {

struct WorldCache {
	HashSet<int64_t> worlds;
	HashMap<int64_t, WorldModel> models;

	bool dirty = true;

	void clear() {
		worlds.clear();
		models.clear();
		dirty = true;
	}
};

class SaveService : public Object {
	GDCLASS(SaveService, Object)

	static SaveService *singleton;

	WorldCache world_cache;

	void init_cache();
	static WorldModel load_world_model_from_disk(int64_t p_id);

protected:
	static void _bind_methods();

public:
	static SaveService *get_singleton();

	SaveService();
	~SaveService() override;

	HashSet<int64_t> get_saved_worlds();
	WorldModel load_world_model(int64_t p_id);
	void delete_world(int64_t p_id);
	static String get_world_dir(int64_t p_id) ;
	int64_t create_world(int32_t p_seed, const String &p_name);
	PackedInt64Array get_saved_worlds_array();
	Dictionary load_world_model_dict(int64_t p_id);

	void invalidate_world(int64_t p_id);
	void invalidate_world_cache();
};

} // namespace godot

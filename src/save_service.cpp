#include "save_service.h"

#include "godot_cpp/classes/dir_access.hpp"
#include "godot_cpp/classes/file_access.hpp"
#include "godot_cpp/classes/json.hpp"
#include "godot_cpp/classes/resource_uid.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/variant/packed_int64_array.hpp"

using namespace godot;

static constexpr int WORLD_MODEL_VERSION = 1;

SaveService *SaveService::singleton = nullptr;

SaveService::SaveService() {
	ERR_FAIL_COND_MSG(
		singleton != nullptr,
		"SaveService singleton cannot be two instance!"
	);

	singleton = this;
}

PackedInt64Array SaveService::get_saved_worlds_array() {
	PackedInt64Array result;

	const HashSet<int64_t> worlds = get_saved_worlds();

	result.resize(worlds.size());

	int64_t *data = result.ptrw();
	int index = 0;

	for (const int64_t id : worlds) {
		data[index++] = id;
	}

	return result;
}
Dictionary SaveService::load_world_model_dict(int64_t p_id) {
	const auto [seed, name, id] = load_world_model(p_id);

	Dictionary result;

	if (id != p_id) {
		return result;
	}

	result["id"] = id;
	result["seed"] = seed;
	result["name"] = name;

	return result;
}

SaveService::~SaveService() {
	if (singleton == this) {
		singleton = nullptr;
	}
}

SaveService *SaveService::get_singleton() {
	return singleton;
}


HashSet<int64_t> SaveService::get_saved_worlds() {
	if (world_cache.dirty) {
		init_cache();
	}

	return world_cache.worlds;
}

WorldModel SaveService::load_world_model(int64_t p_id) {
	if (world_cache.dirty) {
		init_cache();
	}

	if (const WorldModel *cached = world_cache.models.getptr(p_id)) {
		return *cached;
	}

	const WorldModel model = load_world_model_from_disk(p_id);

	if (model.id == p_id) {
		world_cache.models.insert(p_id, model);
	}

	return model;
}
void SaveService::delete_world(int64_t p_id) {
	if (const String dir_path = get_world_dir(p_id); DirAccess::dir_exists_absolute(dir_path)) {
		if (const Ref<DirAccess> dir = DirAccess::open(dir_path); dir.is_valid()) {
			DirAccess::remove_absolute(dir_path);
		}
	}

	world_cache.clear();
}

WorldModel SaveService::load_world_model_from_disk(const int64_t p_id) {
	WorldModel model{};

	const String path = get_world_dir(p_id) + "/level.json";

	if (!FileAccess::file_exists(path)) {
		return model;
	}

	const Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);

	if (file.is_null()) {
		return model;
	}

	const String text = file->get_as_text();

	file->close();

	const Variant parsed = JSON::parse_string(text);

	if (parsed.get_type() != Variant::DICTIONARY) {
		return model;
	}

	const Dictionary root = parsed;

	if (!root.has("version") ||
		!root.has("id") ||
		!root.has("seed") ||
		!root.has("name")) {
		return model;
	}

	if (root["version"].get_type() != Variant::INT ||
		root["id"].get_type() != Variant::INT ||
		root["seed"].get_type() != Variant::INT ||
		root["name"].get_type() != Variant::STRING) {
		return model;
	}

	const int64_t version = root["version"];
	const int64_t id = root["id"];
	const int64_t seed = root["seed"];
	const String name = root["name"];

	if (version != WORLD_MODEL_VERSION) {
		return model;
	}

	if (id < 0) {
		return model;
	}

	if (seed < 0 || seed > UINT32_MAX) {
		return model;
	}

	model.id = static_cast<int64_t>(id);
	model.seed = static_cast<int32_t>(seed);
	model.name = name;

	return model;
}

String SaveService::get_world_dir(const int64_t p_id) {
	return "user://voxelcraft/worlds/" + String::num_int64(p_id);
}

int64_t SaveService::create_world(const int32_t p_seed, const String &p_name) {
	const WorldModel world_model{
		.seed = p_seed,
		.name = p_name,
		.id = ResourceUID::get_singleton()->create_id()
	};

	const String path = get_world_dir(world_model.id) + "/level.json";

	Dictionary root;
	root["version"] = WORLD_MODEL_VERSION;
	root["id"]		= world_model.id;
	root["seed"]	= world_model.seed;
	root["name"]	= world_model.name;

	const String text = JSON::stringify(root, "\t");

	const Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
	if (file.is_null())
		return 0;

	file->store_string(text);
	file->flush();
	file->close();

	world_cache.clear();

	return world_model.id;
}

void SaveService::init_cache() {
	world_cache.worlds.clear();
	world_cache.models.clear();

	const Ref<DirAccess> dir =
		DirAccess::open("user://voxelcraft/worlds");

	if (dir.is_valid()) {
		dir->list_dir_begin();

		String file_name = dir->get_next();

		while (!file_name.is_empty()) {
			if (dir->current_is_dir() && file_name != "." && file_name != "..") {
				if (const int64_t world_id = file_name.to_int(); world_id >= 0) {
					world_cache.worlds.insert(world_id);
				}
			}

			file_name = dir->get_next();
		}

		dir->list_dir_end();
	}

	world_cache.dirty = false;
}

void SaveService::invalidate_world(int64_t p_id) {
	world_cache.models.erase(p_id);
}

void SaveService::invalidate_world_cache() {
	world_cache.clear();
}

void SaveService::_bind_methods() {
	ClassDB::bind_method(
		D_METHOD("get_saved_worlds"),
		&SaveService::get_saved_worlds_array
	);

	ClassDB::bind_method(
		D_METHOD("load_world_model", "id"),
		&SaveService::load_world_model_dict
	);

	ClassDB::bind_method(
		D_METHOD("delete_world", "id"),
		&SaveService::delete_world
	);

	ClassDB::bind_static_method(
		SaveService::get_class_static(),
		D_METHOD("get_world_dir", "id"),
		&SaveService::get_world_dir
	);

	ClassDB::bind_method(
		D_METHOD("create_world", "seed", "name"),
		&SaveService::create_world
	);

	ClassDB::bind_method(
		D_METHOD("invalidate_world", "id"),
		&SaveService::invalidate_world
	);

	ClassDB::bind_method(
		D_METHOD("invalidate_world_cache"),
		&SaveService::invalidate_world_cache
	);
}



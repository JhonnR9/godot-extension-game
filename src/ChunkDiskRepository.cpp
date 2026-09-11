#include "ChunkDiskRepository.h"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/worker_thread_pool.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/array.hpp>

namespace godot {

static constexpr int WORLD_MODEL_VERSION = 1;
static constexpr int REGION_FILE_VERSION = 1;

void ChunkDiskRepository::_bind_methods() {}

void ChunkDiskRepository::set_current_world(uint64_t p_id) {
    current_world_id = p_id;
    String dir = get_world_dir(p_id) + "/regions";
    if (!DirAccess::dir_exists_absolute(dir)) {
        DirAccess::make_dir_recursive_absolute(dir);
    }
}

String ChunkDiskRepository::get_world_dir(uint64_t p_id) const {
    return "user://voxelcraft/worlds/" + String::num_uint64(p_id);
}

String ChunkDiskRepository::get_region_path(Vector3i region_pos) const {
    return get_world_dir(current_world_id) + "/regions/region_" + itos(region_pos.x) + "_" + itos(region_pos.y) + "_" + itos(region_pos.z) + ".json";
}


void ChunkDiskRepository::save_world_model(const WorldModel &model) {
    String path = get_world_dir(model.id) + "/level.json";

    Dictionary root;
    root["version"] = WORLD_MODEL_VERSION;
    root["id"] = (int64_t)model.id;
    root["seed"] = (int64_t)model.seed;
    root["name"] = model.name;

    String text = JSON::stringify(root, "\t");

    Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
    if (file.is_null()) return;

    file->store_string(text);
    file->flush();
    file->close();
}

WorldModel ChunkDiskRepository::load_world_model(const uint64_t p_id) const {
    WorldModel model{};
	const String path = get_world_dir(p_id) + "/level.json";

    if (!FileAccess::file_exists(path)) return model;

	const Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
    if (file.is_null()) return model;

	const String text = file->get_as_text();
    file->close();

	const Variant parsed = JSON::parse_string(text);
    if (parsed.get_type() != Variant::DICTIONARY) return model;

    Dictionary root = parsed;
    if (!root.has("version") || !root.has("id") || !root.has("seed") || !root.has("name")) return model;
    if (static_cast<int>(root["version"]) != WORLD_MODEL_VERSION) return model;

    model.id = (uint64_t)(int64_t)root["id"];
    model.seed = (uint32_t)(int64_t)root["seed"];
    model.name = (String)root["name"];

    return model;
}

HashSet<int64_t> ChunkDiskRepository::get_saved_worlds() const {
    HashSet<int64_t> worlds;
    Ref<DirAccess> dir = DirAccess::open("user://voxelcraft/worlds");
    if (dir.is_valid()) {
        dir->list_dir_begin();
        String file_name = dir->get_next();
        while (!file_name.is_empty()) {
            if (dir->current_is_dir() && file_name != "." && file_name != "..") {
                worlds.insert(file_name.to_int());
            }
            file_name = dir->get_next();
        }
    }
    return worlds;
}

void ChunkDiskRepository::delete_world(uint64_t p_id) {
    String dir_path = get_world_dir(p_id);
    if (DirAccess::dir_exists_absolute(dir_path)) {
        Ref<DirAccess> dir = DirAccess::open(dir_path);
        if (dir.is_valid()) {
            DirAccess::remove_absolute(dir_path);
        }
    }
}

void ChunkDiskRepository::save_region(Vector3i region_pos, const voxel::Region &region) {
    String path = get_region_path(region_pos);

    Dictionary root;
    root["version"] = REGION_FILE_VERSION;

    Dictionary pos_d;
    pos_d["x"] = region_pos.x;
    pos_d["y"] = region_pos.y;
    pos_d["z"] = region_pos.z;
    root["pos"] = pos_d;

    Array chunks_arr;
    for (const auto &E : region.edited_chunks) {
        const Vector3i &chunk_pos = E.key;
        const voxel::ChunkDelta &chunk_delta = E.value;

        Dictionary chunk_d;
        Dictionary c_pos_d;
        c_pos_d["x"] = chunk_pos.x;
        c_pos_d["y"] = chunk_pos.y;
        c_pos_d["z"] = chunk_pos.z;
        chunk_d["pos"] = c_pos_d;

        Array blocks_arr;
        for (const auto &F : chunk_delta.delta) {
            const Vector3i &local_pos = F.key;
            const voxel::Block &block = F.value;

            Dictionary block_d;
            block_d["x"] = local_pos.x;
            block_d["y"] = local_pos.y;
            block_d["z"] = local_pos.z;
            block_d["v"] = (int)block;
            blocks_arr.push_back(block_d);
        }
        chunk_d["blocks"] = blocks_arr;
        chunks_arr.push_back(chunk_d);
    }
    root["chunks"] = chunks_arr;

    String text = JSON::stringify(root, "\t");

    Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
    if (file.is_null()) return;

    file->store_string(text);
    file->flush();
    file->close();
}

void ChunkDiskRepository::save_region_async(Vector3i region_pos, const voxel::Region &region) {
    struct RegionSaveJob {
        Vector3i pos;
        voxel::Region region;
        ChunkDiskRepository *repo;
    };

    auto *job = new RegionSaveJob{region_pos, region, this};
    WorkerThreadPool::get_singleton()->add_native_task([](void *data) {
        auto *save_job = static_cast<RegionSaveJob *>(data);
        save_job->repo->save_region(save_job->pos, save_job->region);
        delete save_job;
    }, job);
}

voxel::Region ChunkDiskRepository::load_region(Vector3i region_pos) {
    voxel::Region region;
    String path = get_region_path(region_pos);
    if (!FileAccess::file_exists(path)) return region;

    Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
    if (file.is_null()) return region;

    String text = file->get_as_text();
    file->close();

    Variant parsed = JSON::parse_string(text);
    if (parsed.get_type() != Variant::DICTIONARY) return region;

    Dictionary root = parsed;
    if (!root.has("version") || (int)root["version"] != REGION_FILE_VERSION) return region;
    if (!root.has("pos") || !root.has("chunks")) return region;

    Dictionary pos_d = root["pos"];
    if (!pos_d.has("x") || !pos_d.has("y") || !pos_d.has("z")) return region;

    Vector3i saved_pos((int)pos_d["x"], (int)pos_d["y"], (int)pos_d["z"]);
    if (saved_pos != region_pos) return region;

    Array chunks_arr = root["chunks"];
    for (int i = 0; i < chunks_arr.size(); i++) {
        Variant chunk_v = chunks_arr[i];
        if (chunk_v.get_type() != Variant::DICTIONARY) continue;
        Dictionary chunk_d = chunk_v;
        if (!chunk_d.has("pos") || !chunk_d.has("blocks")) continue;

        Dictionary c_pos_d = chunk_d["pos"];
        if (!c_pos_d.has("x") || !c_pos_d.has("y") || !c_pos_d.has("z")) continue;

        Vector3i c_pos((int)c_pos_d["x"], (int)c_pos_d["y"], (int)c_pos_d["z"]);

        voxel::ChunkDelta delta;
        Array blocks_arr = chunk_d["blocks"];
        for (int j = 0; j < blocks_arr.size(); j++) {
            Variant block_v = blocks_arr[j];
            if (block_v.get_type() != Variant::DICTIONARY) continue;
            Dictionary block_d = block_v;
            if (!block_d.has("x") || !block_d.has("y") || !block_d.has("z") || !block_d.has("v")) continue;

            Vector3i local_pos((int)block_d["x"], (int)block_d["y"], (int)block_d["z"]);
            const voxel::Block value = (voxel::Block)(int)block_d["v"];
            delta.delta.insert(local_pos, value);
        }
        region.edited_chunks.insert(c_pos, delta);
    }

    return region;
}

Vector<Vector3i> ChunkDiskRepository::get_all_saved_regions() const {
    Vector<Vector3i> regions;
    String dir_path = get_world_dir(current_world_id) + "/regions";
    Ref<DirAccess> dir = DirAccess::open(dir_path);

    if (dir.is_valid()) {
        dir->list_dir_begin();

        String file_name = dir->get_next();
        while (!file_name.is_empty()) {
            if (file_name.begins_with("region_") && file_name.ends_with(".json")) {

                PackedStringArray parts = file_name
                    .trim_prefix("region_")
                    .trim_suffix(".json")
                    .split("_");

                if (parts.size() == 3) {
                    regions.push_back(
                        Vector3i(
                            parts[0].to_int(),
                            parts[1].to_int(),
                            parts[2].to_int()
                        )
                    );
                }
            }

            file_name = dir->get_next();
        }

        dir->list_dir_end();
    }

    return regions;
}
} //namespace godot

#include "ChunkDiskRepository.h"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/dir_access.hpp>

namespace godot {
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
    return get_world_dir(current_world_id) + "/regions/region_" + itos(region_pos.x) + "_" + itos(region_pos.y) + "_" + itos(region_pos.z) + ".dat";
}

void ChunkDiskRepository::save_world_model(const WorldModel &model) {
    String path = get_world_dir(model.id) + "/level.dat";
    Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
    if (file.is_valid()) {
        file->store_64(model.id);
        file->store_32(model.seed);
        file->store_pascal_string(model.name);
    }
}

WorldModel ChunkDiskRepository::load_world_model(uint64_t p_id) {
    WorldModel model;
    String path = get_world_dir(p_id) + "/level.dat";
    if (FileAccess::file_exists(path)) {
        Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
        model.id = file->get_64();
        model.seed = file->get_32();
        model.name = file->get_pascal_string();
    }
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
    Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
    if (file.is_null()) return;

    file->store_32(region.edited_chunks.size());
    for (const auto &E : region.edited_chunks) {
        file->store_32(E.key.x); file->store_32(E.key.y); file->store_32(E.key.z);
        file->store_32(E.value.delta.size());
        for (const auto &F : E.value.delta) {
            file->store_32(F.key.x); file->store_32(F.key.y); file->store_32(F.key.z);
            file->store_32(F.value);
        }
    }

	file->flush();
	file->close();
}

voxel::Region ChunkDiskRepository::load_region(Vector3i region_pos) {
    voxel::Region region;
    String path = get_region_path(region_pos);
    if (!FileAccess::file_exists(path)) return region;

    Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
    uint32_t chunk_count = file->get_32();
    for (uint32_t i = 0; i < chunk_count; ++i) {
        Vector3i c_pos(file->get_32(), file->get_32(), file->get_32());
        uint32_t block_count = file->get_32();
        voxel::ChunkDelta delta;
        for (uint32_t j = 0; j < block_count; ++j) {
            delta.delta.insert(Vector3i(file->get_32(), file->get_32(), file->get_32()), file->get_32());
        }
        region.edited_chunks.insert(c_pos, delta);
    }
    return region;
}
}
#include "ChunkDiskRepository.h"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/worker_thread_pool.hpp>

namespace godot {
static constexpr uint32_t WORLD_MODEL_MAGIC = 0x574D544C; // 'WMTL'
static constexpr uint32_t REGION_FILE_MAGIC = 0x56474552; // 'VREG'
static constexpr uint32_t REGION_FILE_VERSION = 1;
static constexpr uint32_t WORLD_MODEL_VERSION = 1;

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
    if (file.is_null()) return;

    file->store_32(WORLD_MODEL_MAGIC);
    file->store_32(WORLD_MODEL_VERSION);
    file->store_64(model.id);
    file->store_32(model.seed);
    file->store_pascal_string(model.name);

    file->flush();
    file->close();
}

WorldModel ChunkDiskRepository::load_world_model(uint64_t p_id) {
    WorldModel model;
    String path = get_world_dir(p_id) + "/level.dat";
    if (!FileAccess::file_exists(path)) return model;

    Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
    if (file.is_null() || file->get_length() < 16) return model;

    const uint32_t magic = file->get_32();
    const uint32_t version = file->get_32();
    if (magic != WORLD_MODEL_MAGIC || version != WORLD_MODEL_VERSION) return model;

    model.id = file->get_64();
    model.seed = file->get_32();
    model.name = file->get_pascal_string();

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

    file->store_32(REGION_FILE_MAGIC);
    file->store_32(REGION_FILE_VERSION);
    file->store_32(region_pos.x);
    file->store_32(region_pos.y);
    file->store_32(region_pos.z);
    file->store_32(region.edited_chunks.size());

    for (const auto &E : region.edited_chunks) {
        file->store_32(E.key.x);
        file->store_32(E.key.y);
        file->store_32(E.key.z);
        file->store_32(E.value.delta.size());
        for (const auto &F : E.value.delta) {
            file->store_32(F.key.x);
            file->store_32(F.key.y);
            file->store_32(F.key.z);
            file->store_32(F.value);
        }
    }

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
    if (file.is_null() || file->get_length() < 20) return region;

    const uint32_t magic = file->get_32();
    const uint32_t version = file->get_32();
    if (magic != REGION_FILE_MAGIC || version != REGION_FILE_VERSION) return region;

    const int32_t x = file->get_32();
    const int32_t y = file->get_32();
    const int32_t z = file->get_32();
    if (Vector3i(x, y, z) != region_pos) return region;

    const uint32_t chunk_count = file->get_32();
    if (chunk_count > static_cast<uint32_t>(voxel::CHUNKS_PER_REGION) * voxel::CHUNKS_PER_REGION * voxel::CHUNKS_PER_REGION) return region;

    for (uint32_t i = 0; i < chunk_count; ++i) {
        if (file->eof_reached()) break;
        Vector3i c_pos(file->get_32(), file->get_32(), file->get_32());
        const uint32_t block_count = file->get_32();
        if (block_count > 32768) break;

        voxel::ChunkDelta delta;
        for (uint32_t j = 0; j < block_count; ++j) {
            if (file->eof_reached()) break;
            Vector3i local_pos(file->get_32(), file->get_32(), file->get_32());
            const uint32_t value = file->get_32();
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
			if (file_name.begins_with("region_") && file_name.ends_with(".dat")) {

				PackedStringArray parts = file_name
					.trim_prefix("region_")
					.trim_suffix(".dat")
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
}
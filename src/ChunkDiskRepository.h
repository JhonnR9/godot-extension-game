#ifndef CHUNKDISKREPOSITORY_H
#define CHUNKDISKREPOSITORY_H

#include "utils.h"
#include "voxel.h"
#include "godot_cpp/templates/hash_set.hpp"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <mutex>

namespace godot {
class ChunkDiskRepository : public RefCounted {
	GDCLASS(ChunkDiskRepository, RefCounted)
protected:
	static void _bind_methods();
public:
	void set_current_world(uint64_t p_id);
	uint64_t get_current_world_id() const { return current_world_id; }

	void save_world_model(const WorldModel &model);
	WorldModel load_world_model(uint64_t p_id);
	HashSet<int64_t> get_saved_worlds() const;
	void delete_world(uint64_t p_id);

	void save_region(Vector3i region_pos, const voxel::Region &region);
	voxel::Region load_region(Vector3i region_pos);

private:
	uint64_t current_world_id = 0;
	String get_world_dir(uint64_t p_id) const;
	String get_region_path(Vector3i region_pos) const;

	std::mutex regions_mutex;
	HashMap<Vector3i, voxel::Region> regions;
};
}
#endif // CHUNKDISKREPOSITORY_H
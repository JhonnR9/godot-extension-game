#ifndef CHUNKDISKREPOSITORY_H
#define CHUNKDISKREPOSITORY_H
#include "voxel.h"
#include "godot_cpp/templates/hash_map.hpp"

#include <godot_cpp/classes/ref_counted.hpp>

namespace godot {
class ChunkDiskRepository : public RefCounted {
	GDCLASS(ChunkDiskRepository, RefCounted)

protected:
	static void _bind_methods();
public:
	void save_test();

private:

};
} // godot

#endif //CHUNKDISKREPOSITORY_H
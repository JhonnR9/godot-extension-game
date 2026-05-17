

#ifndef ATLAS_LOADER_H
#define ATLAS_LOADER_H

#include "voxel_mesher.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <string>
#include <unordered_map>

namespace godot {
void load_atlas(const String &path);
}

#endif //ATLAS_LOADER_H

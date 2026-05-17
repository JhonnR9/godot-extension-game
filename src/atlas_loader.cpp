//
// Created by jhone on 17/05/2026.
//


#include "atlas_loader.h"

namespace godot {

extern std::unordered_map<std::string, AtlasUV> atlas;
void load_atlas(const String &path) {
	{
		Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);

		if (file.is_null()) {
			ERR_PRINT("Falha ao abrir atlas.json");
			return;
		}

		String content = file->get_as_text();

		Variant data = JSON::parse_string(content);
		Dictionary dict = data;

		Array keys = dict.keys();

		for (int i = 0; i < keys.size(); i++) {
			String key = keys[i];

			Dictionary entry = dict[key];

			Array uv_min = entry["uv_min"];
			Array uv_max = entry["uv_max"];

			AtlasUV uv;
			uv.min = Vector2(uv_min[0], uv_min[1]);
			uv.max = Vector2(uv_max[0], uv_max[1]);

			atlas[std::string(key.utf8().get_data())] = uv;
		}

		print_line("Atlas carregado com sucesso!");
	}
}

} // godot


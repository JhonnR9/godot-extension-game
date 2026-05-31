
#include "ChunkDiskRepository.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/project_settings.hpp>

namespace godot {

void ChunkDiskRepository::_bind_methods() {

}

void ChunkDiskRepository::save_test() {
	HashMap<Vector3i, HashMap<Vector3i, voxel::Block>>edited_chunks;

	HashMap<Vector3i, voxel::Block> list_test_1;
	list_test_1.insert(Vector3i(1,2,3), 12);
	list_test_1.insert(Vector3i(1,2,4), 13);
	list_test_1.insert(Vector3i(1,2,5), 14);
	list_test_1.insert(Vector3i(1,2,6), 15);
	list_test_1.insert(Vector3i(1,2,7), 16);

	HashMap<Vector3i, voxel::Block> list_test_2;
	list_test_2.insert(Vector3i(1,2,3), 17);
	list_test_2.insert(Vector3i(1,2,3), 18);
	list_test_2.insert(Vector3i(1,2,3), 19);
	list_test_2.insert(Vector3i(1,2,3), 20);
	list_test_2.insert(Vector3i(1,2,3), 21);

	edited_chunks.insert(Vector3i(1,2,3), list_test_1);
	edited_chunks.insert(Vector3i(1,5,3), list_test_2);

	String dir_path = "user://voxelcraft/746367/";
	if (!DirAccess::dir_exists_absolute(dir_path)) {
		DirAccess::make_dir_recursive_absolute(dir_path);
	}

	String path = "user://voxelcraft/746367/region000.data";
	String absolute_path = ProjectSettings::get_singleton()->globalize_path(path);

	UtilityFunctions::print("Salvando em: ", absolute_path);

	Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);

	if (file.is_null()) {
		ERR_PRINT("Falha ao abrir arquivo para escrita: " + path);
		return;
	}

	file->store_32(edited_chunks.size());

	for (const KeyValue<Vector3i, HashMap<Vector3i, voxel::Block>> &E : edited_chunks) {
		Vector3i chunk_pos = E.key;
		const HashMap<Vector3i, voxel::Block> &blocks = E.value;

		file->store_32(chunk_pos.x);
		file->store_32(chunk_pos.y);
		file->store_32(chunk_pos.z);

		file->store_32(blocks.size());


		for (const KeyValue<Vector3i, voxel::Block> &F : blocks) {
			Vector3i block_pos = F.key;
			voxel::Block block_val = F.value;

			file->store_32(block_pos.x);
			file->store_32(block_pos.y);
			file->store_32(block_pos.z);

			file->store_32(block_val);
		}
	}

	file->close();
}
} // godot
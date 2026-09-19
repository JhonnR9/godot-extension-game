#include "register_types.h"

#include "ChunkDiskRepository.h"
#include "chunk_mesh_async_generator.h"
#include "chunk_model_generator.h"
#include "chunk_node.h"
#include "chunk_repository.h"
#include "chunk_streaming_manager.h"
#include "crosshair.h"
#include "godot_cpp/classes/engine.hpp"
#include "player.h"
#include "save_service.h"
#include "voxel_api.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

static SaveService *save_service_singleton;

void initialize_gdextension_types(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	GDREGISTER_CLASS(ChunkDiskRepository);
	GDREGISTER_CLASS(SaveService);
	save_service_singleton = memnew(SaveService);
	Engine::get_singleton()->register_singleton("SaveService", SaveService::get_singleton());

	GDREGISTER_CLASS(ChunkNode);
	GDREGISTER_CLASS(VoxelAPI);
	GDREGISTER_CLASS(Player);
	GDREGISTER_CLASS(ChunkPool);
	GDREGISTER_CLASS(ChunkRepository);
	GDREGISTER_CLASS(ChunkStreamingManager);
	GDREGISTER_CLASS(ChunkMeshAsyncGenerator);
	GDREGISTER_CLASS(ChunkModelGenerator);
	GDREGISTER_CLASS(Crosshair);
	GDREGISTER_CLASS(ChunkRegionAsyncLoader);
	GDREGISTER_CLASS(TreeDecorator)

}

void uninitialize_gdextension_types(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	Engine::get_singleton()->unregister_singleton("SaveService");
	memdelete(save_service_singleton);
}

extern "C"
{
	// Initialization
	GDExtensionBool GDE_EXPORT gdextension_game_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
	{
		GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
		init_obj.register_initializer(initialize_gdextension_types);
		init_obj.register_terminator(uninitialize_gdextension_types);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

		return init_obj.init();
	}
}
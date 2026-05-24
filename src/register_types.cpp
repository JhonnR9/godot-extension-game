#include "register_types.h"

#include "atlas_loader.h"
#include "chunk_mesh_async_generator.h"
#include "chunk_model_generator.h"
#include "chunk_node.h"
#include "chunk_repository.h"
#include "chunk_streaming_manager.h"
#include "crosshair.h"
#include "player.h"
#include "world.h"
#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_gdextension_types(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	GDREGISTER_RUNTIME_CLASS(ChunkNode);
	GDREGISTER_RUNTIME_CLASS(World);
	GDREGISTER_RUNTIME_CLASS(Player);
	GDREGISTER_RUNTIME_CLASS(ChunkPool);
	GDREGISTER_RUNTIME_CLASS(ChunkRepository);
	GDREGISTER_RUNTIME_CLASS(ChunkStreamingManager);
	GDREGISTER_RUNTIME_CLASS(ChunkMeshAsyncGenerator);
	GDREGISTER_RUNTIME_CLASS(ChunkModelGenerator);
	GDREGISTER_RUNTIME_CLASS(Crosshair);

	load_atlas("res://sprites/atlas.json");
}

void uninitialize_gdextension_types(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
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
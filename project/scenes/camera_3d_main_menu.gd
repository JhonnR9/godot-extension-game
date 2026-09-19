extends Camera3D

@export var voxel: VoxelAPI

func _enter_tree() -> void:
	call_deferred("_start")
	
func _start():
	voxel.set_focus_position(global_position)
	var ids = SaveService.get_saved_worlds()
	voxel.start_world(ids[0])

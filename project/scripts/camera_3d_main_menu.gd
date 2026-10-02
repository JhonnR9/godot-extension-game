extends Camera3D

@export var voxel: VoxelAPI

func _ready() -> void:
	if is_instance_valid(voxel):
		voxel.set_focus_position(global_position)
		voxel.start_preview(42)

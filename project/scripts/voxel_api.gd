extends VoxelAPI

func _enter_tree() -> void:
	call_deferred("_start_world")

func _start_world() -> void:
	start_world(GameSession.selected_world_id)

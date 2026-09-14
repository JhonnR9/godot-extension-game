extends VBoxContainer

@export var world_model_view_scene: PackedScene

func _on_world_selector_save_detected(id: Variant) -> void:
	var model = SaveService.load_world_model(id)

	var view = world_model_view_scene.instantiate()
	var model_view := view as WorldModelView

	model_view.world_name.text = model["name"]
	model_view.id = model["id"]
	add_child(model_view)

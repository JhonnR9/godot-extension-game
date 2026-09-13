extends VBoxContainer

@export var world_model_view_scene: PackedScene

func _on_world_selector_save_detected(id: Variant) -> void:
	var model = SaveService.load_world_model(id)

	var view = world_model_view_scene.instantiate()
	var model_view := view as WorldModelView

	model_view.world_name.text = model["name"]
	model_view.id = model["id"]
	add_child(model_view)


func _on_load_pressed() -> void:
	if GameSession.selected_world_id == 0:
		print("Nenhum mundo selecionado")
		print(GameSession.selected_world_id)
		return

	print("Carregando mundo:", GameSession.selected_world_id)

	get_tree().change_scene_to_file("res://scenes/world.tscn")

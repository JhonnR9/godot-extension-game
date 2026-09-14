extends Control

signal no_save_detected
signal save_detected(id)

@onready var placeholder: Node = $VBoxContainer/WorldsList/VBoxContainer/Placeholder

func _ready() -> void:
	refresh_world_list()

func refresh_world_list() -> void:
	GameSession.selected_world_id = 0

	var ids = SaveService.get_saved_worlds()

	if ids.is_empty():
		emit_signal("no_save_detected")
		return
		
	for node in placeholder.get_parent().get_children():
		if node != placeholder:
			node.queue_free()

	for id in ids:
		var world = SaveService.load_world_model(id)

		if world.is_empty():
			continue

		emit_signal("save_detected", id)


func _on_back_button_pressed() -> void:
	get_tree().change_scene_to_file("res://scenes/main_menu.tscn")

func _on_create_pressed() -> void:
	get_tree().change_scene_to_file("res://scenes/create_world.tscn")

func _on_delete_pressed() -> void:
	if GameSession.selected_world_id == 0:
		return

	SaveService.delete_world(GameSession.selected_world_id)

	refresh_world_list()


func _on_load_pressed() -> void:
	if GameSession.selected_world_id == 0:
		return

	get_tree().change_scene_to_file("res://scenes/VoxelAPI.tscn")

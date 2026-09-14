extends Control

var editing_name: String =""
var editing_seed: int=0

func _ready() -> void:
	editing_seed = randi()
	
	
func _on_create_pressed() -> void:
	if editing_name.is_empty():
		editing_name = "my-world"
	
	var world_id = SaveService.create_world(editing_seed, editing_name)
	GameSession.selected_world_id = world_id
	get_tree().change_scene_to_file("res://scenes/VoxelAPI.tscn")
	


func _on_back_pressed() -> void:
	get_tree().change_scene_to_file("res://scenes/world_selector.tscn")


func _on_name_line_edit_text_submitted(new_text: String) -> void:
	editing_name = new_text


func _on_seed_line_edit_text_submitted(new_text: String) -> void:
	editing_seed = new_text.to_int()


func _on_name_line_edit_text_changed(new_text: String) -> void:
	editing_name=new_text

extends Control

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE


func _on_back_to_main_menu_pressed() -> void:
	get_tree().change_scene_to_file("res://scenes/main_menu.tscn")

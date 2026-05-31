extends Control

func _ready():
	$BoxContainer/VBoxContainer/SinglePlayer.pressed.connect(_on_singleplayer_pressed)
	$BoxContainer/VBoxContainer/Quit.pressed.connect(_on_quit_pressed)

func _on_singleplayer_pressed():
	get_tree().change_scene_to_file("res://scenes/world_selector.tscn")

func _on_quit_pressed():
	get_tree().quit()

extends Control

func _ready():
	$HUD/Buttons/SinglePlayer.pressed.connect(_on_singleplayer_pressed)
	$HUD/Buttons/Quit.pressed.connect(_on_quit_pressed)
	$HUD/Buttons/Settings.pressed.connect(_on_settings_pressed)

func _on_singleplayer_pressed():
	get_tree().change_scene_to_file("res://scenes/world_selector.tscn")

func _on_quit_pressed():
	get_tree().quit()

func _on_settings_pressed() -> void:
	$SettingsPanel.open_from($HUD, null, $HUD/Buttons/Settings)

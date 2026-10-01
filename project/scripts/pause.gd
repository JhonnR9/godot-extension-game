extends Control

func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	
	visible = false

	mouse_filter = Control.MOUSE_FILTER_STOP
	$"../SettingsPanel".configure(get_node("../../VoxelAPI"))
	$"../SettingsPanel".close_requested.connect(_on_settings_closed)


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("ui_cancel"):
		toggle_pause()
		get_viewport().set_input_as_handled()


func toggle_pause() -> void:
	var pausado := get_tree().paused
	
	if pausado:
		get_tree().paused = false
		visible = false
		
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
	else:
		get_tree().paused = true
		visible = true
		
		Input.mouse_mode = Input.MOUSE_MODE_VISIBLE


func _on_back_to_main_menu_pressed() -> void:
	get_tree().paused = false
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	get_tree().change_scene_to_file("res://scenes/main_menu.tscn")


func _on_button_pressed() -> void:
	toggle_pause()

func _on_settings_pressed() -> void:
	visible = false
	$"../SettingsPanel".show()

func _on_settings_closed() -> void:
	$"../SettingsPanel".hide()
	visible = true

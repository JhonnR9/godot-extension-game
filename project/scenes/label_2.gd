extends Label

func _ready() -> void:
	update_text()

func _process(_delta: float) -> void:
	if Input.is_action_just_pressed("toggle_vsync"):
		toggle_vsync()
	

	update_text()

func toggle_vsync() -> void:
	var modo_atual = DisplayServer.window_get_vsync_mode()
	

	if modo_atual == DisplayServer.VSYNC_DISABLED:
		DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_ENABLED)
	else:
		DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)

func update_text() -> void:
	var fps = Engine.get_frames_per_second()
	var vsync_status = "on" if DisplayServer.window_get_vsync_mode() != DisplayServer.VSYNC_DISABLED else "off"
	
	text = "VSync (F2): %s" % [vsync_status]

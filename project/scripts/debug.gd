extends Label

func _ready() -> void:
	update_text()

func _process(_delta: float) -> void:
	if Input.is_action_just_pressed("toggle_vsync"):
		toggle_vsync()
	

	update_text()

func toggle_vsync() -> void:
	var settings: Dictionary = VoxelAPI.get_default_render_settings()
	settings["vsync"] = DisplayServer.window_get_vsync_mode() == DisplayServer.VSYNC_DISABLED
	VoxelAPI.set_default_render_settings(settings)

func update_text() -> void:
	var vsync_status = "on" if DisplayServer.window_get_vsync_mode() != DisplayServer.VSYNC_DISABLED else "off"
	
	text = "VSync (F2): %s" % [vsync_status]

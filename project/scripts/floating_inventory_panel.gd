extends PanelContainer

signal layout_changed
var interaction_enabled := false
var dragging := false
var resizing := false
var start_mouse := Vector2.ZERO
var start_position := Vector2.ZERO
var start_size := Vector2.ZERO
var resize_handle: Label

func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_TOP_LEFT)
	custom_minimum_size = Vector2(160, 180)
	$Margin/Content/TitleBar.gui_input.connect(_title_input)
	$Margin/Content/TitleBar/Close.pressed.connect(hide)
	resize_handle = Label.new()
	resize_handle.text = "◢"
	resize_handle.mouse_filter = Control.MOUSE_FILTER_STOP
	resize_handle.mouse_default_cursor_shape = Control.CURSOR_FDIAGSIZE
	resize_handle.set_as_top_level(true)
	add_child(resize_handle)
	resize_handle.gui_input.connect(_resize_input)
	resized.connect(_position_handle)
	item_rect_changed.connect(_position_handle)
	_position_handle()

func _position_handle() -> void:
	if is_instance_valid(resize_handle):
		resize_handle.global_position = global_position + size - Vector2(20, 20)
		resize_handle.size = Vector2(20, 20)

func _title_input(event: InputEvent) -> void:
	if not interaction_enabled: return
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT:
		dragging = event.pressed
		if dragging: _begin_gesture()
		else: layout_changed.emit()
		accept_event()

func _resize_input(event: InputEvent) -> void:
	if not interaction_enabled: return
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT:
		resizing = event.pressed
		if resizing: _begin_gesture()
		else: layout_changed.emit()
		accept_event()

func _begin_gesture() -> void:
	move_to_front()
	start_mouse = Vector2.ZERO
	start_position = position
	start_size = size

func _input(event: InputEvent) -> void:
	if not interaction_enabled or not visible: return
	if event is InputEventMouseMotion and (dragging or resizing):
		start_mouse += event.relative
		var delta := start_mouse
		if dragging: position = start_position + delta
		if resizing: size = (start_size + delta).max(custom_minimum_size)
		clamp_to_screen()
		get_viewport().set_input_as_handled()
	elif event is InputEventMouseButton and not event.pressed and (dragging or resizing):
		finish_gesture()

func finish_gesture() -> void:
	if dragging or resizing: layout_changed.emit()
	dragging = false
	resizing = false

func clamp_to_screen() -> void:
	var available: Vector2 = get_parent().size
	size = size.min(available)
	position = position.clamp(Vector2.ZERO, (available - size).max(Vector2.ZERO))
	_position_handle()

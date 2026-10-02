extends CanvasLayer

@onready var world: VoxelAPI = get_node("../VoxelAPI")

var progress_bar: ProgressBar
var status_label: Label
var count_label: Label
var background: ColorRect
var loading_started := false
var closing := false

func _ready() -> void:
	layer = 20
	_build_screen()
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE

func _process(_delta: float) -> void:
	if world.is_initial_loading():
		loading_started = true
		visible = true
		var ready_chunks := world.get_initial_loading_ready_chunks()
		var total_chunks := world.get_initial_loading_total_chunks()
		progress_bar.value = world.get_initial_loading_progress() * 100.0
		count_label.text = "%d / %d chunks" % [ready_chunks, total_chunks]
		status_label.text = "Laying the world..."
	elif loading_started and not closing:
		closing = true
		progress_bar.value = 100.0
		status_label.text = "Ready"
		var tween := create_tween()
		tween.tween_interval(0.25)
		tween.tween_property(background, "modulate:a", 0.0, 0.25)
		tween.tween_callback(_finish_loading)

func _finish_loading() -> void:
	visible = false
	Input.mouse_mode = Input.MOUSE_MODE_CAPTURED

func _build_screen() -> void:
	background = ColorRect.new()
	background.color = Color("101923")
	background.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	background.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(background)

	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	background.add_child(center)

	var content := VBoxContainer.new()
	content.custom_minimum_size = Vector2(420, 0)
	content.add_theme_constant_override("separation", 16)
	center.add_child(content)

	var title := Label.new()
	title.text = "Generating a World"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 28)
	content.add_child(title)

	status_label = Label.new()
	status_label.text = "Laying the world..."
	status_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	content.add_child(status_label)

	progress_bar = ProgressBar.new()
	progress_bar.custom_minimum_size = Vector2(420, 22)
	progress_bar.min_value = 0
	progress_bar.max_value = 100
	progress_bar.show_percentage = false
	content.add_child(progress_bar)

	count_label = Label.new()
	count_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	content.add_child(count_label)

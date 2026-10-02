extends Control

const SLOT_SIZE := 56
const SLOT_GAP := 4
const CREATIVE_COLUMNS := 8
const BLOCK_REGISTRY_PATH := "res://data/block_registry.generated.json"
var blocks: Array[Dictionary] = []

@onready var creative_panel: PanelContainer = $CreativePanel
@onready var creative_scroll: ScrollContainer = $CreativePanel/Margin/Content/CreativeScroll
@onready var creative_grid = $CreativePanel/Margin/Content/CreativeScroll/Center/CreativeGrid
@onready var hotbar_panel: PanelContainer = $HotbarPanel
@onready var hotbar_grid = $HotbarPanel/Margin/HotbarGrid

func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	creative_panel.add_theme_stylebox_override("panel", _make_panel_style())
	hotbar_panel.add_theme_stylebox_override("panel", _make_panel_style())
	creative_panel.hide()
	_load_blocks()

	var creative_rows := maxi(2, ceili(float(blocks.size()) / CREATIVE_COLUMNS))
	_configure_grid(creative_grid, creative_rows, CREATIVE_COLUMNS, Vector2i(50, 50), true)
	creative_grid.set_show_item_count(false)
	creative_grid.set_interaction_enabled(false)
	for index in range(blocks.size()):
		creative_grid.set_item_at(_make_block_item(blocks[index]), Vector2i(index % CREATIVE_COLUMNS, floori(float(index) / CREATIVE_COLUMNS)))

	_configure_grid(hotbar_grid, 1, 9, Vector2i(SLOT_SIZE, SLOT_SIZE), false)
	hotbar_grid.set_show_item_count(false)
	hotbar_grid.set_interaction_enabled(false)
	resized.connect(_update_layout)
	_update_layout()

func _resize_slots(grid: Control, side: int) -> void:
	if grid.get_slot_size() == Vector2i(side, side):
		return
	var items: Array = []
	for row in range(grid.get_rows()):
		for column in range(grid.get_columns()):
			items.append(grid.get_item_at(Vector2i(column, row)))
	grid.set_slot_size(Vector2i(side, side))
	for index in range(items.size()):
		if items[index] != null:
			grid.set_item_at(items[index], Vector2i(index % grid.get_columns(), index / grid.get_columns()))

func _update_layout() -> void:
	var available_width := maxf(240.0, size.x - 32.0)
	var hotbar_side := clampi(floori((available_width - 28.0 - 8 * SLOT_GAP) / 9.0), 20, SLOT_SIZE)
	var creative_side := clampi(floori((available_width - 48.0 - 7 * SLOT_GAP) / CREATIVE_COLUMNS), 20, 50)
	_resize_slots(hotbar_grid, hotbar_side)
	_resize_slots(creative_grid, creative_side)
	var hotbar_size: Vector2 = hotbar_grid.get_combined_minimum_size() + Vector2(20, 18)
	var panel_width := maxf(hotbar_size.x, creative_grid.get_combined_minimum_size().x + 48.0)
	var panel_height := minf(260.0, maxf(140.0, size.y - hotbar_size.y * 2.0 - 64.0))
	hotbar_panel.offset_left = -hotbar_size.x / 2.0
	hotbar_panel.offset_right = hotbar_size.x / 2.0
	hotbar_panel.offset_top = -hotbar_size.y - 16.0
	hotbar_panel.offset_bottom = -16.0
	creative_panel.offset_left = -panel_width / 2.0
	creative_panel.offset_right = panel_width / 2.0
	creative_panel.offset_top = -panel_height / 2.0
	creative_panel.offset_bottom = panel_height / 2.0

func _load_blocks() -> void:
	var file := FileAccess.open(BLOCK_REGISTRY_PATH, FileAccess.READ)
	if file == null:
		push_error("Generated block registry is missing. Run Block Registry > Save and Generate.")
		return
	var data: Variant = JSON.parse_string(file.get_as_text())
	if not (data is Dictionary) or not (data.get("blocks", []) is Array):
		push_error("Generated block registry metadata is invalid.")
		return
	for block: Dictionary in data.blocks:
		if int(block.get("id", 0)) == 0:
			continue
		blocks.append({
			"id": int(block.id),
			"name": str(block.get("display_name", block.name)),
			"category": str(block.get("category", "misc")),
			"flags": block.get("flags", []),
		})

func _configure_grid(grid: Control, rows: int, columns: int, slot_size: Vector2i, creative_source: bool) -> void:
	grid.set_rows(rows)
	grid.set_columns(columns)
	grid.set_slot_size(slot_size)
	grid.set_slot_margin(Vector2i(SLOT_GAP, SLOT_GAP))
	grid.set_grid_padding(Vector2i(4, 4))
	grid.set_creative_source(creative_source)
	grid.set_item_frame(_make_slot_style(false))
	grid.set_item_frame_hover(_make_slot_style(false, true))
	grid.set_item_frame_selected(_make_slot_style(true))

func _make_block_item(block: Dictionary):
	var item = ClassDB.instantiate("ItemView")
	item.set_id(int(block.id))
	item.set_name(str(block.name))
	item.set_category(str(block.category))
	item.set_item_amount(1)
	item.set_icon(BlockIconCache.get_icon(int(block.id)))
	return item

func _block_by_id(block_id: int) -> Dictionary:
	for block in blocks:
		if int(block.id) == block_id:
			return block
	return {}

func _make_panel_style() -> StyleBoxFlat:
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.055, 0.065, 0.085, 0.95)
	style.border_color = Color(0.68, 0.72, 0.8, 0.95)
	style.set_border_width_all(2)
	style.set_corner_radius_all(5)
	return style

func _make_slot_style(selected: bool, hovered: bool = false) -> StyleBoxFlat:
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.13, 0.15, 0.19, 0.98) if not selected else Color(0.24, 0.22, 0.15, 0.99)
	style.border_color = Color(0.78, 0.8, 0.84, 0.95) if not selected else Color(1.0, 0.82, 0.32, 1.0)
	if hovered:
		style.border_color = Color(1.0, 0.92, 0.62, 1.0)
	style.set_border_width_all(2 if selected else 1)
	style.set_corner_radius_all(3)
	return style

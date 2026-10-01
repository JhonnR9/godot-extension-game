extends Control

const SLOT_SIZE := 56
const SLOT_GAP := 4
const CREATIVE_COLUMNS := 8
const BLOCK_REGISTRY_PATH := "res://data/block_registry.generated.json"
var blocks: Array[Dictionary] = []

@onready var creative_panel: PanelContainer = $CreativePanel
@onready var creative_grid = $CreativePanel/Margin/Content/CreativeGrid
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
	for index in range(mini(9, blocks.size())):
		hotbar_grid.set_item_at(_make_block_item(blocks[index]), Vector2i(index, 0))
	for index in range(9):
		var number := Label.new()
		number.text = str(index + 1)
		number.position = Vector2(8 + index * (SLOT_SIZE + SLOT_GAP), 3)
		number.mouse_filter = Control.MOUSE_FILTER_IGNORE
		hotbar_grid.add_child(number)

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
		var textures: Dictionary = block.get("textures", {})
		var texture_name := str(textures.get("side", textures.get("top", textures.get("bottom", "stone_side"))))
		if texture_name.is_empty():
			texture_name = "stone_side"
		blocks.append({
			"id": int(block.id),
			"name": str(block.get("display_name", block.name)),
			"category": str(block.get("category", "misc")),
			"flags": block.get("flags", []),
			"icon": "res://textures/blocks/%s.png" % texture_name,
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
	item.set_hint_description("%s | %s" % [block.category, ", ".join(PackedStringArray(block.flags))])
	item.set_item_amount(1)
	item.set_icon(load(str(block.icon)))
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

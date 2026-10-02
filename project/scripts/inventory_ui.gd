extends Control

const SLOT_SIZE := 56
const SLOT_GAP := 4
const CREATIVE_COLUMNS := 8
const BLOCK_REGISTRY_PATH := "res://data/block_registry.generated.json"
var blocks: Array[Dictionary] = []

@onready var creative_panel: PanelContainer = $CreativePanel
@onready var creative_scroll: ScrollContainer = $CreativePanel/Margin/Content/CreativeScroll
@onready var creative_grid = $CreativePanel/Margin/Content/CreativeScroll/Center/CreativeGrid
@onready var inventory_scroll: ScrollContainer = $CreativePanel/Margin/Content/InventoryScroll
@onready var inventory_grid = $CreativePanel/Margin/Content/InventoryScroll/Center/InventoryGrid
@onready var creative_tab: Button = $CreativePanel/Margin/Content/Tabs/CreativeTab
@onready var inventory_tab: Button = $CreativePanel/Margin/Content/Tabs/InventoryTab
@onready var craft_tab: Button = $CreativePanel/Margin/Content/Tabs/CraftTab
@onready var craft_scroll: ScrollContainer = $CreativePanel/Margin/Content/CraftScroll
@onready var craft_grid = $CreativePanel/Margin/Content/CraftScroll/Center/Recipe/CraftGrid
@onready var craft_result: Button = $CreativePanel/Margin/Content/CraftScroll/Center/Recipe/Result
@onready var hint: Label = $CreativePanel/Margin/Content/Hint
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
	hotbar_grid.set_show_item_count(true)
	hotbar_grid.set_interaction_enabled(false)
	_configure_grid(inventory_grid, 3, 9, Vector2i(SLOT_SIZE, SLOT_SIZE), false)
	inventory_grid.set_show_item_count(true)
	inventory_grid.set_interaction_enabled(false)
	_configure_grid(craft_grid, 2, 2, Vector2i(SLOT_SIZE, SLOT_SIZE), false)
	craft_grid.set_interaction_enabled(false)
	craft_grid.item_changed.connect(_update_craft_result)
	craft_tab.pressed.connect(select_craft_tab)
	craft_result.pressed.connect(_craft)
	_update_craft_result()
	creative_tab.pressed.connect(select_inventory_tab.bind(true))
	inventory_tab.pressed.connect(select_inventory_tab.bind(false))
	select_inventory_tab(true)
	resized.connect(_update_layout)
	_update_layout()

func select_inventory_tab(creative: bool) -> void:
	craft_scroll.hide()
	craft_tab.add_theme_stylebox_override("normal", _make_slot_style(false))
	hint.text = "Drag items to your inventory or hotbar"
	creative_scroll.visible = creative
	inventory_scroll.visible = not creative
	creative_tab.add_theme_stylebox_override("normal", _make_slot_style(creative))
	inventory_tab.add_theme_stylebox_override("normal", _make_slot_style(not creative))

func select_craft_tab() -> void:
	select_inventory_tab(false)
	inventory_scroll.hide()
	inventory_tab.add_theme_stylebox_override("normal", _make_slot_style(false))
	craft_scroll.show()
	craft_tab.add_theme_stylebox_override("normal", _make_slot_style(true))
	hint.text = "1 raw log → 4 planks · Click to craft"

func _update_craft_result(_cell = null, _item = null) -> void:
	var result = craft_grid.get_craft_result()
	craft_result.disabled = result == null
	craft_result.icon = BlockIconCache.get_icon(result.get_id()) if result != null else null
	craft_result.text = "4" if result != null else ""
	craft_result.tooltip_text = ("Craft 4 × " + result.get_name()) if result != null else "Place one raw log in any slot"

func _craft() -> void:
	var crafted: bool = craft_grid.craft_into(inventory_grid)
	if crafted:
		for row in range(inventory_grid.get_rows()):
			for column in range(inventory_grid.get_columns()):
				var cell := Vector2i(column, row)
				var item = inventory_grid.get_item_at(cell)
				if item != null and item.get_icon() == null:
					item.set_icon(BlockIconCache.get_icon(item.get_id()))
					inventory_grid.set_item_at(item, cell)
	_update_craft_result()
	if not crafted:
		craft_result.tooltip_text = "Inventory full — free space for 4 items"

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
	var hotbar_side := clampi(floori((available_width - 36.0 - 8 * SLOT_GAP) / 9.0), 20, SLOT_SIZE)
	var creative_side := clampi(floori((available_width - 48.0 - 7 * SLOT_GAP) / CREATIVE_COLUMNS), 20, 50)
	_resize_slots(hotbar_grid, hotbar_side)
	_resize_slots(creative_grid, creative_side)
	_resize_slots(inventory_grid, hotbar_side)
	_resize_slots(craft_grid, hotbar_side)
	var hotbar_size: Vector2 = hotbar_grid.get_combined_minimum_size() + Vector2(20, 18)
	var panel_width := maxf(hotbar_size.x, creative_grid.get_combined_minimum_size().x + 48.0)
	panel_width = maxf(panel_width, inventory_grid.get_combined_minimum_size().x + 28.0)
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
	item.set_item_amount(99)
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

func serialize_inventory() -> Dictionary:
	return {
		"hotbar": _serialize_grid(hotbar_grid),
		"storage": _serialize_grid(inventory_grid),
		"craft": _serialize_grid(craft_grid),
		"selected_slot": InventoryManager.get_selected_hotbar_slot(),
	}

func _serialize_grid(grid: Control) -> Array:
	var slots: Array = []
	for row in range(grid.get_rows()):
		for column in range(grid.get_columns()):
			var item = grid.get_item_at(Vector2i(column, row))
			if item != null:
				slots.append({"id": item.get_id(), "amount": item.get_item_amount()})
			else:
				slots.append(null)
	return slots

func restore_inventory(data: Dictionary) -> void:
	_restore_grid(hotbar_grid, data.get("hotbar", []))
	_restore_grid(inventory_grid, data.get("storage", []))
	_restore_grid(craft_grid, data.get("craft", []))
	InventoryManager.select_hotbar_slot(clampi(int(data.get("selected_slot", 0)), 0, 8))

func _restore_grid(grid: Control, saved: Variant) -> void:
	for row in range(grid.get_rows()):
		for column in range(grid.get_columns()):
			var cell := Vector2i(column, row)
			grid.clear_item_at(cell)
			var index: int = row * grid.get_columns() + column
			if not saved is Array or index >= saved.size() or not saved[index] is Dictionary:
				continue
			var entry: Dictionary = saved[index]
			var block := _block_by_id(int(entry.get("id", 0)))
			var amount := int(entry.get("amount", 0))
			if block.is_empty() or amount <= 0:
				continue
			var item = _make_block_item(block)
			item.set_item_amount(clampi(amount, 1, 99))
			grid.set_item_at(item, cell)

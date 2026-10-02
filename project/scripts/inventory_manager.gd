extends Node

var ui: Control
var mouse_unlocked := false
var selected_slot := 0

func setup(_player: Node, inventory_ui: Control) -> void:
	ui = inventory_ui
	mouse_unlocked = false
	selected_slot = InventorySession.selected_slot
	ui.tree_exiting.connect(_ui_exiting)
	ui.hotbar_grid.slot_clicked.connect(_hotbar_clicked)
	select_hotbar_slot(selected_slot)
	set_mouse_unlocked(false)

func _ui_exiting() -> void:
	ui = null
	mouse_unlocked = false

func _hotbar_clicked(cell: Vector2i, button: int) -> void:
	if mouse_unlocked and button == MOUSE_BUTTON_LEFT: select_hotbar_slot(cell.x)

func is_mouse_unlocked() -> bool:
	return mouse_unlocked

func set_mouse_unlocked(unlocked: bool) -> void:
	if not is_instance_valid(ui): return
	mouse_unlocked = unlocked
	ui.set_mouse_unlocked(unlocked)
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE if unlocked else Input.MOUSE_MODE_CAPTURED

func toggle_mouse() -> void:
	set_mouse_unlocked(not mouse_unlocked)

func is_inventory_open() -> bool:
	return is_instance_valid(ui) and (ui.inventory_panel.visible or ui.creative_panel.visible)

func open_inventory() -> void:
	if not is_instance_valid(ui): return
	set_mouse_unlocked(true)
	ui.inventory_panel.show()

func close_inventory() -> void:
	if not is_instance_valid(ui): return
	ui.inventory_panel.hide()
	ui.creative_panel.hide()

func toggle_inventory() -> void:
	if not is_instance_valid(ui): return
	if ui.inventory_panel.visible: ui.inventory_panel.hide()
	else: open_inventory()

func select_hotbar_slot(slot: int) -> void:
	selected_slot = posmod(slot, 9)
	InventorySession.selected_slot = selected_slot
	if is_instance_valid(ui): ui.hotbar_grid.set_selected_cell(Vector2i(selected_slot, 0))

func get_selected_hotbar_slot() -> int:
	return selected_slot

func get_selected_block_id() -> int:
	var ids: Dictionary = InventorySession.get_player_ids()
	return int(InventoryService.get_stack(ids.hotbar, selected_slot).get("id", 0))

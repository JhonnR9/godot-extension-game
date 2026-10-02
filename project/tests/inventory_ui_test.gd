extends SceneTree

class TestPlayer extends Node:
	var inventory_open := false

var failures := 0

func check(condition: bool, message: String) -> void:
	if not condition:
		failures += 1
		push_error(message)

func _initialize() -> void:
	call_deferred("run")

func move_mouse(position: Vector2) -> void:
	var event := InputEventMouseMotion.new()
	event.position = position
	event.global_position = position
	root.push_input(event, true)
	await process_frame

func run() -> void:
	root.size = Vector2i(960, 640)
	root.content_scale_size = Vector2i.ZERO
	var player := TestPlayer.new()
	root.add_child(player)
	var ui = load("res://scenes/inventory_ui.tscn").instantiate()
	root.add_child(ui)
	InventoryManager.setup(player, ui)
	for frame in range(4): await process_frame
	for index in range(9):
		check(ui.hotbar_grid.get_item_at(Vector2i(index, 0)) == null, "Hotbar starts prefilled.")
	check(InventoryManager.get_selected_block_id() == 0, "Empty hotbar still selects a block.")
	check(ui.hotbar_grid.get_child_count() == 9, "Hotbar has extra slot-number labels.")
	InventoryManager.open_inventory()
	for frame in range(4): await process_frame
	check(player.inventory_open and ui.creative_panel.visible, "Inventory did not open.")
	var center: Vector2 = ui.creative_panel.global_position + ui.creative_panel.size / 2.0
	check(center.distance_to(ui.size / 2.0) < 1.0, "Creative panel is not centered.")
	var grid_center: float = ui.creative_grid.global_position.x + ui.creative_grid.size.x / 2.0
	var scroll_center: float = ui.creative_scroll.global_position.x + (ui.creative_scroll.size.x - ui.creative_scroll.get_v_scroll_bar().size.x) / 2.0
	check(absf(grid_center - scroll_center) < 2.0, "Creative content is not centered.")
	check(ui.creative_scroll.get_v_scroll_bar().visible, "Creative inventory does not show its scrollbar.")
	for index in range(ui.blocks.size()):
		var item = ui.creative_grid.get_item_at(Vector2i(index % 8, index / 8))
		check(item != null and item.get_id() == ui.blocks[index].id, "Creative block missing.")
	var slot: Control = ui.creative_grid.get_child(0)
	var item = ui.creative_grid.get_item_at(Vector2i.ZERO)
	check(slot.tooltip_text.contains(item.get_name()) and slot.tooltip_text.contains("ID: %s" % item.get_id()) and slot.tooltip_text.contains("Categoria: " + item.get_category()), "C++ tooltip metadata missing.")
	check(item.duplicate_item().get_category() == item.get_category(), "Dragged copy loses category.")
	# Wheel input over a slot must propagate to the enclosing ScrollContainer.
	await move_mouse(slot.global_position + slot.size / 2.0)
	var wheel := InputEventMouseButton.new()
	wheel.button_index = MOUSE_BUTTON_WHEEL_DOWN
	wheel.pressed = true
	wheel.position = slot.global_position + slot.size / 2.0
	root.push_input(wheel, true)
	for frame in range(4): await process_frame
	check(ui.creative_scroll.scroll_vertical > 0, "Wheel over an item did not scroll.")
	ui.creative_scroll.scroll_vertical = 10000
	for frame in range(4): await process_frame
	var last: Control = ui.creative_grid.get_child(ui.blocks.size() - 1)
	check(ui.creative_scroll.get_global_rect().encloses(last.get_global_rect()), "Last creative block is unreachable.")
	# Exercise actual drag forwarding to an empty hotbar slot.
	var copy = item.duplicate_item()
	var preview := Control.new()
	slot.force_drag({"item": copy, "source": ui.creative_grid, "source_cell": Vector2i.ZERO, "creative_source": true}, preview)
	var target: Control = ui.hotbar_grid.get_child(0)
	var drop_position := target.global_position + target.size / 2.0
	await move_mouse(drop_position)
	var release := InputEventMouseButton.new()
	release.button_index = MOUSE_BUTTON_LEFT
	release.pressed = false
	release.position = drop_position
	root.push_input(release, true)
	for frame in range(4): await process_frame
	check(ui.hotbar_grid.get_item_at(Vector2i.ZERO) != null, "Creative drag did not fill hotbar.")
	check(InventoryManager.get_selected_block_id() == item.get_id(), "Selected block was not updated after drop.")
	check(ui.creative_grid.get_item_at(Vector2i.ZERO) == item, "Creative source was consumed.")
	# Smaller windows keep the panels on screen and retain manually added items.
	root.size = Vector2i(480, 360)
	for frame in range(6): await process_frame
	check(root.get_visible_rect().encloses(ui.creative_panel.get_global_rect()), "Creative panel overflows small viewport.")
	check(root.get_visible_rect().encloses(ui.hotbar_panel.get_global_rect()), "Hotbar overflows small viewport.")
	check(not ui.hotbar_panel.get_global_rect().intersects(ui.creative_panel.get_global_rect()), "Inventory overlaps hotbar.")
	var retained = ui.hotbar_grid.get_item_at(Vector2i.ZERO)
	check(retained != null and retained.get_id() == item.get_id(), "Resizing lost hotbar item.")
	ui.hotbar_grid.clear_item_at(Vector2i.ZERO)
	check(ui.hotbar_grid.get_child(0).tooltip_text.is_empty(), "Empty slot retains item tooltip.")
	InventoryManager.close_inventory()
	check(not player.inventory_open and not ui.creative_panel.visible, "Inventory did not close.")
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	ui.free()
	player.free()
	print("Inventory UI checks finished: ", failures, " failures.")
	quit(1 if failures else 0)

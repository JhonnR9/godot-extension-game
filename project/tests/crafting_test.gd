extends SceneTree

class CraftTestPlayer extends Node:
	var inventory_open := false

func _initialize() -> void:
	call_deferred("run")

func run() -> void:
	var ui = load("res://scenes/inventory_ui.tscn").instantiate()
	var player := CraftTestPlayer.new()
	root.add_child(player)
	root.add_child(ui)
	InventoryManager.setup(player, ui)
	InventoryManager.open_inventory()
	ui.craft_tab.pressed.emit()
	assert(ui.craft_scroll.visible and not ui.creative_scroll.visible and not ui.inventory_scroll.visible)
	assert(ui.creative_tab.get_parent().alignment == BoxContainer.ALIGNMENT_BEGIN)
	for id in [8, 23, 27]:
		var output_id: int = {8: 29, 23: 30, 27: 31}[id]
		for row in range(2):
			for column in range(2):
				ui.restore_inventory({})
				var wood = ui._make_block_item(ui._block_by_id(id))
				wood.set_item_amount(2)
				var cell := Vector2i(column, row)
				ui.craft_grid.set_item_at(wood, cell)
				assert(not ui.craft_result.disabled and ui.craft_result.text == "4")
				ui.craft_result.pressed.emit()
				assert(ui.inventory_grid.get_item_at(Vector2i.ZERO).get_id() == output_id)
				assert(ui.inventory_grid.get_item_at(Vector2i.ZERO).get_item_amount() == 4)
				assert(ui.inventory_grid.get_item_at(Vector2i.ZERO).get_category() == "building")
				assert(ui.inventory_grid.get_item_at(Vector2i.ZERO).get_icon() == root.get_node("BlockIconCache").get_icon(output_id))
				var round_trip: Dictionary = ui.serialize_inventory()
				ui.restore_inventory(round_trip)
				assert(ui.serialize_inventory() == round_trip)
				assert(ui.craft_grid.get_item_at(cell).get_item_amount() == 1)
				ui.craft_result.pressed.emit()
				assert(ui.inventory_grid.get_item_at(Vector2i.ZERO).get_item_amount() == 8)
				assert(ui.craft_grid.get_item_at(cell) == null and ui.craft_result.disabled)
	ui.restore_inventory({})
	ui.craft_grid.set_item_at(ui._make_block_item(ui._block_by_id(1)), Vector2i.ZERO)
	assert(ui.craft_grid.get_craft_result() == null)
	ui.restore_inventory({})
	var wood = ui._make_block_item(ui._block_by_id(8))
	wood.set_item_amount(1)
	ui.craft_grid.set_item_at(wood, Vector2i.ZERO)
	ui.craft_grid.set_item_at(wood.duplicate_item(), Vector2i.ONE)
	assert(ui.craft_grid.get_craft_result() == null)
	ui.craft_grid.clear_item_at(Vector2i.ONE)
	for row in range(3):
		for column in range(9):
			ui.inventory_grid.set_item_at(ui._make_block_item(ui._block_by_id(8)), Vector2i(column, row))
	assert(not ui.craft_grid.craft_into(ui.inventory_grid))
	assert(ui.craft_grid.get_item_at(Vector2i.ZERO).get_item_amount() == 1)
	var saved: Dictionary = ui.serialize_inventory()
	ui.restore_inventory({})
	ui.restore_inventory(saved)
	assert(ui.serialize_inventory() == saved)
	InventoryManager.close_inventory()
	assert(not ui.craft_grid.is_interaction_enabled())
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	ui.free()
	player.free()
	print("Crafting checks passed: all log types, all slots, consumption, invalid recipes, full inventory, persistence.")
	quit()

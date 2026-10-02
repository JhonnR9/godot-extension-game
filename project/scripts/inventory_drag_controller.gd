extends Node

signal transfer_finished(result: Dictionary)
var views: Dictionary = {}
var active: Dictionary = {}
var next_token := 0

func _ready() -> void:
	InventoryService.inventory_changed.connect(_model_changed)

func bind_grid(grid: GridInventory, uuid: String) -> void:
	assert(InventoryService.has_inventory(uuid))
	var key := grid.get_instance_id()
	if not views.has(key):
		grid.drag_started.connect(_drag_started.bind(key))
		grid.drop_hovered.connect(_drop_hovered.bind(key))
		grid.drop_requested.connect(_drop_requested.bind(key))
		grid.drag_finished.connect(cancel_drag)
		grid.tree_exiting.connect(_unbind.bind(key))
		grid.visibility_changed.connect(_visibility_changed.bind(key))
	elif not active.is_empty() and active.view == key:
		cancel_drag()
	views[key] = {"ref": weakref(grid), "uuid": uuid}
	grid.set_inventory_id(uuid)
	grid.set_slot_count(InventoryService.get_capacity(uuid))
	refresh(uuid)

func _unbind(key: int) -> void:
	if not active.is_empty() and active.view == key: cancel_drag()
	views.erase(key)

func _visibility_changed(key: int) -> void:
	var grid = _grid(key)
	if not active.is_empty() and active.view == key and (grid == null or not grid.is_visible_in_tree()): cancel_drag()

func _grid(key: int):
	return views[key].ref.get_ref() if views.has(key) else null

func refresh(uuid: String, index: int = -1) -> void:
	for key in views:
		if views[key].uuid != uuid: continue
		var grid = _grid(key)
		if grid == null: continue
		var indices = range(grid.get_slot_count()) if index < 0 else [index]
		for slot in indices:
			grid.set_item_view(slot, InventorySession.make_view(InventoryService.get_stack(uuid, slot)))
		grid.set_hidden_slot(active.index if not active.is_empty() and active.uuid == uuid and not InventoryService.is_copy_source(uuid) else -1)

func _model_changed(uuid: String, index: int) -> void:
	# A mutation invalidates any pending request based on the previous snapshot.
	if not active.is_empty() and active.uuid == uuid and InventoryService.get_revision(uuid) != active.revision:
		cancel_drag()
	refresh(uuid, index)

func _drag_started(index: int, key: int) -> void:
	var grid = _grid(key)
	if grid == null or not grid.is_interaction_enabled() or not grid.is_visible_in_tree(): return
	cancel_drag()
	var uuid: String = views[key].uuid
	var stack: Dictionary = InventoryService.get_stack(uuid, index)
	if stack.is_empty(): return
	next_token += 1
	active = {"token": next_token, "view": key, "uuid": uuid, "index": index, "amount": int(stack.amount), "revision": InventoryService.get_revision(uuid)}
	grid.set_drag_payload({"inventory_drag": next_token})
	refresh(uuid)

func _valid_target(payload: Dictionary, key: int) -> bool:
	if active.is_empty() or payload.get("inventory_drag", -1) != active.token: return false
	var source = _grid(active.view)
	var target = _grid(key)
	return source != null and source.is_visible_in_tree() and source.is_interaction_enabled() and target != null and target.is_visible_in_tree() and target.is_interaction_enabled()

func _drop_hovered(payload: Dictionary, index: int, key: int) -> void:
	var grid = _grid(key)
	if grid == null: return
	var allowed := false
	if _valid_target(payload, key):
		allowed = InventoryService.can_transfer(active.uuid, active.index, views[key].uuid, index, active.amount, active.revision)
	grid.set_drop_allowed(allowed)

func _drop_requested(payload: Dictionary, index: int, key: int) -> void:
	if not _valid_target(payload, key):
		cancel_drag()
		transfer_finished.emit({"success": false, "reason": "invalid_target"})
		return
	var request: Dictionary = active.duplicate()
	var target: String = views[key].uuid
	# Clear transient UI state before model notifications arrive synchronously.
	active = {}
	var result: Dictionary = InventoryService.transfer(request.uuid, request.index, target, index, request.amount, request.revision)
	# Re-read every ItemView field even on rejection, partial merge or same-slot drop.
	refresh(request.uuid)
	refresh(target)
	transfer_finished.emit(result)

func cancel_drag() -> void:
	if active.is_empty(): return
	var source: String = active.uuid
	active = {}
	refresh(source)

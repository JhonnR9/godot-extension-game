# Integration smoke test. Run with an isolated XDG_DATA_HOME to keep test worlds separate.
extends SceneTree

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	var world = ClassDB.instantiate("VoxelAPI")
	root.add_child(world)
	world.set_render_settings({"render_distance": 4, "vertical_render_distance": 2})
	world.set_focus_position(Vector3(0, -48, 0))
	var world_id := int(SaveService.create_world(42, "Underground integration test"))
	world.start_world(world_id)
	var deadline := Time.get_ticks_msec() + 20000
	while world.is_initial_loading() and Time.get_ticks_msec() < deadline:
		await process_frame
	assert(not world.is_initial_loading(), "Underground chunks did not finish loading.")
	var counts: Dictionary = {}
	for z in range(-32, 32):
		for x in range(-32, 32):
			for y in range(-80, -16):
				var id := int(world.get_block_type_at(Vector3(x, y, z)))
				assert(id >= 0, "Expected a loaded underground block.")
				counts[id] = int(counts.get(id, 0)) + 1
	assert(int(counts.get(6, 0)) > 100, "Iron veins missing.")
	assert(int(counts.get(22, 0)) > 100, "Coal veins missing.")
	assert(int(counts.get(2, 0)) > 100, "Dirt pockets missing.")
	assert(int(counts.get(0, 0)) > 100, "Open tunnel volume missing.")
	print("Underground world integration passed. Block counts: ", counts)
	# Allow pending neighbour mesh jobs to drain before process shutdown.
	for frame in range(60):
		await process_frame
	quit()

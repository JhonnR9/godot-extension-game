extends "res://tests/biome_registry_test.gd"

func run() -> void:
	var data: Dictionary = JSON.parse_string(FileAccess.get_file_as_string("res://data/biome_registry.json"))
	for b in data.biomes:
		if b.name == "beach":
			check(b.trees.shape == "palm" and b.trees.trunk == "palm_log", "Beach palm profile missing.")
		elif b.name == "desert" or b.name == "ocean":
			check(b.get("trees", {}).get("max_per_chunk", 0) == 0, "Palms configured outside beach.")
	var invalid := data.duplicate(true)
	invalid.biomes[0].trees.shape = "unknown"
	check(not VoxelAPI.validate_biome_registry(invalid).valid, "Invalid tree shape accepted.")
	# Fully flatten the shoreline and remove rivers to exercise beach trees
	# across positive/negative chunk boundaries with different loading orders.
	data.world.amplitude = 0
	data.world.climate_start = -1
	data.world.climate_span = 0.001
	data.world.coast_start = 1
	data.world.dry_coast_start = 1
	data.world.coast_span = 0.001
	data.world.dry_coast_span = 0.001
	data.world.wet_coast_offset = 0
	for b in data.biomes:
		b.relief = b.get("relief", {})
		b.relief.ridge_amplitude = 0
		b.relief.bias = 0
		b.vegetation = {}
		if b.name == "river": b.rarity = 0
	write_config("user://palm_beach.json", data)
	var a := make_world("user://palm_beach.json", Vector3(0, 24, 0), "Palm beach A")
	var b := make_world("user://palm_beach.json", Vector3(16, 24, 16), "Palm beach B")
	if await wait_for_world(a) and await wait_for_world(b):
		var trunks := 0
		var leaves := 0
		var palm_log := block_id("palm_log")
		var palm_leaves := block_id("palm_leaves")
		for z in range(-16, 32):
			for x in range(-16, 32):
				var c: Dictionary = a.sample_terrain_column(Vector2i(x, z))
				check(c.biome_name == "beach", "Palm fixture did not select beach.")
				for y in range(25, 39):
					var p := Vector3(x, y, z)
					var first := int(a.get_block_type_at(p))
					check(first == int(b.get_block_type_at(p)), "Palm changed across chunk loading order at %s" % p)
					if first == palm_log: trunks += 1
					if first == palm_leaves: leaves += 1
		check(trunks >= 14 and leaves > 50, "Beach palms were not generated.")
	for frame in range(60): await process_frame
	print("Palm tree checks finished: ", failures, " failures.")
	quit(1 if failures else 0)

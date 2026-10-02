extends "res://tests/biome_registry_test.gd"

func run() -> void:
	var data: Dictionary = JSON.parse_string(FileAccess.get_file_as_string("res://data/biome_registry.json"))
	var plain: Dictionary
	for biome in data.biomes:
		if biome.name == "plains": plain = biome
	var snow: Dictionary
	for biome in data.biomes:
		if biome.name == "snow": snow = biome
	var grass := block_id("grass")
	for seed in [42, 1234, 2026]:
		var world: Node = ClassDB.instantiate("VoxelAPI")
		root.add_child(world)
		world.set_render_settings({"render_distance": 4, "vertical_render_distance": 2})
		world.set_focus_position(Vector3(0, 32, 0))
		world.start_world(int(SaveService.create_world(seed, "Biome distribution %s" % seed)))
		await wait_for_world(world)
		var climate := {"snow": 0, "plains": 0, "desert": 0}
		var terrain := {"snow": 0, "mountains": 0, "plains": 0, "desert": 0}
		var green := 0
		var total := 0
		for z in range(-8192, 8193, 128):
			for x in range(-8192, 8193, 128):
				var c: Dictionary = world.sample_terrain_column(Vector2i(x, z))
				climate["snow" if c.climate < snow.selection.climate_max else "plains" if c.climate < plain.selection.climate_max else "desert"] += 1
				if terrain.has(c.biome_name): terrain[c.biome_name] += 1
				if c.biome_name == "plains" and c.surface_block == grass: green += 1
				total += 1
		var land_count: int = terrain.snow + terrain.mountains + terrain.plains + terrain.desert
		check(float(climate.plains) / total > 0.50, "Normal climate is no longer dominant for seed %s." % seed)
		check(float(climate.snow) / total > 0.10 and float(climate.desert) / total > 0.10, "Extreme climates disappeared.")
		check(float(terrain.plains + terrain.mountains) / land_count > 0.45, "Normal land is too scarce for seed %s." % seed)
		check(float(green) / terrain.plains > 0.65, "Normal biome is mostly bare dirt for seed %s." % seed)
		print("Seed ", seed, ": climate ", climate, "; land ", terrain, "; grassy plains ", green)
		world.queue_free()
		for frame in range(8): await process_frame
	print("Biome distribution checks finished: ", failures, " failures.")
	quit(1 if failures else 0)

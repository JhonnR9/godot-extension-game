extends "res://tests/biome_registry_test.gd"

func run() -> void:
	var data: Dictionary = JSON.parse_string(FileAccess.get_file_as_string("res://data/biome_registry.json"))
	var plains: Dictionary
	var mountains: Dictionary
	for biome in data.biomes:
		if biome.name == "plains": plains = biome.duplicate(true)
		if biome.name == "mountains": mountains = biome
	check(mountains.id == 0 and mountains.relief.scale == 2.4, "Original mountain identity/relief changed.")
	check(plains.id == 9 and plains.trees.max_per_chunk == 1, "New plains identity/sparse trees missing.")
	var invalid := data.duplicate(true)
	invalid.biomes[0].vegetation.cluster_radius = 100
	check(not VoxelAPI.validate_biome_registry(invalid).valid, "Oversized flower cluster accepted.")
	# Isolate the real plains profile to measure its relief and decorations.
	plains.selection = {"kind": "land"}
	data.biomes = [plains]
	data.world.coast_start = -1
	data.world.coast_span = 0.001
	write_config("user://flat_plains.json", data)
	var world := make_world("user://flat_plains.json", Vector3(0, 32, 0), "Sparse flat plains")
	await wait_for_world(world)
	var low := 255
	var high := -255
	for z in range(-2048, 2049, 32):
		for x in range(-2048, 2049, 32):
			var c: Dictionary = world.sample_terrain_column(Vector2i(x, z))
			low = mini(low, c.height)
			high = maxi(high, c.height)
	check(high - low <= 5, "Plains still have mountainous relief: %s..%s" % [low, high])
	var flowers := [block_id("flower"), block_id("daisy"), block_id("cornflower"), block_id("poppy")]
	var grass := [block_id("short_grass"), block_id("tall_grass"), block_id("fern")]
	var flower_count := 0
	var occupied_patches := {}
	for z in range(-48, 48):
		for x in range(-48, 48):
			var c: Dictionary = world.sample_terrain_column(Vector2i(x, z))
			var block := int(world.get_block_type_at(Vector3(x, c.height + 1, z)))
			check(not grass.has(block), "Sparse plains generated dense grass/ferns.")
			if flowers.has(block):
				flower_count += 1
				occupied_patches[Vector2i(floori(x / 24.0), floori(z / 24.0))] = true
	check(flower_count > 5 and flower_count < 250, "Plains flowers are absent or too dense: %s" % flower_count)
	check(occupied_patches.size() < 8, "Flowers are scattered across most patches instead of grouped.")
	print("Plains checks finished: ", failures, " failures; height ", low, "..", high, "; ", flower_count, " flowers in ", occupied_patches.size(), " clusters.")
	for frame in range(60): await process_frame
	quit(1 if failures else 0)

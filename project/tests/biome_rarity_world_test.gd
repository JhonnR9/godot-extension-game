# Run each rarity in a separate process to isolate rendering worker lifetimes.
extends "res://tests/biome_registry_test.gd"

func run() -> void:
	var args := OS.get_cmdline_user_args()
	var rarity := int(args[0]) if args.size() else 4
	check(rarity in [0, 1, 2, 4], "Use rarity 0, 1, 2 or 4.")
	# Force the desert climate so only regional rarity determines its presence.
	var data: Dictionary = JSON.parse_string(FileAccess.get_file_as_string("res://data/biome_registry.json"))
	var rarity_config := data.duplicate(true)
	rarity_config.biomes = rarity_config.biomes.slice(0, 2)
	rarity_config.world.base_height = 32
	rarity_config.world.sea_level = 0
	rarity_config.world.amplitude = 0
	rarity_config.world.climate_start = -1
	rarity_config.world.climate_span = 0.001
	rarity_config.world.coast_start = -1
	rarity_config.world.dry_coast_start = -1
	for b in rarity_config.biomes:
		b.relief.ridge_amplitude = 0
		b.relief.bias = 0
		b.trees = {}
		b.vegetation = {}
	rarity_config.biomes[1].rarity = rarity
	# Disabled relief must not leak into the fallback's terrain.
	rarity_config.biomes[1].relief.bias = 128 if rarity == 0 else 0
	var path := "user://rarity_%s.json" % rarity
	write_config(path, rarity_config)
	var rarity_world := make_world(path, Vector3(0, 32, 0), "Biome rarity %s test" % rarity)
	await wait_for_world(rarity_world)
	var count := 0
	for z in range(-4096, 4097, 64):
		for x in range(-4096, 4097, 64):
			var c: Dictionary = rarity_world.sample_terrain_column(Vector2i(x, z))
			if c.biome_name == "desert": count += 1
			check(c.height == 32, "Disabled biome relief affected terrain.")
			if rarity == 0: check(c.biome_name == "plains", "Disabled biome appeared.")
	var expected: Dictionary = {0: 0, 1: 16641, 2: 8395, 4: 2477}
	check(count == expected[rarity], "Unexpected regional biome population: %s" % count)
	for z in [-16, -1, 0, 15, 16]:
		for x in [-16, -1, 0, 15, 16]:
			var c: Dictionary = rarity_world.sample_terrain_column(Vector2i(x, z))
			check(rarity_world.get_block_type_at(Vector3(x, c.height, z)) == c.surface_block, "Rarity sampler/generation disagreement.")
	for frame in range(60):
		await process_frame
	print("Biome rarity ", rarity, ": ", count, " desert samples; ", failures, " failures.")
	quit(1 if failures else 0)

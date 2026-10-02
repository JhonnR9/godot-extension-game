extends "res://tests/biome_registry_test.gd"

func run() -> void:
	var data: Dictionary = JSON.parse_string(FileAccess.get_file_as_string("res://data/biome_registry.json"))
	check(VoxelAPI.validate_biome_registry(data).valid, "Snow registry rejected.")
	var invalid := data.duplicate(true)
	invalid.biomes[0].surface_fill = "flower"
	check(not VoxelAPI.validate_biome_registry(invalid).valid, "Non-solid surface fill accepted.")
	var snow: Dictionary
	for biome in data.biomes:
		if biome.name == "snow": snow = biome.duplicate(true)
	check(snow.trees.shape == "pine" and snow.surface_fill == "ice", "Snow pine/ice profile missing.")
	# Flat snow forest exercises conical pine crowns and scheduling at borders.
	var forest := data.duplicate(true)
	forest.biomes = [snow]
	forest.biomes[0].selection = {"kind": "land"}
	forest.biomes[0].relief = {"anchor": 0, "ridge_amplitude": 0, "bias": 0}
	forest.biomes[0].trees.min_height = 10
	forest.biomes[0].trees.max_height = 10
	forest.world.base_height = 32
	forest.world.amplitude = 0
	forest.world.coast_start = -1
	forest.world.coast_span = 0.001
	write_config("user://snow_forest.json", forest)
	var a := make_world("user://snow_forest.json", Vector3(0, 32, 0), "Snow pine forest A")
	var b := make_world("user://snow_forest.json", Vector3(16, 32, 16), "Snow pine forest B")
	var pine_log := block_id("pine_log")
	var pine_leaves := block_id("pine_leaves")
	if await wait_for_world(a) and await wait_for_world(b):
		var trunks := 0
		var leaves := 0
		var bottom_crown := 0
		var top_crown := 0
		for z in range(-16, 32):
			for x in range(-16, 32):
				var column: Dictionary = a.sample_terrain_column(Vector2i(x, z))
				check(column.height == 32 and column.surface_block == block_id("snow"), "Snow surface incorrect.")
				for y in range(33, 46):
					var p := Vector3(x, y, z)
					var first := int(a.get_block_type_at(p))
					check(first == int(b.get_block_type_at(p)), "Pine changed with chunk loading order at %s" % p)
					if first == pine_log: trunks += 1
					if first == pine_leaves:
						leaves += 1
						if y == 36: bottom_crown += 1
						if y == 44: top_crown += 1
		check(trunks > 30 and leaves > 100, "Snow pines missing.")
		check(bottom_crown > top_crown and top_crown > 0, "Pine crown did not taper to a tip.")
	# Full coastal influence must select a frozen ocean in cold climate and
	# ordinary water in warm climate, using actual generated blocks.
	for cold in [true, false]:
		var coast := data.duplicate(true)
		coast.world.climate_start = 1 if cold else -1
		coast.world.climate_span = 0.001
		coast.world.coast_start = 1
		coast.world.coast_span = 0.001
		coast.world.dry_coast_start = 1
		coast.world.dry_coast_span = 0.001
		for biome in coast.biomes:
			biome.trees = {}
			biome.vegetation = {}
		var path := "user://snow_coast_%s.json" % cold
		write_config(path, coast)
		var world := make_world(path, Vector3(0, 24, 0), "Frozen ocean %s" % cold)
		if await wait_for_world(world):
			var fill := block_id("ice" if cold else "water")
			for z in [-17, -16, -1, 0, 15, 16, 17]:
				for x in [-17, -16, -1, 0, 15, 16, 17]:
					var column: Dictionary = world.sample_terrain_column(Vector2i(x, z))
					check(column.biome_name == ("frozen_ocean" if cold else "ocean"), "Incorrect coastal climate selection.")
					check(column.surface_fill == fill, "Incorrect coastal fill palette.")
					for y in range(column.height + 1, column.water_level):
						var p := Vector3(x, y, z)
						check(world.get_block_type_at(p) == fill, "Generated ice/water disagrees at %s" % p)
						check(world.is_water_at(p) == not cold, "Solid ice was treated as swimming water.")
	for frame in range(60): await process_frame
	print("Snow biome checks finished: ", failures, " failures.")
	quit(1 if failures else 0)

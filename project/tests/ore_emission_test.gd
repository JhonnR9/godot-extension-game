extends SceneTree

func _initialize() -> void:
	call_deferred("_run")

func _mask(color: Color, kind: String) -> float:
	var linear := color.srgb_to_linear()
	return smoothstep(0.045, 0.16, linear.r - linear.b) if kind == "iron_ore" else smoothstep(0.025, 0.15, minf(linear.g, linear.b) - linear.r)

func _run() -> void:
	for kind: String in ["iron_ore", "diamond_ore"]:
		var texture := load("res://textures/blocks/" + kind + ".png") as Texture2D
		var image := texture.get_image()
		assert(image.get_size() == Vector2i(32, 32))
		var emitting := 0
		var neutral := 0
		for y in range(32):
			for x in range(32):
				var color := image.get_pixel(x, y)
				var mask := _mask(color, kind)
				if mask > 0.05:
					emitting += 1
				if absf(color.r - color.g) < 0.015 and absf(color.g - color.b) < 0.015:
					neutral += 1
					assert(mask == 0.0)
		assert(emitting > 30 and emitting < 410)
		assert(neutral > 100)
		print(kind, ": ", emitting, "/1024 inclusion pixels emit")
	# Host rock and coal must remain non-emissive under either analytical mask.
	for name: String in ["stone", "coal_ore"]:
		var image: Image = load("res://textures/blocks/" + name + ".png").get_image()
		var emitting := 0
		for y in range(32):
			for x in range(32):
				var color := image.get_pixel(x, y)
				if _mask(color, "iron_ore") > 0.05 or _mask(color, "diamond_ore") > 0.05:
					emitting += 1
		assert(emitting < 10)
	var registry: Dictionary = JSON.parse_string(FileAccess.get_file_as_string("res://data/block_registry.generated.json"))
	# Additional block types may extend the registry without changing ore IDs.
	assert(registry.blocks.size() >= 23)
	assert(registry.blocks[22].name == "coal_ore")
	for block: Dictionary in registry.blocks:
		for layer: int in block.get("texture_layers", {}).values():
			assert(layer >= 0 and layer < int(registry.water_texture_layer))
	print("Ore emission tests passed: colored inclusions, neutral rock, coal, atlas bounds.")
	quit()

extends SceneTree

const CacheScript = preload("res://scripts/block_icon_cache.gd")

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	var file := FileAccess.open("res://data/block_registry.generated.json", FileAccess.READ)
	var blocks: Array = JSON.parse_string(file.get_as_text()).blocks
	var count := blocks.size() - 1
	var startup = root.get_node("BlockIconCache")
	print("Startup cache: ", startup.last_run)
	var path := "user://cache/icon_test_" + str(Time.get_ticks_usec())
	var first = CacheScript.new()
	assert(first.prepare(blocks, path) == {"generated": count, "loaded": 0})
	for block: Dictionary in blocks:
		if int(block.id) == 0:
			continue
		assert(first.get_icon(int(block.id)) != null)
		assert(FileAccess.file_exists(path.path_join(str(int(block.id)) + ".png")))
	var original: Image = first.get_icon(1).get_image()
	assert(original.get_pixel(0, 0).a == 0.0)
	assert(original.get_pixel(35, 40).a > 0.0)
	# A fresh cache object must reuse every file, without source image loading.
	var second = CacheScript.new()
	assert(second.prepare(blocks, path) == {"generated": 0, "loaded": count})
	assert(second.get_icon(1).get_image().get_data() == original.get_data())
	# A malformed PNG size regenerates only the damaged entry.
	Image.create(2, 2, false, Image.FORMAT_RGBA8).save_png(path.path_join("1.png"))
	assert(second.prepare(blocks, path) == {"generated": 1, "loaded": count - 1})
	# Metadata/tint changes invalidate only the changed block.
	var modified := blocks.duplicate(true)
	modified[1].tint = [0.4, 0.5, 0.6, 1.0]
	assert(second.prepare(modified, path) == {"generated": 1, "loaded": count - 1})
	assert(second.get_icon(1).get_image().get_data() != original.get_data())
	# Missing PNGs are regenerated despite a matching manifest signature.
	DirAccess.remove_absolute(path.path_join("2.png"))
	assert(second.prepare(modified, path) == {"generated": 1, "loaded": count - 1})
	# Projection: independent top/side textures and different side shading.
	var white := Image.create(1, 1, false, Image.FORMAT_RGBA8)
	white.fill(Color.WHITE)
	var red := Image.create(1, 1, false, Image.FORMAT_RGBA8)
	red.fill(Color.RED)
	second._source_images = {"test_side": white, "test_top": red}
	var cube: Image = second._render_icon({"textures": {"side": "test_side", "top": "test_top"}})
	assert(cube.get_pixel(30, 15).r > 0.9 and cube.get_pixel(30, 15).g == 0.0)
	assert(cube.get_pixel(20, 36).r < cube.get_pixel(46, 36).r)
	assert(cube.get_pixel(35, 58).a > 0.0)
	# Inventory and hotbar must use the cached Texture2D instances.
	var inventory = load("res://scenes/inventory_ui.tscn").instantiate()
	root.add_child(inventory)
	var item = inventory._make_block_item(inventory.blocks[0])
	assert(item.get_icon() == startup.get_icon(item.get_id()))
	inventory.free()
	# Remove only this test's temporary files.
	for block: Dictionary in blocks:
		if int(block.id) != 0:
			DirAccess.remove_absolute(path.path_join(str(int(block.id)) + ".png"))
	DirAccess.remove_absolute(path.path_join("manifest.cfg"))
	DirAccess.remove_absolute(path)
	first.free()
	second.free()
	print("Block icon cache tests passed: cold/warm, repair, tint, missing file, cube projection.")
	quit()

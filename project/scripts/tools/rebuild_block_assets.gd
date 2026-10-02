# Run with: godot --headless --path project --script res://scripts/tools/rebuild_block_assets.gd
extends SceneTree

func _initialize() -> void:
	var generator = load("res://addons/block_registry/block_asset_generator.gd").new()
	if not generator.rebuild():
		quit(1)
		return
	print("Block registry, C++ IDs and texture array rebuilt.")
	quit()

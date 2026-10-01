@tool
extends EditorScript

const GENERATOR_PATH := "res://addons/block_registry/block_asset_generator.gd"


func _run() -> void:
	var generator_script: Script = load(GENERATOR_PATH)
	if generator_script == null:
		push_error("Could not load the block asset generator.")
		return
	var generator = generator_script.new()
	if generator.rebuild():
		print("Block texture array and registry metadata generated.")

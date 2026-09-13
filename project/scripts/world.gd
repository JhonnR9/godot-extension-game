extends Node

@onready var voxel_world = $World 

func _ready() -> void:
	load_existing_game(GameSession.selected_world_id)

func _process(_delta: float) -> void:
	if Input.is_action_just_pressed("save"):
		voxel_world.save_world()
		print("Mundo salvo com sucesso")

func start_new_game(world_name: String, world_seed: int) -> void:
	var world_id = SaveService.create_world(world_seed, world_name)
	load_existing_game(world_id)

func load_existing_game(world_id: int) -> void:
	print(GameSession.selected_world_id)
	voxel_world.start_world(world_id)
	print("Mundo carregado com sucesso! ID: ", world_id)

func save_current_game() -> void:
	voxel_world.save_world()
	print("Mundo salvo com sucesso!")

func delete_game(world_id: int) -> void:
	SaveService.delete_world(world_id)
	print("Mundo deletado: ", world_id)

func listar_mundos_salvos() -> void:
	var worlds: PackedInt64Array = SaveService.get_saved_worlds()
	
	if worlds.size() == 0:
		print("Nenhum mundo salvo encontrado.")
		return

	for world_id in worlds:
		print("Mundo encontrado com ID: ", world_id)

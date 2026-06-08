extends Node

@onready var voxel_world = $World 

func _ready() -> void:
	
	var saved_worlds: PackedInt64Array = voxel_world.get_saved_worlds()
	
	
	if saved_worlds.size() > 0:

		var world_to_load = saved_worlds[0]
		print("Mundo encontrado! Carregando ID: ", world_to_load)
		load_existing_game(world_to_load)
	else:
		
		print("Nenhum mundo encontrado. Criando um novo mundo...")
		start_new_game("Meu Mundo", 474734)

func _process(_delta: float) -> void:
	if Input.is_action_just_pressed("save"):
		voxel_world.save_world()
		print("Mundo salvo com sucesso")

func start_new_game(world_name: String, world_seed: int) -> void:
	voxel_world.create_new_world(world_seed, world_name)
	print("Novo mundo gerado: ", world_name, " com seed: ", world_seed)

func load_existing_game(world_id: int) -> void:
	voxel_world.load_world(world_id)
	print("Mundo carregado com sucesso! ID: ", world_id)

func save_current_game() -> void:

	voxel_world.save_world()
	print("Mundo salvo com sucesso!")

func delete_game(world_id: int) -> void:
	voxel_world.delete_world(world_id)
	print("Mundo deletado: ", world_id)

func listar_mundos_salvos() -> void:
	var worlds: PackedInt64Array = voxel_world.get_saved_worlds()
	
	if worlds.size() == 0:
		print("Nenhum mundo salvo encontrado.")
		return

	for world_id in worlds:
		print("Mundo encontrado com ID: ", world_id)

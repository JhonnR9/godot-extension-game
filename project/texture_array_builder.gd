@tool
extends EditorScript

const TEXTURE_PATH = "res://textures/blocks/"
const OUTPUT_PATH = "res://textures/block_array.trezs"
const JSON_PATH = "res://textures/block_mapping.json"

func _run():
	var dir = DirAccess.open(TEXTURE_PATH)
	if not dir:
		printerr("Erro: Pasta não encontrada: ", TEXTURE_PATH)
		return

	dir.list_dir_begin()
	var file_name = dir.get_next()
	var file_paths: Array[String] = []

	# Coleta todos os arquivos de imagem
	while file_name != "":
		if not dir.current_is_dir() and (file_name.ends_with(".png") or file_name.ends_with(".jpg")):
			file_paths.append(TEXTURE_PATH + file_name)
		file_name = dir.get_next()
	
	file_paths.sort()

	if file_paths.size() > 0:
		var images: Array[Image] = []
		
		# Define o tamanho baseado na primeira imagem
		var first_image: Image = load(file_paths[0]).get_image()
		first_image.convert(Image.FORMAT_RGBA8)
		var size = first_image.get_size()
		
		for path in file_paths:
			var tex = load(path)
			if tex:
				var img = tex.get_image()
				img.convert(Image.FORMAT_RGBA8)
				
				# Garante consistência de tamanho
				if img.get_size() == size:
					images.append(img)
					print("Adicionada: ", path.get_file())
				else:
					printerr("Ignorada (tamanho diferente): ", path.get_file())

		# Cria e salva o Texture2DArray
		var tex_array = Texture2DArray.new()
		var error = tex_array.create_from_images(images)
		
		if error == OK:
			ResourceSaver.save(tex_array, OUTPUT_PATH)
			print("Sucesso! Texture2DArray salvo em: ", OUTPUT_PATH)
			
			# Gera o arquivo JSON de mapeamento
			var json_map = {}
			for i in range(file_paths.size()):
				var name = file_paths[i].get_file().get_basename()
				json_map[name] = i
				
			var json_string = JSON.stringify(json_map, "\t")
			var file = FileAccess.open(JSON_PATH, FileAccess.WRITE)
			if file:
				file.store_string(json_string)
				print("Mapeamento JSON salvo em: ", JSON_PATH)
		else:
			printerr("Erro ao criar Texture2DArray. Código de erro: ", error)
	else:
		print("Nenhuma textura encontrada.")

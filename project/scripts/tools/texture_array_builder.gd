@tool
extends EditorScript

const TEXTURE_PATH := "res://textures/blocks/"
const OUTPUT_PATH := "res://textures/block_array.tres"
const JSON_PATH := "res://textures/block_mapping.json"


func _run():
	print("")
	print("========================================")
	print(" GERADOR DE TEXTURE2DARRAY")
	print("========================================")
	print("")


	# ---------------------------------------------------------
	# ABRE A PASTA
	# ---------------------------------------------------------

	var dir := DirAccess.open(TEXTURE_PATH)

	if dir == null:
		printerr("ERRO: Pasta não encontrada: ", TEXTURE_PATH)
		return


	# ---------------------------------------------------------
	# ENCONTRA AS TEXTURAS
	# ---------------------------------------------------------

	dir.list_dir_begin()

	var file_name := dir.get_next()
	var file_paths: Array[String] = []

	while file_name != "":
		if not dir.current_is_dir():
			if file_name.ends_with(".png") or file_name.ends_with(".jpg"):
				file_paths.append(TEXTURE_PATH + file_name)

		file_name = dir.get_next()

	dir.list_dir_end()

	file_paths.sort()


	if file_paths.is_empty():
		print("Nenhuma textura encontrada.")
		return


	print("Texturas encontradas: ", file_paths.size())
	print("")


	# ---------------------------------------------------------
	# ARRAYS
	# ---------------------------------------------------------

	var images: Array[Image] = []
	var valid_names: Array[String] = []

	var reference_size := Vector2i(-1, -1)


	# ---------------------------------------------------------
	# PROCESSA CADA TEXTURA
	# ---------------------------------------------------------

	for path in file_paths:

		print("----------------------------------------")
		print("Processando: ", path.get_file())


		# -----------------------------------------------------
		# CARREGA A TEXTURA
		# -----------------------------------------------------

		var tex = load(path)

		if tex == null:
			printerr("ERRO: Não foi possível carregar: ", path)
			continue


		if not tex is Texture2D:
			printerr("ERRO: Arquivo não é Texture2D: ", path)
			continue


		# -----------------------------------------------------
		# PEGA A IMAGE
		# -----------------------------------------------------

		var img: Image = tex.get_image()

		if img == null:
			printerr("ERRO: Não foi possível obter Image: ", path)
			continue


		print("Tamanho original: ", img.get_size())
		print("Formato original: ", img.get_format())
		print("Comprimida: ", img.is_compressed())
		print("Mipmaps: ", img.has_mipmaps())


		# -----------------------------------------------------
		# DESCOMPRIME
		# -----------------------------------------------------

		if img.is_compressed():

			print("Descomprimindo...")

			var decompress_error := img.decompress()

			if decompress_error != OK:
				printerr(
					"ERRO ao descomprimir ",
					path.get_file(),
					" | Código: ",
					decompress_error
				)

				continue


		# -----------------------------------------------------
		# REMOVE MIPMAPS
		# -----------------------------------------------------
		#
		# Texture2DArray exige que todas as imagens tenham
		# a mesma configuração de mipmaps.
		#
		# Aqui estamos escolhendo NÃO usar mipmaps.
		#

		if img.has_mipmaps():

			print("Removendo mipmaps...")

			img.clear_mipmaps()


		# -----------------------------------------------------
		# CONVERTE PARA RGBA8
		# -----------------------------------------------------
		#
		# convert() retorna void no Godot 4.
		# Portanto NÃO fazemos:
		#
		# img = img.convert(...)
		#
		# Apenas chamamos convert().
		#

		img.convert(Image.FORMAT_RGBA8)


		print("Formato final: ", img.get_format())
		print("Mipmaps final: ", img.has_mipmaps())
		print("Tamanho final: ", img.get_size())


		# -----------------------------------------------------
		# DEFINE O TAMANHO DE REFERÊNCIA
		# -----------------------------------------------------

		if reference_size == Vector2i(-1, -1):

			reference_size = img.get_size()

			print(
				"Tamanho de referência definido como: ",
				reference_size
			)


		# -----------------------------------------------------
		# VERIFICA O TAMANHO
		# -----------------------------------------------------

		if img.get_size() != reference_size:

			printerr(
				"IGNORADA: tamanho diferente!"
			)

			printerr(
				"Arquivo: ",
				path.get_file()
			)

			printerr(
				"Esperado: ",
				reference_size
			)

			printerr(
				"Encontrado: ",
				img.get_size()
			)

			continue


		# -----------------------------------------------------
		# VERIFICA O FORMATO
		# -----------------------------------------------------

		if img.get_format() != Image.FORMAT_RGBA8:

			printerr(
				"IGNORADA: formato ainda não é RGBA8: ",
				path.get_file()
			)

			continue


		# -----------------------------------------------------
		# VERIFICA MIPMAPS
		# -----------------------------------------------------

		if img.has_mipmaps():

			printerr(
				"IGNORADA: ainda possui mipmaps: ",
				path.get_file()
			)

			continue


		# -----------------------------------------------------
		# ADICIONA AO ARRAY
		# -----------------------------------------------------

		images.append(img)
		valid_names.append(
			path.get_file().get_basename()
		)

		print("")
		print("OK: Adicionada ", path.get_file())


	print("")
	print("========================================")
	print(" PROCESSAMENTO TERMINADO")
	print("========================================")
	print("")


	# ---------------------------------------------------------
	# VERIFICA SE TEM IMAGENS
	# ---------------------------------------------------------

	if images.is_empty():

		printerr(
			"ERRO: Nenhuma imagem válida foi encontrada."
		)

		return


	print(
		"Imagens válidas: ",
		images.size()
	)

	print(
		"Tamanho: ",
		reference_size
	)

	print(
		"Formato: RGBA8"
	)

	print(
		"Mipmaps: NÃO"
	)

	print("")


	# ---------------------------------------------------------
	# VERIFICA TODAS AS IMAGENS UMA ÚLTIMA VEZ
	# ---------------------------------------------------------

	for i in range(images.size()):

		var img: Image = images[i]

		if img.get_size() != reference_size:

			printerr(
				"ERRO FINAL: tamanho diferente em ",
				valid_names[i]
			)

			return


		if img.get_format() != Image.FORMAT_RGBA8:

			printerr(
				"ERRO FINAL: formato diferente em ",
				valid_names[i]
			)

			return


		if img.has_mipmaps():

			printerr(
				"ERRO FINAL: mipmaps encontrados em ",
				valid_names[i]
			)

			return


	# ---------------------------------------------------------
	# CRIA TEXTURE2DARRAY
	# ---------------------------------------------------------

	print("Criando Texture2DArray...")

	var tex_array := Texture2DArray.new()

	var error := tex_array.create_from_images(images)


	if error != OK:

		printerr(
			"ERRO ao criar Texture2DArray."
		)

		printerr(
			"Código de erro: ",
			error
		)

		return


	print("Texture2DArray criado com sucesso!")


	# ---------------------------------------------------------
	# SALVA TEXTURE2DARRAY
	# ---------------------------------------------------------

	print("")
	print("Salvando: ", OUTPUT_PATH)

	var save_error := ResourceSaver.save(
		tex_array,
		OUTPUT_PATH
	)


	if save_error != OK:

		printerr(
			"ERRO ao salvar Texture2DArray."
		)

		printerr(
			"Código: ",
			save_error
		)

		return


	print(
		"Texture2DArray salvo com sucesso!"
	)


	# ---------------------------------------------------------
	# CRIA JSON
	# ---------------------------------------------------------

	print("")
	print("Criando mapeamento JSON...")


	var json_map := {}


	# IMPORTANTE:
	#
	# valid_names possui exatamente a mesma ordem
	# usada em images.
	#
	# Portanto:
	#
	# images[0] -> valid_names[0]
	# images[1] -> valid_names[1]
	# images[2] -> valid_names[2]
	#
	# etc.
	#

	for i in range(valid_names.size()):

		var texture_name := valid_names[i]

		json_map[texture_name] = i


	# ---------------------------------------------------------
	# CONVERTE PARA JSON
	# ---------------------------------------------------------

	var json_string := JSON.stringify(
		json_map,
		"\t"
	)


	# ---------------------------------------------------------
	# SALVA JSON
	# ---------------------------------------------------------

	var file := FileAccess.open(
		JSON_PATH,
		FileAccess.WRITE
	)


	if file == null:

		printerr(
			"ERRO: Não foi possível criar ",
			JSON_PATH
		)

		return


	file.store_string(json_string)
	file.close()


	# ---------------------------------------------------------
	# FINAL
	# ---------------------------------------------------------

	print("")
	print("========================================")
	print(" SUCESSO!")
	print("========================================")
	print("")
	print(
		"Texture2DArray: ",
		OUTPUT_PATH
	)
	print(
		"JSON: ",
		JSON_PATH
	)
	print(
		"Quantidade de layers: ",
		images.size()
	)
	print(
		"Tamanho: ",
		reference_size
	)
	print("")

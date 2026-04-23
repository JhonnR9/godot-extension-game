extends TopDownCharacter2D

func _process(_delta: float) -> void:
	add_input(Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down"))

extends DirectionalLight3D

@export var duracao_dia: float = 120.0
@export_range(0.0, 24.0) var hora: float = 8.0


var tempo: float


func _process(delta: float) -> void:
	tempo += delta
	if tempo >= duracao_dia:
		tempo -= duracao_dia

	hora = tempo / duracao_dia * 24.0

	var angulo := (hora / 24.0) * 360.0 - 90.0
	rotation_degrees.x = angulo

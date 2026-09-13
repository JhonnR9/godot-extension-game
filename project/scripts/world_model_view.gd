extends VBoxContainer
class_name WorldModelView

@export var world_name: Label
@export var last_update: Label

@onready var panel_container: PanelContainer = $PanelContainer

var id: int


func _ready() -> void:
	panel_container.selected.connect(_on_panel_selected)


func _on_panel_selected() -> void:
	GameSession.selected_world_id = id
	print("Mundo selecionado: ", id)

extends PanelContainer

@export var style_normal: StyleBox
@export var style_hover: StyleBox
@export var style_focus: StyleBox
@export var style_selected: StyleBox

static var selected_item: PanelContainer = null

var is_hovered: bool = false


func _ready():
	mouse_filter = Control.MOUSE_FILTER_STOP

	mouse_entered.connect(_on_mouse_entered)
	mouse_exited.connect(_on_mouse_exited)

	focus_entered.connect(_on_focus_entered)
	focus_exited.connect(_on_focus_exited)
	

	_update_style()

func _gui_input(event):
	if event is InputEventMouseButton and event.pressed:
		if event.button_index == MOUSE_BUTTON_LEFT:
			_select_this()


func _select_this():
	if selected_item == self:
		return

	if selected_item != null:
		var old_item = selected_item
		selected_item = null 
		old_item._update_style()

	selected_item = self
	_update_style() 


func _set_selected(value: bool):
	_update_style()


func _on_mouse_entered():
	is_hovered = true
	_update_style()


func _on_mouse_exited():
	is_hovered = false
	_update_style()


func _on_focus_entered():
	_update_style()


func _on_focus_exited():
	_update_style()

func _update_style():
	var sb: StyleBox = style_normal

	if selected_item == self and style_selected:
		sb = style_selected
	elif has_focus() and style_focus:
		sb = style_focus
	elif is_hovered and style_hover:
		sb = style_hover

	add_theme_stylebox_override("panel", sb)

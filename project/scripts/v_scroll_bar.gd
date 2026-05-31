extends VScrollBar

@export var scrol: ScrollContainer

	
func _process(delta: float) -> void:
	value = scrol.scroll_vertical

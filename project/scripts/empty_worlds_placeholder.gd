extends PanelContainer


func _on_world_selector_no_save_detected() -> void:
	visible=true;

func _on_world_selector_save_detected(_id) -> void:
	visible=false

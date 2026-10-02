extends Node

const SETTINGS_PATH := "user://graphics_settings.cfg"
const AA_OFF := 0
const AA_FXAA := 1
const AA_MSAA_2X := 2
const AA_MSAA_4X := 3
const UPSCALE_NATIVE := 0
const UPSCALE_FSR1_QUALITY := 1
const UPSCALE_FSR1_BALANCED := 2
const UPSCALE_FSR1_PERFORMANCE := 3
const UPSCALE_FSR2_QUALITY := 4
const UPSCALE_FSR2_BALANCED := 5
const UPSCALE_FSR2_PERFORMANCE := 6

var aa_mode := AA_OFF
var upscale_mode := UPSCALE_NATIVE
var ssao_enabled := false

func _ready() -> void:
	_load_settings()
	_apply_viewport()
	get_tree().scene_changed.connect(_on_scene_changed)

func supports_fxaa() -> bool:
	return RenderingServer.get_current_rendering_method() != "gl_compatibility"

func supports_fsr2() -> bool:
	return RenderingServer.get_current_rendering_method() == "forward_plus"

func supports_fsr1() -> bool:
	return RenderingServer.get_current_rendering_method() == "forward_plus"

func set_aa_mode(value: int) -> void:
	if value < AA_OFF or value > AA_MSAA_4X:
		return
	aa_mode = value
	_apply_viewport()
	_save_settings()

func set_upscale_mode(value: int) -> void:
	if value < UPSCALE_NATIVE or value > UPSCALE_FSR2_PERFORMANCE:
		return
	upscale_mode = value
	_apply_viewport()
	_save_settings()

func set_ssao_enabled(value: bool) -> void:
	ssao_enabled = value
	_apply_environment_to_current_scene()
	_save_settings()

func apply_environment(environment: Environment) -> void:
	if environment == null:
		return
	environment.ssao_enabled = ssao_enabled and supports_ssao()
	if environment.ssao_enabled:
		environment.ssao_intensity = 0.6
		environment.ssao_radius = 1.5

func supports_ssao() -> bool:
	return RenderingServer.get_current_rendering_method() != "mobile"

func _on_scene_changed() -> void:
	_apply_environment_to_current_scene()

func _apply_environment_to_current_scene() -> void:
	var scene := get_tree().current_scene
	if scene == null:
		return
	var world_environment := scene.find_child("WorldEnvironment", true, false) as WorldEnvironment
	if world_environment != null:
		apply_environment(world_environment.environment)

func _apply_viewport() -> void:
	var viewport := get_tree().root
	if viewport == null:
		return
	viewport.screen_space_aa = Viewport.SCREEN_SPACE_AA_FXAA if aa_mode == AA_FXAA and supports_fxaa() else Viewport.SCREEN_SPACE_AA_DISABLED
	viewport.msaa_3d = Viewport.MSAA_2X if aa_mode == AA_MSAA_2X else (Viewport.MSAA_4X if aa_mode == AA_MSAA_4X else Viewport.MSAA_DISABLED)
	viewport.scaling_3d_mode = Viewport.SCALING_3D_MODE_BILINEAR
	viewport.scaling_3d_scale = 1.0
	if upscale_mode >= UPSCALE_FSR2_QUALITY and supports_fsr2():
		viewport.scaling_3d_mode = Viewport.SCALING_3D_MODE_FSR2
		viewport.scaling_3d_scale = 0.77 if upscale_mode == UPSCALE_FSR2_QUALITY else (0.59 if upscale_mode == UPSCALE_FSR2_BALANCED else 0.5)
	elif upscale_mode >= UPSCALE_FSR1_QUALITY and upscale_mode <= UPSCALE_FSR1_PERFORMANCE and supports_fsr1():
		viewport.scaling_3d_mode = Viewport.SCALING_3D_MODE_FSR
		viewport.scaling_3d_scale = 0.77 if upscale_mode == UPSCALE_FSR1_QUALITY else (0.59 if upscale_mode == UPSCALE_FSR1_BALANCED else 0.5)

func _load_settings() -> void:
	var config := ConfigFile.new()
	if config.load(SETTINGS_PATH) != OK:
		return
	aa_mode = clampi(int(config.get_value("graphics", "aa_mode", AA_OFF)), AA_OFF, AA_MSAA_4X)
	upscale_mode = clampi(int(config.get_value("graphics", "upscale_mode", UPSCALE_NATIVE)), UPSCALE_NATIVE, UPSCALE_FSR2_PERFORMANCE)
	ssao_enabled = bool(config.get_value("graphics", "ssao_enabled", false))

func _save_settings() -> void:
	var config := ConfigFile.new()
	config.set_value("graphics", "aa_mode", aa_mode)
	config.set_value("graphics", "upscale_mode", upscale_mode)
	config.set_value("graphics", "ssao_enabled", ssao_enabled)
	var error := config.save(SETTINGS_PATH)
	if error != OK:
		push_warning("Could not save graphics settings: %s" % error_string(error))

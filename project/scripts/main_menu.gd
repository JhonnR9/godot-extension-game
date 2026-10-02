extends Control

const PREVIEW_SEED := 42
const ORBIT_RADIUS := 24.0
const ORBIT_SPEED := 0.025
var orbit_angle := 0.0
var orbit_center := Vector3.ZERO
@onready var preview: VoxelAPI = $Background/Viewport/Backdrop/VoxelAPI
@onready var preview_camera: Camera3D = $Background/Viewport/Backdrop/Camera3D


func _ready():
	_setup_background()
	$HUD/Buttons/SinglePlayer.pressed.connect(_on_singleplayer_pressed)
	$HUD/Buttons/Quit.pressed.connect(_on_quit_pressed)
	$HUD/Buttons/Settings.pressed.connect(_on_settings_pressed)

func _on_singleplayer_pressed():
	get_tree().change_scene_to_file("res://scenes/world_selector.tscn")

func _on_quit_pressed():
	get_tree().quit()

func _on_settings_pressed() -> void:
	$SettingsPanel.open_from($HUD, null, $HUD/Buttons/Settings)

func _setup_background() -> void:
	var sky_material := ProceduralSkyMaterial.new()
	sky_material.sky_top_color = Color(0.18, 0.38, 0.65)
	sky_material.sky_horizon_color = Color(0.84, 0.87, 0.88)
	var sky := Sky.new()
	sky.sky_material = sky_material
	var environment := Environment.new()
	environment.background_mode = Environment.BG_SKY
	environment.sky = sky
	environment.ambient_light_source = Environment.AMBIENT_SOURCE_SKY
	environment.ambient_light_energy = 0.65
	environment.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	$Background/Viewport/Backdrop/WorldEnvironment.environment = environment
	preview.set_render_settings({"render_distance": 4, "vertical_render_distance": 2,
		"distance_fog_enabled": true, "distance_fog_start_percent": 65,
		"vsync": VoxelAPI.get_default_render_settings().get("vsync", true)})
	preview.set_pipeline_settings({"finalize_budget_ms": 1.0})
	preview.set_focus_position(Vector3(8, 48, 8))
	preview.start_preview(PREVIEW_SEED)
	var terrain := preview.sample_terrain_column(Vector2i(8, 8))
	orbit_center = Vector3(8, maxf(float(terrain.height), float(terrain.water_level)) + 6, 8)
	preview.set_focus_position(orbit_center)
	_update_camera()

func _process(delta: float) -> void:
	if not preview.is_initial_loading():
		orbit_angle = fmod(orbit_angle + delta * ORBIT_SPEED, TAU)
		_update_camera()

func _update_camera() -> void:
	preview_camera.position = orbit_center + Vector3(cos(orbit_angle) * ORBIT_RADIUS, 13, sin(orbit_angle) * ORBIT_RADIUS)
	preview_camera.look_at(orbit_center)

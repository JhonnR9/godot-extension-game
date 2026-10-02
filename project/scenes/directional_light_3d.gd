extends DirectionalLight3D

# Full cycle: 13 minutes above the horizon, 7 minutes below it.
@export_range(60.0, 7200.0, 30.0) var duracao_dia: float = 1200.0
@export_range(0.1, 0.9, 0.01) var proporcao_dia: float = 0.65
@export_range(0.0, 24.0) var hora: float = 8.0

@export_range(0.0, 0.15, 0.005) var luz_ambiente_noturna: float = 0.025
@export_range(0.0, 0.1, 0.005) var luz_da_lua: float = 0.015

var tempo: float = 0.0
var _environment: Environment
var _sky_material: ProceduralSkyMaterial
var _moon: DirectionalLight3D
var _visual_timer: float = 0.0

func _ready() -> void:
	var world_environment := get_node_or_null("../WorldEnvironment") as WorldEnvironment
	if world_environment != null and world_environment.environment != null:
		_environment = world_environment.environment.duplicate(true)
		world_environment.environment = _environment
		_environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
		_environment.ambient_light_sky_contribution = 0.0
		_environment.reflected_light_source = Environment.REFLECTION_SOURCE_DISABLED
		if _environment.sky != null:
			_sky_material = _environment.sky.sky_material as ProceduralSkyMaterial
			if _sky_material != null:
				_sky_material.sun_angle_max = 2.0
				_sky_material.sun_curve = 0.07
	var display_settings := get_node_or_null("/root/DisplaySettings")
	if _environment != null and display_settings != null:
		var graphics: Object = display_settings.get("graphics")
		if graphics != null:
			graphics.call("apply_environment", _environment)
	_moon = DirectionalLight3D.new()
	_moon.name = "MoonLight"
	_moon.light_color = Color(0.43, 0.53, 0.78)
	_moon.shadow_enabled = false
	add_child(_moon)
	set_hour(hora)

func set_hour(value: float) -> void:
	hora = fposmod(value, 24.0)
	var daylight_span := clampf(proporcao_dia, 0.1, 0.9)
	var offset := fposmod(hora - 6.0, 24.0)
	var phase := offset / 12.0 * daylight_span if offset < 12.0 else daylight_span + (offset - 12.0) / 12.0 * (1.0 - daylight_span)
	tempo = phase * maxf(duracao_dia, 60.0)
	_update_lighting()

func _process(delta: float) -> void:
	var duration := maxf(duracao_dia, 60.0)
	var daylight_span := clampf(proporcao_dia, 0.1, 0.9)
	tempo = fposmod(tempo + delta, duration)
	var phase := tempo / duration
	hora = fposmod(6.0 + (phase / daylight_span * 12.0 if phase < daylight_span else 12.0 + (phase - daylight_span) / (1.0 - daylight_span) * 12.0), 24.0)
	_visual_timer += delta
	# Avoid rebuilding sky/environment parameters on every render frame.
	if _visual_timer >= 0.1:
		_visual_timer = fmod(_visual_timer, 0.1)
		_update_lighting()

func _update_lighting() -> void:
	var elevation := sin((hora - 6.0) / 24.0 * TAU)
	var daylight := smoothstep(-0.08, 0.28, elevation)
	var twilight := smoothstep(-0.20, -0.01, elevation) * (1.0 - smoothstep(0.02, 0.45, elevation))
	# At noon the sun points down, rather than lighting the terrain from below.
	rotation_degrees = Vector3(-(hora - 6.0) * 15.0, -25.0, 0.0)
	light_energy = smoothstep(-0.02, 0.35, elevation) * 1.05
	light_color = Color(1.0, 0.96, 0.88).lerp(Color(1.0, 0.57, 0.30), twilight * 0.8)
	if _moon != null:
		_moon.global_rotation = global_rotation + Vector3(PI, 0.0, 0.0)
		_moon.light_energy = smoothstep(0.05, 0.55, -elevation) * luz_da_lua
	RenderingServer.global_shader_parameter_set("world_daylight", daylight)
	if _environment == null:
		return
	_environment.ambient_light_color = Color(0.24, 0.30, 0.46).lerp(Color(0.78, 0.84, 0.94), daylight)
	_environment.ambient_light_energy = lerpf(luz_ambiente_noturna, 0.45, daylight)
	_environment.fog_light_color = Color(0.008, 0.012, 0.024).lerp(Color(0.57, 0.68, 0.81), daylight).lerp(Color(0.68, 0.40, 0.29), twilight * 0.35)
	if _sky_material != null:
		_sky_material.sky_top_color = Color(0.008, 0.013, 0.032).lerp(Color(0.22, 0.43, 0.73), daylight)
		_sky_material.sky_horizon_color = Color(0.012, 0.018, 0.038).lerp(Color(0.70, 0.81, 0.94), daylight).lerp(Color(0.94, 0.46, 0.25), twilight * 0.7)
		_sky_material.ground_bottom_color = Color(0.008, 0.011, 0.019).lerp(Color(0.16, 0.20, 0.25), daylight)
		_sky_material.ground_horizon_color = _sky_material.sky_horizon_color

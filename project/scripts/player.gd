extends CharacterBody3D

enum Mode { WALK, FLY, NOCLIP }

@export var speed := 5.0
@export var fly_speed := 8.0
@export var noclip_speed := 12.0
@export var jump_force := 8.0
@export var mouse_sensitivity := 0.003

@export var head: Node3D
@export var camera: Camera3D
@export var collision_shape: CollisionShape3D
@export var world: VoxelAPI

var current_mode := Mode.WALK
var double_tap_timer := 0.0
const DOUBLE_TAP_TIME := 0.3

var yaw := 0.0
var pitch := 0.0
var selected_block_id := 1
var underwater_amount := 0.0
var underwater_overlay: ColorRect
var underwater_material: ShaderMaterial
var ocean_ambience: AudioStreamPlayer
var water_splash: AudioStreamPlayer
var footstep_player: AudioStreamPlayer
var footstep_streams: Dictionary = {}
var footstep_timer := 0.0
var was_underwater := false
var ocean_audio_amount := 0.0

func _ready() -> void:
	Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
	camera.current = true
	_create_underwater_overlay()
	_create_water_audio()
	if world:
		world.set_focus_node(self)

func _process(delta: float) -> void:
	var in_water := world != null and camera != null and world.is_water_at(camera.global_position)
	var target_amount := 1.0 if in_water else 0.0
	if in_water and not was_underwater and water_splash:
		water_splash.play()
	was_underwater = in_water

	var in_ocean := world != null and camera != null and world.is_ocean_at(camera.global_position)
	ocean_audio_amount = move_toward(ocean_audio_amount, 1.0 if in_ocean else 0.0, delta * 0.35)
	if ocean_ambience:
		ocean_ambience.volume_db = lerp(-60.0, -17.0, ocean_audio_amount)

	underwater_amount = move_toward(underwater_amount, target_amount, delta * 2.5)
	if underwater_material:
		underwater_material.set_shader_parameter("underwater_amount", underwater_amount)

func _create_water_audio() -> void:
	ocean_ambience = AudioStreamPlayer.new()
	ocean_ambience.name = "OceanAmbience"
	var ambience := load("res://audio/ocean_ambience.wav") as AudioStreamWAV
	if ambience:
		ambience.loop_mode = AudioStreamWAV.LOOP_FORWARD
		ocean_ambience.stream = ambience
		ocean_ambience.volume_db = -60.0
		add_child(ocean_ambience)
		ocean_ambience.play()

	water_splash = AudioStreamPlayer.new()
	water_splash.name = "WaterSplash"
	water_splash.stream = load("res://audio/water_splash.wav")
	water_splash.volume_db = -5.0
	add_child(water_splash)

	footstep_player = AudioStreamPlayer.new()
	footstep_player.name = "Footsteps"
	footstep_player.volume_db = -7.0
	add_child(footstep_player)
	footstep_streams = {
		"soft": load("res://audio/footstep_grass.wav"),
		"stone": load("res://audio/footstep_stone.wav"),
		"sand": load("res://audio/footstep_sand.wav"),
		"wood": load("res://audio/footstep_wood.wav"),
	}

func _create_underwater_overlay() -> void:
	var canvas_layer := CanvasLayer.new()
	canvas_layer.name = "UnderwaterPostProcess"
	canvas_layer.layer = 0
	add_child(canvas_layer)

	underwater_overlay = ColorRect.new()
	underwater_overlay.name = "ColorGrade"
	underwater_overlay.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	underwater_overlay.mouse_filter = Control.MOUSE_FILTER_IGNORE
	canvas_layer.add_child(underwater_overlay)

	underwater_material = ShaderMaterial.new()
	underwater_material.shader = load("res://shaders/underwater.gdshader")
	underwater_material.set_shader_parameter("underwater_amount", 0.0)
	underwater_overlay.material = underwater_material

func restore_rotation(saved_yaw: float, saved_pitch: float) -> void:
	yaw = saved_yaw
	pitch = clamp(saved_pitch, -PI / 2.0, PI / 2.0)
	rotation = Vector3(0.0, yaw, 0.0)
	head.rotation = Vector3(pitch, 0.0, 0.0)

func _physics_process(delta: float) -> void:
	if Input.is_action_just_pressed("save") and world:
		world.save_world()

	var current_velocity := velocity

	if double_tap_timer > 0:
		double_tap_timer -= delta

	if Input.is_action_just_pressed("jump"):
		if double_tap_timer > 0:
			current_mode = Mode.FLY if current_mode == Mode.WALK else Mode.WALK
			double_tap_timer = 0
		else:
			double_tap_timer = DOUBLE_TAP_TIME

	if Input.is_action_just_pressed("toggle_mode"):
		current_mode = Mode.WALK if current_mode == Mode.NOCLIP else Mode.NOCLIP

	collision_shape.disabled = (current_mode == Mode.NOCLIP)

	var direction := Vector3.ZERO
	var forward := -transform.basis.z
	var right := transform.basis.x
	forward.y = 0
	right.y = 0
	forward = forward.normalized()
	right = right.normalized()

	if Input.is_action_pressed("move_forward"):
		direction += forward
	if Input.is_action_pressed("move_backward"):
		direction -= forward
	if Input.is_action_pressed("move_left"):
		direction -= right
	if Input.is_action_pressed("move_right"):
		direction += right

	if current_mode == Mode.WALK:
		if not is_on_floor():
			current_velocity.y -= ProjectSettings.get_setting("physics/3d/default_gravity") * delta

		if Input.is_action_just_pressed("jump") and is_on_floor():
			current_velocity.y = jump_force

		if direction.length() > 0:
			direction = direction.normalized()
			current_velocity.x = direction.x * speed
			current_velocity.z = direction.z * speed
		else:
			current_velocity.x = lerp(current_velocity.x, 0.0, 0.15)
			current_velocity.z = lerp(current_velocity.z, 0.0, 0.15)
	else:
		if Input.is_action_pressed("move_up"):
			direction.y += 1
		if Input.is_action_pressed("move_down"):
			direction.y -= 1

		var speed_to_use := fly_speed if current_mode == Mode.FLY else noclip_speed

		if direction.length() > 0:
			current_velocity = direction.normalized() * speed_to_use
		else:
			current_velocity = current_velocity.lerp(Vector3.ZERO, 0.1)

	velocity = current_velocity
	move_and_slide()

	if is_on_ceiling():
		velocity.y = 0

	apply_floor_snap()
	_update_footsteps(delta)

func _update_footsteps(delta: float) -> void:
	footstep_timer = maxf(0.0, footstep_timer - delta)
	if current_mode != Mode.WALK or not is_on_floor() or world == null or footstep_timer > 0.0:
		return
	if Vector2(velocity.x, velocity.z).length() < 0.6:
		return

	var ground_type := world.get_block_type_at(global_position + Vector3.DOWN * 1.02)
	var sound_key := "soft"
	match ground_type:
		3, 5:
			sound_key = "stone"
		11, 12:
			sound_key = "sand"
		4, 8:
			sound_key = "wood"
	var sound := footstep_streams.get(sound_key) as AudioStream
	if sound and footstep_player:
		footstep_player.stream = sound
		footstep_player.pitch_scale = randf_range(0.92, 1.08)
		footstep_player.play()
	footstep_timer = clampf(0.5 - Vector2(velocity.x, velocity.z).length() * 0.025, 0.3, 0.5)

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion:
		yaw -= event.relative.x * mouse_sensitivity
		pitch -= event.relative.y * mouse_sensitivity
		pitch = clamp(pitch, -PI/2, PI/2)
		rotation = Vector3(0, yaw, 0)
		head.rotation = Vector3(pitch, 0, 0)

	if event is InputEventKey and event.pressed and not event.is_echo():
		if event.keycode >= KEY_0 and event.keycode <= KEY_9:
			selected_block_id = 10 if event.keycode == KEY_0 else event.keycode - KEY_0

	if event is InputEventMouseButton and event.pressed:
		var fov := camera.fov
		if event.button_index == MOUSE_BUTTON_WHEEL_UP:
			fov -= 2.0
		if event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			fov += 2.0
		camera.fov = clamp(fov, 20.0, 100.0)

		if event.button_index == MOUSE_BUTTON_LEFT:
			var hit := raycast_block(8.0)
			if not hit.is_empty():
				var pos: Vector3 = hit["position"]
				var normal: Vector3 = hit["normal"]
				pos -= normal * 0.01
				if world:
					world.break_block(pos)

		if event.button_index == MOUSE_BUTTON_RIGHT:
			var hit := raycast_block(8.0)
			if not hit.is_empty():
				var pos: Vector3 = hit["position"]
				var normal: Vector3 = hit["normal"]
				pos += normal * 0.01
				if world:
					world.set_block(pos, selected_block_id)

func raycast_block(distance: float) -> Dictionary:
	var from := camera.global_position
	var to := from + (-camera.global_transform.basis.z) * distance
	var query := PhysicsRayQueryParameters3D.create(from, to)
	query.exclude = [get_rid()]
	var space_state := get_world_3d().direct_space_state
	return space_state.intersect_ray(query)

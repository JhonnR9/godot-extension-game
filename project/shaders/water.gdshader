shader_type spatial;
render_mode unshaded, blend_mix, depth_draw_opaque, cull_back;

global uniform float world_daylight;

uniform sampler2D screen_texture : hint_screen_texture, repeat_disable, filter_nearest;

group_uniforms water;
uniform vec4 waterColor : source_color = vec4(0.117, 0.27, 0.58, 1.0);
uniform float colorCorection : hint_range(0.0, 1.0, 0.01) = 0.35;

group_uniforms water_waves;
uniform float distortionForce : hint_range(0.0, 0.1, 0.001) = 0.01;
uniform float WDBrightness : hint_range(0.0, 3.0, 0.05) = 1.5;
uniform float WDFreq : hint_range(0.2, 0.9, 0.05) = 0.6;
uniform float WDSize : hint_range(0.6, 1.2, 0.05) = 0.9;
uniform float WDSpeed : hint_range(1.0, 20.0, 0.05) = 4.0;
uniform vec2 tiling = vec2(1.0);
uniform vec2 offSetSpeed = vec2(0.1);

group_uniforms BG_distortion;
uniform float backGroundDirX : hint_range(-0.1, 0.1) = 0.01;
uniform float backGroundDirY : hint_range(-0.1, 0.1) = 0.01;

varying vec3 world_position;

void vertex() {
	world_position = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
}

float hash_noise(vec2 p) {
	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float value_noise(vec2 p) {
	vec2 cell = floor(p);
	vec2 local = fract(p);
	local = local * local * (3.0 - 2.0 * local);
	float a = hash_noise(cell);
	float b = hash_noise(cell + vec2(1.0, 0.0));
	float c = hash_noise(cell + vec2(0.0, 1.0));
	float d = hash_noise(cell + vec2(1.0, 1.0));
	return mix(mix(a, b, local.x), mix(c, d, local.x), local.y);
}

void fragment() {
	vec2 base_uv = world_position.xz * tiling;
	float distortion_noise = value_noise(base_uv * 1.7 + offSetSpeed * TIME);
	vec2 water_uv = base_uv + offSetSpeed * TIME;
	water_uv.y += cos(TIME * min(1.0, offSetSpeed.y)) * 0.01;
	water_uv += (distortion_noise - 0.5) * distortionForce * WDSpeed;

	float wave_noise = value_noise(water_uv * 2.0);
	float wave_intensity = smoothstep(WDFreq, WDSize, wave_noise);
	float water_light = mix(0.12, 1.0, world_daylight);
	vec3 lit_water_color = waterColor.rgb * water_light;
	vec3 animated_water = (waterColor.rgb + wave_intensity * vec3(WDBrightness * 0.12)) * water_light;

	vec2 background_uv = SCREEN_UV;
	background_uv.x += (distortion_noise - 0.5) * backGroundDirX;
	background_uv.y += (distortion_noise - 0.5) * backGroundDirY;
	vec3 refracted_background = texture(screen_texture, background_uv).rgb;
	vec3 water_color = mix(refracted_background, animated_water, 0.2);
	water_color = mix(water_color, lit_water_color, colorCorection);

	ALBEDO = water_color;
	ALPHA = 0.99;
}

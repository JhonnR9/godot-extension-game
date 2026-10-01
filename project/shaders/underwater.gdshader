shader_type canvas_item;

uniform sampler2D screen_texture : hint_screen_texture, filter_linear_mipmap;
uniform float underwater_amount : hint_range(0.0, 1.0) = 0.0;

void fragment() {
	vec2 wave = vec2(
		sin(SCREEN_UV.y * 34.0 + TIME * 1.7),
		cos(SCREEN_UV.x * 27.0 + TIME * 1.2)
	) * 0.0018 * underwater_amount;
	vec3 scene_color = texture(screen_texture, SCREEN_UV + wave).rgb;
	float luminance = dot(scene_color, vec3(0.299, 0.587, 0.114));
	vec3 muted_color = mix(vec3(luminance), scene_color, 0.78);
	vec3 underwater_color = muted_color * vec3(0.48, 0.82, 0.96) + vec3(0.0, 0.035, 0.065);
	COLOR = vec4(mix(scene_color, underwater_color, underwater_amount), 1.0);
}

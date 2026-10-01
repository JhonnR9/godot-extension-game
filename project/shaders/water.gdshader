shader_type spatial;
render_mode blend_mix, depth_draw_opaque, cull_back;

void fragment() {
	float ripple = sin(UV.x * 2.0 + TIME * 0.8) * cos(UV.y * 2.0 + TIME * 0.6);
	ALBEDO = mix(vec3(0.025, 0.34, 0.58), vec3(0.04, 0.52, 0.72), ripple * 0.12 + 0.5);
	ALPHA = 0.72;
	ROUGHNESS = 0.16;
}

shader_type spatial;
render_mode depth_draw_opaque, cull_back;

uniform sampler2DArray albedo_array : source_color, filter_nearest;

varying flat float v_layer;

void vertex() {
    v_layer = float(CUSTOM0.x);
}
void fragment() {
	vec2 block_uv = UV;
	// The cactus side texture has a black source pixel row on its upper edge.
	// Keep sampling inside the image so that row cannot form a seam on blocks.
	if (v_layer < 0.5) {
		block_uv = fract(UV);
		block_uv.y = mix(1.5 / 32.0, 31.5 / 32.0, block_uv.y);
		block_uv.x = mix(0.5 / 32.0, 31.5 / 32.0, block_uv.x);
	}
	vec3 uvw = vec3(block_uv, v_layer);
    vec4 tex = texture(albedo_array, uvw);

    ALBEDO = tex.rgb * COLOR.rgb;
    // Alpha scissor keeps leaf pixels crisp while the rest of the chunk stays opaque.
    ALPHA = tex.a;
    ALPHA_SCISSOR_THRESHOLD = 0.5;
}

shader_type spatial;
render_mode depth_draw_opaque, cull_back;

// Voxel AO is baked into vertex alpha, independently of biome tint.
uniform float ao_strength : hint_range(0.0, 1.0, 0.05) = 0.35;
uniform float ao_light_affect : hint_range(0.0, 1.0, 0.05) = 0.25;

uniform sampler2DArray albedo_array : source_color, filter_nearest_mipmap_anisotropic;

// Only these atlas layers may emit. Saturated foliage and tinted rock do not.
uniform int iron_ore_layer = -1;
uniform int diamond_ore_layer = -1;
uniform float iron_emission : hint_range(0.0, 3.0, 0.05) = 0.65;
uniform float diamond_emission : hint_range(0.0, 3.0, 0.05) = 1.25;

varying flat float v_layer;

void vertex() {
    v_layer = float(CUSTOM0.x);
}
void fragment() {
	vec2 block_uv = UV;
	vec2 uv_dx = dFdx(UV);
	vec2 uv_dy = dFdy(UV);
	// The cactus side texture has a black source pixel row on its upper edge.
	// Keep sampling inside the image so that row cannot form a seam on blocks.
	if (v_layer < 0.5) {
		block_uv = fract(UV);
		block_uv.y = mix(1.5 / 32.0, 31.5 / 32.0, block_uv.y);
		block_uv.x = mix(0.5 / 32.0, 31.5 / 32.0, block_uv.x);
		// Derivatives from the unwrapped UV avoid blurry mip seams at fract().
		uv_dx *= vec2(31.0, 30.0) / 32.0;
		uv_dy *= vec2(31.0, 30.0) / 32.0;
	}
	vec3 uvw = vec3(block_uv, v_layer);
    vec4 tex = textureGrad(albedo_array, uvw, uv_dx, uv_dy);

    ALBEDO = tex.rgb * COLOR.rgb;
    AO = mix(1.0, clamp(COLOR.a, 0.0, 1.0), ao_strength);
    AO_LIGHT_AFFECT = ao_light_affect;
    EMISSION = vec3(0.0);
    // Analytical color masks in linear space leave the neutral host rock dark.
    // No extra texture fetch, light node, or additional material surface.
    if (abs(v_layer - float(iron_ore_layer)) < 0.5) {
        float inclusion = smoothstep(0.045, 0.16, tex.r - tex.b);
        EMISSION = tex.rgb * inclusion * iron_emission;
    } else if (abs(v_layer - float(diamond_ore_layer)) < 0.5) {
        float inclusion = smoothstep(0.025, 0.15, min(tex.g, tex.b) - tex.r);
        EMISSION = tex.rgb * inclusion * diamond_emission;
    }
    ROUGHNESS = 0.92;
    // Alpha scissor keeps leaf pixels crisp while the rest of the chunk stays opaque.
    ALPHA = tex.a;
    ALPHA_SCISSOR_THRESHOLD = 0.5;
}

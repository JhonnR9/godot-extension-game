shader_type spatial;
render_mode blend_mix, depth_draw_opaque, cull_back;

uniform sampler2DArray albedo_array : source_color, filter_nearest, repeat_enable;

uniform float light_steps = 3.0;
uniform float dark_intensity = 0.45;
uniform vec3 shadow_tint : source_color = vec3(0.82, 0.85, 0.96);

uniform vec3 fresnel_color : source_color = vec3(1.0);
uniform float fresnel_strength = 0.4;
uniform float fresnel_power = 6.0;

varying flat float v_layer;

void vertex() {
    v_layer = float(CUSTOM0.x);
}

void fragment() {
    vec4 tex = texture(albedo_array, vec3(UV, v_layer));

    ALBEDO = tex.rgb;
}

void light() {
    float NdotL = max(dot(NORMAL, normalize(LIGHT)), 0.0);

    float toon = floor(NdotL * light_steps) / light_steps;

    vec3 shadowed = ALBEDO * dark_intensity * shadow_tint;
    vec3 shaded = mix(shadowed, ALBEDO, toon);

    DIFFUSE_LIGHT = shaded * LIGHT_COLOR;
}
shader_type spatial;
render_mode blend_mix, depth_draw_opaque, cull_back;

uniform sampler2DArray albedo_array : source_color, filter_nearest;

uniform float light_steps = 4.0;
uniform float dark_intensity = 0.5;
uniform vec3 shadow_tint : source_color = vec3(0.85, 0.9, 1.0);

uniform vec3 fresnel_color : source_color = vec3(1.0);
uniform float fresnel_strength = 0.03;
uniform float fresnel_power = 2.0;

varying flat float v_layer;

void vertex() {
    v_layer = float(CUSTOM0.x);
}
void fragment() {
    vec3 uvw = vec3(UV, v_layer);
    vec4 tex = texture(albedo_array, uvw);

    ALBEDO = tex.rgb;
	
	float fresnel = pow(1.0 - dot(NORMAL, VIEW), fresnel_power);
	EMISSION += fresnel_color * fresnel * fresnel_strength;
}

shader_type spatial;
render_mode blend_mix, depth_draw_opaque, cull_back;

uniform sampler2DArray albedo_array : source_color, filter_nearest;

varying flat float v_layer;

void vertex() {
    v_layer = float(CUSTOM0.x);
}
void fragment() {
    vec3 uvw = vec3(UV, v_layer);
    vec4 tex = texture(albedo_array, uvw);

    ALBEDO = tex.rgb;
	
}

void light() {
    float ndotl = max(dot(NORMAL, LIGHT), 0.0);

    ndotl = step(0.5, ndotl);

    DIFFUSE_LIGHT += ndotl * ATTENUATION;
}

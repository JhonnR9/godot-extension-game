shader_type spatial;

render_mode diffuse_lambert, blend_mix, cull_disabled, specular_disabled, depth_prepass_alpha;


uniform sampler2D albedo_texture : source_color, filter_nearest;
uniform vec4 albedo_tint : source_color = vec4(1.0);

void fragment() {
    vec4 tex_color = texture(albedo_texture, UV);
    vec4 final_color = tex_color * albedo_tint;

    ALBEDO = final_color.rgb;
    ALPHA = final_color.a;
}

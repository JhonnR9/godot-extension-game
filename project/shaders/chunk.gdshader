shader_type spatial;

uniform sampler2D albedo_tex : source_color, repeat_enable, filter_nearest,hint_default_white ;
uniform float highlight_power = 1.0;

void fragment() {
    float uv_w = COLOR.r;
    float uv_h = COLOR.g;
	
	
    vec2 local_uv = (UV - .005) / 16.0;
    
    vec2 tiled_uv = fract(local_uv * vec2(uv_w, uv_h));
    
    vec2 final_uv = .005 + (tiled_uv * 16.0);
    
    vec4 tex = texture(albedo_tex, final_uv);

    
    // ... restante do código
    ALBEDO = tex.rgb;
    EMISSION = tex.rgb * highlight_power * pow(1.0 - clamp(dot(NORMAL, VIEW), 0.0, 1.0), 2.0);
}
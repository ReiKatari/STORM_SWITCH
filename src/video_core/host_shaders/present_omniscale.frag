// OmniScale HQ - High Quality Pixel-Art & 2D/3D Hybrid Resampling
#version 450

layout (location = 0) in vec2 frag_tex_coord;
layout (location = 0) out vec4 color;
layout (binding = 0) uniform sampler2D color_texture;

float rgb_to_luma(vec3 c) {
    return dot(c, vec3(0.299, 0.587, 0.114));
}

void main() {
    vec2 size = vec2(textureSize(color_texture, 0));
    vec2 inv_size = 1.0 / size;
    vec2 pos = frag_tex_coord * size;
    vec2 p = floor(pos);
    vec2 f = pos - p;

    vec2 uv_center = (p + 0.5) * inv_size;

    vec3 c  = texture(color_texture, uv_center).rgb;
    vec3 n  = texture(color_texture, uv_center + vec2(0.0, -inv_size.y)).rgb;
    vec3 s  = texture(color_texture, uv_center + vec2(0.0,  inv_size.y)).rgb;
    vec3 w  = texture(color_texture, uv_center + vec2(-inv_size.x, 0.0)).rgb;
    vec3 e  = texture(color_texture, uv_center + vec2( inv_size.x, 0.0)).rgb;

    vec3 nw = texture(color_texture, uv_center + vec2(-inv_size.x, -inv_size.y)).rgb;
    vec3 ne = texture(color_texture, uv_center + vec2( inv_size.x, -inv_size.y)).rgb;
    vec3 sw = texture(color_texture, uv_center + vec2(-inv_size.x,  inv_size.y)).rgb;
    vec3 se = texture(color_texture, uv_center + vec2( inv_size.x,  inv_size.y)).rgb;

    float lc = rgb_to_luma(c);
    float ln = rgb_to_luma(n);
    float ls = rgb_to_luma(s);
    float lw = rgb_to_luma(w);
    float le = rgb_to_luma(e);

    float grad_x = abs(le - lw);
    float grad_y = abs(ls - ln);

    vec2 fw = fwidth(pos);
    vec2 sub = clamp((f - 0.5) / max(fw, vec2(1e-4)) + 0.5, 0.0, 1.0);
    sub = sub * sub * (3.0 - 2.0 * sub);

    vec3 h_col = (f.x < 0.5) ? mix(w, c, sub.x) : mix(c, e, sub.x);
    vec3 v_col = (f.y < 0.5) ? mix(n, c, sub.y) : mix(c, s, sub.y);

    float edge_w = (grad_x + grad_y > 1e-4) ? (grad_x / (grad_x + grad_y)) : 0.5;
    vec3 blended = mix(v_col, h_col, edge_w);

    vec3 corner_col = c;
    if (f.x < 0.5 && f.y < 0.5) {
        corner_col = nw;
    } else if (f.x >= 0.5 && f.y < 0.5) {
        corner_col = ne;
    } else if (f.x < 0.5 && f.y >= 0.5) {
        corner_col = sw;
    } else {
        corner_col = se;
    }

    vec2 diag_dist = abs(f - 0.5);
    float corner_weight = clamp((diag_dist.x + diag_dist.y - 0.5) * 1.5, 0.0, 1.0);
    corner_weight = corner_weight * corner_weight;

    vec3 final_color = mix(blended, mix(blended, corner_col, 0.25), corner_weight);
    color = vec4(clamp(final_color, 0.0, 1.0), 1.0);
}

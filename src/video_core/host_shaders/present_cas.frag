// AMD FidelityFX Contrast Adaptive Sharpening (CAS)
#version 460 core

layout (location = 0) in vec2 frag_tex_coord;
layout (location = 0) out vec4 color;
layout (binding = 0) uniform sampler2D color_texture;

vec3 CasFilter(sampler2D tex, vec2 uv) {
    vec2 size = vec2(textureSize(tex, 0));
    vec2 step_val = 1.0 / size;

    vec3 a = texture(tex, uv + vec2(-step_val.x, -step_val.y)).rgb;
    vec3 b = texture(tex, uv + vec2(0.0, -step_val.y)).rgb;
    vec3 c = texture(tex, uv + vec2(step_val.x, -step_val.y)).rgb;
    vec3 d = texture(tex, uv + vec2(-step_val.x, 0.0)).rgb;
    vec3 e = texture(tex, uv).rgb;
    vec3 f = texture(tex, uv + vec2(step_val.x, 0.0)).rgb;
    vec3 g = texture(tex, uv + vec2(-step_val.x, step_val.y)).rgb;
    vec3 h = texture(tex, uv + vec2(0.0, step_val.y)).rgb;
    vec3 i = texture(tex, uv + vec2(step_val.x, step_val.y)).rgb;

    vec3 min_rgb = min(min(min(d, e), min(f, b)), h);
    vec3 min_rgb2 = min(min(min(min_rgb, a), min(c, g)), i);
    min_rgb += min_rgb2;

    vec3 max_rgb = max(max(max(d, e), max(f, b)), h);
    vec3 max_rgb2 = max(max(max(max_rgb, a), max(c, g)), i);
    max_rgb += max_rgb2;

    vec3 rcp_max = 1.0 / max_rgb;
    vec3 amp = clamp(min(min_rgb, 2.0 - max_rgb) * rcp_max, 0.0, 1.0);
    amp = sqrt(amp);

    float sharpness = -0.15;
    vec3 w = amp * sharpness;

    vec3 rcp_weight = 1.0 / (1.0 + 4.0 * w);
    vec3 res = clamp((b * w + d * w + f * w + h * w + e) * rcp_weight, 0.0, 1.0);
    return res;
}

void main() {
    color = vec4(CasFilter(color_texture, frag_tex_coord), 1.0);
}

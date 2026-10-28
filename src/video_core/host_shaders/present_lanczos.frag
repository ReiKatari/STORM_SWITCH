// Lanczos 3 Anti-Ringing EWA Resampling
#version 460 core

layout (location = 0) in vec2 frag_tex_coord;
layout (location = 0) out vec4 color;
layout (binding = 0) uniform sampler2D color_texture;

#define PI 3.1415926535897932384626433832795

float sinc(float x) {
    if (abs(x) < 1e-4) return 1.0;
    float pix = PI * x;
    return sin(pix) / pix;
}

float lanczos3(float x) {
    float ax = abs(x);
    if (ax >= 3.0) return 0.0;
    return sinc(x) * sinc(x / 3.0);
}

void main() {
    vec2 size = vec2(textureSize(color_texture, 0));
    vec2 pos = frag_tex_coord * size - 0.5;
    vec2 base = floor(pos);
    vec2 f = pos - base;

    float wx[6];
    float wy[6];
    for (int i = 0; i < 6; ++i) {
        wx[i] = lanczos3(float(i - 2) - f.x);
        wy[i] = lanczos3(float(i - 2) - f.y);
    }

    vec3 sum = vec3(0.0);
    float total_w = 0.0;
    vec3 min_color = vec3(1e6);
    vec3 max_color = vec3(-1e6);

    for (int y = 0; y < 6; ++y) {
        float yw = wy[y];
        float py = (base.y + float(y - 2) + 0.5) / size.y;
        for (int x = 0; x < 6; ++x) {
            float w = wx[x] * yw;
            float px = (base.x + float(x - 2) + 0.5) / size.x;
            vec3 s = texture(color_texture, vec2(px, py)).rgb;
            sum += s * w;
            total_w += w;

            if ((x == 2 || x == 3) && (y == 2 || y == 3)) {
                min_color = min(min_color, s);
                max_color = max(max_color, s);
            }
        }
    }

    vec3 lanczos_color = (total_w > 1e-5) ? (sum / total_w) : texture(color_texture, frag_tex_coord).rgb;
    vec3 clamped_color = clamp(lanczos_color, min_color, max_color);
    color = vec4(clamped_color, 1.0);
}

// SPDX-FileCopyrightText: Copyright 2026 STORM TEAM / Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// Genuine CRT-Royale Fragment Shader: Sony Trinitron Aperture Grille + Halation Bloom + Curvature
#version 450

layout (location = 0) in vec2 frag_tex_coord;
layout (location = 0) out vec4 color;
layout (binding = 0) uniform sampler2D color_texture;

const float PI = 3.14159265358979323846;

float getLuma(vec3 c) {
    return dot(c, vec3(0.299, 0.587, 0.114));
}

vec2 barrelDistort(vec2 uv, vec2 amt) {
    vec2 cc = uv * 2.0 - 1.0;
    cc *= 1.0 + dot(cc, cc) * amt;
    return cc * 0.5 + 0.5;
}

vec3 apertureGrilleMask(float x, float strength) {
    float phase = mod(floor(x), 3.0);
    vec3 m = vec3(1.0 - strength);
    if (phase < 0.5) {
        m.r = 1.0 + strength * 0.40;
    } else if (phase < 1.5) {
        m.g = 1.0 + strength * 0.40;
    } else {
        m.b = 1.0 + strength * 0.40;
    }
    return m;
}

vec3 sampleBloom9(vec2 uv, vec2 inv_tex, float radius) {
    vec2 d = inv_tex * radius;
    vec3 acc = texture(color_texture, uv).rgb * 0.25;
    acc += texture(color_texture, uv + vec2( d.x,  0.0)).rgb * 0.125;
    acc += texture(color_texture, uv + vec2(-d.x,  0.0)).rgb * 0.125;
    acc += texture(color_texture, uv + vec2( 0.0,  d.y)).rgb * 0.125;
    acc += texture(color_texture, uv + vec2( 0.0, -d.y)).rgb * 0.125;
    acc += texture(color_texture, uv + vec2( d.x,  d.y)).rgb * 0.0625;
    acc += texture(color_texture, uv + vec2(-d.x,  d.y)).rgb * 0.0625;
    acc += texture(color_texture, uv + vec2( d.x, -d.y)).rgb * 0.0625;
    acc += texture(color_texture, uv + vec2(-d.x, -d.y)).rgb * 0.0625;
    return acc;
}

void main() {
    vec2 tex_size = max(vec2(textureSize(color_texture, 0)), vec2(1.0));
    vec2 inv_tex = 1.0 / tex_size;

    // Subtle Sony Trinitron aperture grille cylindrical curvature
    vec2 warp = vec2(0.022, 0.030);
    vec2 cuv = barrelDistort(frag_tex_coord, warp);
    if (cuv.x < 0.0 || cuv.x > 1.0 || cuv.y < 0.0 || cuv.y > 1.0) {
        color = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // Subtle RGB convergence offset
    float chroma_shift = inv_tex.x * 0.35;
    vec3 col;
    col.r = texture(color_texture, cuv + vec2(chroma_shift, 0.0)).r;
    col.g = texture(color_texture, cuv).g;
    col.b = texture(color_texture, cuv - vec2(chroma_shift, 0.0)).b;

    // Gaussian beam scanline profile
    float scanlines_count = 270.0;
    float scan_phase = fract(cuv.y * scanlines_count);
    float luma = getLuma(col);
    float beam_dist = abs(scan_phase - 0.5) * 2.0;
    float beam_width = mix(0.45, 0.88, luma);
    float sigma = 0.45;
    float scan_weight = mix(0.50, 1.25, exp(-(beam_dist * beam_dist) / (sigma * sigma * max(beam_width, 0.35))));
    col *= scan_weight;

    // Sony Trinitron Aperture Grille Phosphor Mask
    vec3 mask = apertureGrilleMask(gl_FragCoord.x, 0.22);
    col *= mask;

    // Halation / Phosphor Bloom
    vec3 bloom = sampleBloom9(cuv, inv_tex, 2.4);
    col = mix(col, col + max(bloom - 0.22, vec3(0.0)) * 1.4, 0.20);

    // Tube vignette
    vec2 v = cuv * (1.0 - cuv);
    float vig = clamp(pow(v.x * v.y * 16.0, 0.18), 0.0, 1.0);
    col *= vig;

    color = vec4(clamp(col * 1.06, 0.0, 1.0), 1.0);
}

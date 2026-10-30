// SPDX-FileCopyrightText: Copyright 2026 STORM TEAM / Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// AMD FSR 2.0 Temporal Accumulation + Motion/Luma Disocclusion Clamp Shader
#version 450

layout (location = 0) in vec2 texcoord;
layout (location = 0) out vec4 out_color;

layout (binding = 0) uniform sampler2D current_frame;
layout (binding = 1) uniform sampler2D history_frame;

layout (push_constant) uniform PushConstants {
    vec4 render_size;   // xy: current input size, zw: 1.0 / input size
    vec4 display_size;  // xy: output size, zw: 1.0 / output size
    vec4 jitter_params; // xy: subpixel jitter offset in UV, z: alpha (default 0.88), w: gamma (default 1.25)
};

// RGB <-> YCoCg conversion for perceptually uniform temporal bounding boxes
vec3 RGBToYCoCg(vec3 rgb) {
    float y  =  0.25 * rgb.r + 0.5 * rgb.g + 0.25 * rgb.b;
    float co =  0.5  * rgb.r                - 0.5  * rgb.b;
    float cg = -0.25 * rgb.r + 0.5 * rgb.g - 0.25 * rgb.b;
    return vec3(y, co, cg);
}

vec3 YCoCgToRGB(vec3 ycocg) {
    float y  = ycocg.r;
    float co = ycocg.g;
    float cg = ycocg.b;
    float r  = y + co - cg;
    float g  = y + cg;
    float b  = y - co - cg;
    return vec3(r, g, b);
}

// Catmull-Rom 5-tap spatial filter for crisp subpixel upscale
vec3 SampleCatmullRom(sampler2D tex, vec2 uv, vec2 inv_size) {
    vec2 pos = uv / inv_size - 0.5;
    vec2 f = fract(pos);
    vec2 base = (floor(pos) + 0.5) * inv_size;

    vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    vec2 w3 = f * f * (-0.5 + 0.5 * f);

    vec2 w12 = w1 + w2;
    vec2 tc12 = base + (w2 / w12) * inv_size;
    vec2 tc0 = base - inv_size;
    vec2 tc3 = base + 2.0 * inv_size;

    vec3 s = vec3(0.0);
    s += texture(tex, vec2(tc12.x, tc12.y)).rgb * (w12.x * w12.y);
    s += texture(tex, vec2(tc0.x, tc12.y)).rgb * (w0.x * w12.y);
    s += texture(tex, vec2(tc3.x, tc12.y)).rgb * (w3.x * w12.y);
    s += texture(tex, vec2(tc12.x, tc0.y)).rgb * (w12.x * w0.y);
    s += texture(tex, vec2(tc12.x, tc3.y)).rgb * (w12.x * w3.y);
    return s;
}

void main() {
    vec2 uv = texcoord;
    vec2 inv_render_size = render_size.zw;

    // Apply subpixel phase offset for current frame sampling
    vec2 uv_curr = uv + jitter_params.xy;

    // Sample current frame using Catmull-Rom filtering
    vec3 curr_rgb = SampleCatmullRom(current_frame, uv_curr, inv_render_size);
    vec3 curr_ycocg = RGBToYCoCg(curr_rgb);

    // 3x3 neighborhood sampling in YCoCg for variance clipping box
    vec3 m1 = vec3(0.0);
    vec3 m2 = vec3(0.0);
    vec3 box_min = vec3(1e6);
    vec3 box_max = vec3(-1e6);

    vec3 sample_N  = RGBToYCoCg(texture(current_frame, uv_curr + vec2( 0.0, -inv_render_size.y)).rgb);
    vec3 sample_S  = RGBToYCoCg(texture(current_frame, uv_curr + vec2( 0.0,  inv_render_size.y)).rgb);
    vec3 sample_W  = RGBToYCoCg(texture(current_frame, uv_curr + vec2(-inv_render_size.x,  0.0)).rgb);
    vec3 sample_E  = RGBToYCoCg(texture(current_frame, uv_curr + vec2( inv_render_size.x,  0.0)).rgb);

    vec3 sample_NW = RGBToYCoCg(texture(current_frame, uv_curr + vec2(-inv_render_size.x, -inv_render_size.y)).rgb);
    vec3 sample_NE = RGBToYCoCg(texture(current_frame, uv_curr + vec2( inv_render_size.x, -inv_render_size.y)).rgb);
    vec3 sample_SW = RGBToYCoCg(texture(current_frame, uv_curr + vec2(-inv_render_size.x,  inv_render_size.y)).rgb);
    vec3 sample_SE = RGBToYCoCg(texture(current_frame, uv_curr + vec2( inv_render_size.x,  inv_render_size.y)).rgb);

    vec3 neighbors[9] = vec3[9](curr_ycocg, sample_N, sample_S, sample_W, sample_E, sample_NW, sample_NE, sample_SW, sample_SE);
    for (int i = 0; i < 9; ++i) {
        m1 += neighbors[i];
        m2 += neighbors[i] * neighbors[i];
        box_min = min(box_min, neighbors[i]);
        box_max = max(box_max, neighbors[i]);
    }

    vec3 mean = m1 / 9.0;
    vec3 variance = m2 / 9.0 - mean * mean;
    vec3 stddev = sqrt(max(variance, vec3(0.0)));

    float gamma = (jitter_params.w > 0.0) ? jitter_params.w : 1.25;
    box_min = max(box_min, mean - gamma * stddev);
    box_max = min(box_max, mean + gamma * stddev);

    // Estimate local luma-gradient optical flow when engine motion vectors are absent
    float grad_x = (sample_E.x - sample_W.x) * 0.5;
    float grad_y = (sample_S.x - sample_N.x) * 0.5;
    vec2 grad = vec2(grad_x, grad_y);

    vec3 hist_raw_rgb = texture(history_frame, uv).rgb;
    vec3 hist_raw_ycocg = RGBToYCoCg(hist_raw_rgb);
    float luma_diff = abs(curr_ycocg.x - hist_raw_ycocg.x);

    vec2 flow = vec2(0.0);
    float grad_sq = dot(grad, grad);
    if (grad_sq > 1e-5) {
        flow = grad * ((hist_raw_ycocg.x - curr_ycocg.x) / (grad_sq + 0.005));
        flow = clamp(flow * inv_render_size, -inv_render_size * 2.0, inv_render_size * 2.0);
    }

    // Reprojected history sampling & variance clipping
    vec3 reprojected_hist_rgb = texture(history_frame, uv + flow).rgb;
    vec3 reprojected_hist_ycocg = RGBToYCoCg(reprojected_hist_rgb);
    vec3 clamped_hist_ycocg = clamp(reprojected_hist_ycocg, box_min, box_max);

    // Disocclusion gating: fast motion / luma changes drop temporal accumulation weight
    float disocclusion = clamp(luma_diff * 8.0, 0.0, 1.0);
    float base_alpha = (jitter_params.z > 0.0) ? jitter_params.z : 0.88;
    float blend_weight = mix(base_alpha, 0.0, disocclusion);

    vec3 accumulated_ycocg = mix(curr_ycocg, clamped_hist_ycocg, blend_weight);
    vec3 result_rgb = clamp(YCoCgToRGB(accumulated_ycocg), 0.0, 1.0);

    out_color = vec4(result_rgb, 1.0);
}

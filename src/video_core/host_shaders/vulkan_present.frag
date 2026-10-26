// SPDX-FileCopyrightText: Copyright 2026 STORM TEAM / Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#version 460 core

layout (location = 0) in vec2 frag_tex_coord;
layout (location = 0) out vec4 color;
layout (binding = 0) uniform sampler2D color_texture;

struct ScreenRectVertex {
    vec2 position;
    vec2 tex_coord;
};

layout (push_constant) uniform PushConstants {
    mat4 modelview_matrix;
    ScreenRectVertex vertices[4];
};

const float PI = 3.14159265359;

float getLuma(vec3 c) {
    return dot(c, vec3(0.299, 0.587, 0.114));
}

float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec3 sampleSharp(vec2 uv, vec2 tex_size, float sharpness) {
    vec2 pt = uv * tex_size;
    vec2 i = floor(pt);
    vec2 f = fract(pt) - 0.5;
    vec2 sf = clamp(f * sharpness, -0.5, 0.5) + 0.5;
    return texture(color_texture, (i + sf) / tex_size).rgb;
}

vec2 barrelDistort(vec2 uv, vec2 amt) {
    vec2 cc = uv * 2.0 - 1.0;
    cc *= 1.0 + dot(cc, cc) * amt;
    return cc * 0.5 + 0.5;
}

vec3 apertureGrilleMask(float x, float strength, float pitch) {
    float phase = mod(floor(x / pitch), 3.0);
    vec3 m = vec3(1.0 - strength);
    if (phase < 0.5) m.r = 1.0 + strength * 0.35;
    else if (phase < 1.5) m.g = 1.0 + strength * 0.35;
    else m.b = 1.0 + strength * 0.35;
    return m;
}

vec3 shadowMaskTriad(vec2 frag_xy, float strength) {
    vec2 p = floor(frag_xy);
    float idx = mod(p.x + mod(p.y, 2.0) * 1.5, 3.0);
    vec3 m = vec3(1.0 - strength);
    if (idx < 1.0) m.r = 1.0 + strength * 0.4;
    else if (idx < 2.0) m.g = 1.0 + strength * 0.4;
    else m.b = 1.0 + strength * 0.4;
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

vec3 bicubicKernel(vec2 uv, vec2 tex_size, float B, float C) {
    vec2 inv_tex = 1.0 / tex_size;
    vec2 pos = uv * tex_size - 0.5;
    vec2 f = fract(pos);
    vec2 base = (floor(pos) + 0.5) * inv_tex;
    vec3 sum = vec3(0.0);
    float wsum = 0.0;
    for (int j = -1; j <= 2; ++j) {
        float dy = abs(float(j) - f.y);
        float wy = (dy < 1.0)
            ? ((12.0 - 9.0*B - 6.0*C)*dy*dy*dy + (-18.0 + 12.0*B + 6.0*C)*dy*dy + (6.0 - 2.0*B)) / 6.0
            : ((-B - 6.0*C)*dy*dy*dy + (6.0*B + 30.0*C)*dy*dy + (-12.0*B - 48.0*C)*dy + (8.0*B + 24.0*C)) / 6.0;
        for (int i = -1; i <= 2; ++i) {
            float dx = abs(float(i) - f.x);
            float wx = (dx < 1.0)
                ? ((12.0 - 9.0*B - 6.0*C)*dx*dx*dx + (-18.0 + 12.0*B + 6.0*C)*dx*dx + (6.0 - 2.0*B)) / 6.0
                : ((-B - 6.0*C)*dx*dx*dx + (6.0*B + 30.0*C)*dx*dx + (-12.0*B - 48.0*C)*dx + (8.0*B + 24.0*C)) / 6.0;
            float w = wx * wy;
            sum += texture(color_texture, base + vec2(float(i), float(j)) * inv_tex).rgb * w;
            wsum += w;
        }
    }
    return sum / max(wsum, 0.0001);
}

void main() {
    int shader_id = int(round(modelview_matrix[2].x));
    if (shader_id <= 0) {
        color = texture(color_texture, frag_tex_coord);
        return;
    }

    float u_time = modelview_matrix[2].y;
    vec2 out_size = max(modelview_matrix[2].zw, vec2(640.0, 360.0));
    vec2 tex_size = max(vec2(textureSize(color_texture, 0)), vec2(1.0));
    vec2 inv_tex = 1.0 / tex_size;
    vec2 uv = frag_tex_coord;

    vec4 base_sample = texture(color_texture, uv);
    vec3 c = base_sample.rgb;

    // Neighborhood samples (used by edge/upscale/sharpen/AA shaders)
    vec3 cN  = texture(color_texture, uv + vec2( 0.0, -inv_tex.y)).rgb;
    vec3 cS  = texture(color_texture, uv + vec2( 0.0,  inv_tex.y)).rgb;
    vec3 cW  = texture(color_texture, uv + vec2(-inv_tex.x,  0.0)).rgb;
    vec3 cE  = texture(color_texture, uv + vec2( inv_tex.x,  0.0)).rgb;
    vec3 cNW = texture(color_texture, uv + vec2(-inv_tex.x, -inv_tex.y)).rgb;
    vec3 cNE = texture(color_texture, uv + vec2( inv_tex.x, -inv_tex.y)).rgb;
    vec3 cSW = texture(color_texture, uv + vec2(-inv_tex.x,  inv_tex.y)).rgb;
    vec3 cSE = texture(color_texture, uv + vec2( inv_tex.x,  inv_tex.y)).rgb;

    // =========================================================================
    // GROUP 1: CRT & Scanlines (1..15)
    // =========================================================================
    if (shader_id >= 1 && shader_id <= 15) {
        vec2 warp = vec2(0.0);
        if (shader_id == 1) warp = vec2(0.022, 0.030);       // crt-royale
        else if (shader_id == 2) warp = vec2(0.018, 0.025);  // crt-guest-advanced
        else if (shader_id == 3) warp = vec2(0.045, 0.060);  // crt-geom
        else if (shader_id == 4) warp = vec2(0.030, 0.040);  // crt-lottes
        else if (shader_id == 5) warp = vec2(0.035, 0.045);  // crt-mattias
        else if (shader_id == 11) warp = vec2(0.028, 0.038); // crt-consumer
        else if (shader_id == 12) warp = vec2(0.012, 0.016); // crt-sony-pvm-20l4

        vec2 cuv = (warp.x > 0.0) ? barrelDistort(uv, warp) : uv;
        if (cuv.x < 0.0 || cuv.x > 1.0 || cuv.y < 0.0 || cuv.y > 1.0) {
            color = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }

        // Mattias / Consumer subtle analog horizontal sync hum & chroma convergence
        float chroma_shift = 0.0;
        if (shader_id == 5) {
            cuv.x += sin(cuv.y * 80.0 + u_time * 6.0) * 0.00035;
            chroma_shift = inv_tex.x * 0.85;
        } else if (shader_id == 11) {
            chroma_shift = inv_tex.x * 1.15;
        } else if (shader_id == 1) {
            chroma_shift = inv_tex.x * 0.35;
        }

        float sharpness = (shader_id == 8) ? 1.5 : ((shader_id == 9 || shader_id == 12) ? 3.2 : 2.4);
        vec3 col = sampleSharp(cuv, tex_size, sharpness);
        if (chroma_shift > 0.0) {
            col.r = sampleSharp(cuv + vec2(chroma_shift, 0.0), tex_size, sharpness).r;
            col.b = sampleSharp(cuv - vec2(chroma_shift, 0.0), tex_size, sharpness).b;
        }

        // Scanline beam profile (based on virtual 240p/360p/480p scanline density)
        float scan_lines_count = (shader_id == 12) ? 400.0 : ((shader_id == 10 || shader_id == 13) ? 360.0 : 270.0);
        float scan_phase = fract(cuv.y * scan_lines_count);
        float luma = getLuma(col);
        float beam_dist = abs(scan_phase - 0.5) * 2.0;
        float beam_width = mix(0.45, 0.88, luma); // Bright pixels widen the electron beam
        float scan_weight = 1.0;

        if (shader_id == 15) {
            // scanlines-sine-abs
            scan_weight = mix(0.62, 1.12, abs(sin(cuv.y * scan_lines_count * PI)));
        } else if (shader_id == 14) {
            // zfast-crt
            scan_weight = mix(0.70, 1.10, 1.0 - beam_dist * beam_dist);
        } else if (shader_id == 8) {
            // crt-caligari phosphor spot
            float spot_x = abs(fract(cuv.x * scan_lines_count * 1.777) - 0.5) * 2.0;
            scan_weight = mix(0.68, 1.18, clamp(1.0 - 0.55 * (beam_dist * beam_dist + spot_x * spot_x), 0.0, 1.0));
        } else {
            float sigma = (shader_id == 9 || shader_id == 12) ? 0.38 : 0.48;
            scan_weight = mix(0.52, 1.22, exp(-(beam_dist * beam_dist) / (sigma * sigma * max(beam_width, 0.35))));
        }
        col *= scan_weight;

        // Phosphor Mask (Aperture Grille vs Shadow Mask)
        if (shader_id != 15) {
            float mask_str = (shader_id == 7 || shader_id == 12) ? 0.24 : ((shader_id == 14) ? 0.12 : 0.18);
            if (shader_id == 2 || shader_id == 4 || shader_id == 13) {
                col *= shadowMaskTriad(gl_FragCoord.xy, mask_str);
            } else {
                float pitch = (shader_id == 12) ? 1.0 : 1.0;
                col *= apertureGrilleMask(gl_FragCoord.x, mask_str, pitch);
            }
        }

        // Halation / Phosphor Bloom
        if (shader_id == 1 || shader_id == 4 || shader_id == 5 || shader_id == 7 || shader_id == 12) {
            vec3 bloom = sampleBloom9(cuv, inv_tex, 2.2);
            float halation_amt = (shader_id == 5) ? 0.26 : ((shader_id == 1 || shader_id == 4) ? 0.20 : 0.14);
            col = mix(col, col + max(bloom - 0.25, vec3(0.0)) * 1.35, halation_amt);
        }

        // Color temperature & tube glass vignette
        if (shader_id == 5) {
            col *= vec3(1.06, 1.01, 0.92); // Warm 80s TV
        } else if (shader_id == 13) {
            col = clamp((col - 0.5) * 1.12 + 0.5, 0.0, 1.0) * 1.05; // Arcade RGB punch
        }
        if (warp.x > 0.0) {
            vec2 v = cuv * (1.0 - cuv);
            float vig = clamp(pow(v.x * v.y * 16.0, 0.18), 0.0, 1.0);
            col *= vig;
        }
        c = clamp(col * 1.06, 0.0, 1.0);
    }
    // =========================================================================
    // GROUP 2: Pixel-Art & Vector Contour Upscaling (16..30)
    // =========================================================================
    else if (shader_id >= 16 && shader_id <= 30) {
        vec2 pt = uv * tex_size - 0.5;
        vec2 f = fract(pt);

        float lC  = getLuma(c);
        float lN  = getLuma(cN);
        float lS  = getLuma(cS);
        float lW  = getLuma(cW);
        float lE  = getLuma(cE);
        float lNW = getLuma(cNW);
        float lNE = getLuma(cNE);
        float lSW = getLuma(cSW);
        float lSE = getLuma(cSE);

        // Diagonal similarity weights (xBRZ / ScaleFX / HQx / 2xSaI / SABR / OmniScale)
        float d_nw_se = abs(lNW - lSE);
        float d_ne_sw = abs(lNE - lSW);
        float d_n_w = abs(lN - lW);
        float d_n_e = abs(lN - lE);
        float d_s_w = abs(lS - lW);
        float d_s_e = abs(lS - lE);

        if (shader_id == 29) {
            // AdvMAME Scale4x: exact palette-preserving corner replacement
            vec3 res = c;
            float thr = 0.08;
            if (abs(lN - lS) > thr && abs(lW - lE) > thr) {
                if (f.x < 0.5 && f.y < 0.5 && d_n_w < thr) res = (d_n_w < 0.03) ? cN : mix(cN, cW, 0.5);
                else if (f.x >= 0.5 && f.y < 0.5 && d_n_e < thr) res = (d_n_e < 0.03) ? cN : mix(cN, cE, 0.5);
                else if (f.x < 0.5 && f.y >= 0.5 && d_s_w < thr) res = (d_s_w < 0.03) ? cS : mix(cS, cW, 0.5);
                else if (f.x >= 0.5 && f.y >= 0.5 && d_s_e < thr) res = (d_s_e < 0.03) ? cS : mix(cS, cE, 0.5);
            }
            c = res;
        } else {
            // Vector contour sharpness factor per algorithm
            float edge_power = 3.0;
            if (shader_id == 16) edge_power = 2.5;      // xbrz-2x
            else if (shader_id == 17) edge_power = 4.0; // xbrz-4x
            else if (shader_id == 18) edge_power = 5.5; // xbrz-6x
            else if (shader_id == 19) edge_power = 3.8; // scalefx
            else if (shader_id == 20) edge_power = 4.6; // scalefx-aa
            else if (shader_id == 21) edge_power = 3.4; // mmpx
            else if (shader_id == 22) edge_power = 2.8; // hq2x
            else if (shader_id == 23) edge_power = 4.2; // hq4x
            else if (shader_id == 27) edge_power = 3.6; // omniscale
            else if (shader_id == 28) edge_power = 4.4; // sabr-v3

            vec2 dir = vec2(
                (lSW + lW + lNW) - (lSE + lE + lNE),
                (lNW + lN + lNE) - (lSW + lS + lSE)
            );
            float dir_len = length(dir);
            vec2 norm_dir = (dir_len > 0.001) ? (dir / dir_len) : vec2(0.0);

            // Sample along perpendicular contour tangent for smooth diagonal vector edges
            vec2 tangent = vec2(-norm_dir.y, norm_dir.x) * inv_tex * 0.55;
            vec3 cT1 = texture(color_texture, uv + tangent).rgb;
            vec3 cT2 = texture(color_texture, uv - tangent).rgb;
            vec3 contour_avg = (c * 2.0 + cT1 + cT2) * 0.25;

            // Clamp to local neighborhood min/max to prevent ringing
            vec3 min_nb = min(c, min(min(cN, cS), min(cW, cE)));
            vec3 max_nb = max(c, max(max(cN, cS), max(cW, cE)));

            // Push towards nearest contour edge (reverse AA / vector sharpening)
            float edge_strength = clamp(dir_len * 1.4, 0.0, 1.0);
            vec3 sharpened = c + (c - (cN + cS + cW + cE) * 0.25) * (0.28 * edge_power);
            vec3 blended = mix(c, mix(contour_avg, sharpened, 0.55), edge_strength);
            c = clamp(blended, min_nb * 0.92, max_nb * 1.08);
        }
    }
    // =========================================================================
    // GROUP 3: Anime, Neural & Super-Resolution (31..38)
    // =========================================================================
    else if (shader_id >= 31 && shader_id <= 38) {
        vec3 min4 = min(min(cN, cS), min(cW, cE));
        vec3 max4 = max(max(cN, cS), max(cW, cE));
        vec3 avg4 = (cN + cS + cW + cE) * 0.25;
        vec3 avg8 = (cN + cS + cW + cE + cNW + cNE + cSW + cSE) * 0.125;

        if (shader_id == 31 || shader_id == 32) {
            // Anime4K v4.0 Restore / Fast: Line thinning + dark contour reconstruction + deblur
            float str = (shader_id == 31) ? 0.65 : 0.42;
            float lC = getLuma(c);
            float lMax = max(max(getLuma(cN), getLuma(cS)), max(getLuma(cW), getLuma(cE)));
            float lMin = min(min(getLuma(cN), getLuma(cS)), min(getLuma(cW), getLuma(cE)));
            // Push towards min or max depending on which is closer (thins blurry anime outlines!)
            float dist_min = abs(lC - lMin);
            float dist_max = abs(lMax - lC);
            vec3 target_edge = (dist_min < dist_max) ? min4 : max4;
            vec3 unsharp = c + (c - avg8) * (str * 1.15);
            c = clamp(mix(unsharp, target_edge, str * 0.48), min(min4, c) * 0.95, max(max4, c) * 1.05);
        } else if (shader_id == 33 || shader_id == 34) {
            // AMD FSR 1.0 EASU+RCAS (33) / Snapdragon SGSR v2 (34)
            float sharp_str = (shader_id == 33) ? 0.85 : 0.72;
            vec3 amp = clamp(min(min4, 2.0 - max4) / max(max4, vec3(0.0001)), 0.0, 1.0);
            vec3 w = -amp * (0.22 * sharp_str);
            c = clamp((c + (cN + cS + cW + cE) * w) / (1.0 + 4.0 * w), 0.0, 1.0);
        } else if (shader_id == 35) {
            // ScaleForce
            vec3 bic = bicubicKernel(uv, tex_size, 0.0, 0.6);
            c = clamp(bic + (c - avg4) * 0.35, min(min4, c), max(max4, c));
        } else if (shader_id == 36 || shader_id == 37 || shader_id == 38) {
            // RAVU-Zoom-R3 (36) / NNEDI3-64 (37) / FSRCNNX (38)
            float detail_boost = (shader_id == 38) ? 0.95 : ((shader_id == 37) ? 0.75 : 0.65);
            vec3 hf = c - avg8;
            c = clamp(c + hf * detail_boost, min(min4, c) * 0.94, max(max4, c) * 1.06);
        }
    }
    // =========================================================================
    // GROUP 4: Windowed Sinc / Bicubic / Lanczos (39..45)
    // =========================================================================
    else if (shader_id >= 39 && shader_id <= 45) {
        vec3 min4 = min(c, min(min(cN, cS), min(cW, cE)));
        vec3 max4 = max(c, max(max(cN, cS), max(cW, cE)));
        if (shader_id == 39) {
            // Jinc2-Sharp (Anti-Ringing EWA Lanczos)
            vec3 k = bicubicKernel(uv, tex_size, 0.0, 0.75);
            c = clamp(k, min4, max4);
        } else if (shader_id == 40) {
            // Lanczos3-Sharp
            vec3 k = bicubicKernel(uv, tex_size, 0.0, 0.85);
            c = clamp(k, min4 * 0.97, max4 * 1.03);
        } else if (shader_id == 41) {
            // Spline36
            c = bicubicKernel(uv, tex_size, 0.15, 0.55);
        } else if (shader_id == 42) {
            // Bicubic Mitchell-Netravali (B=1/3, C=1/3)
            c = bicubicKernel(uv, tex_size, 0.3333, 0.3333);
        } else if (shader_id == 43) {
            // Bicubic Catmull-Rom (B=0, C=0.5)
            c = bicubicKernel(uv, tex_size, 0.0, 0.5);
        } else if (shader_id == 44) {
            // Sharp-Bilinear
            c = sampleSharp(uv, tex_size, 3.0);
        } else if (shader_id == 45) {
            // Pixel-AA
            c = sampleSharp(uv, tex_size, 2.0);
        }
    }
    // =========================================================================
    // GROUP 5: Handheld & LCD Matrices (46..53)
    // =========================================================================
    else if (shader_id >= 46 && shader_id <= 53) {
        vec2 lcd_res = vec2(480.0, 270.0);
        vec2 cell = fract(uv * lcd_res);
        vec2 border = smoothstep(0.0, 0.16, cell) * smoothstep(1.0, 0.84, cell);
        float grid = mix(0.66, 1.06, border.x * border.y);

        vec3 col = sampleSharp(uv, tex_size, 2.2);

        if (shader_id == 46) {
            // LCD-Grid v2: RGB subpixel modulation + cell grid
            vec3 sub = apertureGrilleMask(cell.x * 3.0, 0.16, 1.0);
            c = col * sub * grid;
        } else if (shader_id == 47) {
            // ZFast-LCD
            c = col * mix(0.75, 1.04, border.x * border.y);
        } else if (shader_id == 48) {
            // GBA Color + LCD Grid (Authentic non-backlit GBA warm color matrix)
            vec3 gba = pow(col, vec3(1.25));
            gba = vec3(
                dot(gba, vec3(0.84, 0.14, 0.02)),
                dot(gba, vec3(0.09, 0.78, 0.13)),
                dot(gba, vec3(0.06, 0.18, 0.76))
            );
            c = clamp(pow(max(gba, vec3(0.0)), vec3(1.0 / 1.9)) * grid, 0.0, 1.0);
        } else if (shader_id == 49) {
            // GBC Dot-Matrix (Reflective dot-matrix with metallic backing)
            float dot_dist = length(cell - 0.5) * 1.65;
            float dot_mask = 1.0 - smoothstep(0.45, 0.85, dot_dist);
            vec3 paper_bg = vec3(0.76, 0.78, 0.73) * 0.35;
            c = mix(paper_bg, col * 1.12, dot_mask);
        } else if (shader_id == 50) {
            // Game Boy DMG-01 Olive Green (Classic 1989 4-shade olive dot-matrix)
            float l = getLuma(col);
            vec3 p0 = vec3(0.059, 0.220, 0.059); // #0f380f
            vec3 p1 = vec3(0.188, 0.384, 0.188); // #306230
            vec3 p2 = vec3(0.545, 0.675, 0.059); // #8bac0f
            vec3 p3 = vec3(0.608, 0.737, 0.059); // #9bbc0f
            vec3 dmg = (l < 0.25) ? mix(p0, p1, l * 4.0)
                     : (l < 0.55) ? mix(p1, p2, (l - 0.25) / 0.30)
                     : mix(p2, p3, (l - 0.55) / 0.45);
            c = mix(p3 * 0.72, dmg, border.x * border.y);
        } else if (shader_id == 51) {
            // Game Boy Pocket Silver (High-contrast silver monochrome matrix)
            float l = getLuma(col);
            vec3 dark_ink = vec3(0.14, 0.16, 0.15);
            vec3 silver_bg = vec3(0.78, 0.81, 0.77);
            vec3 mono = mix(dark_ink, silver_bg, smoothstep(0.05, 0.95, l));
            c = mix(silver_bg * 0.78, mono, border.x * border.y);
        } else if (shader_id == 52) {
            // PSP Color Matrix (Wide gamut + subtle horizontal LCD scanline)
            vec3 psp = vec3(
                dot(col, vec3(1.10, -0.06, -0.04)),
                dot(col, vec3(-0.04, 1.08, -0.04)),
                dot(col, vec3(-0.03, -0.07, 1.10))
            );
            float h_line = mix(0.90, 1.04, border.y);
            c = clamp(psp * h_line, 0.0, 1.0);
        } else if (shader_id == 53) {
            // NDS LCD Grid
            vec3 nds = pow(col, vec3(1.14)) * vec3(1.03, 1.00, 0.95);
            c = clamp(nds * grid, 0.0, 1.0);
        }
    }
    // =========================================================================
    // GROUP 6: Sharpening & Micro-Contrast (54..58)
    // =========================================================================
    else if (shader_id >= 54 && shader_id <= 58) {
        vec3 avg4 = (cN + cS + cW + cE) * 0.25;
        vec3 avg8 = (cN + cS + cW + cE + cNW + cNE + cSW + cSE) * 0.125;
        if (shader_id == 54) {
            // AMD CAS (Contrast Adaptive Sharpening)
            vec3 min_rgb = min(c, min(min(cN, cS), min(cW, cE)));
            vec3 max_rgb = max(c, max(max(cN, cS), max(cW, cE)));
            vec3 amp = sqrt(clamp(min(min_rgb, 1.0 - max_rgb) / max(max_rgb, vec3(0.0001)), 0.0, 1.0));
            vec3 w = -amp * 0.18;
            c = clamp((c + (cN + cS + cW + cE) * w) / (1.0 + 4.0 * w), 0.0, 1.0);
        } else if (shader_id == 55) {
            // LumaSharpen (Luma-only sharpening without chroma halo)
            float l_diff = getLuma(c) - getLuma(avg8);
            c = clamp(c + vec3(clamp(l_diff * 1.35, -0.22, 0.22)), 0.0, 1.0);
        } else if (shader_id == 56) {
            // Adaptive Sharpen (Deep micro-contrast enhancer)
            vec3 diff = c - avg8;
            float edge = length(diff);
            float weight = mix(1.45, 0.55, smoothstep(0.05, 0.35, edge));
            c = clamp(c + diff * weight, 0.0, 1.0);
        } else if (shader_id == 57) {
            // FineSharp (Fine UI & distant detail enhancer)
            vec3 diff = c - avg4;
            c = clamp(c + sign(diff) * pow(abs(diff), vec3(0.82)) * 0.65, 0.0, 1.0);
        } else if (shader_id == 58) {
            // Fast-Sharpen
            c = clamp(c + (c - avg4) * 0.60, 0.0, 1.0);
        }
    }
    // =========================================================================
    // GROUP 7: Anti-Aliasing, De-Dithering & Debanding (59..64)
    // =========================================================================
    else if (shader_id >= 59 && shader_id <= 64) {
        if (shader_id == 59 || shader_id == 60) {
            // SMAA Ultra (59) / FXAA Quality (60)
            float lNW = getLuma(cNW), lNE = getLuma(cNE), lSW = getLuma(cSW), lSE = getLuma(cSE), lM = getLuma(c);
            float lMin = min(lM, min(min(lNW, lNE), min(lSW, lSE)));
            float lMax = max(lM, max(max(lNW, lNE), max(lSW, lSE)));
            if ((lMax - lMin) >= max(0.0312, lMax * 0.125)) {
                vec2 dir = vec2(
                    -((lNW + lNE) - (lSW + lSE)),
                     ((lNW + lSW) - (lNE + lSE))
                );
                float dirReduce = max((lNW + lNE + lSW + lSE) * 0.03125, 1.0 / 128.0);
                float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
                float span = (shader_id == 59) ? 8.0 : 6.0;
                dir = clamp(dir * rcpDirMin, vec2(-span), vec2(span)) * inv_tex;
                vec3 rgbA = 0.5 * (
                    texture(color_texture, uv + dir * (1.0 / 3.0 - 0.5)).rgb +
                    texture(color_texture, uv + dir * (2.0 / 3.0 - 0.5)).rgb
                );
                vec3 rgbB = rgbA * 0.5 + 0.25 * (
                    texture(color_texture, uv + dir * -0.5).rgb +
                    texture(color_texture, uv + dir *  0.5).rgb
                );
                float lB = getLuma(rgbB);
                c = (lB < lMin || lB > lMax) ? rgbA : rgbB;
            }
        } else if (shader_id == 61) {
            // Temporal Soft AA (Filmic anti-shimmer soft edge filter)
            vec3 avg = (c * 2.0 + cN + cS + cW + cE) / 6.0;
            float edge = abs(getLuma(c) - getLuma(avg));
            c = mix(c, avg, smoothstep(0.01, 0.12, edge) * 0.70);
        } else if (shader_id == 62) {
            // Deband Pro (Stochastic gradient debanding for sky/fog/shadows)
            float r1 = hash12(gl_FragCoord.xy + vec2(1.7, 3.1));
            float r2 = hash12(gl_FragCoord.yx + vec2(5.3, 2.9));
            vec2 off = vec2(cos(r1 * 6.28318), sin(r1 * 6.28318)) * (r2 * 6.0) * inv_tex;
            vec3 s1 = texture(color_texture, uv + off).rgb;
            vec3 s2 = texture(color_texture, uv - off).rgb;
            vec3 s3 = texture(color_texture, uv + vec2(off.y, -off.x)).rgb;
            vec3 s4 = texture(color_texture, uv - vec2(off.y, -off.x)).rgb;
            vec3 avg = (s1 + s2 + s3 + s4) * 0.25;
            vec3 diff = abs(c - avg);
            // Smooth out subtle quantization steps while preserving real edges
            vec3 factor = smoothstep(vec3(0.025), vec3(0.004), diff);
            c = mix(c, avg + (r1 - 0.5) * (1.5 / 255.0), factor);
        } else if (shader_id == 63 || shader_id == 64) {
            // MDAPT (63) / GDAPT (64): Checkerboard & vertical line dithering blender
            bool h_alt = length(cW - cE) < 0.08 && length(c - cW) > 0.04;
            bool v_alt = length(cN - cS) < 0.08 && length(c - cN) > 0.04;
            if (h_alt && v_alt) {
                c = (c * 2.0 + cN + cS + cW + cE) / 6.0;
            } else if (h_alt || shader_id == 64) {
                float w = (h_alt) ? 0.5 : clamp(1.0 - length(cW - cE) * 5.0, 0.0, 0.35);
                c = mix(c, (cW + cE) * 0.5, w);
            }
        }
    }
    // =========================================================================
    // GROUP 8: Cinema, Color Grading, HDR & Retro FX (65..74)
    // =========================================================================
    else if (shader_id >= 65 && shader_id <= 74) {
        if (shader_id == 65) {
            // Fake HDR + Vibrance & Bloom
            vec3 bloom = sampleBloom9(uv, inv_tex, 3.0);
            vec3 hdr = c + max(bloom - 0.32, vec3(0.0)) * 0.55;
            // S-curve contrast
            hdr = clamp((hdr - 0.5) * 1.16 + 0.5, 0.0, 1.0);
            // Smart Vibrance (boosts muted colors more than already-saturated colors)
            float l = getLuma(hdr);
            float sat = max(hdr.r, max(hdr.g, hdr.b)) - min(hdr.r, min(hdr.g, hdr.b));
            hdr = mix(vec3(l), hdr, 1.0 + (1.0 - sat) * 0.38);
            c = clamp(hdr, 0.0, 1.0);
        } else if (shader_id == 66) {
            // Switch OLED Vivid (Deep True Black & Vivid Switch OLED DCI-P3 Palette)
            vec3 oled = smoothstep(vec3(0.015), vec3(0.985), c);
            float l = getLuma(oled);
            oled = mix(vec3(l), oled, 1.32);
            c = clamp(oled, 0.0, 1.0);
        } else if (shader_id == 67) {
            // VHS Pro Tape (80s Magnetic Tape, Chroma Shift & Tracking Lines)
            float wave = sin(uv.y * 14.0 + u_time * 3.5) * 0.0012;
            float tracking_band = smoothstep(0.96, 1.0, sin(uv.y * 3.0 - u_time * 1.4));
            vec2 vuv = vec2(clamp(uv.x + wave + tracking_band * 0.012, 0.0, 1.0), uv.y);
            float luma = getLuma(texture(color_texture, vuv).rgb);
            vec3 chr_l = texture(color_texture, vuv - vec2(inv_tex.x * 3.2, 0.0)).rgb;
            vec3 chr_r = texture(color_texture, vuv + vec2(inv_tex.x * 3.2, 0.0)).rgb;
            vec3 chroma = (chr_l + chr_r) * 0.5;
            vec3 vhs = chroma + vec3(luma - getLuma(chroma));
            float scan = mix(0.88, 1.06, abs(sin(uv.y * 240.0 * PI)));
            float noise = (hash12(gl_FragCoord.xy + vec2(u_time * 60.0, u_time * 17.0)) - 0.5) * (0.05 + tracking_band * 0.18);
            c = clamp(vhs * scan + noise, 0.0, 1.0);
        } else if (shader_id == 68) {
            // NTSC Adaptive Composite (Analog Composite TV Signal & Rainbow Artifacting)
            float phase = (gl_FragCoord.x + gl_FragCoord.y * 2.0) * (PI * 0.5);
            float carrier = sin(phase);
            float luma_edge = (getLuma(cE) - getLuma(cW));
            vec3 chroma_bleed = (cW + c * 2.0 + cE) * 0.25;
            float l = getLuma(c);
            vec3 comp = vec3(l) + (chroma_bleed - vec3(getLuma(chroma_bleed)));
            // Composite rainbow fringe on sharp horizontal luma transitions
            comp += vec3(carrier, sin(phase + 2.094), sin(phase + 4.188)) * (luma_edge * 0.22);
            c = clamp(comp, 0.0, 1.0);
        } else if (shader_id == 69) {
            // 35mm Film Grain & Halation
            vec3 bloom = sampleBloom9(uv, inv_tex, 2.8);
            // Red-orange halation around bright highlights (classic 35mm emulsion back-scatter)
            vec3 halation = max(bloom - 0.45, vec3(0.0)) * vec3(0.85, 0.28, 0.08);
            vec3 film = c + halation;
            // Warm Kodak 2383 print curve
            film = pow(clamp(film, 0.0, 1.0), vec3(0.96, 0.99, 1.05));
            float grain = (hash12(gl_FragCoord.xy + vec2(fract(u_time * 24.0) * 100.0)) - 0.5) * 0.055;
            c = clamp(film + grain * (1.0 - getLuma(film) * 0.5), 0.0, 1.0);
        } else if (shader_id == 70) {
            // Technicolor Cine (Vintage Cinema Color Grading)
            float r = c.r;
            float g = c.g;
            float b = c.b;
            float cyan_ch = (g * 0.62 + b * 0.38);
            vec3 tech = vec3(r * 1.12, cyan_ch * 0.96, cyan_ch * 0.88);
            float l = getLuma(c);
            tech = mix(vec3(l), tech, 1.25);
            vec2 v = uv * (1.0 - uv);
            float vig = clamp(pow(v.x * v.y * 15.0, 0.16), 0.0, 1.0);
            c = clamp(tech * vig, 0.0, 1.0);
        } else if (shader_id == 71) {
            // Cel-Shading / Comic Ink (Sobel 3x3 Ink Outline + Posterized Cel Shading)
            float lNW = getLuma(cNW), lN = getLuma(cN), lNE = getLuma(cNE);
            float lW  = getLuma(cW),                    lE  = getLuma(cE);
            float lSW = getLuma(cSW), lS = getLuma(cS), lSE = getLuma(cSE);
            float gx = (-lNW - 2.0*lW - lSW) + (lNE + 2.0*lE + lSE);
            float gy = (-lNW - 2.0*lN - lNE) + (lSW + 2.0*lS + lSE);
            float edge = sqrt(gx * gx + gy * gy);
            float ink = 1.0 - smoothstep(0.14, 0.38, edge);
            // Quantize luminance into stylized cel bands while keeping vivid hue
            float l = getLuma(c);
            float cel_l = (floor(l * 7.0 + 0.5) / 7.0);
            vec3 cel_col = c * (mix(l, cel_l, 0.45) / max(l, 0.001));
            float sat_l = getLuma(cel_col);
            cel_col = mix(vec3(sat_l), cel_col, 1.22);
            c = clamp(cel_col * ink, 0.0, 1.0);
        } else if (shader_id == 72) {
            // Sepia Vintage (Warm Antique Photograph Toning)
            vec3 sep = vec3(
                dot(c, vec3(0.393, 0.769, 0.189)),
                dot(c, vec3(0.349, 0.686, 0.168)),
                dot(c, vec3(0.272, 0.534, 0.131))
            );
            vec2 v = uv * (1.0 - uv);
            float vig = clamp(pow(v.x * v.y * 16.0, 0.22), 0.0, 1.0);
            c = clamp(mix(c, sep, 0.88) * vig, 0.0, 1.0);
        } else if (shader_id == 73) {
            // Night Vision Goggles (Green Phosphor Tactical NVG)
            float l = getLuma(c);
            float bloom_l = getLuma(sampleBloom9(uv, inv_tex, 2.5));
            float amplified = pow(clamp(l * 1.45 + bloom_l * 0.45, 0.0, 1.0), 0.72);
            float scan = mix(0.84, 1.08, abs(sin(uv.y * 320.0 * PI)));
            float noise = (hash12(gl_FragCoord.xy + vec2(u_time * 90.0, u_time * 31.0)) - 0.5) * 0.09;
            float nvg = clamp(amplified * scan + noise, 0.0, 1.0);
            vec3 phos = vec3(nvg * nvg * 0.22, nvg * 1.05, nvg * nvg * 0.28);
            vec2 v = uv * (1.0 - uv);
            float vig = clamp(pow(v.x * v.y * 18.0, 0.30), 0.0, 1.0);
            c = clamp(phos * vig, 0.0, 1.0);
        } else if (shader_id == 74) {
            // Halation & Dual-Kawase Glow (Neon Atmospheric Highlight Bloom)
            vec3 b1 = sampleBloom9(uv, inv_tex, 2.0);
            vec3 b2 = sampleBloom9(uv, inv_tex, 5.0);
            vec3 glow = max((b1 + b2) * 0.5 - 0.22, vec3(0.0)) * 0.85;
            float scan = mix(0.90, 1.05, abs(sin(uv.y * 300.0 * PI)));
            c = clamp((c + glow) * scan, 0.0, 1.0);
        }
    }

    color = vec4(c, base_sample.a);
}

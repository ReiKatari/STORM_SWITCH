// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.overlay.model

import org.yuzu.yuzu_emu.R

enum class OverlayRenderStyle {
    AURORA_GLASS_3D,
    SWITCH_2_MAGNETIC,
    CYBERPUNK_EDGE,
    OLED_GHOST,
    TITANIUM_TACTICAL,
    HYRULE_RELIC,
    SANCTUARY_BLOOD,
    ARCADE_CANDY_3D,
    LIQUID_QUARTZ,
    RETRO_PIXEL;

    companion object {
        val NEON_GLOW get() = CYBERPUNK_EDGE
        val SWITCH_JOYCON get() = SWITCH_2_MAGNETIC
        val FROST_GLASS_3D get() = AURORA_GLASS_3D
        val TITANIUM_MECHA get() = TITANIUM_TACTICAL
        val FLAT_MINIMAL get() = OLED_GHOST
        val RETRO_CLASSIC get() = RETRO_PIXEL
        val GOLDEN_ORNATE get() = HYRULE_RELIC
    }
}

enum class OverlayTheme(
    val id: Int,
    val titleResId: Int,
    val descResId: Int,
    val colorDefault: Int,
    val colorPressed: Int,
    val renderStyle: OverlayRenderStyle
) {
    STORM_AURORA_GLASS_3D(
        0,
        R.string.storm_ov20_skin_storm_aurora_glass,
        R.string.storm_ov20_desc_aurora_glass,
        0xFF00E5FF.toInt(),
        0xFF6366F1.toInt(),
        OverlayRenderStyle.AURORA_GLASS_3D
    ),
    SWITCH_2_PRO_MAGNETIC(
        1,
        R.string.storm_ov20_skin_switch_2_magnetic,
        R.string.storm_ov20_desc_switch_2_magnetic,
        0xFF00C3E3.toInt(),
        0xFFFF4B3A.toInt(),
        OverlayRenderStyle.SWITCH_2_MAGNETIC
    ),
    CYBERPUNK_EDGE_NEON(
        2,
        R.string.storm_ov20_skin_cyberpunk_edge,
        R.string.storm_ov20_desc_cyberpunk_edge,
        0xFF06B6D4.toInt(),
        0xFFFACC15.toInt(),
        OverlayRenderStyle.CYBERPUNK_EDGE
    ),
    OLED_GHOST_WIREFRAME(
        3,
        R.string.storm_ov20_skin_oled_ghost,
        R.string.storm_ov20_desc_oled_ghost,
        0xAA94A3B8.toInt(),
        0xFFFFFFFF.toInt(),
        OverlayRenderStyle.OLED_GHOST
    ),
    TITANIUM_MECHA_TACTICAL(
        4,
        R.string.storm_ov20_skin_titanium_tactical,
        R.string.storm_ov20_desc_titanium_tactical,
        0xFF10B981.toInt(),
        0xFFF59E0B.toInt(),
        OverlayRenderStyle.TITANIUM_TACTICAL
    ),
    HYRULE_RELIC_GOLD(
        5,
        R.string.storm_ov20_skin_hyrule_relic,
        R.string.storm_ov20_desc_hyrule_relic,
        0xFFF59E0B.toInt(),
        0xFF14B8A6.toInt(),
        OverlayRenderStyle.HYRULE_RELIC
    ),
    SANCTUARY_BLOOD_RUNE(
        6,
        R.string.storm_ov20_skin_sanctuary_blood,
        R.string.storm_ov20_desc_sanctuary_blood,
        0xFFDC2626.toInt(),
        0xFFEF4444.toInt(),
        OverlayRenderStyle.SANCTUARY_BLOOD
    ),
    ARCADE_CANDY_TACTILE_3D(
        7,
        R.string.storm_ov20_skin_arcade_candy,
        R.string.storm_ov20_desc_arcade_candy,
        0xFFEC4899.toInt(),
        0xFF38BDF8.toInt(),
        OverlayRenderStyle.ARCADE_CANDY_3D
    ),
    LIQUID_QUARTZ_FROST(
        8,
        R.string.storm_ov20_skin_liquid_quartz,
        R.string.storm_ov20_desc_liquid_quartz,
        0xFFE2E8F0.toInt(),
        0xFF1E3A8A.toInt(),
        OverlayRenderStyle.LIQUID_QUARTZ
    ),
    RETRO_PIXEL_MATRIX(
        9,
        R.string.storm_ov20_skin_retro_pixel,
        R.string.storm_ov20_desc_retro_pixel,
        0xFF84CC16.toInt(),
        0xFF991B1B.toInt(),
        OverlayRenderStyle.RETRO_PIXEL
    );

    companion object {
        // Compatibility aliases for legacy enum constants so existing code compiles without breakage
        val CLASSIC_SWITCH get() = SWITCH_2_PRO_MAGNETIC
        val CYBERPUNK_NEON get() = CYBERPUNK_EDGE_NEON
        val FROST_GLASS_3D get() = LIQUID_QUARTZ_FROST
        val TITANIUM_MECHA get() = TITANIUM_MECHA_TACTICAL
        val ARCADE_CANDY_3D get() = ARCADE_CANDY_TACTILE_3D
        val FLAT_MINIMAL get() = OLED_GHOST_WIREFRAME
        val GOLDEN_ZELDA get() = HYRULE_RELIC_GOLD
        val CRIMSON_RED get() = SANCTUARY_BLOOD_RUNE
        val GAMEBOY_DMG_RETRO get() = RETRO_PIXEL_MATRIX

        /**
         * Resolves theme by ID with safe fallback for legacy stored preferences (0..14).
         */
        fun fromId(id: Int): OverlayTheme {
            return when (id) {
                0 -> STORM_AURORA_GLASS_3D
                1 -> SWITCH_2_PRO_MAGNETIC
                2 -> CYBERPUNK_EDGE_NEON
                3 -> OLED_GHOST_WIREFRAME
                4 -> TITANIUM_MECHA_TACTICAL
                5 -> HYRULE_RELIC_GOLD
                6 -> SANCTUARY_BLOOD_RUNE
                7 -> ARCADE_CANDY_TACTILE_3D
                8 -> LIQUID_QUARTZ_FROST
                9 -> RETRO_PIXEL_MATRIX
                // Safe legacy ID mappings:
                10 -> CYBERPUNK_EDGE_NEON
                11 -> OLED_GHOST_WIREFRAME
                12 -> HYRULE_RELIC_GOLD
                13 -> STORM_AURORA_GLASS_3D
                14 -> RETRO_PIXEL_MATRIX
                else -> STORM_AURORA_GLASS_3D
            }
        }
    }
}

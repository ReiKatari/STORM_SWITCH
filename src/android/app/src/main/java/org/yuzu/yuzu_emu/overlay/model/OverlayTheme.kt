// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.overlay.model

import org.yuzu.yuzu_emu.R

enum class OverlayRenderStyle {
    // 20.0.0 procedural styles
    AURORA_GLASS_3D,
    SWITCH_2_MAGNETIC,
    CYBERPUNK_EDGE,
    OLED_GHOST,
    TITANIUM_TACTICAL,
    HYRULE_RELIC,
    SANCTUARY_BLOOD,
    ARCADE_CANDY_3D,
    LIQUID_QUARTZ,
    RETRO_PIXEL,

    // 17.3.2 legacy styles
    NEON_GLOW,
    SWITCH_JOYCON,
    FROST_GLASS_3D,
    TITANIUM_MECHA,
    FLAT_MINIMAL,
    RETRO_CLASSIC,
    GOLDEN_ORNATE;

    companion object {
        // Compatibility aliases for legacy enum constants so existing code compiles without breakage
        val CLASSIC_SWITCH get() = SWITCH_2_MAGNETIC
        val CYBERPUNK_NEON get() = CYBERPUNK_EDGE
        val TITANIUM get() = TITANIUM_TACTICAL
        val GOLDEN_ZELDA get() = HYRULE_RELIC
        val GAMEBOY_DMG get() = RETRO_PIXEL
    }
}

enum class OverlayTheme(
    val id: Int,
    val titleResId: Int,
    val descResId: Int = 0,
    val colorDefault: Int,
    val colorPressed: Int,
    val renderStyle: OverlayRenderStyle,
    val isProceduralV20: Boolean = false
) {
    // 10 Procedural 20.0.0 skins FIRST in declaration order
    STORM_AURORA_GLASS_3D(
        15,
        R.string.storm_ov20_skin_storm_aurora_glass,
        R.string.storm_ov20_desc_aurora_glass,
        0xFF00E5FF.toInt(),
        0xFF6366F1.toInt(),
        OverlayRenderStyle.AURORA_GLASS_3D,
        true
    ),
    SWITCH_2_PRO_MAGNETIC(
        16,
        R.string.storm_ov20_skin_switch_2_magnetic,
        R.string.storm_ov20_desc_switch_2_magnetic,
        0xFF00C3E3.toInt(),
        0xFFFF4B3A.toInt(),
        OverlayRenderStyle.SWITCH_2_MAGNETIC,
        true
    ),
    CYBERPUNK_EDGE_NEON(
        17,
        R.string.storm_ov20_skin_cyberpunk_edge,
        R.string.storm_ov20_desc_cyberpunk_edge,
        0xFF06B6D4.toInt(),
        0xFFFACC15.toInt(),
        OverlayRenderStyle.CYBERPUNK_EDGE,
        true
    ),
    OLED_GHOST_WIREFRAME(
        18,
        R.string.storm_ov20_skin_oled_ghost,
        R.string.storm_ov20_desc_oled_ghost,
        0xAA94A3B8.toInt(),
        0xFFFFFFFF.toInt(),
        OverlayRenderStyle.OLED_GHOST,
        true
    ),
    TITANIUM_MECHA_TACTICAL(
        19,
        R.string.storm_ov20_skin_titanium_tactical,
        R.string.storm_ov20_desc_titanium_tactical,
        0xFF10B981.toInt(),
        0xFFF59E0B.toInt(),
        OverlayRenderStyle.TITANIUM_TACTICAL,
        true
    ),
    HYRULE_RELIC_GOLD(
        20,
        R.string.storm_ov20_skin_hyrule_relic,
        R.string.storm_ov20_desc_hyrule_relic,
        0xFFF59E0B.toInt(),
        0xFF14B8A6.toInt(),
        OverlayRenderStyle.HYRULE_RELIC,
        true
    ),
    SANCTUARY_BLOOD_RUNE(
        21,
        R.string.storm_ov20_skin_sanctuary_blood,
        R.string.storm_ov20_desc_sanctuary_blood,
        0xFFDC2626.toInt(),
        0xFFEF4444.toInt(),
        OverlayRenderStyle.SANCTUARY_BLOOD,
        true
    ),
    ARCADE_CANDY_TACTILE_3D(
        22,
        R.string.storm_ov20_skin_arcade_candy,
        R.string.storm_ov20_desc_arcade_candy,
        0xFFEC4899.toInt(),
        0xFF38BDF8.toInt(),
        OverlayRenderStyle.ARCADE_CANDY_3D,
        true
    ),
    LIQUID_QUARTZ_FROST(
        23,
        R.string.storm_ov20_skin_liquid_quartz,
        R.string.storm_ov20_desc_liquid_quartz,
        0xFFE2E8F0.toInt(),
        0xFF1E3A8A.toInt(),
        OverlayRenderStyle.LIQUID_QUARTZ,
        true
    ),
    RETRO_PIXEL_MATRIX(
        24,
        R.string.storm_ov20_skin_retro_pixel,
        R.string.storm_ov20_desc_retro_pixel,
        0xFF84CC16.toInt(),
        0xFF991B1B.toInt(),
        OverlayRenderStyle.RETRO_PIXEL,
        true
    ),

    // 15 Existing 17.3.2 skins (IDs 0..14)
    CYBERPUNK_NEON(0, R.string.skin_cyberpunk_neon, 0, 0xFF00F2FE.toInt(), 0xFFFF007F.toInt(), OverlayRenderStyle.NEON_GLOW, false),
    CLASSIC_SWITCH(1, R.string.skin_classic_switch, 0, 0xFF00C3E3.toInt(), 0xFFFF3C28.toInt(), OverlayRenderStyle.SWITCH_JOYCON, false),
    FROST_GLASS_3D(2, R.string.skin_frost_glass_3d, 0, 0xFFE0F2FE.toInt(), 0xFF38BDF8.toInt(), OverlayRenderStyle.FROST_GLASS_3D, false),
    TITANIUM_MECHA(3, R.string.skin_titanium_mecha, 0, 0xFF94A3B8.toInt(), 0xFF00E5FF.toInt(), OverlayRenderStyle.TITANIUM_MECHA, false),
    ARCADE_CANDY_3D(4, R.string.skin_arcade_candy_3d, 0, 0xFFF43F5E.toInt(), 0xFFFBBF24.toInt(), OverlayRenderStyle.ARCADE_CANDY_3D, false),
    FLAT_MINIMAL(5, R.string.skin_flat_minimal, 0, 0xFFF1F5F9.toInt(), 0xFF00E5FF.toInt(), OverlayRenderStyle.FLAT_MINIMAL, false),
    STORM_DS_CLASSIC(6, R.string.skin_storm_ds_classic, 0, 0xFFE2E8F0.toInt(), 0xFF00E5FF.toInt(), OverlayRenderStyle.TITANIUM_MECHA, false),
    DS_LITE_WHITE(7, R.string.skin_ds_lite_white, 0, 0xFFF8FAFC.toInt(), 0xFF38BDF8.toInt(), OverlayRenderStyle.FROST_GLASS_3D, false),
    DSI_XL_BLUE(8, R.string.skin_dsi_xl_blue, 0, 0xFF2563EB.toInt(), 0xFF67E8F9.toInt(), OverlayRenderStyle.TITANIUM_MECHA, false),
    CRIMSON_RED(9, R.string.skin_crimson_red, 0, 0xFFEF4444.toInt(), 0xFFF59E0B.toInt(), OverlayRenderStyle.ARCADE_CANDY_3D, false),
    N3DS_AQUA(10, R.string.skin_n3ds_aqua, 0, 0xFF06B6D4.toInt(), 0xFF10B981.toInt(), OverlayRenderStyle.NEON_GLOW, false),
    OLED_STEALTH(11, R.string.skin_oled_stealth, 0, 0xFF64748B.toInt(), 0xFFFFFFFF.toInt(), OverlayRenderStyle.FLAT_MINIMAL, false),
    GOLDEN_ZELDA(12, R.string.skin_golden_zelda, 0, 0xFFF59E0B.toInt(), 0xFFFB923C.toInt(), OverlayRenderStyle.GOLDEN_ORNATE, false),
    GAMEBOY_COLOR_PURPLE(13, R.string.skin_gameboy_color_purple, 0, 0xFFA855F7.toInt(), 0xFFEC4899.toInt(), OverlayRenderStyle.FROST_GLASS_3D, false),
    GAMEBOY_DMG_RETRO(14, R.string.skin_gameboy_dmg_retro, 0, 0xFF84CC16.toInt(), 0xFF991B1B.toInt(), OverlayRenderStyle.RETRO_CLASSIC, false);

    companion object {
        fun fromId(id: Int): OverlayTheme = values().firstOrNull { it.id == id } ?: STORM_AURORA_GLASS_3D
    }
}

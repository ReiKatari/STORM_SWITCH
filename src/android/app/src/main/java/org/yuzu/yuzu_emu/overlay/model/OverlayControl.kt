// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.overlay.model

import androidx.annotation.IntegerRes
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.YuzuApplication

enum class OverlayControl(
    val id: String,
    val defaultVisibility: Boolean,
    @IntegerRes val defaultLandscapePositionResources: Pair<Int, Int>,
    @IntegerRes val defaultPortraitPositionResources: Pair<Int, Int>,
    @IntegerRes val defaultFoldablePositionResources: Pair<Int, Int>,
    val defaultIndividualScaleResource: Float,
) {
    // 12 Visible By Default Primary Controls
    BUTTON_A(
        "button_a",
        true,
        Pair(R.integer.BUTTON_A_X, R.integer.BUTTON_A_Y),
        Pair(R.integer.BUTTON_A_X_PORTRAIT, R.integer.BUTTON_A_Y_PORTRAIT),
        Pair(R.integer.BUTTON_A_X_FOLDABLE, R.integer.BUTTON_A_Y_FOLDABLE),
        1.0f
    ),
    BUTTON_B(
        "button_b",
        true,
        Pair(R.integer.BUTTON_B_X, R.integer.BUTTON_B_Y),
        Pair(R.integer.BUTTON_B_X_PORTRAIT, R.integer.BUTTON_B_Y_PORTRAIT),
        Pair(R.integer.BUTTON_B_X_FOLDABLE, R.integer.BUTTON_B_Y_FOLDABLE),
        1.0f
    ),
    BUTTON_X(
        "button_x",
        true,
        Pair(R.integer.BUTTON_X_X, R.integer.BUTTON_X_Y),
        Pair(R.integer.BUTTON_X_X_PORTRAIT, R.integer.BUTTON_X_Y_PORTRAIT),
        Pair(R.integer.BUTTON_X_X_FOLDABLE, R.integer.BUTTON_X_Y_FOLDABLE),
        1.0f
    ),
    BUTTON_Y(
        "button_y",
        true,
        Pair(R.integer.BUTTON_Y_X, R.integer.BUTTON_Y_Y),
        Pair(R.integer.BUTTON_Y_X_PORTRAIT, R.integer.BUTTON_Y_Y_PORTRAIT),
        Pair(R.integer.BUTTON_Y_X_FOLDABLE, R.integer.BUTTON_Y_Y_FOLDABLE),
        1.0f
    ),
    BUTTON_PLUS(
        "button_plus",
        true,
        Pair(R.integer.BUTTON_PLUS_X, R.integer.BUTTON_PLUS_Y),
        Pair(R.integer.BUTTON_PLUS_X_PORTRAIT, R.integer.BUTTON_PLUS_Y_PORTRAIT),
        Pair(R.integer.BUTTON_PLUS_X_FOLDABLE, R.integer.BUTTON_PLUS_Y_FOLDABLE),
        1.0f
    ),
    BUTTON_MINUS(
        "button_minus",
        true,
        Pair(R.integer.BUTTON_MINUS_X, R.integer.BUTTON_MINUS_Y),
        Pair(R.integer.BUTTON_MINUS_X_PORTRAIT, R.integer.BUTTON_MINUS_Y_PORTRAIT),
        Pair(R.integer.BUTTON_MINUS_X_FOLDABLE, R.integer.BUTTON_MINUS_Y_FOLDABLE),
        1.0f
    ),
    BUTTON_L(
        "button_l",
        true,
        Pair(R.integer.BUTTON_L_X, R.integer.BUTTON_L_Y),
        Pair(R.integer.BUTTON_L_X_PORTRAIT, R.integer.BUTTON_L_Y_PORTRAIT),
        Pair(R.integer.BUTTON_L_X_FOLDABLE, R.integer.BUTTON_L_Y_FOLDABLE),
        1.0f
    ),
    BUTTON_R(
        "button_r",
        true,
        Pair(R.integer.BUTTON_R_X, R.integer.BUTTON_R_Y),
        Pair(R.integer.BUTTON_R_X_PORTRAIT, R.integer.BUTTON_R_Y_PORTRAIT),
        Pair(R.integer.BUTTON_R_X_FOLDABLE, R.integer.BUTTON_R_Y_FOLDABLE),
        1.0f
    ),
    BUTTON_ZL(
        "button_zl",
        true,
        Pair(R.integer.BUTTON_ZL_X, R.integer.BUTTON_ZL_Y),
        Pair(R.integer.BUTTON_ZL_X_PORTRAIT, R.integer.BUTTON_ZL_Y_PORTRAIT),
        Pair(R.integer.BUTTON_ZL_X_FOLDABLE, R.integer.BUTTON_ZL_Y_FOLDABLE),
        1.0f
    ),
    BUTTON_ZR(
        "button_zr",
        true,
        Pair(R.integer.BUTTON_ZR_X, R.integer.BUTTON_ZR_Y),
        Pair(R.integer.BUTTON_ZR_X_PORTRAIT, R.integer.BUTTON_ZR_Y_PORTRAIT),
        Pair(R.integer.BUTTON_ZR_X_FOLDABLE, R.integer.BUTTON_ZR_Y_FOLDABLE),
        1.0f
    ),
    BUTTON_STICK_L(
        "button_stick_l",
        true,
        Pair(R.integer.BUTTON_STICK_L_X, R.integer.BUTTON_STICK_L_Y),
        Pair(R.integer.BUTTON_STICK_L_X_PORTRAIT, R.integer.BUTTON_STICK_L_Y_PORTRAIT),
        Pair(R.integer.BUTTON_STICK_L_X_FOLDABLE, R.integer.BUTTON_STICK_L_Y_FOLDABLE),
        1.0f
    ),
    BUTTON_STICK_R(
        "button_stick_r",
        true,
        Pair(R.integer.BUTTON_STICK_R_X, R.integer.BUTTON_STICK_R_Y),
        Pair(R.integer.BUTTON_STICK_R_X_PORTRAIT, R.integer.BUTTON_STICK_R_Y_PORTRAIT),
        Pair(R.integer.BUTTON_STICK_R_X_FOLDABLE, R.integer.BUTTON_STICK_R_Y_FOLDABLE),
        1.0f
    ),
    STICK_L(
        "stick_l",
        true,
        Pair(R.integer.STICK_L_X, R.integer.STICK_L_Y),
        Pair(R.integer.STICK_L_X_PORTRAIT, R.integer.STICK_L_Y_PORTRAIT),
        Pair(R.integer.STICK_L_X_FOLDABLE, R.integer.STICK_L_Y_FOLDABLE),
        1.0f
    ),
    STICK_R(
        "stick_r",
        true,
        Pair(R.integer.STICK_R_X, R.integer.STICK_R_Y),
        Pair(R.integer.STICK_R_X_PORTRAIT, R.integer.STICK_R_Y_PORTRAIT),
        Pair(R.integer.STICK_R_X_FOLDABLE, R.integer.STICK_R_Y_FOLDABLE),
        1.0f
    ),
    COMBINED_DPAD(
        "dpad",
        true,
        Pair(R.integer.COMBINED_DPAD_X, R.integer.COMBINED_DPAD_Y),
        Pair(R.integer.COMBINED_DPAD_X_PORTRAIT, R.integer.COMBINED_DPAD_Y_PORTRAIT),
        Pair(R.integer.COMBINED_DPAD_X_FOLDABLE, R.integer.COMBINED_DPAD_Y_FOLDABLE),
        1.0f
    ),

    // Hidden By Default Service & Extra Controls
    BUTTON_HOME(
        "button_home",
        false,
        Pair(R.integer.BUTTON_HOME_X, R.integer.BUTTON_HOME_Y),
        Pair(R.integer.BUTTON_HOME_X_PORTRAIT, R.integer.BUTTON_HOME_Y_PORTRAIT),
        Pair(R.integer.BUTTON_HOME_X_FOLDABLE, R.integer.BUTTON_HOME_Y_FOLDABLE),
        1.0f
    ),
    BUTTON_CAPTURE(
        "button_capture",
        false,
        Pair(R.integer.BUTTON_CAPTURE_X, R.integer.BUTTON_CAPTURE_Y),
        Pair(R.integer.BUTTON_CAPTURE_X_PORTRAIT, R.integer.BUTTON_CAPTURE_Y_PORTRAIT),
        Pair(R.integer.BUTTON_CAPTURE_X_FOLDABLE, R.integer.BUTTON_CAPTURE_Y_FOLDABLE),
        1.0f
    ),
    BUTTON_GL(
        "button_gl",
        false,
        Pair(R.integer.storm_ov20_btn_gl_x, R.integer.storm_ov20_btn_gl_y),
        Pair(R.integer.storm_ov20_btn_gl_x_portrait, R.integer.storm_ov20_btn_gl_y_portrait),
        Pair(R.integer.storm_ov20_btn_gl_x_foldable, R.integer.storm_ov20_btn_gl_y_foldable),
        1.0f
    ),
    BUTTON_GR(
        "button_gr",
        false,
        Pair(R.integer.storm_ov20_btn_gr_x, R.integer.storm_ov20_btn_gr_y),
        Pair(R.integer.storm_ov20_btn_gr_x_portrait, R.integer.storm_ov20_btn_gr_y_portrait),
        Pair(R.integer.storm_ov20_btn_gr_x_foldable, R.integer.storm_ov20_btn_gr_y_foldable),
        1.0f
    ),
    BUTTON_SL(
        "button_sl",
        false,
        Pair(R.integer.storm_ov20_btn_sl_x, R.integer.storm_ov20_btn_sl_y),
        Pair(R.integer.storm_ov20_btn_sl_x_portrait, R.integer.storm_ov20_btn_sl_y_portrait),
        Pair(R.integer.storm_ov20_btn_sl_x_foldable, R.integer.storm_ov20_btn_sl_y_foldable),
        1.0f
    ),
    BUTTON_SR(
        "button_sr",
        false,
        Pair(R.integer.storm_ov20_btn_sr_x, R.integer.storm_ov20_btn_sr_y),
        Pair(R.integer.storm_ov20_btn_sr_x_portrait, R.integer.storm_ov20_btn_sr_y_portrait),
        Pair(R.integer.storm_ov20_btn_sr_x_foldable, R.integer.storm_ov20_btn_sr_y_foldable),
        1.0f
    ),
    BUTTON_REWIND(
        "button_rewind",
        false,
        Pair(R.integer.storm_ov20_btn_rewind_x, R.integer.storm_ov20_btn_rewind_y),
        Pair(R.integer.storm_ov20_btn_rewind_x_portrait, R.integer.storm_ov20_btn_rewind_y_portrait),
        Pair(R.integer.storm_ov20_btn_rewind_x_foldable, R.integer.storm_ov20_btn_rewind_y_foldable),
        1.0f
    ),
    BUTTON_FAST_FORWARD(
        "button_fast_forward",
        false,
        Pair(R.integer.storm_ov20_btn_fast_forward_x, R.integer.storm_ov20_btn_fast_forward_y),
        Pair(R.integer.storm_ov20_btn_fast_forward_x_portrait, R.integer.storm_ov20_btn_fast_forward_y_portrait),
        Pair(R.integer.storm_ov20_btn_fast_forward_x_foldable, R.integer.storm_ov20_btn_fast_forward_y_foldable),
        1.0f
    ),
    BUTTON_FRAME_STEP(
        "button_frame_step",
        false,
        Pair(R.integer.storm_ov20_btn_frame_step_x, R.integer.storm_ov20_btn_frame_step_y),
        Pair(R.integer.storm_ov20_btn_frame_step_x_portrait, R.integer.storm_ov20_btn_frame_step_y_portrait),
        Pair(R.integer.storm_ov20_btn_frame_step_x_foldable, R.integer.storm_ov20_btn_frame_step_y_foldable),
        1.0f
    ),
    BUTTON_QUICK_SAVE(
        "button_quick_save",
        false,
        Pair(R.integer.storm_ov20_btn_quick_save_x, R.integer.storm_ov20_btn_quick_save_y),
        Pair(R.integer.storm_ov20_btn_quick_save_x_portrait, R.integer.storm_ov20_btn_quick_save_y_portrait),
        Pair(R.integer.storm_ov20_btn_quick_save_x_foldable, R.integer.storm_ov20_btn_quick_save_y_foldable),
        1.0f
    ),
    BUTTON_QUICK_LOAD(
        "button_quick_load",
        false,
        Pair(R.integer.storm_ov20_btn_quick_load_x, R.integer.storm_ov20_btn_quick_load_y),
        Pair(R.integer.storm_ov20_btn_quick_load_x_portrait, R.integer.storm_ov20_btn_quick_load_y_portrait),
        Pair(R.integer.storm_ov20_btn_quick_load_x_foldable, R.integer.storm_ov20_btn_quick_load_y_foldable),
        1.0f
    ),
    BUTTON_AI_TRANSLATOR(
        "button_ai_translator",
        false,
        Pair(R.integer.storm_ov20_btn_ai_translator_x, R.integer.storm_ov20_btn_ai_translator_y),
        Pair(R.integer.storm_ov20_btn_ai_translator_x_portrait, R.integer.storm_ov20_btn_ai_translator_y_portrait),
        Pair(R.integer.storm_ov20_btn_ai_translator_x_foldable, R.integer.storm_ov20_btn_ai_translator_y_foldable),
        1.0f
    ),
    BUTTON_AUTO_FIX(
        "button_auto_fix",
        false,
        Pair(R.integer.storm_ov20_btn_auto_fix_x, R.integer.storm_ov20_btn_auto_fix_y),
        Pair(R.integer.storm_ov20_btn_auto_fix_x_portrait, R.integer.storm_ov20_btn_auto_fix_y_portrait),
        Pair(R.integer.storm_ov20_btn_auto_fix_x_foldable, R.integer.storm_ov20_btn_auto_fix_y_foldable),
        1.0f
    ),
    BUTTON_TURBO_HOLD(
        "button_turbo_hold",
        false,
        Pair(R.integer.storm_ov20_btn_turbo_hold_x, R.integer.storm_ov20_btn_turbo_hold_y),
        Pair(R.integer.storm_ov20_btn_turbo_hold_x_portrait, R.integer.storm_ov20_btn_turbo_hold_y_portrait),
        Pair(R.integer.storm_ov20_btn_turbo_hold_x_foldable, R.integer.storm_ov20_btn_turbo_hold_y_foldable),
        1.0f
    ),
    BUTTON_PAUSE_COOLING(
        "button_pause_cooling",
        false,
        Pair(R.integer.storm_ov20_btn_pause_cooling_x, R.integer.storm_ov20_btn_pause_cooling_y),
        Pair(R.integer.storm_ov20_btn_pause_cooling_x_portrait, R.integer.storm_ov20_btn_pause_cooling_y_portrait),
        Pair(R.integer.storm_ov20_btn_pause_cooling_x_foldable, R.integer.storm_ov20_btn_pause_cooling_y_foldable),
        1.0f
    ),
    BUTTON_GAME_CHAT(
        "button_game_chat",
        false,
        Pair(R.integer.storm_ov20_btn_game_chat_x, R.integer.storm_ov20_btn_game_chat_y),
        Pair(R.integer.storm_ov20_btn_game_chat_x_portrait, R.integer.storm_ov20_btn_game_chat_y_portrait),
        Pair(R.integer.storm_ov20_btn_game_chat_x_foldable, R.integer.storm_ov20_btn_game_chat_y_foldable),
        1.0f
    ),
    BUTTON_GYRO_TOGGLE(
        "button_gyro_toggle",
        false,
        Pair(R.integer.storm_ov20_btn_gyro_toggle_x, R.integer.storm_ov20_btn_gyro_toggle_y),
        Pair(R.integer.storm_ov20_btn_gyro_toggle_x_portrait, R.integer.storm_ov20_btn_gyro_toggle_y_portrait),
        Pair(R.integer.storm_ov20_btn_gyro_toggle_x_foldable, R.integer.storm_ov20_btn_gyro_toggle_y_foldable),
        1.0f
    ),
    BUTTON_MOUSE_MODE(
        "button_mouse_mode",
        false,
        Pair(R.integer.storm_ov20_btn_mouse_mode_x, R.integer.storm_ov20_btn_mouse_mode_y),
        Pair(R.integer.storm_ov20_btn_mouse_mode_x_portrait, R.integer.storm_ov20_btn_mouse_mode_y_portrait),
        Pair(R.integer.storm_ov20_btn_mouse_mode_x_foldable, R.integer.storm_ov20_btn_mouse_mode_y_foldable),
        1.0f
    ),
    BUTTON_MACRO_M1(
        "button_macro_m1",
        false,
        Pair(R.integer.storm_ov20_btn_macro_m1_x, R.integer.storm_ov20_btn_macro_m1_y),
        Pair(R.integer.storm_ov20_btn_macro_m1_x_portrait, R.integer.storm_ov20_btn_macro_m1_y_portrait),
        Pair(R.integer.storm_ov20_btn_macro_m1_x_foldable, R.integer.storm_ov20_btn_macro_m1_y_foldable),
        1.0f
    ),
    BUTTON_HIDE_HUD(
        "button_hide_hud",
        false,
        Pair(R.integer.storm_ov20_btn_hide_hud_x, R.integer.storm_ov20_btn_hide_hud_y),
        Pair(R.integer.storm_ov20_btn_hide_hud_x_portrait, R.integer.storm_ov20_btn_hide_hud_y_portrait),
        Pair(R.integer.storm_ov20_btn_hide_hud_x_foldable, R.integer.storm_ov20_btn_hide_hud_y_foldable),
        1.0f
    );

    fun getDefaultPositionForLayout(layout: OverlayLayout): Pair<Double, Double> {
        val rawResourcePair: Pair<Int, Int> = when (layout) {
            OverlayLayout.Landscape -> defaultLandscapePositionResources
            OverlayLayout.Portrait -> defaultPortraitPositionResources
            OverlayLayout.Foldable -> defaultFoldablePositionResources
        }

        val res = YuzuApplication.appContext.resources
        val xRaw = try { res.getInteger(rawResourcePair.first) } catch (_: Exception) { 500 }
        val yRaw = try { res.getInteger(rawResourcePair.second) } catch (_: Exception) { 500 }

        return Pair(
            xRaw.toDouble() / 1000.0,
            yRaw.toDouble() / 1000.0
        )
    }

    fun toOverlayControlData(): OverlayControlData =
        OverlayControlData(
            id,
            defaultVisibility,
            getDefaultPositionForLayout(OverlayLayout.Landscape),
            getDefaultPositionForLayout(OverlayLayout.Portrait),
            getDefaultPositionForLayout(OverlayLayout.Foldable),
            defaultIndividualScaleResource,
            1.0f
        )

    companion object {
        val map: HashMap<String, OverlayControl> by lazy {
            val hashMap = hashMapOf<String, OverlayControl>()
            entries.forEach { hashMap[it.id] = it }
            hashMap
        }

        fun from(id: String): OverlayControl? = map[id]
    }
}

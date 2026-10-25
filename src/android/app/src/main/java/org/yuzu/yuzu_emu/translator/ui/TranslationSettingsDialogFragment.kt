// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.translator.ui

import android.app.Dialog
import android.content.res.Configuration
import android.os.Bundle
import android.widget.ArrayAdapter
import androidx.fragment.app.DialogFragment
import androidx.preference.PreferenceManager
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.databinding.DialogTranslationSettingsBinding
import org.yuzu.yuzu_emu.translator.model.TranslatorEngineType
import org.yuzu.yuzu_emu.translator.model.TranslatorOverlayStyle
import org.yuzu.yuzu_emu.translator.tts.GameTtsManager

class TranslationSettingsDialogFragment : DialogFragment() {

    private var _binding: DialogTranslationSettingsBinding? = null
    private val binding get() = _binding!!
    private var ttsManager: GameTtsManager? = null

    var onSettingsChanged: (() -> Unit)? = null
    var onConfigureRegionsRequested: (() -> Unit)? = null

    companion object {
        const val TAG = "TranslationSettingsDialogFragment"
        fun newInstance(): TranslationSettingsDialogFragment = TranslationSettingsDialogFragment()
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        _binding = DialogTranslationSettingsBinding.inflate(layoutInflater)
        ttsManager = GameTtsManager(requireContext())

        setupUI()

        binding.btnCloseSettings.setOnClickListener {
            savePreferences()
            onSettingsChanged?.invoke()
            dismiss()
        }

        return MaterialAlertDialogBuilder(requireContext(), R.style.EdenMaterialDialog)
            .setView(binding.root)
            .create()
    }

    override fun onStart() {
        super.onStart()
        dialog?.window?.let { window ->
            val dm = resources.displayMetrics
            val isLandscape = resources.configuration.orientation == Configuration.ORIENTATION_LANDSCAPE
            val width = if (isLandscape) (dm.widthPixels * 0.70).toInt() else (dm.widthPixels * 0.92).toInt()
            val height = if (isLandscape) (dm.heightPixels * 0.90).toInt() else (dm.heightPixels * 0.85).toInt()
            window.setLayout(width, height)
            window.setBackgroundDrawableResource(android.R.color.transparent)
        }
    }

    private fun setupUI() {
        val prefs = PreferenceManager.getDefaultSharedPreferences(requireContext())

        // 1. Engine
        val engines = listOf(
            "google" to getString(R.string.translator_engine_opt_google),
            "yandex" to getString(R.string.translator_engine_opt_yandex),
            "lingva" to getString(R.string.translator_engine_opt_lingva),
            "deepl" to getString(R.string.translator_engine_opt_deepl),
            "libre" to getString(R.string.translator_engine_opt_libre),
            "mymemory" to getString(R.string.translator_engine_opt_mymemory),
            "custom_ai" to getString(R.string.translator_engine_opt_custom_ai)
        )
        val engineAdapter = ArrayAdapter(requireContext(), R.layout.spinner_item_eden, engines.map { it.second }).apply {
            setDropDownViewResource(R.layout.spinner_dropdown_item_eden)
        }
        binding.spinnerEngine.adapter = engineAdapter
        val curEngine = prefs.getString("translator_engine", "google") ?: "google"
        val engineIdx = engines.indexOfFirst { it.first == curEngine }.coerceAtLeast(0)
        binding.spinnerEngine.setSelection(engineIdx)

        // 2. Source Lang
        val sourceLangs = listOf(
            "auto" to getString(R.string.translator_lang_auto),
            "ja" to getString(R.string.translator_lang_ja),
            "en" to getString(R.string.translator_lang_en),
            "zh" to getString(R.string.translator_lang_zh),
            "ko" to getString(R.string.translator_lang_ko),
            "es" to getString(R.string.translator_lang_es),
            "fr" to getString(R.string.translator_lang_fr),
            "de" to getString(R.string.translator_lang_de),
            "it" to getString(R.string.translator_lang_it)
        )
        val sourceAdapter = ArrayAdapter(requireContext(), R.layout.spinner_item_eden, sourceLangs.map { it.second }).apply {
            setDropDownViewResource(R.layout.spinner_dropdown_item_eden)
        }
        binding.spinnerSourceLang.adapter = sourceAdapter
        val curSource = prefs.getString("translator_source_lang", "auto") ?: "auto"
        val sourceIdx = sourceLangs.indexOfFirst { it.first == curSource }.coerceAtLeast(0)
        binding.spinnerSourceLang.setSelection(sourceIdx)

        // 3. Target Lang
        val targetLangs = listOf(
            "ru" to getString(R.string.translator_lang_ru),
            "en" to getString(R.string.translator_lang_en),
            "es" to getString(R.string.translator_lang_es),
            "de" to getString(R.string.translator_lang_de),
            "fr" to getString(R.string.translator_lang_fr),
            "it" to getString(R.string.translator_lang_it),
            "zh" to getString(R.string.translator_lang_zh),
            "ja" to getString(R.string.translator_lang_ja)
        )
        val targetAdapter = ArrayAdapter(requireContext(), R.layout.spinner_item_eden, targetLangs.map { it.second }).apply {
            setDropDownViewResource(R.layout.spinner_dropdown_item_eden)
        }
        binding.spinnerTargetLang.adapter = targetAdapter
        val curTarget = prefs.getString("translator_target_lang", "ru") ?: "ru"
        val targetIdx = targetLangs.indexOfFirst { it.first == curTarget }.coerceAtLeast(0)
        binding.spinnerTargetLang.setSelection(targetIdx)

        // 4. Style
        val styles = listOf(
            "smart_background_match" to getString(R.string.translator_style_native),
            "semi_transparent" to getString(R.string.translator_style_dark),
            "translucent_bubble" to getString(R.string.translator_style_neon),
            "outline_only" to getString(R.string.translator_style_outline)
        )
        val styleAdapter = ArrayAdapter(requireContext(), R.layout.spinner_item_eden, styles.map { it.second }).apply {
            setDropDownViewResource(R.layout.spinner_dropdown_item_eden)
        }
        binding.spinnerStyle.adapter = styleAdapter
        val curStyle = prefs.getString("translator_overlay_style", "smart_background_match") ?: "smart_background_match"
        val styleIdx = styles.indexOfFirst { it.first == curStyle }.coerceAtLeast(0)
        binding.spinnerStyle.setSelection(styleIdx)

        // 5. Scan Region
        val regions = listOf(
            "all" to getString(R.string.translator_roi_all),
            "bottom" to getString(R.string.translator_roi_bottom),
            "center" to getString(R.string.translator_roi_center),
            "top" to getString(R.string.translator_roi_top),
            "custom" to getString(R.string.translator_roi_custom)
        )
        val regionAdapter = ArrayAdapter(requireContext(), R.layout.spinner_item_eden, regions.map { it.second }).apply {
            setDropDownViewResource(R.layout.spinner_dropdown_item_eden)
        }
        binding.spinnerScanRegion.adapter = regionAdapter
        val curRegion = prefs.getString("translator_scan_region", "all") ?: "all"
        val regionIdx = regions.indexOfFirst { it.first == curRegion }.coerceAtLeast(0)
        binding.spinnerScanRegion.setSelection(regionIdx)

        binding.buttonConfigureRegions.setOnClickListener {
            savePreferences()
            dismiss()
            onConfigureRegionsRequested?.invoke()
        }

        // 6. TTS & Smart Voices
        val autoSpeak = prefs.getBoolean("translator_auto_speak", false)
        binding.switchAutoTts.isChecked = autoSpeak
        val smartVoices = prefs.getBoolean("translator_smart_voices", true)
        binding.switchSmartVoices.isChecked = smartVoices

        val ttsSpeed = prefs.getFloat("translator_tts_speed", 1.0f)
        binding.sliderTtsSpeed.value = ttsSpeed.coerceIn(0.5f, 2.0f)
        binding.textTtsSpeed.text = getString(R.string.translator_lbl_tts_speed_fmt, String.format("%.1f", ttsSpeed))
        binding.sliderTtsSpeed.addOnChangeListener { _, value, _ ->
            binding.textTtsSpeed.text = getString(R.string.translator_lbl_tts_speed_fmt, String.format("%.1f", value))
        }

        binding.buttonTestTts.setOnClickListener {
            savePreferences()
            val target = prefs.getString("translator_target_lang", "ru") ?: "ru"
            val phrase = if (target == "ru") "Голосовая озвучка STORM SWITCH успешно активирована." else "STORM SWITCH text-to-speech is ready."
            ttsManager?.speak(phrase, "Zelda")
        }

        // 7. Floating button
        val enableFloating = prefs.getBoolean("translator_enable_floating_button", true)
        binding.switchFloatingBtn.isChecked = enableFloating
        val opacity = prefs.getInt("translator_floating_btn_opacity", 90)
        binding.sliderFloatingOpacity.value = opacity.toFloat().coerceIn(20f, 100f)
        binding.textFloatingOpacity.text = getString(R.string.translator_lbl_floating_opacity_fmt, opacity)
        binding.sliderFloatingOpacity.addOnChangeListener { _, value, _ ->
            binding.textFloatingOpacity.text = getString(R.string.translator_lbl_floating_opacity_fmt, value.toInt())
        }
    }

    private fun savePreferences() {
        val prefs = PreferenceManager.getDefaultSharedPreferences(requireContext())

        val engines = listOf("google", "yandex", "lingva", "deepl", "libre", "mymemory", "custom_ai")
        val sourceLangs = listOf("auto", "ja", "en", "zh", "ko", "es", "fr", "de", "it")
        val targetLangs = listOf("ru", "en", "es", "de", "fr", "it", "zh", "ja")
        val styles = listOf("smart_background_match", "semi_transparent", "translucent_bubble", "outline_only")
        val regions = listOf("all", "bottom", "center", "top", "custom")

        val selEngine = engines.getOrElse(binding.spinnerEngine.selectedItemPosition) { "google" }
        val selSource = sourceLangs.getOrElse(binding.spinnerSourceLang.selectedItemPosition) { "auto" }
        val selTarget = targetLangs.getOrElse(binding.spinnerTargetLang.selectedItemPosition) { "ru" }
        val selStyle = styles.getOrElse(binding.spinnerStyle.selectedItemPosition) { "smart_background_match" }
        val selRegion = regions.getOrElse(binding.spinnerScanRegion.selectedItemPosition) { "all" }

        prefs.edit()
            .putString("translator_engine", selEngine)
            .putString("translator_source_lang", selSource)
            .putString("translator_target_lang", selTarget)
            .putString("translator_overlay_style", selStyle)
            .putString("translator_scan_region", selRegion)
            .putBoolean("translator_auto_speak", binding.switchAutoTts.isChecked)
            .putBoolean("translator_smart_voices", binding.switchSmartVoices.isChecked)
            .putFloat("translator_tts_speed", binding.sliderTtsSpeed.value)
            .putBoolean("translator_enable_floating_button", binding.switchFloatingBtn.isChecked)
            .putInt("translator_floating_btn_opacity", binding.sliderFloatingOpacity.value.toInt())
            .apply()
    }

    override fun onDestroy() {
        ttsManager?.shutdown()
        _binding = null
        super.onDestroy()
    }
}

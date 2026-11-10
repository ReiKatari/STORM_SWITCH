// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.view.KeyEvent

import android.app.Dialog
import android.content.DialogInterface
import android.content.res.Configuration
import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.fragment.app.DialogFragment
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.databinding.DialogGameFixBinding
import org.yuzu.yuzu_emu.model.Game
import org.yuzu.yuzu_emu.model.GameFixDatabase
import org.yuzu.yuzu_emu.utils.GameIconUtils
import org.yuzu.yuzu_emu.utils.ThemeHelper
import java.util.Locale

class GameFixDialogFragment : DialogFragment() {

    private var _binding: DialogGameFixBinding? = null
    private val binding get() = _binding!!

    private var game: Game? = null
    private var onLaunchCallback: ((GameFixDatabase.LaunchMode) -> Unit)? = null

    companion object {
        const val TAG = "GameFixDialogFragment"

        fun newInstance(game: Game, onLaunch: (GameFixDatabase.LaunchMode) -> Unit): GameFixDialogFragment {
            val fragment = GameFixDialogFragment()
            fragment.game = game
            fragment.onLaunchCallback = onLaunch
            return fragment
        }
    }

    private fun sanitizeText(str: String): String {
        if (str.isEmpty()) return str
        if (str.contains("вЂ") || str.contains("вњ") || str.contains("Р") || str.contains("С")) {
            return try {
                val bytes = str.toByteArray(Charsets.ISO_8859_1)
                val decoded = String(bytes, Charsets.UTF_8)
                if (decoded.contains("•") || decoded.contains("✓") || decoded.any { it in 'а'..'я' || it in 'А'..'Я' }) {
                    decoded
                } else {
                    str
                }
            } catch (_: Exception) {
                str
            }
        }
        return str
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val dialog = super.onCreateDialog(savedInstanceState)
        dialog.requestWindowFeature(android.view.Window.FEATURE_NO_TITLE)
        dialog.window?.let { window ->
            window.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
            WindowCompat.setDecorFitsSystemWindows(window, false)
        }
        return dialog
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = DialogGameFixBinding.inflate(inflater, container, false)
        return binding.root
    }

    
    private fun localizeFixesText(rawText: String): String {
        var text = rawText
        val keyMap = mapOf(
            "Airplane mode" to "Режим полёта (офлайн)",
            "airplane_mode" to "Режим полёта (офлайн)",
            "Vram garbage collection" to "Сборка мусора видеопамяти (VRAM)",
            "vram_garbage_collection" to "Сборка мусора видеопамяти (VRAM)",
            "Vram budget governor" to "Контроллер бюджета VRAM",
            "vram_budget_governor" to "Контроллер бюджета VRAM",
            "Vram usage mode" to "Режим использования видеопамяти",
            "vram_usage_mode" to "Режим использования видеопамяти",
            "Cpu clock" to "Тактовая частота ЦП",
            "Gpu clock" to "Тактовая частота ГПУ",
            "Anisotropy" to "Анизотропная фильтрация",
            "Backend" to "Графический бэкенд (Vulkan)",
            "Dyna state" to "Динамическое состояние Vulkan (EDS)",
            "Vertex input dynamic state" to "Динамический ввод вершин",
            "Drs resolution lock" to "Фиксация разрешения (DRS)",
            "Dynamic performance scaler" to "Динамическое масштабирование производительности",
            "Emulate bgr565" to "Эмуляция формата BGR565",
            "Enable frame skipping" to "Пропуск кадров для стабильности",
            "Fix bloom effects" to "Коррекция эффектов свечения (Bloom)",
            "Force max clock" to "Принудительная максимальная частота ГПУ",
            "Frame pacing mode" to "Режим синхронизации кадров (Pacing)",
            "Fsr sharpening slider" to "Резкость фильтра AMD FSR",
            "Rescale hack" to "Коррекция масштаба 2D-спрайтов",
            "Smart shader throttle" to "Умное дросселирование шейдеров",
            "Sync core speed" to "Синхронизация скорости ядер",
            "Use auto stub" to "Автоматические заглушки сервисов",
            "Use storm tier2 compiler" to "JIT-компилятор STORM Tier-2",
            "Use video framerate" to "Синхронизация частоты видео",
            "Cpuopt recompile exclusives" to "Перекомпиляция эксклюзивных инструкций",
            "Cpuopt unsafe host mmu" to "Прямой доступ Host MMU",
            "Cpuopt unsafe ignore standard fpcr" to "Игнорирование стандартного FPCR",
            "Cpuopt unsafe reduce fp error" to "Уменьшение погрешности плавающей точки",
            "Cpuopt unsafe unfuse fma" to "Разделение операций FMA",
            "Disable macro hle" to "Отключение HLE макросов",
            "Disable macro jit" to "Отключение JIT макросов",
            "Disable web applet" to "Отключение веб-апплета",
            "Language index" to "Язык системы Switch",
            "Region index" to "Регион системы Switch",
            "Custom" to "Пользовательский профиль",
            "Default" to "Значение по умолчанию",
            "Use global" to "Использовать глобальные настройки",
            "Driver path" to "Пользовательский драйвер GPU"
        )
        for ((k, v) in keyMap) {
            text = text.replace(k, v)
        }
        val valMap = mapOf(
            "Disabled" to "Выключено",
            "Enabled" to "Включено",
            "disabled" to "Выключено",
            "enabled" to "Включено"
        )
        for ((k, v) in valMap) {
            text = text.replace("<b>$k</b>", "<b>$v</b>")
            text = text.replace("→ $k", "→ $v")
        }
        return text
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        ViewCompat.setOnApplyWindowInsetsListener(binding.root) { v, insets ->
            val systemBars = insets.getInsets(
                WindowInsetsCompat.Type.systemBars() or WindowInsetsCompat.Type.displayCutout()
            )
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom)
            insets
        }
        binding.root.requestApplyInsets()

        val currentGame = game ?: run {
            dismissAllowingStateLoss()
            return
        }

        val profile = GameFixDatabase.getFix(currentGame)
        val hexId = GameFixDatabase.getProgramIdHex(currentGame)

        binding.checkboxDontAskAgain.isChecked = GameFixDatabase.isDontAskAgain(requireContext(), currentGame)
        binding.checkboxDontAskAgain.setOnCheckedChangeListener { _, isChecked ->
            context?.let { ctx ->
                GameFixDatabase.setDontAskAgain(ctx, currentGame, isChecked)
            }
        }

        binding.textGameFixTitle.text = currentGame.title
        val idString = "ID: $hexId"
        binding.textGameFixId.text = idString
        binding.textGameFixTitleId.text = idString
        val currentLang = resources.configuration.locales[0].language
        val isRu = currentLang.equals("ru", ignoreCase = true) || currentLang.equals("uk", ignoreCase = true) || currentLang.equals("be", ignoreCase = true)
        binding.textGameFixStatusBadge.text = if (isRu) "АВТО-ИСПРАВЛЕНИЕ" else "AUTO-FIX"

        val cleanVer = currentGame.version.trim().removePrefix("v").removePrefix("V").trim()
        try {
            binding.textGameFixVersion.visibility = if (cleanVer.isNotEmpty()) View.VISIBLE else View.GONE
            binding.textGameFixVersion.text = cleanVer
        } catch (_: Throwable) {}

        GameIconUtils.loadGameIcon(currentGame, binding.imageGameFixIcon)

        if (profile != null) {
            val currentLang = resources.configuration.locales[0].language
            val isRu = currentLang.equals("ru", ignoreCase = true) || currentLang.equals("uk", ignoreCase = true) || currentLang.equals("be", ignoreCase = true)
            val issues = if (isRu && profile.issuesRu.isNotEmpty()) profile.issuesRu else profile.issuesEn
            val fixes = GameFixDatabase.getFormattedFixes(profile, isRu)
            val localizedFixes = if (isRu) localizeFixesText(fixes) else fixes
            val issuesClean = sanitizeText(issues)
            val fixesHtml = androidx.core.text.HtmlCompat.fromHtml(
                sanitizeText(localizedFixes).replace("\n", "<br>"),
                androidx.core.text.HtmlCompat.FROM_HTML_MODE_LEGACY
            )
            binding.textGameFixIssues.text = issuesClean
            binding.textGameFixRecommendations.text = fixesHtml
            binding.textGameFixRecommended.text = fixesHtml
        }

        context?.let { ctx ->
            binding.checkboxDontAskAgain.isChecked = GameFixDatabase.isDontAskAgain(ctx, currentGame)
        }

        val hasUserCustom = GameFixDatabase.isUserCustomConfig(currentGame)
        if (hasUserCustom) {
            binding.cardLaunchCustom.strokeWidth = (2.2f * resources.displayMetrics.density).toInt()
            binding.cardLaunchCustom.strokeColor = android.graphics.Color.parseColor("#00E5FF")
        }

        binding.cardLaunchAutoFix.setOnClickListener {
            val ctx = context
            handleLaunch(GameFixDatabase.LaunchMode.AUTO_FIX) {
                GameFixDatabase.selectedLaunchMode = GameFixDatabase.LaunchMode.AUTO_FIX
                GameFixDatabase.applyCleanFix(currentGame)
                if (ctx != null) {
                    val msg = ctx.getString(R.string.game_fix_toast_autofix_clean)
                    Toast.makeText(ctx, msg, Toast.LENGTH_SHORT).show()
                }
            }
        }

        binding.cardLaunchAutoFixCustom.setOnClickListener {
            val ctx = context
            handleLaunch(GameFixDatabase.LaunchMode.AUTO_FIX_WITH_CUSTOM) {
                GameFixDatabase.selectedLaunchMode = GameFixDatabase.LaunchMode.AUTO_FIX_WITH_CUSTOM
                GameFixDatabase.applyFixWithCustomOverrides(currentGame)
                if (ctx != null) {
                    val msg = ctx.getString(R.string.game_fix_toast_custom_saved)
                    Toast.makeText(ctx, msg, Toast.LENGTH_SHORT).show()
                }
            }
        }

        binding.cardLaunchCustom.setOnClickListener {
            val ctx = context
            handleLaunch(GameFixDatabase.LaunchMode.CUSTOM) {
                GameFixDatabase.selectedLaunchMode = GameFixDatabase.LaunchMode.CUSTOM
                GameFixDatabase.prepareCustomLaunch(currentGame)
                if (ctx != null) {
                    val msg = ctx.getString(R.string.game_fix_toast_launch_custom)
                    Toast.makeText(ctx, msg, Toast.LENGTH_SHORT).show()
                }
            }
        }

        binding.cardLaunchGlobal.setOnClickListener {
            val ctx = context
            handleLaunch(GameFixDatabase.LaunchMode.GLOBAL) {
                GameFixDatabase.selectedLaunchMode = GameFixDatabase.LaunchMode.GLOBAL
                GameFixDatabase.prepareGlobalLaunch(currentGame)
                if (ctx != null) {
                    val msg = ctx.getString(R.string.game_fix_toast_launch_global)
                    Toast.makeText(ctx, msg, Toast.LENGTH_SHORT).show()
                }
            }
        }

        val cancelAction = {
            val currentG = game
            val ctx = context
            if (currentG != null && ctx != null && binding.checkboxDontAskAgain.isChecked) {
                GameFixDatabase.setDontAskAgain(ctx, currentG, true)
            }
            GameFixDatabase.selectedLaunchMode = GameFixDatabase.LaunchMode.CANCEL
            val cb = onLaunchCallback
            dismissAllowingStateLoss()
            cb?.invoke(GameFixDatabase.LaunchMode.CANCEL)
        }

        binding.buttonCancel.setOnClickListener { cancelAction() }
        binding.btnCancelLaunch.setOnClickListener { cancelAction() }
    }

    private fun handleLaunch(mode: GameFixDatabase.LaunchMode, action: () -> Unit) {
        val currentGame = game
        val ctx = context
        if (currentGame != null && ctx != null && binding.checkboxDontAskAgain.isChecked) {
            GameFixDatabase.setDontAskAgain(ctx, currentGame, true)
            val prefMode = when (mode) {
                GameFixDatabase.LaunchMode.GLOBAL -> "standard"
                GameFixDatabase.LaunchMode.CUSTOM -> "custom"
                else -> "autofix"
            }
            GameFixDatabase.setPreferredLaunchMode(ctx, currentGame, prefMode)
        }
        try {
            action()
        } catch (_: Exception) {}
        val cb = onLaunchCallback
        dismissAllowingStateLoss()
        cb?.invoke(mode)
    }

    override fun onStart() {
        super.onStart()
        dialog?.setOnKeyListener { _, keyCode, event ->
            if (keyCode == KeyEvent.KEYCODE_ESCAPE && event.action == KeyEvent.ACTION_UP) {
                dismissAllowingStateLoss()
                true
            } else {
                false
            }
        }
        dialog?.window?.let { window ->
            window.setLayout(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
            )
            window.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
            WindowCompat.setDecorFitsSystemWindows(window, false)
            ThemeHelper.applySystemBarsTheme(window, requireContext())
        }
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        dialog?.window?.let { window ->
            window.setLayout(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
            )
            window.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
            WindowCompat.setDecorFitsSystemWindows(window, false)
            ThemeHelper.applySystemBarsTheme(window, requireContext())
        }
    }

    override fun onCancel(dialog: DialogInterface) {
        super.onCancel(dialog)
        val currentG = game
        val ctx = context
        if (currentG != null && ctx != null && _binding != null && binding.checkboxDontAskAgain.isChecked) {
            GameFixDatabase.setDontAskAgain(ctx, currentG, true)
        }
        GameFixDatabase.selectedLaunchMode = GameFixDatabase.LaunchMode.CANCEL
        onLaunchCallback?.invoke(GameFixDatabase.LaunchMode.CANCEL)
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}

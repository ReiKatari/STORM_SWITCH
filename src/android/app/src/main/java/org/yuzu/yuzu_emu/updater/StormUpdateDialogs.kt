// SPDX-FileCopyrightText: Copyright 2026 STORM SWITCH Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.updater

import android.animation.ValueAnimator
import android.app.Activity
import android.app.Dialog
import android.content.Context
import android.content.res.ColorStateList
import android.graphics.Color
import android.graphics.Typeface
import android.graphics.drawable.ColorDrawable
import android.graphics.drawable.GradientDrawable
import android.text.SpannableStringBuilder
import android.text.Spanned
import android.text.style.ForegroundColorSpan
import android.text.style.StyleSpan
import android.text.style.TypefaceSpan
import android.util.TypedValue
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.view.Window
import android.view.animation.DecelerateInterpolator
import android.widget.FrameLayout
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.ProgressBar
import android.widget.ScrollView
import android.widget.TextView
import androidx.annotation.AttrRes
import androidx.annotation.ColorInt
import androidx.annotation.DrawableRes
import androidx.appcompat.app.AppCompatDelegate
import com.google.android.material.button.MaterialButton
import org.yuzu.yuzu_emu.R
import java.util.Locale
import kotlin.math.min
import kotlin.math.roundToInt

data class StormUpdateDisplayInfo(
    val currentVersion: String,
    val newVersion: String,
    val releaseNotes: String,
    val sourceMirror: String,
    val assetSizeBytes: Long = 0L,
    val publishedDateFormatted: String = "",
    val releasePageUrl: String = ""
)

class StormUpdateProgressController(
    val dialog: Dialog,
    private val updateUi: (percent: Int, downloadedBytes: Long, totalBytes: Long, speedBytesPerSec: Double, statusText: String?) -> Unit
) {
    fun update(
        percent: Int,
        downloadedBytes: Long,
        totalBytes: Long,
        speedBytesPerSec: Double,
        statusText: String? = null
    ) {
        updateUi(percent, downloadedBytes, totalBytes, speedBytesPerSec, statusText)
    }

    fun dismissSafe() {
        runCatching {
            if (dialog.isShowing) {
                dialog.dismiss()
            }
        }
    }
}

object StormUpdateDialogs {

    private data class Palette(
        @ColorInt val cardBg: Int,
        @ColorInt val surfaceSecondary: Int,
        @ColorInt val notesBg: Int,
        @ColorInt val border: Int,
        @ColorInt val accent: Int,
        @ColorInt val onAccent: Int,
        @ColorInt val textPrimary: Int,
        @ColorInt val textSecondary: Int,
        @ColorInt val btnBg: Int,
        @ColorInt val btnBorder: Int,
        @ColorInt val green: Int,
        @ColorInt val red: Int
    )

    private sealed class ReleaseNoteBlock {
        data class Heading(val text: String, val level: Int) : ReleaseNoteBlock()
        data class Bullet(val text: String) : ReleaseNoteBlock()
        data class Paragraph(val text: String) : ReleaseNoteBlock()
        data class TableRow(val cells: List<String>) : ReleaseNoteBlock()
        object Divider : ReleaseNoteBlock()
    }

    private fun resolveAttrColor(context: Context, @AttrRes attr: Int, @ColorInt fallback: Int): Int {
        val tv = TypedValue()
        return if (context.theme.resolveAttribute(attr, tv, true)) {
            if (tv.resourceId != 0) context.getColor(tv.resourceId) else tv.data
        } else {
            fallback
        }
    }

    private fun withAlpha(@ColorInt color: Int, alphaFraction: Float): Int {
        val a = (alphaFraction.coerceIn(0f, 1f) * 255f).roundToInt()
        return Color.argb(a, Color.red(color), Color.green(color), Color.blue(color))
    }

    private fun isBrightColor(@ColorInt color: Int): Boolean {
        val lum = (0.299 * Color.red(color) + 0.587 * Color.green(color) + 0.114 * Color.blue(color)) / 255.0
        return lum > 0.62
    }

    private fun resolvePalette(context: Context): Palette {
        val surface = resolveAttrColor(context, com.google.android.material.R.attr.colorSurface, 0xFF0E1420.toInt())
        val surfaceVar = resolveAttrColor(context, com.google.android.material.R.attr.colorSurfaceVariant, 0xFF172033.toInt())
        val accent = resolveAttrColor(context, com.google.android.material.R.attr.colorPrimary, 0xFF22D3EE.toInt())
        val textPri = resolveAttrColor(context, com.google.android.material.R.attr.colorOnSurface, 0xFFF8FAFC.toInt())
        val textSec = resolveAttrColor(context, com.google.android.material.R.attr.colorOnSurfaceVariant, 0xFF94A3B8.toInt())
        val outline = resolveAttrColor(context, com.google.android.material.R.attr.colorOutline, withAlpha(textSec, 0.35f))

        val isLight = isBrightColor(surface)
        val notesBg = if (isLight) {
            resolveAttrColor(context, android.R.attr.colorBackground, 0xFFF1F5F9.toInt())
        } else {
            Color.rgb(
                (Color.red(surface) * 0.78f).roundToInt(),
                (Color.green(surface) * 0.78f).roundToInt(),
                (Color.blue(surface) * 0.82f).roundToInt()
            )
        }

        return Palette(
            cardBg = surface,
            surfaceSecondary = surfaceVar,
            notesBg = notesBg,
            border = if (isLight) withAlpha(outline, 0.55f) else withAlpha(accent, 0.32f),
            accent = accent,
            onAccent = if (isBrightColor(accent)) 0xFF0B1120.toInt() else Color.WHITE,
            textPrimary = textPri,
            textSecondary = textSec,
            btnBg = surfaceVar,
            btnBorder = withAlpha(textSec, 0.28f),
            green = 0xFF10B981.toInt(),
            red = 0xFFEF4444.toInt()
        )
    }

    private fun dp(context: Context, value: Float): Int {
        return (value * context.resources.displayMetrics.density).roundToInt()
    }

    private fun sanitizeDashes(input: String): String {
        return input
            .replace('\u2014', '-')
            .replace('\u2013', '-')
            .replace('\u2015', '-')
            .replace('\u2012', '-')
    }

    fun cleanVersionName(raw: String): String {
        var clean = raw.trim()
        while (clean.startsWith("v", ignoreCase = true) || clean.startsWith(" ")) {
            clean = clean.drop(1)
        }
        return clean.trim()
    }

    fun formatBytes(context: Context, bytes: Long): String {
        if (bytes <= 0L) return ""
        val mb = bytes.toDouble() / (1024.0 * 1024.0)
        return if (mb >= 0.1) {
            context.getString(R.string.storm_update_size_mb_fmt, mb)
        } else {
            val kb = maxOf(1L, bytes / 1024L)
            context.getString(R.string.storm_update_size_kb_fmt, kb)
        }
    }

    private fun makeRoundedBg(
        @ColorInt fill: Int,
        cornerPx: Float,
        @ColorInt stroke: Int = Color.TRANSPARENT,
        strokeWidthPx: Int = 0
    ): GradientDrawable {
        return GradientDrawable().apply {
            shape = GradientDrawable.RECTANGLE
            cornerRadius = cornerPx
            setColor(fill)
            if (strokeWidthPx > 0 && stroke != Color.TRANSPARENT) {
                setStroke(strokeWidthPx, stroke)
            }
        }
    }

    private fun createCardShell(
        activity: Activity,
        palette: Palette,
        cancelable: Boolean
    ): Pair<Dialog, LinearLayout> {
        val dialog = Dialog(activity)
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE)
        dialog.setCancelable(cancelable)
        dialog.setCanceledOnTouchOutside(cancelable)

        val rootCard = LinearLayout(activity).apply {
            orientation = LinearLayout.VERTICAL
            background = makeRoundedBg(
                fill = palette.cardBg,
                cornerPx = dp(activity, 20f).toFloat(),
                stroke = palette.border,
                strokeWidthPx = maxOf(1, dp(activity, 1.2f))
            )
            val padH = dp(activity, 18f)
            val padV = dp(activity, 16f)
            setPadding(padH, padV, padH, padV)
        }

        val wrapper = FrameLayout(activity).apply {
            addView(
                rootCard,
                FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT
                )
            )
        }

        dialog.setContentView(wrapper)
        dialog.window?.apply {
            setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
            val dm = activity.resources.displayMetrics
            val maxW = min((dm.widthPixels * 0.92f).roundToInt(), dp(activity, 480f))
            setLayout(maxW, ViewGroup.LayoutParams.WRAP_CONTENT)
        }

        return dialog to rootCard
    }

    private fun buildHeader(
        context: Context,
        palette: Palette,
        title: String,
        @ColorInt badgeColor: Int,
        @DrawableRes iconRes: Int,
        showClose: Boolean,
        onClose: () -> Unit
    ): LinearLayout {
        val header = LinearLayout(context).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            )
        }

        val badgeSize = dp(context, 40f)
        val badge = FrameLayout(context).apply {
            layoutParams = LinearLayout.LayoutParams(badgeSize, badgeSize)
            background = makeRoundedBg(
                fill = withAlpha(badgeColor, 0.14f),
                cornerPx = dp(context, 12f).toFloat(),
                stroke = withAlpha(badgeColor, 0.45f),
                strokeWidthPx = maxOf(1, dp(context, 1f))
            )
        }
        val icon = ImageView(context).apply {
            setImageResource(iconRes)
            setColorFilter(badgeColor)
            val iconSize = dp(context, 22f)
            layoutParams = FrameLayout.LayoutParams(iconSize, iconSize, Gravity.CENTER)
        }
        badge.addView(icon)
        header.addView(badge)

        val titleCol = LinearLayout(context).apply {
            orientation = LinearLayout.VERTICAL
            layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f).apply {
                marginStart = dp(context, 12f)
                marginEnd = dp(context, 8f)
            }
        }
        val titleTv = TextView(context).apply {
            text = sanitizeDashes(title)
            setTextColor(palette.textPrimary)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 16.5f)
            setTypeface(null, Typeface.BOLD)
            maxLines = 2
        }
        val subTv = TextView(context).apply {
            text = "STORM SWITCH"
            setTextColor(palette.textSecondary)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 11f)
            typeface = Typeface.MONOSPACE
        }
        titleCol.addView(titleTv)
        titleCol.addView(subTv)
        header.addView(titleCol)

        if (showClose) {
            val closeSize = dp(context, 32f)
            val closeBtn = TextView(context).apply {
                text = "X"
                gravity = Gravity.CENTER
                setTextColor(palette.textSecondary)
                setTextSize(TypedValue.COMPLEX_UNIT_SP, 13f)
                setTypeface(Typeface.DEFAULT_BOLD)
                layoutParams = LinearLayout.LayoutParams(closeSize, closeSize)
                background = makeRoundedBg(
                    fill = palette.surfaceSecondary,
                    cornerPx = dp(context, 9f).toFloat(),
                    stroke = palette.btnBorder,
                    strokeWidthPx = maxOf(1, dp(context, 1f))
                )
                setOnClickListener { onClose() }
            }
            header.addView(closeBtn)
        }

        return header
    }

    private fun buildDivider(context: Context, palette: Palette, topDp: Float = 12f, bottomDp: Float = 12f): View {
        return View(context).apply {
            setBackgroundColor(palette.btnBorder)
            layoutParams = LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, maxOf(1, dp(context, 1f))).apply {
                topMargin = dp(context, topDp)
                bottomMargin = dp(context, bottomDp)
            }
        }
    }

    private fun buildStyledButton(
        context: Context,
        palette: Palette,
        label: String,
        primary: Boolean,
        onClick: () -> Unit
    ): MaterialButton {
        return MaterialButton(context).apply {
            text = sanitizeDashes(label)
            isAllCaps = false
            letterSpacing = 0f
            insetTop = 0
            insetBottom = 0
            cornerRadius = dp(context, if (primary) 12f else 11f)
            minHeight = dp(context, if (primary) 44f else 38f)
            minimumHeight = minHeight
            setTextSize(TypedValue.COMPLEX_UNIT_SP, if (primary) 13.5f else 12.5f)
            setTypeface(null, if (primary) Typeface.BOLD else Typeface.NORMAL)

            if (primary) {
                backgroundTintList = ColorStateList.valueOf(palette.accent)
                setTextColor(palette.onAccent)
                strokeWidth = 0
                elevation = 0f
            } else {
                backgroundTintList = ColorStateList.valueOf(palette.btnBg)
                setTextColor(palette.textPrimary)
                strokeColor = ColorStateList.valueOf(palette.btnBorder)
                strokeWidth = maxOf(1, dp(context, 1f))
                elevation = 0f
            }

            setOnClickListener { onClick() }
        }
    }

    private fun normalizeReleaseLocaleCode(raw: String): String {
        val clean = raw.trim().lowercase(Locale.ROOT).replace('_', '-')
        return when {
            clean == "in" || clean.startsWith("in-") -> "id" + clean.removePrefix("in")
            clean == "iw" || clean.startsWith("iw-") -> "he" + clean.removePrefix("iw")
            else -> clean
        }
    }

    private fun resolveActiveLocale(context: Context): Locale {
        val appLocale = runCatching { AppCompatDelegate.getApplicationLocales().get(0) }.getOrNull()
        if (appLocale != null && appLocale.language.isNotBlank()) {
            return appLocale
        }
        val cfgLocale = runCatching { context.resources.configuration.locales.get(0) }.getOrNull()
        if (cfgLocale != null && cfgLocale.language.isNotBlank()) {
            return cfgLocale
        }
        return Locale.getDefault()
    }

    private fun matchReleaseLanguageHeading(rawHeading: String): String? {
        val trimmed = rawHeading.trim()
            .removePrefix("###").removePrefix("##").removePrefix("#")
            .trim()
            .trim('[', ']', '(', ')')
            .lowercase(Locale.ROOT)
            .replace('_', '-')
        return when {
            trimmed == "ru" || trimmed.contains("русский") || trimmed.contains("russian") -> "ru"
            trimmed == "en" || trimmed.contains("english") || trimmed.contains("английск") -> "en"
            trimmed == "de" || trimmed.contains("deutsch") || trimmed.contains("german") -> "de"
            trimmed == "es" || trimmed.contains("español") || trimmed.contains("espanol") || trimmed.contains("spanish") -> "es"
            trimmed == "fr" || trimmed.contains("français") || trimmed.contains("francais") || trimmed.contains("french") -> "fr"
            trimmed == "id" || trimmed.contains("bahasa indonesia") || trimmed.contains("indonesian") -> "id"
            trimmed == "it" || trimmed.contains("italiano") || trimmed.contains("italian") -> "it"
            trimmed == "ja" || trimmed.contains("日本語") || trimmed.contains("japanese") -> "ja"
            trimmed == "ko" || trimmed.contains("한국어") || trimmed.contains("korean") -> "ko"
            trimmed == "hi" || trimmed.contains("हिन्दी") || trimmed.contains("hindi") -> "hi"
            trimmed == "pt-br" || trimmed == "pt" || trimmed.contains("português") || trimmed.contains("portugues") || trimmed.contains("portuguese") ->
                if (trimmed.contains("br")) "pt-br" else "pt"
            trimmed == "zh-hans" || trimmed == "zh-cn" || trimmed == "zh" || trimmed.contains("简体中文") || trimmed.contains("中文") || trimmed.contains("chinese") ->
                if (trimmed.contains("hans") || trimmed.contains("cn") || trimmed.contains("简体")) "zh-hans" else "zh"
            else -> null
        }
    }

    internal fun selectLocalizedReleaseNotes(raw: String, locale: Locale = Locale.getDefault()): String {
        val normalized = sanitizeDashes(raw).replace("\r\n", "\n").replace("\r", "\n")
        val lines = normalized.lines()
        val commentTagRegex = Regex("""<!--\s*lang\s*:\s*([a-zA-Z_-]+)\s*-->""", RegexOption.IGNORE_CASE)
        val commentEndRegex = Regex("""<!--\s*/lang\s*-->""", RegexOption.IGNORE_CASE)
        val htmlCommentLineRegex = Regex("""^\s*<!--.*-->\s*$""")

        val markerSections = linkedMapOf<String, MutableList<String>>()
        var activeMarker: String? = null
        for (line in lines) {
            val startMatch = commentTagRegex.find(line)
            if (startMatch != null) {
                activeMarker = normalizeReleaseLocaleCode(startMatch.groupValues[1])
                markerSections.getOrPut(activeMarker) { mutableListOf() }
                val stripped = line.replace(commentTagRegex, "").replace(commentEndRegex, "").trimEnd()
                if (stripped.isNotBlank()) markerSections[activeMarker]?.add(stripped)
                if (commentEndRegex.containsMatchIn(line)) activeMarker = null
                continue
            }
            if (commentEndRegex.containsMatchIn(line)) {
                activeMarker = null
                continue
            }
            val current = activeMarker
            if (current != null && !htmlCommentLineRegex.matches(line)) {
                markerSections[current]?.add(line)
            }
        }

        val headingSections = linkedMapOf<String, MutableList<String>>()
        if (markerSections.size < 2) {
            var activeHeadingLang: String? = null
            for (line in lines) {
                val trimmed = line.trim()
                if (trimmed.startsWith("## ") || trimmed.startsWith("### ")) {
                    val detected = matchReleaseLanguageHeading(trimmed)
                    if (detected != null) {
                        activeHeadingLang = detected
                        headingSections.getOrPut(detected) { mutableListOf() }
                        continue
                    }
                }
                val current = activeHeadingLang
                if (current != null && !htmlCommentLineRegex.matches(line)) {
                    headingSections[current]?.add(line)
                }
            }
        }

        val chosenMap = when {
            markerSections.size >= 2 -> markerSections
            headingSections.size >= 2 -> headingSections
            else -> null
        } ?: return lines.filterNot { htmlCommentLineRegex.matches(it) }.joinToString("\n").trim()

        val lang = normalizeReleaseLocaleCode(locale.language)
        val country = locale.country.lowercase(Locale.ROOT)
        val script = locale.script.lowercase(Locale.ROOT)
        val candidates = buildList {
            if (lang.isNotEmpty() && script.isNotEmpty()) add("$lang-$script")
            if (lang.isNotEmpty() && country.isNotEmpty()) add("$lang-$country")
            if (lang == "zh") add("zh-hans")
            if (lang.isNotEmpty()) add(lang)
            add("en")
            add("ru")
        }

        val picked = candidates.firstNotNullOfOrNull { key ->
            chosenMap[key]?.takeIf { list -> list.any { it.isNotBlank() } }
        } ?: chosenMap.values.firstOrNull { list -> list.any { it.isNotBlank() } }

        return picked
            ?.filterNot { htmlCommentLineRegex.matches(it) }
            ?.joinToString("\n")
            ?.trim()
            ?: normalized.trim()
    }

    private fun parseReleaseNotes(raw: String): List<ReleaseNoteBlock> {
        if (raw.isBlank()) return emptyList()
        val normalized = sanitizeDashes(raw)
            .replace("\r\n", "\n")
            .replace("\r", "\n")
            .replace(Regex("<br\\s*/?>", RegexOption.IGNORE_CASE), "\n")
            .replace(Regex("</?(details|summary|b|i|strong|em|p)>", RegexOption.IGNORE_CASE), "")

        val blocks = mutableListOf<ReleaseNoteBlock>()
        for (rawLine in normalized.lines()) {
            var line = rawLine.trim()
            if (line.isEmpty()) continue
            if (line.startsWith("<!--") && line.endsWith("-->")) continue
            if (line.startsWith(">")) {
                line = line.removePrefix(">").trim()
                if (line.isEmpty()) continue
            }
            if (line == "---" || line == "***" || line == "___") {
                blocks.add(ReleaseNoteBlock.Divider)
                continue
            }
            if (line.startsWith("#")) {
                val level = line.takeWhile { it == '#' }.length
                val content = line.drop(level).trim()
                if (content.isNotEmpty()) {
                    blocks.add(ReleaseNoteBlock.Heading(content, level))
                }
                continue
            }
            if (line.startsWith("- ") || line.startsWith("* ")) {
                val content = line.drop(2).trim()
                if (content.isNotEmpty()) {
                    blocks.add(ReleaseNoteBlock.Bullet(content))
                }
                continue
            }
            if (line.startsWith("|") && line.endsWith("|")) {
                val cells = line.trim('|').split('|').map { it.trim() }
                val isAlignRow = cells.isNotEmpty() && cells.all { cell ->
                    cell.isNotEmpty() && cell.all { ch -> ch == '-' || ch == ':' || ch == ' ' }
                }
                if (!isAlignRow && cells.any { it.isNotEmpty() }) {
                    blocks.add(ReleaseNoteBlock.TableRow(cells.filter { it.isNotEmpty() }))
                }
                continue
            }
            blocks.add(ReleaseNoteBlock.Paragraph(line))
        }
        return blocks
    }

    private fun renderInlineMarkdown(
        text: String,
        @ColorInt boldColor: Int,
        @ColorInt codeColor: Int
    ): SpannableStringBuilder {
        val sb = SpannableStringBuilder()
        var i = 0
        while (i < text.length) {
            if (text.startsWith("**", i)) {
                val end = text.indexOf("**", i + 2)
                if (end > i + 2) {
                    val startIdx = sb.length
                    sb.append(text.substring(i + 2, end))
                    val endIdx = sb.length
                    sb.setSpan(StyleSpan(Typeface.BOLD), startIdx, endIdx, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE)
                    sb.setSpan(ForegroundColorSpan(boldColor), startIdx, endIdx, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE)
                    i = end + 2
                    continue
                }
                sb.append("**")
                i += 2
                continue
            }
            if (text[i] == '`') {
                val end = text.indexOf('`', i + 1)
                if (end > i + 1) {
                    val startIdx = sb.length
                    sb.append(text.substring(i + 1, end))
                    val endIdx = sb.length
                    sb.setSpan(TypefaceSpan("monospace"), startIdx, endIdx, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE)
                    sb.setSpan(StyleSpan(Typeface.BOLD), startIdx, endIdx, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE)
                    sb.setSpan(ForegroundColorSpan(codeColor), startIdx, endIdx, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE)
                    i = end + 1
                    continue
                }
                sb.append('`')
                i += 1
                continue
            }

            val nextBold = text.indexOf("**", i)
            val nextCode = text.indexOf('`', i)
            var next = text.length
            if (nextBold in (i + 1) until next) next = nextBold
            if (nextCode in (i + 1) until next) next = nextCode
            if (nextBold == i || nextCode == i) next = i + 1

            sb.append(text.substring(i, next))
            i = next
        }
        return sb
    }

    private fun populateReleaseNotesContainer(
        context: Context,
        palette: Palette,
        host: LinearLayout,
        notesMarkdown: String
    ) {
        val localizedMarkdown = selectLocalizedReleaseNotes(notesMarkdown, resolveActiveLocale(context))
        val blocks = parseReleaseNotes(localizedMarkdown)
        if (blocks.isEmpty()) {
            host.addView(TextView(context).apply {
                text = context.getString(R.string.storm_update_no_notes)
                setTextColor(palette.textSecondary)
                setTextSize(TypedValue.COMPLEX_UNIT_SP, 12.5f)
            })
            return
        }

        blocks.forEachIndexed { idx, block ->
            val topMarginPx = if (idx == 0) 0 else dp(context, 6f)
            when (block) {
                is ReleaseNoteBlock.Heading -> {
                    val row = LinearLayout(context).apply {
                        orientation = LinearLayout.HORIZONTAL
                        gravity = Gravity.CENTER_VERTICAL
                        layoutParams = LinearLayout.LayoutParams(
                            ViewGroup.LayoutParams.MATCH_PARENT,
                            ViewGroup.LayoutParams.WRAP_CONTENT
                        ).apply {
                            topMargin = if (idx == 0) 0 else dp(context, 8f)
                        }
                    }
                    val bar = View(context).apply {
                        background = makeRoundedBg(palette.accent, dp(context, 2f).toFloat())
                        layoutParams = LinearLayout.LayoutParams(dp(context, 3f), dp(context, 14f))
                    }
                    val tv = TextView(context).apply {
                        text = renderInlineMarkdown(block.text, palette.textPrimary, palette.accent)
                        setTextColor(palette.textPrimary)
                        setTextSize(TypedValue.COMPLEX_UNIT_SP, if (block.level <= 2) 13.5f else 12.5f)
                        setTypeface(null, Typeface.BOLD)
                        layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f).apply {
                            marginStart = dp(context, 8f)
                        }
                    }
                    row.addView(bar)
                    row.addView(tv)
                    host.addView(row)
                }

                is ReleaseNoteBlock.Bullet -> {
                    val row = LinearLayout(context).apply {
                        orientation = LinearLayout.HORIZONTAL
                        gravity = Gravity.TOP
                        layoutParams = LinearLayout.LayoutParams(
                            ViewGroup.LayoutParams.MATCH_PARENT,
                            ViewGroup.LayoutParams.WRAP_CONTENT
                        ).apply {
                            topMargin = topMarginPx
                        }
                    }
                    val bullet = TextView(context).apply {
                        text = "-"
                        setTextColor(palette.accent)
                        setTextSize(TypedValue.COMPLEX_UNIT_SP, 12.5f)
                        typeface = Typeface.MONOSPACE
                        setTypeface(typeface, Typeface.BOLD)
                        layoutParams = LinearLayout.LayoutParams(dp(context, 14f), ViewGroup.LayoutParams.WRAP_CONTENT)
                    }
                    val tv = TextView(context).apply {
                        text = renderInlineMarkdown(block.text, palette.textPrimary, palette.accent)
                        setTextColor(palette.textSecondary)
                        setTextSize(TypedValue.COMPLEX_UNIT_SP, 12.5f)
                        setLineSpacing(dp(context, 2f).toFloat(), 1.0f)
                        layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f)
                    }
                    row.addView(bullet)
                    row.addView(tv)
                    host.addView(row)
                }

                is ReleaseNoteBlock.TableRow -> {
                    val pill = TextView(context).apply {
                        text = block.cells.joinToString(" | ")
                        setTextColor(palette.textSecondary)
                        setTextSize(TypedValue.COMPLEX_UNIT_SP, 11f)
                        typeface = Typeface.MONOSPACE
                        setPadding(dp(context, 8f), dp(context, 4f), dp(context, 8f), dp(context, 4f))
                        background = makeRoundedBg(withAlpha(palette.border, 0.25f), dp(context, 7f).toFloat())
                        layoutParams = LinearLayout.LayoutParams(
                            ViewGroup.LayoutParams.MATCH_PARENT,
                            ViewGroup.LayoutParams.WRAP_CONTENT
                        ).apply {
                            topMargin = topMarginPx
                        }
                    }
                    host.addView(pill)
                }

                is ReleaseNoteBlock.Divider -> {
                    val rule = View(context).apply {
                        setBackgroundColor(palette.btnBorder)
                        layoutParams = LinearLayout.LayoutParams(
                            ViewGroup.LayoutParams.MATCH_PARENT,
                            maxOf(1, dp(context, 1f))
                        ).apply {
                            topMargin = topMarginPx
                        }
                    }
                    host.addView(rule)
                }

                is ReleaseNoteBlock.Paragraph -> {
                    val tv = TextView(context).apply {
                        text = renderInlineMarkdown(block.text, palette.textPrimary, palette.accent)
                        setTextColor(palette.textSecondary)
                        setTextSize(TypedValue.COMPLEX_UNIT_SP, 12.5f)
                        setLineSpacing(dp(context, 2f).toFloat(), 1.0f)
                        layoutParams = LinearLayout.LayoutParams(
                            ViewGroup.LayoutParams.MATCH_PARENT,
                            ViewGroup.LayoutParams.WRAP_CONTENT
                        ).apply {
                            topMargin = topMarginPx
                        }
                    }
                    host.addView(tv)
                }
            }
        }
    }

    private fun buildMetaChip(context: Context, palette: Palette, label: String, highlighted: Boolean): TextView {
        return TextView(context).apply {
            text = sanitizeDashes(label)
            typeface = Typeface.MONOSPACE
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 11f)
            setTypeface(typeface, if (highlighted) Typeface.BOLD else Typeface.NORMAL)
            setTextColor(if (highlighted) palette.accent else palette.textSecondary)
            setPadding(dp(context, 9f), dp(context, 5f), dp(context, 9f), dp(context, 5f))
            background = makeRoundedBg(
                fill = if (highlighted) withAlpha(palette.accent, 0.14f) else palette.surfaceSecondary,
                cornerPx = dp(context, 9f).toFloat(),
                stroke = if (highlighted) withAlpha(palette.accent, 0.40f) else palette.btnBorder,
                strokeWidthPx = maxOf(1, dp(context, 1f))
            )
        }
    }

    fun showCheckingDialog(activity: Activity, onCancel: () -> Unit): Dialog {
        val palette = resolvePalette(activity)
        val (dialog, root) = createCardShell(activity, palette, cancelable = true)

        root.addView(
            buildHeader(
                context = activity,
                palette = palette,
                title = activity.getString(R.string.storm_update_checking_title),
                badgeColor = palette.accent,
                iconRes = R.drawable.ic_system_update_alt,
                showClose = false,
                onClose = {}
            )
        )
        root.addView(buildDivider(activity, palette))

        val body = LinearLayout(activity).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(dp(activity, 2f), dp(activity, 8f), dp(activity, 2f), dp(activity, 8f))
        }
        val spinner = ProgressBar(activity).apply {
            isIndeterminate = true
            indeterminateTintList = ColorStateList.valueOf(palette.accent)
            val sz = dp(activity, 26f)
            layoutParams = LinearLayout.LayoutParams(sz, sz)
        }
        val msg = TextView(activity).apply {
            text = activity.getString(R.string.storm_update_checking_desc)
            setTextColor(palette.textSecondary)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 13f)
            layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f).apply {
                marginStart = dp(activity, 14f)
            }
        }
        body.addView(spinner)
        body.addView(msg)
        root.addView(body)

        dialog.setOnCancelListener { onCancel() }
        dialog.show()
        return dialog
    }

    fun showUpdateAvailableDialog(
        activity: Activity,
        info: StormUpdateDisplayInfo,
        onUpdateNow: () -> Unit,
        onRemindLater: () -> Unit,
        onSkipVersion: () -> Unit,
        onOpenReleasePage: () -> Unit
    ): Dialog {
        val palette = resolvePalette(activity)
        val (dialog, root) = createCardShell(activity, palette, cancelable = true)

        root.addView(
            buildHeader(
                context = activity,
                palette = palette,
                title = activity.getString(R.string.storm_update_available_title),
                badgeColor = palette.accent,
                iconRes = R.drawable.ic_system_update_alt,
                showClose = true,
                onClose = {
                    dialog.dismiss()
                    onRemindLater()
                }
            )
        )
        root.addView(buildDivider(activity, palette))

        // Version transition card
        val verCard = LinearLayout(activity).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(dp(activity, 12f), dp(activity, 10f), dp(activity, 12f), dp(activity, 10f))
            background = makeRoundedBg(
                fill = palette.surfaceSecondary,
                cornerPx = dp(activity, 14f).toFloat(),
                stroke = palette.btnBorder,
                strokeWidthPx = maxOf(1, dp(activity, 1f))
            )
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            )
        }

        val curCol = LinearLayout(activity).apply {
            orientation = LinearLayout.VERTICAL
            layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f)
        }
        curCol.addView(TextView(activity).apply {
            text = activity.getString(R.string.storm_update_current_label)
            setTextColor(palette.textSecondary)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 11f)
        })
        curCol.addView(TextView(activity).apply {
            text = cleanVersionName(info.currentVersion).ifEmpty { "-" }
            setTextColor(palette.textPrimary)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 15f)
            typeface = Typeface.MONOSPACE
            setTypeface(typeface, Typeface.BOLD)
        })

        val arrowBadge = TextView(activity).apply {
            text = "->"
            gravity = Gravity.CENTER
            setTextColor(palette.accent)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 13f)
            typeface = Typeface.MONOSPACE
            setTypeface(typeface, Typeface.BOLD)
            background = makeRoundedBg(
                fill = withAlpha(palette.accent, 0.14f),
                cornerPx = dp(activity, 8f).toFloat()
            )
            val sz = dp(activity, 30f)
            layoutParams = LinearLayout.LayoutParams(sz, sz).apply {
                marginStart = dp(activity, 8f)
                marginEnd = dp(activity, 8f)
            }
        }

        val newCol = LinearLayout(activity).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(activity, 10f), dp(activity, 6f), dp(activity, 10f), dp(activity, 6f))
            background = makeRoundedBg(
                fill = withAlpha(palette.accent, 0.14f),
                cornerPx = dp(activity, 10f).toFloat(),
                stroke = withAlpha(palette.accent, 0.45f),
                strokeWidthPx = maxOf(1, dp(activity, 1f))
            )
            layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f)
        }
        newCol.addView(TextView(activity).apply {
            text = activity.getString(R.string.storm_update_new_label)
            setTextColor(palette.accent)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 11f)
        })
        newCol.addView(TextView(activity).apply {
            text = cleanVersionName(info.newVersion).ifEmpty { "-" }
            setTextColor(palette.accent)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 15f)
            typeface = Typeface.MONOSPACE
            setTypeface(typeface, Typeface.BOLD)
        })

        verCard.addView(curCol)
        verCard.addView(arrowBadge)
        verCard.addView(newCol)
        root.addView(verCard)

        // Metadata chips row
        val chipsRow = LinearLayout(activity).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply {
                topMargin = dp(activity, 10f)
            }
        }
        val mirrorText = info.sourceMirror.ifBlank { "GitHub" }
        chipsRow.addView(buildMetaChip(activity, palette, mirrorText, highlighted = true))

        val sizeLabel = formatBytes(activity, info.assetSizeBytes)
        if (sizeLabel.isNotEmpty()) {
            chipsRow.addView(buildMetaChip(activity, palette, sizeLabel, highlighted = false).apply {
                layoutParams = LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT
                ).apply { marginStart = dp(activity, 6f) }
            })
        }
        if (info.publishedDateFormatted.isNotBlank()) {
            chipsRow.addView(buildMetaChip(activity, palette, info.publishedDateFormatted, highlighted = false).apply {
                layoutParams = LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT
                ).apply { marginStart = dp(activity, 6f) }
            })
        }
        root.addView(chipsRow)

        // Notes section header
        root.addView(TextView(activity).apply {
            text = activity.getString(R.string.storm_update_whats_new_hdr)
            typeface = Typeface.MONOSPACE
            setTypeface(typeface, Typeface.BOLD)
            setTextColor(palette.textSecondary)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 11f)
            letterSpacing = 0.06f
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply {
                topMargin = dp(activity, 12f)
                bottomMargin = dp(activity, 6f)
            }
        })

        // Scrollable release notes card
        val dm = activity.resources.displayMetrics
        val maxNotesH = min((dm.heightPixels * 0.34f).roundToInt(), dp(activity, 240f))
        val notesFrame = FrameLayout(activity).apply {
            minimumHeight = dp(activity, 110f)
            background = makeRoundedBg(
                fill = palette.notesBg,
                cornerPx = dp(activity, 14f).toFloat(),
                stroke = palette.btnBorder,
                strokeWidthPx = maxOf(1, dp(activity, 1f))
            )
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                maxNotesH
            )
        }
        val notesScroll = ScrollView(activity).apply {
            isVerticalScrollBarEnabled = true
            layoutParams = FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
            )
        }
        val notesContent = LinearLayout(activity).apply {
            orientation = LinearLayout.VERTICAL
            val pad = dp(activity, 12f)
            setPadding(pad, pad, pad, pad)
        }
        populateReleaseNotesContainer(activity, palette, notesContent, info.releaseNotes)
        notesScroll.addView(notesContent)
        notesFrame.addView(notesScroll)
        root.addView(notesFrame)

        // Primary CTA button
        root.addView(
            buildStyledButton(
                context = activity,
                palette = palette,
                label = activity.getString(R.string.storm_update_now),
                primary = true
            ) {
                dialog.dismiss()
                onUpdateNow()
            }.apply {
                layoutParams = LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT
                ).apply { topMargin = dp(activity, 14f) }
            }
        )

        // Secondary action row
        val secRow = LinearLayout(activity).apply {
            orientation = LinearLayout.HORIZONTAL
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { topMargin = dp(activity, 8f) }
        }
        secRow.addView(
            buildStyledButton(
                context = activity,
                palette = palette,
                label = activity.getString(R.string.storm_update_remind_later),
                primary = false
            ) {
                dialog.dismiss()
                onRemindLater()
            }.apply {
                layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f).apply {
                    marginEnd = dp(activity, 4f)
                }
            }
        )
        secRow.addView(
            buildStyledButton(
                context = activity,
                palette = palette,
                label = activity.getString(R.string.storm_update_skip_version),
                primary = false
            ) {
                dialog.dismiss()
                onSkipVersion()
            }.apply {
                layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f).apply {
                    marginStart = dp(activity, 4f)
                }
            }
        )
        root.addView(secRow)

        // Footer link
        root.addView(TextView(activity).apply {
            text = activity.getString(R.string.storm_update_open_release_page)
            gravity = Gravity.CENTER
            setTextColor(palette.accent)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 12.5f)
            setTypeface(null, Typeface.BOLD)
            setPadding(dp(activity, 10f), dp(activity, 8f), dp(activity, 10f), dp(activity, 2f))
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT,
                0f
            ).apply {
                gravity = Gravity.CENTER_HORIZONTAL
                topMargin = dp(activity, 4f)
            }
            setOnClickListener {
                onOpenReleasePage()
            }
        })

        dialog.setOnCancelListener { onRemindLater() }
        dialog.show()
        return dialog
    }

    fun showProgressDialog(
        activity: Activity,
        versionName: String,
        onMoveToBackground: () -> Unit,
        onCancel: () -> Unit
    ): StormUpdateProgressController {
        val palette = resolvePalette(activity)
        val (dialog, root) = createCardShell(activity, palette, cancelable = false)

        root.addView(
            buildHeader(
                context = activity,
                palette = palette,
                title = activity.getString(R.string.downloading_update),
                badgeColor = palette.accent,
                iconRes = R.drawable.ic_system_update_alt,
                showClose = false,
                onClose = {}
            )
        )
        root.addView(buildDivider(activity, palette))

        val cleanVer = cleanVersionName(versionName)
        val statusTv = TextView(activity).apply {
            text = activity.getString(R.string.storm_update_downloading_ver_fmt, cleanVer)
            setTextColor(palette.textSecondary)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 13f)
        }
        root.addView(statusTv)

        // Rounded custom progress track + animated fill
        val trackHeight = dp(activity, 8f)
        val trackFrame = FrameLayout(activity).apply {
            background = makeRoundedBg(palette.surfaceSecondary, dp(activity, 4f).toFloat())
            layoutParams = LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, trackHeight).apply {
                topMargin = dp(activity, 12f)
            }
        }
        val fillView = View(activity).apply {
            background = makeRoundedBg(palette.accent, dp(activity, 4f).toFloat())
            layoutParams = FrameLayout.LayoutParams(0, trackHeight, Gravity.START or Gravity.CENTER_VERTICAL)
        }
        trackFrame.addView(fillView)
        root.addView(trackFrame)

        // Stats row: bytes on left, speed + percent on right
        val statsRow = LinearLayout(activity).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { topMargin = dp(activity, 8f) }
        }
        val bytesTv = TextView(activity).apply {
            text = activity.getString(R.string.storm_update_preparing)
            typeface = Typeface.MONOSPACE
            setTextColor(palette.textSecondary)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 11.5f)
            layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f)
        }
        val speedTv = TextView(activity).apply {
            text = ""
            typeface = Typeface.MONOSPACE
            setTextColor(palette.textSecondary)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 11.5f)
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { marginEnd = dp(activity, 8f) }
        }
        val percentTv = TextView(activity).apply {
            text = "0%"
            typeface = Typeface.MONOSPACE
            setTypeface(typeface, Typeface.BOLD)
            setTextColor(palette.accent)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 12f)
        }
        statsRow.addView(bytesTv)
        statsRow.addView(speedTv)
        statsRow.addView(percentTv)
        root.addView(statsRow)

        val btnRow = LinearLayout(activity).apply {
            orientation = LinearLayout.HORIZONTAL
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { topMargin = dp(activity, 14f) }
        }
        btnRow.addView(
            buildStyledButton(
                context = activity,
                palette = palette,
                label = activity.getString(R.string.storm_update_background),
                primary = false
            ) {
                dialog.dismiss()
                onMoveToBackground()
            }.apply {
                layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f).apply {
                    marginEnd = dp(activity, 4f)
                }
            }
        )
        btnRow.addView(
            buildStyledButton(
                context = activity,
                palette = palette,
                label = activity.getString(R.string.cancel),
                primary = false
            ) {
                dialog.dismiss()
                onCancel()
            }.apply {
                layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f).apply {
                    marginStart = dp(activity, 4f)
                }
            }
        )
        root.addView(btnRow)

        var lastFraction = 0f
        var progressAnim: ValueAnimator? = null

        dialog.show()

        return StormUpdateProgressController(dialog) { pct, curBytes, totBytes, speedBps, statusOverride ->
            if (!statusOverride.isNullOrBlank()) {
                statusTv.text = sanitizeDashes(statusOverride)
            }
            val clampedPct = pct.coerceIn(0, 100)
            percentTv.text = String.format(Locale.US, "%d%%", clampedPct)

            val curMb = curBytes.toDouble() / (1024.0 * 1024.0)
            bytesTv.text = when {
                totBytes > 0L -> {
                    val totMb = totBytes.toDouble() / (1024.0 * 1024.0)
                    activity.getString(R.string.storm_update_progress_mb_total_fmt, curMb, totMb)
                }
                curBytes > 0L -> activity.getString(R.string.storm_update_size_mb_fmt, curMb)
                else -> activity.getString(R.string.storm_update_preparing)
            }

            if (speedBps > 1024.0) {
                val mbPerSec = speedBps / (1024.0 * 1024.0)
                speedTv.text = activity.getString(R.string.storm_update_speed_mbs_fmt, mbPerSec)
            } else {
                speedTv.text = ""
            }

            val targetFraction = if (totBytes > 0L) {
                (curBytes.toFloat() / totBytes.toFloat()).coerceIn(0f, 1f)
            } else {
                (clampedPct / 100f).coerceIn(0f, 1f)
            }
            trackFrame.post {
                val w = trackFrame.width
                if (w > 0) {
                    progressAnim?.cancel()
                    val fromFrac = lastFraction
                    progressAnim = ValueAnimator.ofFloat(fromFrac, targetFraction).apply {
                        duration = 160L
                        interpolator = DecelerateInterpolator()
                        addUpdateListener { anim ->
                            val f = anim.animatedValue as Float
                            lastFraction = f
                            val lp = fillView.layoutParams
                            lp.width = (w * f).roundToInt()
                            fillView.layoutParams = lp
                        }
                        start()
                    }
                }
            }
        }
    }

    fun showInfoDialog(
        activity: Activity,
        isError: Boolean,
        title: String,
        message: String,
        versionBadge: String? = null,
        sourceMirror: String? = null,
        onRetry: (() -> Unit)? = null,
        onOpenReleasePage: (() -> Unit)? = null
    ): Dialog {
        val palette = resolvePalette(activity)
        val (dialog, root) = createCardShell(activity, palette, cancelable = true)

        val badgeColor = if (isError) palette.red else palette.green
        val iconRes = if (isError) android.R.drawable.ic_dialog_alert else R.drawable.ic_check

        root.addView(
            buildHeader(
                context = activity,
                palette = palette,
                title = title,
                badgeColor = badgeColor,
                iconRes = iconRes,
                showClose = true,
                onClose = { dialog.dismiss() }
            )
        )
        root.addView(buildDivider(activity, palette))

        if (isError) {
            val errCard = TextView(activity).apply {
                text = sanitizeDashes(message)
                setTextColor(palette.textPrimary)
                setTextSize(TypedValue.COMPLEX_UNIT_SP, 12.5f)
                setPadding(dp(activity, 12f), dp(activity, 10f), dp(activity, 12f), dp(activity, 10f))
                background = makeRoundedBg(
                    fill = withAlpha(palette.red, 0.12f),
                    cornerPx = dp(activity, 12f).toFloat(),
                    stroke = withAlpha(palette.red, 0.42f),
                    strokeWidthPx = maxOf(1, dp(activity, 1f))
                )
            }
            root.addView(errCard)
        } else {
            root.addView(TextView(activity).apply {
                text = sanitizeDashes(message)
                setTextColor(palette.textSecondary)
                setTextSize(TypedValue.COMPLEX_UNIT_SP, 13f)
            })
        }

        if (!versionBadge.isNullOrBlank()) {
            root.addView(buildMetaChip(activity, palette, cleanVersionName(versionBadge), highlighted = true).apply {
                layoutParams = LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT
                ).apply { topMargin = dp(activity, 10f) }
            })
        }

        if (!sourceMirror.isNullOrBlank()) {
            root.addView(TextView(activity).apply {
                text = activity.getString(R.string.storm_update_checked_via_fmt, sourceMirror)
                setTextColor(palette.textSecondary)
                setTextSize(TypedValue.COMPLEX_UNIT_SP, 12f)
                layoutParams = LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT
                ).apply { topMargin = dp(activity, 8f) }
            })
        }

        if (isError && onRetry != null) {
            val row = LinearLayout(activity).apply {
                orientation = LinearLayout.HORIZONTAL
                layoutParams = LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT
                ).apply { topMargin = dp(activity, 14f) }
            }
            row.addView(
                buildStyledButton(
                    context = activity,
                    palette = palette,
                    label = activity.getString(R.string.retry),
                    primary = true
                ) {
                    dialog.dismiss()
                    onRetry()
                }.apply {
                    layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f).apply {
                        marginEnd = dp(activity, 4f)
                    }
                }
            )
            row.addView(
                buildStyledButton(
                    context = activity,
                    palette = palette,
                    label = activity.getString(android.R.string.ok),
                    primary = false
                ) {
                    dialog.dismiss()
                }.apply {
                    layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f).apply {
                        marginStart = dp(activity, 4f)
                    }
                }
            )
            root.addView(row)
        } else {
            root.addView(
                buildStyledButton(
                    context = activity,
                    palette = palette,
                    label = activity.getString(android.R.string.ok),
                    primary = true
                ) {
                    dialog.dismiss()
                }.apply {
                    layoutParams = LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT
                    ).apply { topMargin = dp(activity, 14f) }
                }
            )
        }

        if (onOpenReleasePage != null) {
            root.addView(TextView(activity).apply {
                text = activity.getString(R.string.storm_update_open_release_page)
                gravity = Gravity.CENTER
                setTextColor(palette.accent)
                setTextSize(TypedValue.COMPLEX_UNIT_SP, 12.5f)
                setTypeface(null, Typeface.BOLD)
                setPadding(dp(activity, 10f), dp(activity, 8f), dp(activity, 10f), dp(activity, 2f))
                layoutParams = LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT
                ).apply {
                    gravity = Gravity.CENTER_HORIZONTAL
                    topMargin = dp(activity, 4f)
                }
                setOnClickListener { onOpenReleasePage() }
            })
        }

        dialog.show()
        return dialog
    }
}

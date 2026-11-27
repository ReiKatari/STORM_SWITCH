// SPDX-FileCopyrightText: 2026 STORM SWITCH Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.updater

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.widget.Toast
import androidx.preference.PreferenceManager
import okhttp3.OkHttpClient
import okhttp3.Request
import org.json.JSONObject
import org.yuzu.yuzu_emu.BuildConfig
import org.yuzu.yuzu_emu.NativeLibrary
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.features.settings.model.BooleanSetting
import org.yuzu.yuzu_emu.utils.Log
import org.yuzu.yuzu_emu.utils.NativeConfig
import org.yuzu.yuzu_emu.utils.SmartDns
import java.io.File
import java.text.SimpleDateFormat
import java.util.Locale
import java.util.TimeZone
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicBoolean

object StormUpdateManager {
    private const val PREFS_NAME = "storm_switch_updater"
    private const val PREF_SKIPPED_VERSION = "skipped_version"
    private const val PREF_HEALED_UPDATE_CHECK_VER = "storm_healed_update_check_ver"
    private const val AUTO_CHECK_COOLDOWN_MS = 120_000L
    private const val FALLBACK_RELEASE_URL = "https://github.com/ReiKatari/STORM_SWITCH/releases/latest"

    private data class ReleaseCandidate(
        val release: NativeLibrary.UpdateResult,
        val sourceMirror: String,
        val assetSizeBytes: Long,
        val publishedDateFormatted: String
    )

    private val isChecking = AtomicBoolean(false)

    @Volatile
    private var lastAutoCheckMs: Long = 0L

    @Volatile
    private var dismissedTagInSession: String? = null

    fun parseVersionParts(versionStr: String): List<Int> {
        val clean = StormUpdateDialogs.cleanVersionName(versionStr)
        val parts = Regex("\\d+").findAll(clean).map { it.value.toIntOrNull() ?: 0 }.toMutableList()
        while (parts.size < 3) {
            parts.add(0)
        }
        return parts.take(3)
    }

    fun compareVersions(v1: String, v2: String): Int {
        val p1 = parseVersionParts(v1)
        val p2 = parseVersionParts(v2)
        for (i in 0 until 3) {
            if (p1[i] != p2[i]) return p1[i].compareTo(p2[i])
        }
        return 0
    }

    fun isRemoteVersionNewer(remoteTag: String, currentVer: String): Boolean {
        val cleanRemote = StormUpdateDialogs.cleanVersionName(remoteTag)
        val cleanLocal = StormUpdateDialogs.cleanVersionName(currentVer)
        if (cleanRemote.isEmpty() || cleanRemote.equals(cleanLocal, ignoreCase = true)) {
            return false
        }
        return compareVersions(cleanRemote, cleanLocal) > 0
    }

    private fun getSkippedVersion(context: Context): String {
        return try {
            val prefs = context.applicationContext.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            StormUpdateDialogs.cleanVersionName(prefs.getString(PREF_SKIPPED_VERSION, "").orEmpty())
        } catch (_: Throwable) {
            ""
        }
    }

    private fun setSkippedVersion(context: Context, version: String) {
        try {
            val prefs = context.applicationContext.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            prefs.edit().putString(PREF_SKIPPED_VERSION, StormUpdateDialogs.cleanVersionName(version)).apply()
        } catch (_: Throwable) {}
    }

    private fun clearSkippedVersion(context: Context) {
        try {
            val prefs = context.applicationContext.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            prefs.edit().remove(PREF_SKIPPED_VERSION).apply()
        } catch (_: Throwable) {}
    }

    private fun formatIsoDateToRu(iso: String): String {
        val trimmed = iso.trim()
        if (trimmed.isEmpty()) return ""
        val patterns = listOf(
            "yyyy-MM-dd'T'HH:mm:ss'Z'",
            "yyyy-MM-dd'T'HH:mm:ss.SSS'Z'",
            "yyyy-MM-dd'T'HH:mm:ssXXX",
            "yyyy-MM-dd"
        )
        val outFmt = SimpleDateFormat("dd.MM.yyyy", Locale.US)
        for (pattern in patterns) {
            try {
                val parser = SimpleDateFormat(pattern, Locale.US).apply {
                    timeZone = TimeZone.getTimeZone("UTC")
                }
                val date = parser.parse(trimmed)
                if (date != null) {
                    return outFmt.format(date)
                }
            } catch (_: Throwable) {}
        }
        if (trimmed.length >= 10) {
            val datePart = trimmed.substring(0, 10)
            val m = Regex("^(\\d{4})-(\\d{2})-(\\d{2})$").matchEntire(datePart)
            if (m != null) {
                return "${m.groupValues[3]}.${m.groupValues[2]}.${m.groupValues[1]}"
            }
        }
        return ""
    }

    private fun expectedVariantSuffix(context: Context): String {
        val pkg = context.packageName.lowercase(Locale.US)
        return when {
            pkg.contains("legacy") -> "_LEGACY.apk"
            pkg.contains("sdk27") -> "_SDK27.apk"
            else -> ".apk"
        }
    }

    private fun isMatchingVariantNameOrUrl(context: Context, nameOrUrl: String): Boolean {
        val lower = nameOrUrl.lowercase(Locale.US)
        if (!lower.endsWith(".apk")) return false
        val pkg = context.packageName.lowercase(Locale.US)
        return when {
            pkg.contains("legacy") -> lower.endsWith("_legacy.apk")
            pkg.contains("sdk27") -> lower.endsWith("_sdk27.apk")
            else -> !lower.endsWith("_legacy.apk") && !lower.endsWith("_sdk27.apk")
        }
    }

    private fun ensureUpdateChecksDefaultEnabled(activity: Activity) {
        try {
            val prefs = PreferenceManager.getDefaultSharedPreferences(activity.applicationContext)
            val currentVer = StormUpdateDialogs.cleanVersionName(BuildConfig.VERSION_NAME)
            val healedVer = prefs.getString(PREF_HEALED_UPDATE_CHECK_VER, "")
            if (healedVer != currentVer) {
                prefs.edit().putString(PREF_HEALED_UPDATE_CHECK_VER, currentVer).apply()
                if (!BooleanSetting.ENABLE_UPDATE_CHECKS.getBoolean()) {
                    BooleanSetting.ENABLE_UPDATE_CHECKS.setBoolean(true)
                    NativeConfig.saveGlobalConfig()
                }
            }
        } catch (_: Throwable) {}
    }

    fun checkForUpdates(activity: Activity, manual: Boolean = false) {
        if (activity.isFinishing || activity.isDestroyed) return

        if (!manual) {
            ensureUpdateChecksDefaultEnabled(activity)
            try {
                if (!BooleanSetting.ENABLE_UPDATE_CHECKS.getBoolean()) {
                    return
                }
            } catch (_: Throwable) {}

            val now = System.currentTimeMillis()
            if (now - lastAutoCheckMs < AUTO_CHECK_COOLDOWN_MS) {
                return
            }
            lastAutoCheckMs = now
        }

        if (!isChecking.compareAndSet(false, true)) return

        val checkCancelled = AtomicBoolean(false)
        val checkingDialog = if (manual) {
            StormUpdateDialogs.showCheckingDialog(activity) {
                checkCancelled.set(true)
            }
        } else {
            null
        }

        val appContext = activity.applicationContext

        Thread {
            try {
                val candidates = fetchAllRemoteReleases(appContext)
                val currentVersion = StormUpdateDialogs.cleanVersionName(BuildConfig.VERSION_NAME)
                val bestCandidate = pickBestCandidate(candidates)

                activity.runOnUiThread {
                    runCatching { checkingDialog?.dismiss() }
                    if (activity.isFinishing || activity.isDestroyed || checkCancelled.get()) {
                        return@runOnUiThread
                    }

                    if (bestCandidate == null) {
                        if (manual) {
                            StormUpdateDialogs.showInfoDialog(
                                activity = activity,
                                isError = true,
                                title = activity.getString(R.string.storm_update_error_title),
                                message = activity.getString(R.string.storm_update_error_unreachable),
                                onRetry = { checkForUpdates(activity, manual = true) },
                                onOpenReleasePage = { openUrlSafe(activity, FALLBACK_RELEASE_URL) }
                            )
                        }
                        return@runOnUiThread
                    }

                    val bestRel = bestCandidate.release
                    val cleanRemote = StormUpdateDialogs.cleanVersionName(bestRel.tag)
                    if (!isRemoteVersionNewer(cleanRemote, currentVersion)) {
                        if (manual) {
                            StormUpdateDialogs.showInfoDialog(
                                activity = activity,
                                isError = false,
                                title = activity.getString(R.string.storm_update_uptodate_title),
                                message = activity.getString(R.string.storm_update_uptodate_desc),
                                versionBadge = "STORM SWITCH $currentVersion",
                                sourceMirror = bestCandidate.sourceMirror
                            )
                        }
                        return@runOnUiThread
                    }

                    if (!manual) {
                        val skipped = getSkippedVersion(appContext)
                        if (dismissedTagInSession == cleanRemote || (skipped.isNotEmpty() && skipped == cleanRemote)) {
                            return@runOnUiThread
                        }
                    }

                    val displayInfo = StormUpdateDisplayInfo(
                        currentVersion = currentVersion,
                        newVersion = cleanRemote,
                        releaseNotes = bestRel.body,
                        sourceMirror = bestCandidate.sourceMirror,
                        assetSizeBytes = bestCandidate.assetSizeBytes,
                        publishedDateFormatted = bestCandidate.publishedDateFormatted,
                        releasePageUrl = bestRel.url.ifBlank { FALLBACK_RELEASE_URL }
                    )

                    StormUpdateDialogs.showUpdateAvailableDialog(
                        activity = activity,
                        info = displayInfo,
                        onUpdateNow = {
                            clearSkippedVersion(appContext)
                            downloadAndInstallUpdate(activity, bestCandidate)
                        },
                        onRemindLater = {
                            dismissedTagInSession = cleanRemote
                        },
                        onSkipVersion = {
                            setSkippedVersion(appContext, cleanRemote)
                            dismissedTagInSession = cleanRemote
                        },
                        onOpenReleasePage = {
                            openUrlSafe(activity, displayInfo.releasePageUrl)
                        }
                    )
                }
            } catch (_: Throwable) {
                activity.runOnUiThread {
                    runCatching { checkingDialog?.dismiss() }
                    if (manual && !activity.isFinishing && !activity.isDestroyed && !checkCancelled.get()) {
                        StormUpdateDialogs.showInfoDialog(
                            activity = activity,
                            isError = true,
                            title = activity.getString(R.string.storm_update_error_title),
                            message = activity.getString(R.string.storm_update_error_unreachable),
                            onRetry = { checkForUpdates(activity, manual = true) },
                            onOpenReleasePage = { openUrlSafe(activity, FALLBACK_RELEASE_URL) }
                        )
                    }
                }
            } finally {
                isChecking.set(false)
            }
        }.start()
    }

    private fun pickBestCandidate(candidates: List<ReleaseCandidate>): ReleaseCandidate? {
        if (candidates.isEmpty()) return null
        val sorted = candidates.sortedWith { a, b ->
            val cmp = compareVersions(b.release.tag, a.release.tag)
            if (cmp != 0) {
                cmp
            } else {
                val aSgw = a.sourceMirror == "STORM GAMES WORLD"
                val bSgw = b.sourceMirror == "STORM GAMES WORLD"
                when {
                    aSgw && !bSgw -> -1
                    bSgw && !aSgw -> 1
                    else -> 0
                }
            }
        }
        val primary = sorted.first()
        var mergedBody = primary.release.body
        var mergedSize = primary.assetSizeBytes
        var mergedDate = primary.publishedDateFormatted
        val mergedAssets = primary.release.assets.toMutableList()
        var mergedUrl = primary.release.url

        for (other in sorted.drop(1)) {
            if (compareVersions(other.release.tag, primary.release.tag) == 0) {
                if (mergedBody.isBlank() && other.release.body.isNotBlank()) {
                    mergedBody = other.release.body
                }
                if (mergedSize <= 0L && other.assetSizeBytes > 0L) {
                    mergedSize = other.assetSizeBytes
                }
                if (mergedDate.isBlank() && other.publishedDateFormatted.isNotBlank()) {
                    mergedDate = other.publishedDateFormatted
                }
                if (mergedUrl.isBlank() && other.release.url.isNotBlank()) {
                    mergedUrl = other.release.url
                }
                for (u in other.release.assets) {
                    if (!mergedAssets.contains(u)) {
                        mergedAssets.add(u)
                    }
                }
            }
        }

        val finalRelease = primary.release.copy(
            body = mergedBody,
            url = mergedUrl,
            assets = mergedAssets
        )
        return primary.copy(
            release = finalRelease,
            assetSizeBytes = mergedSize,
            publishedDateFormatted = mergedDate
        )
    }

    private fun fetchAllRemoteReleases(context: Context): List<ReleaseCandidate> {
        val candidates = mutableListOf<ReleaseCandidate>()
        val client = OkHttpClient.Builder()
            .dns(SmartDns)
            .connectTimeout(8, TimeUnit.SECONDS)
            .readTimeout(10, TimeUnit.SECONDS)
            .followRedirects(true)
            .followSslRedirects(true)
            .build()

        // Source 1: Official STORM GAMES WORLD API
        try {
            val sgwReq = Request.Builder()
                .url("https://stormgamesworld.ru/api/programs?repo=STORM_SWITCH")
                .header("User-Agent", "STORM-SWITCH-Android/${BuildConfig.VERSION_NAME}")
                .header("Accept", "application/json")
                .build()
            client.newCall(sgwReq).execute().use { resp ->
                if (resp.isSuccessful) {
                    val bodyStr = resp.body?.string().orEmpty()
                    val root = JSONObject(bodyStr)
                    val rels = root.optJSONArray("releases")
                    if (rels != null && rels.length() > 0) {
                        for (idx in 0 until minOf(5, rels.length())) {
                            val rel = rels.getJSONObject(idx)
                            val ver = StormUpdateDialogs.cleanVersionName(rel.optString("version", ""))
                            if (ver.isNotEmpty()) {
                                val title = rel.optString("title", "STORM SWITCH $ver")
                                val notes = rel.optString("notes", rel.optString("body", ""))
                                val pubDate = formatIsoDateToRu(
                                    rel.optString("publishedAt", rel.optString("createdAt", ""))
                                )
                                var matchingSize = 0L
                                val assets = mutableListOf<String>()
                                val assetsArr = rel.optJSONArray("assets")
                                if (assetsArr != null) {
                                    for (j in 0 until assetsArr.length()) {
                                        val aObj = assetsArr.getJSONObject(j)
                                        val aName = aObj.optString("name", "")
                                        val dlUrl = aObj.optString("downloadUrl", "")
                                        val sz = aObj.optLong("size", 0L)
                                        if (dlUrl.isNotEmpty()) {
                                            assets.add(dlUrl)
                                            if (matchingSize == 0L && sz > 0L &&
                                                (isMatchingVariantNameOrUrl(context, aName) ||
                                                    isMatchingVariantNameOrUrl(context, dlUrl))
                                            ) {
                                                matchingSize = sz
                                            }
                                        }
                                    }
                                }
                                val ghBase = "https://github.com/ReiKatari/STORM_SWITCH/releases/download/$ver"
                                val sgwBase = "https://stormgamesworld.ru/uploads/programs/STORM_SWITCH"
                                assets.addAll(
                                    listOf(
                                        "$sgwBase/STORM_SWITCH_$ver.apk",
                                        "$sgwBase/STORM_SWITCH_${ver}_LEGACY.apk",
                                        "$sgwBase/STORM_SWITCH_${ver}_SDK27.apk",
                                        "$ghBase/STORM_SWITCH_$ver.apk",
                                        "$ghBase/STORM_SWITCH_${ver}_LEGACY.apk",
                                        "$ghBase/STORM_SWITCH_${ver}_SDK27.apk"
                                    )
                                )
                                candidates.add(
                                    ReleaseCandidate(
                                        release = NativeLibrary.UpdateResult(
                                            tag = ver,
                                            title = title,
                                            body = notes,
                                            url = "https://stormgamesworld.ru/programs?repo=STORM_SWITCH",
                                            assets = assets.distinct().toMutableList()
                                        ),
                                        sourceMirror = "STORM GAMES WORLD",
                                        assetSizeBytes = matchingSize,
                                        publishedDateFormatted = pubDate
                                    )
                                )
                                break
                            }
                        }
                    }
                }
            }
        } catch (_: Throwable) {}

        // Source 2: GitHub REST API (/releases/latest)
        try {
            val req = Request.Builder()
                .url("https://api.github.com/repos/ReiKatari/STORM_SWITCH/releases/latest")
                .header("User-Agent", "STORM-SWITCH-Updater/${BuildConfig.VERSION_NAME}")
                .header("Accept", "application/vnd.github.v3+json")
                .build()
            client.newCall(req).execute().use { resp ->
                if (resp.isSuccessful) {
                    val bodyStr = resp.body?.string().orEmpty()
                    val json = JSONObject(bodyStr)
                    val tag = StormUpdateDialogs.cleanVersionName(json.optString("tag_name", ""))
                    if (tag.isNotEmpty()) {
                        val title = json.optString("name", "STORM SWITCH $tag")
                        val htmlUrl = json.optString("html_url", "https://github.com/ReiKatari/STORM_SWITCH/releases/tag/$tag")
                        val pubDate = formatIsoDateToRu(json.optString("published_at", ""))
                        val assetsArr = json.optJSONArray("assets")
                        val assetList = mutableListOf<String>()
                        var matchingSize = 0L
                        if (assetsArr != null) {
                            for (i in 0 until assetsArr.length()) {
                                val aObj = assetsArr.getJSONObject(i)
                                val u = aObj.optString("browser_download_url", "")
                                val aName = aObj.optString("name", "")
                                val sz = aObj.optLong("size", 0L)
                                if (u.isNotEmpty()) {
                                    assetList.add(u)
                                    if (matchingSize == 0L && sz > 0L &&
                                        (isMatchingVariantNameOrUrl(context, aName) ||
                                            isMatchingVariantNameOrUrl(context, u))
                                    ) {
                                        matchingSize = sz
                                    }
                                }
                            }
                        }
                        val sgwBase = "https://stormgamesworld.ru/uploads/programs/STORM_SWITCH"
                        val ghBase = "https://github.com/ReiKatari/STORM_SWITCH/releases/download/$tag"
                        assetList.addAll(
                            listOf(
                                "$sgwBase/STORM_SWITCH_$tag.apk",
                                "$sgwBase/STORM_SWITCH_${tag}_LEGACY.apk",
                                "$sgwBase/STORM_SWITCH_${tag}_SDK27.apk",
                                "$ghBase/STORM_SWITCH_$tag.apk",
                                "$ghBase/STORM_SWITCH_${tag}_LEGACY.apk",
                                "$ghBase/STORM_SWITCH_${tag}_SDK27.apk"
                            )
                        )
                        candidates.add(
                            ReleaseCandidate(
                                release = NativeLibrary.UpdateResult(
                                    tag = tag,
                                    title = title,
                                    body = json.optString("body", ""),
                                    url = htmlUrl,
                                    assets = assetList.distinct().toMutableList()
                                ),
                                sourceMirror = "GitHub",
                                assetSizeBytes = matchingSize,
                                publishedDateFormatted = pubDate
                            )
                        )
                    }
                }
            }
        } catch (_: Throwable) {}

        // Source 3: GitHub HTML redirect fallback (immune to REST API rate limits)
        if (candidates.isEmpty()) {
            try {
                val webReq = Request.Builder()
                    .url(FALLBACK_RELEASE_URL)
                    .header("User-Agent", "Mozilla/5.0")
                    .build()
                client.newCall(webReq).execute().use { resp ->
                    val finalUrl = resp.request.url.toString()
                    val match = Regex("/releases/tag/(v?[0-9]+\\.[0-9]+(?:\\.[0-9]+)*)").find(finalUrl)
                    if (match != null) {
                        val rawTag = match.groupValues[1]
                        val cleanVer = StormUpdateDialogs.cleanVersionName(rawTag)
                        val baseDl = "https://github.com/ReiKatari/STORM_SWITCH/releases/download/$rawTag"
                        val sgwDl = "https://stormgamesworld.ru/uploads/programs/STORM_SWITCH"
                        val assets = mutableListOf(
                            "$sgwDl/STORM_SWITCH_$cleanVer.apk",
                            "$sgwDl/STORM_SWITCH_${cleanVer}_LEGACY.apk",
                            "$sgwDl/STORM_SWITCH_${cleanVer}_SDK27.apk",
                            "$baseDl/STORM_SWITCH_$cleanVer.apk",
                            "$baseDl/STORM_SWITCH_${cleanVer}_LEGACY.apk",
                            "$baseDl/STORM_SWITCH_${cleanVer}_SDK27.apk"
                        )
                        candidates.add(
                            ReleaseCandidate(
                                release = NativeLibrary.UpdateResult(
                                    tag = cleanVer,
                                    title = "STORM SWITCH $cleanVer",
                                    body = "",
                                    url = finalUrl,
                                    assets = assets
                                ),
                                sourceMirror = "GitHub",
                                assetSizeBytes = 0L,
                                publishedDateFormatted = ""
                            )
                        )
                    }
                }
            } catch (_: Throwable) {}
        }

        return candidates
    }

    private fun selectMatchingApkUrls(context: Context, release: NativeLibrary.UpdateResult): List<String> {
        val pkg = context.packageName.lowercase(Locale.US)
        val cleanVer = StormUpdateDialogs.cleanVersionName(release.tag)

        val expectedFileName = when {
            pkg.contains("legacy") -> "STORM_SWITCH_${cleanVer}_LEGACY.apk"
            pkg.contains("sdk27") -> "STORM_SWITCH_${cleanVer}_SDK27.apk"
            else -> "STORM_SWITCH_$cleanVer.apk"
        }

        val primaryMirror = "https://stormgamesworld.ru/uploads/programs/STORM_SWITCH/$expectedFileName"
        val secondaryMirror = "https://github.com/ReiKatari/STORM_SWITCH/releases/download/$cleanVer/$expectedFileName"

        val apkAssets = release.assets.filter { it.endsWith(".apk", ignoreCase = true) }
        val matchedFromAssets = when {
            pkg.contains("legacy") -> apkAssets.filter { it.endsWith("_LEGACY.apk", ignoreCase = true) }
            pkg.contains("sdk27") -> apkAssets.filter { it.endsWith("_SDK27.apk", ignoreCase = true) }
            else -> apkAssets.filter {
                !it.endsWith("_LEGACY.apk", ignoreCase = true) && !it.endsWith("_SDK27.apk", ignoreCase = true)
            }
        }

        val result = mutableListOf<String>()
        result.add(primaryMirror)
        for (u in matchedFromAssets) {
            if (u.contains("stormgamesworld.ru", ignoreCase = true) && !result.contains(u)) {
                result.add(u)
            }
        }
        if (!result.contains(secondaryMirror)) {
            result.add(secondaryMirror)
        }
        for (u in matchedFromAssets) {
            if (!result.contains(u)) result.add(u)
        }

        return result.distinct()
    }

    private fun downloadAndInstallUpdate(activity: Activity, candidate: ReleaseCandidate) {
        val release = candidate.release
        val matchingUrls = selectMatchingApkUrls(activity, release)
        val fallbackPage = release.url.ifBlank { FALLBACK_RELEASE_URL }
        if (matchingUrls.isEmpty()) {
            StormUpdateDialogs.showInfoDialog(
                activity = activity,
                isError = true,
                title = activity.getString(R.string.update_download_failed),
                message = activity.getString(R.string.online_tools_err_all_sources),
                onRetry = { downloadAndInstallUpdate(activity, candidate) },
                onOpenReleasePage = { openUrlSafe(activity, fallbackPage) }
            )
            return
        }

        val updatesDir = File(activity.cacheDir, "updates")
        if (!updatesDir.exists()) updatesDir.mkdirs()
        val targetApk = File(updatesDir, "storm_switch_update.apk")

        var downloader: APKDownloader? = null
        val movedToBackground = AtomicBoolean(false)
        val userCancelled = AtomicBoolean(false)

        val progressController = StormUpdateDialogs.showProgressDialog(
            activity = activity,
            versionName = release.tag,
            onMoveToBackground = {
                movedToBackground.set(true)
                Toast.makeText(
                    activity,
                    activity.getString(R.string.storm_update_background_toast),
                    Toast.LENGTH_SHORT
                ).show()
            },
            onCancel = {
                userCancelled.set(true)
                downloader?.cancel()
                try { if (targetApk.exists()) targetApk.delete() } catch (_: Throwable) {}
            }
        )

        var lastUiUpdateMs = 0L
        var lastSpeedSampleMs = System.currentTimeMillis()
        var lastSpeedSampleBytes = 0L
        var smoothedSpeedBps = 0.0

        downloader = APKDownloader(matchingUrls, targetApk)
        downloader.download(
            onProgress = { percent, currentBytes, totalBytes ->
                val now = System.currentTimeMillis()
                val dtSpeed = now - lastSpeedSampleMs
                if (dtSpeed >= 180L) {
                    val dBytes = currentBytes - lastSpeedSampleBytes
                    if (dBytes >= 0L) {
                        val instBps = (dBytes.toDouble() * 1000.0) / dtSpeed.toDouble()
                        smoothedSpeedBps = if (smoothedSpeedBps <= 1.0) {
                            instBps
                        } else {
                            smoothedSpeedBps * 0.65 + instBps * 0.35
                        }
                    }
                    lastSpeedSampleMs = now
                    lastSpeedSampleBytes = currentBytes
                }

                if (now - lastUiUpdateMs < 120L && percent < 100) {
                    return@download
                }
                lastUiUpdateMs = now
                val effectiveTotal = if (totalBytes > 0L) totalBytes else candidate.assetSizeBytes

                activity.runOnUiThread {
                    if (activity.isFinishing || activity.isDestroyed ||
                        movedToBackground.get() || userCancelled.get()
                    ) {
                        return@runOnUiThread
                    }
                    progressController.update(
                        percent = percent,
                        downloadedBytes = currentBytes,
                        totalBytes = effectiveTotal,
                        speedBytesPerSec = smoothedSpeedBps,
                        statusText = null
                    )
                }
            },
            onComplete = { success ->
                activity.runOnUiThread {
                    if (activity.isFinishing || activity.isDestroyed) return@runOnUiThread
                    progressController.dismissSafe()
                    if (userCancelled.get()) return@runOnUiThread

                    val archiveInfo = runCatching {
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                            activity.packageManager.getPackageArchiveInfo(
                                targetApk.absolutePath,
                                PackageManager.PackageInfoFlags.of(0L)
                            )
                        } else {
                            @Suppress("DEPRECATION")
                            activity.packageManager.getPackageArchiveInfo(targetApk.absolutePath, 0)
                        }
                    }.getOrNull()

                    val isPackageValid = archiveInfo != null && archiveInfo.packageName == activity.packageName

                    if (success && targetApk.exists() && isPackageValid && APKDownloader.isValidApkFile(targetApk)) {
                        APKInstaller.installApk(
                            context = activity,
                            apkFile = targetApk,
                            onComplete = {
                                Log.info("[StormUpdateManager] Install initiated successfully")
                            },
                            onFailure = { err ->
                                Toast.makeText(
                                    activity,
                                    activity.getString(R.string.update_install_failed, err.localizedMessage ?: ""),
                                    Toast.LENGTH_LONG
                                ).show()
                            }
                        )
                    } else {
                        StormUpdateDialogs.showInfoDialog(
                            activity = activity,
                            isError = true,
                            title = activity.getString(R.string.update_download_failed),
                            message = activity.getString(R.string.online_tools_err_all_sources),
                            onRetry = { downloadAndInstallUpdate(activity, candidate) },
                            onOpenReleasePage = { openUrlSafe(activity, fallbackPage) }
                        )
                    }
                }
            }
        )
    }

    private fun openUrlSafe(context: Context, url: String) {
        try {
            val intent = Intent(Intent.ACTION_VIEW, Uri.parse(url)).apply {
                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            }
            context.startActivity(intent)
        } catch (e: Throwable) {
            Log.warning("[StormUpdateManager] Failed to open release URL $url: ${e.message}")
        }
    }
}

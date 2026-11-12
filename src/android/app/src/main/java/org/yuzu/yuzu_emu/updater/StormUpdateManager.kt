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
import androidx.appcompat.app.AlertDialog
import androidx.preference.PreferenceManager
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import okhttp3.OkHttpClient
import okhttp3.Request
import org.json.JSONObject
import org.yuzu.yuzu_emu.BuildConfig
import org.yuzu.yuzu_emu.NativeLibrary
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.databinding.DialogDownloadProgressBinding
import org.yuzu.yuzu_emu.features.settings.model.BooleanSetting
import org.yuzu.yuzu_emu.utils.Log
import org.yuzu.yuzu_emu.utils.NativeConfig
import org.yuzu.yuzu_emu.utils.SmartDns
import java.io.File
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicBoolean

object StormUpdateManager {
    private const val PREF_HEALED_UPDATE_CHECK_VER = "storm_healed_update_check_ver"
    private const val AUTO_CHECK_COOLDOWN_MS = 120_000L

    private val isChecking = AtomicBoolean(false)

    @Volatile
    private var lastAutoCheckMs: Long = 0L

    @Volatile
    private var dismissedTagInSession: String? = null

    fun parseVersionParts(versionStr: String): List<Int> {
        val clean = versionStr.trim().removePrefix("v").removePrefix("V").trim()
        val parts = Regex("\\\\d+").findAll(clean).map { it.value.toIntOrNull() ?: 0 }.toMutableList()
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
        val cleanRemote = remoteTag.trim().removePrefix("v").removePrefix("V").trim()
        val cleanLocal = currentVer.trim().removePrefix("v").removePrefix("V").trim()
        if (cleanRemote.isEmpty() || cleanRemote.equals(cleanLocal, ignoreCase = true)) {
            return false
        }
        return compareVersions(cleanRemote, cleanLocal) > 0
    }

    private fun ensureUpdateChecksDefaultEnabled(activity: Activity) {
        try {
            val prefs = PreferenceManager.getDefaultSharedPreferences(activity.applicationContext)
            val currentVer = BuildConfig.VERSION_NAME.trim()
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
        } else {
            Toast.makeText(
                activity,
                activity.getString(R.string.storm_checking_updates),
                Toast.LENGTH_SHORT
            ).show()
        }

        if (!isChecking.compareAndSet(false, true)) return

        Thread {
            try {
                val candidates = fetchAllRemoteReleases()
                val currentVersion = BuildConfig.VERSION_NAME.trim()
                val bestRelease = candidates.maxWithOrNull { a, b -> compareVersions(a.tag, b.tag) }

                activity.runOnUiThread {
                    if (activity.isFinishing || activity.isDestroyed) return@runOnUiThread
                    if (bestRelease == null) {
                        if (manual) {
                            Toast.makeText(
                                activity,
                                activity.getString(R.string.update_download_failed),
                                Toast.LENGTH_SHORT
                            ).show()
                        }
                        return@runOnUiThread
                    }

                    val cleanRemote = bestRelease.tag.trim().removePrefix("v").removePrefix("V").trim()
                    if (!isRemoteVersionNewer(cleanRemote, currentVersion)) {
                        if (manual) {
                            Toast.makeText(
                                activity,
                                activity.getString(R.string.storm_up_to_date_fmt, currentVersion),
                                Toast.LENGTH_SHORT
                            ).show()
                        }
                        return@runOnUiThread
                    }

                    if (!manual && dismissedTagInSession == cleanRemote) {
                        return@runOnUiThread
                    }

                    showUpdateDialog(activity, bestRelease)
                }
            } catch (_: Throwable) {
                if (manual) {
                    activity.runOnUiThread {
                        if (!activity.isFinishing && !activity.isDestroyed) {
                            Toast.makeText(
                                activity,
                                activity.getString(R.string.update_download_failed),
                                Toast.LENGTH_SHORT
                            ).show()
                        }
                    }
                }
            } finally {
                isChecking.set(false)
            }
        }.start()
    }

    private fun fetchAllRemoteReleases(): List<NativeLibrary.UpdateResult> {
        val candidates = mutableListOf<NativeLibrary.UpdateResult>()
        val client = OkHttpClient.Builder()
            .dns(SmartDns)
            .connectTimeout(8, TimeUnit.SECONDS)
            .readTimeout(10, TimeUnit.SECONDS)
            .followRedirects(true)
            .followSslRedirects(true)
            .build()

        // Source 1: Official STORM GAMES WORLD API (fast direct endpoint in RU and Worldwide)
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
                            val ver = rel.optString("version", "").trim().removePrefix("v").removePrefix("V")
                            if (ver.isNotEmpty()) {
                                val title = rel.optString("title", "STORM SWITCH $ver")
                                val notes = rel.optString("notes", rel.optString("body", ""))
                                val assets = mutableListOf<String>()
                                val assetsArr = rel.optJSONArray("assets")
                                if (assetsArr != null) {
                                    for (j in 0 until assetsArr.length()) {
                                        val dlUrl = assetsArr.getJSONObject(j).optString("downloadUrl", "")
                                        if (dlUrl.isNotEmpty()) assets.add(dlUrl)
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
                                    NativeLibrary.UpdateResult(
                                        tag = ver,
                                        title = title,
                                        body = notes,
                                        url = "https://github.com/ReiKatari/STORM_SWITCH/releases/tag/$ver",
                                        assets = assets.distinct().toMutableList()
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
                    val tag = json.optString("tag_name", "").trim().removePrefix("v").removePrefix("V")
                    if (tag.isNotEmpty()) {
                        val title = json.optString("name", "STORM SWITCH $tag")
                        val htmlUrl = json.optString("html_url", "https://github.com/ReiKatari/STORM_SWITCH/releases/tag/$tag")
                        val assetsArr = json.optJSONArray("assets")
                        val assetList = mutableListOf<String>()
                        if (assetsArr != null) {
                            for (i in 0 until assetsArr.length()) {
                                val u = assetsArr.getJSONObject(i).optString("browser_download_url", "")
                                if (u.isNotEmpty()) assetList.add(u)
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
                            NativeLibrary.UpdateResult(
                                tag = tag,
                                title = title,
                                body = json.optString("body", ""),
                                url = htmlUrl,
                                assets = assetList.distinct().toMutableList()
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
                    .url("https://github.com/ReiKatari/STORM_SWITCH/releases/latest")
                    .header("User-Agent", "Mozilla/5.0")
                    .build()
                client.newCall(webReq).execute().use { resp ->
                    val finalUrl = resp.request.url.toString()
                    val match = Regex("/releases/tag/(v?[0-9]+\\\\.[0-9]+(?:\\\\.[0-9]+)*)").find(finalUrl)
                    if (match != null) {
                        val rawTag = match.groupValues[1]
                        val cleanVer = rawTag.removePrefix("v").removePrefix("V")
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
                            NativeLibrary.UpdateResult(
                                tag = cleanVer,
                                title = "STORM SWITCH $cleanVer",
                                body = "",
                                url = finalUrl,
                                assets = assets
                            )
                        )
                    }
                }
            } catch (_: Throwable) {}
        }

        return candidates
    }

    private fun extractCleanSummary(body: String): String {
        if (body.isBlank()) return ""
        val beforeDetails = body.substringBefore("<details>").trim()
        val cleaned = beforeDetails
            .lines()
            .map { it.trim() }
            .filter { it.isNotEmpty() && it != "---" }
            .joinToString("\\n") { line ->
                line.removePrefix("> ")
                    .replace("**", "")
                    .replace("*", "")
                    .replace("`", "")
                    .replace("\u2014", "-")
                    .replace("\u2013", "-")
            }
        return if (cleaned.length > 650) cleaned.take(650) + "..." else cleaned
    }

    private fun showUpdateDialog(activity: Activity, release: NativeLibrary.UpdateResult) {
        val cleanTag = release.tag.trim().removePrefix("v").removePrefix("V").trim()
        val summary = extractCleanSummary(release.body)
        val baseMsg = activity.getString(R.string.update_available_description, release.title.ifBlank { "STORM SWITCH $cleanTag" })
        val fullMsg = if (summary.isNotBlank()) "$baseMsg\\n\\n$summary" else baseMsg

        MaterialAlertDialogBuilder(activity)
            .setTitle(R.string.update_available)
            .setMessage(fullMsg)
            .setPositiveButton(R.string.update) { _, _ ->
                downloadAndInstallUpdate(activity, release)
            }
            .setNegativeButton(R.string.cancel) { dialog, _ ->
                dismissedTagInSession = cleanTag
                dialog.dismiss()
            }
            .show()
    }

    private fun selectMatchingApkUrls(context: Context, release: NativeLibrary.UpdateResult): List<String> {
        val pkg = context.packageName.lowercase()
        val cleanVer = release.tag.trim().removePrefix("v").removePrefix("V").trim()

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
        // 1. Primary fast mirror first
        result.add(primaryMirror)
        for (u in matchedFromAssets) {
            if (u.contains("stormgamesworld.ru", ignoreCase = true)) {
                if (!result.contains(u)) result.add(u)
            }
        }
        // 2. Secondary GitHub Release mirror
        if (!result.contains(secondaryMirror)) {
            result.add(secondaryMirror)
        }
        for (u in matchedFromAssets) {
            if (!result.contains(u)) result.add(u)
        }

        return result.distinct()
    }

    private fun downloadAndInstallUpdate(activity: Activity, release: NativeLibrary.UpdateResult) {
        val matchingUrls = selectMatchingApkUrls(activity, release)
        if (matchingUrls.isEmpty()) {
            MaterialAlertDialogBuilder(activity)
                .setTitle(R.string.update_download_failed)
                .setMessage(R.string.online_tools_err_all_sources)
                .setPositiveButton(R.string.retry) { _, _ ->
                    downloadAndInstallUpdate(activity, release)
                }
                .setNegativeButton(R.string.cancel, null)
                .show()
            return
        }

        val updatesDir = File(activity.cacheDir, "updates")
        if (!updatesDir.exists()) updatesDir.mkdirs()
        val targetApk = File(updatesDir, "storm_switch_update.apk")

        val dialogBinding = DialogDownloadProgressBinding.inflate(activity.layoutInflater)
        dialogBinding.downloadProgressBar.isIndeterminate = false
        dialogBinding.downloadProgressBar.progress = 0
        dialogBinding.downloadProgressMessage.text = "0% (0.0 MB / 0.0 MB)"

        var downloader: APKDownloader? = null

        val progressDialog = MaterialAlertDialogBuilder(activity)
            .setTitle(R.string.downloading_update)
            .setView(dialogBinding.root)
            .setNegativeButton(R.string.cancel) { dialog, _ ->
                downloader?.cancel()
                try { if (targetApk.exists()) targetApk.delete() } catch (_: Throwable) {}
                dialog.dismiss()
            }
            .setCancelable(false)
            .create()

        progressDialog.show()

        downloader = APKDownloader(matchingUrls, targetApk)
        downloader.download(
            onProgress = { percent, currentBytes, totalBytes ->
                activity.runOnUiThread {
                    if (activity.isFinishing || activity.isDestroyed) return@runOnUiThread
                    dialogBinding.downloadProgressBar.progress = percent
                    val curMb = currentBytes / (1024.0 * 1024.0)
                    if (totalBytes > 0) {
                        val totalMb = totalBytes / (1024.0 * 1024.0)
                        dialogBinding.downloadProgressMessage.text = String.format(
                            java.util.Locale.US,
                            "%d%% (%.1f MB / %.1f MB)",
                            percent,
                            curMb,
                            totalMb
                        )
                    } else {
                        dialogBinding.downloadProgressMessage.text = String.format(
                            java.util.Locale.US,
                            "%d%% (%.1f MB)",
                            percent,
                            curMb
                        )
                    }
                }
            },
            onComplete = { success ->
                activity.runOnUiThread {
                    if (activity.isFinishing || activity.isDestroyed) return@runOnUiThread
                    progressDialog.dismiss()

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
                        MaterialAlertDialogBuilder(activity)
                            .setTitle(R.string.update_download_failed)
                            .setMessage(R.string.online_tools_err_all_sources)
                            .setPositiveButton(R.string.retry) { _, _ ->
                                downloadAndInstallUpdate(activity, release)
                            }
                            .setNegativeButton(R.string.cancel, null)
                            .setCancelable(false)
                            .show()
                    }
                }
            }
        )
    }
}

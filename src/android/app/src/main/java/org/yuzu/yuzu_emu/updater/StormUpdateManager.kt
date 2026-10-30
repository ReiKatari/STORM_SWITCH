// SPDX-FileCopyrightText: 2026 STORM SWITCH Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.updater

import android.app.Activity
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.preference.PreferenceManager
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import com.google.android.material.progressindicator.LinearProgressIndicator
import com.google.android.material.textview.MaterialTextView
import okhttp3.OkHttpClient
import okhttp3.Request
import org.json.JSONObject
import org.yuzu.yuzu_emu.BuildConfig
import org.yuzu.yuzu_emu.NativeLibrary
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.features.settings.model.BooleanSetting
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

        // Source 1: Official STORM GAMES WORLD API (fast direct endpoint in RU & Worldwide)
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
                        assetList.addAll(
                            listOf(
                                "$sgwBase/STORM_SWITCH_$tag.apk",
                                "$sgwBase/STORM_SWITCH_${tag}_LEGACY.apk",
                                "$sgwBase/STORM_SWITCH_${tag}_SDK27.apk"
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
                    val match = Regex("/releases/tag/(v?[0-9]+\\.[0-9]+(?:\\.[0-9]+)*)").find(finalUrl)
                    if (match != null) {
                        val rawTag = match.groupValues[1]
                        val cleanVer = rawTag.removePrefix("v").removePrefix("V")
                        val baseDl = "https://github.com/ReiKatari/STORM_SWITCH/releases/download/$rawTag"
                        val sgwDl = "https://stormgamesworld.ru/uploads/programs/STORM_SWITCH"
                        val assets = mutableListOf(
                            "$baseDl/STORM_SWITCH_$cleanVer.apk",
                            "$baseDl/STORM_SWITCH_${cleanVer}_LEGACY.apk",
                            "$baseDl/STORM_SWITCH_${cleanVer}_SDK27.apk",
                            "$sgwDl/STORM_SWITCH_$cleanVer.apk",
                            "$sgwDl/STORM_SWITCH_${cleanVer}_LEGACY.apk",
                            "$sgwDl/STORM_SWITCH_${cleanVer}_SDK27.apk"
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
            .joinToString("\n") { line ->
                line.removePrefix("> ")
                    .replace("**", "")
                    .replace("*", "")
                    .replace("`", "")
            }
        return if (cleaned.length > 650) cleaned.take(650) + "…" else cleaned
    }

    private fun showUpdateDialog(activity: Activity, release: NativeLibrary.UpdateResult) {
        val cleanTag = release.tag.trim().removePrefix("v").removePrefix("V").trim()
        val summary = extractCleanSummary(release.body)
        val baseMsg = activity.getString(R.string.update_available_description, release.title.ifBlank { "STORM SWITCH $cleanTag" })
        val fullMsg = if (summary.isNotBlank()) "$baseMsg\n\n$summary" else baseMsg

        MaterialAlertDialogBuilder(activity)
            .setTitle(R.string.update_available)
            .setMessage(fullMsg)
            .setPositiveButton(R.string.update) { _, _ ->
                if (release.assets.isEmpty()) {
                    openLink(activity, release.url)
                } else {
                    downloadAndInstallUpdate(activity, release)
                }
            }
            .setNegativeButton(R.string.cancel) { dialog, _ ->
                dismissedTagInSession = cleanTag
                dialog.dismiss()
            }
            .show()
    }

    private fun selectMatchingApkUrls(release: NativeLibrary.UpdateResult): List<String> {
        val flavor = BuildConfig.FLAVOR.lowercase()
        val isOlderDevice = Build.VERSION.SDK_INT < 33
        val apkAssets = release.assets.filter { it.endsWith(".apk", ignoreCase = true) }

        val matched = when {
            flavor.contains("legacy") -> {
                apkAssets.filter { it.contains("LEGACY", ignoreCase = true) }
            }
            flavor.contains("sdk27") || (Build.VERSION.SDK_INT in 27..32 && !flavor.contains("legacy")) -> {
                apkAssets.filter { it.contains("SDK27", ignoreCase = true) }
                    .ifEmpty { if (isOlderDevice) apkAssets.filter { it.contains("LEGACY", ignoreCase = true) } else emptyList() }
            }
            else -> {
                if (isOlderDevice) {
                    apkAssets.filter { it.contains("LEGACY", ignoreCase = true) || it.contains("SDK27", ignoreCase = true) }
                } else {
                    apkAssets.filter {
                        !it.contains("LEGACY", ignoreCase = true) && !it.contains("SDK27", ignoreCase = true)
                    }
                }
            }
        }
        return matched.distinct()
    }

    private fun downloadAndInstallUpdate(activity: Activity, release: NativeLibrary.UpdateResult) {
        val matchingUrls = selectMatchingApkUrls(release)
        if (matchingUrls.isEmpty()) {
            openLink(activity, release.url)
            return
        }

        val artifactName = matchingUrls.first().substringAfterLast("/").removeSuffix(".apk")
        val apkFile = File(activity.cacheDir, "update-$artifactName.apk")

        val progressView = activity.layoutInflater.inflate(R.layout.dialog_download_progress, null)
        val progressBar = progressView.findViewById<LinearProgressIndicator>(R.id.download_progress_bar)
        val progressMessage = progressView.findViewById<MaterialTextView>(R.id.download_progress_message)
        progressBar?.max = 100
        progressBar?.progress = 0
        progressMessage?.text = "0%"

        val progressDialog: AlertDialog = MaterialAlertDialogBuilder(activity)
            .setTitle(R.string.downloading_update)
            .setView(progressView)
            .setCancelable(false)
            .create()
        progressDialog.show()

        val downloader = APKDownloader(matchingUrls, apkFile)
        downloader.download(
            onProgress = { pct ->
                activity.runOnUiThread {
                    if (!activity.isFinishing && !activity.isDestroyed) {
                        progressBar?.progress = pct
                        progressMessage?.text = "$pct%"
                    }
                }
            },
            onComplete = { success ->
                activity.runOnUiThread {
                    try {
                        if (progressDialog.isShowing) progressDialog.dismiss()
                    } catch (_: Throwable) {}

                    if (activity.isFinishing || activity.isDestroyed) return@runOnUiThread

                    if (success && APKDownloader.isValidApkFile(apkFile)) {
                        APKInstaller(activity).install(
                            apkFile,
                            onComplete = {
                                Toast.makeText(
                                    activity.applicationContext,
                                    R.string.update_installed_successfully,
                                    Toast.LENGTH_LONG
                                ).show()
                            },
                            onFailure = { e ->
                                Toast.makeText(
                                    activity.applicationContext,
                                    activity.getString(R.string.update_install_failed, e.message ?: ""),
                                    Toast.LENGTH_LONG
                                ).show()
                            }
                        )
                    } else {
                        try {
                            if (apkFile.exists()) apkFile.delete()
                        } catch (_: Throwable) {}
                        Toast.makeText(
                            activity,
                            activity.getString(R.string.update_download_failed),
                            Toast.LENGTH_LONG
                        ).show()
                    }
                }
            }
        )
    }

    private fun openLink(activity: Activity, link: String) {
        try {
            val intent = Intent(Intent.ACTION_VIEW, Uri.parse(link))
            activity.startActivity(intent)
        } catch (_: Throwable) {}
    }
}

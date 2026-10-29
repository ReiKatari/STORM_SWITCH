// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.ui.main

import android.content.Intent
import android.content.Context
import android.net.Uri
import android.os.Bundle
import android.view.View
import android.view.ViewGroup.MarginLayoutParams
import android.view.WindowManager
import android.view.animation.PathInterpolator
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.core.splashscreen.SplashScreen.Companion.installSplashScreen
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.navigation.NavController
import androidx.navigation.fragment.NavHostFragment
import androidx.preference.PreferenceManager
import com.google.android.material.color.MaterialColors
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import java.io.File
import org.yuzu.yuzu_emu.NativeLibrary
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.databinding.ActivityMainBinding
import org.yuzu.yuzu_emu.dialogs.NetPlayDialog
import org.yuzu.yuzu_emu.features.settings.model.Settings
import org.yuzu.yuzu_emu.fragments.AddGameFolderDialogFragment
import org.yuzu.yuzu_emu.fragments.MessageDialogFragment
import org.yuzu.yuzu_emu.model.AddonViewModel
import org.yuzu.yuzu_emu.model.DriverViewModel
import org.yuzu.yuzu_emu.model.GamesViewModel
import org.yuzu.yuzu_emu.model.HomeViewModel
import android.os.Build
import org.yuzu.yuzu_emu.model.TaskViewModel
import org.yuzu.yuzu_emu.utils.*
import org.yuzu.yuzu_emu.utils.ViewUtils.setVisible
import androidx.core.content.edit
import org.yuzu.yuzu_emu.activities.EmulationActivity
import kotlin.text.compareTo
import androidx.core.net.toUri
import com.google.android.material.progressindicator.LinearProgressIndicator
import com.google.android.material.textview.MaterialTextView
import org.yuzu.yuzu_emu.features.settings.model.BooleanSetting
import org.yuzu.yuzu_emu.YuzuApplication
import org.yuzu.yuzu_emu.updater.APKDownloader
import org.yuzu.yuzu_emu.updater.APKInstaller
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import androidx.documentfile.provider.DocumentFile

class MainActivity : AppCompatActivity(), ThemeProvider {
    private lateinit var binding: ActivityMainBinding

    private val homeViewModel: HomeViewModel by viewModels()
    private val gamesViewModel: GamesViewModel by viewModels()
    private val taskViewModel: TaskViewModel by viewModels()
    private val addonViewModel: AddonViewModel by viewModels()
    private val driverViewModel: DriverViewModel by viewModels()

    override var themeId: Int = 0

    private val CHECKED_DECRYPTION = "CheckedDecryption"
    private var checkedDecryption = false

    private val requestStoragePermissionLauncher = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            if (android.os.Environment.isExternalStorageManager()) {
                DirectoryInitialization.initializeSharedStorage()
            }
        }
    }

    private val requestLegacyStorageLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { permissions ->
        if (permissions[android.Manifest.permission.WRITE_EXTERNAL_STORAGE] == true ||
            permissions[android.Manifest.permission.READ_EXTERNAL_STORAGE] == true) {
            DirectoryInitialization.initializeSharedStorage()
        }
    }

    fun checkAllFilesAccessPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            if (!android.os.Environment.isExternalStorageManager()) {
                try {
                    val intent = Intent(android.provider.Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION).apply {
                        data = Uri.fromParts("package", packageName, null)
                    }
                    requestStoragePermissionLauncher.launch(intent)
                } catch (_: Exception) {
                    try {
                        val intent = Intent(android.provider.Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION)
                        requestStoragePermissionLauncher.launch(intent)
                    } catch (_: Exception) {}
                }
            } else {
                DirectoryInitialization.initializeSharedStorage()
            }
        } else {
            val writeGranted = ContextCompat.checkSelfPermission(this, android.Manifest.permission.WRITE_EXTERNAL_STORAGE) == android.content.pm.PackageManager.PERMISSION_GRANTED
            val readGranted = ContextCompat.checkSelfPermission(this, android.Manifest.permission.READ_EXTERNAL_STORAGE) == android.content.pm.PackageManager.PERMISSION_GRANTED
            if (!writeGranted || !readGranted) {
                requestLegacyStorageLauncher.launch(arrayOf(
                    android.Manifest.permission.READ_EXTERNAL_STORAGE,
                    android.Manifest.permission.WRITE_EXTERNAL_STORAGE
                ))
            } else {
                DirectoryInitialization.initializeSharedStorage()
            }
        }
    }

    override fun attachBaseContext(base: Context) {
        super.attachBaseContext(YuzuApplication.applyLanguage(base))
        CrashHandler.install(base)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        try {
            val splashScreen = installSplashScreen()
            splashScreen.setKeepOnScreenCondition { !DirectoryInitialization.areDirectoriesReady }
        } catch (t: Throwable) {
            CrashHandler.logError(this, "MainActivity.installSplashScreen", t)
        }

        try {
            ThemeHelper.ThemeChangeListener(this)
            ThemeHelper.setTheme(this)
        } catch (t: Throwable) {
            CrashHandler.logError(this, "MainActivity.ThemeHelper", t)
        }

        super.onCreate(savedInstanceState)

        val firstTimeSetup = PreferenceManager.getDefaultSharedPreferences(applicationContext)
            .getBoolean(Settings.PREF_FIRST_APP_LAUNCH, true)
        if (!firstTimeSetup) {
            checkAllFilesAccessPermission()
        }

        try {
            NativeLibrary.initMultiplayer()
        } catch (t: Throwable) {
            CrashHandler.logError(this, "MainActivity.initMultiplayer", t)
        }

        binding = ActivityMainBinding.inflate(layoutInflater)

        display?.let {
            val supportedModes = it.supportedModes
            val maxRefreshRate = supportedModes.maxByOrNull { mode -> mode.refreshRate }

            if (maxRefreshRate != null) {
                val layoutParams = window.attributes
                layoutParams.preferredDisplayModeId = maxRefreshRate.modeId
                window.attributes = layoutParams
            }
        }

        setContentView(binding.root)

        if (savedInstanceState != null) {
            checkedDecryption = savedInstanceState.getBoolean(CHECKED_DECRYPTION)
        }
        if (!checkedDecryption) {
            StormHardwareCalibrator.autoCalibrate(applicationContext)
            val firstTimeSetup = PreferenceManager.getDefaultSharedPreferences(applicationContext)
                .getBoolean(Settings.PREF_FIRST_APP_LAUNCH, true)
            if (!firstTimeSetup) {
                checkKeys()
            }
            checkedDecryption = true
        }

        WindowCompat.setDecorFitsSystemWindows(window, false)
        window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_NOTHING)
        ThemeHelper.applySystemBarsTheme(window, this)

        window.statusBarColor =
            ContextCompat.getColor(applicationContext, android.R.color.transparent)
        window.navigationBarColor =
            ContextCompat.getColor(applicationContext, android.R.color.transparent)

        binding.statusBarShade.setBackgroundColor(
            ThemeHelper.getColorWithOpacity(
                MaterialColors.getColor(
                    binding.root,
                    com.google.android.material.R.attr.colorSurface
                ),
                ThemeHelper.SYSTEM_BAR_ALPHA
            )
        )
        if (InsetsHelper.getSystemGestureType(applicationContext) != InsetsHelper.GESTURE_NAVIGATION) {
            binding.navigationBarShade.setBackgroundColor(
                ThemeHelper.getColorWithOpacity(
                    MaterialColors.getColor(
                        binding.root,
                        com.google.android.material.R.attr.colorSurface
                    ),
                    ThemeHelper.SYSTEM_BAR_ALPHA
                )
            )
        }

        if (FullscreenHelper.isFullscreenEnabled(applicationContext)) {
            binding.statusBarShade.visibility = View.GONE
            binding.navigationBarShade.visibility = View.GONE
        }

        val navHostFragment =
            supportFragmentManager.findFragmentById(R.id.fragment_container) as NavHostFragment
        setUpNavigation(navHostFragment.navController)

        homeViewModel.statusBarShadeVisible.collect(this) { showStatusBarShade(it) }
        homeViewModel.contentToInstall.collect(
            this,
            resetState = { homeViewModel.setContentToInstall(null) }
        ) {
            if (it != null) {
                installContent(it)
            }
        }
        homeViewModel.checkKeys.collect(this, resetState = { homeViewModel.setCheckKeys(false) }) {
            if (it) checkKeys()
        }

        // Dismiss previous notifications (should not happen unless a crash occurred)
        EmulationActivity.stopForegroundService(this)

        ThemeHelper.applySystemBarsTheme(window, this)

        val isFirstLaunch = PreferenceManager.getDefaultSharedPreferences(applicationContext)
                .getBoolean(Settings.PREF_FIRST_APP_LAUNCH, true)

        if (!isFirstLaunch && NativeLibrary.isUpdateCheckerEnabled() && BooleanSetting.ENABLE_UPDATE_CHECKS.getBoolean()) {
             checkForUpdates()
        }
        setInsets()
        applyFullscreenPreference()

        // Synchronize STORM GAMES WORLD catalog in background on app launch
        CoroutineScope(Dispatchers.IO).launch {
            try {
                org.yuzu.yuzu_emu.fragments.StormGamesWorldDialogFragment.syncCatalogInBackground(applicationContext)
            } catch (_: Throwable) {}
        }
    }

    private fun parseVersionParts(versionStr: String): List<Int> {
        val clean = versionStr.trim().removePrefix("v").removePrefix("V").trim()
        val parts = Regex("\\d+").findAll(clean).map { it.value.toIntOrNull() ?: 0 }.toMutableList()
        while (parts.size < 3) {
            parts.add(0)
        }
        return parts
    }

    private fun compareVersions(v1: String, v2: String): Int {
        val p1 = parseVersionParts(v1)
        val p2 = parseVersionParts(v2)
        val maxLen = maxOf(p1.size, p2.size)
        for (i in 0 until maxLen) {
            val a = if (i < p1.size) p1[i] else 0
            val b = if (i < p2.size) p2[i] else 0
            if (a != b) return a.compareTo(b)
        }
        return 0
    }

    private fun isRemoteVersionNewer(remoteTag: String, currentVer: String): Boolean {
        val cleanRemote = remoteTag.trim().removePrefix("v").removePrefix("V").trim()
        val cleanLocal = currentVer.trim().removePrefix("v").removePrefix("V").trim()
        if (cleanRemote.isEmpty() || cleanRemote.equals(cleanLocal, ignoreCase = true)) {
            return false
        }
        return compareVersions(cleanRemote, cleanLocal) > 0
    }

    private fun fetchAllRemoteReleases(): List<NativeLibrary.UpdateResult> {
        val candidates = mutableListOf<NativeLibrary.UpdateResult>()
        val client = okhttp3.OkHttpClient.Builder()
            .connectTimeout(15, java.util.concurrent.TimeUnit.SECONDS)
            .readTimeout(15, java.util.concurrent.TimeUnit.SECONDS)
            .followRedirects(true)
            .followSslRedirects(true)
            .build()

        // Mirror 1: GitHub REST API
        try {
            val req = okhttp3.Request.Builder()
                .url("https://api.github.com/repos/ReiKatari/STORM_SWITCH/releases/latest")
                .header("User-Agent", "STORM-SWITCH-Updater")
                .header("Accept", "application/vnd.github.v3+json")
                .build()
            client.newCall(req).execute().use { resp ->
                if (resp.isSuccessful) {
                    val bodyStr = resp.body?.string() ?: ""
                    val json = org.json.JSONObject(bodyStr)
                    val tag = json.optString("tag_name", "").trim()
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
                        candidates.add(NativeLibrary.UpdateResult(tag = tag, title = title, body = json.optString("body", ""), url = htmlUrl, assets = assetList))
                    }
                }
            }
        } catch (_: Exception) {}

        // Fallback GitHub HTML redirect if API failed or rate-limited
        if (candidates.isEmpty()) {
            try {
                val webReq = okhttp3.Request.Builder()
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
                        candidates.add(NativeLibrary.UpdateResult(tag = rawTag, title = "STORM SWITCH $cleanVer", body = "", url = finalUrl, assets = assets))
                    }
                }
            } catch (_: Exception) {}
        }

        // Mirror 2: Official STORM GAMES WORLD Mirror (https://stormgamesworld.ru/api/programs?repo=STORM_SWITCH)
        try {
            val sgwReq = okhttp3.Request.Builder()
                .url("https://stormgamesworld.ru/api/programs?repo=STORM_SWITCH")
                .header("User-Agent", "STORM-SWITCH-Android")
                .header("Accept", "application/json")
                .build()
            client.newCall(sgwReq).execute().use { resp ->
                if (resp.isSuccessful) {
                    val bodyStr = resp.body?.string() ?: ""
                    val root = org.json.JSONObject(bodyStr)
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
                                if (assetsArr != null && assetsArr.length() > 0) {
                                    for (j in 0 until assetsArr.length()) {
                                        val aObj = assetsArr.getJSONObject(j)
                                        val dlUrl = aObj.optString("downloadUrl", "")
                                        if (dlUrl.isNotEmpty()) assets.add(dlUrl)
                                    }
                                }
                                if (assets.isEmpty()) {
                                    val sgwDl = "https://stormgamesworld.ru/uploads/programs/STORM_SWITCH"
                                    assets.addAll(listOf(
                                        "$sgwDl/STORM_SWITCH_$ver.apk",
                                        "$sgwDl/STORM_SWITCH_${ver}_LEGACY.apk",
                                        "$sgwDl/STORM_SWITCH_${ver}_SDK27.apk"
                                    ))
                                }
                                candidates.add(
                                    NativeLibrary.UpdateResult(
                                        tag = ver,
                                        title = title,
                                        body = notes,
                                        url = "https://stormgamesworld.ru/programs?repo=STORM_SWITCH",
                                        assets = assets
                                    )
                                )
                                break
                            }
                        }
                    }
                }
            }
        } catch (_: Exception) {}

        // Also check NativeLibrary
        try {
            val nativeResult = NativeLibrary.checkForUpdate()
            if (nativeResult != null && nativeResult.tag.isNotBlank()) {
                candidates.add(nativeResult)
            }
        } catch (_: Exception) {}

        return candidates
    }

    private fun checkForUpdates() {
        Thread {
            val candidates = fetchAllRemoteReleases()
            if (candidates.isEmpty()) return@Thread

            val currentVersion = org.yuzu.yuzu_emu.BuildConfig.VERSION_NAME.trim()
            val bestRelease = candidates.maxWithOrNull { a, b -> compareVersions(a.tag, b.tag) } ?: return@Thread

            if (!isRemoteVersionNewer(bestRelease.tag, currentVersion)) {
                return@Thread
            }
            runOnUiThread {
                showUpdateDialog(bestRelease)
            }
        }.start()
    }

    private fun showUpdateDialog(release: NativeLibrary.UpdateResult) {
        MaterialAlertDialogBuilder(this)
            .setTitle(R.string.update_available)
            .setMessage(getString(R.string.update_available_description, release.title))
            .setPositiveButton(R.string.update) { _, _ ->
                val assets = release.assets

                if (assets.isEmpty()) {
                    openLink(release.url)
                } else {
                    downloadAndInstallUpdate(release)
                }
            }
            .setNeutralButton(R.string.cancel) { dialog, _ ->
                dialog.dismiss()
            }
            .setNegativeButton(R.string.dont_show_again) { dialog, _ ->
                BooleanSetting.ENABLE_UPDATE_CHECKS.setBoolean(false)
                NativeConfig.saveGlobalConfig()
                dialog.dismiss()
            }
            .show()
    }

    private fun openLink(link: String) {
        val intent = Intent(Intent.ACTION_VIEW, link.toUri())
        startActivity(intent)
    }

    private fun downloadAndInstallUpdate(release: NativeLibrary.UpdateResult) {
        CoroutineScope(Dispatchers.IO).launch {
            val packageId = applicationContext.packageName
            val flavor = org.yuzu.yuzu_emu.BuildConfig.FLAVOR.lowercase()
            val asset = when {
                flavor.contains("legacy") -> {
                    release.assets.firstOrNull { it.contains("LEGACY", ignoreCase = true) && it.endsWith(".apk", ignoreCase = true) }
                }
                flavor.contains("sdk27") -> {
                    release.assets.firstOrNull { it.contains("SDK27", ignoreCase = true) && it.endsWith(".apk", ignoreCase = true) }
                }
                else -> {
                    release.assets.firstOrNull {
                        !it.contains("LEGACY", ignoreCase = true) &&
                            !it.contains("SDK27", ignoreCase = true) &&
                            it.endsWith(".apk", ignoreCase = true)
                    }
                }
            } ?: release.assets.firstOrNull { it.endsWith(".apk", ignoreCase = true) } ?: release.assets[0]
            val artifact = asset.split("/").last().removeSuffix(".apk")
            val apkFile = File(cacheDir, "update-$artifact.apk")

            withContext(Dispatchers.Main) {
                showDownloadProgressDialog()
            }

            val downloader = APKDownloader(asset, apkFile)
            downloader.download(
                onProgress = { progress ->
                    runOnUiThread {
                        updateDownloadProgress(progress)
                    }
                },
                onComplete = { success ->
                    runOnUiThread {
                        dismissDownloadProgressDialog()
                        if (success) {
                            val isApkValid = try {
                                if (apkFile.exists() && apkFile.length() > 1024 * 1024) {
                                    val magic = ByteArray(4)
                                    val readBytes = apkFile.inputStream().use { it.read(magic) }
                                    readBytes == 4 && magic[0] == 0x50.toByte() && magic[1] == 0x4B.toByte() && magic[2] == 0x03.toByte() && magic[3] == 0x04.toByte()
                                } else {
                                    false
                                }
                            } catch (_: Exception) {
                                false
                            }

                            if (!isApkValid) {
                                try { apkFile.delete() } catch (_: Exception) {}
                                Toast.makeText(
                                    this@MainActivity,
                                    getString(R.string.update_download_failed) + " (Invalid APK package)",
                                    Toast.LENGTH_LONG
                                ).show()
                                return@runOnUiThread
                            }

                            val installer = APKInstaller(this@MainActivity)
                            installer.install(
                                apkFile,
                                onComplete = {
                                    Toast.makeText(
                                        this@MainActivity,
                                        R.string.update_installed_successfully,
                                        Toast.LENGTH_LONG
                                    ).show()
                                },
                                onFailure = { exception ->
                                    Toast.makeText(
                                        this@MainActivity,
                                        getString(R.string.update_install_failed, exception.message),
                                        Toast.LENGTH_LONG
                                    ).show()
                                }
                            )
                        } else {
                            Toast.makeText(
                                this@MainActivity,
                                getString(R.string.update_download_failed) + "\n\nURL: $asset",
                                Toast.LENGTH_LONG
                            ).show()
                        }
                    }
                }
            )
        }
    }

    private var progressDialog: androidx.appcompat.app.AlertDialog? = null
    private var progressBar: LinearProgressIndicator? = null
    private var progressMessage: MaterialTextView? = null

    private fun showDownloadProgressDialog() {
        val progressView = layoutInflater.inflate(R.layout.dialog_download_progress, null)
        progressBar = progressView.findViewById(R.id.download_progress_bar)
        progressMessage = progressView.findViewById(R.id.download_progress_message)

        progressDialog = MaterialAlertDialogBuilder(this)
            .setTitle(R.string.downloading_update)
            .setView(progressView)
            .setCancelable(false)
            .create()
        progressDialog?.show()
    }

    private fun updateDownloadProgress(progress: Int) {
        progressBar?.progress = progress
        progressMessage?.text = getString(R.string.percent, progress)
    }

    private fun dismissDownloadProgressDialog() {
        progressDialog?.dismiss()
        progressDialog = null
        progressBar = null
        progressMessage = null
    }


    fun displayMultiplayerDialog() {
        val dialog = NetPlayDialog(this)
        dialog.show()
    }

    private fun checkKeys() {
        if (!NativeLibrary.areKeysPresent()) {
            MessageDialogFragment.newInstance(
                titleId = R.string.keys_missing,
                descriptionId = R.string.keys_missing_description,
                helpLinkId = R.string.keys_missing_help
            ).show(supportFragmentManager, MessageDialogFragment.TAG)
        }
    }
    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        outState.putBoolean(CHECKED_DECRYPTION, checkedDecryption)
    }

    fun finishSetup(navController: NavController) {
        navController.navigate(R.id.action_firstTimeSetupFragment_to_gamesFragment)
    }

    private fun setUpNavigation(navController: NavController) {
        val firstTimeSetup = PreferenceManager.getDefaultSharedPreferences(applicationContext)
            .getBoolean(Settings.PREF_FIRST_APP_LAUNCH, true)

        if (firstTimeSetup && !homeViewModel.navigatedToSetup) {
            navController.navigate(R.id.firstTimeSetupFragment)
            homeViewModel.navigatedToSetup = true
        }
    }

    private fun showStatusBarShade(visible: Boolean) {
        if (FullscreenHelper.isFullscreenEnabled(applicationContext)) {
            binding.statusBarShade.visibility = View.GONE
            return
        }
        binding.statusBarShade.animate().apply {
            if (visible) {
                binding.statusBarShade.setVisible(true)
                binding.statusBarShade.translationY = binding.statusBarShade.height.toFloat() * -2
                duration = 300
                translationY(0f)
                interpolator = PathInterpolator(0.05f, 0.7f, 0.1f, 1f)
            } else {
                duration = 300
                translationY(binding.statusBarShade.height.toFloat() * -2)
                interpolator = PathInterpolator(0.3f, 0f, 0.8f, 0.15f)
            }
        }.withEndAction {
            if (!visible) {
                binding.statusBarShade.setVisible(visible = false, gone = false)
            }
        }.start()
    }

    override fun onResume() {
        ThemeHelper.setCorrectTheme(this)
        super.onResume()
        applyFullscreenPreference()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            if (android.os.Environment.isExternalStorageManager()) {
                DirectoryInitialization.initializeSharedStorage()
            }
        }
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) {
            applyFullscreenPreference()
        }
    }

    private fun setInsets() = ViewCompat.setOnApplyWindowInsetsListener(
        binding.root
    ) { _: View, windowInsets: WindowInsetsCompat ->
        val isFullscreen = FullscreenHelper.isFullscreenEnabled(applicationContext)
        if (isFullscreen) {
            binding.statusBarShade.visibility = View.GONE
            binding.navigationBarShade.visibility = View.GONE
        } else {
            val insets = windowInsets.getInsets(WindowInsetsCompat.Type.systemBars())
            val mlpStatusShade = binding.statusBarShade.layoutParams as MarginLayoutParams
            mlpStatusShade.height = insets.top
            binding.statusBarShade.layoutParams = mlpStatusShade

            val mlpNavShade = binding.navigationBarShade.layoutParams as MarginLayoutParams
            mlpNavShade.height = insets.bottom
            binding.navigationBarShade.layoutParams = mlpNavShade
        }

        windowInsets
    }

    private fun applyFullscreenPreference() {
        val isFullscreen = FullscreenHelper.isFullscreenEnabled(this)
        FullscreenHelper.applyToActivity(this)
        if (isFullscreen) {
            binding.statusBarShade.visibility = View.GONE
            binding.navigationBarShade.visibility = View.GONE
        }
    }

    override fun setTheme(resId: Int) {
        super.setTheme(resId)
        themeId = resId
    }

    override fun onDestroy() {
        EmulationActivity.stopForegroundService(this)
        super.onDestroy()
    }

    val getGamesDirectory =
        registerForActivityResult(ActivityResultContracts.OpenDocumentTree()) { result ->
            if (result != null) {
                processGamesDir(result)
            }
        }

    val getExternalContentDirectory =
        registerForActivityResult(ActivityResultContracts.OpenDocumentTree()) { result ->
            if (result != null) {
                processExternalContentDir(result)
            }
        }

    fun processGamesDir(result: Uri, calledFromGameFragment: Boolean = false) {
        try {
            contentResolver.takePersistableUriPermission(
                result,
                Intent.FLAG_GRANT_READ_URI_PERMISSION
            )
        } catch (e: Throwable) {
            android.util.Log.e("STORM_SWITCH", "takePersistableUriPermission error: ${e.message}")
        }

        val uriString = result.toString()
        val folder = gamesViewModel.folders.value.firstOrNull { it.uriString == uriString }
        if (folder != null) {
            Toast.makeText(
                applicationContext,
                R.string.folder_already_added,
                Toast.LENGTH_SHORT
            ).show()
            return
        }

        AddGameFolderDialogFragment.newInstance(uriString, calledFromGameFragment)
            .show(supportFragmentManager, AddGameFolderDialogFragment.TAG)
    }

    fun processExternalContentDir(result: Uri) {
        try {
            contentResolver.takePersistableUriPermission(
                result,
                Intent.FLAG_GRANT_READ_URI_PERMISSION
            )
        } catch (e: Throwable) {
            android.util.Log.e("STORM_SWITCH", "processExternalContentDir takePermission error: ${e.message}")
        }

        val uriString = result.toString()
        val folder = gamesViewModel.folders.value.firstOrNull {
            it.uriString == uriString && it.type == org.yuzu.yuzu_emu.model.DirectoryType.EXTERNAL_CONTENT
        }
        if (folder != null) {
            Toast.makeText(
                applicationContext,
                R.string.folder_already_added,
                Toast.LENGTH_SHORT
            ).show()
            return
        }

        val externalContentDir = org.yuzu.yuzu_emu.model.GameDir(uriString, false, org.yuzu.yuzu_emu.model.DirectoryType.EXTERNAL_CONTENT)
        gamesViewModel.addFolder(externalContentDir, savedFromGameFragment = false)
    }

    val getProdKey = registerForActivityResult(ActivityResultContracts.OpenDocument()) { result ->
        if (result != null) {
            processKey(result, "keys")
        }
    }

    val getAmiiboKey = registerForActivityResult(ActivityResultContracts.OpenDocument()) { result ->
        if (result != null) {
            processKey(result, "bin")
        }
    }

    fun processKey(result: Uri, extension: String = "keys") {
        InstallableActions.processKey(
            activity = this,
            fragmentManager = supportFragmentManager,
            gamesViewModel = gamesViewModel,
            result = result,
            extension = extension
        )
    }

    fun processKeysFolder(result: Uri) {
        InstallableActions.processKeysFolder(
            activity = this,
            fragmentManager = supportFragmentManager,
            gamesViewModel = gamesViewModel,
            treeUri = result
        )
    }

    val getFirmware = registerForActivityResult(ActivityResultContracts.OpenDocument()) { result ->
        if (result != null) {
            processFirmware(result)
        }
    }

    fun processFirmware(result: Uri, onComplete: (() -> Unit)? = null) {
        InstallableActions.processFirmware(
            activity = this,
            fragmentManager = supportFragmentManager,
            homeViewModel = homeViewModel,
            result = result,
            onComplete = onComplete
        )
    }

    fun processFirmwareFolder(result: Uri, onComplete: (() -> Unit)? = null) {
        InstallableActions.processFirmwareFolder(
            activity = this,
            fragmentManager = supportFragmentManager,
            homeViewModel = homeViewModel,
            treeUri = result,
            onComplete = onComplete
        )
    }

    fun uninstallFirmware() {
        InstallableActions.uninstallFirmware(
            activity = this,
            fragmentManager = supportFragmentManager,
            homeViewModel = homeViewModel
        )
    }

    private fun installContent(documents: List<Uri>) {
        InstallableActions.installContent(
            activity = this,
            fragmentManager = supportFragmentManager,
            addonViewModel = addonViewModel,
            documents = documents
        )
    }

    val exportUserData = registerForActivityResult(
        ActivityResultContracts.CreateDocument("application/zip")
    ) { result ->
        if (result == null) {
            return@registerForActivityResult
        }
        InstallableActions.exportUserData(
            activity = this,
            fragmentManager = supportFragmentManager,
            result = result
        )
    }

    val importUserData =
        registerForActivityResult(ActivityResultContracts.OpenDocument()) { result ->
            if (result == null) {
                return@registerForActivityResult
            }
            InstallableActions.importUserData(
                activity = this,
                fragmentManager = supportFragmentManager,
                gamesViewModel = gamesViewModel,
                driverViewModel = driverViewModel,
                result = result
            )
        }
}

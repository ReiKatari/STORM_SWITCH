// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

package org.yuzu.yuzu_emu.features.settings.ui

import android.content.Context
import org.yuzu.yuzu_emu.YuzuApplication
import android.os.Bundle
import android.view.View
import android.view.ViewGroup.MarginLayoutParams
import android.widget.Toast
import androidx.activity.OnBackPressedCallback
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.navigation.fragment.NavHostFragment
import androidx.navigation.navArgs
import com.google.android.material.color.MaterialColors
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import org.yuzu.yuzu_emu.NativeLibrary
import java.io.IOException
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.databinding.ActivitySettingsBinding
import org.yuzu.yuzu_emu.features.input.NativeInput
import org.yuzu.yuzu_emu.features.settings.utils.SettingsFile
import org.yuzu.yuzu_emu.activities.EmulationActivity
import org.yuzu.yuzu_emu.fragments.ResetSettingsDialogFragment
import org.yuzu.yuzu_emu.model.GameFixDatabase
import org.yuzu.yuzu_emu.utils.*
import org.yuzu.yuzu_emu.utils.collect

class SettingsActivity : AppCompatActivity() {
    private lateinit var binding: ActivitySettingsBinding

    private val args by navArgs<SettingsActivityArgs>()

    private val settingsViewModel: SettingsViewModel by viewModels()

    override fun attachBaseContext(base: Context) {
        super.attachBaseContext(YuzuApplication.applyLanguage(base))
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        ThemeHelper.ThemeChangeListener(this)
        ThemeHelper.setTheme(this)

        super.onCreate(savedInstanceState)

        binding = ActivitySettingsBinding.inflate(layoutInflater)
        setContentView(binding.root)

        if (args.game == null) {
            NativeConfig.unloadPerGameConfig()
            NativeConfig.reloadGlobalConfig()
        } else {
            NativeConfig.unloadPerGameConfig()
            SettingsFile.loadCustomConfig(args.game!!)
        }
        settingsViewModel.game = args.game

        val navHostFragment =
            supportFragmentManager.findFragmentById(R.id.fragment_container) as NavHostFragment
        navHostFragment.navController.setGraph(R.navigation.settings_navigation, intent.extras)

        WindowCompat.setDecorFitsSystemWindows(window, false)
        ThemeHelper.applySystemBarsTheme(window, this)

        if (InsetsHelper.getSystemGestureType(applicationContext) !=
            InsetsHelper.GESTURE_NAVIGATION
        ) {
            binding.navigationBarShade.setBackgroundColor(
                ThemeHelper.getColorWithOpacity(
                    MaterialColors.getColor(
                        binding.navigationBarShade,
                        com.google.android.material.R.attr.colorSurface
                    ),
                    ThemeHelper.SYSTEM_BAR_ALPHA
                )
            )
        }

        settingsViewModel.shouldRecreate.collect(
            this,
            resetState = { settingsViewModel.setShouldRecreate(false) }
        ) { if (it) recreate() }
        settingsViewModel.shouldNavigateBack.collect(
            this,
            resetState = { settingsViewModel.setShouldNavigateBack(false) }
        ) { if (it) navigateBack() }
        settingsViewModel.shouldShowResetSettingsDialog.collect(
            this,
            resetState = { settingsViewModel.setShouldShowResetSettingsDialog(false) }
        ) {
            if (it) {
                ResetSettingsDialogFragment().show(
                    supportFragmentManager,
                    ResetSettingsDialogFragment.TAG
                )
            }
        }
        settingsViewModel.shouldShowApplyGlobalToAllDialog.collect(
            this,
            resetState = { settingsViewModel.setShouldShowApplyGlobalToAllDialog(false) }
        ) {
            if (it) {
                MaterialAlertDialogBuilder(this, R.style.EdenMaterialDialog)
                    .setTitle(R.string.apply_global_to_all_games_confirm_title)
                    .setMessage(R.string.apply_global_to_all_games_confirm_message)
                    .setPositiveButton(R.string.apply_global_to_all_games_positive) { _, _ ->
                        NativeConfig.saveGlobalConfig()
                        NativeLibrary.applySettings()
                        val resetCount = GameFixDatabase.resetAllPerGameConfigs()
                        Toast.makeText(
                            this,
                            getString(R.string.apply_global_to_all_games_success),
                            Toast.LENGTH_LONG
                        ).show()
                        Log.info("[SettingsActivity] Applied global settings to all games. Reset configs count: $resetCount")
                    }
                    .setNegativeButton(android.R.string.cancel, null)
                    .show()
            }
        }

        onBackPressedDispatcher.addCallback(
            this,
            object : OnBackPressedCallback(true) {
                override fun handleOnBackPressed() = navigateBack()
            }
        )

        setInsets()
        applyFullscreenPreference()
    }

    private fun saveSettings() {
        try {
            if (args.game == null) {
                NativeConfig.saveGlobalConfig()
                NativeLibrary.applySettings()
                Log.info("[SettingsActivity] Saved global settings to INI")
            } else if (NativeConfig.isPerGameConfigLoaded()) {
                NativeConfig.savePerGameConfig()
                args.game?.let { GameFixDatabase.markConfigAsUserCustom(it) }
                NativeLibrary.logSettings()
                NativeLibrary.applySettings()
                Log.info("[SettingsActivity] Saved per-game settings to INI for ${args.game?.title}")
            }
        } catch (e: Exception) {
            Log.error("[SettingsActivity] Failed to save settings: ${e.message}")
        }
    }

    fun navigateBack() {
        saveSettings()
        val navHostFragment =
            supportFragmentManager.findFragmentById(R.id.fragment_container) as NavHostFragment
        if (navHostFragment.childFragmentManager.backStackEntryCount > 0) {
            navHostFragment.navController.popBackStack()
        } else {
            finish()
        }
    }

    override fun onStart() {
        super.onStart()
        if (!DirectoryInitialization.areDirectoriesReady) {
            DirectoryInitialization.start()
        }
    }

    override fun onResume() {
        super.onResume()
        ThemeHelper.applySystemBarsTheme(window, this)
        applyFullscreenPreference()
    }

    override fun onPause() {
        saveSettings()
        super.onPause()
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) {
            applyFullscreenPreference()
        }
    }

    override fun onStop() {
        saveSettings()
        super.onStop()
        Log.info("[SettingsActivity] Settings activity stopping. Saving settings to INI...")
        if (isFinishing) {
            NativeInput.reloadInputDevices()
            if (args.game == null) {
                NativeConfig.saveGlobalConfig()
                NativeLibrary.applySettings()
            } else if (NativeConfig.isPerGameConfigLoaded()) {
                NativeConfig.savePerGameConfig()
                args.game?.let { GameFixDatabase.markConfigAsUserCustom(it) }
                NativeLibrary.logSettings()
                NativeLibrary.applySettings()

                if (!EmulationActivity.isEmulationRunning) {
                    NativeConfig.unloadPerGameConfig()
                } else {
                    Log.info("[SettingsActivity] Preserving loaded per-game config because emulation session is active")
                }
            }

            if (settingsViewModel.shouldRecreateForLanguageChange.value) {
                settingsViewModel.setShouldRecreateForLanguageChange(false)
                val relaunchIntent = packageManager?.getLaunchIntentForPackage(packageName)
                if (relaunchIntent != null) {
                    relaunchIntent.addFlags(android.content.Intent.FLAG_ACTIVITY_CLEAR_TASK or android.content.Intent.FLAG_ACTIVITY_NEW_TASK)
                    startActivity(relaunchIntent)
                    android.os.Process.killProcess(android.os.Process.myPid())
                }
            }
        }
    }

    override fun onDestroy() {
        saveSettings()
        if (args.game != null && !EmulationActivity.isEmulationRunning) {
            NativeConfig.unloadPerGameConfig()
            NativeConfig.reloadGlobalConfig()
        }
        super.onDestroy()
    }

    fun onSettingsReset() {
        // Delete settings file because the user may have changed values that do not exist in the UI
        if (args.game == null) {
            NativeConfig.unloadGlobalConfig()
            val settingsFile = SettingsFile.getSettingsFile(SettingsFile.FILE_NAME_CONFIG)
            if (!settingsFile.delete()) {
                throw IOException("Failed to delete $settingsFile")
            }
            NativeConfig.initializeGlobalConfig()
            try {
                org.yuzu.yuzu_emu.utils.StormHardwareCalibrator.autoCalibrate(applicationContext, force = true)
            } catch (_: Exception) {}
        } else {
            NativeConfig.unloadPerGameConfig()
            val settingsFile = SettingsFile.getCustomSettingsFile(args.game!!)
            if (settingsFile.exists() && !settingsFile.delete()) {
                throw IOException("Failed to delete $settingsFile")
            }
        }

        Toast.makeText(
            applicationContext,
            getString(R.string.settings_reset),
            Toast.LENGTH_LONG
        ).show()
        finish()
    }

    private fun setInsets() {
        ViewCompat.setOnApplyWindowInsetsListener(
            binding.navigationBarShade
        ) { _: View, windowInsets: WindowInsetsCompat ->
            val barInsets = windowInsets.getInsets(WindowInsetsCompat.Type.systemBars())

            // The only situation where we care to have a nav bar shade is when it's at the bottom
            // of the screen where scrolling list elements can go behind it.
            val mlpNavShade = binding.navigationBarShade.layoutParams as MarginLayoutParams
            mlpNavShade.height = barInsets.bottom
            binding.navigationBarShade.layoutParams = mlpNavShade

            windowInsets
        }
    }

    private fun applyFullscreenPreference() {
        FullscreenHelper.applyToActivity(this)
    }
}

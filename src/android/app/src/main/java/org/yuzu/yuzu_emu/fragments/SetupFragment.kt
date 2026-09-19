// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.Manifest
import android.content.Intent
import android.os.Build
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.activity.OnBackPressedCallback
import androidx.activity.result.contract.ActivityResultContracts
import androidx.annotation.RequiresApi
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.NotificationManagerCompat
import androidx.core.content.ContextCompat
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.isVisible
import androidx.core.view.updatePadding
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import androidx.navigation.findNavController
import androidx.preference.PreferenceManager
import androidx.viewpager2.widget.ViewPager2.OnPageChangeCallback
import com.google.android.material.transition.MaterialFadeThrough
import org.yuzu.yuzu_emu.NativeLibrary
import java.io.File
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.YuzuApplication
import org.yuzu.yuzu_emu.adapters.SetupAdapter
import org.yuzu.yuzu_emu.databinding.FragmentSetupBinding
import org.yuzu.yuzu_emu.features.settings.model.Settings
import org.yuzu.yuzu_emu.model.ButtonState
import org.yuzu.yuzu_emu.model.GamesViewModel
import org.yuzu.yuzu_emu.model.HomeViewModel
import org.yuzu.yuzu_emu.model.PageButton
import org.yuzu.yuzu_emu.model.SetupCallback
import org.yuzu.yuzu_emu.model.SetupPage
import org.yuzu.yuzu_emu.model.PageState
import org.yuzu.yuzu_emu.ui.main.MainActivity
import org.yuzu.yuzu_emu.utils.DirectoryInitialization
import org.yuzu.yuzu_emu.utils.LosslessScalingHelper
import org.yuzu.yuzu_emu.utils.NativeConfig
import org.yuzu.yuzu_emu.utils.ThemeHelper
import org.yuzu.yuzu_emu.utils.ViewUtils
import org.yuzu.yuzu_emu.utils.ViewUtils.setVisible
import org.yuzu.yuzu_emu.utils.collect

class SetupFragment : Fragment() {
    private var _binding: FragmentSetupBinding? = null
    private val binding get() = _binding!!

    private val homeViewModel: HomeViewModel by activityViewModels()
    private val gamesViewModel: GamesViewModel by activityViewModels()

    private lateinit var mainActivity: MainActivity

    private lateinit var hasBeenWarned: BooleanArray

    private lateinit var pages: MutableList<SetupPage>

    private lateinit var pageButtonCallback: SetupCallback

    companion object {
        const val KEY_NEXT_VISIBILITY = "NextButtonVisibility"
        const val KEY_BACK_VISIBILITY = "BackButtonVisibility"
        const val KEY_HAS_BEEN_WARNED = "HasBeenWarned"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        exitTransition = MaterialFadeThrough()
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentSetupBinding.inflate(layoutInflater)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        mainActivity = requireActivity() as MainActivity

        requireActivity().onBackPressedDispatcher.addCallback(
            viewLifecycleOwner,
            object : OnBackPressedCallback(true) {
                override fun handleOnBackPressed() {
                    if (binding.viewPager2.currentItem > 0) {
                        pageBackward()
                    } else {
                        requireActivity().finish()
                    }
                }
            }
        )

        requireActivity().window.navigationBarColor =
            ContextCompat.getColor(requireContext(), android.R.color.transparent)

        pages = mutableListOf<SetupPage>()
        pages.apply {
            add(
                SetupPage(
                    R.drawable.ic_permission,
                    R.string.permissions,
                    R.string.permissions_description,
                    mutableListOf<PageButton>().apply {
                        add(
                            PageButton(
                                R.drawable.ic_folder_open,
                                R.string.storage_permission,
                                R.string.storage_permission_description,
                                {
                                    pageButtonCallback = it
                                    requestStoragePermission()
                                },
                                {
                                    if (isStoragePermissionGranted()) {
                                        ButtonState.BUTTON_ACTION_COMPLETE
                                    } else {
                                        ButtonState.BUTTON_ACTION_INCOMPLETE
                                    }
                                },
                                false,
                                false,
                            )
                        )
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                            add(
                                PageButton(
                                    R.drawable.ic_notification,
                                    R.string.notifications,
                                    R.string.notifications_description,
                                    {
                                        pageButtonCallback = it
                                        permissionLauncher.launch(
                                            Manifest.permission.POST_NOTIFICATIONS
                                        )
                                    },
                                    {
                                        if (NotificationManagerCompat.from(requireContext())
                                                .areNotificationsEnabled()
                                        ) {
                                            ButtonState.BUTTON_ACTION_COMPLETE
                                        } else {
                                            ButtonState.BUTTON_ACTION_INCOMPLETE
                                        }
                                    },
                                    false,
                                    false,
                                )
                            )
                        }
                    },
                    {
                        val storageOk = isStoragePermissionGranted()
                        val notifOk = Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU ||
                            NotificationManagerCompat.from(requireContext()).areNotificationsEnabled()
                        if (storageOk && notifOk) {
                            PageState.COMPLETE
                        } else {
                            PageState.INCOMPLETE
                        }
                    }
                )
            )
            add(
                SetupPage(
                    R.drawable.ic_folder_open,
                    R.string.emulator_data,
                    R.string.emulator_data_description,
                    mutableListOf<PageButton>().apply {
                        add(
                            PageButton(
                                R.drawable.ic_key,
                                R.string.keys,
                                R.string.keys_description,
                                {
                                    pageButtonCallback = it
                                    showKeysDialog()
                                },
                                {
                                    if (areKeysInstalled()) {
                                        ButtonState.BUTTON_ACTION_COMPLETE
                                    } else {
                                        ButtonState.BUTTON_ACTION_INCOMPLETE
                                    }
                                },
                                false,
                                false,
                                R.string.install_prod_keys_warning,
                                R.string.install_prod_keys_warning_description,
                                R.string.install_prod_keys_warning_help,
                            )
                        )
                        add(
                            PageButton(
                                R.drawable.ic_firmware,
                                R.string.firmware,
                                R.string.firmware_description,
                                {
                                    pageButtonCallback = it
                                    showFirmwareDialog()
                                },
                                {
                                    if (isFirmwareInstalled()) {
                                        ButtonState.BUTTON_ACTION_COMPLETE
                                    } else {
                                        ButtonState.BUTTON_ACTION_INCOMPLETE
                                    }
                                },
                                false,
                                false,
                                R.string.install_firmware_warning,
                                R.string.install_firmware_warning_description,
                                R.string.install_firmware_warning_help,
                            )
                        )
                        add(
                            PageButton(
                                R.drawable.ic_duck,
                                R.string.lossless_scaling,
                                R.string.lossless_scaling_setup_description,
                                {
                                    pageButtonCallback = it
                                    getLosslessDll.launch(arrayOf("*/*"))
                                },
                                {
                                    if (isFrameGenInstalled()) {
                                        ButtonState.BUTTON_ACTION_COMPLETE
                                    } else {
                                        ButtonState.BUTTON_ACTION_INCOMPLETE
                                    }
                                }
                            )
                        )
                        add(
                            PageButton(
                                R.drawable.ic_controller,
                                R.string.games,
                                R.string.games_description,
                                {
                                    pageButtonCallback = it
                                    getGamesDirectory.launch(Intent(Intent.ACTION_OPEN_DOCUMENT_TREE).data)
                                },
                                {
                                    if (areGamesConfigured()) {
                                        ButtonState.BUTTON_ACTION_COMPLETE
                                    } else {
                                        ButtonState.BUTTON_ACTION_INCOMPLETE
                                    }
                                },
                                false,
                                false,
                                R.string.add_games_warning,
                                R.string.add_games_warning_description,
                                R.string.add_games_warning_help,
                            )
                        )
                    },
                    { PageState.INCOMPLETE }
                )
            )
            add(
                SetupPage(
                    R.drawable.ic_check,
                    R.string.done,
                    R.string.done_description,
                    mutableListOf<PageButton>().apply {
                        add(
                            PageButton(
                                R.drawable.ic_arrow_forward,
                                R.string.get_started,
                                0,
                                buttonAction = {
                                    finishSetup()
                                },
                                buttonState = {
                                    ButtonState.BUTTON_ACTION_UNDEFINED
                                },
                            )
                        )
                    }
                ) { PageState.UNDEFINED }
            )
        }

        homeViewModel.shouldPageForward.collect(
            viewLifecycleOwner,
            resetState = { homeViewModel.setShouldPageForward(false) }
        ) { if (it) pageForward() }
        homeViewModel.gamesDirSelected.collect(
            viewLifecycleOwner,
            resetState = { homeViewModel.setGamesDirSelected(false) }
        ) { if (it) checkForButtonState.invoke() }
        homeViewModel.checkKeys.collect(
            viewLifecycleOwner,
            resetState = { homeViewModel.setCheckKeys(false) }
        ) { if (it) checkForButtonState.invoke() }

        binding.viewPager2.apply {
            adapter = SetupAdapter(requireActivity() as AppCompatActivity, pages)
            offscreenPageLimit = 2
            isUserInputEnabled = false
        }

        binding.viewPager2.registerOnPageChangeCallback(object : OnPageChangeCallback() {
            var previousPosition: Int = 0

            override fun onPageSelected(position: Int) {
                super.onPageSelected(position)

                val isFirstPage = position == 0
                val isLastPage = position == pages.size - 1

                if (isFirstPage) {
                    ViewUtils.hideView(binding.buttonBack)
                } else {
                    ViewUtils.showView(binding.buttonBack)
                }

                if (isLastPage) {
                    ViewUtils.hideView(binding.buttonNext)
                } else {
                    ViewUtils.showView(binding.buttonNext)
                }

                previousPosition = position
            }
        })

        binding.buttonNext.setOnClickListener {
            val index = binding.viewPager2.currentItem
            if (index == 0 && !isStoragePermissionGranted()) {
                com.google.android.material.snackbar.Snackbar.make(
                    binding.root,
                    R.string.storage_permission_required_notice,
                    com.google.android.material.snackbar.Snackbar.LENGTH_LONG
                ).show()
                requestStoragePermission()
                return@setOnClickListener
            }
            val currentPage = pages[index]

            val warningMessages =
                mutableListOf<Triple<Int, Int, Int>>() // title, description, helpLink

            currentPage.pageButtons?.forEach { button ->
                if (button.hasWarning) {
                    val buttonState = button.buttonState()
                    if (buttonState == ButtonState.BUTTON_ACTION_COMPLETE) {
                        return@forEach
                    }

                    if (!hasBeenWarned[index]) {
                        warningMessages.add(
                            Triple(
                                button.warningTitleId,
                                button.warningDescriptionId,
                                button.warningHelpLinkId
                            )
                        )
                    }
                }
            }

            if (warningMessages.isNotEmpty()) {
                SetupWarningDialogFragment.newInstance(
                    warningMessages.map { it.first }.toIntArray(),
                    warningMessages.map { it.second }.toIntArray(),
                    warningMessages.map { it.third }.toIntArray(),
                    index
                ).show(childFragmentManager, SetupWarningDialogFragment.TAG)
                return@setOnClickListener
            }
            pageForward()
        }
        binding.buttonBack.setOnClickListener { pageBackward() }


        if (savedInstanceState != null) {
            val nextIsVisible = savedInstanceState.getBoolean(KEY_NEXT_VISIBILITY)
            val backIsVisible = savedInstanceState.getBoolean(KEY_BACK_VISIBILITY)
            hasBeenWarned = savedInstanceState.getBooleanArray(KEY_HAS_BEEN_WARNED)!!

            if (nextIsVisible) {
                binding.buttonNext.visibility = View.VISIBLE
            }
            if (backIsVisible) {
                binding.buttonBack.visibility = View.VISIBLE
            }
        } else {
            hasBeenWarned = BooleanArray(pages.size)
        }

        setInsets()
    }

    override fun onResume() {
        super.onResume()
        refreshAllButtonStates()
    }

    override fun onStop() {
        super.onStop()
        NativeConfig.saveGlobalConfig()
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        outState.putBoolean(KEY_NEXT_VISIBILITY, binding.buttonNext.isVisible)
        outState.putBoolean(KEY_BACK_VISIBILITY, binding.buttonBack.isVisible)
        outState.putBooleanArray(KEY_HAS_BEEN_WARNED, hasBeenWarned)
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }

    val checkForButtonState: () -> Unit = {
        refreshAllButtonStates()
    }

    fun refreshAllButtonStates() {
        if (_binding != null) {
            (binding.viewPager2.adapter as? SetupAdapter)?.refreshAllButtonStates()
        }
    }

    @RequiresApi(Build.VERSION_CODES.TIRAMISU)
    private val permissionLauncher =
        registerForActivityResult(ActivityResultContracts.RequestPermission()) {
            refreshAllButtonStates()

            if (!it &&
                !shouldShowRequestPermissionRationale(Manifest.permission.POST_NOTIFICATIONS)
            ) {
                PermissionDeniedDialogFragment().show(
                    childFragmentManager,
                    PermissionDeniedDialogFragment.TAG
                )
            }
        }

    val getProdKey =
        registerForActivityResult(ActivityResultContracts.OpenDocument()) { result ->
            if (result != null) {
                mainActivity.processKey(result, "keys")
                refreshAllButtonStates()
            }
        }

    val getFirmware =
        registerForActivityResult(ActivityResultContracts.OpenDocument()) { result ->
            if (result != null) {
                mainActivity.processFirmware(result) {
                    refreshAllButtonStates()
                }
            }
        }

    val getLosslessDll =
        registerForActivityResult(ActivityResultContracts.OpenDocument()) { result ->
            if (result == null) {
                return@registerForActivityResult
            }

            val resultStrings = resources.getStringArray(R.array.losslessDllResults)
            ProgressDialogFragment.newInstance(
                requireActivity(),
                R.string.lossless_scaling_installing,
                false
            ) { _, _ ->
                val installResult = LosslessScalingHelper.install(result)
                if (installResult == LosslessScalingHelper.RESULT_OK) {
                    getString(R.string.lossless_scaling_install_success)
                } else {
                    val errorDesc = resultStrings.getOrElse(installResult) { resultStrings[1] }
                    MessageDialogFragment.newInstance(
                        titleId = R.string.lossless_scaling_install_failed,
                        descriptionString = errorDesc
                    )
                }
            }.apply {
                onDialogComplete = { refreshAllButtonStates() }
            }.show(parentFragmentManager, ProgressDialogFragment.TAG)
        }

    val getGamesDirectory =
        registerForActivityResult(ActivityResultContracts.OpenDocumentTree()) { result ->
            if (result != null) {
                try {
                    requireContext().contentResolver.takePersistableUriPermission(
                        result,
                        Intent.FLAG_GRANT_READ_URI_PERMISSION
                    )
                } catch (_: Exception) {}
                mainActivity.processGamesDir(result)
                refreshAllButtonStates()
            }
        }

    private fun isStoragePermissionGranted(): Boolean {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            android.os.Environment.isExternalStorageManager()
        } else {
            ContextCompat.checkSelfPermission(requireContext(), Manifest.permission.WRITE_EXTERNAL_STORAGE) == android.content.pm.PackageManager.PERMISSION_GRANTED
        }
    }

    private fun requestStoragePermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            try {
                val intent = Intent(android.provider.Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION).apply {
                    data = android.net.Uri.fromParts("package", requireContext().packageName, null)
                }
                storagePermissionLauncher.launch(intent)
            } catch (_: Exception) {
                try {
                    val intent = Intent(android.provider.Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION)
                    storagePermissionLauncher.launch(intent)
                } catch (_: Exception) {}
            }
        } else {
            legacyStorageLauncher.launch(arrayOf(Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE))
        }
    }

    private val storagePermissionLauncher = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) {
        if (isStoragePermissionGranted()) {
            DirectoryInitialization.initializeSharedStorage()
            checkForButtonState.invoke()
        }
    }

    private val legacyStorageLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { permissions ->
        if (permissions[Manifest.permission.WRITE_EXTERNAL_STORAGE] == true || permissions[Manifest.permission.READ_EXTERNAL_STORAGE] == true) {
            DirectoryInitialization.initializeSharedStorage()
            checkForButtonState.invoke()
        }
    }

    val getKeysFolder =
        registerForActivityResult(ActivityResultContracts.OpenDocumentTree()) { result ->
            if (result != null) {
                mainActivity.processKeysFolder(result)
                refreshAllButtonStates()
            }
        }

    val getFirmwareFolder =
        registerForActivityResult(ActivityResultContracts.OpenDocumentTree()) { result ->
            if (result != null) {
                mainActivity.processFirmwareFolder(result) {
                    refreshAllButtonStates()
                }
            }
        }

    private fun areKeysInstalled(): Boolean {
        val candidates = listOfNotNull(
            DirectoryInitialization.userDirectory?.let { File(it, "keys/prod.keys") },
            File(android.os.Environment.getExternalStorageDirectory(), "STORM SWITCH/keys/prod.keys"),
            context?.getExternalFilesDir(null)?.let { File(it, "keys/prod.keys") },
            context?.filesDir?.let { File(it, "keys/prod.keys") }
        )
        val fileExists = candidates.any { it.exists() && it.length() > 0 }
        if (fileExists) {
            try {
                NativeLibrary.reloadKeys()
            } catch (_: Throwable) {}
        }
        return fileExists || NativeLibrary.areKeysPresent()
    }

    private fun isFirmwareInstalled(): Boolean {
        val nandDir = try { NativeConfig.getNandDir() } catch (_: Throwable) { "" }
        val candidates = listOfNotNull(
            context?.filesDir?.let { File(it, "nand/system/Contents/registered") },
            context?.getExternalFilesDir(null)?.let { File(it, "nand/system/Contents/registered") },
            DirectoryInitialization.userDirectory?.let { File(it, "nand/system/Contents/registered") },
            if (nandDir.isNotBlank()) File(nandDir, "system/Contents/registered") else null,
            File(android.os.Environment.getExternalStorageDirectory(), "STORM SWITCH/nand/system/Contents/registered")
        ).distinctBy { it.canonicalPath }
        val filesExist = candidates.any { dir ->
            dir.exists() && dir.isDirectory && (dir.listFiles { f -> f.extension.equals("nca", ignoreCase = true) }?.isNotEmpty() == true)
        }
        if (filesExist) {
            try {
                NativeLibrary.initializeSystem(true)
            } catch (_: Throwable) {}
        }
        return filesExist || NativeLibrary.isFirmwareAvailable()
    }

    private fun isFrameGenInstalled(): Boolean {
        return LosslessScalingHelper.isInstalled()
    }

    private fun areGamesConfigured(): Boolean {
        return hasValidGameDirectories()
    }

    private fun hasValidGameDirectories(): Boolean {
        val dirs = NativeConfig.getGameDirs().filter { it.uriString.isNotBlank() }
        if (dirs.isEmpty()) return false
        val persisted = try { requireContext().contentResolver.persistedUriPermissions } catch (_: Exception) { emptyList() }
        return dirs.any { d ->
            val uri = android.net.Uri.parse(d.uriString)
            if (uri.scheme == "file" || uri.scheme.isNullOrEmpty()) {
                File(d.uriString).exists()
            } else {
                persisted.any { p -> p.uri == uri && p.isReadPermission }
            }
        }
    }

    private fun finishSetup() {
        if (!isStoragePermissionGranted()) {
            binding.viewPager2.currentItem = 0
            com.google.android.material.snackbar.Snackbar.make(
                binding.root,
                R.string.storage_permission_required_notice,
                com.google.android.material.snackbar.Snackbar.LENGTH_LONG
            ).show()
            requestStoragePermission()
            return
        }

        PreferenceManager.getDefaultSharedPreferences(YuzuApplication.appContext)
            .edit()
            .putBoolean(Settings.PREF_FIRST_APP_LAUNCH, false)
            .apply()

        gamesViewModel.reloadGames(directoriesChanged = true, firstStartup = false)

        mainActivity.finishSetup(binding.root.findNavController())
    }

    fun pageForward() {
        if (_binding != null) {
            binding.viewPager2.currentItem += 1
        }
    }

    fun pageBackward() {
        if (_binding != null) {
            binding.viewPager2.currentItem -= 1
        }
    }

    fun setPageWarned(page: Int) {
        hasBeenWarned[page] = true
    }

    private fun setInsets() =
        ViewCompat.setOnApplyWindowInsetsListener(
            binding.root
        ) { _: View, windowInsets: WindowInsetsCompat ->
            val barInsets =
                windowInsets.getInsets(WindowInsetsCompat.Type.systemBars())
            val cutoutInsets =
                windowInsets.getInsets(WindowInsetsCompat.Type.displayCutout())

            val leftPadding = barInsets.left + cutoutInsets.left
            val topPadding = barInsets.top + cutoutInsets.top
            val rightPadding = barInsets.right + cutoutInsets.right
            val bottomPadding = barInsets.bottom + cutoutInsets.bottom

            if (resources.getBoolean(R.bool.small_layout)) {
                binding.viewPager2
                    .updatePadding(
                        left = leftPadding,
                        top = topPadding,
                        right = rightPadding
                    )
                binding.constraintButtons
                    .updatePadding(
                        left = leftPadding,
                        right = rightPadding,
                        bottom = bottomPadding
                    )
            } else {
                binding.viewPager2.updatePadding(
                    top = topPadding,
                    bottom = bottomPadding
                )
                binding.constraintButtons
                    .updatePadding(
                        left = leftPadding,
                        right = rightPadding,
                        bottom = bottomPadding
                    )
            }
            windowInsets
        }

    private fun showKeysDialog() {
        val context = androidx.appcompat.view.ContextThemeWrapper(
            requireContext(),
            ThemeHelper.getSelectedStaticThemeColor()
        )
        val options = arrayOf(
            getString(R.string.online_install_recommended),
            getString(R.string.select_keys_file),
            getString(R.string.select_keys_folder)
        )
        val icons = intArrayOf(
            R.drawable.ic_website,
            R.drawable.ic_key,
            R.drawable.ic_folder_open
        )

        val linearLayout = android.widget.LinearLayout(context).apply {
            orientation = android.widget.LinearLayout.VERTICAL
            val padH = (20 * resources.displayMetrics.density).toInt()
            val padV = (12 * resources.displayMetrics.density).toInt()
            setPadding(padH, padV, padH, padV)
        }

        val dialog = com.google.android.material.dialog.MaterialAlertDialogBuilder(context)
            .setTitle(R.string.keys)
            .setView(linearLayout)
            .setNegativeButton(R.string.close, null)
            .create()

        val primaryColor = ThemeHelper.getColor(context, com.google.android.material.R.attr.colorPrimary)
        val outlineColor = ThemeHelper.getColor(context, com.google.android.material.R.attr.colorOutline)
        val onSurfaceColor = ThemeHelper.getColor(context, com.google.android.material.R.attr.colorOnSurface)

        for (i in options.indices) {
            val btn = com.google.android.material.button.MaterialButton(
                context,
                null,
                com.google.android.material.R.attr.materialButtonOutlinedStyle
            ).apply {
                text = options[i]
                isAllCaps = false
                textAlignment = View.TEXT_ALIGNMENT_VIEW_START
                icon = ContextCompat.getDrawable(context, icons[i])
                iconGravity = com.google.android.material.button.MaterialButton.ICON_GRAVITY_START
                iconPadding = (12 * resources.displayMetrics.density).toInt()
                cornerRadius = (12 * resources.displayMetrics.density).toInt()
                strokeWidth = (1 * resources.displayMetrics.density).toInt()

                if (i == 0) {
                    strokeColor = android.content.res.ColorStateList.valueOf(primaryColor)
                    setTextColor(primaryColor)
                    iconTint = android.content.res.ColorStateList.valueOf(primaryColor)
                } else {
                    strokeColor = android.content.res.ColorStateList.valueOf(outlineColor)
                    setTextColor(onSurfaceColor)
                    iconTint = android.content.res.ColorStateList.valueOf(onSurfaceColor)
                }

                val lp = android.widget.LinearLayout.LayoutParams(
                    android.widget.LinearLayout.LayoutParams.MATCH_PARENT,
                    android.widget.LinearLayout.LayoutParams.WRAP_CONTENT
                ).apply {
                    topMargin = if (i > 0) (8 * resources.displayMetrics.density).toInt() else 0
                }
                layoutParams = lp

                setOnClickListener {
                    dialog.dismiss()
                    when (i) {
                        0 -> {
                            val onlineDialog = OnlineToolsDialogFragment.newInstance(OnlineToolsDialogFragment.TYPE_KEYS)
                            onlineDialog.onInstalled = { refreshAllButtonStates() }
                            onlineDialog.show(parentFragmentManager, OnlineToolsDialogFragment.TAG)
                        }
                        1 -> getProdKey.launch(arrayOf("*/*"))
                        2 -> getKeysFolder.launch(null)
                    }
                }
            }
            linearLayout.addView(btn)
        }

        dialog.show()
    }

    private fun showFirmwareDialog() {
        val context = androidx.appcompat.view.ContextThemeWrapper(
            requireContext(),
            ThemeHelper.getSelectedStaticThemeColor()
        )
        val options = arrayOf(
            getString(R.string.online_install_recommended),
            getString(R.string.select_firmware_zip),
            getString(R.string.select_firmware_folder)
        )
        val icons = intArrayOf(
            R.drawable.ic_website,
            R.drawable.ic_firmware,
            R.drawable.ic_folder_open
        )

        val linearLayout = android.widget.LinearLayout(context).apply {
            orientation = android.widget.LinearLayout.VERTICAL
            val padH = (20 * resources.displayMetrics.density).toInt()
            val padV = (12 * resources.displayMetrics.density).toInt()
            setPadding(padH, padV, padH, padV)
        }

        val dialog = com.google.android.material.dialog.MaterialAlertDialogBuilder(context)
            .setTitle(R.string.firmware)
            .setView(linearLayout)
            .setNegativeButton(R.string.close, null)
            .create()

        val primaryColor = ThemeHelper.getColor(context, com.google.android.material.R.attr.colorPrimary)
        val outlineColor = ThemeHelper.getColor(context, com.google.android.material.R.attr.colorOutline)
        val onSurfaceColor = ThemeHelper.getColor(context, com.google.android.material.R.attr.colorOnSurface)

        for (i in options.indices) {
            val btn = com.google.android.material.button.MaterialButton(
                context,
                null,
                com.google.android.material.R.attr.materialButtonOutlinedStyle
            ).apply {
                text = options[i]
                isAllCaps = false
                textAlignment = View.TEXT_ALIGNMENT_VIEW_START
                icon = ContextCompat.getDrawable(context, icons[i])
                iconGravity = com.google.android.material.button.MaterialButton.ICON_GRAVITY_START
                iconPadding = (12 * resources.displayMetrics.density).toInt()
                cornerRadius = (12 * resources.displayMetrics.density).toInt()
                strokeWidth = (1 * resources.displayMetrics.density).toInt()

                if (i == 0) {
                    strokeColor = android.content.res.ColorStateList.valueOf(primaryColor)
                    setTextColor(primaryColor)
                    iconTint = android.content.res.ColorStateList.valueOf(primaryColor)
                } else {
                    strokeColor = android.content.res.ColorStateList.valueOf(outlineColor)
                    setTextColor(onSurfaceColor)
                    iconTint = android.content.res.ColorStateList.valueOf(onSurfaceColor)
                }

                val lp = android.widget.LinearLayout.LayoutParams(
                    android.widget.LinearLayout.LayoutParams.MATCH_PARENT,
                    android.widget.LinearLayout.LayoutParams.WRAP_CONTENT
                ).apply {
                    topMargin = if (i > 0) (8 * resources.displayMetrics.density).toInt() else 0
                }
                layoutParams = lp

                setOnClickListener {
                    dialog.dismiss()
                    when (i) {
                        0 -> {
                            val onlineDialog = OnlineToolsDialogFragment.newInstance(OnlineToolsDialogFragment.TYPE_FIRMWARE)
                            onlineDialog.onInstalled = { refreshAllButtonStates() }
                            onlineDialog.show(parentFragmentManager, OnlineToolsDialogFragment.TAG)
                        }
                        1 -> getFirmware.launch(arrayOf("application/zip", "application/x-zip-compressed", "*/*"))
                        2 -> getFirmwareFolder.launch(null)
                    }
                }
            }
            linearLayout.addView(btn)
        }

        dialog.show()
    }
}

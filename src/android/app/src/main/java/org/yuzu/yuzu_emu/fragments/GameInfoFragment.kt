// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.updatePadding
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import androidx.navigation.fragment.navArgs
import com.google.android.material.transition.MaterialSharedAxis
import org.yuzu.yuzu_emu.NativeLibrary
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.databinding.FragmentGameInfoBinding
import org.yuzu.yuzu_emu.model.GameVerificationResult
import org.yuzu.yuzu_emu.model.HomeViewModel
import org.yuzu.yuzu_emu.utils.GameHelper
import org.yuzu.yuzu_emu.utils.GameIconUtils
import org.yuzu.yuzu_emu.utils.GameMetadata
import org.yuzu.yuzu_emu.utils.ThemeHelper
import org.yuzu.yuzu_emu.utils.ViewUtils.setVisible
import org.yuzu.yuzu_emu.utils.ViewUtils.updateMargins

class GameInfoFragment : Fragment() {
    private var _binding: FragmentGameInfoBinding? = null
    private val binding get() = _binding!!

    private val homeViewModel: HomeViewModel by activityViewModels()

    private val args by navArgs<GameInfoFragmentArgs>()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enterTransition = MaterialSharedAxis(MaterialSharedAxis.X, true)
        returnTransition = MaterialSharedAxis(MaterialSharedAxis.X, false)
        reenterTransition = MaterialSharedAxis(MaterialSharedAxis.X, false)

        // Ensure containers and updates are mounted before checking metadata
        GameHelper.restoreContentForGame(args.game)
        GameHelper.upgradeGameVersionIfNeeded(args.game)
        val newVer = GameMetadata.getVersion(args.game.path, true)
        val newIntVer = GameMetadata.getInternalVersion(args.game.path)
        if (newVer.isNotEmpty() && !GameHelper.isBaseVersion(newVer)) {
            args.game.version = newVer
        } else if (GameHelper.isBaseVersion(args.game.version) && newVer.isNotEmpty()) {
            args.game.version = newVer
        }
        if (newIntVer.isNotEmpty() && newIntVer != "0") {
            args.game.internalVersion = newIntVer
        }
        GameHelper.upgradeGameVersionIfNeeded(args.game)
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentGameInfoBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        homeViewModel.setStatusBarShadeVisibility(false)

        binding.apply {
            toolbarInfo.title = getString(R.string.game_info_title)
            toolbarInfo.setNavigationOnClickListener {
                requireActivity().onBackPressedDispatcher.onBackPressed()
            }

            // Bind Game Header Card
            textGameTitle.text = args.game.title
            textGameDeveloperHeader.text = if (args.game.developer.isNotEmpty()) args.game.developer else "Nintendo"
            GameIconUtils.loadGameIcon(args.game, imageGameIcon)

            val ext = try {
                val p = Uri.parse(args.game.path).path ?: args.game.path
                val dot = p.lastIndexOf('.')
                if (dot != -1) p.substring(dot + 1).uppercase() else "NSP"
            } catch (_: Exception) {
                "NSP"
            }
            badgeGameFormat.text = ext

            val cleanVer = args.game.version.trim().removePrefix("v").removePrefix("V").ifEmpty { "1.0.0" }
            val cleanIntVer = args.game.internalVersion.trim().removePrefix("v").removePrefix("V")
            val fullVersionText = if (cleanIntVer.isNotEmpty() && cleanIntVer != "0") {
                "$cleanVer ($cleanIntVer)"
            } else {
                cleanVer
            }
            badgeGameVersion.text = cleanVer

            // Bind Path
            val pathString = Uri.parse(args.game.path).path ?: ""
            pathField.text = pathString
            pathField.setOnClickListener { copyToClipboard(getString(R.string.path), pathString) }
            path.setOnClickListener { copyToClipboard(getString(R.string.path), pathString) }
            buttonCopyPath.setOnClickListener { copyToClipboard(getString(R.string.path), pathString) }

            // Bind Program ID
            programIdField.text = args.game.programIdHex
            programIdField.setOnClickListener { copyToClipboard(getString(R.string.program_id), args.game.programIdHex) }
            programId.setOnClickListener { copyToClipboard(getString(R.string.program_id), args.game.programIdHex) }
            buttonCopyProgramId.setOnClickListener { copyToClipboard(getString(R.string.program_id), args.game.programIdHex) }

            // Bind Developer
            if (args.game.developer.isNotEmpty()) {
                developerField.text = args.game.developer
                developerField.setOnClickListener { copyToClipboard(getString(R.string.developer), args.game.developer) }
                developer.setOnClickListener { copyToClipboard(getString(R.string.developer), args.game.developer) }
                buttonCopyDeveloper.setOnClickListener { copyToClipboard(getString(R.string.developer), args.game.developer) }
            } else {
                developer.setVisible(false)
            }

            // Bind Version
            versionField.text = fullVersionText
            versionField.setOnClickListener { copyToClipboard(getString(R.string.version), fullVersionText) }
            version.setOnClickListener { copyToClipboard(getString(R.string.version), fullVersionText) }
            buttonCopyVersion.setOnClickListener { copyToClipboard(getString(R.string.version), fullVersionText) }

            // Bind File Size
            val fileSizeBytes: Long = try {
                val uri = Uri.parse(args.game.path)
                if (uri.scheme == "content") {
                    requireContext().contentResolver.openFileDescriptor(uri, "r")?.use { it.statSize } ?: 0L
                } else {
                    val f = java.io.File(uri.path ?: args.game.path)
                    if (f.exists()) f.length() else 0L
                }
            } catch (_: Exception) {
                0L
            }
            val formattedSize = if (fileSizeBytes > 0) {
                android.text.format.Formatter.formatFileSize(requireContext(), fileSizeBytes)
            } else {
                "—"
            }
            fileSizeField.text = formattedSize
            fileSizeField.setOnClickListener { copyToClipboard(getString(R.string.file_size), formattedSize) }
            cardFileSize.setOnClickListener { copyToClipboard(getString(R.string.file_size), formattedSize) }
            buttonCopyFileSize.setOnClickListener { copyToClipboard(getString(R.string.file_size), formattedSize) }

            buttonCopy.setOnClickListener {
                val details = """
                    ${args.game.title}
                    ${getString(R.string.path)} - $pathString
                    ${getString(R.string.program_id)} - ${args.game.programIdHex}
                    ${getString(R.string.developer)} - ${args.game.developer}
                    ${getString(R.string.version)} - $fullVersionText
                    ${getString(R.string.file_size)} - $formattedSize
                """.trimIndent()
                copyToClipboard(args.game.title, details)
            }

            buttonVerifyIntegrity.setOnClickListener {
                ProgressDialogFragment.newInstance(
                    requireActivity(),
                    R.string.verifying,
                    true
                ) { progressCallback, _ ->
                    val result = GameVerificationResult.from(
                        NativeLibrary.verifyGameContents(
                            args.game.path,
                            progressCallback
                        )
                    )
                    return@newInstance when (result) {
                        GameVerificationResult.Success ->
                            MessageDialogFragment.newInstance(
                                titleId = R.string.verify_success,
                                descriptionId = R.string.operation_completed_successfully
                            )

                        GameVerificationResult.Failed ->
                            MessageDialogFragment.newInstance(
                                titleId = R.string.verify_failure,
                                descriptionId = R.string.verify_failure_description
                            )

                        GameVerificationResult.NotImplemented ->
                            MessageDialogFragment.newInstance(
                                titleId = R.string.verify_no_result,
                                descriptionId = R.string.verify_no_result_description
                            )
                    }
                }.show(parentFragmentManager, ProgressDialogFragment.TAG)
            }
        }

        setInsets()
    }

    private fun copyToClipboard(label: String, body: String) {
        val clipBoard =
            requireContext().getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
        val clip = ClipData.newPlainText(label, body)
        clipBoard.setPrimaryClip(clip)

        ThemeHelper.showThemedSnackbar(binding.root, getString(R.string.copied_to_clipboard))
    }

    private fun setInsets() =
        ViewCompat.setOnApplyWindowInsetsListener(
            binding.root
        ) { _: View, windowInsets: WindowInsetsCompat ->
            val barInsets = windowInsets.getInsets(WindowInsetsCompat.Type.systemBars())
            val cutoutInsets = windowInsets.getInsets(WindowInsetsCompat.Type.displayCutout())

            val leftInsets = barInsets.left + cutoutInsets.left
            val rightInsets = barInsets.right + cutoutInsets.right

            binding.toolbarInfo.updateMargins(left = leftInsets, right = rightInsets)
            binding.scrollInfo.updateMargins(left = leftInsets, right = rightInsets)

            binding.contentInfo.updatePadding(bottom = barInsets.bottom)

            windowInsets
        }
}

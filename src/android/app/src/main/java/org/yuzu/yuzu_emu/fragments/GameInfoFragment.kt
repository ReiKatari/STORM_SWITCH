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

            // Bind Program ID (TID)
            val tidText = args.game.programIdHex
            programIdField.text = tidText
            programIdField.setOnClickListener { copyToClipboard("Title ID (TID)", tidText) }
            programId.setOnClickListener { copyToClipboard("Title ID (TID)", tidText) }
            buttonCopyProgramId.setOnClickListener { copyToClipboard("Title ID (TID)", tidText) }

            // Bind Build ID (BID) (Issue #79)
            val bidText = getGameBuildId(args.game)
            buildIdField?.text = bidText
            buildIdField?.setOnClickListener {
                if (bidText != "—") {
                    copyToClipboard("Build ID (BID)", bidText)
                }
            }
            cardBuildId?.setOnClickListener {
                if (bidText != "—") {
                    copyToClipboard("Build ID (BID)", bidText)
                }
            }
            buttonCopyBuildId?.setOnClickListener {
                if (bidText != "—") {
                    copyToClipboard("Build ID (BID)", bidText)
                }
            }

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
                    Title ID (TID) - $tidText
                    Build ID (BID) - $bidText
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

    private fun getGameBuildId(game: org.yuzu.yuzu_emu.model.Game): String {
        val tid = game.programIdHex
        // 1. Check cheats directory: userDirectory/cheats/<TID>/
        try {
            val baseDir = org.yuzu.yuzu_emu.utils.DirectoryInitialization.userDirectory
                ?: requireContext().filesDir.absolutePath
            val cheatsDir = java.io.File(baseDir, "cheats/$tid")
            if (cheatsDir.exists()) {
                cheatsDir.listFiles()?.forEach { f ->
                    val name = f.nameWithoutExtension.uppercase()
                    if (name.length in 16..64 && name.all { it.isDigit() || (it in 'A'..'F') } && name != tid && name != "CHEATS" && name != "ALL") {
                        return name.take(16)
                    }
                }
            }
        } catch (_: Exception) {}

        // 2. Check load directory: userDirectory/load/<TID>/
        try {
            val baseDir = org.yuzu.yuzu_emu.utils.DirectoryInitialization.userDirectory
                ?: requireContext().filesDir.absolutePath
            val loadDir = java.io.File(baseDir, "load/$tid")
            if (loadDir.exists()) {
                val files = loadDir.walkTopDown().maxDepth(4)
                for (f in files) {
                    if (f.isFile) {
                        val name = f.nameWithoutExtension.uppercase()
                        if (name.length in 16..64 && name.all { it.isDigit() || (it in 'A'..'F') } && name != tid) {
                            return name.take(16)
                        }
                        if (f.extension.equals("pchtxt", ignoreCase = true)) {
                            f.useLines { lines ->
                                for (l in lines) {
                                    val trimmed = l.trim()
                                    if (trimmed.startsWith("@nsobid-", ignoreCase = true)) {
                                        val bid = trimmed.substring(8).trim().uppercase()
                                        if (bid.isNotEmpty()) {
                                            return bid.take(16)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } catch (_: Exception) {}

        // 3. Scan game file header for NSO0 magic (0x4E 0x53 0x4F 0x30)
        try {
            val uri = Uri.parse(game.path)
            val pfd = if (uri.scheme == "content") {
                requireContext().contentResolver.openFileDescriptor(uri, "r")
            } else {
                android.os.ParcelFileDescriptor.open(java.io.File(uri.path ?: game.path), android.os.ParcelFileDescriptor.MODE_READ_ONLY)
            }
            pfd?.use { fd ->
                java.io.FileInputStream(fd.fileDescriptor).use { fis ->
                    val buf = ByteArray(65536)
                    var totalRead = 0
                    val maxScan = 16 * 1024 * 1024
                    while (totalRead < maxScan) {
                        val read = fis.read(buf)
                        if (read <= 0) break
                        for (i in 0 until read - 0x60) {
                            if (buf[i] == 0x4E.toByte() && buf[i+1] == 0x53.toByte() &&
                                buf[i+2] == 0x4F.toByte() && buf[i+3] == 0x30.toByte()) {
                                val bidBytes = ByteArray(16)
                                System.arraycopy(buf, i + 0x40, bidBytes, 0, 16)
                                val hex = bidBytes.joinToString("") { "%02X".format(it) }
                                if (hex != "00000000000000000000000000000000" && hex.isNotEmpty()) {
                                    return hex.take(16)
                                }
                            }
                        }
                        totalRead += read
                    }
                }
            }
        } catch (_: Exception) {}

        return "—"
    }

    private fun copyToClipboard(label: String, body: String) {
        val clipBoard =
            requireContext().getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
        val clip = ClipData.newPlainText(label, body)
        clipBoard.setPrimaryClip(clip)

        Toast.makeText(requireContext(), "$label: ${getString(R.string.copied_to_clipboard)}", Toast.LENGTH_SHORT).show()
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

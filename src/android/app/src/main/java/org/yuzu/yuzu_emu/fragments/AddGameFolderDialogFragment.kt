// SPDX-FileCopyrightText: Copyright 2025 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2025 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.app.Dialog
import android.content.DialogInterface
import android.net.Uri
import android.os.Bundle
import android.view.LayoutInflater
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.activityViewModels
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.databinding.DialogAddFolderBinding
import org.yuzu.yuzu_emu.model.GameDir
import org.yuzu.yuzu_emu.model.GamesViewModel
import org.yuzu.yuzu_emu.model.HomeViewModel

class AddGameFolderDialogFragment : DialogFragment() {
    private val homeViewModel: HomeViewModel by activityViewModels()
    private val gamesViewModel: GamesViewModel by activityViewModels()

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val themedContext = androidx.appcompat.view.ContextThemeWrapper(
            requireContext(),
            org.yuzu.yuzu_emu.utils.ThemeHelper.getSelectedStaticThemeColor()
        )
        val binding = DialogAddFolderBinding.inflate(LayoutInflater.from(themedContext))
        val folderUriString = arguments?.getString(FOLDER_URI_STRING) ?: ""
        if (folderUriString.isBlank()) {
            dismiss()
        }
        val rawPath = try {
            Uri.parse(folderUriString).path ?: folderUriString
        } catch (_: Exception) {
            folderUriString
        }
        val displayPath = try {
            Uri.decode(rawPath).replace("/tree/primary:", "")
        } catch (_: Exception) {
            rawPath
        }
        binding.path.text = displayPath
        binding.deepScanSwitch.isChecked = true

        return MaterialAlertDialogBuilder(themedContext)
            .setTitle(R.string.add_game_folder)
            .setPositiveButton(android.R.string.ok) { _: DialogInterface, _: Int ->
                if (folderUriString.isNotBlank()) {
                    val newGameDir = GameDir(folderUriString, binding.deepScanSwitch.isChecked)
                    val calledFromGameFragment = arguments?.getBoolean(
                        "calledFromGameFragment",
                        false
                    ) ?: false
                    try {
                        val hvm = try { homeViewModel } catch (_: Exception) { null }
                        val gvm = try { gamesViewModel } catch (_: Exception) { null }
                        val job = gvm?.addFolder(newGameDir, calledFromGameFragment)
                        job?.invokeOnCompletion {
                            try {
                                hvm?.setGamesDirSelected(true)
                            } catch (_: Exception) {}
                        }
                    } catch (e: Throwable) {
                        android.util.Log.e("STORM_SWITCH", "addFolder error: ${e.message}")
                    }
                }
            }
            .setNegativeButton(android.R.string.cancel, null)
            .setView(binding.root)
            .create()
    }

    companion object {
        const val TAG = "AddGameFolderDialogFragment"

        private const val FOLDER_URI_STRING = "FolderUriString"

        fun newInstance(folderUriString: String, calledFromGameFragment: Boolean): AddGameFolderDialogFragment {
            val args = Bundle()
            args.putString(FOLDER_URI_STRING, folderUriString)
            args.putBoolean("calledFromGameFragment", calledFromGameFragment)
            val fragment = AddGameFolderDialogFragment()
            fragment.arguments = args
            return fragment
        }
    }
}

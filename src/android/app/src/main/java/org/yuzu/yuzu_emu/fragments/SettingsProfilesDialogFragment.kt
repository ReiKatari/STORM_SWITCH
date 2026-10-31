// SPDX-FileCopyrightText: Copyright 2026 STORM TEAM
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.app.Dialog
import android.content.res.Configuration
import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.net.Uri
import android.os.Bundle
import android.view.KeyEvent
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.fragment.app.DialogFragment
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.databinding.DialogSaveProfileBinding
import org.yuzu.yuzu_emu.databinding.DialogSettingsProfilesBinding
import org.yuzu.yuzu_emu.databinding.ItemSettingsProfileBinding
import org.yuzu.yuzu_emu.features.settings.utils.SettingsProfile
import org.yuzu.yuzu_emu.features.settings.utils.SettingsProfileManager
import org.yuzu.yuzu_emu.utils.ThemeHelper

class SettingsProfilesDialogFragment : DialogFragment() {

    private var _binding: DialogSettingsProfilesBinding? = null
    private val binding get() = _binding!!

    private var profilesList = mutableListOf<SettingsProfile>()
    private var adapter: ProfilesAdapter? = null

    private val importProfileLauncher = registerForActivityResult(ActivityResultContracts.OpenDocument()) { uri: Uri? ->
        if (uri != null) {
            try {
                requireContext().contentResolver.openInputStream(uri)?.use { stream ->
                    val imported = SettingsProfileManager.importProfileFromStream(stream)
                    if (imported != null) {
                        Toast.makeText(requireContext(), getString(R.string.profile_import_success, imported.name), Toast.LENGTH_SHORT).show()
                        refreshProfiles()
                    } else {
                        Toast.makeText(requireContext(), getString(R.string.profile_import_failed), Toast.LENGTH_SHORT).show()
                    }
                }
            } catch (e: Exception) {
                Toast.makeText(requireContext(), getString(R.string.profile_import_failed), Toast.LENGTH_SHORT).show()
            }
        }
    }

    companion object {
        const val TAG = "SettingsProfilesDialogFragment"

        fun newInstance(): SettingsProfilesDialogFragment {
            return SettingsProfilesDialogFragment()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setStyle(STYLE_NORMAL, ThemeHelper.getSelectedStaticThemeColor())
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val dialog = super.onCreateDialog(savedInstanceState)
        dialog.requestWindowFeature(android.view.Window.FEATURE_NO_TITLE)
        dialog.window?.let { window ->
            window.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
            WindowCompat.setDecorFitsSystemWindows(window, false)
        }
        return dialog
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = DialogSettingsProfilesBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        ViewCompat.setOnApplyWindowInsetsListener(binding.root) { v, insets ->
            val systemBars = insets.getInsets(
                WindowInsetsCompat.Type.systemBars() or WindowInsetsCompat.Type.displayCutout()
            )
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom)
            insets
        }
        binding.root.requestApplyInsets()

        binding.btnCloseProfiles.setOnClickListener { dismiss() }

        binding.btnSaveCurrentProfile.setOnClickListener {
            showSaveProfileDialog()
        }

        binding.btnImportProfile.setOnClickListener {
            importProfileLauncher.launch(arrayOf("*/*"))
        }

        // Apply 3D Faceted Tactile Button System
                        binding.listProfiles.layoutManager = LinearLayoutManager(requireContext())
        adapter = ProfilesAdapter(
            profiles = profilesList,
            onApply = { profile ->
                val success = SettingsProfileManager.applyProfile(profile)
                if (success) {
                    Toast.makeText(requireContext(), getString(R.string.profile_applied_toast, profile.name), Toast.LENGTH_SHORT).show()
                    dismiss()
                } else {
                    Toast.makeText(requireContext(), getString(R.string.profile_apply_failed), Toast.LENGTH_SHORT).show()
                }
            },
            onShare = { profile ->
                SettingsProfileManager.shareProfile(requireContext(), profile)
            },
            onRename = { profile ->
                showRenameProfileDialog(profile)
            },
            onDelete = { profile ->
                showDeleteProfileDialog(profile)
            }
        )
        binding.listProfiles.adapter = adapter

        refreshProfiles()
    }

    private fun refreshProfiles() {
        profilesList.clear()
        profilesList.addAll(SettingsProfileManager.getCustomProfiles())
        _binding?.let { b ->
            b.textProfilesCounter?.text = profilesList.size.toString()
            if (profilesList.isEmpty()) {
                b.textEmptyProfiles?.visibility = View.VISIBLE
                b.listProfiles.visibility = View.GONE
            } else {
                b.textEmptyProfiles?.visibility = View.GONE
                b.listProfiles.visibility = View.VISIBLE
            }
        }
        adapter?.notifyDataSetChanged()
    }

    private fun showSaveProfileDialog() {
        val dialogBinding = DialogSaveProfileBinding.inflate(layoutInflater)
        dialogBinding.textSaveProfileTitle.text = getString(R.string.profile_create_title)
        dialogBinding.textSaveProfileMsg.text = getString(R.string.profile_create_msg)

        val dialog = MaterialAlertDialogBuilder(requireContext())
            .setView(dialogBinding.root)
            .create()

        dialog.window?.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))

                        dialogBinding.btnCancelSaveProfile.setOnClickListener { dialog.dismiss() }
        dialogBinding.btnConfirmSaveProfile.setOnClickListener {
            val name = dialogBinding.editProfileName.text?.toString()?.trim().orEmpty()
            if (name.isNotEmpty()) {
                val saved = SettingsProfileManager.saveCurrentSettingsAsProfile(name)
                if (saved != null) {
                    Toast.makeText(requireContext(), getString(R.string.profile_saved_toast, name), Toast.LENGTH_SHORT).show()
                    refreshProfiles()
                    dialog.dismiss()
                }
            } else {
                dialogBinding.inputLayoutProfileName.error = getString(R.string.profile_name_hint)
            }
        }

        dialog.show()
    }

    private fun showRenameProfileDialog(profile: SettingsProfile) {
        val dialogBinding = DialogSaveProfileBinding.inflate(layoutInflater)
        dialogBinding.textSaveProfileTitle.text = getString(R.string.profile_rename_title)
        dialogBinding.textSaveProfileMsg.text = ""
        dialogBinding.editProfileName.setText(profile.name)

        val dialog = MaterialAlertDialogBuilder(requireContext())
            .setView(dialogBinding.root)
            .create()

        dialog.window?.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))

                        dialogBinding.btnCancelSaveProfile.setOnClickListener { dialog.dismiss() }
        dialogBinding.btnConfirmSaveProfile.setOnClickListener {
            val newName = dialogBinding.editProfileName.text?.toString()?.trim().orEmpty()
            if (newName.isNotEmpty() && SettingsProfileManager.renameProfile(profile, newName)) {
                refreshProfiles()
                dialog.dismiss()
            } else {
                dialogBinding.inputLayoutProfileName.error = getString(R.string.profile_name_hint)
            }
        }

        dialog.show()
    }

    private fun showDeleteProfileDialog(profile: SettingsProfile) {
        val alert = MaterialAlertDialogBuilder(requireContext(), R.style.EdenMaterialDialog)
            .setTitle(R.string.profile_delete_title)
            .setMessage(getString(R.string.profile_delete_confirmation, profile.name))
            .setPositiveButton(R.string.delete) { _, _ ->
                if (SettingsProfileManager.deleteProfile(profile)) {
                    refreshProfiles()
                }
            }
            .setNegativeButton(android.R.string.cancel, null)
            .create()

        alert.setOnShowListener {
                    }
        alert.show()
    }

    override fun onStart() {
        super.onStart()
        dialog?.let { d ->
            d.window?.let { window ->
                window.setLayout(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT
                )
                window.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
                WindowCompat.setDecorFitsSystemWindows(window, false)
                ThemeHelper.applySystemBarsTheme(window, requireContext())
            }
            // Mandatory ESC Modal Dismissal
            d.setOnKeyListener { _, keyCode, event ->
                if (keyCode == KeyEvent.KEYCODE_ESCAPE && event.action == KeyEvent.ACTION_UP) {
                    dismiss()
                    true
                } else {
                    false
                }
            }
        }
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        dialog?.window?.let { window ->
            window.setLayout(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
            )
            window.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
            WindowCompat.setDecorFitsSystemWindows(window, false)
            ThemeHelper.applySystemBarsTheme(window, requireContext())
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }

    private class ProfilesAdapter(
        private val profiles: List<SettingsProfile>,
        private val onApply: (SettingsProfile) -> Unit,
        private val onShare: (SettingsProfile) -> Unit,
        private val onRename: (SettingsProfile) -> Unit,
        private val onDelete: (SettingsProfile) -> Unit
    ) : RecyclerView.Adapter<ProfilesAdapter.ViewHolder>() {

        class ViewHolder(val binding: ItemSettingsProfileBinding) : RecyclerView.ViewHolder(binding.root)

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
            val binding = ItemSettingsProfileBinding.inflate(LayoutInflater.from(parent.context), parent, false)
            return ViewHolder(binding)
        }

        override fun onBindViewHolder(holder: ViewHolder, position: Int) {
            val profile = profiles[position]
            holder.binding.textProfileName.text = profile.name
            holder.binding.textProfileDesc.text = profile.description

            if (profile.isPreset) {
                holder.binding.imageProfileIcon.setImageResource(R.drawable.ic_settings)
                holder.binding.layoutCustomActions.visibility = View.GONE
            } else {
                holder.binding.imageProfileIcon.setImageResource(R.drawable.ic_folder_open)
                holder.binding.layoutCustomActions.visibility = View.VISIBLE
            }

            // Apply 3D Faceted Tactile Buttons to item action buttons
                                                            holder.binding.btnProfileApply.setOnClickListener { onApply(profile) }
            holder.binding.btnProfileShare.setOnClickListener { onShare(profile) }
            holder.binding.btnProfileRename.setOnClickListener { onRename(profile) }
            holder.binding.btnProfileDelete.setOnClickListener { onDelete(profile) }
        }

        override fun getItemCount(): Int = profiles.size
    }
}

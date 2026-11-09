// SPDX-FileCopyrightText: Copyright 2026 STORM TEAM

// SPDX-License-Identifier: GPL-3.0-or-later



package org.yuzu.yuzu_emu.fragments



import android.app.Dialog

import android.content.Context

import android.content.res.Configuration

import android.graphics.Color

import android.graphics.drawable.ColorDrawable

import android.net.Uri

import android.os.Bundle

import android.text.Editable

import android.text.TextWatcher

import android.view.Gravity

import android.view.KeyEvent

import android.view.LayoutInflater

import android.view.View

import android.view.ViewGroup

import android.widget.LinearLayout

import android.widget.TextView

import android.widget.Toast

import androidx.activity.result.contract.ActivityResultContracts

import androidx.core.view.ViewCompat

import androidx.core.view.WindowCompat

import androidx.core.view.WindowInsetsCompat

import androidx.core.widget.doAfterTextChanged

import androidx.fragment.app.DialogFragment

import androidx.recyclerview.widget.LinearLayoutManager

import androidx.recyclerview.widget.RecyclerView

import com.google.android.material.card.MaterialCardView

import com.google.android.material.dialog.MaterialAlertDialogBuilder

import org.yuzu.yuzu_emu.R

import org.yuzu.yuzu_emu.databinding.DialogSaveProfileBinding

import org.yuzu.yuzu_emu.databinding.DialogSettingsProfilesBinding

import org.yuzu.yuzu_emu.databinding.ItemSettingsProfileBinding

import org.yuzu.yuzu_emu.features.settings.utils.SettingsProfile

import org.yuzu.yuzu_emu.features.settings.utils.SettingsProfileManager

import org.yuzu.yuzu_emu.utils.StormDesignEngine

import org.yuzu.yuzu_emu.utils.ThemeHelper

import java.text.SimpleDateFormat

import java.util.Date

import java.util.Locale



class SettingsProfilesDialogFragment : DialogFragment() {



    private var _binding: DialogSettingsProfilesBinding? = null

    private val binding get() = _binding!!



    private var allBuiltInProfiles = listOf<SettingsProfile>()

    private var allCustomProfiles = listOf<SettingsProfile>()

    private var currentFilterMode = FILTER_ALL

    private var currentSearchQuery = ""



    private var adapter: GroupedProfilesAdapter? = null



    private val importProfileLauncher = registerForActivityResult(ActivityResultContracts.OpenDocument()) { uri: Uri? ->

        if (uri != null) {

            try {

                requireContext().contentResolver.openInputStream(uri)?.use { stream ->

                    val imported = SettingsProfileManager.importProfileFromStream(stream)

                    if (imported != null) {

                        Toast.makeText(

                            requireContext(),

                            getString(R.string.storm_prof_import_success_fmt, imported.name),

                            Toast.LENGTH_SHORT

                        ).show()

                        refreshProfiles()

                    } else {

                        Toast.makeText(requireContext(), getString(R.string.storm_prof_import_error), Toast.LENGTH_SHORT).show()

                    }

                }

            } catch (e: Exception) {

                Toast.makeText(requireContext(), getString(R.string.storm_prof_import_error), Toast.LENGTH_SHORT).show()

            }

        }

    }



    companion object {

        const val TAG = "SettingsProfilesDialogFragment"



        private const val FILTER_ALL = 0

        private const val FILTER_PRESETS = 1

        private const val FILTER_CUSTOM = 2



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



        setupFiltersAndSearch()

        setupRecyclerView()

        refreshProfiles()

    }



    private fun setupFiltersAndSearch() {

        binding.chipGroupProfiles.setOnCheckedStateChangeListener { _, checkedIds ->

            currentFilterMode = when {

                checkedIds.contains(binding.chipFilterPresets.id) -> FILTER_PRESETS

                checkedIds.contains(binding.chipFilterCustom.id) -> FILTER_CUSTOM

                else -> FILTER_ALL

            }

            updateDisplayedList()

        }



        binding.editSearchProfile.doAfterTextChanged { text ->

            currentSearchQuery = text?.toString()?.trim().orEmpty().lowercase()

            updateDisplayedList()

        }

    }



    private fun setupRecyclerView() {

        binding.listProfiles.layoutManager = LinearLayoutManager(requireContext())

        adapter = GroupedProfilesAdapter(

            onApply = { profile ->

                val success = SettingsProfileManager.applyProfile(profile, requireContext())

                if (success) {

                    Toast.makeText(

                        requireContext(),

                        getString(R.string.storm_prof_applied_fmt, profile.name),

                        Toast.LENGTH_SHORT

                    ).show()

                    dismiss()

                } else {

                    Toast.makeText(requireContext(), getString(R.string.profile_apply_failed), Toast.LENGTH_SHORT).show()

                }

            },

            onDuplicate = { profile ->

                val duplicated = SettingsProfileManager.duplicatePreset(profile, requireContext())

                if (duplicated != null) {

                    Toast.makeText(

                        requireContext(),

                        getString(R.string.storm_prof_duplicate_success, duplicated.name),

                        Toast.LENGTH_SHORT

                    ).show()

                    refreshProfiles()

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

            },

            onCreateCustomClicked = {

                showSaveProfileDialog()

            }

        )

        binding.listProfiles.adapter = adapter

    }



    private fun refreshProfiles() {

        allBuiltInProfiles = SettingsProfileManager.getBuiltInPresets(requireContext())

        allCustomProfiles = SettingsProfileManager.getCustomProfiles()



        _binding?.let { b ->

            val totalCount = allBuiltInProfiles.size + allCustomProfiles.size

            b.textProfilesCounter.text = totalCount.toString()

            b.textProfilesSubtitle.text = getString(

                R.string.storm_prof_subtitle_fmt,

                allBuiltInProfiles.size,

                allCustomProfiles.size

            )

        }



        updateDisplayedList()

    }



    private fun updateDisplayedList() {

        val query = currentSearchQuery

        val filteredBuiltIn = if (query.isEmpty()) {

            allBuiltInProfiles

        } else {

            allBuiltInProfiles.filter {

                it.name.lowercase().contains(query) || it.description.lowercase().contains(query)

            }

        }



        val filteredCustom = if (query.isEmpty()) {

            allCustomProfiles

        } else {

            allCustomProfiles.filter {

                it.name.lowercase().contains(query) || it.description.lowercase().contains(query)

            }

        }



        val items = mutableListOf<AdapterItem>()



        when (currentFilterMode) {

            FILTER_ALL -> {

                if (filteredBuiltIn.isNotEmpty()) {

                    items.add(AdapterItem.Header(getString(R.string.storm_prof_section_presets), filteredBuiltIn.size))

                    filteredBuiltIn.forEach { items.add(AdapterItem.Profile(it)) }

                }



                items.add(AdapterItem.Header(getString(R.string.storm_prof_section_custom), filteredCustom.size))

                if (filteredCustom.isNotEmpty()) {

                    filteredCustom.forEach { items.add(AdapterItem.Profile(it)) }

                } else if (query.isEmpty()) {

                    items.add(AdapterItem.EmptyCustomHint)

                }

            }

            FILTER_PRESETS -> {

                if (filteredBuiltIn.isNotEmpty()) {

                    items.add(AdapterItem.Header(getString(R.string.storm_prof_section_presets), filteredBuiltIn.size))

                    filteredBuiltIn.forEach { items.add(AdapterItem.Profile(it)) }

                }

            }

            FILTER_CUSTOM -> {

                items.add(AdapterItem.Header(getString(R.string.storm_prof_section_custom), filteredCustom.size))

                if (filteredCustom.isNotEmpty()) {

                    filteredCustom.forEach { items.add(AdapterItem.Profile(it)) }

                } else if (query.isEmpty()) {

                    items.add(AdapterItem.EmptyCustomHint)

                }

            }

        }



        _binding?.let { b ->

            if (items.isEmpty()) {

                b.textEmptyProfiles.visibility = View.VISIBLE

                b.listProfiles.visibility = View.GONE

            } else {

                b.textEmptyProfiles.visibility = View.GONE

                b.listProfiles.visibility = View.VISIBLE

            }

        }



        adapter?.submitItems(items)

    }



    private fun showSaveProfileDialog() {

        val dialogBinding = DialogSaveProfileBinding.inflate(layoutInflater)

        dialogBinding.textSaveProfileTitle.text = getString(R.string.storm_prof_save_title)

        dialogBinding.textSaveProfileMsg.text = getString(R.string.storm_prof_save_subtitle)



        val dialog = MaterialAlertDialogBuilder(requireContext())

            .setView(dialogBinding.root)

            .create()



        dialog.window?.let { window ->

            window.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))

        }



        dialogBinding.btnCancelSaveProfile.setOnClickListener { dialog.dismiss() }

        dialogBinding.btnConfirmSaveProfile.setOnClickListener {

            val name = dialogBinding.editProfileName.text?.toString()?.trim().orEmpty()

            if (name.isNotEmpty()) {

                val saved = SettingsProfileManager.saveCurrentSettingsAsProfile(name)

                if (saved != null) {

                    Toast.makeText(

                        requireContext(),

                        getString(R.string.profile_saved_toast, name),

                        Toast.LENGTH_SHORT

                    ).show()

                    refreshProfiles()

                    dialog.dismiss()

                }

            } else {

                dialogBinding.inputLayoutProfileName.error = getString(R.string.profile_name_hint)

            }

        }



        dialog.show()

        StormDesignEngine.applyToDialog(dialog, requireContext())

    }



    private fun showRenameProfileDialog(profile: SettingsProfile) {

        val dialogBinding = DialogSaveProfileBinding.inflate(layoutInflater)

        dialogBinding.textSaveProfileTitle.text = getString(R.string.storm_prof_rename_title)

        dialogBinding.textSaveProfileMsg.text = ""

        dialogBinding.editProfileName.setText(profile.name)



        val dialog = MaterialAlertDialogBuilder(requireContext())

            .setView(dialogBinding.root)

            .create()



        dialog.window?.let { window ->

            window.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))

        }



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

        StormDesignEngine.applyToDialog(dialog, requireContext())

    }



    private fun showDeleteProfileDialog(profile: SettingsProfile) {

        val alert = MaterialAlertDialogBuilder(requireContext(), R.style.EdenMaterialDialog)

            .setTitle(R.string.storm_prof_delete_title)

            .setMessage(getString(R.string.storm_prof_delete_msg_fmt, profile.name))

            .setPositiveButton(R.string.storm_prof_btn_delete) { _, _ ->

                if (SettingsProfileManager.deleteProfile(profile)) {

                    Toast.makeText(

                        requireContext(),

                        getString(R.string.storm_prof_deleted_fmt, profile.name),

                        Toast.LENGTH_SHORT

                    ).show()

                    refreshProfiles()

                }

            }

            .setNegativeButton(android.R.string.cancel, null)

            .create()



        alert.show()

        StormDesignEngine.applyToDialog(alert, requireContext())

    }



    override fun onStart() {

        super.onStart()

        dialog?.let { d ->

            applyWindowDimensions()

            StormDesignEngine.applyToDialog(d, requireContext())



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

        applyWindowDimensions()

        dialog?.let { StormDesignEngine.applyToDialog(it, requireContext()) }

    }



    private fun applyWindowDimensions() {

        dialog?.window?.let { window ->

            window.setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT)

            window.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))

            WindowCompat.setDecorFitsSystemWindows(window, false)

            ThemeHelper.applySystemBarsTheme(window, requireContext())

        }

    }



    override fun onDestroyView() {

        super.onDestroyView()

        _binding = null

    }



    sealed class AdapterItem {

        data class Header(val title: String, val count: Int) : AdapterItem()

        data class Profile(val profile: SettingsProfile) : AdapterItem()

        object EmptyCustomHint : AdapterItem()

    }



    private class GroupedProfilesAdapter(

        private val onApply: (SettingsProfile) -> Unit,

        private val onDuplicate: (SettingsProfile) -> Unit,

        private val onShare: (SettingsProfile) -> Unit,

        private val onRename: (SettingsProfile) -> Unit,

        private val onDelete: (SettingsProfile) -> Unit,

        private val onCreateCustomClicked: () -> Unit

    ) : RecyclerView.Adapter<RecyclerView.ViewHolder>() {



        private val items = mutableListOf<AdapterItem>()

        private val dateFormat = SimpleDateFormat("d MMM", Locale.getDefault())



        companion object {

            private const val TYPE_HEADER = 0

            private const val TYPE_PROFILE = 1

            private const val TYPE_EMPTY_HINT = 2

        }



        fun submitItems(newItems: List<AdapterItem>) {

            items.clear()

            items.addAll(newItems)

            notifyDataSetChanged()

        }



        override fun getItemViewType(position: Int): Int {

            return when (items[position]) {

                is AdapterItem.Header -> TYPE_HEADER

                is AdapterItem.Profile -> TYPE_PROFILE

                is AdapterItem.EmptyCustomHint -> TYPE_EMPTY_HINT

            }

        }



        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): RecyclerView.ViewHolder {

            val context = parent.context

            return when (viewType) {

                TYPE_HEADER -> {

                    val headerLayout = LinearLayout(context).apply {

                        orientation = LinearLayout.HORIZONTAL

                        gravity = Gravity.CENTER_VERTICAL

                        layoutParams = ViewGroup.MarginLayoutParams(

                            ViewGroup.LayoutParams.MATCH_PARENT,

                            ViewGroup.LayoutParams.WRAP_CONTENT

                        ).apply {

                            setMargins(4, 16, 4, 8)

                        }

                    }

                    val titleView = TextView(context).apply {

                        id = View.generateViewId()

                        textSize = 12f

                        paint.isFakeBoldText = true

                        letterSpacing = 0.05f

                        setTextColor(ThemeHelper.getColorFromAttr(context, com.google.android.material.R.attr.colorPrimary))

                    }

                    val countBadge = TextView(context).apply {

                        id = View.generateViewId()

                        textSize = 10f

                        paint.isFakeBoldText = true

                        setBackgroundResource(R.drawable.bg_storm_badge_capsule)

                        setPadding(16, 4, 16, 4)

                        setTextColor(ThemeHelper.getColorFromAttr(context, com.google.android.material.R.attr.colorPrimary))

                        val lp = LinearLayout.LayoutParams(

                            LinearLayout.LayoutParams.WRAP_CONTENT,

                            LinearLayout.LayoutParams.WRAP_CONTENT

                        ).apply {

                            marginStart = 16

                        }

                        layoutParams = lp

                    }

                    headerLayout.addView(titleView)

                    headerLayout.addView(countBadge)

                    HeaderViewHolder(headerLayout, titleView, countBadge)

                }

                TYPE_EMPTY_HINT -> {

                    val card = MaterialCardView(context).apply {

                        radius = 32f

                        cardElevation = 0f

                        strokeWidth = 0

                        setCardBackgroundColor(ThemeHelper.getColorFromAttr(context, com.google.android.material.R.attr.colorSurfaceVariant))

                        layoutParams = ViewGroup.MarginLayoutParams(

                            ViewGroup.LayoutParams.MATCH_PARENT,

                            ViewGroup.LayoutParams.WRAP_CONTENT

                        ).apply {

                            setMargins(4, 8, 4, 12)

                        }

                    }

                    val container = LinearLayout(context).apply {

                        orientation = LinearLayout.VERTICAL

                        setPadding(36, 32, 36, 32)

                        gravity = Gravity.CENTER_HORIZONTAL

                    }

                    val titleView = TextView(context).apply {

                        text = context.getString(R.string.storm_prof_empty_custom_title)

                        textSize = 14f

                        paint.isFakeBoldText = true

                        setTextColor(ThemeHelper.getColorFromAttr(context, com.google.android.material.R.attr.colorOnSurface))

                        gravity = Gravity.CENTER_HORIZONTAL

                    }

                    val descView = TextView(context).apply {

                        text = context.getString(R.string.storm_prof_empty_custom_desc)

                        textSize = 12f

                        setTextColor(ThemeHelper.getColorFromAttr(context, com.google.android.material.R.attr.colorOnSurfaceVariant))

                        gravity = Gravity.CENTER_HORIZONTAL

                        val lp = LinearLayout.LayoutParams(

                            LinearLayout.LayoutParams.MATCH_PARENT,

                            LinearLayout.LayoutParams.WRAP_CONTENT

                        ).apply {

                            topMargin = 10

                            bottomMargin = 24

                        }

                        layoutParams = lp

                    }

                    val btn = com.google.android.material.button.MaterialButton(context).apply {

                        text = context.getString(R.string.storm_prof_btn_save)

                        textSize = 12.5f

                        setBackgroundResource(R.drawable.storm_btn_3d_primary)

                        backgroundTintList = null

                        strokeWidth = 0

                        setTextColor(Color.WHITE)

                        setIconResource(R.drawable.ic_add)

                        iconTint = android.content.res.ColorStateList.valueOf(Color.WHITE)

                        iconSize = 36

                        setOnClickListener { onCreateCustomClicked() }

                        layoutParams = LinearLayout.LayoutParams(

                            LinearLayout.LayoutParams.WRAP_CONTENT,

                            ViewGroup.LayoutParams.WRAP_CONTENT

                        )

                    }

                    container.addView(titleView)

                    container.addView(descView)

                    container.addView(btn)

                    card.addView(container)

                    EmptyHintViewHolder(card)

                }

                else -> {

                    val binding = ItemSettingsProfileBinding.inflate(LayoutInflater.from(context), parent, false)

                    ProfileViewHolder(binding)

                }

            }

        }



        override fun onBindViewHolder(holder: RecyclerView.ViewHolder, position: Int) {

            when (val item = items[position]) {

                is AdapterItem.Header -> {

                    (holder as HeaderViewHolder).bind(item.title, item.count)

                }

                is AdapterItem.EmptyCustomHint -> {

                    // Static bind

                }

                is AdapterItem.Profile -> {

                    (holder as ProfileViewHolder).bind(

                        profile = item.profile,

                        dateFormat = dateFormat,

                        onApply = onApply,

                        onDuplicate = onDuplicate,

                        onShare = onShare,

                        onRename = onRename,

                        onDelete = onDelete

                    )

                }

            }

        }



        override fun getItemCount(): Int = items.size



        class HeaderViewHolder(

            view: View,

            val titleView: TextView,

            val countView: TextView

        ) : RecyclerView.ViewHolder(view) {

            fun bind(title: String, count: Int) {

                titleView.text = title

                countView.text = count.toString()

            }

        }



        class EmptyHintViewHolder(view: View) : RecyclerView.ViewHolder(view)



        class ProfileViewHolder(val binding: ItemSettingsProfileBinding) : RecyclerView.ViewHolder(binding.root) {

            fun bind(

                profile: SettingsProfile,

                dateFormat: SimpleDateFormat,

                onApply: (SettingsProfile) -> Unit,

                onDuplicate: (SettingsProfile) -> Unit,

                onShare: (SettingsProfile) -> Unit,

                onRename: (SettingsProfile) -> Unit,

                onDelete: (SettingsProfile) -> Unit

            ) {

                val context = binding.root.context

                binding.textProfileName.text = profile.name

                binding.textProfileDesc.text = profile.description



                if (profile.isPreset) {

                    binding.imageProfileIcon.setImageResource(R.drawable.ic_settings)

                    binding.textProfileBadge.text = context.getString(R.string.storm_prof_badge_preset)

                    binding.textProfileBadge.setTextColor(ThemeHelper.getColorFromAttr(context, com.google.android.material.R.attr.colorPrimary))



                    binding.layoutPresetActions.visibility = View.VISIBLE

                    binding.layoutCustomActions.visibility = View.GONE



                    binding.btnProfileDuplicate.setOnClickListener { onDuplicate(profile) }

                } else {

                    binding.imageProfileIcon.setImageResource(R.drawable.ic_folder_open)

                    val dateStr = dateFormat.format(Date(profile.timestamp))

                    binding.textProfileBadge.text = context.getString(R.string.storm_prof_badge_custom_fmt, dateStr)

                    binding.textProfileBadge.setTextColor(ThemeHelper.getColorFromAttr(context, com.google.android.material.R.attr.colorOnSurfaceVariant))



                    binding.layoutPresetActions.visibility = View.GONE

                    binding.layoutCustomActions.visibility = View.VISIBLE



                    binding.btnProfileShare.setOnClickListener { onShare(profile) }

                    binding.btnProfileRename.setOnClickListener { onRename(profile) }

                    binding.btnProfileDelete.setOnClickListener { onDelete(profile) }

                }



                // Bind parameter tags

                val tags = profile.tags

                val tagViews = listOf(

                    binding.tagProfile1,

                    binding.tagProfile2,

                    binding.tagProfile3,

                    binding.tagProfile4

                )

                for (i in tagViews.indices) {

                    if (i < tags.size) {

                        tagViews[i].text = tags[i]

                        tagViews[i].visibility = View.VISIBLE

                    } else {

                        tagViews[i].visibility = View.GONE

                    }

                }



                binding.btnProfileApply.setOnClickListener { onApply(profile) }

            }

        }

    }

}

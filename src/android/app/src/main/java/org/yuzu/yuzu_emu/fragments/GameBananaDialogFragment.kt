// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.app.Dialog
import android.os.Build
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.LinearLayout
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.view.isVisible
import androidx.fragment.app.DialogFragment
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.color.MaterialColors
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import org.yuzu.yuzu_emu.NativeLibrary
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.databinding.DialogGamebananaModsBinding
import org.yuzu.yuzu_emu.databinding.ListItemAddonBinding
import org.yuzu.yuzu_emu.databinding.ListItemGamebananaModBinding
import org.yuzu.yuzu_emu.model.Game
import org.yuzu.yuzu_emu.model.Patch
import org.yuzu.yuzu_emu.model.PatchType
import org.yuzu.yuzu_emu.utils.GameBananaFile
import org.yuzu.yuzu_emu.utils.GameBananaHelper
import org.yuzu.yuzu_emu.utils.GameBananaMod
import org.yuzu.yuzu_emu.utils.NativeConfig

class GameBananaDialogFragment : DialogFragment() {

    private var _binding: DialogGamebananaModsBinding? = null
    private val binding get() = _binding!!

    private var game: Game? = null
    private var onModsUpdated: (() -> Unit)? = null
    private val modsList = mutableListOf<GameBananaMod>()
    private val installedModsList = mutableListOf<Patch>()
    private var isInstalledView = false

    private var currentPage: Int = 1
    private var currentSortIndex: Int = 0

    private val installManualModFile =
        registerForActivityResult(ActivityResultContracts.OpenDocument()) { uri ->
            if (uri == null) return@registerForActivityResult
            val g = game ?: return@registerForActivityResult
            val progressDialog = MaterialAlertDialogBuilder(requireContext())
                .setTitle(getString(R.string.gamebanana_install_manual))
                .setMessage(getString(R.string.gamebanana_installing_manual))
                .setCancelable(false)
                .show()

            lifecycleScope.launch {
                val (success, modName) = GameBananaHelper.installManualMod(g, uri, requireContext())
                progressDialog.dismiss()
                if (success) {
                    Toast.makeText(
                        requireContext(),
                        getString(R.string.gamebanana_manual_success, modName),
                        Toast.LENGTH_LONG
                    ).show()
                    onModsUpdated?.invoke()
                    loadInstalledMods()
                } else {
                    Toast.makeText(
                        requireContext(),
                        getString(R.string.gamebanana_manual_error),
                        Toast.LENGTH_LONG
                    ).show()
                }
            }
        }

    private val installManualModFolder =
        registerForActivityResult(ActivityResultContracts.OpenDocumentTree()) { uri ->
            if (uri == null) return@registerForActivityResult
            val g = game ?: return@registerForActivityResult
            val progressDialog = MaterialAlertDialogBuilder(requireContext())
                .setTitle(getString(R.string.gamebanana_install_manual))
                .setMessage(getString(R.string.gamebanana_installing_manual))
                .setCancelable(false)
                .show()

            lifecycleScope.launch {
                val (success, modName) = GameBananaHelper.installManualModDirectory(g, uri, requireContext())
                progressDialog.dismiss()
                if (success) {
                    Toast.makeText(
                        requireContext(),
                        getString(R.string.gamebanana_manual_success, modName),
                        Toast.LENGTH_LONG
                    ).show()
                    onModsUpdated?.invoke()
                    loadInstalledMods()
                } else {
                    Toast.makeText(
                        requireContext(),
                        getString(R.string.gamebanana_manual_error),
                        Toast.LENGTH_LONG
                    ).show()
                }
            }
        }

    companion object {
        const val TAG = "GameBananaDialogFragment"
        private const val ARG_GAME = "arg_game"

        fun newInstance(game: Game, onModsUpdated: (() -> Unit)? = null): GameBananaDialogFragment {
            return GameBananaDialogFragment().apply {
                arguments = Bundle().apply {
                    putParcelable(ARG_GAME, game)
                }
                this.game = game
                this.onModsUpdated = onModsUpdated
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (game == null) {
            game = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                arguments?.getParcelable(ARG_GAME, Game::class.java)
            } else {
                @Suppress("DEPRECATION")
                arguments?.getParcelable(ARG_GAME)
            }
        }
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        _binding = DialogGamebananaModsBinding.inflate(layoutInflater)

        setupUI()
        performSearch(1)

        return MaterialAlertDialogBuilder(requireContext())
            .setTitle(getString(R.string.gamebanana_mods) + " - " + (game?.title ?: ""))
            .setView(binding.root)
            .create()
    }

    override fun onStart() {
        super.onStart()
        dialog?.window?.let { window ->
            val dm = resources.displayMetrics
            val isLandscape = resources.configuration.orientation == android.content.res.Configuration.ORIENTATION_LANDSCAPE
            val width = if (isLandscape) (dm.widthPixels * 0.92).toInt() else (dm.widthPixels * 0.95).toInt()
            val height = if (isLandscape) (dm.heightPixels * 0.92).toInt() else (dm.heightPixels * 0.85).toInt()
            window.setLayout(width, height)
            window.setBackgroundDrawableResource(R.drawable.eden_dialog_background)

            val customPanel = window.findViewById<View>(androidx.appcompat.R.id.customPanel)
            if (customPanel != null) {
                val params = customPanel.layoutParams
                if (params is LinearLayout.LayoutParams) {
                    params.weight = 1f
                    params.height = 0
                    customPanel.layoutParams = params
                }
            }

            val buttonPanel = window.findViewById<View>(androidx.appcompat.R.id.buttonPanel)
            buttonPanel?.visibility = View.GONE
        }
    }

    private fun setupUI() {
        val isLandscape = resources.configuration.orientation == android.content.res.Configuration.ORIENTATION_LANDSCAPE
        val spanCount = if (isLandscape) 2 else 1
        binding.listMods.layoutManager = androidx.recyclerview.widget.GridLayoutManager(requireContext(), spanCount)
        binding.listMods.adapter = GameBananaModAdapter()

        binding.buttonClose.setOnClickListener {
            dismiss()
        }

        // Sort spinner
        val sortOptions = arrayOf(
            getString(R.string.sort_downloads),
            getString(R.string.sort_likes),
            getString(R.string.sort_views),
            getString(R.string.sort_latest),
            getString(R.string.sort_name)
        )
        val spinnerAdapter = ArrayAdapter(
            requireContext(),
            android.R.layout.simple_spinner_dropdown_item,
            sortOptions
        )
        binding.spinnerSort.adapter = spinnerAdapter
        binding.spinnerSort.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                if (currentSortIndex != position) {
                    currentSortIndex = position
                    performSearch(1)
                }
            }
            override fun onNothingSelected(parent: AdapterView<*>?) {}
        }

        binding.buttonSearch.setOnClickListener {
            performSearch(1)
        }

        binding.inputSearch.setOnEditorActionListener { _, _, _ ->
            performSearch(1)
            true
        }

        binding.buttonFirstPage.setOnClickListener {
            if (currentPage > 1) performSearch(1)
        }

        binding.buttonPrevPage.setOnClickListener {
            if (currentPage > 1) performSearch(currentPage - 1)
        }

        binding.buttonNextPage.setOnClickListener {
            performSearch(currentPage + 1)
        }

        binding.buttonTabOnline.setOnClickListener {
            switchToOnlineTab()
        }

        binding.buttonTabInstalled.setOnClickListener {
            switchToInstalledTab()
        }

        binding.buttonCloseInstalled.setOnClickListener {
            dismiss()
        }

        binding.buttonRefreshInstalled.setOnClickListener {
            loadInstalledMods()
        }

        binding.buttonManualInstallInstalled.setOnClickListener {
            binding.buttonManualInstall.performClick()
        }

        binding.listInstalledMods.layoutManager = LinearLayoutManager(requireContext())
        binding.listInstalledMods.adapter = InstalledModAdapter()

        binding.buttonManualInstall.setOnClickListener {
            val options = arrayOf(
                getString(R.string.gamebanana_manual_source_zip),
                getString(R.string.gamebanana_manual_source_folder)
            )
            MaterialAlertDialogBuilder(requireContext())
                .setTitle(getString(R.string.gamebanana_install_manual))
                .setItems(options) { _, which ->
                    when (which) {
                        0 -> installManualModFile.launch(arrayOf("application/zip", "application/x-zip-compressed", "application/octet-stream", "*/*"))
                        1 -> installManualModFolder.launch(null)
                    }
                }
                .setNegativeButton(R.string.close, null)
                .show()
        }
    }

    private fun performSearch(page: Int) {
        val g = game ?: return
        val query = binding.inputSearch.text.toString().trim()
        currentPage = page

        binding.progressLoading.isVisible = true
        binding.textEmptyMods.isVisible = false
        binding.textStatus.text = getString(R.string.page_format, currentPage)
        binding.textPageIndicator.text = "$currentPage"
        binding.buttonFirstPage.isEnabled = (currentPage > 1)
        binding.buttonPrevPage.isEnabled = (currentPage > 1)

        lifecycleScope.launch {
            val results = GameBananaHelper.searchMods(
                game = g,
                query = query,
                page = currentPage,
                sortIndex = currentSortIndex
            )
            binding.progressLoading.isVisible = false
            modsList.clear()
            modsList.addAll(results)
            binding.listMods.adapter?.notifyDataSetChanged()
            binding.textEmptyMods.isVisible = results.isEmpty()
            binding.buttonNextPage.isEnabled = results.size >= 15
        }
    }

    private fun showModDetailsDialog(mod: GameBananaMod) {
        val g = game ?: return
        val loadingDialog = MaterialAlertDialogBuilder(requireContext())
            .setTitle(mod.name)
            .setMessage(getString(R.string.loading_mod_details))
            .setCancelable(true)
            .show()

        lifecycleScope.launch {
            val (desc, files) = GameBananaHelper.getModFiles(mod.id)
            loadingDialog.dismiss()

            if (files.isEmpty()) {
                Toast.makeText(requireContext(), R.string.gamebanana_no_files, Toast.LENGTH_SHORT).show()
                return@launch
            }

            val context = requireContext()
            val layout = android.widget.LinearLayout(context).apply {
                orientation = android.widget.LinearLayout.VERTICAL
                setPadding(48, 24, 48, 24)
            }

            // Info header
            val headerText = android.widget.TextView(context).apply {
                text = "${mod.category} • ${getString(R.string.gamebanana_author, mod.submitter)}\n" +
                       "📥 ${mod.downloads}  ❤️ ${mod.likes}  👁️ ${mod.views}"
                setTextColor(0xFF38BDF8.toInt())
                textSize = 13f
                setTypeface(null, android.graphics.Typeface.BOLD)
                setPadding(0, 0, 0, 16)
            }
            layout.addView(headerText)

            // Description ScrollView
            val cleanDesc = if (desc.isNotBlank()) {
                desc
            } else if (mod.description.isNotBlank()) {
                mod.description
            } else {
                "Описание мода отсутствует."
            }

            val descHeightPx = (150 * context.resources.displayMetrics.density).toInt()
            val bgDrawable = android.graphics.drawable.GradientDrawable().apply {
                setColor(0xFF131927.toInt())
                cornerRadius = 10 * context.resources.displayMetrics.density
                setStroke((1 * context.resources.displayMetrics.density).toInt(), 0xFF1E293B.toInt())
            }
            val scrollView = android.widget.ScrollView(context).apply {
                layoutParams = android.widget.LinearLayout.LayoutParams(
                    android.widget.LinearLayout.LayoutParams.MATCH_PARENT,
                    descHeightPx
                )
                background = bgDrawable
                setPadding(28, 20, 28, 20)
            }

            val descTextView = android.widget.TextView(context).apply {
                val formattedText = try {
                    if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N) {
                        android.text.Html.fromHtml(cleanDesc, android.text.Html.FROM_HTML_MODE_COMPACT)
                    } else {
                        @Suppress("DEPRECATION")
                        android.text.Html.fromHtml(cleanDesc)
                    }
                } catch (_: Exception) {
                    cleanDesc
                }
                text = formattedText
                setTextColor(0xFFE2E8F0.toInt())
                textSize = 13f
                movementMethod = android.text.method.LinkMovementMethod.getInstance()
            }
            scrollView.addView(descTextView)
            layout.addView(scrollView)

            // Files Header
            val filesHeader = android.widget.TextView(context).apply {
                text = "\n📦 Доступные файлы для скачивания:"
                setTextColor(0xFFFFCA28.toInt())
                textSize = 14f
                setTypeface(null, android.graphics.Typeface.BOLD)
                setPadding(0, 8, 0, 8)
            }
            layout.addView(filesHeader)

            // Files RadioGroup
            val radioGroup = android.widget.RadioGroup(context).apply {
                orientation = android.widget.LinearLayout.VERTICAL
            }

            files.forEachIndexed { index, file ->
                val sizeMb = if (file.filesize > 0) " (${file.filesize / 1024 / 1024} MB)" else ""
                val rb = android.widget.RadioButton(context).apply {
                    id = index
                    text = "${file.filename}$sizeMb\n${file.description.ifEmpty { mod.name }}"
                    setTextColor(0xFFF1F5F9.toInt())
                    textSize = 13f
                    isChecked = (index == 0)
                }
                radioGroup.addView(rb)
            }
            layout.addView(radioGroup)

            val outerScrollView = android.widget.ScrollView(context).apply {
                isFillViewport = true
            }
            outerScrollView.addView(layout)

            MaterialAlertDialogBuilder(context)
                .setTitle(mod.name)
                .setView(outerScrollView)
                .setPositiveButton(R.string.gamebanana_download_button) { _, _ ->
                    val selectedId = radioGroup.checkedRadioButtonId
                    val chosenIndex = if (selectedId in files.indices) selectedId else 0
                    val fileToDownload = files[chosenIndex]
                    startDownloadFile(mod, fileToDownload)
                }
                .setNegativeButton(R.string.close, null)
                .show()
        }
    }

    private fun startDownloadFile(mod: GameBananaMod, file: GameBananaFile) {
        val g = game ?: return
        val progressDialog = MaterialAlertDialogBuilder(requireContext())
            .setTitle(getString(R.string.installing_mod_title, mod.name))
            .setMessage(file.filename)
            .setCancelable(false)
            .show()

        lifecycleScope.launch {
            val success = GameBananaHelper.downloadAndInstallMod(
                game = g,
                modName = mod.name,
                file = file,
                onProgress = { progress ->
                    lifecycleScope.launch(Dispatchers.Main) {
                        progressDialog.setMessage("${file.filename}\n$progress%")
                    }
                }
            )

            progressDialog.dismiss()

            if (success) {
                Toast.makeText(
                    requireContext(),
                    getString(R.string.gamebanana_installed_success, mod.name),
                    Toast.LENGTH_LONG
                ).show()
                onModsUpdated?.invoke()
                loadInstalledMods()
            } else {
                Toast.makeText(
                    requireContext(),
                    R.string.gamebanana_install_error,
                    Toast.LENGTH_LONG
                ).show()
            }
        }
    }

    private inner class GameBananaModAdapter :
        RecyclerView.Adapter<GameBananaModAdapter.ModViewHolder>() {

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ModViewHolder {
            val binding = ListItemGamebananaModBinding.inflate(
                LayoutInflater.from(parent.context),
                parent,
                false
            )
            return ModViewHolder(binding)
        }

        override fun onBindViewHolder(holder: ModViewHolder, position: Int) {
            holder.bind(modsList[position])
        }

        override fun getItemCount(): Int = modsList.size

        inner class ModViewHolder(private val itemBinding: ListItemGamebananaModBinding) :
            RecyclerView.ViewHolder(itemBinding.root) {

            fun bind(mod: GameBananaMod) {
                itemBinding.textModTitle.text = mod.name
                val authorInfo = if (mod.submitter.isNotEmpty()) {
                    getString(R.string.gamebanana_author, mod.submitter) + " • " +
                            getString(R.string.gamebanana_downloads, mod.downloads) + " • " +
                            getString(R.string.gamebanana_likes, mod.likes)
                } else {
                    getString(R.string.gamebanana_downloads, mod.downloads)
                }
                itemBinding.textModAuthor.text = authorInfo
                itemBinding.buttonDownloadMod.isEnabled = true
                itemBinding.progressModDownload.isVisible = false

                itemBinding.buttonDownloadMod.setOnClickListener {
                    showModDetailsDialog(mod)
                }

                itemView.setOnClickListener {
                    showModDetailsDialog(mod)
                }
            }
        }
    }

    private fun switchToOnlineTab() {
        isInstalledView = false
        binding.layoutOnlineContainer.isVisible = true
        binding.layoutInstalledContainer.isVisible = false
        try {
            binding.buttonTabOnline.setBackgroundColor(
                MaterialColors.getColor(binding.buttonTabOnline, com.google.android.material.R.attr.colorPrimary)
            )
            binding.buttonTabOnline.setTextColor(
                MaterialColors.getColor(binding.buttonTabOnline, com.google.android.material.R.attr.colorOnPrimary)
            )
            binding.buttonTabInstalled.setBackgroundColor(android.graphics.Color.TRANSPARENT)
            binding.buttonTabInstalled.setTextColor(
                MaterialColors.getColor(binding.buttonTabInstalled, com.google.android.material.R.attr.colorPrimary)
            )
        } catch (_: Exception) {}
    }

    private fun switchToInstalledTab() {
        isInstalledView = true
        binding.layoutOnlineContainer.isVisible = false
        binding.layoutInstalledContainer.isVisible = true
        try {
            binding.buttonTabInstalled.setBackgroundColor(
                MaterialColors.getColor(binding.buttonTabInstalled, com.google.android.material.R.attr.colorPrimary)
            )
            binding.buttonTabInstalled.setTextColor(
                MaterialColors.getColor(binding.buttonTabInstalled, com.google.android.material.R.attr.colorOnPrimary)
            )
            binding.buttonTabOnline.setBackgroundColor(android.graphics.Color.TRANSPARENT)
            binding.buttonTabOnline.setTextColor(
                MaterialColors.getColor(binding.buttonTabOnline, com.google.android.material.R.attr.colorPrimary)
            )
        } catch (_: Exception) {}
        loadInstalledMods()
    }

    private fun loadInstalledMods() {
        val g = game ?: return
        lifecycleScope.launch {
            val patches = withContext(Dispatchers.IO) {
                NativeLibrary.getPatchesForFile(g.path, g.programId)
            } ?: emptyArray()

            val mods = patches.filter { PatchType.from(it.type) == PatchType.Mod }
            installedModsList.clear()
            installedModsList.addAll(mods)

            binding.listInstalledMods.adapter?.notifyDataSetChanged()
            binding.textEmptyInstalled.isVisible = installedModsList.isEmpty()

            val enabledCount = installedModsList.count { it.enabled }
            val disabledCount = installedModsList.size - enabledCount
            if (installedModsList.isEmpty()) {
                binding.textInstalledStatus.text = getString(R.string.gamebanana_no_installed_mods)
            } else {
                binding.textInstalledStatus.text = getString(
                    R.string.gamebanana_installed_stats,
                    installedModsList.size,
                    enabledCount,
                    disabledCount
                )
            }
        }
    }

    private inner class InstalledModAdapter :
        RecyclerView.Adapter<InstalledModAdapter.InstalledModViewHolder>() {

        inner class InstalledModViewHolder(val itemBinding: ListItemAddonBinding) :
            RecyclerView.ViewHolder(itemBinding.root)

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): InstalledModViewHolder {
            val view = ListItemAddonBinding.inflate(
                LayoutInflater.from(parent.context),
                parent,
                false
            )
            return InstalledModViewHolder(view)
        }

        override fun getItemCount(): Int = installedModsList.size

        override fun onBindViewHolder(holder: InstalledModViewHolder, position: Int) {
            val patch = installedModsList[position]
            holder.itemBinding.title.text = patch.name
            val hasVer = patch.version.isNotEmpty() && patch.version != "0"
            holder.itemBinding.version.text = if (hasVer) "v${patch.version}" else ""
            holder.itemBinding.version.isVisible = hasVer

            holder.itemBinding.addonSwitch.setOnCheckedChangeListener(null)
            holder.itemBinding.addonSwitch.isChecked = patch.enabled

            holder.itemBinding.addonSwitch.setOnCheckedChangeListener { _, isChecked ->
                patch.enabled = isChecked
                saveAddonsState()
                val enabledCount = installedModsList.count { it.enabled }
                val disabledCount = installedModsList.size - enabledCount
                binding.textInstalledStatus.text = getString(
                    R.string.gamebanana_installed_stats,
                    installedModsList.size,
                    enabledCount,
                    disabledCount
                )
                onModsUpdated?.invoke()
            }

            holder.itemBinding.addonCard.setOnClickListener {
                holder.itemBinding.addonSwitch.toggle()
            }

            holder.itemBinding.deleteCard.isVisible = patch.isRemovable
            holder.itemBinding.buttonDelete.setOnClickListener {
                MaterialAlertDialogBuilder(requireContext())
                    .setTitle(R.string.delete)
                    .setMessage(getString(R.string.gamebanana_delete_mod_confirm, patch.name))
                    .setPositiveButton(R.string.delete) { _, _ ->
                        NativeLibrary.removeMod(patch.programId, patch.name)
                        Toast.makeText(
                            requireContext(),
                            getString(R.string.gamebanana_mod_deleted, patch.name),
                            Toast.LENGTH_SHORT
                        ).show()
                        loadInstalledMods()
                        onModsUpdated?.invoke()
                    }
                    .setNegativeButton(R.string.close, null)
                    .show()
            }
        }

        private fun saveAddonsState() {
            val g = game ?: return
            val currentPatches = NativeLibrary.getPatchesForFile(g.path, g.programId) ?: return
            val disabledAddons = mutableListOf<String>()
            for (p in currentPatches) {
                if (PatchType.from(p.type) == PatchType.Mod) {
                    val matching = installedModsList.find { it.name == p.name }
                    if (matching != null && !matching.enabled) {
                        disabledAddons.add(p.name)
                    }
                } else if (!p.enabled) {
                    disabledAddons.add(p.name)
                }
            }
            NativeConfig.setDisabledAddons(g.programId, disabledAddons.toTypedArray())
            NativeConfig.saveGlobalConfig()
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}

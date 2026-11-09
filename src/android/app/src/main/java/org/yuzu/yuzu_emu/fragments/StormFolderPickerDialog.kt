// SPDX-FileCopyrightText: Copyright 2026 STORM SWITCH Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.app.Dialog
import android.content.Intent
import android.content.res.ColorStateList
import android.graphics.Color
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.Settings
import android.text.TextUtils
import android.util.TypedValue
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.widget.HorizontalScrollView
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.TextView
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.content.ContextCompat
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.FragmentManager
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.button.MaterialButton
import com.google.android.material.checkbox.MaterialCheckBox
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import com.google.android.material.materialswitch.MaterialSwitch
import org.yuzu.yuzu_emu.R
import java.io.File
import java.util.Locale

class StormFolderPickerDialog : DialogFragment() {

    var onFolderPicked: ((Uri) -> Unit)? = null
    var onFoldersPicked: ((List<Uri>, Boolean) -> Unit)? = null
    var onRemoveExistingUri: ((String) -> Unit)? = null
    var onSystemPickerRequested: (() -> Unit)? = null

    private var allowMultiple: Boolean = false
    private val selectedPaths = LinkedHashSet<String>()
    private val existingUriMap = LinkedHashMap<String, String>() // normalizedPath -> rawUriString

    private var currentDir: File = Environment.getExternalStorageDirectory()
    private var rootMinDir: File = Environment.getExternalStorageDirectory()
    private lateinit var pathTextView: TextView
    private var addCurrentBtn: MaterialButton? = null
    private var selectedSection: LinearLayout? = null
    private var selectedHeaderText: TextView? = null
    private var selectedChipsContainer: LinearLayout? = null
    private var deepScanSwitch: MaterialSwitch? = null
    private lateinit var selectBtn: MaterialButton
    private lateinit var emptyTextView: TextView
    private lateinit var permissionBanner: LinearLayout
    private lateinit var recyclerView: RecyclerView
    private lateinit var adapter: FolderEntryAdapter

    private val romExtensions = setOf("nsp", "nsz", "xci", "xcz", "nro", "nca", "kip", "nup")

    private val permissionLauncher = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) {
        refreshDirectory()
    }

    override fun onResume() {
        super.onResume()
        if (::adapter.isInitialized) {
            refreshDirectory()
        }
    }

    private fun normalizePathFromUriString(uriStr: String): String {
        val parsed = runCatching { Uri.parse(uriStr) }.getOrNull()
        if (parsed != null && parsed.scheme == "file") {
            val p = parsed.path ?: Uri.decode(uriStr.removePrefix("file://"))
            return p.trimEnd('/').ifEmpty { "/" }
        }
        val pathPart = parsed?.path?.let { runCatching { Uri.decode(it) }.getOrDefault(it) } ?: uriStr
        if (pathPart.contains("/tree/primary:")) {
            val rel = pathPart.substringAfter("/tree/primary:").trim('/')
            val ext = Environment.getExternalStorageDirectory().absolutePath.trimEnd('/')
            return if (rel.isEmpty()) ext else "$ext/$rel"
        }
        return pathPart.trimEnd('/').ifEmpty { uriStr }
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val ctx = requireContext()
        val density = resources.displayMetrics.density
        fun dp(v: Float): Int = (v * density).toInt()

        allowMultiple = arguments?.getBoolean(ARG_ALLOW_MULTIPLE, false) ?: false
        arguments?.getStringArrayList(ARG_EXISTING_URIS)?.forEach { rawUri ->
            if (rawUri.isNotBlank()) {
                existingUriMap[normalizePathFromUriString(rawUri)] = rawUri
            }
        }

        val initialPath = arguments?.getString(ARG_INITIAL_PATH)
        val startFile = if (!initialPath.isNullOrBlank()) File(initialPath) else Environment.getExternalStorageDirectory()
        val extRoot = Environment.getExternalStorageDirectory()
        currentDir = if (startFile.exists() && startFile.isDirectory) startFile else extRoot
        rootMinDir = detectRootFor(currentDir)

        val surfaceColor = resolveAttrColor(com.google.android.material.R.attr.colorSurface, 0xFF121826.toInt())
        val surfaceVarColor = resolveAttrColor(com.google.android.material.R.attr.colorSurfaceVariant, 0xFF1B2436.toInt())
        val primaryColor = resolveAttrColor(com.google.android.material.R.attr.colorPrimary, 0xFF38BDF8.toInt())
        val onSurfaceColor = resolveAttrColor(com.google.android.material.R.attr.colorOnSurface, Color.WHITE)
        val onSurfaceVarColor = resolveAttrColor(com.google.android.material.R.attr.colorOnSurfaceVariant, 0xFF94A3B8.toInt())
        val emeraldColor = 0xFF10B981.toInt()

        val rootLayout = LinearLayout(ctx).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(16f), dp(14f), dp(16f), dp(12f))
        }

        // Title
        val titleStr = arguments?.getString(ARG_TITLE)?.takeIf { it.isNotBlank() }
            ?: getString(R.string.storm_folder_picker_title)
        val titleView = TextView(ctx).apply {
            text = titleStr
            setTextColor(onSurfaceColor)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 17.5f)
            setTypeface(typeface, Typeface.BOLD)
            setPadding(0, 0, 0, dp(8f))
        }
        rootLayout.addView(titleView)

        // Storage roots switcher row (Internal Storage + any SD cards)
        val roots = discoverStorageRoots()
        if (roots.isNotEmpty()) {
            val hsv = HorizontalScrollView(ctx).apply {
                isHorizontalScrollBarEnabled = false
                setPadding(0, 0, 0, dp(6f))
            }
            val chipsRow = LinearLayout(ctx).apply {
                orientation = LinearLayout.HORIZONTAL
            }
            roots.forEach { (label, rootFile) ->
                val btn = MaterialButton(ctx).apply {
                    text = label
                    isAllCaps = false
                    strokeWidth = 0
                    insetTop = 0
                    insetBottom = 0
                    cornerRadius = dp(10f)
                    minHeight = dp(34f)
                    minimumHeight = dp(34f)
                    setTextSize(TypedValue.COMPLEX_UNIT_SP, 12f)
                    setPadding(dp(12f), 0, dp(12f), 0)
                    backgroundTintList = ColorStateList.valueOf(surfaceVarColor)
                    setTextColor(onSurfaceColor)
                    layoutParams = LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.WRAP_CONTENT,
                        dp(34f)
                    ).apply { marginEnd = dp(8f) }
                    setOnClickListener {
                        rootMinDir = rootFile
                        currentDir = rootFile
                        refreshDirectory()
                    }
                }
                chipsRow.addView(btn)
            }
            hsv.addView(chipsRow)
            rootLayout.addView(hsv)
        }

        // Current path row + "+ Current" toggle button in multi-select mode
        val pathRow = LinearLayout(ctx).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { bottomMargin = dp(8f) }
        }

        pathTextView = TextView(ctx).apply {
            setTextColor(primaryColor)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 12f)
            setTypeface(Typeface.MONOSPACE, Typeface.BOLD)
            maxLines = 2
            ellipsize = TextUtils.TruncateAt.START
            setPadding(dp(10f), dp(7f), dp(10f), dp(7f))
            background = GradientDrawable().apply {
                shape = GradientDrawable.RECTANGLE
                cornerRadius = dp(10f).toFloat()
                setColor(surfaceVarColor)
            }
            layoutParams = LinearLayout.LayoutParams(
                0,
                ViewGroup.LayoutParams.WRAP_CONTENT,
                1f
            )
        }
        pathRow.addView(pathTextView)

        if (allowMultiple) {
            addCurrentBtn = MaterialButton(ctx).apply {
                text = getString(R.string.storm_folder_picker_add_current)
                isAllCaps = false
                strokeWidth = 0
                insetTop = 0
                insetBottom = 0
                cornerRadius = dp(10f)
                minHeight = dp(34f)
                minimumHeight = dp(34f)
                setPadding(dp(10f), 0, dp(10f), 0)
                setTextSize(TypedValue.COMPLEX_UNIT_SP, 11.5f)
                setTypeface(typeface, Typeface.BOLD)
                backgroundTintList = ColorStateList.valueOf(surfaceVarColor)
                setTextColor(primaryColor)
                layoutParams = LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                    dp(34f)
                ).apply { marginStart = dp(6f) }
                setOnClickListener {
                    val normCur = currentDir.absolutePath.trimEnd('/').ifEmpty { "/" }
                    if (existingUriMap.containsKey(normCur)) {
                        val raw = existingUriMap.remove(normCur)
                        if (raw != null) onRemoveExistingUri?.invoke(raw)
                    } else if (selectedPaths.contains(normCur)) {
                        selectedPaths.remove(normCur)
                    } else {
                        selectedPaths.add(normCur)
                    }
                    updateMultiSelectionUi(surfaceVarColor, primaryColor, emeraldColor, onSurfaceColor, onSurfaceVarColor)
                    adapter.notifyDataSetChanged()
                }
            }
            pathRow.addView(addCurrentBtn)
        }
        rootLayout.addView(pathRow)

        // Multi-folder Selected & Already-Added strip
        if (allowMultiple) {
            val sec = LinearLayout(ctx).apply {
                orientation = LinearLayout.VERTICAL
                visibility = View.GONE
                layoutParams = LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT
                ).apply { bottomMargin = dp(8f) }
            }
            val hdr = TextView(ctx).apply {
                setTextColor(onSurfaceVarColor)
                setTextSize(TypedValue.COMPLEX_UNIT_SP, 11.5f)
                setTypeface(typeface, Typeface.BOLD)
                setPadding(dp(2f), 0, dp(2f), dp(4f))
            }
            val chipsScroll = HorizontalScrollView(ctx).apply {
                isHorizontalScrollBarEnabled = false
            }
            val chipsRow = LinearLayout(ctx).apply {
                orientation = LinearLayout.HORIZONTAL
                gravity = Gravity.CENTER_VERTICAL
            }
            chipsScroll.addView(chipsRow)
            sec.addView(hdr)
            sec.addView(chipsScroll)

            selectedSection = sec
            selectedHeaderText = hdr
            selectedChipsContainer = chipsRow
            rootLayout.addView(sec)
        }

        // All-Files-Access prompt banner (visible only if permission is missing)
        permissionBanner = LinearLayout(ctx).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(dp(12f), dp(10f), dp(12f), dp(10f))
            background = GradientDrawable().apply {
                shape = GradientDrawable.RECTANGLE
                cornerRadius = dp(10f).toFloat()
                setColor(0x33F59E0B)
            }
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { bottomMargin = dp(8f) }
            visibility = View.GONE

            val msg = TextView(ctx).apply {
                text = getString(R.string.storm_folder_picker_grant_permission)
                setTextColor(onSurfaceColor)
                setTextSize(TypedValue.COMPLEX_UNIT_SP, 12f)
                layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f).apply {
                    marginEnd = dp(8f)
                }
            }
            val grantBtn = MaterialButton(ctx).apply {
                text = getString(R.string.storm_folder_picker_grant_btn)
                isAllCaps = false
                strokeWidth = 0
                insetTop = 0
                insetBottom = 0
                cornerRadius = dp(8f)
                minHeight = dp(36f)
                setTextSize(TypedValue.COMPLEX_UNIT_SP, 12f)
                backgroundTintList = ColorStateList.valueOf(primaryColor)
                setTextColor(Color.WHITE)
                setOnClickListener { requestAllFilesAccess() }
            }
            addView(msg)
            addView(grantBtn)
        }
        rootLayout.addView(permissionBanner)

        // Folder list RecyclerView
        val screenH = resources.displayMetrics.heightPixels
        val heightRatio = if (allowMultiple) 0.40f else 0.46f
        val listHeight = (screenH * heightRatio).toInt().coerceIn(dp(190f), dp(390f))

        adapter = FolderEntryAdapter(
            rowBgColor = surfaceVarColor,
            iconTintColor = primaryColor,
            emeraldColor = emeraldColor,
            titleColor = onSurfaceColor,
            subColor = onSurfaceVarColor,
            onNavigate = { clickedEntry ->
                currentDir = clickedEntry.file
                refreshDirectory()
            },
            onToggleSelect = { toggledEntry ->
                val norm = toggledEntry.file.absolutePath.trimEnd('/').ifEmpty { "/" }
                if (existingUriMap.containsKey(norm)) {
                    val raw = existingUriMap.remove(norm)
                    if (raw != null) onRemoveExistingUri?.invoke(raw)
                } else if (selectedPaths.contains(norm)) {
                    selectedPaths.remove(norm)
                } else {
                    selectedPaths.add(norm)
                }
                updateMultiSelectionUi(surfaceVarColor, primaryColor, emeraldColor, onSurfaceColor, onSurfaceVarColor)
                adapter.notifyDataSetChanged()
            }
        )

        recyclerView = RecyclerView(ctx).apply {
            layoutManager = LinearLayoutManager(ctx)
            this.adapter = this@StormFolderPickerDialog.adapter
            clipToPadding = false
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                listHeight
            )
        }
        rootLayout.addView(recyclerView)

        emptyTextView = TextView(ctx).apply {
            text = getString(R.string.storm_folder_picker_empty)
            setTextColor(onSurfaceVarColor)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 12f)
            gravity = Gravity.CENTER
            setPadding(dp(10f), dp(10f), dp(10f), dp(10f))
            visibility = View.GONE
        }
        rootLayout.addView(emptyTextView)

        // Deep scan switch in multi-folder mode
        if (allowMultiple) {
            val sw = MaterialSwitch(ctx).apply {
                text = getString(R.string.storm_folder_picker_deep_scan)
                isChecked = true
                setTextColor(onSurfaceColor)
                setTextSize(TypedValue.COMPLEX_UNIT_SP, 12.5f)
                layoutParams = LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT
                ).apply { topMargin = dp(6f) }
            }
            deepScanSwitch = sw
            rootLayout.addView(sw)
        }

        // Bottom Action Bar
        val actionsRow = LinearLayout(ctx).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.END or Gravity.CENTER_VERTICAL
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { topMargin = dp(10f) }
        }

        if (onSystemPickerRequested != null) {
            val sysBtn = MaterialButton(ctx).apply {
                text = getString(R.string.storm_folder_picker_system_saf)
                isAllCaps = false
                strokeWidth = 0
                insetTop = 0
                insetBottom = 0
                cornerRadius = dp(10f)
                minHeight = dp(40f)
                backgroundTintList = ColorStateList.valueOf(surfaceVarColor)
                setTextColor(onSurfaceVarColor)
                setTextSize(TypedValue.COMPLEX_UNIT_SP, 12f)
                layoutParams = LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                    dp(40f)
                ).apply { marginEnd = dp(6f) }
                setOnClickListener {
                    val sysCb = onSystemPickerRequested
                    // Save any already-ticked folders before launching system SAF picker
                    if (allowMultiple && selectedPaths.isNotEmpty()) {
                        val uris = selectedPaths.map { Uri.fromFile(File(it)) }
                        val ds = deepScanSwitch?.isChecked ?: true
                        onFoldersPicked?.invoke(uris, ds)
                    }
                    dismissAllowingStateLoss()
                    sysCb?.invoke()
                }
            }
            actionsRow.addView(sysBtn)
        }

        val spacer = View(ctx).apply {
            layoutParams = LinearLayout.LayoutParams(0, 1, 1f)
        }
        actionsRow.addView(spacer)

        val cancelBtn = MaterialButton(ctx).apply {
            text = getString(android.R.string.cancel)
            isAllCaps = false
            strokeWidth = 0
            insetTop = 0
            insetBottom = 0
            cornerRadius = dp(10f)
            minHeight = dp(40f)
            backgroundTintList = ColorStateList.valueOf(surfaceVarColor)
            setTextColor(onSurfaceColor)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 12.5f)
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                dp(40f)
            ).apply { marginEnd = dp(8f) }
            setOnClickListener { dismissAllowingStateLoss() }
        }
        actionsRow.addView(cancelBtn)

        selectBtn = MaterialButton(ctx).apply {
            text = getString(R.string.storm_folder_picker_select_current)
            isAllCaps = false
            strokeWidth = 0
            insetTop = 0
            insetBottom = 0
            cornerRadius = dp(10f)
            minHeight = dp(40f)
            backgroundTintList = ColorStateList.valueOf(primaryColor)
            setTextColor(Color.WHITE)
            setTypeface(typeface, Typeface.BOLD)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 13f)
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                dp(40f)
            )
            setOnClickListener {
                if (allowMultiple) {
                    val ds = deepScanSwitch?.isChecked ?: true
                    val urisToDeliver = if (selectedPaths.isNotEmpty()) {
                        selectedPaths.map { Uri.fromFile(File(it)) }
                    } else {
                        val normCur = currentDir.absolutePath.trimEnd('/').ifEmpty { "/" }
                        if (existingUriMap.containsKey(normCur)) emptyList()
                        else listOf(Uri.fromFile(currentDir))
                    }
                    val multiCb = onFoldersPicked
                    val singleCb = onFolderPicked
                    dismissAllowingStateLoss()
                    if (multiCb != null) {
                        if (urisToDeliver.isNotEmpty()) {
                            multiCb.invoke(urisToDeliver, ds)
                        }
                    } else if (singleCb != null && urisToDeliver.isNotEmpty()) {
                        singleCb.invoke(urisToDeliver.first())
                    }
                } else {
                    val chosenUri = Uri.fromFile(currentDir)
                    val cb = onFolderPicked
                    dismissAllowingStateLoss()
                    cb?.invoke(chosenUri)
                }
            }
        }
        actionsRow.addView(selectBtn)
        rootLayout.addView(actionsRow)

        refreshDirectory()

        val dlg = MaterialAlertDialogBuilder(ctx, R.style.EdenMaterialDialog)
            .setView(rootLayout)
            .create()
        dlg.window?.setBackgroundDrawable(GradientDrawable().apply {
            shape = GradientDrawable.RECTANGLE
            cornerRadius = dp(20f).toFloat()
            setColor(surfaceColor)
        })
        return dlg
    }

    private fun updateMultiSelectionUi(
        surfaceVarColor: Int,
        primaryColor: Int,
        emeraldColor: Int,
        onSurfaceColor: Int,
        onSurfaceVarColor: Int
    ) {
        if (!allowMultiple) return
        val ctx = context ?: return
        val density = resources.displayMetrics.density
        fun dp(v: Float): Int = (v * density).toInt()

        val normCur = currentDir.absolutePath.trimEnd('/').ifEmpty { "/" }
        val curChecked = existingUriMap.containsKey(normCur) || selectedPaths.contains(normCur)
        addCurrentBtn?.apply {
            text = if (curChecked) "✓" else getString(R.string.storm_folder_picker_add_current)
            setTextColor(if (curChecked) emeraldColor else primaryColor)
        }

        val totalActive = existingUriMap.size + selectedPaths.size
        val sec = selectedSection
        val hdr = selectedHeaderText
        val chipsRow = selectedChipsContainer
        if (sec != null && hdr != null && chipsRow != null) {
            if (totalActive == 0) {
                sec.visibility = View.GONE
            } else {
                sec.visibility = View.VISIBLE
                hdr.text = getString(R.string.storm_folder_picker_selected_header, totalActive)
                chipsRow.removeAllViews()

                fun addChip(displayPath: String, isSaved: Boolean, onRemove: () -> Unit) {
                    val shortLabel = displayPath.trimEnd('/').substringAfterLast('/').ifEmpty { displayPath }
                    val chip = LinearLayout(ctx).apply {
                        orientation = LinearLayout.HORIZONTAL
                        gravity = Gravity.CENTER_VERTICAL
                        setPadding(dp(10f), dp(4f), dp(8f), dp(4f))
                        background = GradientDrawable().apply {
                            shape = GradientDrawable.RECTANGLE
                            cornerRadius = dp(8f).toFloat()
                            setColor(surfaceVarColor)
                        }
                        layoutParams = LinearLayout.LayoutParams(
                            ViewGroup.LayoutParams.WRAP_CONTENT,
                            ViewGroup.LayoutParams.WRAP_CONTENT
                        ).apply { marginEnd = dp(6f) }
                    }
                    val tv = TextView(ctx).apply {
                        text = shortLabel
                        setTextColor(if (isSaved) emeraldColor else primaryColor)
                        setTextSize(TypedValue.COMPLEX_UNIT_SP, 11.5f)
                        setTypeface(typeface, Typeface.BOLD)
                        maxLines = 1
                    }
                    val closeTv = TextView(ctx).apply {
                        text = "×"
                        setTextColor(onSurfaceVarColor)
                        setTextSize(TypedValue.COMPLEX_UNIT_SP, 14f)
                        setTypeface(typeface, Typeface.BOLD)
                        setPadding(dp(6f), 0, dp(2f), 0)
                        setOnClickListener { onRemove() }
                    }
                    chip.setOnClickListener { onRemove() }
                    chip.addView(tv)
                    chip.addView(closeTv)
                    chipsRow.addView(chip)
                }

                existingUriMap.entries.toList().forEach { (norm, rawUri) ->
                    addChip(norm, isSaved = true) {
                        existingUriMap.remove(norm)
                        onRemoveExistingUri?.invoke(rawUri)
                        updateMultiSelectionUi(surfaceVarColor, primaryColor, emeraldColor, onSurfaceColor, onSurfaceVarColor)
                        adapter.notifyDataSetChanged()
                    }
                }
                selectedPaths.toList().forEach { norm ->
                    addChip(norm, isSaved = false) {
                        selectedPaths.remove(norm)
                        updateMultiSelectionUi(surfaceVarColor, primaryColor, emeraldColor, onSurfaceColor, onSurfaceVarColor)
                        adapter.notifyDataSetChanged()
                    }
                }
            }
        }

        if (::selectBtn.isInitialized) {
            if (selectedPaths.isNotEmpty()) {
                selectBtn.text = getString(R.string.storm_folder_picker_select_multiple, selectedPaths.size)
            } else if (existingUriMap.isNotEmpty() && existingUriMap.containsKey(normCur)) {
                selectBtn.text = getString(android.R.string.ok)
            } else {
                selectBtn.text = getString(R.string.storm_folder_picker_select_current)
            }
        }
    }

    private fun hasAllFilesAccess(): Boolean {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            Environment.isExternalStorageManager()
        } else {
            true
        }
    }

    private fun requestAllFilesAccess() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            try {
                val intent = Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION).apply {
                    data = Uri.fromParts("package", requireContext().packageName, null)
                }
                permissionLauncher.launch(intent)
            } catch (_: Exception) {
                try {
                    permissionLauncher.launch(Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION))
                } catch (_: Exception) {}
            }
        }
    }

    private fun discoverStorageRoots(): List<Pair<String, File>> {
        val result = mutableListOf<Pair<String, File>>()
        val primary = Environment.getExternalStorageDirectory()
        result.add(getString(R.string.storm_folder_picker_internal) to primary)

        runCatching {
            val storageParent = File("/storage")
            storageParent.listFiles()?.forEach { vol ->
                val name = vol.name
                if (vol.isDirectory && vol.canRead() &&
                    name != "emulated" && name != "self" && !name.startsWith(".")
                ) {
                    result.add(getString(R.string.storm_folder_picker_sd_card, name) to vol)
                }
            }
        }
        return result
    }

    private fun detectRootFor(dir: File): File {
        val abs = dir.absolutePath
        for ((_, root) in discoverStorageRoots()) {
            if (abs.startsWith(root.absolutePath)) return root
        }
        return Environment.getExternalStorageDirectory()
    }

    private fun refreshDirectory() {
        val permOk = hasAllFilesAccess()
        permissionBanner.visibility = if (permOk) View.GONE else View.VISIBLE

        pathTextView.text = currentDir.absolutePath
        val entries = mutableListOf<FolderRow>()

        val parent = currentDir.parentFile
        if (parent != null && currentDir.absolutePath != rootMinDir.absolutePath && parent.canRead()) {
            entries.add(FolderRow(parent, getString(R.string.storm_folder_picker_up), "", isParent = true))
        }

        val rawChildren = runCatching { currentDir.listFiles() }.getOrNull()
        val subDirs = rawChildren
            ?.filter { it.isDirectory && !it.name.startsWith(".") }
            ?.sortedBy { it.name.lowercase(Locale.getDefault()) }
            ?: emptyList()

        for (dir in subDirs) {
            val children = runCatching { dir.listFiles() }.getOrNull()
            var subCount = 0
            var romCount = 0
            children?.forEach { ch ->
                if (ch.isDirectory && !ch.name.startsWith(".")) {
                    subCount++
                } else if (ch.isFile) {
                    val ext = ch.extension.lowercase(Locale.ROOT)
                    if (romExtensions.contains(ext)) romCount++
                }
            }
            val subtitle = if (romCount > 0) {
                getString(R.string.storm_folder_picker_stats_games, subCount, romCount)
            } else {
                getString(R.string.storm_folder_picker_stats_folders, subCount)
            }
            entries.add(FolderRow(dir, dir.name, subtitle, isParent = false))
        }

        adapter.submit(entries)
        emptyTextView.visibility = if (subDirs.isEmpty() && permOk) View.VISIBLE else View.GONE

        if (allowMultiple) {
            val surfaceVarColor = resolveAttrColor(com.google.android.material.R.attr.colorSurfaceVariant, 0xFF1B2436.toInt())
            val primaryColor = resolveAttrColor(com.google.android.material.R.attr.colorPrimary, 0xFF38BDF8.toInt())
            val onSurfaceColor = resolveAttrColor(com.google.android.material.R.attr.colorOnSurface, Color.WHITE)
            val onSurfaceVarColor = resolveAttrColor(com.google.android.material.R.attr.colorOnSurfaceVariant, 0xFF94A3B8.toInt())
            updateMultiSelectionUi(surfaceVarColor, primaryColor, 0xFF10B981.toInt(), onSurfaceColor, onSurfaceVarColor)
        }
    }

    private fun resolveAttrColor(attr: Int, fallback: Int): Int {
        val tv = TypedValue()
        return if (requireContext().theme.resolveAttribute(attr, tv, true)) {
            if (tv.type >= TypedValue.TYPE_FIRST_COLOR_INT && tv.type <= TypedValue.TYPE_LAST_COLOR_INT) {
                tv.data
            } else {
                ContextCompat.getColor(requireContext(), tv.resourceId)
            }
        } else {
            fallback
        }
    }

    private data class FolderRow(
        val file: File,
        val title: String,
        val subtitle: String,
        val isParent: Boolean
    )

    private inner class FolderEntryAdapter(
        private val rowBgColor: Int,
        private val iconTintColor: Int,
        private val emeraldColor: Int,
        private val titleColor: Int,
        private val subColor: Int,
        private val onNavigate: (FolderRow) -> Unit,
        private val onToggleSelect: (FolderRow) -> Unit
    ) : RecyclerView.Adapter<FolderEntryAdapter.VH>() {

        private val items = ArrayList<FolderRow>()

        fun submit(newItems: List<FolderRow>) {
            items.clear()
            items.addAll(newItems)
            notifyDataSetChanged()
        }

        inner class VH(
            val container: LinearLayout,
            val icon: ImageView,
            val title: TextView,
            val subtitle: TextView,
            val checkBox: MaterialCheckBox
        ) : RecyclerView.ViewHolder(container)

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): VH {
            val ctx = parent.context
            val density = ctx.resources.displayMetrics.density
            fun dp(v: Float): Int = (v * density).toInt()

            val row = LinearLayout(ctx).apply {
                orientation = LinearLayout.HORIZONTAL
                gravity = Gravity.CENTER_VERTICAL
                setPadding(dp(12f), dp(9f), dp(10f), dp(9f))
                isClickable = true
                isFocusable = true
                background = GradientDrawable().apply {
                    shape = GradientDrawable.RECTANGLE
                    cornerRadius = dp(12f).toFloat()
                    setColor(rowBgColor)
                }
                layoutParams = RecyclerView.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT
                ).apply { bottomMargin = dp(6f) }
            }

            val icon = ImageView(ctx).apply {
                setImageResource(R.drawable.ic_folder_open)
                imageTintList = ColorStateList.valueOf(iconTintColor)
                layoutParams = LinearLayout.LayoutParams(dp(24f), dp(24f)).apply {
                    marginEnd = dp(10f)
                }
            }

            val textCol = LinearLayout(ctx).apply {
                orientation = LinearLayout.VERTICAL
                layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f)
            }

            val title = TextView(ctx).apply {
                setTextColor(titleColor)
                setTextSize(TypedValue.COMPLEX_UNIT_SP, 14f)
                setTypeface(typeface, Typeface.BOLD)
                maxLines = 1
                ellipsize = TextUtils.TruncateAt.END
            }

            val subtitle = TextView(ctx).apply {
                setTextColor(subColor)
                setTextSize(TypedValue.COMPLEX_UNIT_SP, 11.5f)
                maxLines = 1
                ellipsize = TextUtils.TruncateAt.END
            }

            val checkBox = MaterialCheckBox(ctx).apply {
                minWidth = dp(32f)
                minHeight = dp(32f)
                minimumWidth = dp(32f)
                minimumHeight = dp(32f)
                buttonTintList = ColorStateList.valueOf(iconTintColor)
                layoutParams = LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT
                ).apply { marginStart = dp(6f) }
            }

            textCol.addView(title)
            textCol.addView(subtitle)
            row.addView(icon)
            row.addView(textCol)
            row.addView(checkBox)
            return VH(row, icon, title, subtitle, checkBox)
        }

        override fun onBindViewHolder(holder: VH, position: Int) {
            val item = items[position]
            holder.title.text = item.title
            if (item.subtitle.isBlank()) {
                holder.subtitle.visibility = View.GONE
            } else {
                holder.subtitle.visibility = View.VISIBLE
                holder.subtitle.text = item.subtitle
            }

            if (allowMultiple && !item.isParent) {
                val norm = item.file.absolutePath.trimEnd('/').ifEmpty { "/" }
                val isSaved = existingUriMap.containsKey(norm)
                val isSelected = selectedPaths.contains(norm)
                holder.checkBox.visibility = View.VISIBLE
                holder.checkBox.setOnCheckedChangeListener(null)
                holder.checkBox.isChecked = isSaved || isSelected
                holder.checkBox.buttonTintList = ColorStateList.valueOf(if (isSaved) emeraldColor else iconTintColor)
                holder.checkBox.setOnClickListener {
                    onToggleSelect(item)
                }
            } else {
                holder.checkBox.setOnCheckedChangeListener(null)
                holder.checkBox.visibility = View.GONE
            }

            holder.container.setOnClickListener { onNavigate(item) }
            holder.container.setOnLongClickListener {
                if (allowMultiple && !item.isParent) {
                    onToggleSelect(item)
                    true
                } else {
                    false
                }
            }
        }

        override fun getItemCount(): Int = items.size
    }

    companion object {
        const val TAG = "StormFolderPickerDialog"
        private const val ARG_INITIAL_PATH = "initial_path"
        private const val ARG_TITLE = "title"
        private const val ARG_ALLOW_MULTIPLE = "allow_multiple"
        private const val ARG_EXISTING_URIS = "existing_uris"

        fun show(
            fragmentManager: FragmentManager,
            title: String? = null,
            initialPath: String? = null,
            onSystemPickerRequested: (() -> Unit)? = null,
            onFolderPicked: (Uri) -> Unit
        ) {
            val dialog = StormFolderPickerDialog().apply {
                arguments = Bundle().apply {
                    if (!title.isNullOrBlank()) putString(ARG_TITLE, title)
                    if (!initialPath.isNullOrBlank()) putString(ARG_INITIAL_PATH, initialPath)
                    putBoolean(ARG_ALLOW_MULTIPLE, false)
                }
                this.onFolderPicked = onFolderPicked
                this.onSystemPickerRequested = onSystemPickerRequested
            }
            dialog.show(fragmentManager, TAG)
        }

        fun showMulti(
            fragmentManager: FragmentManager,
            title: String? = null,
            initialPath: String? = null,
            existingUris: List<String> = emptyList(),
            onRemoveExistingUri: ((String) -> Unit)? = null,
            onSystemPickerRequested: (() -> Unit)? = null,
            onFoldersPicked: (List<Uri>, Boolean) -> Unit
        ) {
            val dialog = StormFolderPickerDialog().apply {
                arguments = Bundle().apply {
                    if (!title.isNullOrBlank()) putString(ARG_TITLE, title)
                    if (!initialPath.isNullOrBlank()) putString(ARG_INITIAL_PATH, initialPath)
                    putBoolean(ARG_ALLOW_MULTIPLE, true)
                    putStringArrayList(ARG_EXISTING_URIS, ArrayList(existingUris))
                }
                this.onFoldersPicked = onFoldersPicked
                this.onRemoveExistingUri = onRemoveExistingUri
                this.onSystemPickerRequested = onSystemPickerRequested
            }
            dialog.show(fragmentManager, TAG)
        }
    }
}

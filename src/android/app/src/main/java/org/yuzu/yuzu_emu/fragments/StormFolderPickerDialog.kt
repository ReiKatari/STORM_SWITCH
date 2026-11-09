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
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import org.yuzu.yuzu_emu.R
import java.io.File
import java.util.Locale

class StormFolderPickerDialog : DialogFragment() {

    var onFolderPicked: ((Uri) -> Unit)? = null
    var onSystemPickerRequested: (() -> Unit)? = null

    private var currentDir: File = Environment.getExternalStorageDirectory()
    private var rootMinDir: File = Environment.getExternalStorageDirectory()
    private lateinit var pathTextView: TextView
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

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val ctx = requireContext()
        val density = resources.displayMetrics.density
        fun dp(v: Float): Int = (v * density).toInt()

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

        val rootLayout = LinearLayout(ctx).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(18f), dp(16f), dp(18f), dp(14f))
        }

        // Title
        val titleStr = arguments?.getString(ARG_TITLE)?.takeIf { it.isNotBlank() }
            ?: getString(R.string.storm_folder_picker_title)
        val titleView = TextView(ctx).apply {
            text = titleStr
            setTextColor(onSurfaceColor)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 18f)
            setTypeface(typeface, Typeface.BOLD)
            setPadding(0, 0, 0, dp(10f))
        }
        rootLayout.addView(titleView)

        // Storage roots switcher row (Internal Storage + any SD cards)
        val roots = discoverStorageRoots()
        if (roots.isNotEmpty()) {
            val hsv = HorizontalScrollView(ctx).apply {
                isHorizontalScrollBarEnabled = false
                setPadding(0, 0, 0, dp(8f))
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
                    minHeight = dp(36f)
                    minimumHeight = dp(36f)
                    setTextSize(TypedValue.COMPLEX_UNIT_SP, 12.5f)
                    setPadding(dp(12f), 0, dp(12f), 0)
                    backgroundTintList = ColorStateList.valueOf(surfaceVarColor)
                    setTextColor(onSurfaceColor)
                    layoutParams = LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.WRAP_CONTENT,
                        dp(36f)
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

        // Current path pill (clean borderless well)
        pathTextView = TextView(ctx).apply {
            setTextColor(primaryColor)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 12.5f)
            setTypeface(Typeface.MONOSPACE, Typeface.BOLD)
            maxLines = 2
            ellipsize = TextUtils.TruncateAt.START
            setPadding(dp(12f), dp(8f), dp(12f), dp(8f))
            background = GradientDrawable().apply {
                shape = GradientDrawable.RECTANGLE
                cornerRadius = dp(10f).toFloat()
                setColor(surfaceVarColor)
            }
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { bottomMargin = dp(10f) }
        }
        rootLayout.addView(pathTextView)

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
            ).apply { bottomMargin = dp(10f) }
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
        val listHeight = (screenH * 0.46f).toInt().coerceIn(dp(220f), dp(420f))

        adapter = FolderEntryAdapter(surfaceVarColor, primaryColor, onSurfaceColor, onSurfaceVarColor) { clickedEntry ->
            currentDir = clickedEntry.file
            refreshDirectory()
        }

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
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 12.5f)
            gravity = Gravity.CENTER
            setPadding(dp(12f), dp(12f), dp(12f), dp(12f))
            visibility = View.GONE
        }
        rootLayout.addView(emptyTextView)

        // Bottom Action Bar
        val actionsRow = LinearLayout(ctx).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.END or Gravity.CENTER_VERTICAL
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { topMargin = dp(12f) }
        }

        if (onSystemPickerRequested != null) {
            val sysBtn = MaterialButton(ctx).apply {
                text = getString(R.string.storm_folder_picker_system_saf)
                isAllCaps = false
                strokeWidth = 0
                insetTop = 0
                insetBottom = 0
                cornerRadius = dp(10f)
                minHeight = dp(42f)
                backgroundTintList = ColorStateList.valueOf(surfaceVarColor)
                setTextColor(onSurfaceVarColor)
                setTextSize(TypedValue.COMPLEX_UNIT_SP, 12.5f)
                layoutParams = LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                    dp(42f)
                ).apply { marginEnd = dp(6f) }
                setOnClickListener {
                    val cb = onSystemPickerRequested
                    dismissAllowingStateLoss()
                    cb?.invoke()
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
            minHeight = dp(42f)
            backgroundTintList = ColorStateList.valueOf(surfaceVarColor)
            setTextColor(onSurfaceColor)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 13f)
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                dp(42f)
            ).apply { marginEnd = dp(8f) }
            setOnClickListener { dismissAllowingStateLoss() }
        }
        actionsRow.addView(cancelBtn)

        val selectBtn = MaterialButton(ctx).apply {
            text = getString(R.string.storm_folder_picker_select_current)
            isAllCaps = false
            strokeWidth = 0
            insetTop = 0
            insetBottom = 0
            cornerRadius = dp(10f)
            minHeight = dp(42f)
            backgroundTintList = ColorStateList.valueOf(primaryColor)
            setTextColor(Color.WHITE)
            setTypeface(typeface, Typeface.BOLD)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 13.5f)
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                dp(42f)
            )
            setOnClickListener {
                val chosenUri = Uri.fromFile(currentDir)
                val cb = onFolderPicked
                dismissAllowingStateLoss()
                cb?.invoke(chosenUri)
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
        private val titleColor: Int,
        private val subColor: Int,
        private val onClick: (FolderRow) -> Unit
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
            val subtitle: TextView
        ) : RecyclerView.ViewHolder(container)

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): VH {
            val ctx = parent.context
            val density = ctx.resources.displayMetrics.density
            fun dp(v: Float): Int = (v * density).toInt()

            val row = LinearLayout(ctx).apply {
                orientation = LinearLayout.HORIZONTAL
                gravity = Gravity.CENTER_VERTICAL
                setPadding(dp(14f), dp(11f), dp(14f), dp(11f))
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
                    marginEnd = dp(12f)
                }
            }

            val textCol = LinearLayout(ctx).apply {
                orientation = LinearLayout.VERTICAL
                layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f)
            }

            val title = TextView(ctx).apply {
                setTextColor(titleColor)
                setTextSize(TypedValue.COMPLEX_UNIT_SP, 14.5f)
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

            textCol.addView(title)
            textCol.addView(subtitle)
            row.addView(icon)
            row.addView(textCol)
            return VH(row, icon, title, subtitle)
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
            holder.container.setOnClickListener { onClick(item) }
        }

        override fun getItemCount(): Int = items.size
    }

    companion object {
        const val TAG = "StormFolderPickerDialog"
        private const val ARG_INITIAL_PATH = "initial_path"
        private const val ARG_TITLE = "title"

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
                }
                this.onFolderPicked = onFolderPicked
                this.onSystemPickerRequested = onSystemPickerRequested
            }
            dialog.show(fragmentManager, TAG)
        }
    }
}

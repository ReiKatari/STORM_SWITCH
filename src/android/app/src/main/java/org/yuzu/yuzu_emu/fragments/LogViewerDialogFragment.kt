// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.fragments
import org.yuzu.yuzu_emu.R

import android.app.Dialog
import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.content.Intent
import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.net.Uri
import android.os.Bundle
import android.text.SpannableStringBuilder
import android.text.style.ForegroundColorSpan
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.Window
import android.widget.Toast
import androidx.core.content.FileProvider
import androidx.core.widget.doOnTextChanged
import androidx.fragment.app.DialogFragment
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import org.yuzu.yuzu_emu.NativeLibrary
import org.yuzu.yuzu_emu.databinding.DialogLogViewerBinding
import org.yuzu.yuzu_emu.model.Game
import org.yuzu.yuzu_emu.utils.DirectoryInitialization
import org.yuzu.yuzu_emu.utils.Log
import java.io.File

class LogViewerDialogFragment : DialogFragment() {

    private var _binding: DialogLogViewerBinding? = null
    private val binding get() = _binding!!

    private var game: Game? = null
    private var rawLogLines = listOf<String>()
    private var filteredLogLines = listOf<String>()

    private var currentFilterType = FILTER_ALL

    companion object {
        const val TAG = "LogViewerDialogFragment"
        private const val ARG_GAME = "game"

        const val FILTER_ALL = 0
        const val FILTER_ERRORS = 1
        const val FILTER_WARNINGS = 2
        const val FILTER_INFO = 3

        fun newInstance(game: Game?): LogViewerDialogFragment {
            val fragment = LogViewerDialogFragment()
            val args = Bundle().apply {
                if (game != null) {
                    putParcelable(ARG_GAME, game)
                }
            }
            fragment.arguments = args
            return fragment
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        game = arguments?.getParcelable(ARG_GAME)
        setStyle(STYLE_NO_TITLE, 0)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val dialog = super.onCreateDialog(savedInstanceState)
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE)
        dialog.window?.let { window ->
            window.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
        }
        return dialog
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = DialogLogViewerBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        val isRunning = NativeLibrary.isRunning()
        val gameName = game?.title?.ifBlank { null } ?: game?.programIdHex ?: "STORM SWITCH"
        val progId = game?.programIdHex ?: ""
        binding.textLogSubtitle.text = "$gameName ${if (progId.isNotEmpty()) "[$progId]" else ""} | ${if (isRunning) getString(R.string.log_viewer_status_ingame) else getString(R.string.log_viewer_status_offline)}"

        binding.btnClose.setOnClickListener { dismissAllowingStateLoss() }
        binding.btnRefreshLog.setOnClickListener {
            Toast.makeText(requireContext(), getString(R.string.log_viewer_toast_refreshing), Toast.LENGTH_SHORT).show()
            loadLogFile(true)
        }
        binding.btnCopyLog.setOnClickListener { copyLogToClipboard() }
        binding.btnSaveLog.setOnClickListener { saveLogToFile() }
        binding.btnShareLog.setOnClickListener { shareLogFile() }
        binding.btnShareMessenger.setOnClickListener { sendToTelegramOrMessenger() }

        binding.chipFilterAll.setOnClickListener { setFilter(FILTER_ALL) }
        binding.chipFilterErrors.setOnClickListener { setFilter(FILTER_ERRORS) }
        binding.chipFilterWarnings.setOnClickListener { setFilter(FILTER_WARNINGS) }
        binding.chipFilterInfo.setOnClickListener { setFilter(FILTER_INFO) }

        binding.editSearchLog.doOnTextChanged { text, _, _, _ ->
            val query = text?.toString()?.trim() ?: ""
            binding.btnClearSearch.visibility = if (query.isNotEmpty()) View.VISIBLE else View.GONE
            applyFilter()
        }

        binding.btnClearSearch.setOnClickListener {
            binding.editSearchLog.text?.clear()
        }

        loadLogFile(false)
    }

    private fun setFilter(filterType: Int) {
        currentFilterType = filterType
        applyFilter()
    }

    override fun onStart() {
        super.onStart()
        dialog?.window?.let { window ->
            val dm = resources.displayMetrics
            val isLandscape = resources.configuration.orientation == android.content.res.Configuration.ORIENTATION_LANDSCAPE
            val width = if (isLandscape) (dm.widthPixels * 0.92).toInt() else (dm.widthPixels * 0.95).toInt()
            val height = if (isLandscape) (dm.heightPixels * 0.92).toInt() else (dm.heightPixels * 0.85).toInt()
            window.setLayout(width, height)
            window.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
            window.setGravity(android.view.Gravity.CENTER)

            val lp = window.attributes
            lp.width = width
            lp.height = height
            lp.gravity = android.view.Gravity.CENTER
            window.attributes = lp
        }
    }

    private fun getActiveLogFile(): File? {
        val rootDir = DirectoryInitialization.userDirectory ?: requireContext().filesDir.absolutePath
        val logDir = File(rootDir, "log")
        return listOf(
            File(logDir, "storm_switch.txt"),
            File(logDir, "storm_switch_log.txt"),
            File(logDir, "storm_eden_log.txt"),
            File(logDir, "eden_log.txt"),
            File(logDir, "yuzu_log.txt")
        ).firstOrNull { it.exists() && it.length() > 0 } ?: File(logDir, "storm_switch.txt")
    }

    private fun loadLogFile(scrollToBottom: Boolean) {
        lifecycleScope.launch(Dispatchers.IO) {
            val logFile = getActiveLogFile()
            val lines = if (logFile != null && logFile.exists()) {
                try {
                    logFile.readLines(Charsets.UTF_8).takeLast(2500)
                } catch (_: Exception) {
                    emptyList()
                }
            } else {
                emptyList()
            }

            rawLogLines = lines
            val sizeKb = if (logFile != null && logFile.exists()) logFile.length() / 1024 else 0

            withContext(Dispatchers.Main) {
                if (_binding == null) return@withContext
                binding.textLogStats.text = getString(R.string.log_viewer_stats_total_fmt, lines.size, sizeKb)
                applyFilter()

                if (scrollToBottom) {
                    binding.scrollLog.post {
                        binding.scrollLog.fullScroll(View.FOCUS_DOWN)
                    }
                }
            }
        }
    }

    private fun applyFilter() {
        val searchQuery = binding.editSearchLog.text?.toString()?.trim() ?: ""

        filteredLogLines = rawLogLines.filter { line ->
            val matchesCategory = when (currentFilterType) {
                FILTER_ERRORS -> line.contains("<Error>", ignoreCase = true) || line.contains("<Critical>", ignoreCase = true)
                FILTER_WARNINGS -> line.contains("<Warning>", ignoreCase = true)
                FILTER_INFO -> line.contains("<Info>", ignoreCase = true)
                else -> true
            }

            val matchesSearch = if (searchQuery.isEmpty()) true else line.contains(searchQuery, ignoreCase = true)

            matchesCategory && matchesSearch
        }

        val ssb = SpannableStringBuilder()
        for (line in filteredLogLines) {
            val start = ssb.length
            ssb.append(line).append("\n")
            val end = ssb.length

            val color = when {
                line.contains("<Error>", ignoreCase = true) || line.contains("<Critical>", ignoreCase = true) -> 0xFFFF6B6B.toInt()
                line.contains("<Warning>", ignoreCase = true) -> 0xFFFFD166.toInt()
                line.contains("<Info>", ignoreCase = true) -> 0xFF4FD1C5.toInt()
                line.contains("<Debug>", ignoreCase = true) -> 0xFFA0AEC0.toInt()
                else -> 0xFFC9D1D9.toInt()
            }
            ssb.setSpan(ForegroundColorSpan(color), start, end, SpannableStringBuilder.SPAN_EXCLUSIVE_EXCLUSIVE)
        }

        val filterLabel = when (currentFilterType) {
            FILTER_ERRORS -> getString(R.string.log_viewer_filter_errors_label)
            FILTER_WARNINGS -> getString(R.string.log_viewer_filter_warnings_label)
            FILTER_INFO -> getString(R.string.log_viewer_filter_info_label)
            else -> getString(R.string.log_viewer_filter_all_label)
        }
        binding.textLogStats.text = getString(R.string.log_viewer_stats_filtered_fmt, filterLabel, filteredLogLines.size, rawLogLines.size)

        if (filteredLogLines.isEmpty()) {
            binding.textLogContent.text = getString(R.string.log_viewer_empty_filtered)
        } else {
            binding.textLogContent.text = ssb
        }
    }

    private fun copyLogToClipboard() {
        val text = if (filteredLogLines.isNotEmpty()) {
            filteredLogLines.joinToString("\n")
        } else {
            rawLogLines.joinToString("\n")
        }
        if (text.isBlank()) {
            Toast.makeText(requireContext(), getString(R.string.log_viewer_toast_empty), Toast.LENGTH_SHORT).show()
            return
        }

        val clipboard = requireContext().getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
        val clip = ClipData.newPlainText("STORM SWITCH Log", text)
        clipboard.setPrimaryClip(clip)
        Toast.makeText(requireContext(), getString(R.string.log_viewer_toast_copied_fmt, filteredLogLines.size), Toast.LENGTH_SHORT).show()
    }

    private fun saveLogToFile() {
        val textToSave = if (filteredLogLines.isNotEmpty()) {
            filteredLogLines.joinToString("\n")
        } else {
            rawLogLines.joinToString("\n")
        }

        if (textToSave.isBlank()) {
            Toast.makeText(requireContext(), getString(R.string.log_viewer_toast_empty_save), Toast.LENGTH_SHORT).show()
            return
        }

        lifecycleScope.launch(Dispatchers.IO) {
            try {
                val timeStamp = java.text.SimpleDateFormat("yyyyMMdd_HHmmss", java.util.Locale.getDefault()).format(java.util.Date())
                val targetDir = File(requireContext().getExternalFilesDir(null), "logs").apply { mkdirs() }
                val targetFile = File(targetDir, "STORM_SWITCH_log_$timeStamp.txt")
                targetFile.writeText(textToSave, Charsets.UTF_8)

                withContext(Dispatchers.Main) {
                    Toast.makeText(
                        requireContext(),
                        getString(R.string.log_viewer_toast_saved_fmt, targetFile.name),
                        Toast.LENGTH_LONG
                    ).show()
                }
            } catch (e: Exception) {
                withContext(Dispatchers.Main) {
                    Toast.makeText(requireContext(), getString(R.string.log_viewer_toast_save_err_fmt, e.message ?: ""), Toast.LENGTH_SHORT).show()
                }
            }
        }
    }

    private fun shareLogFile() {
        val textToShare = if (filteredLogLines.isNotEmpty()) {
            filteredLogLines.joinToString("\n")
        } else {
            rawLogLines.joinToString("\n")
        }

        if (textToShare.isBlank()) {
            Toast.makeText(requireContext(), getString(R.string.log_viewer_toast_empty), Toast.LENGTH_SHORT).show()
            return
        }

        lifecycleScope.launch(Dispatchers.IO) {
            try {
                val cacheDir = requireContext().cacheDir
                val shareFile = File(cacheDir, "storm_switch.txt")
                shareFile.writeText(textToShare, Charsets.UTF_8)

                val uri: Uri = FileProvider.getUriForFile(
                    requireContext(),
                    "${requireContext().packageName}.provider",
                    shareFile
                )

                withContext(Dispatchers.Main) {
                    if (!isAdded || context == null) return@withContext
                    val filterSuffix = when (currentFilterType) {
                        FILTER_ERRORS -> getString(R.string.log_viewer_suffix_errors)
                        FILTER_WARNINGS -> getString(R.string.log_viewer_suffix_warnings)
                        else -> ""
                    }
                    val intent = Intent(Intent.ACTION_SEND).apply {
                        type = "text/plain"
                        putExtra(Intent.EXTRA_STREAM, uri)
                        putExtra(Intent.EXTRA_SUBJECT, "STORM SWITCH Log - ${game?.title ?: "Switch"}$filterSuffix")
                        putExtra(Intent.EXTRA_TEXT, textToShare.take(4000))
                        addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
                    }
                    val chooser = Intent.createChooser(intent, getString(R.string.log_viewer_share_title))
                    chooser.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                    startActivity(chooser)
                }
            } catch (e: Exception) {
                Log.error("[LogViewer] Failed to share log file: ${e.message}")
                withContext(Dispatchers.Main) {
                    if (!isAdded || context == null) return@withContext
                    try {
                        val intent = Intent(Intent.ACTION_SEND).apply {
                            type = "text/plain"
                            putExtra(Intent.EXTRA_TEXT, textToShare.take(2000))
                        }
                        val chooser = Intent.createChooser(intent, getString(R.string.log_viewer_share_title))
                        chooser.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                        startActivity(chooser)
                    } catch (t: Throwable) {
                        Toast.makeText(requireContext(), getString(R.string.log_viewer_toast_share_err_fmt, e.localizedMessage ?: getString(R.string.storm_dl_error_default)), Toast.LENGTH_SHORT).show()
                    }
                }
            }
        }
    }

    private fun sendToTelegramOrMessenger() {
        val textToShare = if (filteredLogLines.isNotEmpty()) {
            filteredLogLines.joinToString("\n")
        } else {
            rawLogLines.joinToString("\n")
        }

        if (textToShare.isBlank()) {
            Toast.makeText(requireContext(), getString(R.string.log_viewer_toast_empty), Toast.LENGTH_SHORT).show()
            return
        }

        lifecycleScope.launch(Dispatchers.IO) {
            try {
                val cacheDir = requireContext().cacheDir
                val shareFile = File(cacheDir, "storm_switch.txt")
                shareFile.writeText(textToShare, Charsets.UTF_8)

                val uri: Uri = FileProvider.getUriForFile(
                    requireContext(),
                    "${requireContext().packageName}.provider",
                    shareFile
                )

                withContext(Dispatchers.Main) {
                    if (!isAdded || context == null) return@withContext
                    val intent = Intent(Intent.ACTION_SEND).apply {
                        type = "text/plain"
                        putExtra(Intent.EXTRA_STREAM, uri)
                        putExtra(Intent.EXTRA_SUBJECT, "STORM SWITCH Log - ${game?.title ?: "Switch"}")
                        putExtra(Intent.EXTRA_TEXT, getString(R.string.log_viewer_share_header_fmt, game?.title ?: "Switch") + textToShare.take(3000))
                        addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
                    }
                    val chooser = Intent.createChooser(intent, getString(R.string.log_viewer_share_tg_title))
                    chooser.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                    startActivity(chooser)
                }
            } catch (e: Exception) {
                Log.error("[LogViewer] Failed to share log to Telegram: ${e.message}")
                withContext(Dispatchers.Main) {
                    if (!isAdded || context == null) return@withContext
                    try {
                        val intent = Intent(Intent.ACTION_SEND).apply {
                            type = "text/plain"
                            putExtra(Intent.EXTRA_TEXT, textToShare.take(2000))
                        }
                        val chooser = Intent.createChooser(intent, getString(R.string.log_viewer_share_tg_title))
                        chooser.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                        startActivity(chooser)
                    } catch (t: Throwable) {
                        Toast.makeText(requireContext(), getString(R.string.log_viewer_toast_share_err_fmt, e.localizedMessage ?: getString(R.string.storm_dl_error_default)), Toast.LENGTH_SHORT).show()
                    }
                }
            }
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}


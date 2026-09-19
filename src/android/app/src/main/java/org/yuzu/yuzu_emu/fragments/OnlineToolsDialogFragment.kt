// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.app.Dialog
import android.graphics.Color
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.core.view.isVisible
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.activityViewModels
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import okhttp3.Call
import okhttp3.OkHttpClient
import okhttp3.Request
import org.json.JSONArray
import org.json.JSONObject
import org.yuzu.yuzu_emu.NativeLibrary
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.databinding.DialogOnlineToolsBinding
import org.yuzu.yuzu_emu.databinding.ListItemOnlineToolBinding
import org.yuzu.yuzu_emu.model.GamesViewModel
import org.yuzu.yuzu_emu.model.HomeViewModel
import org.yuzu.yuzu_emu.utils.DirectoryInitialization
import org.yuzu.yuzu_emu.utils.Log
import org.yuzu.yuzu_emu.utils.NativeConfig
import org.yuzu.yuzu_emu.utils.SmartDns
import org.yuzu.yuzu_emu.utils.ThemeHelper
import java.io.File
import java.io.FileOutputStream
import java.io.InputStream
import java.util.Locale
import java.util.concurrent.TimeUnit
import java.util.zip.ZipEntry
import java.util.zip.ZipInputStream

data class OnlineToolAsset(
    val version: String,
    val name: String,
    val size: Long,
    val downloadUrl: String,
    val displayTitle: String,
    val displaySize: String,
    val isRecommended: Boolean = false
)

data class InstalledInfo(
    val isInstalled: Boolean,
    val version: String?,
    val displayText: String
)

class OnlineToolsDialogFragment : DialogFragment() {
    private var _binding: DialogOnlineToolsBinding? = null
    private val binding get() = _binding!!

    private val homeViewModel: HomeViewModel by activityViewModels()
    private val gamesViewModel: GamesViewModel by activityViewModels()

    private var toolType: Int = TYPE_FIRMWARE
    private val assets = mutableListOf<OnlineToolAsset>()
    private var currentInstalledInfo = InstalledInfo(false, null, "")
    private var selectedIndex = 0
    private var isDownloading = false
    private var downloadJob: Job? = null
    private var activeCall: Call? = null
    var onInstalled: (() -> Unit)? = null

    private lateinit var adapter: ToolsAdapter

    private val httpClient by lazy {
        OkHttpClient.Builder()
            .dns(SmartDns)
            .connectTimeout(10, TimeUnit.SECONDS)
            .readTimeout(20, TimeUnit.SECONDS)
            .writeTimeout(20, TimeUnit.SECONDS)
            .connectionPool(okhttp3.ConnectionPool(5, 5, TimeUnit.MINUTES))
            .protocols(listOf(okhttp3.Protocol.HTTP_2, okhttp3.Protocol.HTTP_1_1))
            .followRedirects(true)
            .followSslRedirects(true)
            .retryOnConnectionFailure(true)
            .build()
    }

    companion object {
        const val TAG = "OnlineToolsDialogFragment"
        const val ARG_TYPE = "online_tool_type"
        const val TYPE_FIRMWARE = 0
        const val TYPE_KEYS = 1

        fun newInstance(type: Int): OnlineToolsDialogFragment {
            val args = Bundle().apply {
                putInt(ARG_TYPE, type)
            }
            return OnlineToolsDialogFragment().apply {
                arguments = args
            }
        }

        fun formatBytes(bytes: Long, context: android.content.Context? = null): String {
            val ctx = context ?: org.yuzu.yuzu_emu.YuzuApplication.appContext
            val gb = ctx.getString(R.string.unit_gb)
            val mb = ctx.getString(R.string.unit_mb)
            val kb = ctx.getString(R.string.unit_kb)
            val b = ctx.getString(R.string.unit_b)
            return when {
                bytes >= 1024 * 1024 * 1024 -> String.format("%.1f %s", bytes.toDouble() / (1024 * 1024 * 1024), gb)
                bytes >= 1024 * 1024 -> String.format("%.1f %s", bytes.toDouble() / (1024 * 1024), mb)
                bytes >= 1024 -> String.format("%.1f %s", bytes.toDouble() / 1024, kb)
                else -> "$bytes $b"
            }
        }

        fun compareVersions(v1: String, v2: String): Int {
            val p1 = v1.split('.', '-', '_').mapNotNull { it.filter { c -> c.isDigit() }.toIntOrNull() }
            val p2 = v2.split('.', '-', '_').mapNotNull { it.filter { c -> c.isDigit() }.toIntOrNull() }
            val maxLen = maxOf(p1.size, p2.size)
            for (i in 0 until maxLen) {
                val n1 = p1.getOrElse(i) { 0 }
                val n2 = p2.getOrElse(i) { 0 }
                if (n1 != n2) {
                    return n1.compareTo(n2)
                }
            }
            return v1.compareTo(v2, ignoreCase = true)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setStyle(STYLE_NORMAL, ThemeHelper.getSelectedStaticThemeColor())
        toolType = arguments?.getInt(ARG_TYPE, TYPE_FIRMWARE) ?: TYPE_FIRMWARE
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val dialog = super.onCreateDialog(savedInstanceState)
        dialog.requestWindowFeature(android.view.Window.FEATURE_NO_TITLE)
        dialog.window?.let { window ->
            window.setBackgroundDrawable(android.graphics.drawable.ColorDrawable(android.graphics.Color.TRANSPARENT))
        }
        return dialog
    }

    override fun onStart() {
        super.onStart()
        dialog?.window?.let { window ->
            window.setLayout(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
            )
            window.setBackgroundDrawableResource(android.R.color.transparent)
            ThemeHelper.applySystemBarsTheme(window, requireContext())
        }
    }

    override fun onDismiss(dialog: android.content.DialogInterface) {
        super.onDismiss(dialog)
        activity?.let { ThemeHelper.applySystemBarsTheme(it.window, it) }
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = DialogOnlineToolsBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        ViewCompat.setOnApplyWindowInsetsListener(binding.root) { _, insets ->
            val systemBars = insets.getInsets(
                WindowInsetsCompat.Type.systemBars() or
                WindowInsetsCompat.Type.displayCutout()
            )
            binding.root.setPadding(
                systemBars.left,
                systemBars.top,
                systemBars.right,
                systemBars.bottom
            )
            insets
        }
        binding.root.requestApplyInsets()

        val isFirmware = toolType == TYPE_FIRMWARE
        binding.headerIcon.text = if (isFirmware) "🌐" else "🔑"
        binding.textHeaderTitle.text = if (isFirmware) {
            getString(R.string.online_install_firmware)
        } else {
            getString(R.string.online_install_keys)
        }
        binding.textHeaderSubtitle.text = if (isFirmware) {
            getString(R.string.online_install_firmware_description)
        } else {
            getString(R.string.online_install_keys_description)
        }

        binding.btnCloseDialog.setOnClickListener {
            if (isDownloading) {
                cancelDownload(cleanOnly = true)
            }
            dismiss()
        }
        binding.btnCancel.setOnClickListener {
            if (isDownloading) {
                cancelDownload(cleanOnly = false)
            } else {
                dismiss()
            }
        }
        binding.btnRefresh.setOnClickListener { refreshCatalog() }
        binding.btnInstall.setOnClickListener { startDownloadAndInstall() }

        adapter = ToolsAdapter()
        binding.listTools.layoutManager = LinearLayoutManager(requireContext())
        binding.listTools.adapter = adapter

        updateInstalledStatusHeader()
        populateFallbackCatalog()
        refreshCatalog()
    }

    private fun detectInstalledInfo(): InstalledInfo {
        val ctx = context?.applicationContext ?: return InstalledInfo(false, null, "Не определено")
        val prefs = androidx.preference.PreferenceManager.getDefaultSharedPreferences(ctx)

        return if (toolType == TYPE_FIRMWARE) {
            val isAvail = try { NativeLibrary.isFirmwareAvailable() } catch (_: Throwable) { false }
            val fwVerNative = if (isAvail) {
                try {
                    val v = NativeLibrary.firmwareVersion()
                    if (v.isNotBlank() && !v.equals("N/A", ignoreCase = true)) v.trim() else null
                } catch (_: Throwable) { null }
            } else null

            val savedVer = prefs.getString("installed_online_firmware_version", null)
            val resolvedVer = fwVerNative ?: savedVer

            if (isAvail || resolvedVer != null) {
                val display = if (resolvedVer != null) {
                    "Установлена версия: $resolvedVer"
                } else {
                    "Установлена прошивка Switch"
                }
                InstalledInfo(true, resolvedVer, display)
            } else {
                InstalledInfo(false, null, "Прошивка не установлена")
            }
        } else {
            val savedVer = prefs.getString("installed_online_keys_version", null)
            val diskVer = detectKeysVersionFromDisk(ctx)
            val keysValid = try { NativeLibrary.reloadKeys() } catch (_: Throwable) { false }

            val detectedVer = when {
                savedVer != null && (keysValid || diskVer != null) -> savedVer
                diskVer != null -> diskVer
                else -> null
            }

            if (keysValid || detectedVer != null) {
                val display = if (detectedVer != null) {
                    "Установлена версия: $detectedVer"
                } else {
                    "Установлены ключи (prod.keys)"
                }
                InstalledInfo(true, detectedVer, display)
            } else {
                InstalledInfo(false, null, "Ключи не установлены")
            }
        }
    }

    private fun detectKeysVersionFromDisk(ctx: android.content.Context): String? {
        val targetDirs = listOfNotNull(
            ctx.filesDir?.let { File(it, "keys") },
            DirectoryInitialization.userDirectory?.let { File(it, "keys") },
            File(android.os.Environment.getExternalStorageDirectory(), "STORM SWITCH/keys"),
            ctx.getExternalFilesDir(null)?.let { File(it, "keys") }
        ).distinctBy { it.canonicalPath }

        for (dir in targetDirs) {
            val prodKeys = File(dir, "prod.keys")
            if (prodKeys.exists() && prodKeys.isFile && prodKeys.length() > 0) {
                try {
                    val lines = prodKeys.readLines()
                    for (line in lines.take(15)) {
                        val trimmed = line.trim()
                        if (trimmed.startsWith("#") || trimmed.startsWith("//")) {
                            val match = Regex("""(?:v|\b)(\d+\.\d+(?:\.\d+)?)\b""").find(trimmed)
                            if (match != null) {
                                return match.groupValues[1]
                            }
                        }
                    }
                    var highestMasterKeyHex = -1
                    for (line in lines) {
                        val trimmed = line.trim().lowercase(Locale.ROOT)
                        if (trimmed.startsWith("master_key_")) {
                            val hexStr = trimmed.substringAfter("master_key_").substringBefore('=').trim()
                            val num = hexStr.toIntOrNull(16) ?: -1
                            if (num > highestMasterKeyHex) {
                                highestMasterKeyHex = num
                            }
                        }
                    }
                    if (highestMasterKeyHex >= 0) {
                        return when {
                            highestMasterKeyHex >= 0x16 -> "23.0.0"
                            highestMasterKeyHex == 0x15 -> "21.2.0"
                            highestMasterKeyHex == 0x14 -> "20.5.0"
                            highestMasterKeyHex == 0x13 -> "19.0.1"
                            highestMasterKeyHex == 0x12 -> "18.1.0"
                            highestMasterKeyHex == 0x11 -> "17.0.1"
                            highestMasterKeyHex == 0x10 -> "16.1.0"
                            highestMasterKeyHex == 0x0f -> "15.0.1"
                            highestMasterKeyHex == 0x0e -> "14.1.2"
                            highestMasterKeyHex == 0x0d -> "13.2.1"
                            highestMasterKeyHex == 0x0c -> "12.1.0"
                            else -> null
                        }
                    }
                } catch (_: Throwable) {}
            }
        }
        return null
    }

    private fun isVersionMatching(assetVer: String, installedVer: String?): Boolean {
        if (installedVer == null) return false
        val v1 = assetVer.trim().lowercase(Locale.ROOT).removePrefix("v")
        val v2 = installedVer.trim().lowercase(Locale.ROOT).removePrefix("v")
        if (v1 == v2) return true

        val p1 = v1.split('.').take(2)
        val p2 = v2.split('.').take(2)
        if (p1.size == 2 && p2.size == 2 && p1 == p2) return true

        return false
    }

    private fun updateInstalledStatusHeader() {
        val binding = _binding ?: return
        currentInstalledInfo = detectInstalledInfo()
        binding.textInstalledStatus.text = currentInstalledInfo.displayText
        if (currentInstalledInfo.isInstalled) {
            binding.textInstalledStatusIcon.text = "🟢"
            binding.badgeInstalledHeader.text = "АКТИВНА"
            binding.badgeInstalledHeader.setTextColor(0xFF10B981.toInt())
            binding.badgeInstalledHeader.setBackgroundResource(R.drawable.badge_pill_emerald)
        } else {
            binding.textInstalledStatusIcon.text = "⚪"
            binding.badgeInstalledHeader.text = "НЕ УСТАНОВЛЕНА"
            binding.badgeInstalledHeader.setTextColor(0xFFF59E0B.toInt())
            binding.badgeInstalledHeader.setBackgroundResource(R.drawable.badge_pill_amber)
        }
    }

    private fun populateFallbackCatalog() {
        assets.clear()
        if (toolType == TYPE_FIRMWARE) {
            val list = listOf(
                Pair("20.0.1", 356052129L),
                Pair("20.0.0", 356051105L),
                Pair("19.0.1", 338082652L),
                Pair("19.0.0", 338076508L)
            )
            for (i in list.indices) {
                val (ver, sz) = list[i]
                assets.add(
                    OnlineToolAsset(
                        version = ver,
                        name = "$ver.zip",
                        size = sz,
                        downloadUrl = "https://github.com/ReiKatari/STORM_SWITCH_TOOLS/releases/download/STORM_SWITCH_TOOLS_FIRMWARES/$ver.zip",
                        displayTitle = "Прошивка $ver",
                        displaySize = formatBytes(sz),
                        isRecommended = (i == 0)
                    )
                )
            }
        } else {
            val list = listOf(
                Pair("23.0.0", 12177L),
                Pair("22.5.0", 7423L),
                Pair("22.1.0", 7407L),
                Pair("22.0.0", 7407L),
                Pair("21.2.0", 11343L),
                Pair("21.1.0", 11343L),
                Pair("21.0.1", 11343L),
                Pair("21.0.0", 11343L),
                Pair("20.5.0", 6976L),
                Pair("20.1.0", 6130L),
                Pair("20.0.1", 6130L),
                Pair("20.0.0", 6123L),
                Pair("19.0.1", 8149L)
            )
            for (i in list.indices) {
                val (ver, sz) = list[i]
                assets.add(
                    OnlineToolAsset(
                        version = ver,
                        name = "$ver.zip",
                        size = sz,
                        downloadUrl = "https://github.com/ReiKatari/STORM_SWITCH_TOOLS/releases/download/STORM_SWITCH_TOOLS_KEYS/$ver.zip",
                        displayTitle = "Ключи дешифрования $ver",
                        displaySize = formatBytes(sz),
                        isRecommended = (i == 0)
                    )
                )
            }
        }
        val installedIdx = assets.indexOfFirst { isVersionMatching(it.version, currentInstalledInfo.version) }
        selectedIndex = if (installedIdx >= 0) installedIdx else 0
        adapter.notifyDataSetChanged()
        updateSelectionStatus()
    }

    private fun refreshCatalog() {
        binding.btnRefresh.isEnabled = false
        binding.textStatus.text = "Проверка обновлений каталога в GitHub..."

        val tagName = if (toolType == TYPE_FIRMWARE) {
            "STORM_SWITCH_TOOLS_FIRMWARES"
        } else {
            "STORM_SWITCH_TOOLS_KEYS"
        }
        val candidateUrls = listOf(
            "https://api.github.com/repos/ReiKatari/STORM_SWITCH_TOOLS/releases/tags/$tagName",
            "https://ghfast.top/https://api.github.com/repos/ReiKatari/STORM_SWITCH_TOOLS/releases/tags/$tagName",
            "https://gh-proxy.net/https://api.github.com/repos/ReiKatari/STORM_SWITCH_TOOLS/releases/tags/$tagName"
        )

        viewLifecycleOwner.lifecycleScope.launch(Dispatchers.IO) {
            try {
                var body: String? = null
                for (reqUrl in candidateUrls) {
                    try {
                        val request = Request.Builder()
                            .url(reqUrl)
                            .header("User-Agent", "STORM_SWITCH_Android")
                            .build()
                        val response = httpClient.newCall(request).execute()
                        if (response.isSuccessful) {
                            val resBody = response.body?.string()
                            if (!resBody.isNullOrEmpty()) {
                                body = resBody
                                break
                            }
                        }
                    } catch (_: Throwable) {}
                }
                    if (!body.isNullOrEmpty()) {
                        val json = JSONObject(body)
                        val assetsArray = json.optJSONArray("assets") ?: JSONArray()
                        val newAssets = mutableListOf<OnlineToolAsset>()
                        for (i in 0 until assetsArray.length()) {
                            val obj = assetsArray.getJSONObject(i)
                            val name = obj.optString("name", "")
                            if (!name.endsWith(".zip", ignoreCase = true)) continue
                            val ver = name.removeSuffix(".zip")
                            val sz = obj.optLong("size", 0L)
                            val dlUrl = obj.optString("browser_download_url", "")
                            newAssets.add(
                                OnlineToolAsset(
                                    version = ver,
                                    name = name,
                                    size = sz,
                                    downloadUrl = dlUrl,
                                    displayTitle = if (toolType == TYPE_FIRMWARE) "Прошивка $ver" else "Ключи дешифрования $ver",
                                    displaySize = formatBytes(sz),
                                    isRecommended = (i == 0)
                                )
                            )
                        }
                        // Sort strictly descending from newest to oldest version
                        newAssets.sortWith { a, b -> compareVersions(b.version, a.version) }
                        val sortedAssets = newAssets.mapIndexed { index, item ->
                            item.copy(isRecommended = (index == 0))
                        }
                        if (sortedAssets.isNotEmpty()) {
                            withContext(Dispatchers.Main) {
                                assets.clear()
                                assets.addAll(sortedAssets)
                                val installedIdx = assets.indexOfFirst { isVersionMatching(it.version, currentInstalledInfo.version) }
                                selectedIndex = if (installedIdx >= 0) installedIdx else 0
                                adapter.notifyDataSetChanged()
                                updateSelectionStatus()
                                binding.textStatus.text = "Каталог успешно обновлен из репозитория."
                            }
                        }
                    }
            } catch (e: Exception) {
                Log.error("[OnlineTools] Failed to fetch catalog: ${e.message}")
            } finally {
                withContext(Dispatchers.Main) {
                    binding.btnRefresh.isEnabled = true
                }
            }
        }
    }

    private fun updateSelectionStatus() {
        if (selectedIndex in assets.indices) {
            val asset = assets[selectedIndex]
            val isCurrentInstalled = isVersionMatching(asset.version, currentInstalledInfo.version)

            if (isCurrentInstalled) {
                binding.textStatus.text = "Выбрана текущая установленная версия: ${asset.displayTitle} (${asset.displaySize})"
                binding.btnInstall.text = "Переустановить"
            } else if (currentInstalledInfo.isInstalled && currentInstalledInfo.version != null && compareVersions(asset.version, currentInstalledInfo.version!!) > 0) {
                binding.textStatus.text = "Доступно обновление: ${asset.displayTitle} (${asset.displaySize})"
                binding.btnInstall.text = "Обновить"
            } else {
                binding.textStatus.text = getString(R.string.online_tools_selected_format, asset.displayTitle, asset.displaySize)
                binding.btnInstall.text = "Скачать и установить"
            }
            binding.btnInstall.isEnabled = !isDownloading
        }
    }

    private fun cancelDownload(cleanOnly: Boolean = false) {
        val job = downloadJob
        downloadJob = null
        val call = activeCall
        activeCall = null
        isDownloading = false

        try {
            call?.cancel()
        } catch (_: Throwable) {}
        try {
            job?.cancel()
        } catch (_: Throwable) {}

        try {
            val cacheDir = context?.cacheDir
            if (cacheDir != null) {
                File(cacheDir, "firmware_stream.tmp").delete()
                File(cacheDir, "keys_stream.tmp").delete()
                File(cacheDir, "extracted_firmware").deleteRecursively()
            }
        } catch (_: Throwable) {}

        if (!cleanOnly && _binding != null) {
            binding.progressBar.isIndeterminate = false
            binding.progressBar.progress = 0
            binding.btnInstall.isEnabled = (selectedIndex in assets.indices)
            binding.btnInstall.text = "Скачать и установить"
            binding.btnRefresh.isEnabled = true
            binding.btnCancel.isEnabled = true
            updateSelectionStatus()
            context?.let {
                Toast.makeText(it, "Загрузка отменена", Toast.LENGTH_SHORT).show()
            }
        }
    }

    private fun startDownloadAndInstall() {
        if (selectedIndex !in assets.indices || isDownloading) return
        val asset = assets[selectedIndex]

        val appContext = requireContext().applicationContext
        val cacheDir = appContext.cacheDir
        val tempZip = File(cacheDir, if (toolType == TYPE_FIRMWARE) "firmware_stream.tmp" else "keys_stream.tmp")

        isDownloading = true
        binding.btnInstall.isEnabled = false
        binding.btnInstall.text = "Загрузка..."
        binding.btnRefresh.isEnabled = false
        binding.btnCancel.isEnabled = true
        binding.progressBar.isIndeterminate = false
        binding.progressBar.progress = 0

        downloadJob = viewLifecycleOwner.lifecycleScope.launch(Dispatchers.IO) {
            try {
                withContext(Dispatchers.Main) {
                    if (_binding != null) {
                        binding.textStatus.text = getString(R.string.online_tools_connecting_format, asset.name)
                    }
                }

                val candidateDlUrls = mutableListOf<String>()
                // Direct GitHub first: CDN (1.3+ MB/s) and no proxy bottlenecks or arbitrary size limits!
                candidateDlUrls.add(asset.downloadUrl)

                if (asset.downloadUrl.startsWith("https://github.com/")) {
                    candidateDlUrls.add("https://gh.con.sh/" + asset.downloadUrl)
                    candidateDlUrls.add("https://gh.llkk.cc/" + asset.downloadUrl)
                    candidateDlUrls.add("https://ghfast.top/" + asset.downloadUrl)
                }

                var downloadSuccess = false
                var lastError: Exception? = null

                for (dlUrl in candidateDlUrls) {
                    if (!isActive) break
                    try {
                        withContext(Dispatchers.Main) {
                            if (_binding != null) {
                                binding.textStatus.text = getString(R.string.online_tools_connecting_format, asset.name)
                            }
                        }

                        val request = Request.Builder()
                            .url(dlUrl)
                            .header("User-Agent", "STORM_SWITCH_Installer")
                            .header("Accept-Encoding", "identity")
                            .build()

                        val call = httpClient.newCall(request)
                        activeCall = call
                        val response = call.execute()
                        if (!response.isSuccessful) {
                            response.close()
                            throw java.io.IOException("HTTP ${response.code}")
                        }

                        val contentType = response.header("Content-Type") ?: ""
                        if (contentType.contains("text/html", ignoreCase = true)) {
                            response.close()
                            throw java.io.IOException("Ответ сервера — веб-страница вместо файла")
                        }

                        val body = response.body ?: throw java.io.IOException("Пустой ответ")
                        val totalLength = body.contentLength()
                        var downloaded = 0L

                        val buffer = ByteArray(65536)
                        java.io.BufferedInputStream(body.byteStream(), 65536).use { input ->
                            java.io.BufferedOutputStream(FileOutputStream(tempZip), 65536).use { output ->
                                var read = 0
                                var lastReportTime = System.currentTimeMillis()

                                while (isActive && input.read(buffer).also { read = it } != -1) {
                                    output.write(buffer, 0, read)
                                    downloaded += read

                                    val now = System.currentTimeMillis()
                                    if (now - lastReportTime > 250) {
                                        lastReportTime = now
                                        val progress = if (totalLength > 0) ((downloaded * 100) / totalLength).toInt() else 0
                                        val dlFormatted = formatBytes(downloaded, appContext)
                                        val totalFormatted = if (totalLength > 0) formatBytes(totalLength, appContext) else ""
                                        withContext(Dispatchers.Main) {
                                            if (_binding != null && isDownloading) {
                                                binding.progressBar.progress = progress
                                                binding.textStatus.text = if (totalLength > 0) {
                                                    getString(
                                                        R.string.online_tools_downloading_format,
                                                        progress,
                                                        dlFormatted,
                                                        totalFormatted
                                                    )
                                                } else {
                                                    "Загрузка: $dlFormatted"
                                                }
                                            }
                                        }
                                    }
                                }
                                output.flush()
                            }
                        }

                        activeCall = null

                        val isComplete = if (totalLength > 0) {
                            downloaded >= totalLength
                        } else if (asset.size > 0) {
                            downloaded >= (asset.size * 95 / 100)
                        } else {
                            downloaded > 0
                        }

                        if (tempZip.exists() && isComplete) {
                            downloadSuccess = true
                            break
                        } else {
                            tempZip.delete()
                        }
                    } catch (e: CancellationException) {
                        tempZip.delete()
                        throw e
                    } catch (e: Exception) {
                        activeCall = null
                        tempZip.delete()
                        lastError = e
                        Log.error("[OnlineToolsDialogFragment] Failed downloading from $dlUrl: ${e.message}")
                    }
                }

                if (!downloadSuccess) {
                    throw lastError ?: Exception("Не удалось загрузить файл из доступных источников")
                }

                withContext(Dispatchers.Main) {
                    if (_binding != null) {
                        binding.progressBar.isIndeterminate = true
                        binding.textStatus.text = getString(R.string.online_tools_extracting)
                    }
                }

                if (toolType == TYPE_FIRMWARE) {
                    val primaryNand = try {
                        val nd = NativeConfig.getNandDir()
                        if (nd.isNotBlank()) File(nd, "system/Contents/registered") else null
                    } catch (_: Throwable) { null } ?: File(appContext.filesDir, "nand/system/Contents/registered")

                    primaryNand.mkdirs()

                    java.util.zip.ZipFile(tempZip).use { zipFile ->
                        val entries = zipFile.entries()
                        while (isActive && entries.hasMoreElements()) {
                            val entry = entries.nextElement()
                            if (entry.isDirectory) continue
                            val entryName = entry.name.substringAfterLast('/')
                            if (!entryName.endsWith(".nca", ignoreCase = true)) continue

                            val outFile = File(primaryNand, entryName)
                            zipFile.getInputStream(entry).use { inStream ->
                                FileOutputStream(outFile).use { outStream ->
                                    inStream.copyTo(outStream)
                                }
                            }
                        }
                    }

                    tempZip.delete()

                    val targetFirmwareDirs = listOfNotNull(
                        appContext.filesDir?.let { File(it, "nand/system/Contents/registered") },
                        appContext.getExternalFilesDir(null)?.let { File(it, "nand/system/Contents/registered") },
                        DirectoryInitialization.userDirectory?.let { File(it, "nand/system/Contents/registered") },
                        File(android.os.Environment.getExternalStorageDirectory(), "STORM SWITCH/nand/system/Contents/registered")
                    ).distinctBy { it.canonicalPath }

                    for (targetDir in targetFirmwareDirs) {
                        if (targetDir.canonicalPath != primaryNand.canonicalPath) {
                            try {
                                targetDir.mkdirs()
                                primaryNand.copyRecursively(targetDir, overwrite = true)
                            } catch (_: Throwable) {}
                        }
                    }

                    withContext(Dispatchers.Main) {
                        try {
                            androidx.preference.PreferenceManager.getDefaultSharedPreferences(appContext)
                                .edit()
                                .putString("installed_online_firmware_version", asset.version)
                                .apply()
                        } catch (_: Throwable) {}
                        try {
                            NativeLibrary.initializeSystem(true)
                        } catch (_: Throwable) {}
                        homeViewModel.setCheckKeys(true)
                        onInstalled?.invoke()
                        Toast.makeText(
                            appContext,
                            getString(R.string.online_firmware_installed_success, asset.version),
                            Toast.LENGTH_LONG
                        ).show()
                        dismiss()
                    }
                } else {
                    val targetDirs = listOfNotNull(
                        appContext.filesDir?.let { File(it, "keys") },
                        DirectoryInitialization.userDirectory?.let { File(it, "keys") },
                        File(android.os.Environment.getExternalStorageDirectory(), "STORM SWITCH/keys"),
                        appContext.getExternalFilesDir(null)?.let { File(it, "keys") }
                    ).distinctBy { it.canonicalPath }

                    for (kd in targetDirs) {
                        kd.mkdirs()
                    }

                    java.util.zip.ZipFile(tempZip).use { zipFile ->
                        val entries = zipFile.entries()
                        while (isActive && entries.hasMoreElements()) {
                            val entry = entries.nextElement()
                            if (entry.isDirectory) continue
                            val entryName = entry.name.substringAfterLast('/')
                            val isProd = entryName.equals("prod.keys", ignoreCase = true)
                            val isTitle = entryName.equals("title.keys", ignoreCase = true)

                            if (isProd || isTitle) {
                                val rawBytes = zipFile.getInputStream(entry).use { it.readBytes() }
                                val content = if (isProd) {
                                    val header = "# STORM SWITCH KEYS: ${asset.version}\n".toByteArray(Charsets.UTF_8)
                                    header + rawBytes
                                } else {
                                    rawBytes
                                }
                                for (kd in targetDirs) {
                                    try {
                                        val targetFile = File(kd, entryName.lowercase())
                                        FileOutputStream(targetFile).use { outStream ->
                                            outStream.write(content)
                                        }
                                    } catch (_: Throwable) {}
                                }
                            }
                        }
                    }

                    tempZip.delete()

                    withContext(Dispatchers.Main) {
                        try {
                            androidx.preference.PreferenceManager.getDefaultSharedPreferences(appContext)
                                .edit()
                                .putString("installed_online_keys_version", asset.version)
                                .apply()
                        } catch (_: Throwable) {}
                        try {
                            NativeLibrary.reloadKeys()
                            NativeLibrary.initializeSystem(true)
                            gamesViewModel.reloadGames(true)
                        } catch (_: Throwable) {}
                        homeViewModel.setCheckKeys(true)
                        onInstalled?.invoke()
                        Toast.makeText(
                            appContext,
                            getString(R.string.online_keys_installed_success, asset.version),
                            Toast.LENGTH_LONG
                        ).show()
                        dismiss()
                    }
                }
            } catch (e: CancellationException) {
                Log.info("[OnlineTools] Operation cancelled by user")
                if (tempZip.exists()) tempZip.delete()
            } catch (e: Exception) {
                Log.error("[OnlineTools] Download/Install failed: ${e.message}")
                if (tempZip.exists()) tempZip.delete()
                withContext(Dispatchers.Main) {
                    isDownloading = false
                    activeCall = null
                    downloadJob = null
                    if (_binding != null) {
                        binding.btnInstall.isEnabled = true
                        binding.btnInstall.text = "Скачать и установить"
                        binding.btnRefresh.isEnabled = true
                        binding.btnCancel.isEnabled = true
                        binding.progressBar.isIndeterminate = false
                        binding.progressBar.progress = 0
                        val errText = e.localizedMessage ?: "Сбой сети"
                        binding.textStatus.text = "Ошибка установки: $errText"
                        Toast.makeText(appContext, "Сбой онлайн-установки: $errText", Toast.LENGTH_SHORT).show()
                    }
                }
            } finally {
                activeCall = null
                downloadJob = null
            }
        }
    }

    override fun onDestroyView() {
        if (isDownloading) {
            cancelDownload(cleanOnly = true)
        }
        super.onDestroyView()
        _binding = null
    }

    inner class ToolsAdapter : RecyclerView.Adapter<ToolsAdapter.ViewHolder>() {
        inner class ViewHolder(val itemBinding: ListItemOnlineToolBinding) :
            RecyclerView.ViewHolder(itemBinding.root)

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
            val inflater = LayoutInflater.from(parent.context)
            val itemBinding = ListItemOnlineToolBinding.inflate(inflater, parent, false)
            return ViewHolder(itemBinding)
        }

        override fun onBindViewHolder(holder: ViewHolder, position: Int) {
            val asset = assets[position]
            val isSelected = position == selectedIndex
            val isInstalled = isVersionMatching(asset.version, currentInstalledInfo.version)

            holder.itemBinding.apply {
                iconToolType.text = if (toolType == TYPE_FIRMWARE) "📦" else "🔑"
                textToolTitle.text = asset.displayTitle
                textToolSubtitle.text = "Файл: ${asset.name} • Размер: ${asset.displaySize}"

                badgeInstalled.isVisible = isInstalled
                badgeRecommended.isVisible = asset.isRecommended && !isInstalled
                badgeSelected.isVisible = isSelected

                val context = cardToolItem.context
                val primaryColor = ThemeHelper.getColor(context, androidx.appcompat.R.attr.colorPrimary)
                val surfaceVariantColor = ThemeHelper.getColor(context, com.google.android.material.R.attr.colorSurfaceVariant)
                val surfaceColor = ThemeHelper.getColor(context, com.google.android.material.R.attr.colorSurface)
                val outlineColor = ThemeHelper.getColor(context, com.google.android.material.R.attr.colorOutline)
                val onSurfaceColor = ThemeHelper.getColor(context, com.google.android.material.R.attr.colorOnSurface)
                val onSurfaceVariantColor = ThemeHelper.getColor(context, com.google.android.material.R.attr.colorOnSurfaceVariant)

                if (isSelected) {
                    cardToolItem.setCardBackgroundColor(surfaceVariantColor)
                    cardToolItem.strokeColor = primaryColor
                    cardToolItem.strokeWidth = (2 * resources.displayMetrics.density).toInt()
                    textToolTitle.setTextColor(onSurfaceColor)
                    textToolSubtitle.setTextColor(primaryColor)
                } else if (isInstalled) {
                    cardToolItem.setCardBackgroundColor(surfaceColor)
                    cardToolItem.strokeColor = 0xFF10B981.toInt()
                    cardToolItem.strokeWidth = (1.5f * resources.displayMetrics.density).toInt()
                    textToolTitle.setTextColor(onSurfaceColor)
                    textToolSubtitle.setTextColor(onSurfaceVariantColor)
                } else {
                    cardToolItem.setCardBackgroundColor(surfaceColor)
                    cardToolItem.strokeColor = outlineColor
                    cardToolItem.strokeWidth = (1 * resources.displayMetrics.density).toInt()
                    textToolTitle.setTextColor(onSurfaceColor)
                    textToolSubtitle.setTextColor(onSurfaceVariantColor)
                }

                cardToolItem.setOnClickListener {
                    if (isDownloading) return@setOnClickListener
                    val prevIndex = selectedIndex
                    selectedIndex = holder.bindingAdapterPosition
                    notifyItemChanged(prevIndex)
                    notifyItemChanged(selectedIndex)
                    updateSelectionStatus()
                }
            }
        }

        override fun getItemCount(): Int = assets.size
    }
}

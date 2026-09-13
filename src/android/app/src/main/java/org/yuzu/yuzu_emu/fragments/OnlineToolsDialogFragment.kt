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
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.activityViewModels
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
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
import java.io.File
import java.io.FileOutputStream
import java.io.InputStream
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

class OnlineToolsDialogFragment : DialogFragment() {
    private var _binding: DialogOnlineToolsBinding? = null
    private val binding get() = _binding!!

    private val homeViewModel: HomeViewModel by activityViewModels()
    private val gamesViewModel: GamesViewModel by activityViewModels()

    private var toolType: Int = TYPE_FIRMWARE
    private val assets = mutableListOf<OnlineToolAsset>()
    private var selectedIndex = 0
    private var isDownloading = false

    private lateinit var adapter: ToolsAdapter

    private val httpClient by lazy {
        OkHttpClient.Builder()
            .connectTimeout(15, TimeUnit.SECONDS)
            .readTimeout(60, TimeUnit.SECONDS)
            .followRedirects(true)
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

        private fun formatBytes(bytes: Long): String {
            return when {
                bytes >= 1024 * 1024 * 1024 -> String.format("%.1f ГБ", bytes.toDouble() / (1024 * 1024 * 1024))
                bytes >= 1024 * 1024 -> String.format("%.1f МБ", bytes.toDouble() / (1024 * 1024))
                bytes >= 1024 -> String.format("%.1f КБ", bytes.toDouble() / 1024)
                else -> "$bytes Б"
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        toolType = arguments?.getInt(ARG_TYPE, TYPE_FIRMWARE) ?: TYPE_FIRMWARE
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

        binding.btnCloseDialog.setOnClickListener { dismiss() }
        binding.btnCancel.setOnClickListener { dismiss() }
        binding.btnRefresh.setOnClickListener { refreshCatalog() }
        binding.btnInstall.setOnClickListener { startDownloadAndInstall() }

        adapter = ToolsAdapter()
        binding.listTools.layoutManager = LinearLayoutManager(requireContext())
        binding.listTools.adapter = adapter

        populateFallbackCatalog()
        refreshCatalog()
    }

    override fun onStart() {
        super.onStart()
        dialog?.window?.setLayout(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT
        )
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
        selectedIndex = 0
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
        val url = "https://api.github.com/repos/ReiKatari/STORM_SWITCH_TOOLS/releases/tags/$tagName"

        viewLifecycleOwner.lifecycleScope.launch(Dispatchers.IO) {
            try {
                val request = Request.Builder()
                    .url(url)
                    .header("User-Agent", "STORM_SWITCH_Android")
                    .build()
                val response = httpClient.newCall(request).execute()
                if (response.isSuccessful) {
                    val body = response.body?.string()
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
                        if (newAssets.isNotEmpty()) {
                            withContext(Dispatchers.Main) {
                                assets.clear()
                                assets.addAll(newAssets)
                                selectedIndex = 0
                                adapter.notifyDataSetChanged()
                                updateSelectionStatus()
                                binding.textStatus.text = "Каталог успешно обновлен из репозитория."
                            }
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
            binding.textStatus.text = "Выбрано: ${asset.displayTitle} (${asset.displaySize})"
            binding.btnInstall.isEnabled = !isDownloading
        }
    }

    private fun startDownloadAndInstall() {
        if (selectedIndex !in assets.indices || isDownloading) return
        val asset = assets[selectedIndex]

        isDownloading = true
        binding.btnInstall.isEnabled = false
        binding.btnRefresh.isEnabled = false
        binding.progressBar.isIndeterminate = false
        binding.progressBar.progress = 0

        viewLifecycleOwner.lifecycleScope.launch(Dispatchers.IO) {
            val cacheDir = requireContext().cacheDir
            val tempZip = File(cacheDir, if (toolType == TYPE_FIRMWARE) "firmware_stream.tmp" else "keys_stream.tmp")

            try {
                withContext(Dispatchers.Main) {
                    binding.textStatus.text = "Подключение и загрузка ${asset.name}..."
                }

                val request = Request.Builder()
                    .url(asset.downloadUrl)
                    .header("User-Agent", "STORM_SWITCH_Installer")
                    .build()
                val response = httpClient.newCall(request).execute()
                if (!response.isSuccessful) {
                    throw Exception("Ошибка загрузки HTTP ${response.code}")
                }

                val body = response.body ?: throw Exception("Пустой ответ сервера")
                val totalLength = body.contentLength()
                var downloaded = 0L

                body.byteStream().use { input ->
                    FileOutputStream(tempZip).use { output ->
                        val buffer = ByteArray(32768)
                        var read: Int
                        var lastReportTime = System.currentTimeMillis()

                        while (input.read(buffer).also { read = it } != -1) {
                            output.write(buffer, 0, read)
                            downloaded += read

                            val now = System.currentTimeMillis()
                            if (now - lastReportTime > 200) {
                                lastReportTime = now
                                val progress = if (totalLength > 0) ((downloaded * 100) / totalLength).toInt() else 0
                                withContext(Dispatchers.Main) {
                                    binding.progressBar.progress = progress
                                    binding.textStatus.text = "Загрузка: $progress% (${formatBytes(downloaded)} / ${formatBytes(totalLength)})"
                                }
                            }
                        }
                    }
                }

                withContext(Dispatchers.Main) {
                    binding.progressBar.isIndeterminate = true
                    binding.textStatus.text = "Распаковка и установка компонентов в систему..."
                }

                if (toolType == TYPE_FIRMWARE) {
                    // Extract NCA files directly to temp directory then move to NAND registered
                    val tempExtract = File(cacheDir, "extracted_firmware")
                    if (tempExtract.exists()) tempExtract.deleteRecursively()
                    tempExtract.mkdirs()

                    java.util.zip.ZipFile(tempZip).use { zipFile ->
                        val entries = zipFile.entries()
                        while (entries.hasMoreElements()) {
                            val entry = entries.nextElement()
                            if (entry.isDirectory) continue
                            val entryName = entry.name.substringAfterLast('/')
                            if (!entryName.endsWith(".nca", ignoreCase = true)) continue

                            val outFile = File(tempExtract, entryName)
                            zipFile.getInputStream(entry).use { inStream ->
                                FileOutputStream(outFile).use { outStream ->
                                    inStream.copyTo(outStream)
                                }
                            }
                        }
                    }

                    val nandFirmwareDir = File(NativeConfig.getNandDir() + "/system/Contents/registered/")
                    if (nandFirmwareDir.exists()) {
                        nandFirmwareDir.deleteRecursively()
                    }
                    nandFirmwareDir.mkdirs()

                    tempExtract.copyRecursively(nandFirmwareDir, overwrite = true)
                    tempExtract.deleteRecursively()
                    tempZip.delete()

                    withContext(Dispatchers.Main) {
                        NativeLibrary.initializeSystem(true)
                        homeViewModel.setCheckKeys(true)
                        Toast.makeText(requireContext(), "✅ Прошивка ${asset.version} успешно установлена онлайн!", Toast.LENGTH_LONG).show()
                        dismiss()
                    }
                } else {
                    // Keys installation: extract prod.keys and title.keys directly to userDirectory/keys/
                    val keysDir = File(DirectoryInitialization.userDirectory, "keys")
                    if (!keysDir.exists()) keysDir.mkdirs()

                    java.util.zip.ZipFile(tempZip).use { zipFile ->
                        val entries = zipFile.entries()
                        while (entries.hasMoreElements()) {
                            val entry = entries.nextElement()
                            if (entry.isDirectory) continue
                            val entryName = entry.name.substringAfterLast('/')
                            val isProd = entryName.equals("prod.keys", ignoreCase = true)
                            val isTitle = entryName.equals("title.keys", ignoreCase = true)

                            if (isProd || isTitle) {
                                val targetFile = File(keysDir, entryName.lowercase())
                                zipFile.getInputStream(entry).use { inStream ->
                                    FileOutputStream(targetFile).use { outStream ->
                                        inStream.copyTo(outStream)
                                    }
                                }
                            }
                        }
                    }

                    tempZip.delete()

                    withContext(Dispatchers.Main) {
                        NativeLibrary.reloadKeys()
                        NativeLibrary.initializeSystem(true)
                        gamesViewModel.reloadGames(true)
                        homeViewModel.setCheckKeys(true)
                        Toast.makeText(requireContext(), "✅ Ключи ${asset.version} успешно установлены онлайн!", Toast.LENGTH_LONG).show()
                        dismiss()
                    }
                }
            } catch (e: Exception) {
                Log.error("[OnlineTools] Download/Install failed: ${e.message}")
                if (tempZip.exists()) tempZip.delete()
                withContext(Dispatchers.Main) {
                    isDownloading = false
                    binding.btnInstall.isEnabled = true
                    binding.btnRefresh.isEnabled = true
                    binding.progressBar.isIndeterminate = false
                    binding.textStatus.text = "Ошибка установки: ${e.localizedMessage ?: "Сбой сети"}"
                    Toast.makeText(requireContext(), "Сбой онлайн-установки: ${e.message}", Toast.LENGTH_SHORT).show()
                }
            }
        }
    }

    override fun onDestroyView() {
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

            holder.itemBinding.apply {
                iconToolType.text = if (toolType == TYPE_FIRMWARE) "📦" else "🔑"
                textToolTitle.text = asset.displayTitle
                textToolSubtitle.text = "Файл: ${asset.name} • Размер: ${asset.displaySize}"

                badgeRecommended.isVisible = asset.isRecommended
                badgeSelected.isVisible = isSelected

                if (isSelected) {
                    cardToolItem.setCardBackgroundColor(Color.parseColor("#162A3F"))
                    cardToolItem.strokeColor = Color.parseColor("#00F0FF")
                    cardToolItem.strokeWidth = (2 * resources.displayMetrics.density).toInt()
                    textToolTitle.setTextColor(Color.parseColor("#FFFFFF"))
                    textToolSubtitle.setTextColor(Color.parseColor("#38BDF8"))
                } else {
                    cardToolItem.setCardBackgroundColor(Color.parseColor("#151922"))
                    cardToolItem.strokeColor = Color.parseColor("#232B3B")
                    cardToolItem.strokeWidth = (1 * resources.displayMetrics.density).toInt()
                    textToolTitle.setTextColor(Color.parseColor("#CBD5E1"))
                    textToolSubtitle.setTextColor(Color.parseColor("#64748B"))
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

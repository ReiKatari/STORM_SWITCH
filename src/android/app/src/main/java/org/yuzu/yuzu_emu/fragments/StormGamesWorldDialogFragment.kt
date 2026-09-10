// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.app.Dialog
import android.net.Uri
import android.os.Bundle
import android.os.Environment
import android.text.Editable
import android.text.TextWatcher
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.core.view.isVisible
import androidx.documentfile.provider.DocumentFile
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.activityViewModels
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import coil.load
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import okhttp3.Call
import okhttp3.OkHttpClient
import okhttp3.Request
import org.json.JSONArray
import org.json.JSONObject
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.databinding.DialogStormGamesWorldBinding
import org.yuzu.yuzu_emu.databinding.ListItemStormWorldGameBinding
import org.yuzu.yuzu_emu.model.GamesViewModel
import org.yuzu.yuzu_emu.utils.FileUtil
import org.yuzu.yuzu_emu.utils.Log
import org.yuzu.yuzu_emu.utils.NativeConfig
import java.io.File
import java.io.FileOutputStream
import java.io.InputStream
import java.io.OutputStream
import java.util.Locale
import java.util.concurrent.TimeUnit

data class StormWorldGameItem(
    val id: Int,
    val title: String,
    val finalTitle: String,
    val version: String,
    val serialId: String,
    val size: String,
    val cover: String,
    val fileExists: Boolean,
    val hasFile: Boolean,
    val regions: List<String>,
    val textLangs: List<String>,
    var description: String = "",
    var fileSizeBytes: Long = 0L,
    var realExtension: String = ".nsp",
    var isDownloaded: Boolean = false
)

class StormGamesWorldDialogFragment : DialogFragment() {

    private var _binding: DialogStormGamesWorldBinding? = null
    private val binding get() = _binding!!

    private val gamesViewModel: GamesViewModel by activityViewModels()

    private val allGames = mutableListOf<StormWorldGameItem>()
    private val filteredGames = mutableListOf<StormWorldGameItem>()
    private var selectedGame: StormWorldGameItem? = null

    private var activeDownloadCall: Call? = null
    private var isDownloading = false

    private val httpClient = OkHttpClient.Builder()
        .connectTimeout(30, TimeUnit.SECONDS)
        .readTimeout(60, TimeUnit.SECONDS)
        .build()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setStyle(STYLE_NORMAL, R.style.Theme_Yuzu_Main)
    }

    override fun onStart() {
        super.onStart()
        dialog?.window?.setLayout(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.MATCH_PARENT
        )
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = DialogStormGamesWorldBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        binding.recyclerGames.layoutManager = LinearLayoutManager(requireContext())
        binding.recyclerGames.adapter = GamesAdapter()

        binding.btnClose.setOnClickListener {
            if (isDownloading) {
                Toast.makeText(requireContext(), "Идёт скачивание игры. Отмените перед закрытием.", Toast.LENGTH_SHORT).show()
            } else {
                dismiss()
            }
        }

        binding.btnRefresh.setOnClickListener {
            fetchCatalog()
        }

        binding.editSearch.addTextChangedListener(object : TextWatcher {
            override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) {}
            override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) {
                filterGames(s?.toString().orEmpty())
            }
            override fun afterTextChanged(s: Editable?) {}
        })

        binding.btnStartDownload.setOnClickListener {
            val game = selectedGame ?: return@setOnClickListener
            startDownload(game)
        }

        binding.btnCancelDownload.setOnClickListener {
            cancelDownload()
        }

        fetchCatalog()
    }

    private fun fetchCatalog() {
        binding.progressLoading.isVisible = true
        binding.textEmpty.isVisible = false
        binding.btnRefresh.isEnabled = false

        lifecycleScope.launch(Dispatchers.IO) {
            try {
                val req = Request.Builder()
                    .url("https://stormgamesworld.ru/api/games/index")
                    .header("User-Agent", "STORM_SWITCH/8.0.4 (Android)")
                    .build()

                val resp = httpClient.newCall(req).execute()
                val body = resp.body?.string().orEmpty()

                val jsonArray = JSONArray(body)
                val parsed = mutableListOf<StormWorldGameItem>()

                for (i in 0 until jsonArray.length()) {
                    val obj = jsonArray.optJSONObject(i) ?: continue
                    val platform = obj.optString("platformName")
                    val platformType = obj.optString("platformTypeName")
                    val fileExists = obj.optBoolean("fileExists", false)
                    val hasFile = obj.optBoolean("hasFile", false)

                    if (platform == "Nintendo Switch" && platformType == "CONSOLES" && (fileExists || hasFile)) {
                        val regList = mutableListOf<String>()
                        val regArr = obj.optJSONArray("regions")
                        if (regArr != null) {
                            for (r in 0 until regArr.length()) regList.add(regArr.optString(r))
                        }

                        val langList = mutableListOf<String>()
                        val langArr = obj.optJSONArray("textLangs")
                        if (langArr != null) {
                            for (l in 0 until langArr.length()) langList.add(langArr.optString(l))
                        }

                        parsed.add(
                            StormWorldGameItem(
                                id = obj.optInt("id"),
                                title = obj.optString("title"),
                                finalTitle = obj.optString("finalTitle"),
                                version = obj.optString("version", "1.0.0"),
                                serialId = obj.optString("serialId"),
                                size = obj.optString("size", "—"),
                                cover = obj.optString("cover"),
                                fileExists = fileExists,
                                hasFile = hasFile,
                                regions = regList,
                                textLangs = langList
                            )
                        )
                    }
                }

                // Check local files for downloaded status
                checkDownloadedStatus(parsed)

                withContext(Dispatchers.Main) {
                    if (_binding == null) return@withContext
                    allGames.clear()
                    allGames.addAll(parsed)
                    filterGames(binding.editSearch.text?.toString().orEmpty())
                    binding.progressLoading.isVisible = false
                    binding.btnRefresh.isEnabled = true
                    binding.textCatalogStatus.text = "Доступно игр Nintendo Switch: ${allGames.size}"
                }
            } catch (e: Exception) {
                Log.error("[StormGamesWorld] Catalog fetch error: ${e.message}")
                withContext(Dispatchers.Main) {
                    if (_binding == null) return@withContext
                    binding.progressLoading.isVisible = false
                    binding.btnRefresh.isEnabled = true
                    binding.textEmpty.isVisible = true
                    binding.textEmpty.text = "Ошибка подключения: ${e.localizedMessage ?: "Сбой сети"}"
                    Toast.makeText(requireContext(), "Не удалось загрузить каталог", Toast.LENGTH_SHORT).show()
                }
            }
        }
    }

    private fun checkDownloadedStatus(games: List<StormWorldGameItem>) {
        val gameDirs = NativeConfig.getGameDirs()
        val allFiles = mutableListOf<String>()

        for (dir in gameDirs) {
            try {
                val uri = Uri.parse(dir.uriString)
                if (uri.scheme == "content") {
                    val rootDoc = DocumentFile.fromTreeUri(requireContext(), uri)
                    rootDoc?.listFiles()?.forEach { f ->
                        if (f.isFile) allFiles.add(f.name.orEmpty().lowercase(Locale.ROOT))
                    }
                } else {
                    val path = uri.path ?: dir.uriString
                    File(path).listFiles()?.forEach { f ->
                        if (f.isFile) allFiles.add(f.name.lowercase(Locale.ROOT))
                    }
                }
            } catch (_: Exception) {}
        }

        games.forEach { g ->
            val tid = g.serialId.lowercase(Locale.ROOT)
            val title = (if (g.finalTitle.isNotEmpty()) g.finalTitle else g.title).lowercase(Locale.ROOT)
            g.isDownloaded = allFiles.any { name ->
                (tid.isNotEmpty() && name.contains(tid)) || (name.contains(title) && (name.endsWith(".nsp") || name.endsWith(".xci") || name.endsWith(".nsz")))
            }
        }
    }

    private fun filterGames(query: String) {
        val q = query.trim().lowercase(Locale.ROOT)
        filteredGames.clear()
        if (q.isEmpty()) {
            filteredGames.addAll(allGames)
        } else {
            for (g in allGames) {
                val matchTitle = g.title.lowercase(Locale.ROOT).contains(q)
                val matchFinal = g.finalTitle.lowercase(Locale.ROOT).contains(q)
                val matchTid = g.serialId.lowercase(Locale.ROOT).contains(q)
                if (matchTitle || matchFinal || matchTid) {
                    filteredGames.add(g)
                }
            }
        }
        binding.recyclerGames.adapter?.notifyDataSetChanged()
        binding.textEmpty.isVisible = filteredGames.isEmpty()
    }

    private fun onGameSelected(game: StormWorldGameItem) {
        selectedGame = game
        binding.cardDetailsPanel.isVisible = true

        val dispTitle = if (game.finalTitle.isNotEmpty()) game.finalTitle else game.title
        binding.detailGameTitle.text = dispTitle
        binding.detailGameMeta.text = "Версия: ${if (game.version.isNotEmpty()) game.version else "1.0.0"} • Размер: ${game.size}"
        binding.detailGameId.text = if (game.serialId.isNotEmpty()) "ID: ${game.serialId}" else ""
        binding.detailGameDescription.text = "Загрузка информации..."

        binding.detailGameCover.load(game.cover) {
            placeholder(R.drawable.default_icon)
            error(R.drawable.default_icon)
        }

        val targetDir = getTargetDownloadDirectoryDescription()
        binding.detailSaveFolder.text = "📁 Каталог: $targetDir"

        if (game.isDownloaded) {
            binding.btnStartDownload.text = "Уже скачано"
            binding.btnStartDownload.isEnabled = false
        } else {
            binding.btnStartDownload.text = "Скачать игру"
            binding.btnStartDownload.isEnabled = !isDownloading
        }

        fetchGameDetails(game)
    }

    private fun fetchGameDetails(game: StormWorldGameItem) {
        lifecycleScope.launch(Dispatchers.IO) {
            try {
                val req = Request.Builder()
                    .url("https://stormgamesworld.ru/api/games?id=${game.id}")
                    .header("User-Agent", "STORM_SWITCH/8.0.4 (Android)")
                    .build()
                val resp = httpClient.newCall(req).execute()
                val body = resp.body?.string().orEmpty()
                val obj = JSONObject(body)

                val desc = obj.optString("description", "")
                val bytes = obj.optLong("fileSizeBytes", 0L)

                game.description = desc
                game.fileSizeBytes = bytes

                // Also check HEAD Content-Disposition to detect real extension (.nsz / .xci / .nsp)
                try {
                    val headReq = Request.Builder()
                        .url("https://stormgamesworld.ru/api/games/${game.id}/download")
                        .head()
                        .header("User-Agent", "STORM_SWITCH/8.0.4 (Android)")
                        .build()
                    val headResp = httpClient.newCall(headReq).execute()
                    val disp = headResp.header("Content-Disposition").orEmpty().lowercase(Locale.ROOT)
                    if (disp.contains(".nsz")) game.realExtension = ".nsz"
                    else if (disp.contains(".xci")) game.realExtension = ".xci"
                    else if (disp.contains(".nsp")) game.realExtension = ".nsp"
                } catch (_: Exception) {}

                withContext(Dispatchers.Main) {
                    if (_binding == null || selectedGame?.id != game.id) return@withContext
                    binding.detailGameDescription.text = if (desc.isNotEmpty()) desc else "Описание отсутствует"
                }
            } catch (_: Exception) {
                withContext(Dispatchers.Main) {
                    if (_binding == null || selectedGame?.id != game.id) return@withContext
                    binding.detailGameDescription.text = "Описание отсутствует"
                }
            }
        }
    }

    private fun getTargetDownloadDirectoryDescription(): String {
        val gameDirs = NativeConfig.getGameDirs()
        val first = gameDirs.firstOrNull()
        if (first != null) {
            val uri = Uri.parse(first.uriString)
            return uri.lastPathSegment ?: uri.toString()
        }
        val defaultDir = File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS), "STORM_SWITCH_GAMES")
        return defaultDir.name
    }

    private fun startDownload(game: StormWorldGameItem) {
        if (isDownloading) return

        isDownloading = true
        binding.btnStartDownload.isEnabled = false
        binding.layoutDownloadProgress.isVisible = true
        binding.progressDownload.isIndeterminate = true
        binding.textDownloadStats.text = "Подключение к серверу загрузки..."

        lifecycleScope.launch(Dispatchers.IO) {
            var outputStream: OutputStream? = null
            var targetDocFile: DocumentFile? = null
            var targetNormalFile: File? = null

            try {
                val cleanTitle = (if (game.finalTitle.isNotEmpty()) game.finalTitle else game.title)
                    .replace(Regex("[\\\\/:*?\"<>|]"), "_")
                    .trim()
                val filename = if (cleanTitle.endsWith(".nsp", ignoreCase = true) ||
                    cleanTitle.endsWith(".xci", ignoreCase = true) ||
                    cleanTitle.endsWith(".nsz", ignoreCase = true)) {
                    cleanTitle
                } else {
                    "$cleanTitle${game.realExtension}"
                }

                val gameDirs = NativeConfig.getGameDirs()
                val firstDir = gameDirs.firstOrNull()

                if (firstDir != null) {
                    val dirUri = Uri.parse(firstDir.uriString)
                    if (dirUri.scheme == "content") {
                        val tree = DocumentFile.fromTreeUri(requireContext(), dirUri)
                        targetDocFile = tree?.createFile("application/octet-stream", filename)
                        if (targetDocFile != null) {
                            outputStream = requireContext().contentResolver.openOutputStream(targetDocFile.uri)
                        }
                    } else {
                        val p = dirUri.path ?: firstDir.uriString
                        val folder = File(p)
                        if (!folder.exists()) folder.mkdirs()
                        targetNormalFile = File(folder, filename)
                        outputStream = FileOutputStream(targetNormalFile)
                    }
                }

                if (outputStream == null) {
                    val defaultFolder = File(
                        Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS),
                        "STORM_SWITCH_GAMES"
                    )
                    if (!defaultFolder.exists()) defaultFolder.mkdirs()
                    targetNormalFile = File(defaultFolder, filename)
                    outputStream = FileOutputStream(targetNormalFile)
                }

                val downloadUrl = "https://stormgamesworld.ru/api/games/${game.id}/download"
                val req = Request.Builder()
                    .url(downloadUrl)
                    .header("User-Agent", "STORM_SWITCH/8.0.4 (Android)")
                    .build()

                val call = httpClient.newCall(req)
                activeDownloadCall = call
                val resp = call.execute()

                if (!resp.isSuccessful) {
                    throw RuntimeException("HTTP ${resp.code}: ${resp.message}")
                }

                val body = resp.body ?: throw RuntimeException("Empty response body")
                val totalBytes = if (body.contentLength() > 0) body.contentLength() else game.fileSizeBytes
                val inputStream: InputStream = body.byteStream()

                val buffer = ByteArray(64 * 1024)
                var bytesRead: Int
                var totalRead = 0L
                var lastUpdateTime = System.currentTimeMillis()
                var bytesSinceLastUpdate = 0L
                var currentSpeedMbps = 0.0

                withContext(Dispatchers.Main) {
                    binding.progressDownload.isIndeterminate = false
                }

                while (inputStream.read(buffer).also { bytesRead = it } != -1) {
                    outputStream.write(buffer, 0, bytesRead)
                    totalRead += bytesRead
                    bytesSinceLastUpdate += bytesRead

                    val now = System.currentTimeMillis()
                    val delta = now - lastUpdateTime
                    if (delta >= 600) {
                        currentSpeedMbps = (bytesSinceLastUpdate.toDouble() / (1024.0 * 1024.0)) / (delta.toDouble() / 1000.0)
                        bytesSinceLastUpdate = 0L
                        lastUpdateTime = now

                        val pct = if (totalBytes > 0) ((totalRead * 100) / totalBytes).toInt() else 0
                        val recMb = totalRead.toDouble() / (1024.0 * 1024.0)
                        val totMb = if (totalBytes > 0) totalBytes.toDouble() / (1024.0 * 1024.0) else 0.0
                        val remMb = (totMb - recMb).coerceAtLeast(0.0)
                        val etaSec = if (currentSpeedMbps > 0.05) (remMb / currentSpeedMbps).toInt() else 0

                        val statsText = if (totMb > 0) {
                            String.format(
                                Locale.US,
                                "%.1f МБ из %.1f МБ (%.0f%%) • %.2f МБ/с • Ост: %02d:%02d",
                                recMb, totMb, pct.toDouble(), currentSpeedMbps, etaSec / 60, etaSec % 60
                            )
                        } else {
                            String.format(Locale.US, "%.1f МБ • %.2f МБ/с", recMb, currentSpeedMbps)
                        }

                        withContext(Dispatchers.Main) {
                            if (_binding != null) {
                                binding.progressDownload.progress = pct
                                binding.textDownloadStats.text = statsText
                            }
                        }
                    }
                }

                outputStream.flush()
                outputStream.close()
                outputStream = null

                withContext(Dispatchers.Main) {
                    isDownloading = false
                    activeDownloadCall = null
                    game.isDownloaded = true
                    if (_binding != null) {
                        binding.layoutDownloadProgress.isVisible = false
                        binding.btnStartDownload.text = "Уже скачано"
                        binding.btnStartDownload.isEnabled = false
                        binding.recyclerGames.adapter?.notifyDataSetChanged()
                    }
                    Toast.makeText(requireContext(), "✅ Игра успешно скачана: ${game.finalTitle.ifEmpty { game.title }}", Toast.LENGTH_LONG).show()
                    gamesViewModel.reloadGames(directoriesChanged = true)
                }

            } catch (e: Exception) {
                try { outputStream?.close() } catch (_: Exception) {}
                try { targetDocFile?.delete() } catch (_: Exception) {}
                try { targetNormalFile?.delete() } catch (_: Exception) {}

                withContext(Dispatchers.Main) {
                    isDownloading = false
                    activeDownloadCall = null
                    if (_binding != null) {
                        binding.layoutDownloadProgress.isVisible = false
                        binding.btnStartDownload.isEnabled = true
                    }
                    if (e.message != "Socket closed" && e.message != "Canceled") {
                        Log.error("[StormGamesWorld] Download failed: ${e.message}")
                        Toast.makeText(requireContext(), "Ошибка загрузки: ${e.localizedMessage ?: "Сбой сети"}", Toast.LENGTH_SHORT).show()
                    }
                }
            }
        }
    }

    private fun cancelDownload() {
        if (!isDownloading) return
        activeDownloadCall?.cancel()
        activeDownloadCall = null
        isDownloading = false
        binding.layoutDownloadProgress.isVisible = false
        binding.btnStartDownload.isEnabled = true
        Toast.makeText(requireContext(), "Загрузка отменена", Toast.LENGTH_SHORT).show()
    }

    override fun onDestroyView() {
        if (isDownloading) {
            activeDownloadCall?.cancel()
            activeDownloadCall = null
        }
        super.onDestroyView()
        _binding = null
    }

    private inner class GamesAdapter : RecyclerView.Adapter<GamesAdapter.ViewHolder>() {

        inner class ViewHolder(val b: ListItemStormWorldGameBinding) : RecyclerView.ViewHolder(b.root)

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
            val b = ListItemStormWorldGameBinding.inflate(layoutInflater, parent, false)
            return ViewHolder(b)
        }

        override fun onBindViewHolder(holder: ViewHolder, position: Int) {
            val item = filteredGames[position]
            val dispTitle = if (item.finalTitle.isNotEmpty()) item.finalTitle else item.title

            holder.b.textGameTitle.text = dispTitle
            holder.b.textGameVersion.text = if (item.version.isNotEmpty()) "v${item.version.removePrefix("v").removePrefix("V")}" else "v1.0.0"
            holder.b.textGameSize.text = item.size
            holder.b.textGameLangs.text = if (item.textLangs.isNotEmpty()) item.textLangs.joinToString(", ") else "Multi"
            holder.b.textGameSerial.text = item.serialId

            holder.b.imageGameCover.load(item.cover) {
                placeholder(R.drawable.default_icon)
                error(R.drawable.default_icon)
            }

            if (item.isDownloaded) {
                holder.b.textDownloadStatus.text = "✅ Скачано"
                holder.b.textDownloadStatus.setTextColor(0xFF00FF66.toInt())
            } else {
                holder.b.textDownloadStatus.text = "⬇️ Скачать"
                holder.b.textDownloadStatus.setTextColor(0xFF00D2FF.toInt())
            }

            val isSelected = selectedGame?.id == item.id
            holder.b.root.strokeColor = if (isSelected) 0xFF00F0FF.toInt() else 0xFF20354E.toInt()
            holder.b.root.strokeWidth = if (isSelected) 2 else 1

            holder.b.root.setOnClickListener {
                onGameSelected(item)
                notifyDataSetChanged()
            }
        }

        override fun getItemCount(): Int = filteredGames.size
    }

    companion object {
        const val TAG = "StormGamesWorldDialogFragment"

        fun newInstance(): StormGamesWorldDialogFragment {
            return StormGamesWorldDialogFragment()
        }
    }
}
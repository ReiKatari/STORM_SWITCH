// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.app.Dialog
import android.content.Context
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
import kotlinx.coroutines.flow.collectLatest
import org.yuzu.yuzu_emu.services.StormDownloadManager
import org.yuzu.yuzu_emu.services.StormDownloadProgress
import org.yuzu.yuzu_emu.services.StormDownloadStatus
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async
import kotlinx.coroutines.awaitAll
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import okhttp3.Call
import okhttp3.ConnectionPool
import okhttp3.Dispatcher
import okhttp3.OkHttpClient
import okhttp3.Protocol
import okhttp3.Request
import org.json.JSONArray
import org.json.JSONObject
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.databinding.DialogStormGamesWorldBinding
import org.yuzu.yuzu_emu.databinding.DialogStormWorldDlcListBinding
import org.yuzu.yuzu_emu.databinding.ListItemStormWorldDlcBinding
import org.yuzu.yuzu_emu.databinding.ListItemStormWorldGameBinding
import org.yuzu.yuzu_emu.model.GamesViewModel
import org.yuzu.yuzu_emu.utils.FileUtil
import org.yuzu.yuzu_emu.utils.Log
import org.yuzu.yuzu_emu.utils.NativeConfig
import org.yuzu.yuzu_emu.utils.ThemeHelper
import java.io.BufferedInputStream
import java.io.BufferedOutputStream
import java.io.File
import java.io.FileOutputStream
import java.io.InputStream
import java.io.OutputStream
import java.util.Locale
import java.util.concurrent.TimeUnit

data class StormWorldDlcItem(
    val id: String,
    val name: String,
    val description: String = ""
)

data class StormWorldGameItem(
    val id: Int,
    val title: String,
    val finalTitle: String,
    val version: String,
    val internalVersion: String,
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
    var isDownloaded: Boolean = false,
    var dlcCount: Int = 0,
    var modCount: Int = 0,
    var isRecommended: Boolean = false,
    val dlcs: MutableList<StormWorldDlcItem> = mutableListOf()
) {
    fun toJson(): JSONObject {
        val obj = JSONObject()
        obj.put("id", id)
        obj.put("title", title)
        obj.put("finalTitle", finalTitle)
        obj.put("version", version)
        obj.put("internalVersion", internalVersion)
        obj.put("serialId", serialId)
        obj.put("size", size)
        obj.put("cover", cover)
        obj.put("fileExists", fileExists)
        obj.put("hasFile", hasFile)
        obj.put("regions", JSONArray(regions))
        obj.put("textLangs", JSONArray(textLangs))
        obj.put("description", description)
        obj.put("fileSizeBytes", fileSizeBytes)
        obj.put("realExtension", realExtension)
        obj.put("dlcCount", dlcCount)
        obj.put("modCount", modCount)
        obj.put("isRecommended", isRecommended)
        return obj
    }

    companion object {
        fun fromJson(obj: JSONObject): StormWorldGameItem {
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
            return StormWorldGameItem(
                id = obj.optInt("id"),
                title = obj.optString("title"),
                finalTitle = obj.optString("finalTitle"),
                version = obj.optString("version"),
                internalVersion = obj.optString("internalVersion"),
                serialId = obj.optString("serialId"),
                size = obj.optString("size"),
                cover = obj.optString("cover"),
                fileExists = obj.optBoolean("fileExists", true),
                hasFile = obj.optBoolean("hasFile", true),
                regions = regList,
                textLangs = langList,
                description = obj.optString("description"),
                fileSizeBytes = obj.optLong("fileSizeBytes"),
                realExtension = obj.optString("realExtension", ".nsp"),
                dlcCount = obj.optInt("dlcCount"),
                modCount = obj.optInt("modCount"),
                isRecommended = obj.optBoolean("isRecommended", false)
            )
        }
    }
}

class StormGamesWorldDialogFragment : DialogFragment() {

    private var _binding: DialogStormGamesWorldBinding? = null
    private val binding get() = _binding!!

    private val gamesViewModel: GamesViewModel by activityViewModels()

    enum class SortMode(val titleRes: Int, val labelRu: String, val labelEn: String) {
        TITLE_ASC(R.string.sort_by_title_asc, "А-Я", "A-Z"),
        TITLE_DESC(R.string.sort_by_title_desc, "Я-А", "Z-A"),
        SIZE_DESC(R.string.sort_by_size_desc, "Размер ↓", "Size ↓"),
        SIZE_ASC(R.string.sort_by_size_asc, "Размер ↑", "Size ↑"),
        RECOMMENDED(R.string.sort_by_addons_mods, "DLC и моды", "DLC and mods");

        val label: String
            get() = labelRu

        fun getLabel(context: Context): String {
            val lang = context.resources.configuration.locales[0].language
            return if (lang == "ru") labelRu else labelEn
        }
    }

    private val allGames = mutableListOf<StormWorldGameItem>()
    private val filteredAndSortedGames = mutableListOf<StormWorldGameItem>()
    private val pagedGames = mutableListOf<StormWorldGameItem>()
    private var currentSortMode = SortMode.TITLE_ASC
    private var currentPage = 1
    private val pageSize = 25
    private var selectedGame: StormWorldGameItem? = null



    private val httpClient get() = sharedHttpClient


    private fun loadCoverForGame(item: StormWorldGameItem, imageView: android.widget.ImageView) {
        val localGame = gamesViewModel.games.value.firstOrNull { it.programIdHex.equals(item.serialId, ignoreCase = true) }
        if (localGame != null) {
            org.yuzu.yuzu_emu.utils.GameIconUtils.loadGameIcon(localGame, imageView)
            return
        }
        val tid = item.serialId.uppercase(Locale.ROOT)
        val url = if (item.cover.isNotBlank() && item.cover.startsWith("http")) {
            item.cover
        } else {
            SWITCH_CDN_ICONS[tid] ?: if (tid.length == 16) "https://raw.githubusercontent.com/blawar/titledb/master/icons/$tid.jpg" else null
        }
        if (!url.isNullOrBlank()) {
            imageView.load(url) {
                crossfade(true)
                placeholder(R.drawable.default_icon)
                error(R.drawable.default_icon)
            }
        } else {
            imageView.setImageResource(R.drawable.default_icon)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setStyle(STYLE_NORMAL, ThemeHelper.getSelectedStaticThemeColor())
    }

    override fun onStart() {
        super.onStart()
        dialog?.window?.let { window ->
            val dm = resources.displayMetrics
            val isLandscape = resources.configuration.orientation == android.content.res.Configuration.ORIENTATION_LANDSCAPE
            val width = if (isLandscape) (dm.widthPixels * 0.94).toInt() else ViewGroup.LayoutParams.MATCH_PARENT
            val height = ViewGroup.LayoutParams.MATCH_PARENT
            window.setLayout(width, height)
            window.setGravity(android.view.Gravity.CENTER)
        }
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

        androidx.core.view.ViewCompat.setOnApplyWindowInsetsListener(binding.root) { _, insets ->
            val systemBars = insets.getInsets(
                androidx.core.view.WindowInsetsCompat.Type.systemBars() or
                androidx.core.view.WindowInsetsCompat.Type.displayCutout()
            )
            binding.topBar.setPadding(
                binding.topBar.paddingLeft,
                systemBars.top,
                binding.topBar.paddingRight,
                binding.topBar.paddingBottom
            )
            val isLandscape = resources.configuration.orientation == android.content.res.Configuration.ORIENTATION_LANDSCAPE
            if (isLandscape) {
                binding.root.setPadding(systemBars.left, 0, systemBars.right, systemBars.bottom)
            } else {
                binding.root.setPadding(0, 0, 0, systemBars.bottom)
            }
            insets
        }

        binding.recyclerGames.layoutManager = LinearLayoutManager(requireContext())
        binding.recyclerGames.adapter = GamesAdapter()

        binding.btnClose.setOnClickListener {
            if (StormDownloadManager.isDownloading()) {
                Toast.makeText(requireContext(), "Загрузка игры продолжается в фоновом режиме", Toast.LENGTH_SHORT).show()
            }
            dismiss()
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
            StormDownloadManager.startDownload(requireContext(), game)
        }

        binding.btnPauseDownload.setOnClickListener {
            StormDownloadManager.togglePause(requireContext())
        }

        binding.btnCancelDownload.setOnClickListener {
            StormDownloadManager.cancelDownload(requireContext())
        }

        binding.btnPagePrev.setOnClickListener {
            if (currentPage > 1) {
                currentPage--
                applySortAndPagination(resetPage = false)
                binding.recyclerGames.scrollToPosition(0)
            }
        }

        binding.btnPageNext.setOnClickListener {
            val totalItems = filteredAndSortedGames.size
            val totalPages = if (totalItems > 0) ((totalItems - 1) / pageSize) + 1 else 1
            if (currentPage < totalPages) {
                currentPage++
                applySortAndPagination(resetPage = false)
                binding.recyclerGames.scrollToPosition(0)
            }
        }

        binding.btnSortCatalog.setOnClickListener {
            val sortOptions = arrayOf(
                getString(R.string.sort_by_title_asc),
                getString(R.string.sort_by_title_desc),
                getString(R.string.sort_by_size_desc),
                getString(R.string.sort_by_size_asc),
                getString(R.string.sort_by_addons_mods)
            )
            val selectedIndex = when (currentSortMode) {
                SortMode.TITLE_ASC -> 0
                SortMode.TITLE_DESC -> 1
                SortMode.SIZE_DESC -> 2
                SortMode.SIZE_ASC -> 3
                SortMode.RECOMMENDED -> 4
            }
            val themeContext = androidx.appcompat.view.ContextThemeWrapper(
                requireContext(),
                ThemeHelper.getSelectedStaticThemeColor()
            )
            com.google.android.material.dialog.MaterialAlertDialogBuilder(themeContext)
                .setTitle(R.string.sort_games_title)
                .setSingleChoiceItems(sortOptions, selectedIndex) { dialog: android.content.DialogInterface, which: Int ->
                    currentSortMode = when (which) {
                        0 -> SortMode.TITLE_ASC
                        1 -> SortMode.TITLE_DESC
                        2 -> SortMode.SIZE_DESC
                        3 -> SortMode.SIZE_ASC
                        4 -> SortMode.RECOMMENDED
                        else -> SortMode.TITLE_ASC
                    }
                    applySortAndPagination(resetPage = true)
                    binding.recyclerGames.scrollToPosition(0)
                    binding.btnSortCatalog.text = currentSortMode.getLabel(requireContext())
                    dialog.dismiss()
                }
                .setNegativeButton(R.string.close, null)
                .show()
        }

        viewLifecycleOwner.lifecycleScope.launch {
            StormDownloadManager.state.collectLatest { progress ->
                updateDownloadUi(progress)
            }
        }

        val appCtx = context?.applicationContext
        val cached = if (appCtx != null) getCachedCatalog(appCtx) else emptyList()
        if (cached.isNotEmpty()) {
            allGames.clear()
            allGames.addAll(cached)
            filterGames(binding.editSearch.text?.toString().orEmpty())
            binding.textCatalogStatus.text = "Доступно игр Nintendo Switch: ${allGames.size}"
            binding.progressLoading.isVisible = false

            if (appCtx != null) {
                scheduleDownloadedStatusCheck(appCtx)
            }
        }

        fetchCatalog()
    }

    private var downloadCheckJob: kotlinx.coroutines.Job? = null

    private fun scheduleDownloadedStatusCheck(ctx: Context) {
        downloadCheckJob?.cancel()
        downloadCheckJob = viewLifecycleOwner.lifecycleScope.launch(Dispatchers.IO) {
            checkDownloadedStatus(allGames, ctx)
            withContext(Dispatchers.Main) {
                if (_binding != null) {
                    binding.recyclerGames.adapter?.notifyDataSetChanged()
                }
            }
        }
    }

    private fun fetchCatalog() {
        val appCtx = context?.applicationContext ?: return
        if (allGames.isEmpty()) {
            binding.progressLoading.isVisible = true
            binding.textCatalogStatus.text = "Синхронизация каталога облака..."
        }
        binding.textEmpty.isVisible = false
        binding.btnRefresh.isEnabled = false

        lifecycleScope.launch(Dispatchers.IO) {
            try {
                val freshGames = syncCatalogInBackground(appCtx)

                withContext(Dispatchers.Main) {
                    if (_binding == null) return@withContext
                    allGames.clear()
                    allGames.addAll(freshGames)
                    filterGames(binding.editSearch.text?.toString().orEmpty())
                    binding.progressLoading.isVisible = false
                    binding.btnRefresh.isEnabled = true
                    binding.textCatalogStatus.text = "Доступно игр Nintendo Switch: ${allGames.size}"
                    if (allGames.isEmpty()) {
                        binding.textEmpty.isVisible = true
                        binding.textEmpty.text = "Нет доступных игр в облаке"
                    }
                }

                scheduleDownloadedStatusCheck(appCtx)
            } catch (e: Exception) {
                Log.error("[StormGamesWorld] Catalog fetch error: ${e.message}")
                withContext(Dispatchers.Main) {
                    if (_binding == null) return@withContext
                    binding.progressLoading.isVisible = false
                    binding.btnRefresh.isEnabled = true
                    if (allGames.isEmpty()) {
                        binding.textEmpty.isVisible = true
                        binding.textEmpty.text = "Ошибка подключения: ${e.localizedMessage ?: "Сбой сети"}"
                        context?.let { c ->
                            Toast.makeText(c, "Не удалось загрузить каталог", Toast.LENGTH_SHORT).show()
                        }
                    }
                }
            }
        }
    }

    private fun checkDownloadedStatus(games: List<StormWorldGameItem>, ctx: Context) {
        val gameDirs = NativeConfig.getGameDirs()
        data class LocalFile(
            val name: String,
            val stem: String,
            val size: Long,
            val modCount: Int,
            val dlcCount: Int,
            val hasRus: Boolean
        )
        val allFiles = mutableListOf<LocalFile>()

        val modCountRegex = Regex("""(?:[+(\[{\s]|^)(\d+)m(?:[+)\]}\s]|$)""", RegexOption.IGNORE_CASE)
        val rusModRegex = Regex("""(?:\bmod\b)|(?:\bмод\b)|русификатор|озвучка""", RegexOption.IGNORE_CASE)
        val dlcCountRegex = Regex("""(?:[+(\[{\s]|^)(\d+)d(?:[+)\]}\s]|$)""", RegexOption.IGNORE_CASE)

        fun extractModCount(str: String): Int {
            return try {
                val m = modCountRegex.find(str)
                m?.groupValues?.get(1)?.toIntOrNull() ?: if (rusModRegex.containsMatchIn(str)) 1 else 0
            } catch (_: Exception) {
                0
            }
        }

        fun extractDlcCount(str: String): Int {
            return try {
                val m = dlcCountRegex.find(str)
                m?.groupValues?.get(1)?.toIntOrNull() ?: if (str.contains("dlc", ignoreCase = true)) 1 else 0
            } catch (_: Exception) {
                0
            }
        }

        for (dir in gameDirs) {
            try {
                val uri = Uri.parse(dir.uriString)
                if (uri.scheme == "content") {
                    val rootDoc = DocumentFile.fromTreeUri(ctx, uri)
                    rootDoc?.listFiles()?.forEach { f ->
                        if (f.isFile) {
                            val lower = f.name.orEmpty().lowercase(Locale.ROOT)
                            if (lower.endsWith(".nsp") || lower.endsWith(".xci") || lower.endsWith(".nsz")) {
                                val stem = lower.substringBeforeLast('.')
                                allFiles.add(
                                    LocalFile(
                                        lower,
                                        stem,
                                        f.length(),
                                        extractModCount(lower),
                                        extractDlcCount(lower),
                                        lower.contains("rus") || lower.contains("рус")
                                    )
                                )
                            }
                        }
                    }
                } else {
                    val path = uri.path ?: dir.uriString
                    File(path).listFiles()?.forEach { f ->
                        if (f.isFile) {
                            val lower = f.name.lowercase(Locale.ROOT)
                            if (lower.endsWith(".nsp") || lower.endsWith(".xci") || lower.endsWith(".nsz")) {
                                val stem = lower.substringBeforeLast('.')
                                allFiles.add(
                                    LocalFile(
                                        lower,
                                        stem,
                                        f.length(),
                                        extractModCount(lower),
                                        extractDlcCount(lower),
                                        lower.contains("rus") || lower.contains("рус")
                                    )
                                )
                            }
                        }
                    }
                }
            } catch (_: Exception) {}
        }

        val exactStems = allFiles.map { it.stem }.toHashSet()

        val groupCounts = mutableMapOf<String, Int>()
        games.forEach { g ->
            val tid = g.serialId.trim().uppercase(Locale.ROOT)
            val cleanBaseTitle = g.title.replace(Regex("[\\\\/:*?\"<>|]"), "_").trim().uppercase(Locale.ROOT)
            val key = if (tid.isNotEmpty() && tid != "—" && tid != "-") tid else cleanBaseTitle
            groupCounts[key] = (groupCounts[key] ?: 0) + 1
        }

        val verRegex = Regex("""(?:v|\-|\b)(\d+\.\d+(?:\.\d+)?|\d{5,8})(?:\b|\]|\))""")

        games.forEach { g ->
            val tid = g.serialId.trim().lowercase(Locale.ROOT)
            val cleanBaseTitle = g.title.replace(Regex("[\\\\/:*?\"<>|]"), "_").trim().uppercase(Locale.ROOT)
            val groupKey = if (tid.isNotEmpty() && tid != "—" && tid != "-") tid.uppercase(Locale.ROOT) else cleanBaseTitle
            val isSingle = (groupCounts[groupKey] ?: 0) <= 1
            val cleanTitle = g.title.replace(Regex("[\\\\/:*?\"<>|]"), "_").trim().lowercase(Locale.ROOT)
            val cleanFinal = (if (g.finalTitle.isNotEmpty()) g.finalTitle else g.title).replace(Regex("[\\\\/:*?\"<>|]"), "_").trim().lowercase(Locale.ROOT)
            val ver = g.version.trim().lowercase(Locale.ROOT)
            val intVer = g.internalVersion.trim()
            val fullTitle = "${g.finalTitle} ${g.title}".lowercase(Locale.ROOT)
            val gameIsRus = fullTitle.contains("rus") || fullTitle.contains("рус")
            val gameModCount = maxOf(g.modCount, extractModCount(fullTitle))
            val gameDlcCount = maxOf(g.dlcCount, extractDlcCount(fullTitle))
            val gameIsMod = gameModCount > 0
            val gameHasDlc = gameDlcCount > 0

            // Fast path: exact stem match
            if (cleanFinal.isNotEmpty() && exactStems.contains(cleanFinal)) {
                g.isDownloaded = true
                return@forEach
            }

            g.isDownloaded = allFiles.any { fileItem ->
                val lowerName = fileItem.name
                val fileStem = fileItem.stem

                if (cleanFinal.isNotEmpty() && fileStem == cleanFinal) {
                    return@any true
                }

                if (!isSingle) {
                    // Multiple versions in catalog: must verify exact version/mod/dlc/rus
                    if (gameIsMod != (fileItem.modCount > 0)) return@any false
                    if (gameIsMod && gameModCount != fileItem.modCount) return@any false
                    if (gameHasDlc != (fileItem.dlcCount > 0)) return@any false
                    if (gameHasDlc && gameDlcCount != fileItem.dlcCount) return@any false
                    if (gameIsRus != fileItem.hasRus) return@any false

                    if (cleanFinal.isNotEmpty() && (fileStem == cleanFinal || lowerName.contains(cleanFinal))) {
                        return@any true
                    }

                    if (tid.isNotEmpty() && tid != "—" && lowerName.contains(tid)) {
                        if (intVer.isNotEmpty() && intVer != "0") {
                            if (lowerName.contains(intVer) || lowerName.contains("v$intVer") || lowerName.contains("-$intVer-")) {
                                return@any true
                            }
                        }

                        if (ver.isNotEmpty() && ver != "1.0.0") {
                            if (lowerName.contains(ver) || lowerName.contains("v$ver")) {
                                return@any true
                            }
                        }

                        if ((ver.isEmpty() || ver == "1.0.0") && (intVer.isEmpty() || intVer == "0")) {
                            if (!verRegex.containsMatchIn(lowerName)) {
                                return@any true
                            }
                        }
                    }
                    false
                } else {
                    // Single version in catalog
                    if (cleanFinal.isNotEmpty() && lowerName.contains(cleanFinal)) return@any true
                    if (tid.isNotEmpty() && tid != "—" && lowerName.contains(tid)) return@any true
                    if (cleanTitle.isNotEmpty() && cleanTitle.length >= 4 && lowerName.contains(cleanTitle)) return@any true
                    false
                }
            }
        }
    }

    private fun filterGames(query: String) {
        val q = query.trim().lowercase(Locale.ROOT)
        filteredAndSortedGames.clear()
        if (q.isEmpty()) {
            filteredAndSortedGames.addAll(allGames)
        } else {
            for (g in allGames) {
                val matchTitle = g.title.lowercase(Locale.ROOT).contains(q)
                val matchFinal = g.finalTitle.lowercase(Locale.ROOT).contains(q)
                val matchTid = g.serialId.lowercase(Locale.ROOT).contains(q)
                if (matchTitle || matchFinal || matchTid) {
                    filteredAndSortedGames.add(g)
                }
            }
        }
        applySortAndPagination(resetPage = true)
    }

    private fun parseSizeToBytes(sizeStr: String): Long {
        val trimmed = sizeStr.trim().uppercase(Locale.ROOT)
        val num = trimmed.replace(Regex("[^0-9.]"), "").toDoubleOrNull() ?: 0.0
        return when {
            trimmed.endsWith("ГБ") || trimmed.endsWith("GB") -> (num * 1024.0 * 1024.0 * 1024.0).toLong()
            trimmed.endsWith("МБ") || trimmed.endsWith("MB") -> (num * 1024.0 * 1024.0).toLong()
            trimmed.endsWith("КБ") || trimmed.endsWith("KB") -> (num * 1024.0).toLong()
            else -> num.toLong()
        }
    }

    private fun applySortAndPagination(resetPage: Boolean = false) {
        when (currentSortMode) {
            SortMode.TITLE_ASC -> filteredAndSortedGames.sortBy { (if (it.title.isNotBlank()) it.title else it.finalTitle).lowercase(Locale.ROOT) }
            SortMode.TITLE_DESC -> filteredAndSortedGames.sortByDescending { (if (it.title.isNotBlank()) it.title else it.finalTitle).lowercase(Locale.ROOT) }
            SortMode.SIZE_DESC -> filteredAndSortedGames.sortByDescending { parseSizeToBytes(it.size) }
            SortMode.SIZE_ASC -> filteredAndSortedGames.sortBy { parseSizeToBytes(it.size) }
            SortMode.RECOMMENDED -> filteredAndSortedGames.sortByDescending { (it.dlcCount * 2 + it.modCount * 3) }
        }

        val totalItems = filteredAndSortedGames.size
        val totalPages = if (totalItems > 0) ((totalItems - 1) / pageSize) + 1 else 1
        if (resetPage) {
            currentPage = 1
        } else {
            currentPage = currentPage.coerceIn(1, totalPages)
        }

        val startIndex = (currentPage - 1) * pageSize
        val endIndex = Math.min(startIndex + pageSize, totalItems)

        pagedGames.clear()
        if (startIndex < totalItems) {
            pagedGames.addAll(filteredAndSortedGames.subList(startIndex, endIndex))
        }

        binding.recyclerGames.adapter?.notifyDataSetChanged()
        binding.textEmpty.isVisible = totalItems == 0

        updatePaginationUI(totalPages, totalItems)
    }

    private fun updatePaginationUI(totalPages: Int, totalItems: Int) {
        val binding = _binding ?: return
        binding.btnSortCatalog.text = currentSortMode.getLabel(binding.root.context)
        binding.textPaginationInfo.text = "Стр. $currentPage из $totalPages ($totalItems)"

        binding.btnPagePrev.isEnabled = currentPage > 1
        binding.btnPagePrev.alpha = if (currentPage > 1) 1.0f else 0.4f

        binding.btnPageNext.isEnabled = currentPage < totalPages
        binding.btnPageNext.alpha = if (currentPage < totalPages) 1.0f else 0.4f

        binding.layoutPageButtons.removeAllViews()
        if (totalPages <= 1) {
            binding.scrollPageNumbers.isVisible = false
            return
        }
        binding.scrollPageNumbers.isVisible = true

        val density = resources.displayMetrics.density
        val btnSize = (36 * density).toInt()
        val margin = (2 * density).toInt()

        val pagesToShow = linkedSetOf<Int>()
        pagesToShow.add(1)
        pagesToShow.add(totalPages)
        for (i in (currentPage - 2)..(currentPage + 2)) {
            if (i in 1..totalPages) {
                pagesToShow.add(i)
            }
        }
        val sortedPages = pagesToShow.sorted()

        val ctx = context ?: return
        val primaryColor = ThemeHelper.getColor(ctx, com.google.android.material.R.attr.colorPrimary)
        val onPrimaryColor = ThemeHelper.getColor(ctx, com.google.android.material.R.attr.colorOnPrimary)
        val surfaceVariantColor = ThemeHelper.getColor(ctx, com.google.android.material.R.attr.colorSurfaceVariant)
        val onSurfaceColor = ThemeHelper.getColor(ctx, com.google.android.material.R.attr.colorOnSurface)
        val onSurfaceVariantColor = ThemeHelper.getColor(ctx, com.google.android.material.R.attr.colorOnSurfaceVariant)
        val outlineColor = ThemeHelper.getColor(ctx, com.google.android.material.R.attr.colorOutline)

        var prev = 0
        for (p in sortedPages) {
            if (prev != 0 && p > prev + 1) {
                val ellipsis = android.widget.TextView(ctx).apply {
                    text = "…"
                    setTextColor(onSurfaceVariantColor)
                    textSize = 12f
                    setPadding(margin * 2, 0, margin * 2, 0)
                }
                binding.layoutPageButtons.addView(ellipsis)
            }
            prev = p

            val btn = com.google.android.material.button.MaterialButton(
                ctx,
                null,
                com.google.android.material.R.attr.materialButtonOutlinedStyle
            ).apply {
                layoutParams = android.widget.LinearLayout.LayoutParams(btnSize, btnSize).apply {
                    setMargins(margin, 0, margin, 0)
                }
                insetTop = 0
                insetBottom = 0
                setPadding(0, 0, 0, 0)
                text = p.toString()
                textSize = 12f
                cornerRadius = (6 * density).toInt()

                if (p == currentPage) {
                    backgroundTintList = android.content.res.ColorStateList.valueOf(primaryColor)
                    setTextColor(onPrimaryColor)
                    strokeColor = android.content.res.ColorStateList.valueOf(primaryColor)
                    strokeWidth = (1.5 * density).toInt()
                    typeface = android.graphics.Typeface.DEFAULT_BOLD
                } else {
                    backgroundTintList = android.content.res.ColorStateList.valueOf(surfaceVariantColor)
                    setTextColor(onSurfaceColor)
                    strokeColor = android.content.res.ColorStateList.valueOf(outlineColor)
                    strokeWidth = (1 * density).toInt()
                    setOnClickListener {
                        currentPage = p
                        applySortAndPagination(resetPage = false)
                        binding.recyclerGames.scrollToPosition(0)
                    }
                }
            }
            binding.layoutPageButtons.addView(btn)
        }
    }

    private fun onGameSelected(game: StormWorldGameItem) {
        selectedGame = game
        binding.cardDetailsPanel.isVisible = true

        val dispTitle = if (game.title.isNotBlank()) game.title else game.finalTitle
        binding.detailGameTitle.text = dispTitle
        binding.detailGameVersion.text = game.version
        if (game.internalVersion.isNotEmpty()) {
            binding.detailInternalVersion.isVisible = true
            binding.detailInternalVersion.text = game.internalVersion
        } else {
            binding.detailInternalVersion.isVisible = false
        }
        binding.detailGameSize.text = game.size
        val langsStr = if (game.textLangs.isNotEmpty()) game.textLangs.joinToString(", ") else "Multi"
        binding.detailGameLangs.text = langsStr
        binding.detailGameId.text = if (game.serialId.isNotEmpty()) "ID: ${game.serialId}" else ""

        if (game.dlcCount > 0) {
            binding.detailGameDlc.isVisible = true
            binding.detailGameDlc.text = "+${game.dlcCount} DLC"
            binding.detailGameDlc.setOnClickListener {
                showDlcDialog(game)
            }
        } else {
            binding.detailGameDlc.isVisible = false
            binding.detailGameDlc.setOnClickListener(null)
        }

        if (game.modCount > 0) {
            binding.detailGameMod.isVisible = true
            binding.detailGameMod.text = "+${game.modCount} MOD"
        } else {
            binding.detailGameMod.isVisible = false
        }

        binding.detailGameDescription.text = "Загрузка информации..."

        loadCoverForGame(game, binding.detailGameCover)

        val targetDir = getTargetDownloadDirectoryDescription()
        binding.detailSaveFolder.text = "📁 Каталог: $targetDir"

        updateDownloadUi(StormDownloadManager.state.value)

        fetchGameDetails(game)
    }

    private fun fetchGameDetails(game: StormWorldGameItem) {
        lifecycleScope.launch(Dispatchers.IO) {
            try {
                val req = Request.Builder()
                    .url("https://stormgamesworld.ru/api/games?id=${game.id}")
                    .header("User-Agent", "STORM_SWITCH/8.7.7 (Android)")
                    .build()
                val resp = httpClient.newCall(req).execute()
                val body = resp.body?.string().orEmpty()
                val trimmed = body.trim()
                val obj: JSONObject? = if (trimmed.startsWith("[")) {
                    val arr = JSONArray(trimmed)
                    if (arr.length() > 0) arr.optJSONObject(0) else null
                } else if (trimmed.startsWith("{")) {
                    JSONObject(trimmed)
                } else null

                if (obj != null) {
                    val desc = obj.optString("description", "")
                    val bytes = obj.optLong("fileSizeBytes", 0L)

                    game.description = desc
                    game.fileSizeBytes = bytes

                    val dlcsArr = obj.optJSONArray("dlcs")
                    if (dlcsArr != null) {
                        game.dlcs.clear()
                        for (d in 0 until dlcsArr.length()) {
                            val dObj = dlcsArr.optJSONObject(d) ?: continue
                            val dlcId = dObj.optString("id", "")
                            val rawName = dObj.optString("name", "").trim()
                            val dlcName = if (rawName.isNotEmpty()) rawName else "Дополнение ${d + 1}"
                            val dlcDesc = dObj.optString("description", "")
                            game.dlcs.add(StormWorldDlcItem(id = dlcId, name = dlcName, description = dlcDesc))
                        }
                        if (game.dlcs.isNotEmpty()) {
                            game.dlcCount = game.dlcs.size
                        }
                    }

                    val modsArr = obj.optJSONArray("mods")
                    if (modsArr != null && modsArr.length() > 0) {
                        game.modCount = maxOf(game.modCount, modsArr.length())
                    }
                }

                // Also check HEAD Content-Disposition to detect real extension (.nsz / .xci / .nsp)
                try {
                    val headReq = Request.Builder()
                        .url("https://stormgamesworld.ru/api/games/${game.id}/download")
                        .head()
                        .header("User-Agent", "STORM_SWITCH/8.7.7 (Android)")
                        .build()
                    val headResp = httpClient.newCall(headReq).execute()
                    val disp = headResp.header("Content-Disposition").orEmpty().lowercase(Locale.ROOT)
                    if (disp.contains(".nsz")) game.realExtension = ".nsz"
                    else if (disp.contains(".xci")) game.realExtension = ".xci"
                    else if (disp.contains(".nsp")) game.realExtension = ".nsp"
                } catch (_: Exception) {}

                withContext(Dispatchers.Main) {
                    if (_binding == null || selectedGame?.id != game.id) return@withContext
                    binding.detailGameDescription.text = if (game.description.isNotEmpty()) game.description else "Описание отсутствует"
                    if (game.dlcCount > 0) {
                        binding.detailGameDlc.isVisible = true
                        binding.detailGameDlc.text = "+${game.dlcCount} DLC"
                        binding.detailGameDlc.setOnClickListener {
                            showDlcDialog(game)
                        }
                    }
                    if (game.modCount > 0) {
                        binding.detailGameMod.isVisible = true
                        binding.detailGameMod.text = "+${game.modCount} MOD"
                    }
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

    private fun updateDownloadUi(progress: StormDownloadProgress?) {
        if (_binding == null) return
        val game = selectedGame

        if (progress == null || progress.status == StormDownloadStatus.IDLE || progress.status == StormDownloadStatus.CANCELLED) {
            binding.layoutDownloadProgress.isVisible = false
            if (game != null) {
                val ctx = requireContext()
                val primaryColor = ThemeHelper.getColor(ctx, com.google.android.material.R.attr.colorPrimary)
                if (game.isDownloaded) {
                    binding.btnStartDownload.text = "Скачано"
                    binding.btnStartDownload.setIconResource(R.drawable.ic_check)
                    binding.btnStartDownload.isEnabled = false
                    binding.btnStartDownload.backgroundTintList = android.content.res.ColorStateList.valueOf(0xFF10B981.toInt())
                    binding.btnStartDownload.strokeColor = android.content.res.ColorStateList.valueOf(0xFF10B981.toInt())
                    binding.btnStartDownload.setTextColor(0xFFFFFFFF.toInt())
                    binding.btnStartDownload.iconTint = android.content.res.ColorStateList.valueOf(0xFFFFFFFF.toInt())
                } else {
                    binding.btnStartDownload.text = "Скачать игру"
                    binding.btnStartDownload.setIconResource(R.drawable.ic_install)
                    binding.btnStartDownload.isEnabled = true
                    binding.btnStartDownload.backgroundTintList = android.content.res.ColorStateList.valueOf(0x00000000)
                    binding.btnStartDownload.strokeColor = android.content.res.ColorStateList.valueOf(primaryColor)
                    binding.btnStartDownload.setTextColor(primaryColor)
                    binding.btnStartDownload.iconTint = android.content.res.ColorStateList.valueOf(primaryColor)
                }
            }
            return
        }

        if (game?.id == progress.game.id) {
            when (progress.status) {
                StormDownloadStatus.CONNECTING -> {
                    binding.layoutDownloadProgress.isVisible = true
                    binding.progressDownload.isIndeterminate = true
                    binding.textDownloadStats.text = progress.statsText
                    binding.btnStartDownload.isEnabled = false
                    binding.btnStartDownload.text = "Подключение..."
                    binding.btnPauseDownload.text = "Пауза"
                }

                StormDownloadStatus.DOWNLOADING -> {
                    binding.layoutDownloadProgress.isVisible = true
                    binding.progressDownload.isIndeterminate = false
                    binding.progressDownload.progress = progress.progressPercent
                    binding.textDownloadStats.text = progress.statsText
                    binding.btnStartDownload.isEnabled = false
                    binding.btnStartDownload.text = "Идет загрузка..."
                    binding.btnPauseDownload.text = "Пауза"
                }

                StormDownloadStatus.PAUSED -> {
                    binding.layoutDownloadProgress.isVisible = true
                    binding.progressDownload.isIndeterminate = false
                    binding.progressDownload.progress = progress.progressPercent
                    binding.textDownloadStats.text = progress.statsText
                    binding.btnStartDownload.isEnabled = true
                    binding.btnStartDownload.text = "Возобновить"
                    binding.btnPauseDownload.text = "Продолжить"
                }

                StormDownloadStatus.COMPLETED -> {
                    binding.layoutDownloadProgress.isVisible = false
                    binding.btnStartDownload.text = "Скачано"
                    binding.btnStartDownload.setIconResource(R.drawable.ic_check)
                    binding.btnStartDownload.isEnabled = false
                    binding.btnStartDownload.backgroundTintList = android.content.res.ColorStateList.valueOf(0xFF10B981.toInt())
                    binding.btnStartDownload.strokeColor = android.content.res.ColorStateList.valueOf(0xFF10B981.toInt())
                    binding.btnStartDownload.setTextColor(0xFFFFFFFF.toInt())
                    binding.btnStartDownload.iconTint = android.content.res.ColorStateList.valueOf(0xFFFFFFFF.toInt())
                    game.isDownloaded = true
                    binding.recyclerGames.adapter?.notifyDataSetChanged()
                    gamesViewModel.reloadGames(directoriesChanged = true)
                }

                StormDownloadStatus.ERROR -> {
                    binding.layoutDownloadProgress.isVisible = true
                    binding.textDownloadStats.text = progress.statsText
                    binding.btnStartDownload.isEnabled = true
                    binding.btnStartDownload.text = "Повторить"
                    binding.btnPauseDownload.text = "Повторить"
                }

                else -> {}
            }
        } else {
            if (progress.status == StormDownloadStatus.DOWNLOADING || progress.status == StormDownloadStatus.CONNECTING) {
                binding.textCatalogStatus.text = "Фоновая загрузка: ${progress.game.finalTitle.ifEmpty { progress.game.title }} (${progress.progressPercent}%)"
            }
        }
    }

    override fun onDestroyView() {
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
            val item = pagedGames[position]
            val dispTitle = if (item.title.isNotBlank()) item.title else item.finalTitle
            val context = holder.itemView.context
            val primaryColor = ThemeHelper.getColor(context, com.google.android.material.R.attr.colorPrimary)
            val outlineColor = ThemeHelper.getColor(context, com.google.android.material.R.attr.colorOutline)

            holder.b.textGameTitle.text = dispTitle
            holder.b.textGameVersion.text = item.version
            holder.b.textGameVersion.setTextColor(primaryColor)
            if (item.internalVersion.isNotEmpty()) {
                holder.b.textInternalVersion.isVisible = true
                holder.b.textInternalVersion.text = item.internalVersion
            } else {
                holder.b.textInternalVersion.isVisible = false
            }
            holder.b.textGameSize.text = item.size
            holder.b.textGameLangs.text = if (item.textLangs.isNotEmpty()) item.textLangs.joinToString(", ") else "Multi"
            holder.b.textGameSerial.text = item.serialId

            if (item.dlcCount > 0) {
                holder.b.textGameDlc.isVisible = true
                holder.b.textGameDlc.text = "+${item.dlcCount} DLC"
                holder.b.textGameDlc.setOnClickListener {
                    showDlcDialog(item)
                }
            } else {
                holder.b.textGameDlc.isVisible = false
                holder.b.textGameDlc.setOnClickListener(null)
            }

            if (item.modCount > 0) {
                holder.b.badgeMod.isVisible = true
                holder.b.badgeMod.text = "+${item.modCount} MOD"
            } else {
                holder.b.badgeMod.isVisible = false
            }

            holder.b.badgeRecommended.isVisible = item.isRecommended

            loadCoverForGame(item, holder.b.imageGameCover)

            if (item.isDownloaded) {
                holder.b.btnGameAction.text = "Скачано"
                holder.b.btnGameAction.setIconResource(R.drawable.ic_check)
                holder.b.btnGameAction.isEnabled = false
                holder.b.btnGameAction.backgroundTintList = android.content.res.ColorStateList.valueOf(0xFF10B981.toInt())
                holder.b.btnGameAction.strokeColor = android.content.res.ColorStateList.valueOf(0xFF10B981.toInt())
                holder.b.btnGameAction.setTextColor(0xFFFFFFFF.toInt())
                holder.b.btnGameAction.iconTint = android.content.res.ColorStateList.valueOf(0xFFFFFFFF.toInt())
            } else {
                holder.b.btnGameAction.text = "Скачать"
                holder.b.btnGameAction.setIconResource(R.drawable.ic_install)
                holder.b.btnGameAction.isEnabled = !StormDownloadManager.isDownloading()
                holder.b.btnGameAction.backgroundTintList = android.content.res.ColorStateList.valueOf(0x00000000)
                holder.b.btnGameAction.strokeColor = android.content.res.ColorStateList.valueOf(primaryColor)
                holder.b.btnGameAction.setTextColor(primaryColor)
                holder.b.btnGameAction.iconTint = android.content.res.ColorStateList.valueOf(primaryColor)
            }

            holder.b.btnGameAction.setOnClickListener {
                onGameSelected(item)
                if (!item.isDownloaded && !StormDownloadManager.isDownloading()) {
                    StormDownloadManager.startDownload(holder.itemView.context, item)
                }
            }

            val isSelected = selectedGame?.id == item.id
            holder.b.root.strokeColor = if (isSelected) primaryColor else outlineColor
            holder.b.root.strokeWidth = if (isSelected) 2 else 1

            holder.b.root.setOnClickListener {
                onGameSelected(item)
                notifyDataSetChanged()
            }
        }

        override fun getItemCount(): Int = pagedGames.size
    }

    private fun showDlcDialog(game: StormWorldGameItem) {
        val dialog = Dialog(requireContext())
        val dialogBinding = DialogStormWorldDlcListBinding.inflate(layoutInflater)
        dialog.setContentView(dialogBinding.root)
        dialog.window?.setBackgroundDrawableResource(android.R.color.transparent)
        dialog.window?.setLayout(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT
        )

        val dispTitle = if (game.title.isNotBlank()) game.title else game.finalTitle
        dialogBinding.textDlcDialogGameTitle.text = dispTitle
        dialogBinding.textDlcDialogBadge.text = "${game.dlcCount} DLC"

        val dlcList = mutableListOf<StormWorldDlcItem>()
        dlcList.addAll(game.dlcs)

        val adapter = DlcAdapter(dlcList)
        dialogBinding.recyclerDlcItems.layoutManager = LinearLayoutManager(requireContext())
        dialogBinding.recyclerDlcItems.adapter = adapter

        if (dlcList.isEmpty() && game.dlcCount > 0) {
            dialogBinding.progressDlcLoading.isVisible = true
            dialogBinding.textDlcEmpty.isVisible = false

            lifecycleScope.launch(Dispatchers.IO) {
                try {
                    val req = Request.Builder()
                        .url("https://stormgamesworld.ru/api/games?id=${game.id}")
                        .header("User-Agent", "STORM_SWITCH/8.7.7 (Android)")
                        .build()
                    val resp = httpClient.newCall(req).execute()
                    val body = resp.body?.string().orEmpty()
                    val trimmed = body.trim()
                    val obj: JSONObject? = if (trimmed.startsWith("[")) {
                        val arr = JSONArray(trimmed)
                        if (arr.length() > 0) arr.optJSONObject(0) else null
                    } else if (trimmed.startsWith("{")) {
                        JSONObject(trimmed)
                    } else null
                    val dlcsArr = obj?.optJSONArray("dlcs")

                    val fetchedDlcs = mutableListOf<StormWorldDlcItem>()
                    if (dlcsArr != null && dlcsArr.length() > 0) {
                        for (d in 0 until dlcsArr.length()) {
                            val dObj = dlcsArr.optJSONObject(d) ?: continue
                            val dlcId = dObj.optString("id", "")
                            val rawName = dObj.optString("name", "").trim()
                            val dlcName = if (rawName.isNotEmpty()) rawName else "Официальное дополнение (DLC #${d + 1})"
                            val dlcDesc = dObj.optString("description", "")
                            fetchedDlcs.add(StormWorldDlcItem(id = dlcId, name = dlcName, description = dlcDesc))
                        }
                    }

                    withContext(Dispatchers.Main) {
                        if (!dialog.isShowing) return@withContext
                        game.dlcs.clear()
                        game.dlcs.addAll(fetchedDlcs)
                        if (game.dlcs.isNotEmpty()) {
                            game.dlcCount = game.dlcs.size
                            dialogBinding.textDlcDialogBadge.text = "${game.dlcCount} DLC"
                        }
                        dlcList.clear()
                        dlcList.addAll(game.dlcs)
                        adapter.notifyDataSetChanged()
                        dialogBinding.progressDlcLoading.isVisible = false
                        dialogBinding.textDlcEmpty.isVisible = dlcList.isEmpty()
                    }
                } catch (e: Exception) {
                    Log.error("[StormGamesWorld] DLC fetch error: ${e.message}")
                    withContext(Dispatchers.Main) {
                        if (!dialog.isShowing) return@withContext
                        dialogBinding.progressDlcLoading.isVisible = false
                        dialogBinding.textDlcEmpty.isVisible = dlcList.isEmpty()
                    }
                }
            }
        } else {
            dialogBinding.progressDlcLoading.isVisible = false
            dialogBinding.textDlcEmpty.isVisible = dlcList.isEmpty()
        }

        dialogBinding.btnCloseDlcDialog.setOnClickListener { dialog.dismiss() }
        dialogBinding.btnOkDlcDialog.setOnClickListener { dialog.dismiss() }

        dialog.show()
    }

    private inner class DlcAdapter(
        private val items: List<StormWorldDlcItem>
    ) : RecyclerView.Adapter<DlcAdapter.ViewHolder>() {

        inner class ViewHolder(val b: ListItemStormWorldDlcBinding) : RecyclerView.ViewHolder(b.root)

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
            val b = ListItemStormWorldDlcBinding.inflate(layoutInflater, parent, false)
            return ViewHolder(b)
        }

        override fun onBindViewHolder(holder: ViewHolder, position: Int) {
            val item = items[position]
            holder.b.textDlcIndex.text = "#${position + 1}"
            holder.b.textDlcName.text = item.name

            if (item.description.isNotBlank()) {
                holder.b.textDlcDescription.isVisible = true
                holder.b.textDlcDescription.text = item.description
            } else {
                holder.b.textDlcDescription.isVisible = false
            }

            if (item.id.isNotBlank()) {
                holder.b.textDlcId.isVisible = true
                holder.b.textDlcId.text = "ID: ${item.id}"
            } else {
                holder.b.textDlcId.isVisible = false
            }
        }

        override fun getItemCount(): Int = items.size
    }

    companion object {
        const val TAG = "StormGamesWorldDialogFragment"
        private const val CATALOG_CACHE_FILE = "storm_world_catalog_cache.json"

        val SWITCH_CDN_ICONS = mapOf(
        "01000B900D8B0000" to "https://img-eshop.cdn.nintendo.net/i/1972ebb4a507e7d83c7d4592ae4702ebdc5bf3738659644bc29c243b144782ee.jpg", // Cadence of Hyrule: Crypt of the NecroDancer featuring The Legend of Zelda
        "010015100B514000" to "https://img-eshop.cdn.nintendo.net/i/bf2fca7eed5ad7ec96d03025907ea52c3efe168e02c8be96e868d8430a247a57.jpg", // Super Mario Bros. Wonder
        "01001B300B9BE000" to "https://img-eshop.cdn.nintendo.net/i/bf924a38ce1da69413bdba496afad2ef562f73ea5273621c09cac878ab7ad0b5.jpg", // Diablo III: Eternal Collection
        "010020D01AD24000" to "https://img-eshop.cdn.nintendo.net/i/4519ab30d1b40a67a3ea3caa03cf792575954a15ecb1639f0e6b9eb9df169f37.jpg", // Animal Well
        "010022201229A000" to "https://img-eshop.cdn.nintendo.net/i/2035f3f0fc956dc61d691a2d3234974d6416f8175a9fc0989bfb8e6cf9477c92.jpg", // Super Robot Wars 30
        "010026800E304000" to "https://img-eshop.cdn.nintendo.net/i/7a48b73ce4dbcacbf25a4103ffad0f4414bcefc0a39d94ebcdc28f66e2e09f7d.jpg", // Super Robot Wars X
        "01002DA013484000" to "https://img-eshop.cdn.nintendo.net/i/7e575dbcd62ff138066bd6276afda8770f8c91c80ea99d13ab7480aed91b1ae1.jpg", // The Legend of Zelda: Skyward Sword HD
        "01002EF01A316000" to "https://img-eshop.cdn.nintendo.net/i/a00f56c54d8caf6c59f544d699b4ef42ae59e8960da3d1c2df49e3286a68e71c.jpg", // Brotato
        "01002FC00412C000" to "https://img-eshop.cdn.nintendo.net/i/765c5c93eaa0adc70d13b5ff3af3ed3a940cbf537a8de576e67d2c8622b19040.jpg", // Little Nightmares
        "0100307018934000" to "https://img-eshop.cdn.nintendo.net/i/e030ea7fe3a0ffa10b8fa371feba97ab92f94b2457c5baaf3e5e5ec77b4c3821.jpg", // Signalis
        "010040502453E000" to "https://img-eshop.cdn.nintendo.net/i/352a5f0b19d037318d3c3add59f8e22ad19dd15715d425e59f47ce7223c0de62.jpg", // Vampire Crawlers
        "010042D00D900000" to "https://img-eshop.cdn.nintendo.net/i/6849b7cca03fea9c8363524f31faaae42a196ddc8fe011a480e001858b36f53a.jpg", // LEGO Star Wars: The Skywalker Saga
        "010044700DEB0000" to "https://img-eshop.cdn.nintendo.net/i/66b639f88213ebf50af4a9eaca2da8c64a24a7314a0c80d587fec466df74d8b9.jpg", // Assassin’s Creed: The Rebel Collection
        "010057901E9E6000" to "https://img-eshop.cdn.nintendo.net/i/2e0cb59b4cd9d4443ae36bf1a94490d0ce08c38058c6e7a0d48befc1fecfeaaf.jpg", // Underling Uprising
        "010059D020C26000" to "https://img-eshop.cdn.nintendo.net/i/2d8f509b557453409619dbde88b0346d6a6f2a1bb6a3cb62fd9cea8c8925ba1f.jpg", // Marvel Cosmic Invasion
        "01005CF01E784000" to "https://img-eshop.cdn.nintendo.net/i/50d96f21074d590ba0572b7e8d10947cec190f600aa898ab3af540039f36fe6f.jpg", // Teenage Mutant Ninja Turtles: Splintered Fate
        "01005EC01E6A4000" to "https://img-eshop.cdn.nintendo.net/i/c94866c17a79c2c8ec8816df7db3433d88f19b880969af491ebe51dc14d4a590.jpg", // The Art of Dave the Diver: Digital Artbook
        "010063301BD50000" to "https://img-eshop.cdn.nintendo.net/i/d3fe8a7a991e1408e635b8f7c356c0e49213f8818cf456faf6a7d5256e9001e8.jpg", // Super Robot Wars Y
        "01006560184E6000" to "https://img-eshop.cdn.nintendo.net/i/f73a31a4c276dce115550ed9eda3ea813606b5e4ac53fae9f39d48f8f45099e1.jpg", // Mortal Kombat 1
        "010066101A55A000" to "https://img-eshop.cdn.nintendo.net/i/f8e6ecd237605ae5a839257a87190858bfebfa8f3b735eb73692a1e665699c80.jpg", // Little Nightmares III
        "0100670014482000" to "https://img-eshop.cdn.nintendo.net/i/aeab72bcb6fb6c79c32bddec987764f6e47c3e90cd32642a84de75f631b4eed0.jpg", // Assassin's Creed: The Ezio Collection
        "01006BB00C6F0000" to "https://img-eshop.cdn.nintendo.net/i/b0b0b2d150830b70b5bb259cdabefe21d2009b55cbd58854cf6a897587249054.jpg", // The Legend of Zelda: Link's Awakening
        "01006C900CC60000" to "https://img-eshop.cdn.nintendo.net/i/fc9a60cfb3a86cc0fbb45cc5377cd3d4ae1fa5dd05ebd8be991480992c809d46.jpg", // Super Robot Wars T
        "0100726014352000" to "https://img-eshop.cdn.nintendo.net/i/3555961fefd935624036664963fdcc3b4409d223802a52eb30040b3a84d256bc.jpg", // Diablo II: Resurrected
        "01007EF00011E000" to "https://img-eshop.cdn.nintendo.net/i/d3c210e61e8487200fc4c344987243a60257838187a69a6a81c42d7447d5d192.jpg", // The Legend of Zelda: Breath of the Wild
        "01007F600B134000" to "https://img-eshop.cdn.nintendo.net/i/b7c8b605f109a0f090fd4231a33c87eaf91ff1bb3a1b6fe94b9057a822162a6d.jpg", // Assassin's Creed III: Remastered
        "010089A0197E4000" to "https://img-eshop.cdn.nintendo.net/i/98bd188f34a48db53c83b6789993d167924a21f47d83fe3801e75d33a30b3d1c.jpg", // Vampire Survivors
        "01008BA02525A000" to "https://img-eshop.cdn.nintendo.net/i/6f498743317e0689bd8812450e41c80551f7eee1a68ef56aed4f6e93ac1aa56e.jpg", // Dispatch
        "01008CF01BAAC000" to "https://img-eshop.cdn.nintendo.net/i/2dc088f59a77a690046661216ae33948dc72dfe91675cf19d7d7ac757856bfc1.jpg", // The Legend of Zelda: Echoes of Wisdom
        "010093801237C000" to "https://img-eshop.cdn.nintendo.net/i/924b1b82bc75719dba325773795096359df9e6b4be0de77efa90b0e3039c6fff.jpg", // Metroid Dread
        "010094D023A28000" to "https://img-eshop.cdn.nintendo.net/i/e8fd1fe443c9e3b6a0768377247b978402ee04eb226d57d5ac8819d11a5bdcc5.jpg", // Drill Core
        "010097100EDD6000" to "https://img-eshop.cdn.nintendo.net/i/4dfb37171bdd954247af39a826e7bb700a52ae7c14b9fd5baa17c9a5be6aa965.jpg", // Little Nightmares II
        "010097F018538000" to "https://img-eshop.cdn.nintendo.net/i/c94866c17a79c2c8ec8816df7db3433d88f19b880969af491ebe51dc14d4a590.jpg", // Dave the Diver
        "0100AC300919A000" to "https://img-eshop.cdn.nintendo.net/i/05233ea213c6659cdadf4fcb592704c1c8f5ca76160b4d6673e67e5a67c0d11b.jpg", // Firewatch
        "0100B11027658000" to "https://img-eshop.cdn.nintendo.net/i/416666806a9808eb88e926d83098b30ccd956326b766f171f0b9cb99b58d0147.jpg", // Defender of the Crown: The Legend Returns
        "0100BAC01E57E000" to "https://img-eshop.cdn.nintendo.net/i/da8bf3f6e66a4914ceefa21f84462e4a1c0678e6a6f6a32831be0ad059a4b84d.jpg", // Ys X: Nordics
        "0100BDA01AABC000" to "https://img-eshop.cdn.nintendo.net/i/20b1474cd664e5e945f4e94bc7d36f9a7dee3e51929613f9d7875365a3a99bb0.jpg", // Rift of the NecroDancer
        "0100C6A0235D4000" to "https://img-eshop.cdn.nintendo.net/i/f4b0e53362b1df1880b54010fb4427780b5756f696164c38bab1d0f857ed8be0.jpg", // Devil Jam
        "0100CA400E300000" to "https://img-eshop.cdn.nintendo.net/i/77cf808ec79894dca98a64d9c0b281f3b513992f84897967f8e115891d4cd91f.jpg", // Super Robot Wars V
        "0100CEA007D08000" to "https://img-eshop.cdn.nintendo.net/i/eae2eb74652a1f82b3c8a0fd9dee68260ab4274509076df52330d784e80e8ac9.jpg", // Crypt of the NecroDancer
        "0100D59022590000" to "https://img-eshop.cdn.nintendo.net/i/4f61d9a29f29a2be5e070f8ff69c801224a7c5bd1656b6a4c17bc98c93f861bb.jpg", // Scott Pilgrim EX
        "0100E65002BB8000" to "https://img-eshop.cdn.nintendo.net/i/2c2d14a11ac7ee9439cfc88449360d238db52cedd921ac31309eea04053c08e7.jpg", // Stardew Valley
        "0100EC9010258000" to "https://img-eshop.cdn.nintendo.net/i/79be0825e04a14281c9b4ea2360ec609d63197d329bd19370f4ba0c7476659ad.jpg", // Streets of Rage 4
        "0100F2200C984000" to "https://img-eshop.cdn.nintendo.net/i/a24c3bf5c318afdf67b92dd4de87e2202738262ff4db62d3f2d10c4126a48f67.jpg", // Mortal Kombat 11
        "0100F2C0115B6000" to "https://img-eshop.cdn.nintendo.net/i/4b53da7ca4b118fe37c8b8040609b84dc63214d6131c51592486de9bf29ef29c.jpg" // The Legend of Zelda: Tears of the Kingdom
        )

        val sharedHttpClient: OkHttpClient by lazy {
            OkHttpClient.Builder()
                .dispatcher(Dispatcher().apply {
                    maxRequests = 32
                    maxRequestsPerHost = 16
                })
                .connectionPool(ConnectionPool(16, 5, TimeUnit.MINUTES))
                .connectTimeout(15, TimeUnit.SECONDS)
                .readTimeout(30, TimeUnit.SECONDS)
                .writeTimeout(30, TimeUnit.SECONDS)
                .retryOnConnectionFailure(true)
                .build()
        }

        fun newInstance(): StormGamesWorldDialogFragment {
            return StormGamesWorldDialogFragment()
        }

        fun getCachedCatalog(context: Context): List<StormWorldGameItem> {
            return try {
                val file = File(context.filesDir, CATALOG_CACHE_FILE)
                val jsonStr = if (file.exists() && file.length() > 0) {
                    file.readText()
                } else {
                    try {
                        context.assets.open(CATALOG_CACHE_FILE).bufferedReader().use { it.readText() }
                    } catch (_: Exception) {
                        ""
                    }
                }
                if (jsonStr.isBlank()) return emptyList()
                val jsonArr = JSONArray(jsonStr)
                val list = mutableListOf<StormWorldGameItem>()
                for (i in 0 until jsonArr.length()) {
                    val obj = jsonArr.optJSONObject(i) ?: continue
                    list.add(StormWorldGameItem.fromJson(obj))
                }
                list
            } catch (e: Exception) {
                Log.error("[StormGamesWorld] Failed to read cached catalog: ${e.message}")
                emptyList()
            }
        }

        fun saveCatalogCache(context: Context, games: List<StormWorldGameItem>) {
            try {
                val jsonArr = JSONArray()
                for (g in games) {
                    jsonArr.put(g.toJson())
                }
                val file = File(context.filesDir, CATALOG_CACHE_FILE)
                file.writeText(jsonArr.toString())
            } catch (e: Exception) {
                Log.error("[StormGamesWorld] Failed to save cached catalog: ${e.message}")
            }
        }

        suspend fun syncCatalogInBackground(context: Context): List<StormWorldGameItem> = withContext(Dispatchers.IO) {
            try {
                val req = Request.Builder()
                    .url("https://stormgamesworld.ru/api/games/index")
                    .header("User-Agent", "STORM_SWITCH/8.7.7 (Android)")
                    .build()

                val resp = sharedHttpClient.newCall(req).execute()
                val body = resp.body?.string().orEmpty()
                resp.close()

                val jsonArray = JSONArray(body)
                val candidateList = mutableListOf<StormWorldGameItem>()

                for (i in 0 until jsonArray.length()) {
                    val obj = jsonArray.optJSONObject(i) ?: continue
                    val platform = obj.optString("platformName")
                    val platformType = obj.optString("platformTypeName")
                    val fileExists = obj.optBoolean("fileExists", false)
                    val hasFile = obj.optBoolean("hasFile", false)
                    val sizeStr = obj.optString("size", "").trim()

                    // Exclude any game with missing flags or placeholder size
                    if (platform == "Nintendo Switch" && platformType == "CONSOLES" && fileExists && hasFile && sizeStr.isNotEmpty() && sizeStr != "—") {
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

                        val rawTitle = obj.optString("title")
                        val rawFinalTitle = obj.optString("finalTitle")
                        var rawVersion = obj.optString("version", "1.0.0").removePrefix("v").removePrefix("V").trim()
                        var internalVer = ""
                        var serialId = obj.optString("serialId").trim()

                        val versionRegex = Regex("""\(\s*([^-\)]+?)\s*-\s*(\d+)\s*-\s*([0-9A-Fa-f]{16})\s*\)""")
                        val match = versionRegex.find(rawFinalTitle) ?: versionRegex.find(rawTitle)
                        if (match != null) {
                            val vStr = match.groupValues[1].removePrefix("v").removePrefix("V").trim()
                            if (vStr.isNotEmpty()) rawVersion = vStr
                            internalVer = match.groupValues[2].trim()
                            if (serialId.isEmpty()) serialId = match.groupValues[3].trim()
                        }

                        val dlcMatch = Regex("""(?:\+|[\(\[])(\d+)D(?:\)|\]|\+)""", RegexOption.IGNORE_CASE).find(rawFinalTitle)
                        val dlcNum = dlcMatch?.groupValues?.get(1)?.toIntOrNull() ?: 0

                        val modMatch = Regex("""(?:\+|[\(\[])(\d+)M(?:\)|\]|\+)""", RegexOption.IGNORE_CASE).find(rawFinalTitle)
                        var modNum = modMatch?.groupValues?.get(1)?.toIntOrNull() ?: 0
                        if (modNum == 0) {
                            if (rawFinalTitle.contains("MOD", ignoreCase = true) || rawTitle.contains("MOD", ignoreCase = true) ||
                                langList.any { it.contains("MOD", ignoreCase = true) || it.contains("русификатор", ignoreCase = true) || it.contains("озвучка", ignoreCase = true) } ||
                                rawFinalTitle.contains("русификатор", ignoreCase = true) || rawTitle.contains("русификатор", ignoreCase = true) ||
                                rawFinalTitle.contains("озвучка", ignoreCase = true) || rawTitle.contains("озвучка", ignoreCase = true)) {
                                modNum = 1
                            }
                        }

                        candidateList.add(
                            StormWorldGameItem(
                                id = obj.optInt("id"),
                                title = rawTitle,
                                finalTitle = rawFinalTitle,
                                version = if (rawVersion.isNotEmpty()) rawVersion else "1.0.0",
                                internalVersion = internalVer,
                                serialId = serialId,
                                size = sizeStr,
                                cover = obj.optString("cover"),
                                fileExists = fileExists,
                                hasFile = hasFile,
                                regions = regList,
                                textLangs = langList,
                                dlcCount = dlcNum,
                                modCount = modNum
                            )
                        )
                    }
                }

                // Fast catalog compilation: candidateList already filters games with valid sizes and files
                val verifiedGames = candidateList

                fun compareVers(v1: String, v2: String): Int {
                    val p1 = v1.removePrefix("v").removePrefix("V").split(".").mapNotNull { it.toIntOrNull() }
                    val p2 = v2.removePrefix("v").removePrefix("V").split(".").mapNotNull { it.toIntOrNull() }
                    for (idx in 0 until maxOf(p1.size, p2.size)) {
                        val n1 = p1.getOrElse(idx) { 0 }
                        val n2 = p2.getOrElse(idx) { 0 }
                        if (n1 > n2) return 1
                        if (n1 < n2) return -1
                    }
                    return 0
                }

                fun calcPriority(g: StormWorldGameItem): Int {
                    val t = "${g.finalTitle} ${g.title}".uppercase(Locale.ROOT)
                    if (t.contains("MOD - RUS") || t.contains("MOD - M. RUS") ||
                        t.contains("MOD-RUS") || t.contains("MOD - M.RUS")) {
                        return 300
                    }
                    if (t.contains("[RUS]") || t.contains("(RUS)") ||
                        t.contains(" RUS ") || t.endsWith(" RUS")) {
                        return 200
                    }
                    for (l in g.textLangs) {
                        val lu = l.uppercase(Locale.ROOT)
                        if (lu == "RUS" || lu.contains("RUSSIAN") || lu.contains("РУССКИЙ")) {
                            return 200
                        }
                    }
                    return 100
                }

                val groups = mutableMapOf<String, MutableList<StormWorldGameItem>>()
                for (g in verifiedGames) {
                    val key = g.serialId.ifEmpty { g.finalTitle.ifEmpty { g.title } }.uppercase(Locale.ROOT)
                    groups.getOrPut(key) { mutableListOf() }.add(g)
                }

                for ((_, list) in groups) {
                    if (list.size <= 1) {
                        for (g in list) g.isRecommended = false
                        continue
                    }

                    var best: StormWorldGameItem? = null
                    for (g in list) {
                        g.isRecommended = false
                        if (best == null) {
                            best = g
                            continue
                        }
                        val prioCur = calcPriority(g)
                        val prioBest = calcPriority(best)
                        if (prioCur > prioBest) {
                            best = g
                        } else if (prioCur == prioBest) {
                            val cmp = compareVers(g.version, best.version)
                            if (cmp > 0 || (cmp == 0 && g.id > best.id)) {
                                best = g
                            }
                        }
                    }
                    best?.isRecommended = true
                }

                val sortedGames = verifiedGames.sortedWith { a, b ->
                    val keyA = a.serialId.ifEmpty { a.finalTitle.ifEmpty { a.title } }.uppercase(Locale.ROOT)
                    val keyB = b.serialId.ifEmpty { b.finalTitle.ifEmpty { b.title } }.uppercase(Locale.ROOT)
                    val keyCmp = keyA.compareTo(keyB)
                    if (keyCmp != 0) {
                        keyCmp
                    } else {
                        if (a.isRecommended != b.isRecommended) {
                            if (a.isRecommended) -1 else 1
                        } else {
                            val prioA = calcPriority(a)
                            val prioB = calcPriority(b)
                            if (prioA != prioB) {
                                prioB.compareTo(prioA)
                            } else {
                                val verCmp = compareVers(b.version, a.version)
                                if (verCmp != 0) verCmp else b.id.compareTo(a.id)
                            }
                        }
                    }
                }

                if (sortedGames.isNotEmpty()) {
                    saveCatalogCache(context, sortedGames)
                }
                sortedGames
            } catch (e: kotlinx.coroutines.CancellationException) {
                throw e
            } catch (e: Exception) {
                Log.error("[StormGamesWorld] Sync error: ${e.message}")
                getCachedCatalog(context)
            }
        }
    }
}

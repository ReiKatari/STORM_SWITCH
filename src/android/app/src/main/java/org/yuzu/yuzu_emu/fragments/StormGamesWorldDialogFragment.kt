// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.app.Dialog
import android.content.Context
import android.content.DialogInterface
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.os.Environment
import android.text.Editable
import android.text.TextWatcher
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.view.isVisible
import androidx.documentfile.provider.DocumentFile
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.activityViewModels
import androidx.lifecycle.lifecycleScope
import androidx.preference.PreferenceManager
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import coil.load
import org.yuzu.yuzu_emu.model.GameDir
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
    val extensionClean: String
        get() = realExtension.removePrefix(".").trim().uppercase(Locale.ROOT).ifEmpty { "NSP" }

    val rawGameId: Int
        get() = if (id >= 10000000) id % 10000000 else if (id < 0) -id else id

    val catalogKey: String
        get() = "${rawGameId}_$extensionClean"

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
        fun sanitizeEncoding(str: String): String {
            if (str.isEmpty()) return str
            if (!str.contains('Р') && !str.contains('С') && !str.contains('р') && !str.contains("вЂ")) return str
            return try {
                val cp1251 = java.nio.charset.Charset.forName("windows-1251")
                val bytes = str.toByteArray(cp1251)
                val decoded = String(bytes, java.nio.charset.StandardCharsets.UTF_8)
                if (decoded.contains('\uFFFD')) str else decoded
            } catch (_: Exception) {
                str
            }
        }

        fun fromJson(obj: JSONObject): StormWorldGameItem {
            val regList = mutableListOf<String>()
            val regArr = obj.optJSONArray("regions")
            if (regArr != null) {
                for (r in 0 until regArr.length()) regList.add(regArr.optString(r))
            }
            val langList = mutableListOf<String>()
            val langArr = obj.optJSONArray("textLangs")
            if (langArr != null) {
                for (l in 0 until langArr.length()) langList.add(sanitizeEncoding(langArr.optString(l)))
            }
            return StormWorldGameItem(
                id = obj.optInt("id"),
                title = sanitizeEncoding(obj.optString("title")),
                finalTitle = sanitizeEncoding(obj.optString("finalTitle")),
                version = obj.optString("version"),
                internalVersion = obj.optString("internalVersion"),
                serialId = obj.optString("serialId"),
                size = obj.optString("size"),
                cover = obj.optString("cover"),
                fileExists = obj.optBoolean("fileExists", true),
                hasFile = obj.optBoolean("hasFile", true),
                regions = regList,
                textLangs = langList,
                description = let {
                    val d = sanitizeEncoding(obj.optString("description"))
                    if (d == "null" || d.isBlank()) "" else d
                },
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

    private val selectDownloadDirLauncher =
        registerForActivityResult(ActivityResultContracts.OpenDocumentTree()) { uri ->
            if (uri == null) return@registerForActivityResult
            val ctx = context ?: return@registerForActivityResult
            try {
                val takeFlags = Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_GRANT_WRITE_URI_PERMISSION
                ctx.contentResolver.takePersistableUriPermission(uri, takeFlags)
            } catch (e: Exception) {
                Log.error("[StormGamesWorld] takePersistableUriPermission error: ${e.message}")
            }

            val prefs = PreferenceManager.getDefaultSharedPreferences(ctx)
            prefs.edit().putString(StormDownloadManager.PREF_CUSTOM_DOWNLOAD_DIR, uri.toString()).apply()

            val uriStr = uri.toString()
            val existing = NativeConfig.getGameDirs()
            if (existing.none { it.uriString == uriStr }) {
                NativeConfig.addGameDir(GameDir(uriStr, true))
            }

            val friendlyName = DocumentFile.fromTreeUri(ctx, uri)?.name ?: uri.lastPathSegment ?: "Каталог"
            binding.detailSaveFolder.text = "📁 Каталог: $friendlyName ✏️"
            Toast.makeText(ctx, "Каталог загрузки: $friendlyName", Toast.LENGTH_SHORT).show()
        }

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

    enum class LanguageFilter {
        ALL,
        RUS,
        ENG,
        MULTI,
        DLC_OR_MODS
    }

    enum class FormatFilter {
        ALL,
        NSP,
        NSZ,
        XCI,
        XCZ
    }

    private val allGames = mutableListOf<StormWorldGameItem>()
    private val filteredAndSortedGames = mutableListOf<StormWorldGameItem>()
    private val pagedGames = mutableListOf<StormWorldGameItem>()
    private var currentSortMode = SortMode.TITLE_ASC
    private var selectedLanguageFilter = LanguageFilter.ALL
    private var selectedFormatFilter = FormatFilter.ALL
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
        val localCoverRes = when (tid) {
            "058E630A38C70000" -> R.drawable.cover_diablo_hellfire
            "9B485EB8" -> R.drawable.cover_gta_v
            "010034B00E14C000" -> R.drawable.cover_tokyo_2020
            else -> null
        }
        if (localCoverRes != null) {
            imageView.setImageResource(localCoverRes)
            return
        }

        val url = if (item.cover.isNotBlank() && item.cover.startsWith("http")) {
            item.cover
        } else {
            SWITCH_CDN_ICONS[tid] ?: dynamicCoverCache[tid]
        }
        if (!url.isNullOrBlank()) {
            imageView.load(url) {
                crossfade(true)
                placeholder(R.drawable.default_icon)
                error(R.drawable.default_icon)
            }
        } else {
            imageView.setImageResource(R.drawable.default_icon)
            val isHomebrew = item.finalTitle.contains("Homebrew", ignoreCase = true) ||
                             item.title.contains("Homebrew", ignoreCase = true)
            if (isHomebrew) {
                return
            }
            val gameId = item.id
            imageView.tag = gameId
            viewLifecycleOwner.lifecycleScope.launch(Dispatchers.IO) {
                try {
                    var foundImgUrl: String? = null
                    // 1. Strict priority: search by exact Title ID on Nintendo eShop
                    if (tid.length == 16) {
                        val searchByTidUrl = "https://search.nintendo-europe.com/en/select?q=*&fq=application_id_s:${tid.lowercase(Locale.ROOT)}&wt=json"
                        val req = Request.Builder().url(searchByTidUrl).header("User-Agent", "Mozilla/5.0").build()
                        val resp = sharedHttpClient.newCall(req).execute()
                        val body = resp.body?.string().orEmpty()
                        resp.close()
                        if (body.isNotEmpty()) {
                            val json = JSONObject(org.json.JSONTokener(body))
                            val docs = json.optJSONObject("response")?.optJSONArray("docs")
                            if (docs != null && docs.length() > 0) {
                                val doc = docs.getJSONObject(0)
                                foundImgUrl = doc.optString("image_url_sq_s").ifEmpty {
                                    doc.optString("image_url").ifEmpty { doc.optString("image_url_h2x1_s") }
                                }
                            }
                        }
                    }

                    // 2. Fallback: search by clean title with strict title verification
                    if (foundImgUrl.isNullOrEmpty()) {
                        val cleanTitle = (item.title.ifEmpty { item.finalTitle })
                            .replace(Regex("""\([^\)]*\)"""), "")
                            .replace(Regex("""\[[^\]]*\]"""), "")
                            .replace(Regex("""\{[^\}]*\}"""), "")
                            .replace("MOD", "", ignoreCase = true)
                            .trim()
                        if (cleanTitle.length >= 3) {
                            val searchByTitleUrl = "https://search.nintendo-europe.com/en/select?q=${Uri.encode(cleanTitle)}&fq=type:GAME&rows=3&wt=json"
                            val req = Request.Builder().url(searchByTitleUrl).header("User-Agent", "Mozilla/5.0").build()
                            val resp = sharedHttpClient.newCall(req).execute()
                            val body = resp.body?.string().orEmpty()
                            resp.close()
                            if (body.isNotEmpty()) {
                                val json = JSONObject(org.json.JSONTokener(body))
                                val docs = json.optJSONObject("response")?.optJSONArray("docs")
                                if (docs != null) {
                                    for (dIdx in 0 until docs.length()) {
                                        val doc = docs.getJSONObject(dIdx)
                                        val docTitle = doc.optString("title", "")
                                        val match = docTitle.contains(cleanTitle, ignoreCase = true) ||
                                                    cleanTitle.contains(docTitle, ignoreCase = true)
                                        if (match) {
                                            foundImgUrl = doc.optString("image_url_sq_s").ifEmpty {
                                                doc.optString("image_url").ifEmpty { doc.optString("image_url_h2x1_s") }
                                            }
                                            if (!foundImgUrl.isNullOrEmpty()) break
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (!foundImgUrl.isNullOrEmpty() && foundImgUrl.startsWith("http")) {
                        if (tid.isNotEmpty()) {
                            dynamicCoverCache[tid] = foundImgUrl
                        }
                        withContext(Dispatchers.Main) {
                            if (imageView.tag == gameId) {
                                imageView.load(foundImgUrl) {
                                    crossfade(true)
                                    placeholder(R.drawable.default_icon)
                                    error(R.drawable.default_icon)
                                }
                            }
                        }
                    }
                } catch (_: Exception) {}
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setStyle(STYLE_NORMAL, ThemeHelper.getSelectedStaticThemeColor())
    }

    override fun onStart() {
        super.onStart()
        dialog?.window?.let { window ->
            window.setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT)
            window.setBackgroundDrawableResource(android.R.color.transparent)
            ThemeHelper.applySystemBarsTheme(window, requireContext())
        }
    }

    override fun onConfigurationChanged(newConfig: android.content.res.Configuration) {
        super.onConfigurationChanged(newConfig)
        dialog?.window?.let { window ->
            window.setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT)
            window.setBackgroundDrawableResource(android.R.color.transparent)
            ThemeHelper.applySystemBarsTheme(window, requireContext())
        }
    }

    override fun onDismiss(dialog: DialogInterface) {
        super.onDismiss(dialog)
        activity?.let { ThemeHelper.applySystemBarsTheme(it.window, it) }
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
        binding.root.requestApplyInsets()

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

        val chipMappings = listOfNotNull(
            binding.chipLangAll?.let { it to LanguageFilter.ALL },
            binding.chipLangRus?.let { it to LanguageFilter.RUS },
            binding.chipLangEng?.let { it to LanguageFilter.ENG },
            binding.chipLangMulti?.let { it to LanguageFilter.MULTI },
            binding.chipWithDlc?.let { it to LanguageFilter.DLC_OR_MODS }
        )
        for ((chip, filter) in chipMappings) {
            chip.setOnClickListener {
                if (selectedLanguageFilter != filter) {
                    selectedLanguageFilter = filter
                    updateLanguageChipsUi()
                    filterGames(binding.editSearch.text?.toString().orEmpty())
                    binding.recyclerGames.scrollToPosition(0)
                }
            }
        }
        updateLanguageChipsUi()

        val formatChipMappings = listOfNotNull(
            binding.chipExtAll?.let { it to FormatFilter.ALL },
            binding.chipExtNsp?.let { it to FormatFilter.NSP },
            binding.chipExtNsz?.let { it to FormatFilter.NSZ },
            binding.chipExtXci?.let { it to FormatFilter.XCI },
            binding.chipExtXcz?.let { it to FormatFilter.XCZ }
        )
        for ((chip, filter) in formatChipMappings) {
            chip.setOnClickListener {
                if (selectedFormatFilter != filter) {
                    selectedFormatFilter = filter
                } else if (filter != FormatFilter.ALL) {
                    selectedFormatFilter = FormatFilter.ALL
                }
                updateFormatChipsUi()
                filterGames(binding.editSearch.text?.toString().orEmpty())
                binding.recyclerGames.scrollToPosition(0)
            }
        }
        updateFormatChipsUi()

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
        viewLifecycleOwner.lifecycleScope.launch(Dispatchers.IO) {
            val cached = if (appCtx != null) getCachedCatalog(appCtx) else emptyList()
            withContext(Dispatchers.Main) {
                if (_binding != null && cached.isNotEmpty()) {
                    synchronized(allGames) {
                        allGames.clear()
                        allGames.addAll(cached)
                    }
                    filterGames(binding.editSearch.text?.toString().orEmpty())
                    binding.textCatalogStatus.text = "Доступно игр Nintendo Switch: ${allGames.size}"
                    binding.progressLoading.isVisible = false

                    if (appCtx != null) {
                        scheduleDownloadedStatusCheck(appCtx)
                    }
                }
            }
        }

        fetchCatalog()
    }

    private var downloadCheckJob: kotlinx.coroutines.Job? = null

    private fun scheduleDownloadedStatusCheck(ctx: Context) {
        downloadCheckJob?.cancel()
        val snapshot = synchronized(allGames) { allGames.toList() }
        downloadCheckJob = viewLifecycleOwner.lifecycleScope.launch(Dispatchers.IO) {
            checkDownloadedStatus(snapshot, ctx)
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
                    synchronized(allGames) {
                        allGames.clear()
                        allGames.addAll(freshGames)
                    }
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
            val hasRus: Boolean,
            val extension: String = ""
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
                            if (lower.endsWith(".nsp") || lower.endsWith(".xci") || lower.endsWith(".nsz") || lower.endsWith(".xcz")) {
                                val stem = lower.substringBeforeLast('.')
                                val ext = lower.substringAfterLast('.', "")
                                allFiles.add(
                                    LocalFile(
                                        lower,
                                        stem,
                                        f.length(),
                                        extractModCount(lower),
                                        extractDlcCount(lower),
                                        lower.contains("rus") || lower.contains("рус"),
                                        ext
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
                            if (lower.endsWith(".nsp") || lower.endsWith(".xci") || lower.endsWith(".nsz") || lower.endsWith(".xcz")) {
                                val stem = lower.substringBeforeLast('.')
                                val ext = lower.substringAfterLast('.', "")
                                allFiles.add(
                                    LocalFile(
                                        lower,
                                        stem,
                                        f.length(),
                                        extractModCount(lower),
                                        extractDlcCount(lower),
                                        lower.contains("rus") || lower.contains("рус"),
                                        ext
                                    )
                                )
                            }
                        }
                    }
                }
            } catch (_: Exception) {}
        }

        val exactStems = allFiles.map { it.stem }.toHashSet()
        val exactFileNames = allFiles.map { it.name }.toHashSet()

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

            // Fast path: exact filename with extension match
            val expectedFullName = "$cleanFinal.${g.extensionClean.lowercase(Locale.ROOT)}"
            if (cleanFinal.isNotEmpty() && exactFileNames.contains(expectedFullName)) {
                g.isDownloaded = true
                return@forEach
            }
            if (isSingle && cleanFinal.isNotEmpty() && exactStems.contains(cleanFinal)) {
                g.isDownloaded = true
                return@forEach
            }

            g.isDownloaded = allFiles.any { fileItem ->
                val lowerName = fileItem.name
                val fileStem = fileItem.stem

                if (isSingle && cleanFinal.isNotEmpty() && fileStem == cleanFinal) {
                    return@any true
                }

                if (!isSingle) {
                    // Multiple versions/formats in catalog: must verify exact format/version/mod/dlc/rus
                    if (fileItem.extension.isNotEmpty() && !fileItem.extension.equals(g.extensionClean, ignoreCase = true)) return@any false
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

    private fun matchesLanguage(g: StormWorldGameItem, filter: LanguageFilter): Boolean {
        return when (filter) {
            LanguageFilter.ALL -> true
            LanguageFilter.RUS -> {
                val full = "${g.title} ${g.finalTitle} ${g.description}".lowercase(Locale.ROOT)
                val hasTextLang = g.textLangs.any {
                    val l = it.lowercase(Locale.ROOT).trim()
                    l == "ru" || l == "rus" || l == "russian" || l.startsWith("ru-")
                }
                hasTextLang || full.contains("rus") || full.contains("рус") || full.contains("русификатор") || full.contains("озвучка")
            }
            LanguageFilter.ENG -> {
                val full = "${g.title} ${g.finalTitle}".lowercase(Locale.ROOT)
                val hasTextLang = g.textLangs.any {
                    val l = it.lowercase(Locale.ROOT).trim()
                    l == "en" || l == "eng" || l == "english" || l.startsWith("en-")
                }
                hasTextLang || full.contains("eng") || full.contains("english") || g.regions.any { r ->
                    val reg = r.uppercase(Locale.ROOT)
                    reg == "US" || reg == "USA" || reg == "EUR"
                }
            }
            LanguageFilter.MULTI -> {
                true
            }
            LanguageFilter.DLC_OR_MODS -> {
                val full = "${g.title} ${g.finalTitle}".lowercase(Locale.ROOT)
                g.dlcCount > 0 || g.modCount > 0 || g.dlcs.isNotEmpty() ||
                    full.contains("dlc") || full.contains("mod") || full.contains("мод") ||
                    full.contains("дополнение") || full.contains("update") || full.contains("обновление")
            }
        }
    }

    private fun updateLanguageChipsUi() {
        val binding = _binding ?: return
        val ctx = context ?: return

        val primaryColor = ThemeHelper.getColor(ctx, com.google.android.material.R.attr.colorPrimary)
        val onPrimaryColor = ThemeHelper.getColor(ctx, com.google.android.material.R.attr.colorOnPrimary)
        val surfaceVariantColor = ThemeHelper.getColor(ctx, com.google.android.material.R.attr.colorSurfaceVariant)
        val onSurfaceColor = ThemeHelper.getColor(ctx, com.google.android.material.R.attr.colorOnSurface)
        val outlineColor = ThemeHelper.getColor(ctx, com.google.android.material.R.attr.colorOutline)

        val chips = listOfNotNull(
            binding.chipLangAll?.let { it to LanguageFilter.ALL },
            binding.chipLangRus?.let { it to LanguageFilter.RUS },
            binding.chipLangEng?.let { it to LanguageFilter.ENG },
            binding.chipLangMulti?.let { it to LanguageFilter.MULTI },
            binding.chipWithDlc?.let { it to LanguageFilter.DLC_OR_MODS }
        )

        val density = resources.displayMetrics.density
        for ((chip, filter) in chips) {
            val isSelected = (filter == selectedLanguageFilter)
            if (isSelected) {
                chip.backgroundTintList = android.content.res.ColorStateList.valueOf(primaryColor)
                chip.setTextColor(onPrimaryColor)
                chip.strokeColor = android.content.res.ColorStateList.valueOf(primaryColor)
                chip.strokeWidth = (1.5f * density).toInt()
            } else {
                chip.backgroundTintList = android.content.res.ColorStateList.valueOf(surfaceVariantColor)
                chip.setTextColor(onSurfaceColor)
                chip.strokeColor = android.content.res.ColorStateList.valueOf(outlineColor)
                chip.strokeWidth = (1f * density).toInt()
            }
        }
    }

    private fun updateFormatChipsUi() {
        val binding = _binding ?: return
        val ctx = context ?: return

        val primaryColor = ThemeHelper.getColor(ctx, com.google.android.material.R.attr.colorPrimary)
        val onPrimaryColor = ThemeHelper.getColor(ctx, com.google.android.material.R.attr.colorOnPrimary)
        val surfaceVariantColor = ThemeHelper.getColor(ctx, com.google.android.material.R.attr.colorSurfaceVariant)
        val onSurfaceColor = ThemeHelper.getColor(ctx, com.google.android.material.R.attr.colorOnSurface)
        val outlineColor = ThemeHelper.getColor(ctx, com.google.android.material.R.attr.colorOutline)

        val chips = listOfNotNull(
            binding.chipExtAll?.let { it to FormatFilter.ALL },
            binding.chipExtNsp?.let { it to FormatFilter.NSP },
            binding.chipExtNsz?.let { it to FormatFilter.NSZ },
            binding.chipExtXci?.let { it to FormatFilter.XCI },
            binding.chipExtXcz?.let { it to FormatFilter.XCZ }
        )

        val density = resources.displayMetrics.density
        for ((chip, filter) in chips) {
            val isSelected = (filter == selectedFormatFilter)
            if (isSelected) {
                chip.backgroundTintList = android.content.res.ColorStateList.valueOf(primaryColor)
                chip.setTextColor(onPrimaryColor)
                chip.strokeColor = android.content.res.ColorStateList.valueOf(primaryColor)
                chip.strokeWidth = (1.5f * density).toInt()
            } else {
                chip.backgroundTintList = android.content.res.ColorStateList.valueOf(surfaceVariantColor)
                chip.setTextColor(onSurfaceColor)
                chip.strokeColor = android.content.res.ColorStateList.valueOf(outlineColor)
                chip.strokeWidth = (1f * density).toInt()
            }
        }
    }

    private fun matchesFormat(game: StormWorldGameItem, filter: FormatFilter): Boolean {
        return when (filter) {
            FormatFilter.ALL -> true
            FormatFilter.NSP -> game.extensionClean.equals("NSP", ignoreCase = true)
            FormatFilter.NSZ -> game.extensionClean.equals("NSZ", ignoreCase = true)
            FormatFilter.XCI -> game.extensionClean.equals("XCI", ignoreCase = true)
            FormatFilter.XCZ -> game.extensionClean.equals("XCZ", ignoreCase = true)
        }
    }

    private fun filterGames(query: String) {
        val q = query.trim().lowercase(Locale.ROOT)
        filteredAndSortedGames.clear()
        for (g in allGames) {
            if (!matchesLanguage(g, selectedLanguageFilter)) continue
            if (!matchesFormat(g, selectedFormatFilter)) continue
            if (q.isNotEmpty()) {
                val matchTitle = g.title.lowercase(Locale.ROOT).contains(q)
                val matchFinal = g.finalTitle.lowercase(Locale.ROOT).contains(q)
                val matchTid = g.serialId.lowercase(Locale.ROOT).contains(q)
                val matchExt = g.extensionClean.lowercase(Locale.ROOT) == q || g.realExtension.lowercase(Locale.ROOT) == q
                if (!matchTitle && !matchFinal && !matchTid && !matchExt) continue
            }
            filteredAndSortedGames.add(g)
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
        binding.textPaginationInfo.text = "Страница $currentPage из $totalPages ($totalItems)"

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
        val btnSize = (30 * density).toInt()
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
                    textSize = 11f
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
                textSize = 11f
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
        binding.detailBadgeExtension.text = game.extensionClean
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

        binding.detailGameDescription.text = if (game.description.isNotBlank() && game.description != "null") {
            game.description
        } else {
            "Загрузка информации..."
        }

        loadCoverForGame(game, binding.detailGameCover)

        val targetDir = getTargetDownloadDirectoryDescription()
        binding.detailSaveFolder.text = "📁 Каталог: $targetDir ✏️"
        binding.detailSaveFolder.setOnClickListener {
            showDownloadDirectoryPicker()
        }

        updateDownloadUi(StormDownloadManager.state.value)

        fetchGameDetails(game)
    }

    private fun fetchGameDetails(game: StormWorldGameItem) {
        lifecycleScope.launch(Dispatchers.IO) {
            try {
                val req = Request.Builder()
                    .url("https://stormgamesworld.ru/api/games?id=${game.rawGameId}")
                    .header("User-Agent", "STORM_SWITCH/9.1.0 (Android)")
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
                    val rawDesc = obj.optString("description", "")
                    val safeDesc = if (rawDesc == "null" || rawDesc.isBlank()) "" else rawDesc
                    val bytes = obj.optLong("fileSizeBytes", 0L)

                    game.description = StormWorldGameItem.sanitizeEncoding(safeDesc)
                    if (game.fileSizeBytes == 0L) {
                        game.fileSizeBytes = bytes
                    }

                    val dlcsArr = obj.optJSONArray("dlcs")
                    if (dlcsArr != null) {
                        game.dlcs.clear()
                        for (d in 0 until dlcsArr.length()) {
                            val dObj = dlcsArr.optJSONObject(d) ?: continue
                            val dlcId = dObj.optString("id", "")
                            val rawName = dObj.optString("name", "").trim()
                            val dlcName = StormWorldGameItem.sanitizeEncoding(if (rawName.isNotEmpty()) rawName else "Дополнение ${d + 1}")
                            val dlcDesc = StormWorldGameItem.sanitizeEncoding(dObj.optString("description", ""))
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

                // Check HEAD Content-Disposition only if current extension is completely empty
                var extChanged = false
                if (game.realExtension.isEmpty()) {
                    try {
                        val headReq = Request.Builder()
                            .url("https://stormgamesworld.ru/api/games/${game.rawGameId}/download")
                            .head()
                            .header("User-Agent", "STORM_SWITCH/9.1.0 (Android)")
                            .build()
                        val headResp = httpClient.newCall(headReq).execute()
                        val disp = headResp.header("Content-Disposition").orEmpty().lowercase(Locale.ROOT)
                        if (disp.contains(".nsz")) game.realExtension = ".nsz"
                        else if (disp.contains(".xcz")) game.realExtension = ".xcz"
                        else if (disp.contains(".xci")) game.realExtension = ".xci"
                        else if (disp.contains(".nsp")) game.realExtension = ".nsp"
                        if (game.realExtension.isNotEmpty()) {
                            extChanged = true
                        }
                    } catch (e: Exception) {
                        Log.error("[StormGamesWorld] HEAD request error: ${e.message}")
                    }
                }

                withContext(Dispatchers.Main) {
                    if (_binding == null || selectedGame?.id != game.id) return@withContext
                    binding.detailBadgeExtension.text = game.extensionClean
                    if (extChanged) {
                        binding.recyclerGames.adapter?.notifyDataSetChanged()
                        context?.applicationContext?.let { appCtx ->
                            saveCatalogCache(appCtx, allGames)
                        }
                    }
                    binding.detailGameDescription.text = if (game.description.isNotBlank() && game.description != "null") {
                        game.description
                    } else {
                        "Описание отсутствует"
                    }
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

    private fun showDownloadDirectoryPicker() {
        val ctx = context ?: return
        val gameDirs = NativeConfig.getGameDirs().filter { it.uriString.isNotBlank() }
        val options = mutableListOf<String>()
        val actions = mutableListOf<() -> Unit>()

        options.add("📂 Выбрать новую папку на устройстве...")
        actions.add { selectDownloadDirLauncher.launch(null) }

        for (dir in gameDirs) {
            val uri = Uri.parse(dir.uriString)
            val name = DocumentFile.fromTreeUri(ctx, uri)?.name ?: uri.lastPathSegment ?: dir.uriString
            options.add("📁 Папка игр: $name")
            actions.add {
                val prefs = PreferenceManager.getDefaultSharedPreferences(ctx)
                prefs.edit().putString(StormDownloadManager.PREF_CUSTOM_DOWNLOAD_DIR, dir.uriString).apply()
                binding.detailSaveFolder.text = "📁 Каталог: $name ✏️"
                Toast.makeText(ctx, "Каталог загрузки: $name", Toast.LENGTH_SHORT).show()
            }
        }

        options.add("🔄 По умолчанию (Download/STORM_SWITCH_GAMES)")
        actions.add {
            val prefs = PreferenceManager.getDefaultSharedPreferences(ctx)
            prefs.edit().remove(StormDownloadManager.PREF_CUSTOM_DOWNLOAD_DIR).apply()
            val defName = getTargetDownloadDirectoryDescription()
            binding.detailSaveFolder.text = "📁 Каталог: $defName ✏️"
            Toast.makeText(ctx, "Каталог сброшен по умолчанию", Toast.LENGTH_SHORT).show()
        }

        com.google.android.material.dialog.MaterialAlertDialogBuilder(ctx)
            .setTitle("Каталог для загрузки игр")
            .setItems(options.toTypedArray()) { _, which ->
                if (which in actions.indices) {
                    actions[which].invoke()
                }
            }
            .setNegativeButton("Отмена", null)
            .show()
    }

    private fun getTargetDownloadDirectoryDescription(): String {
        val ctx = context ?: return "STORM_SWITCH_GAMES"
        val prefs = PreferenceManager.getDefaultSharedPreferences(ctx)
        val customUriStr = prefs.getString(StormDownloadManager.PREF_CUSTOM_DOWNLOAD_DIR, null)
        if (!customUriStr.isNullOrBlank()) {
            val uri = Uri.parse(customUriStr)
            val name = DocumentFile.fromTreeUri(ctx, uri)?.name ?: uri.lastPathSegment
            if (!name.isNullOrBlank()) return name
        }

        val gameDirs = NativeConfig.getGameDirs().filter { it.uriString.isNotBlank() }
        val preferredDir = gameDirs.firstOrNull { dir ->
            val s = dir.uriString.lowercase(Locale.ROOT)
            !s.contains("mod") && !s.contains("cheat") && !s.contains("60fps") && !s.contains("patch")
        } ?: gameDirs.firstOrNull()

        if (preferredDir != null) {
            val uri = Uri.parse(preferredDir.uriString)
            val name = DocumentFile.fromTreeUri(ctx, uri)?.name ?: uri.lastPathSegment
            if (!name.isNullOrBlank()) return name
        }

        return "Download/STORM_SWITCH_GAMES"
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
            holder.b.badgeGameExtension.text = item.extensionClean
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

            val isSelected = selectedGame?.id == item.id && selectedGame?.realExtension == item.realExtension
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
                        .url("https://stormgamesworld.ru/api/games?id=${game.rawGameId}")
                        .header("User-Agent", "STORM_SWITCH/9.1.0 (Android)")
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
                            val dlcName = StormWorldGameItem.sanitizeEncoding(if (rawName.isNotEmpty()) rawName else "Официальное дополнение (DLC #${d + 1})")
                            val dlcDesc = StormWorldGameItem.sanitizeEncoding(dObj.optString("description", ""))
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

        val dynamicCoverCache = java.util.concurrent.ConcurrentHashMap<String, String>()

        data class CloudFormatVariant(
            val extension: String,
            val size: String,
            val fileSizeBytes: Long
        )

        val KNOWN_CLOUD_FORMAT_VARIANTS = mapOf(
            "0100FC001ACE0000" to listOf( // Anvil Saga
                CloudFormatVariant(".nsp", "694,29 MB", 728016736L),
                CloudFormatVariant(".nsz", "308,51 MB", 323499452L)
            ),
            "010004D00A9C0000" to listOf( // Aggelos
                CloudFormatVariant(".nsp", "112,93 MB", 118411040L),
                CloudFormatVariant(".nsz", "105,46 MB", 110581026L)
            ),
            "010040F01EC60000" to listOf( // Aggelos 2
                CloudFormatVariant(".nsp", "107,12 MB", 112319264L),
                CloudFormatVariant(".nsz", "89,56 MB", 93908837L)
            ),
            "010020D01AD24000" to listOf( // Animal Well
                CloudFormatVariant(".nsp", "37,63 MB", 39457568L),
                CloudFormatVariant(".nsz", "34,83 MB", 36522797L)
            ),
            "0100F3E024DFC000" to listOf( // Another Eden Begins
                CloudFormatVariant(".nsp", "3,61 GB", 3876480288L),
                CloudFormatVariant(".nsz", "3,29 GB", 3535621250L)
            ),
            "01006DD02868A000" to listOf( // Artis Impact
                CloudFormatVariant(".nsp", "1,39 GB", 1496537888L),
                CloudFormatVariant(".nsz", "970,82 MB", 1017977718L)
            ),
            "01008F1008DA6000" to listOf( // Darkest Dungeon [Ancestral Edition]
                CloudFormatVariant(".nsp", "3,26 GB", 3497294656L),
                CloudFormatVariant(".nsz", "1,16 GB", 1243524535L)
            ),
            "0100E5E01C098000" to listOf( // Darkest Dungeon II
                CloudFormatVariant(".nsp", "4,00 GB", 4298739840L),
                CloudFormatVariant(".nsz", "2,45 GB", 2630667468L)
            ),
            "0100F2C0115B6000" to listOf( // The Legend of Zelda: Tears of the Kingdom (ONLY NSZ in cloud)
                CloudFormatVariant(".nsz", "15,45 GB", 16591941089L)
            )
        )

        val SWITCH_CDN_ICONS = mapOf(
            "01007F600B134000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_AssassinsCreedIIIDefinitiveEdition_image500w.jpg",
            "01009B90006DC000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_SuperMarioMaker2_image500w.jpg",
            "0100F3E024DFC000" to "https://assets.nintendo.eu/image/private/f_auto,q_auto,w_500/pnikpcs0uhydw9lw8672",
            "0100FD8022DAA000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/supermariogalaxy2/1x1_NSwitch_SuperMarioGalaxy2_image500w.jpg",
            "058E630A38C70000" to "https://upload.wikimedia.org/wikipedia/en/d/d9/HellfireCoverSmall.jpg",
            "0100C69018E4A000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/1x1_NSwitchDS_TheLastFaith_image500w.jpg",
            "0100CEA007D08000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_CryptOfTheNecroDancerNintendoSwitchEdition_image500w.jpg",
            "0100B36008F90000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/inazumaelevenvictoryroad/1x1_InazumaElevenVictoryRoad_EN_image500w.jpg",
            "0100CA400E300000" to "https://img-eshop.cdn.nintendo.net/i/77cf808ec79894dca98a64d9c0b281f3b513992f84897967f8e115891d4cd91f.jpg",
            "0100000000010000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_SuperMarioOdyssey_Alt01_image500w.jpg",
            "01008CF01BAAC000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_TheLegendofZeldaEchoesOfWisdom_image500w.jpg",
            "01005EC01E6A4000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/1x1_NSwitchDS_DaveTheDiver_image500w.jpg",
            "01001BB01E8E2000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/fantasianneodimension/1x1_FantasianNeoDimension_image500w.jpg",
            "010042D00D900000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_LegoStarWarsTheSkywalkerSaga_image500w.jpg",
            "01007EF00011E000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/wii_u_20/SQ_WiiU_TheLegendOfZeldaBreathOfTheWild_image500w.jpg",
            "0100E65002BB8000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_StardewValley_image500w.jpg",
            "0100EAE010560000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_CaptainTsubasaRiseOfNewChampions_image500w.jpg",
            "0100152000022000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_MarioKart8Deluxe_image500w.jpg",
            "010013F009B88000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_XenoCrisis_image500w.jpg",
            "010075D026910000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/poppyplaytimechapter5/1x1_PoppyPlaytimeChapter5_image500w.jpg",
            "0100BC0018138000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_SuperMarioRPG_image500w.jpg",
            "0100F7901971C000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/dungeons4nintendoswitchedition/1x1_Dungeons4NintendoSwitchEdition_image500w.jpg",
            "01009D6022DC2000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/reus2/1x1_Reus2_image500w.jpg",
            "019232F2781D0000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/virtual_console_nintendo_3ds_8/SQ_3DSVC_DrMario_image500w.jpg",
            "01008E20257E0000" to "https://assets.nintendo.eu/image/private/f_auto,q_auto,w_500/zcxmw0qbm6ytnvgf69ey",
            "010015100B514000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_SuperMarioBrosWonder_image500w.jpg",
            "0100F8F00C4F2000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_DCSuperHeroGirlsTeenPower_image500w.jpg",
            "010093801237C000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_MetroidDread_image500w.jpg",
            "0100F2200C984000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_MortalKombat11_image500w.jpg",
            "0100C2801F22C000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/talesofberseriaremastered/1x1_TalesOfBerseriaRemastered_image500w.jpg",
            "01006560184E6000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_MortalKombat1_image500w.jpg",
            "010034B00E14C000" to "https://upload.wikimedia.org/wikipedia/en/8/80/Tokyo_2020_game_cover.png",
            "010092A0172E4000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_ItTakesTwo_image500w.jpg",
            "010022201229A000" to "https://img-eshop.cdn.nintendo.net/i/2035f3f0fc956dc61d691a2d3234974d6416f8175a9fc0989bfb8e6cf9477c92.jpg",
            "01002FC00412C000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_LittleNightmaresCompleteEdition_image500w.jpg",
            "0100BDA01AABC000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/riftofthenecrodancer/1x1_RiftOfTheNecroDancer_image500w.jpg",
            "0100ECD018EBE000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_PaperMarioTheThousandYearDoor_GB_en_image500w.jpg",
            "0100FA501AF90000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/1x1_NSwitchDS_CastlevaniaDominusCollection_image500w.jpg",
            "010033100691A000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_TheComaRecut_image500w.jpg",
            "01849C8CA8080000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_9/SQ_N64_MarioKart64_image500w.jpg",
            "0100FF100FB68000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_FindingTeddyTwoDefinitiveEdition_image500w.jpg",
            "01001AA022B66000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/tentacletango/1x1_TentacleTango_image500w.jpg",
            "0100A31020078000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/thecoma2bcatacomb/1x1_TheComa2BCatacomb_image500w.jpg",
            "0100F43008C44000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/pokemonlegendsza/1x1_NSwitch2_PokemonLegendsZA_KV_GB_en_image500w.jpg",
            "010003000E146000" to "https://upload.wikimedia.org/wikipedia/en/1/1a/Mario_%26_Sonic_at_the_Olympic_Games_Tokyo_2020_box_art.jpg",
            "0100C3801C786000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/1x1_NSwitchDS_PoppyPlaytimeChapter1_image500w.jpg",
            "010019401051C000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_MarioStrikersBattleLeagueFootball_image500w.jpg",
            "010033001F050000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/ysvstrailsintheskyalternativesaga/1x1_YsVsTrailsInTheSkyAlternativeSaga_image500w.jpg",
            "0100BDE00862A000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_MarioTennisAces_image500w.jpg",
            "01006C900CC60000" to "https://img-eshop.cdn.nintendo.net/i/fc9a60cfb3a86cc0fbb45cc5377cd3d4ae1fa5dd05ebd8be991480992c809d46.jpg",
            "010028600EBDA000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_SuperMario3DWorldAndBowsersFury_image500w.jpg",
            "01000B900D8B0000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_CadenceOfHyruleCryptOfTheNecroDancerFeaturingTheLegendOfZelda_v2_image500w.jpg",
            "01006DD02868A000" to "https://assets.nintendo.eu/image/private/f_auto,q_auto,w_500/mv8kxcjjajbjx536qx0q",
            "0100FC001ACE0000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/anvilsaga/1x1_AnvilSaga_image500w.jpg",
            "01008F1008DA6000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_DarkestDungeon_image500w.jpg",
            "010020D01AD24000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/1x1_NSwitchDS_AnimalWell_V3_image500w.jpg",
            "010059D020C26000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/marvelcosmicinvasion_1/1x1_MarvelCosmicInvasion_image500w.jpg",
            "01007A2027548000" to "https://assets.nintendo.eu/image/private/f_auto,q_auto,w_500/fg0htmfgjrnu87m4m9oo",
            "0100E1C0252F8000" to "https://assets.nintendo.eu/image/private/f_auto,q_auto,w_500/fj3inknfvf8hvgfjerci",
            "0100EC9010258000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_StreetsOfRage4_image500w.jpg",
            "010040502453E000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/vampirecrawlerstheturbowildcardfromvampiresurvivors/1x1_VampireCrawlersTheTurboWildcardFromVampireSurvivors_image500w.jpg",
            "0100BB901FA12000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/littlebigadventuretwinsensquest/1x1_LittleBigAdventureTwinsensQuest_image500w.jpg",
            "01007DE013A48000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_GoldenForce_image500w.jpg",
            "01052BA7AC450000" to "https://www.nintendo.com/eu/media/images/03_teaser_module_1_square/games_3/wiiu_download_software_1/TM_WiiUDS_DuckTalesRemastered_image500w.png",
            "01542031DCEC0000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/virtual_console_nintendo_3ds_8/SQ_3DSVC_SuperMarioBros_image500w.jpg",
            "01006BB00C6F0000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_TheLegendOfZeldaLinksAwakening_image500w.jpg",
            "0100217023F6C000" to "https://assets.nintendo.eu/image/private/f_auto,q_auto,w_500/wk30tmoq2t6mvfhlo4ee",
            "0100317013770000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_MarioAndRabbidsSparksOfHope_enGB_image500w.jpg",
            "010036B0034E4000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_SuperMarioParty_image500w.jpg",
            "0100D3801E6CE000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/poppyplaytimechapter2/1x1_PoppyPlaytimeChapter2_image500w.jpg",
            "01001B300B9BE000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_DiabloIIIEternalCollection_image500w.jpg",
            "010026800E304000" to "https://img-eshop.cdn.nintendo.net/i/7a48b73ce4dbcacbf25a4103ffad0f4414bcefc0a39d94ebcdc28f66e2e09f7d.jpg",
            "010099C022B96000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/supermariogalaxy/1x1_NSwitch_SuperMarioGalaxy_image500w.jpg",
            "0100BAC01E57E000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/1x1_NSwitchDS_YsXNordics_image500w.jpg",
            "010094D023A28000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/drillcore/1x1_DrillCore_image500w.jpg",
            "0100B7C01169C000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_TheComa2ViciousSisters_image500w.jpg",
            "0100307018934000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_Signalis_image500w.jpg",
            "01009720213B0000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/captaintsubasa2worldfighters/1x1_CaptainTsubasa2WorldFighters_image500w.jpg",
            "01005CF01E784000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/1x1_NSwitchDS_TeenageMutantNinjaTurtlesSplinteredFate_new_image500w.jpg",
            "01006D0017F7A000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_MarioLuigiBrothership_Base_GB_en_image500w.jpg",
            "010063301BD50000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/superrobotwarsy/1x1_SuperRobotWarsY_image500w.jpg",
            "0132B3143DF50000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/virtual_console_nintendo_3ds_8/SQ_3DSVC_SuperMarioBros_image500w.jpg",
            "01002C0008E52000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_TalesOfVesperiaDefinitiveEdition_image500w.jpg",
            "0100A3900C3E2000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_PaperMarioTheOrigamiKing_image500w.jpg",
            "0100FBE015910000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_TheLegendOfHeroesTrailsToAzure_image500w.jpg",
            "0100C6A0235D4000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/deviljam/1x1_DevilJam_image500w.jpg",
            "0100EA80032EA000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_NewSuperMarioBrosUDeluxe_image500w.jpg",
            "0100A4601ECA8000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/1x1_NSwitchDS_CryptCustodian_image500w.jpg",
            "01005E701D168000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/talesofgracesfremastered/1x1_TalesOfGracesFRemastered_image500w.jpg",
            "010044700DEB0000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_AssassinsCreedTheRebelCollection_image500w.jpg",
            "010066101A55A000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/littlenightmaresiii_1/1x1_LittleNightmaresIII_image500w.jpg",
            "01008BA02525A000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_2_games/dispatchns2e/1x1_DispatchNS2E_image500w.jpg",
            "9B485EB8" to "https://upload.wikimedia.org/wikipedia/en/a/a5/Grand_Theft_Auto_V.png",
            "0100852026502000" to "https://assets.nintendo.eu/image/private/f_auto,q_auto,w_500/pgsmphppjax0yb5aydgk",
            "010097100EDD6000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_LittleNightmaresII_image500w.jpg",
            "0100622020F5A000" to "https://assets.nintendo.eu/image/private/f_auto,q_auto,w_500/ufxlo3aficvsyiamjzg7",
            "0100E5E01C098000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_DarkestDungeonII_image500w.jpg",
            "01006FE013472000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_MarioPartySuperStars_image500w.jpg",
            "010057901E9E6000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/underlinguprising/1x1_UnderlingUprising_image500w.jpg",
            "0100304027592000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/urbanjungle/1x1_UrbanJungle_image500w.jpg",
            "0100B11027658000" to "https://assets.nintendo.eu/image/private/f_auto,q_auto,w_500/xnznx3wpdmxfyrjxwfq6",
            "0100965017338000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_SuperMarioPartyJamboree_BASE_image500w.jpg",
            "0100646009FBE000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_DeadCells_image500w.jpg",
            "01009A5009A9E000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_ShiningResonanceRefrain_image500w.jpg",
            "010040F01EC60000" to "https://assets.nintendo.eu/image/private/f_auto,q_auto,w_500/uuov0vj1qffnjd27os5d",
            "0100726014352000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_DiabloIIResurrected_image500w.jpg",
            "010067300059A000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_MarioAndRabbidsKingdomBattle_EU_image500w.jpg",
            "0100D59022590000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/scottpilgrimex/1x1_ScottPilgrimEx_image500w.jpg",
            "010004D00A9C0000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_Aggelos_image500w.jpg",
            "01002DA013484000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_TheLegendOfZeldaSkywardSwordHD_image500w.jpg",
            "0100BD601EC3E000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/poppyplaytimechapter3/1x1_PoppyPlaytimeChapter3_image500w.jpg",
            "010089A0197E4000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/1x1_NSwitchDS_VampireSurvivors_image500w.jpg",
            "01002EF01A316000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/1x1_NSwitchDS_Brotato_image500w.jpg",
            "010058C017024000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_Dungeon3NintendoSwitchEdition_image500w.jpg",
            "0100A2902051A000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/poppyplaytimechapter4/1x1_PoppyPlaytimeChapter4_image500w.jpg",
            "0100670014482000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_AssassinsCreedTheEzioCollection_image500w.jpg",
            "0100AC300919A000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_Firewatch_image500w.jpg",
            "0100F1101BB9E000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/talesofxilliaremastered/1x1_TalesOfXilliaRemastered_image500w.jpg",
            "0100919027DBE000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/thecoma3bloodlines/1x1_TheComa3Bloodlines_image500w.jpg",
            "0100B51020B68000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/terminator2dnofate/1x1_Terminator2DNoFate_image500w.jpg",
            "010097F018538000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/1x1_NSwitchDS_DaveTheDiver_image500w.jpg",
            "01001B90277BE000" to "https://assets.nintendo.eu/image/private/f_auto,q_auto,w_500/sm89rnvoxaacheso2a9b",
            "0100EB60202C8000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/kalanoro/1x1_Kalanoro_image500w.jpg",
            "01008970149B0000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_RabbidsPartyOfLegends_EN_image500w.jpg",
            "0100A410169A4000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_TalesOfSymphoniaRemastered_image500w.jpg",
            "0100F2C0115B6000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_TloZTearsOfTheKingdom_BASE_image500w.jpg",
            "010027901C89C000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/hellokittyislandadventure/1x1_HelloKittyIslandAdventure_image500w.jpg",
            "01000EB0276F2000" to "https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/garfieldescapefrommonday/1x1_GarfieldEscapeFromMonday_image500w.jpg",
            "0100B99019412000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_MarioVsDonkeyKong_image500w.jpg",
            "0100C9C00E25C000" to "https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_MarioGolfSuperRush_image500w.jpg",
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

        @Volatile
        private var memoryCachedCatalog: List<StormWorldGameItem>? = null

        fun getCachedCatalog(context: Context): List<StormWorldGameItem> {
            memoryCachedCatalog?.let { return it }
            return try {
                // Baseline bundled assets catalog
                val assetList = mutableListOf<StormWorldGameItem>()
                try {
                    val assetJson = context.assets.open(CATALOG_CACHE_FILE).bufferedReader().use { it.readText() }
                    if (assetJson.isNotBlank()) {
                        val arr = JSONArray(assetJson)
                        for (i in 0 until arr.length()) {
                            val obj = arr.optJSONObject(i) ?: continue
                            assetList.add(StormWorldGameItem.fromJson(obj))
                        }
                    }
                } catch (_: Exception) {}

                val file = File(context.filesDir, CATALOG_CACHE_FILE)
                val pkgInfo = try {
                    context.packageManager.getPackageInfo(context.packageName, 0)
                } catch (_: Exception) { null }
                if (file.exists() && pkgInfo != null && file.lastModified() < pkgInfo.lastUpdateTime) {
                    file.delete()
                }

                val jsonStr = if (file.exists() && file.length() > 0) {
                    file.readText()
                } else {
                    ""
                }

                val list = mutableListOf<StormWorldGameItem>()
                if (jsonStr.isNotBlank()) {
                    val jsonArr = JSONArray(jsonStr)
                    for (i in 0 until jsonArr.length()) {
                        val obj = jsonArr.optJSONObject(i) ?: continue
                        val item = StormWorldGameItem.fromJson(obj)
                        val resolvedCover = if (item.cover.isNotBlank() && item.cover.startsWith("http")) {
                            item.cover
                        } else {
                            SWITCH_CDN_ICONS[item.serialId.uppercase(Locale.ROOT)] ?: item.cover
                        }
                        list.add(item.copy(cover = resolvedCover))
                    }
                }

                // Merge items from bundled assets that may be missing in filesDir cache
                val presentKeys = list.map { "${it.id}|${it.realExtension.lowercase(Locale.ROOT)}" }.toSet()
                for (ai in assetList) {
                    val key = "${ai.id}|${ai.realExtension.lowercase(Locale.ROOT)}"
                    if (key !in presentKeys) {
                        val resolvedCover = if (ai.cover.isNotBlank() && ai.cover.startsWith("http")) {
                            ai.cover
                        } else {
                            SWITCH_CDN_ICONS[ai.serialId.uppercase(Locale.ROOT)] ?: ai.cover
                        }
                        list.add(ai.copy(cover = resolvedCover))
                    }
                }

                if (list.isEmpty()) {
                    for (ai in assetList) {
                        val resolvedCover = if (ai.cover.isNotBlank() && ai.cover.startsWith("http")) {
                            ai.cover
                        } else {
                            SWITCH_CDN_ICONS[ai.serialId.uppercase(Locale.ROOT)] ?: ai.cover
                        }
                        list.add(ai.copy(cover = resolvedCover))
                    }
                }

                // Ensure all KNOWN_CLOUD_FORMAT_VARIANTS are injected if missing
                for ((tid, variants) in KNOWN_CLOUD_FORMAT_VARIANTS) {
                    val baseItem = list.firstOrNull { it.serialId.equals(tid, ignoreCase = true) } ?: continue
                    for (v in variants) {
                        val isNsz = v.extension.equals(".nsz", ignoreCase = true)
                        val targetId = if (isNsz && variants.size > 1) 10000000 + baseItem.rawGameId else baseItem.rawGameId
                        val targetKey = "${targetId}|${v.extension.lowercase(Locale.ROOT)}"
                        val alreadyExists = list.any { "${it.id}|${it.realExtension.lowercase(Locale.ROOT)}" == targetKey }
                        if (!alreadyExists) {
                            val resolvedCover = if (baseItem.cover.isNotBlank() && baseItem.cover.startsWith("http")) {
                                baseItem.cover
                            } else {
                                SWITCH_CDN_ICONS[tid] ?: baseItem.cover
                            }
                            list.add(
                                baseItem.copy(
                                    id = targetId,
                                    realExtension = v.extension,
                                    size = v.size,
                                    fileSizeBytes = v.fileSizeBytes,
                                    cover = resolvedCover
                                )
                            )
                        }
                    }
                }

                val result = list.filterNot { item ->
                    val t = "${item.finalTitle} ${item.title} ${item.version}".lowercase(Locale.ROOT)
                    t.contains("копия") || t.contains("рљропрёсџ") || t.contains("(copy)")
                }.distinctBy { item ->
                    val k = (item.serialId.ifEmpty { item.title }).uppercase(Locale.ROOT)
                    val cleanVer = item.version.split(" ").firstOrNull().orEmpty()
                    val langs = item.textLangs.sorted().joinToString(",")
                    val ext = item.extensionClean
                    val rawId = item.rawGameId
                    "$k|$cleanVer|${item.internalVersion}|$langs|${item.dlcCount}|${item.modCount}|$ext|$rawId"
                }
                memoryCachedCatalog = result
                result
            } catch (e: Exception) {
                Log.error("[StormGamesWorld] Failed to read cached catalog: ${e.message}")
                emptyList()
            }
        }

        fun saveCatalogCache(context: Context, games: List<StormWorldGameItem>) {
            memoryCachedCatalog = games
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
                    .header("User-Agent", "STORM_SWITCH/9.1.0 (Android)")
                    .build()

                val resp = sharedHttpClient.newCall(req).execute()
                val body = resp.body?.string().orEmpty()
                resp.close()

                val jsonArray = JSONArray(body)
                val candidateList = mutableListOf<StormWorldGameItem>()
                val cachedGames = try {
                    getCachedCatalog(context)
                } catch (_: Exception) {
                    emptyList()
                }
                val cachedBySerial = cachedGames.groupBy { it.serialId.uppercase(Locale.ROOT) }

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

                        val gameId = obj.optInt("id")
                        val cleanSerial = serialId.uppercase(Locale.ROOT)
                        val knownVariants = KNOWN_CLOUD_FORMAT_VARIANTS[cleanSerial]

                        if (knownVariants != null && knownVariants.isNotEmpty()) {
                            // Known multiple formats or format override
                            for (v in knownVariants) {
                                val isNsz = v.extension.equals(".nsz", ignoreCase = true)
                                val effectiveId = if (isNsz && knownVariants.size > 1) 10000000 + gameId else gameId
                                candidateList.add(
                                    StormWorldGameItem(
                                        id = effectiveId,
                                        title = rawTitle,
                                        finalTitle = rawFinalTitle,
                                        version = if (rawVersion.isNotEmpty()) rawVersion else "1.0.0",
                                        internalVersion = internalVer,
                                        serialId = serialId,
                                        size = v.size,
                                        fileSizeBytes = v.fileSizeBytes,
                                        cover = obj.optString("cover").ifEmpty {
                                            SWITCH_CDN_ICONS[cleanSerial] ?: ""
                                        },
                                        fileExists = fileExists,
                                        hasFile = hasFile,
                                        regions = regList,
                                        textLangs = langList,
                                        realExtension = v.extension,
                                        dlcCount = dlcNum,
                                        modCount = modNum
                                    )
                                )
                            }
                        } else {
                            val cachedVariants = cachedBySerial[cleanSerial].orEmpty()
                            if (cachedVariants.size > 1) {
                                for (cv in cachedVariants) {
                                    val isNsz = cv.realExtension.equals(".nsz", ignoreCase = true)
                                    val effectiveId = if (isNsz && cv.id >= 10000000) cv.id else if (isNsz) 10000000 + gameId else gameId
                                    candidateList.add(
                                        StormWorldGameItem(
                                            id = effectiveId,
                                            title = rawTitle,
                                            finalTitle = rawFinalTitle,
                                            version = if (rawVersion.isNotEmpty()) rawVersion else "1.0.0",
                                            internalVersion = internalVer,
                                            serialId = serialId,
                                            size = cv.size.ifEmpty { sizeStr },
                                            fileSizeBytes = cv.fileSizeBytes,
                                            cover = obj.optString("cover").ifEmpty {
                                                SWITCH_CDN_ICONS[cleanSerial] ?: ""
                                            },
                                            fileExists = fileExists,
                                            hasFile = hasFile,
                                            regions = regList,
                                            textLangs = langList,
                                            realExtension = cv.realExtension,
                                            dlcCount = dlcNum,
                                            modCount = modNum
                                        )
                                    )
                                }
                            } else {
                                val fullTitleLower = "$rawFinalTitle $rawTitle".lowercase(Locale.ROOT)
                                val detectedExt = when {
                                    fullTitleLower.contains(".nsz") || fullTitleLower.contains("[nsz]") || fullTitleLower.contains("(nsz)") -> ".nsz"
                                    fullTitleLower.contains(".xcz") || fullTitleLower.contains("[xcz]") || fullTitleLower.contains("(xcz)") -> ".xcz"
                                    fullTitleLower.contains(".xci") || fullTitleLower.contains("[xci]") || fullTitleLower.contains("(xci)") -> ".xci"
                                    fullTitleLower.contains(".nsp") || fullTitleLower.contains("[nsp]") || fullTitleLower.contains("(nsp)") -> ".nsp"
                                    cachedVariants.isNotEmpty() && cachedVariants[0].realExtension.isNotEmpty() -> cachedVariants[0].realExtension
                                    else -> ".nsp"
                                }

                                candidateList.add(
                                    StormWorldGameItem(
                                        id = gameId,
                                        title = rawTitle,
                                        finalTitle = rawFinalTitle,
                                        version = if (rawVersion.isNotEmpty()) rawVersion else "1.0.0",
                                        internalVersion = internalVer,
                                        serialId = serialId,
                                        size = sizeStr,
                                        cover = obj.optString("cover").ifEmpty {
                                            SWITCH_CDN_ICONS[cleanSerial] ?: ""
                                        },
                                        fileExists = fileExists,
                                        hasFile = hasFile,
                                        regions = regList,
                                        textLangs = langList,
                                        realExtension = detectedExt,
                                        dlcCount = dlcNum,
                                        modCount = modNum
                                    )
                                )
                            }
                        }
                    }
                }

                val verifiedGames = candidateList
                    .filterNot { item ->
                        val t = "${item.finalTitle} ${item.title} ${item.version}".lowercase(Locale.ROOT)
                        t.contains("копия") || t.contains("рљропрёсџ") || t.contains("(copy)")
                    }
                    .distinctBy { item ->
                        val k = (item.serialId.ifEmpty { item.title }).uppercase(Locale.ROOT)
                        val cleanVer = item.version.split(" ").firstOrNull().orEmpty()
                        val langs = item.textLangs.sorted().joinToString(",")
                        val ext = item.extensionClean
                        val rawId = item.rawGameId
                        "$k|$cleanVer|${item.internalVersion}|$langs|${item.dlcCount}|${item.modCount}|$ext|$rawId"
                    }

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

                    // If all items only differ by extension/format (same version, dlc, mod), don't show recommended star
                    val distinctVersions = list.map { "${it.version}|${it.dlcCount}|${it.modCount}" }.distinct()
                    if (distinctVersions.size <= 1) {
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
                                if (verCmp != 0) verCmp else {
                                    val extCmp = a.extensionClean.compareTo(b.extensionClean)
                                    if (extCmp != 0) extCmp else a.id.compareTo(b.id)
                                }
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

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

    private val SWITCH_CDN_ICONS = mapOf(
        "01007EF00011E000" to "https://img-eshop.cdn.nintendo.net/i/4b53da7ca4b118fe37c8b8040609b84dc63214d6131c51592486de9bf29ef29c.jpg",
        "01002FC00412C000" to "https://img-eshop.cdn.nintendo.net/i/bf7b71eb6ac6eae6bc9cf6e49b275b6e933622e88e32550a4929b2d1c82a6dd0.jpg",
        "0100E65002BB8000" to "https://img-eshop.cdn.nintendo.net/i/5f00bc8a8440171d41735d4d8960d2d3f476c9e64c9910ead8bff53b9654385f.jpg",
        "0100CEA007D08000" to "https://img-eshop.cdn.nintendo.net/i/37eab904acbc030394ebf0f5e0856c83c6870fc077c71b0e253321be8a92a511.jpg",
        "0100AC300919A000" to "https://img-eshop.cdn.nintendo.net/i/29f552314dc9a0d628a06c3cb80844b9c87ea63ac333fb63d117f827e5d4b8db.jpg",
        "01007F600B134000" to "https://img-eshop.cdn.nintendo.net/i/bb542985ad06d20f09e302414d0d02942ebccb71cd44f5c889b2165bbfa01ee9.jpg",
        "01001B300B9BE000" to "https://img-eshop.cdn.nintendo.net/i/5850dd84fdb585d3d76947209daf6df8acedb3c822e3674b41ed864e055ab073.jpg",
        "0100F2200C984000" to "https://img-eshop.cdn.nintendo.net/i/7f63d6a81a6b979cc9fb28f52bd1ddb7510aca8ae43c8dfe6fd62b1a5b03968a.jpg",
        "010042D00D900000" to "https://img-eshop.cdn.nintendo.net/i/c3153c6369c02e79063886b0adce635b3dd3acc2b0b84b3aba2e5b7fb9c29d50.jpg",
        "010044700DEB0000" to "https://img-eshop.cdn.nintendo.net/i/244a80c16d8c248f606fbfd9550a3f073903d29b268316686c3586757995145f.jpg",
        "01006BB00C6F0000" to "https://img-eshop.cdn.nintendo.net/i/50f8839e28d06296e7e0eea8ba061b89a227743bb7d5c61ffebf5aa0646d9ea5.jpg",
        "01000B900D8B0000" to "https://img-eshop.cdn.nintendo.net/i/7d5dcf746e8c647b967ec8dde69f7c4f5dd0935957f707cd09bec98346a83ab2.jpg",
        "010097100EDD6000" to "https://img-eshop.cdn.nintendo.net/i/02e4efea58daaf58ba022c711c3b79b6d3dff69e0cbc9926f019c0b3043361e2.jpg",
        "0100EC9010258000" to "https://img-eshop.cdn.nintendo.net/i/664d1fb5429b3dbf0943244d40bb8d68fe4ec17a0f5a7fa55ab037ec8c2b59b5.jpg",
        "01002DA013484000" to "https://img-eshop.cdn.nintendo.net/i/0ce5f0940911eab2cc34eb41b121e84c9d00081d02b816585851f9f1819bb256.jpg",
        "0100670014482000" to "https://img-eshop.cdn.nintendo.net/i/f1bcd4eac7a70fe9dc33b172ec5135e97cd7ed5beba3b2fb23f110cab7634b44.jpg",
        "0100726014352000" to "https://img-eshop.cdn.nintendo.net/i/eedca17878b20f750e6150c371c5a5d210484eb125c5d1042c366d11616b5c36.jpg",
        "010093801237C000" to "https://img-eshop.cdn.nintendo.net/i/37e0b0a38fc81f8cd10c2a0721747dc919a1662ac7ec19c20d107603b99cbed1.jpg",
        "01006560184E6000" to "https://img-eshop.cdn.nintendo.net/i/e9090ef9b0c7f88185541a74884611bff8898388feca779548cf21791333f2ef.jpg",
        "0100307018934000" to "https://img-eshop.cdn.nintendo.net/i/ca56cc14f1cb24dbdb2d110d614ae36a7a415b83890cc335a9f7a28af1a2b5a6.jpg",
        "010089A0197E4000" to "https://img-eshop.cdn.nintendo.net/i/d08e3b85e22e18030afb75a13239933b37158bc603a1f6b6bb9d12a4d1c55eb7.jpg",
        "010097F018538000" to "https://img-eshop.cdn.nintendo.net/i/eb29eeb114f357c2c5b5ec09cbf7dfe5909079ec48369bdf5ea71ece1bd56772.jpg",
        "01002EF01A316000" to "https://img-eshop.cdn.nintendo.net/i/e580976bf46d50465e5a333d6caa66db38f4132e64bc525cfecfa6299e71176b.jpg",
        "010066101A55A000" to "https://img-eshop.cdn.nintendo.net/i/e513b44913974d42d6820bb92d91e07303f0ae6218f49e4873ff423ddcb2a86b.jpg",
        "0100F2C0115B6000" to "https://img-eshop.cdn.nintendo.net/i/404244f33a9808ec62bcb41c6a0d659fac887cb3ee376e3e42c91ab28548e06f.jpg",
        "0100BDA01AABC000" to "https://img-eshop.cdn.nintendo.net/i/d3ee8bbe25eba57c4d04b22589ef1c8fe8157381380be14880c94339ce67cc30.jpg",
        "010020D01AD24000" to "https://img-eshop.cdn.nintendo.net/i/d3bbbb4de0b03636dd547272d502deabe1f77a8a4ba4143591c8575d0af35a37.jpg",
        "010015100B514000" to "https://img-eshop.cdn.nintendo.net/i/0680f7ea84f9d434ff7b0f73f23f86c35b0c8e8f8ed6733b831e42762289eca7.jpg",
        "0100BAC01E57E000" to "https://img-eshop.cdn.nintendo.net/i/ae24a7b74e26a1d24f66895d3d2f11b2ae9573f607ba45cb99c10396f412b995.jpg",
        "01005CF01E784000" to "https://img-eshop.cdn.nintendo.net/i/a439d1cb1e81700d962b1cb25b7ee2efff8c450c82ae4959d8a832a20448f54a.jpg",
        "01005EC01E6A4000" to "https://img-eshop.cdn.nintendo.net/i/a439d1cb1e81700d962b1cb25b7ee2efff8c450c82ae4959d8a832a20448f54a.jpg",
        "010057901E9E6000" to "https://img-eshop.cdn.nintendo.net/i/62a42fe044c56b93aad399068b82c89f4dd861d33a82ba61c92b730f20dd6be3.jpg",
        "01008CF01BAAC000" to "https://img-eshop.cdn.nintendo.net/i/4c72018535cd7b3efaf0f9a02fb5a54fa00e4b843f142a45e0aa900bef517c67.jpg",
        "010059D020C26000" to "https://img-eshop.cdn.nintendo.net/i/92e171e19f7896269d3b57236141a4dcab874eb7080a34c69082d643962d17b7.jpg",
        "0100D59022590000" to "https://img-eshop.cdn.nintendo.net/i/ffd0c05f5754a29f9ff0c6500b3399c7a8dac6dab455634c4668580dd7607974.jpg",
        "0100C6A0235D4000" to "https://img-eshop.cdn.nintendo.net/i/996e6300fe6ebc896a707f16ea6b341eeff0bd089f7a070b12398b68124f83c8.jpg",
        "010094D023A28000" to "https://img-eshop.cdn.nintendo.net/i/0d794a22aa99f33a9d443b1a776e71a934f06520dcec52e3011af922eea75dcf.jpg",
        "010063301BD50000" to "https://img-eshop.cdn.nintendo.net/i/be3e65dea8c7bac88660cdd55d331af2434959e172d6174b302d259bfb022c69.jpg",
        "010040502453E000" to "https://img-eshop.cdn.nintendo.net/i/72d61bf1246ef96fb916b55777023c66e8906df78f8ae8db16425eafedfbf08e.jpg",
        "01008BA02525A000" to "https://img-eshop.cdn.nintendo.net/i/bfc720168c2244f1ceaa0a7bc1e3e57bfe9b3862d5fa3acfbd816e08e9e82dc0.jpg",
        "0100B11027658000" to "https://img-eshop.cdn.nintendo.net/i/48d225712ec56a0b0b87cf332b70c9ce8584415507063ddc62ac77b8bb11996f.jpg",
        "0100CA400E300000" to "https://img-eshop.cdn.nintendo.net/i/cabdadca35545f14e0b34a18ae8e4657dc4f9fd71f58c7ffd5aad2decd65af6d.jpg",
        "010026800E304000" to "https://img-eshop.cdn.nintendo.net/i/c208f110ff193d568799a2fc2bcb04b096ecfe9f1ba6f91f0a94c257ffde3b67.jpg",
        "010022201229A000" to "https://img-eshop.cdn.nintendo.net/i/5b8022601f719dc8e51a32852065029c85b12fa1b4f6d60bf03f2148d7618dc4.jpg",
        "01006C900CC60000" to "https://img-eshop.cdn.nintendo.net/i/37e0b0a38fc81f8cd10c2a0721747dc919a1662ac7ec19c20d107603b99cbed1.jpg"
    )

    private fun loadCoverForGame(item: StormWorldGameItem, imageView: android.widget.ImageView) {
        val localGame = gamesViewModel.games.value.firstOrNull { it.programIdHex.equals(item.serialId, ignoreCase = true) }
        if (localGame != null) {
            org.yuzu.yuzu_emu.utils.GameIconUtils.loadGameIcon(localGame, imageView)
            return
        }
        val url = if (item.cover.isNotBlank() && item.cover.startsWith("http")) item.cover else SWITCH_CDN_ICONS[item.serialId.uppercase(Locale.ROOT)]
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
        setStyle(STYLE_NORMAL, R.style.Theme_Yuzu_Main)
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
                    .header("User-Agent", "STORM_SWITCH/8.0.5 (Android)")
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

                        parsed.add(
                            StormWorldGameItem(
                                id = obj.optInt("id"),
                                title = rawTitle,
                                finalTitle = rawFinalTitle,
                                version = if (rawVersion.isNotEmpty()) rawVersion else "1.0.0",
                                internalVersion = internalVer,
                                serialId = serialId,
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
        binding.detailGameDescription.text = "Загрузка информации..."

        loadCoverForGame(game, binding.detailGameCover)

        val targetDir = getTargetDownloadDirectoryDescription()
        binding.detailSaveFolder.text = "📁 Каталог: $targetDir"

        if (game.isDownloaded) {
            binding.btnStartDownload.text = "Уже скачано"
            binding.btnStartDownload.setIconResource(R.drawable.ic_check)
            binding.btnStartDownload.isEnabled = false
        } else {
            binding.btnStartDownload.text = "Скачать игру"
            binding.btnStartDownload.setIconResource(R.drawable.ic_install)
            binding.btnStartDownload.isEnabled = !isDownloading
        }

        fetchGameDetails(game)
    }

    private fun fetchGameDetails(game: StormWorldGameItem) {
        lifecycleScope.launch(Dispatchers.IO) {
            try {
                val req = Request.Builder()
                    .url("https://stormgamesworld.ru/api/games?id=${game.id}")
                    .header("User-Agent", "STORM_SWITCH/8.0.5 (Android)")
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
                        .header("User-Agent", "STORM_SWITCH/8.0.5 (Android)")
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
                    .header("User-Agent", "STORM_SWITCH/8.0.5 (Android)")
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
            val dispTitle = if (item.title.isNotBlank()) item.title else item.finalTitle

            holder.b.textGameTitle.text = dispTitle
            holder.b.textGameVersion.text = item.version
            if (item.internalVersion.isNotEmpty()) {
                holder.b.textInternalVersion.isVisible = true
                holder.b.textInternalVersion.text = item.internalVersion
            } else {
                holder.b.textInternalVersion.isVisible = false
            }
            holder.b.textGameSize.text = item.size
            holder.b.textGameLangs.text = if (item.textLangs.isNotEmpty()) item.textLangs.joinToString(", ") else "Multi"
            holder.b.textGameSerial.text = item.serialId

            loadCoverForGame(item, holder.b.imageGameCover)

            if (item.isDownloaded) {
                holder.b.btnGameAction.text = "Скачано"
                holder.b.btnGameAction.setIconResource(R.drawable.ic_check)
                holder.b.btnGameAction.isEnabled = false
                holder.b.btnGameAction.strokeColor = android.content.res.ColorStateList.valueOf(0xFF00FF66.toInt())
            } else {
                holder.b.btnGameAction.text = "Скачать"
                holder.b.btnGameAction.setIconResource(R.drawable.ic_install)
                holder.b.btnGameAction.isEnabled = !isDownloading
                holder.b.btnGameAction.strokeColor = android.content.res.ColorStateList.valueOf(0xFF38BDF8.toInt())
            }

            holder.b.btnGameAction.setOnClickListener {
                onGameSelected(item)
                if (!item.isDownloaded && !isDownloading) {
                    startDownload(item)
                }
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
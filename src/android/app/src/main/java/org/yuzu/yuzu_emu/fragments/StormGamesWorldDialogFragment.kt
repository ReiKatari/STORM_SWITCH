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
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import okhttp3.Call
import okhttp3.OkHttpClient
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
    val dlcs: MutableList<StormWorldDlcItem> = mutableListOf()
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
    private var isPaused = false
    private var isCancelled = false

    private val httpClient = OkHttpClient.Builder()
        .connectTimeout(30, TimeUnit.SECONDS)
        .readTimeout(60, TimeUnit.SECONDS)
        .writeTimeout(60, TimeUnit.SECONDS)
        .retryOnConnectionFailure(true)
        .build()

private val SWITCH_CDN_ICONS = mapOf(
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
            isCancelled = false
            isPaused = false
            startDownload(game)
        }

        binding.btnPauseDownload.setOnClickListener {
            togglePauseDownload()
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
                    .header("User-Agent", "STORM_SWITCH/8.1.2 (Android)")
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

                        val dlcMatch = Regex("""(?:\+|[\(\[])(\d+)D(?:\)|\]|\+)""", RegexOption.IGNORE_CASE).find(rawFinalTitle)
                        val dlcNum = dlcMatch?.groupValues?.get(1)?.toIntOrNull() ?: 0

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
                                textLangs = langList,
                                dlcCount = dlcNum
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

        binding.detailGameDescription.text = "Загрузка информации..."

        loadCoverForGame(game, binding.detailGameCover)

        val targetDir = getTargetDownloadDirectoryDescription()
        binding.detailSaveFolder.text = "📁 Каталог: $targetDir"

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
            binding.btnStartDownload.isEnabled = !isDownloading
            binding.btnStartDownload.backgroundTintList = android.content.res.ColorStateList.valueOf(0x00000000)
            binding.btnStartDownload.strokeColor = android.content.res.ColorStateList.valueOf(0xFF334155.toInt())
            binding.btnStartDownload.setTextColor(0xFF94A3B8.toInt())
            binding.btnStartDownload.iconTint = android.content.res.ColorStateList.valueOf(0xFF94A3B8.toInt())
        }

        fetchGameDetails(game)
    }

    private fun fetchGameDetails(game: StormWorldGameItem) {
        lifecycleScope.launch(Dispatchers.IO) {
            try {
                val req = Request.Builder()
                    .url("https://stormgamesworld.ru/api/games?id=${game.id}")
                    .header("User-Agent", "STORM_SWITCH/8.1.2 (Android)")
                    .build()
                val resp = httpClient.newCall(req).execute()
                val body = resp.body?.string().orEmpty()
                val obj = JSONObject(body)

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
                        val dlcName = dObj.optString("name", "Дополнение ${d + 1}")
                        val dlcDesc = dObj.optString("description", "")
                        game.dlcs.add(StormWorldDlcItem(id = dlcId, name = dlcName, description = dlcDesc))
                    }
                    if (game.dlcs.isNotEmpty()) {
                        game.dlcCount = game.dlcs.size
                    }
                }

                // Also check HEAD Content-Disposition to detect real extension (.nsz / .xci / .nsp)
                try {
                    val headReq = Request.Builder()
                        .url("https://stormgamesworld.ru/api/games/${game.id}/download")
                        .head()
                        .header("User-Agent", "STORM_SWITCH/8.1.2 (Android)")
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
                    if (game.dlcCount > 0) {
                        binding.detailGameDlc.isVisible = true
                        binding.detailGameDlc.text = "+${game.dlcCount} DLC"
                        binding.detailGameDlc.setOnClickListener {
                            showDlcDialog(game)
                        }
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

    private fun togglePauseDownload() {
        if (!isDownloading && !isPaused) return

        if (!isPaused) {
            isPaused = true
            activeDownloadCall?.cancel()
            binding.btnPauseDownload.text = "Продолжить"
            binding.textDownloadStats.text = "⏸ Загрузка приостановлена"
            binding.btnStartDownload.isEnabled = true
            binding.btnStartDownload.text = "Возобновить"
        } else {
            isPaused = false
            binding.btnPauseDownload.text = "Пауза"
            val game = selectedGame ?: return
            startDownload(game)
        }
    }

    private fun startDownload(game: StormWorldGameItem) {
        if (isDownloading) return

        isDownloading = true
        isCancelled = false
        isPaused = false
        binding.btnStartDownload.isEnabled = false
        binding.layoutDownloadProgress.isVisible = true
        binding.progressDownload.isIndeterminate = true
        binding.btnPauseDownload.text = "Пауза"
        binding.textDownloadStats.text = "Подключение к серверу загрузки..."

        lifecycleScope.launch(Dispatchers.IO) {
            val cleanTitle = (if (game.finalTitle.isNotEmpty()) game.finalTitle else game.title)
                .replace(Regex("[\\\\/:*?\"<>|]"), "_")
                .trim()
            val baseFilename = if (cleanTitle.endsWith(".nsp", ignoreCase = true) ||
                cleanTitle.endsWith(".xci", ignoreCase = true) ||
                cleanTitle.endsWith(".nsz", ignoreCase = true)) {
                cleanTitle
            } else {
                "$cleanTitle${game.realExtension}"
            }
            val partFilename = "$baseFilename.part"

            val gameDirs = NativeConfig.getGameDirs()
            val firstDir = gameDirs.firstOrNull()
            var isSaf = false
            var targetFolder: File? = null
            var safTree: DocumentFile? = null
            var targetDocPart: DocumentFile? = null
            var targetNormalPart: File? = null

            if (firstDir != null) {
                val dirUri = Uri.parse(firstDir.uriString)
                if (dirUri.scheme == "content") {
                    isSaf = true
                    safTree = DocumentFile.fromTreeUri(requireContext(), dirUri)
                } else {
                    val p = dirUri.path ?: firstDir.uriString
                    targetFolder = File(p)
                    if (!targetFolder.exists()) targetFolder.mkdirs()
                }
            }
            if (!isSaf && targetFolder == null) {
                targetFolder = File(
                    Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS),
                    "STORM_SWITCH_GAMES"
                )
                if (!targetFolder.exists()) targetFolder.mkdirs()
            }

            var retryCount = 0
            val maxRetries = 5
            var completed = false

            while (retryCount <= maxRetries && !isCancelled && !isPaused && !completed) {
                var outputStream: OutputStream? = null
                var inputStream: InputStream? = null

                try {
                    var existingBytes = 0L

                    if (isSaf && safTree != null) {
                        targetDocPart = safTree.findFile(partFilename)
                        if (targetDocPart != null && targetDocPart.exists()) {
                            existingBytes = targetDocPart.length()
                            outputStream = requireContext().contentResolver.openOutputStream(targetDocPart.uri, "wa")
                        } else {
                            targetDocPart = safTree.createFile("application/octet-stream", partFilename)
                            if (targetDocPart != null) {
                                outputStream = requireContext().contentResolver.openOutputStream(targetDocPart.uri, "w")
                            }
                        }
                    } else if (targetFolder != null) {
                        targetNormalPart = File(targetFolder, partFilename)
                        if (targetNormalPart.exists()) {
                            existingBytes = targetNormalPart.length()
                        }
                        outputStream = FileOutputStream(targetNormalPart, true)
                    }

                    if (outputStream == null) {
                        throw RuntimeException("Не удалось открыть файл для записи")
                    }

                    val downloadUrl = "https://stormgamesworld.ru/api/games/${game.id}/download"
                    val reqBuilder = Request.Builder()
                        .url(downloadUrl)
                        .header("User-Agent", "STORM_SWITCH/8.1.2 (Android)")

                    if (existingBytes > 0L) {
                        reqBuilder.header("Range", "bytes=$existingBytes-")
                    }

                    val req = reqBuilder.build()
                    val call = httpClient.newCall(req)
                    activeDownloadCall = call

                    withContext(Dispatchers.Main) {
                        if (_binding != null) {
                            if (retryCount > 0) {
                                binding.textDownloadStats.text = "Восстановление соединения (попытка $retryCount из $maxRetries)..."
                            } else if (existingBytes > 0L) {
                                val mb = existingBytes / (1024 * 1024)
                                binding.textDownloadStats.text = "Возобновление загрузки с ${mb} МБ..."
                            } else {
                                binding.textDownloadStats.text = "Подключение к серверу загрузки..."
                            }
                        }
                    }

                    val resp = call.execute()

                    if (resp.code == 416) {
                        outputStream.close()
                        completed = true
                        break
                    }

                    if (!resp.isSuccessful) {
                        throw RuntimeException("HTTP ${resp.code}: ${resp.message}")
                    }

                    val body = resp.body ?: throw RuntimeException("Пустой ответ сервера")
                    val isPartial = (resp.code == 206)

                    if (existingBytes > 0L && !isPartial) {
                        outputStream.close()
                        if (isSaf && targetDocPart != null) {
                            outputStream = requireContext().contentResolver.openOutputStream(targetDocPart.uri, "w")
                        } else if (targetNormalPart != null) {
                            outputStream = FileOutputStream(targetNormalPart, false)
                        }
                        existingBytes = 0L
                    }

                    val streamLen = body.contentLength()
                    val totalBytes = if (isPartial) {
                        existingBytes + (if (streamLen > 0) streamLen else 0L)
                    } else {
                        if (streamLen > 0) streamLen else game.fileSizeBytes
                    }

                    inputStream = body.byteStream()
                    val buffer = ByteArray(256 * 1024)
                    var bytesRead: Int
                    var totalRead = existingBytes
                    var lastUpdateTime = System.currentTimeMillis()
                    var bytesSinceLastUpdate = 0L
                    var currentSpeedMbps = 0.0

                    withContext(Dispatchers.Main) {
                        if (_binding != null) {
                            binding.progressDownload.isIndeterminate = false
                        }
                    }

                    while (inputStream.read(buffer).also { bytesRead = it } != -1) {
                        if (isPaused || isCancelled) break

                        outputStream!!.write(buffer, 0, bytesRead)
                        totalRead += bytesRead
                        bytesSinceLastUpdate += bytesRead

                        val now = System.currentTimeMillis()
                        val delta = now - lastUpdateTime
                        if (delta >= 500) {
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
                                    "%.1f МБ из %.1f МБ (%d%%) • %.2f МБ/с • Ост: %02d:%02d",
                                    recMb, totMb, pct, currentSpeedMbps, etaSec / 60, etaSec % 60
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

                    outputStream?.flush()

                    if (!isPaused && !isCancelled) {
                        completed = true
                        break
                    }

                } catch (e: Exception) {
                    if (isPaused || isCancelled) {
                        break
                    }
                    Log.error("[StormGamesWorld] Download interrupted: ${e.message}")
                    retryCount++
                    if (retryCount <= maxRetries) {
                        withContext(Dispatchers.Main) {
                            if (_binding != null) {
                                binding.textDownloadStats.text = "Обрыв соединения (${e.localizedMessage ?: "Сбой"}). Повтор $retryCount из $maxRetries..."
                            }
                        }
                        delay(retryCount * 1500L)
                    } else {
                        throw e
                    }
                } finally {
                    try { inputStream?.close() } catch (_: Exception) {}
                    try { outputStream?.flush() } catch (_: Exception) {}
                    try { outputStream?.close() } catch (_: Exception) {}
                }
            }

            if (completed) {
                try {
                    if (isSaf && safTree != null && targetDocPart != null) {
                        val existingFinal = safTree.findFile(baseFilename)
                        if (existingFinal != null && existingFinal.exists()) {
                            existingFinal.delete()
                        }
                        targetDocPart.renameTo(baseFilename)
                    } else if (targetNormalPart != null) {
                        val finalFile = File(targetNormalPart.parentFile, baseFilename)
                        if (finalFile.exists()) finalFile.delete()
                        targetNormalPart.renameTo(finalFile)
                    }
                } catch (e: Exception) {
                    Log.error("[StormGamesWorld] Rename error: ${e.message}")
                }

                withContext(Dispatchers.Main) {
                    isDownloading = false
                    isPaused = false
                    activeDownloadCall = null
                    game.isDownloaded = true
                    if (_binding != null) {
                        binding.layoutDownloadProgress.isVisible = false
                        binding.btnStartDownload.text = "Скачано"
                        binding.btnStartDownload.setIconResource(R.drawable.ic_check)
                        binding.btnStartDownload.isEnabled = false
                        binding.btnStartDownload.backgroundTintList = android.content.res.ColorStateList.valueOf(0xFF10B981.toInt())
                        binding.btnStartDownload.strokeColor = android.content.res.ColorStateList.valueOf(0xFF10B981.toInt())
                        binding.btnStartDownload.setTextColor(0xFFFFFFFF.toInt())
                        binding.btnStartDownload.iconTint = android.content.res.ColorStateList.valueOf(0xFFFFFFFF.toInt())
                        binding.recyclerGames.adapter?.notifyDataSetChanged()
                    }
                    Toast.makeText(requireContext(), "✅ Игра успешно скачана: ${game.finalTitle.ifEmpty { game.title }}", Toast.LENGTH_LONG).show()
                    gamesViewModel.reloadGames(directoriesChanged = true)
                }
            } else if (isPaused) {
                withContext(Dispatchers.Main) {
                    isDownloading = false
                    activeDownloadCall = null
                    if (_binding != null) {
                        binding.btnPauseDownload.text = "Продолжить"
                        binding.btnStartDownload.isEnabled = true
                        binding.btnStartDownload.text = "Возобновить"
                    }
                }
            } else if (!isCancelled) {
                withContext(Dispatchers.Main) {
                    isDownloading = false
                    activeDownloadCall = null
                    if (_binding != null) {
                        binding.btnStartDownload.isEnabled = true
                        binding.btnStartDownload.text = "Продолжить скачивание"
                        binding.textDownloadStats.text = "Загрузка прервана. Нажмите «Продолжить скачивание»."
                    }
                    Toast.makeText(requireContext(), "Загрузка прервана. Прогресс сохранён — можно докачать!", Toast.LENGTH_LONG).show()
                }
            }
        }
    }

    private fun cancelDownload() {
        if (!isDownloading && !isPaused) return
        isCancelled = true
        isPaused = false
        activeDownloadCall?.cancel()
        activeDownloadCall = null
        isDownloading = false
        binding.layoutDownloadProgress.isVisible = false
        binding.btnStartDownload.text = "Скачать игру"
        binding.btnStartDownload.setIconResource(R.drawable.ic_install)
        binding.btnStartDownload.isEnabled = true
        binding.btnStartDownload.backgroundTintList = android.content.res.ColorStateList.valueOf(0x00000000)
        binding.btnStartDownload.strokeColor = android.content.res.ColorStateList.valueOf(0xFF334155.toInt())
        binding.btnStartDownload.setTextColor(0xFF94A3B8.toInt())
        binding.btnStartDownload.iconTint = android.content.res.ColorStateList.valueOf(0xFF94A3B8.toInt())
        binding.btnPauseDownload.text = "Пауза"
        Toast.makeText(requireContext(), "Загрузка отменена (прогресс сохранён)", Toast.LENGTH_SHORT).show()
    }

    override fun onDestroyView() {
        if (isDownloading) {
            isPaused = true
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
                holder.b.btnGameAction.isEnabled = !isDownloading
                holder.b.btnGameAction.backgroundTintList = android.content.res.ColorStateList.valueOf(0x00000000)
                holder.b.btnGameAction.strokeColor = android.content.res.ColorStateList.valueOf(0xFF334155.toInt())
                holder.b.btnGameAction.setTextColor(0xFF94A3B8.toInt())
                holder.b.btnGameAction.iconTint = android.content.res.ColorStateList.valueOf(0xFF94A3B8.toInt())
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
                        .header("User-Agent", "STORM_SWITCH/8.1.1 (Android)")
                        .build()
                    val resp = httpClient.newCall(req).execute()
                    val body = resp.body?.string().orEmpty()
                    val obj = JSONObject(body)
                    val dlcsArr = obj.optJSONArray("dlcs")

                    val fetchedDlcs = mutableListOf<StormWorldDlcItem>()
                    if (dlcsArr != null && dlcsArr.length() > 0) {
                        for (d in 0 until dlcsArr.length()) {
                            val dObj = dlcsArr.optJSONObject(d) ?: continue
                            val dlcId = dObj.optString("id", "")
                            val dlcName = dObj.optString("name", "Дополнение ${d + 1}")
                            val dlcDesc = dObj.optString("description", "")
                            fetchedDlcs.add(StormWorldDlcItem(id = dlcId, name = dlcName, description = dlcDesc))
                        }
                    }

                    if (fetchedDlcs.isEmpty() && game.dlcCount > 0) {
                        for (i in 1..game.dlcCount) {
                            fetchedDlcs.add(
                                StormWorldDlcItem(
                                    id = "",
                                    name = "Официальное дополнение #$i",
                                    description = "Контент из расширенного издания игры"
                                )
                            )
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
                    withContext(Dispatchers.Main) {
                        if (!dialog.isShowing) return@withContext
                        if (dlcList.isEmpty() && game.dlcCount > 0) {
                            for (i in 1..game.dlcCount) {
                                dlcList.add(
                                    StormWorldDlcItem(
                                        id = "",
                                        name = "Официальное дополнение #$i",
                                        description = "Входит в состав данного релиза"
                                    )
                                )
                            }
                            adapter.notifyDataSetChanged()
                        }
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

        fun newInstance(): StormGamesWorldDialogFragment {
            return StormGamesWorldDialogFragment()
        }
    }
}
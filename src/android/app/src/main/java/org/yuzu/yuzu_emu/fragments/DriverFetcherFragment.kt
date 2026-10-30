// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project

// SPDX-License-Identifier: GPL-3.0-or-later



package org.yuzu.yuzu_emu.fragments



import android.os.Bundle

import androidx.fragment.app.Fragment

import android.view.LayoutInflater

import android.view.View

import android.view.ViewGroup

import androidx.core.view.ViewCompat

import androidx.core.view.WindowInsetsCompat

import androidx.core.view.isVisible

import androidx.core.view.updatePadding

import androidx.fragment.app.activityViewModels

import androidx.recyclerview.widget.LinearLayoutManager

import androidx.lifecycle.Lifecycle

import androidx.lifecycle.lifecycleScope

import androidx.lifecycle.repeatOnLifecycle

import com.fasterxml.jackson.databind.JsonNode

import com.fasterxml.jackson.module.kotlin.jacksonObjectMapper

import com.google.android.material.dialog.MaterialAlertDialogBuilder

import com.google.android.material.transition.MaterialSharedAxis

import kotlinx.coroutines.CoroutineScope

import kotlinx.coroutines.Dispatchers

import kotlinx.coroutines.launch

import kotlinx.coroutines.withContext

import okhttp3.OkHttpClient

import okhttp3.Request

import org.yuzu.yuzu_emu.R

import org.yuzu.yuzu_emu.databinding.FragmentDriverFetcherBinding

import org.yuzu.yuzu_emu.features.fetcher.DriverGroupAdapter

import org.yuzu.yuzu_emu.model.DriverViewModel

import org.yuzu.yuzu_emu.model.HomeViewModel

import org.yuzu.yuzu_emu.utils.GpuDriverHelper

import org.yuzu.yuzu_emu.utils.SmartDns

import org.yuzu.yuzu_emu.utils.ViewUtils.updateMargins

import java.io.IOException

import java.net.URL

import java.time.Instant

import java.time.LocalDateTime

import java.time.ZoneId

import kotlin.getValue



class DriverFetcherFragment : Fragment() {

    private var _binding: FragmentDriverFetcherBinding? = null

    private val binding get() = _binding!!



    private val gpuModel: String?

        get() = GpuDriverHelper.hookLibPath?.let { GpuDriverHelper.getGpuModel(hookLibPath = it) }



    private val adrenoModel: Int

        get() = parseAdrenoModel()



    private val isMaliOrNonAdrenoGpu: Boolean

        get() {

            val model = (gpuModel ?: "").lowercase()

            return model.contains("mali") || model.contains("immortalis") || model.contains("xclipse")

        }



    private val recommendedDriver: String

        get() {

            if (isMaliOrNonAdrenoGpu) {

                return getString(R.string.system_gpu_driver)

            }

            return driverMap.firstOrNull { adrenoModel in it.first }?.second ?: "Unsupported"

        }



    private val activeDriverInfo: String

        get() {

            val customData = GpuDriverHelper.customDriverSettingData

            val path = org.yuzu.yuzu_emu.features.settings.model.StringSetting.DRIVER_PATH.getString()

            return if (path.isEmpty() || customData == org.yuzu.yuzu_emu.utils.GpuDriverMetadata()) {

                val sys = GpuDriverHelper.getSystemDriverInfo()

                val sysVer = org.yuzu.yuzu_emu.NativeLibrary.getVulkanDriverVersion().takeIf { !it.isNullOrEmpty() }

                    ?: sys?.get(0) ?: getString(R.string.system_gpu_driver)

                "${getString(R.string.system_gpu_driver)} ($sysVer)"

            } else {

                val vendor = customData.vendor?.takeIf { it.isNotBlank() } ?: "Custom"

                val name = customData.name?.takeIf { it.isNotBlank() } ?: java.io.File(path).name

                val version = customData.version?.takeIf { it.isNotBlank() } ?: ""

                "$name $version [$vendor]"

            }

        }



    enum class SortMode {

        Default, PublishTime,

    }



    private data class DriverRepo(

        val name: String = "",

        val path: String = "",

        val sort: Int = 0,

        val useTagName: Boolean = false,

        val sortMode: SortMode = SortMode.Default

    )



    private val repoList: List<DriverRepo> = listOf(

        DriverRepo("STORM DRIVER", "ReiKatari/STORM_DRIVER", 0, true, SortMode.PublishTime),

        DriverRepo("Balemuni Turnip (Apex)", "Balemuni/Balemunis-Aurora", 1),

        DriverRepo("KIMCHI Turnip Drivers", "K11MCH1/AdrenoToolsDrivers", 2, true, SortMode.PublishTime),

        DriverRepo("Mr. Purple Turnip", "MrPurple666/purple-turnip", 3),

        DriverRepo("Weab-Chan Freedreno CI", "Weab-chan/freedreno_turnip-CI", 4),

        DriverRepo("Whitebelyash Turnip Hub", "whitebelyash/freedreno_turnip-CI", 5, false, SortMode.PublishTime),

        DriverRepo("GameHub Adreno 8xx", "crueter/GameHub-8Elite-Drivers", 6),

        DriverRepo("Amaral Adreno Tools", "rickamaral94/Amaral-Adreno-Tools", 7, true, SortMode.PublishTime),

        DriverRepo("WinNative Drivers", "WinNative-Emu/Drivers", 8, true, SortMode.PublishTime),

        DriverRepo("AK-Turnip (Mesa Unified)", "aydar-kamaltdinov/mesa-unified", 9, true, SortMode.PublishTime),

        DriverRepo("StevenMX Turnip CI", "StevenMXZ/freedreno_turnip-CI", 10, true, SortMode.PublishTime),

        DriverRepo("The412Banner Nightly Turnip", "The412Banner/Banners-Turnip", 11, true, SortMode.PublishTime),

        DriverRepo("Vauzi-17 Adreno 710 / 720 / 722 Turnip", "Vauzi-17/710", 12, true, SortMode.PublishTime),

    )



    private val driverMap = listOf(

        IntRange(Integer.MIN_VALUE, 9) to "Unsupported",

        IntRange(10, 99) to "STORM DRIVER 7.0.0 / KIMCHI Turnip", // Special case for Adreno Axx

        IntRange(100, 599) to "Unsupported",

        IntRange(600, 639) to "STORM DRIVER 7.0.0 / Mr. Purple EOL / AK-Turnip",

        IntRange(640, 699) to "STORM DRIVER 7.0.0 / KIMCHI Turnip / AK-Turnip",

        IntRange(710, 725) to "STORM DRIVER 7.0.0 / Vauzi-17 (A710/A720) / AK-Turnip",

        IntRange(726, 750) to "STORM DRIVER 7.0.0 / Amaral 26.2.0 / Balemuni Apex",

        IntRange(751, 799) to "STORM DRIVER 7.0.0 / Amaral Turnip / AK-Turnip",

        IntRange(800, 829) to "STORM DRIVER 7.0.0 / Amaral A8xx / AK-Turnip",

        IntRange(830, 840) to "STORM DRIVER 7.0.0 / Amaral 26.2.0 (8Elite) / AK-Turnip",

        IntRange(841, 899) to "STORM DRIVER 7.0.0 / Amaral A8xx / Whitebelyash",

        IntRange(900, Int.MAX_VALUE) to "STORM DRIVER 7.0.0 / Mesa Unified"

    )



    private lateinit var driverGroupAdapter: DriverGroupAdapter

    private val driverViewModel: DriverViewModel by activityViewModels()

    private val homeViewModel: HomeViewModel by activityViewModels()



    private fun parseAdrenoModel(): Int {

        if (gpuModel == null) {

            return 0

        }



        val modelList = gpuModel!!.split(" ")



        // format: Adreno (TM) <ModelNumber>

        if (modelList.size < 3 || modelList[0] != "Adreno") {

            return 0

        }



        val model = modelList[2]



        try {

            // special case for Axx GPUs (e.g. AYANEO Pocket S2)

            // driverMap has specific ranges for this

            if (model.startsWith("A")) {

                return model.substring(1).toInt()

            }



            return model.toInt()

        } catch (e: Exception) {

            // Model parse error, just say unsupported

            e.printStackTrace()

            return 0

        }

    }



    override fun onCreate(savedInstanceState: Bundle?) {

        super.onCreate(savedInstanceState)

        enterTransition = MaterialSharedAxis(MaterialSharedAxis.X, true)

        returnTransition = MaterialSharedAxis(MaterialSharedAxis.X, false)

        reenterTransition = MaterialSharedAxis(MaterialSharedAxis.X, false)

    }



    override fun onCreateView(

        inflater: LayoutInflater,

        container: ViewGroup?,

        savedInstanceState: Bundle?

    ): View {

        _binding = FragmentDriverFetcherBinding.inflate(inflater)

        binding.badgeRecommendedDriver.text = recommendedDriver

        binding.badgeGpuModel.text = gpuModel ?: "Qualcomm Adreno"

        binding.badgeActiveDriver.text = activeDriverInfo

        if (isMaliOrNonAdrenoGpu) {

            binding.textMaliDriverInfo.visibility = View.VISIBLE

        }



        return binding.root

    }



    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {

        super.onViewCreated(view, savedInstanceState)

        homeViewModel.setStatusBarShadeVisibility(visible = false)

        binding.toolbarDrivers.setNavigationOnClickListener {

            requireActivity().onBackPressedDispatcher.onBackPressed()

        }



        binding.listDrivers.layoutManager = LinearLayoutManager(context)

        driverGroupAdapter = DriverGroupAdapter(requireActivity(), driverViewModel)

        binding.listDrivers.adapter = driverGroupAdapter



        setInsets()



        viewLifecycleOwner.lifecycleScope.launch {

            viewLifecycleOwner.repeatOnLifecycle(androidx.lifecycle.Lifecycle.State.STARTED) {

                driverViewModel.driverList.collect {

                    binding.badgeActiveDriver.text = activeDriverInfo

                }

            }

        }



        fetchDrivers()

    }



    override fun onResume() {

        super.onResume()

        binding.badgeActiveDriver.text = activeDriverInfo

    }



    private val client = OkHttpClient.Builder()

        .dns(SmartDns)

        .connectTimeout(15, java.util.concurrent.TimeUnit.SECONDS)

        .readTimeout(20, java.util.concurrent.TimeUnit.SECONDS)

        .followRedirects(true)

        .followSslRedirects(true)

        .retryOnConnectionFailure(true)

        .build()



    private fun getCuratedFallbackReleases(path: String, name: String): ArrayList<Release> {

        val list = ArrayList<Release>()

        val now = LocalDateTime.now()

        when {

            path.contains("STORM_DRIVER", ignoreCase = true) -> {

                list.add(

                    Release(

                        tagName = "v7.0.0",

                        titleName = "STORM DRIVER 7.0.0 (Unified Mesa Turnip Adreno 6xx/7xx/8xx)",

                        title = "STORM DRIVER 7.0.0 (Unified Mesa Turnip Adreno 6xx/7xx/8xx)",

                        body = "STORM DRIVER 7.0.0 Unified Driver for Snapdragon Adreno 6xx, 7xx, 8xx (Qualcomm 8 Gen 1/2/3/Elite) with 4PDA curated reference packs.",

                        artifacts = listOf(

                            Artifact(URL("https://github.com/ReiKatari/STORM_DRIVER/releases/download/v7.0.0/STORM_DRIVER_7.0.0_Mesa_Turnip_A6xx_A7xx_A8xx.zip"), "STORM_DRIVER_7.0.0_Mesa_Turnip_A6xx_A7xx_A8xx.zip"),

                            Artifact(URL("https://stormgamesworld.ru/drivers/STORM_DRIVER_7.0.0_Unified.zip"), "STORM_DRIVER_7.0.0_Unified_Direct.zip"),

                            Artifact(URL("https://stormgamesworld.ru/drivers/4PDA_Turnip_Qualcomm_Reference_Pack.zip"), "4PDA_Turnip_Qualcomm_Reference_Pack.zip")

                        ),

                        prerelease = false,

                        latest = true,

                        publishTime = now

                    )

                )

            }

            path.contains("AdrenoToolsDrivers", ignoreCase = true) || path.contains("K11MCH1", ignoreCase = true) -> {

                list.add(

                    Release(

                        tagName = "v24.3.0",

                        titleName = "KIMCHI Turnip Drivers v24.3.0",

                        title = "KIMCHI Turnip Drivers v24.3.0",

                        body = "Turnip drivers by Kimchi. Compatible with Adreno 6xx and 7xx.",

                        artifacts = listOf(

                            Artifact(URL("https://github.com/K11MCH1/AdrenoToolsDrivers/releases/download/v24.3.0/turnip-24.3.0.zip"), "turnip-24.3.0.zip"),

                            Artifact(URL("https://stormgamesworld.ru/drivers/KIMCHI_Turnip_v24.3.0.zip"), "KIMCHI_Turnip_v24.3.0.zip")

                        ),

                        prerelease = false,

                        latest = true,

                        publishTime = now

                    )

                )

            }

            path.contains("Amaral", ignoreCase = true) -> {

                list.add(

                    Release(

                        tagName = "v26.2.0",

                        titleName = "Amaral Turnip 26.2.0 (A7xx/A8xx)",

                        title = "Amaral Turnip 26.2.0 (A7xx/A8xx)",

                        body = "Turnip drivers compiled by Rick Amaral for Snapdragon 8 Gen 2 / Gen 3 / 8 Elite.",

                        artifacts = listOf(

                            Artifact(URL("https://github.com/rickamaral94/Amaral-Adreno-Tools/releases/download/v26.2.0/turnip-26.2.0-A7xx-A8xx.zip"), "turnip-26.2.0-A7xx-A8xx.zip"),

                            Artifact(URL("https://stormgamesworld.ru/drivers/Amaral_Turnip_26.2.0.zip"), "Amaral_Turnip_26.2.0.zip")

                        ),

                        prerelease = false,

                        latest = true,

                        publishTime = now

                    )

                )

            }

            path.contains("Balemuni", ignoreCase = true) -> {

                list.add(

                    Release(

                        tagName = "v24.2.0",

                        titleName = "Balemuni Turnip (Apex)",

                        title = "Balemuni Turnip (Apex)",

                        body = "Balemuni Apex Turnip release for Snapdragon Adreno 7xx.",

                        artifacts = listOf(

                            Artifact(URL("https://github.com/Balemuni/Balemunis-Aurora/releases/download/v24.2.0/turnip-v24.2.0-apex.zip"), "turnip-v24.2.0-apex.zip"),

                            Artifact(URL("https://stormgamesworld.ru/drivers/Balemuni_Turnip_Apex.zip"), "Balemuni_Turnip_Apex.zip")

                        ),

                        prerelease = false,

                        latest = true,

                        publishTime = now

                    )

                )

            }

            else -> {

                list.add(

                    Release(

                        tagName = "latest",

                        titleName = "$name (Curated Mirror)",

                        title = "$name (Curated Mirror)",

                        body = "Direct curated driver mirror for $name from 4PDA / stormgamesworld.ru.",

                        artifacts = listOf(

                            Artifact(URL("https://stormgamesworld.ru/drivers/4PDA_Turnip_Qualcomm_Reference_Pack.zip"), "4PDA_Turnip_Qualcomm_Reference_Pack.zip")

                        ),

                        prerelease = false,

                        latest = true,

                        publishTime = now

                    )

                )

            }

        }

        return list

    }



    private fun fetchDrivers() {

        binding.loadingIndicator.isVisible = true



        val driverGroups = arrayListOf<DriverGroup>()

        val appCacheDir = context?.cacheDir



        repoList.forEach { driver ->

            val name = driver.name

            val path = driver.path

            val useTagName = driver.useTagName

            val sortMode = driver.sortMode

            val sort = driver.sort



            viewLifecycleOwner.lifecycleScope.launch {

                withContext(Dispatchers.IO) {

                    var releases = ArrayList<Release>()

                    val cacheFile = if (appCacheDir != null) {

                        java.io.File(appCacheDir, "driver_repo_${path.replace('/', '_')}.json")

                    } else null



                    // 1. Check 30-minute disk cache (Law 5 / Rate limit protection)

                    if (cacheFile != null && cacheFile.exists() && (System.currentTimeMillis() - cacheFile.lastModified() < 30 * 60 * 1000L)) {

                        try {

                            val cached = cacheFile.readText()

                            if (cached.isNotBlank()) {

                                val parsed = Release.fromJsonArray(cached, useTagName, sortMode)

                                if (parsed.isNotEmpty()) {

                                    releases = parsed

                                }

                            }

                        } catch (_: Exception) {}

                    }



                    // 2. If cache miss, fetch from network/mirrors

                    if (releases.isEmpty()) {

                        val urlsToTry = listOf(

                            "https://api.github.com/repos/$path/releases",

                            "https://gh.con.sh/https://api.github.com/repos/$path/releases",

                            "https://gh.llkk.cc/https://api.github.com/repos/$path/releases",

                            "https://ghfast.top/https://api.github.com/repos/$path/releases"

                        )



                        for (url in urlsToTry) {

                            try {

                                val request = Request.Builder()

                                    .url(url)

                                    .header("User-Agent", "Mozilla/5.0 (Linux; Android 14; Mobile) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/128.0.0.0 StormSwitch/4.8.8")

                                    .header("Accept", "application/vnd.github.v3+json, application/json, text/plain, */*")

                                    .build()



                                client.newCall(request).execute().use { response ->

                                    if (response.isSuccessful) {

                                        val body = response.body?.string()

                                        if (!body.isNullOrBlank()) {

                                            val parsed = Release.fromJsonArray(body, useTagName, sortMode)

                                            if (parsed.isNotEmpty()) {

                                                releases = parsed

                                                // Save to 30-min disk cache

                                                try {

                                                    cacheFile?.writeText(body)

                                                } catch (_: Exception) {}

                                                return@use

                                            }

                                        }

                                    }

                                }

                            } catch (_: Exception) {

                                // Continue to next mirror

                            }

                            if (releases.isNotEmpty()) break

                        }

                    }



                    // 3. Fallback to stale cache if network failed/rate-limited

                    if (releases.isEmpty() && cacheFile != null && cacheFile.exists()) {

                        try {

                            val stale = cacheFile.readText()

                            if (stale.isNotBlank()) {

                                val parsed = Release.fromJsonArray(stale, useTagName, sortMode)

                                if (parsed.isNotEmpty()) {

                                    releases = parsed

                                }

                            }

                        } catch (_: Exception) {}

                    }



                    // 4. Built-in curated fallback catalog (STORM DRIVER 7.0.0 / 4PDA / Mesa Turnip)

                    if (releases.isEmpty()) {

                        releases = getCuratedFallbackReleases(path, name)

                    }



                    val group = DriverGroup(

                        name,

                        releases,

                        sort

                    )



                    synchronized(driverGroups) {

                        driverGroups.add(group)

                        driverGroups.sortBy {

                            it.sort

                        }

                    }



                    withContext(Dispatchers.Main) {

                        if (_binding != null) {

                            driverGroupAdapter.updateDriverGroups(driverGroups)



                            if (driverGroups.size >= repoList.size) {

                                binding.loadingIndicator.isVisible = false

                            }

                        }

                    }

                }

            }

        }

    }



    private fun setInsets() = ViewCompat.setOnApplyWindowInsetsListener(

        binding.root

    ) { _: View, windowInsets: WindowInsetsCompat ->

        val barInsets = windowInsets.getInsets(WindowInsetsCompat.Type.systemBars())

        val cutoutInsets = windowInsets.getInsets(WindowInsetsCompat.Type.displayCutout())



        val topInsets = maxOf(barInsets.top, cutoutInsets.top)

        val leftInsets = barInsets.left + cutoutInsets.left

        val rightInsets = barInsets.right + cutoutInsets.right



        binding.appbarDrivers.updatePadding(top = topInsets)

        binding.toolbarDrivers.updateMargins(left = leftInsets, right = rightInsets)

        val density = resources.displayMetrics.density

        binding.cardDriverInfo.updateMargins(

            left = leftInsets + (12 * density).toInt(),

            right = rightInsets + (12 * density).toInt()

        )

        binding.listDrivers.updateMargins(left = leftInsets, right = rightInsets)



        binding.listDrivers.updatePadding(

            bottom = barInsets.bottom + resources.getDimensionPixelSize(

                R.dimen.spacing_bottom_list_fab

            )

        )



        windowInsets

    }



    data class Artifact(val url: URL, val name: String)



    data class Release(

        var tagName: String = "",

        var titleName: String = "",

        var title: String = "",

        var body: String = "",

        var artifacts: List<Artifact> = ArrayList(),

        var prerelease: Boolean = false,

        var latest: Boolean = false,

        var publishTime: LocalDateTime = LocalDateTime.now()

    ) {

        companion object {

            fun fromJsonArray(

                jsonString: String,

                useTagName: Boolean,

                sortMode: SortMode

            ): ArrayList<Release> {

                val mapper = jacksonObjectMapper()



                try {

                    val rootNode = mapper.readTree(jsonString)



                    val releases = ArrayList<Release>()



                    var latestRelease: Release? = null



                    if (rootNode.isArray) {

                        rootNode.forEach { node ->

                            val release = fromJson(node, useTagName)



                            if (latestRelease == null && !release.prerelease) {

                                latestRelease = release

                                release.latest = true

                            }



                            releases.add(release)

                        }

                    }



                    when (sortMode) {

                        SortMode.PublishTime -> releases.sortByDescending {

                            it.publishTime

                        }



                        else -> {}

                    }



                    return releases

                } catch (e: Exception) {

                    e.printStackTrace()

                    return ArrayList()

                }

            }



            private fun fromJson(node: JsonNode, useTagName: Boolean): Release {

                try {

                    val tagName = node.get("tag_name").toString().removeSurrounding("\"")

                    val body = node.get("body").toString().removeSurrounding("\"")

                    val prerelease = node.get("prerelease").toString().toBoolean()

                    val titleName = node.get("name").toString().removeSurrounding("\"")



                    val published = node.get("published_at").toString().removeSurrounding("\"")

                    val instantTime: Instant? = Instant.parse(published)

                    val localTime = instantTime?.atZone(ZoneId.systemDefault())?.toLocalDateTime() ?: LocalDateTime.now()



                    val title = if (useTagName) tagName else titleName



                    val assets = node.get("assets")

                    val artifacts = ArrayList<Artifact>()

                    if (assets?.isArray == true) {

                        assets.forEach { subNode ->

                            val urlStr = subNode.get("browser_download_url").toString()

                                .removeSurrounding("\"")



                            val url = URL(urlStr)

                            val name = subNode.get("name").toString().removeSurrounding("\"")



                            val artifact = Artifact(url, name)

                            artifacts.add(artifact)

                        }

                    }



                    return Release(

                        tagName,

                        titleName,

                        title,

                        body,

                        artifacts,

                        prerelease,

                        false,

                        localTime

                    )

                } catch (e: Exception) {

                    // TODO: handle malformed input.

                    e.printStackTrace()

                }



                return Release()

            }

        }

    }



    data class DriverGroup(

        val name: String,

        val releases: ArrayList<Release>,

        val sort: Int

    )

}

// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.annotation.SuppressLint
import android.app.Dialog
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.WindowManager
import android.widget.EditText
import android.widget.Toast
import androidx.core.widget.doOnTextChanged
import androidx.fragment.app.DialogFragment
import androidx.lifecycle.lifecycleScope
import androidx.preference.PreferenceManager
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import org.json.JSONObject
import org.yuzu.yuzu_emu.NativeLibrary
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.YuzuApplication
import org.yuzu.yuzu_emu.databinding.DialogCheatsBinding
import org.yuzu.yuzu_emu.databinding.ItemCheatBinding
import org.yuzu.yuzu_emu.model.Game
import org.yuzu.yuzu_emu.utils.DirectoryInitialization
import org.yuzu.yuzu_emu.utils.ThemeHelper
import java.io.BufferedReader
import java.io.File
import java.io.InputStreamReader
import java.net.HttpURLConnection
import java.net.URL

class CheatsDialogFragment : DialogFragment() {

    private var _binding: DialogCheatsBinding? = null
    private val binding get() = _binding!!

    private lateinit var game: Game
    private val allCheats = mutableListOf<CheatModel>()
    private val displayedCheats = mutableListOf<CheatModel>()
    private lateinit var adapter: CheatsAdapter

    data class CheatModel(
        var name: String,
        var code: String,
        var buildId: String = "",
        var isEnabled: Boolean = false,
        var isCustom: Boolean = false,
        var sourceFile: File? = null
    )

    companion object {
        const val TAG = "CheatsDialogFragment"
        private const val ARG_GAME = "game"

        fun newInstance(game: Game): CheatsDialogFragment {
            val fragment = CheatsDialogFragment()
            val args = Bundle().apply {
                putParcelable(ARG_GAME, game)
            }
            fragment.arguments = args
            return fragment
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        game = requireArguments().getParcelable(ARG_GAME)!!
        setStyle(STYLE_NORMAL, ThemeHelper.getSelectedStaticThemeColor())
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val dialog = super.onCreateDialog(savedInstanceState)
        dialog.requestWindowFeature(android.view.Window.FEATURE_NO_TITLE)
        dialog.window?.let { window ->
            window.setBackgroundDrawable(android.graphics.drawable.ColorDrawable(android.graphics.Color.TRANSPARENT))
        }
        dialog.setOnKeyListener { _, keyCode, event ->
            if (keyCode == android.view.KeyEvent.KEYCODE_ESCAPE && event.action == android.view.KeyEvent.ACTION_UP) {
                dismissAllowingStateLoss()
                true
            } else {
                false
            }
        }
        return dialog
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = DialogCheatsBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        androidx.core.view.ViewCompat.setOnApplyWindowInsetsListener(binding.root) { v, insets ->
            val systemBars = insets.getInsets(
                androidx.core.view.WindowInsetsCompat.Type.systemBars() or
                androidx.core.view.WindowInsetsCompat.Type.displayCutout()
            )
            val density = resources.displayMetrics.density
            val topInset = maxOf(systemBars.top, (24 * density).toInt())
            v.setPadding(systemBars.left, topInset, systemBars.right, systemBars.bottom)
            insets
        }
        binding.root.requestApplyInsets()

        val isRunning = NativeLibrary.isRunning()
        val versionStr = game.version.ifEmpty { "" }

        binding.textGameInfo.text = buildString {
            append("ID: ")
            append(game.programIdHex)
            if (versionStr.isNotEmpty()) {
                append(getString(R.string.cheat_header_version_prefix))
                append(versionStr)
            }
            append(if (isRunning) getString(R.string.cheat_header_in_game) else getString(R.string.cheat_header_offline))
            append(getString(R.string.cheat_header_dmnt_active))
        }

        adapter = CheatsAdapter(
            displayedCheats,
            isRunning,
            onCheckChanged = { cheat, isChecked ->
                cheat.isEnabled = isChecked
                updateSummary()
                saveCheatsSilently()
            },
            onEditValueClicked = { cheat ->
                showEditCheatValueDialog(cheat)
            },
            onLongClicked = { cheat ->
                showCheatActionMenu(cheat)
            }
        )

        binding.recyclerCheats.layoutManager = LinearLayoutManager(requireContext())
        binding.recyclerCheats.adapter = adapter

        binding.btnClose.setOnClickListener { dismiss() }
        binding.btnSaveCheats.setOnClickListener {
            saveCheats()
            dismiss()
        }

        binding.btnDownloadOnline.setOnClickListener {
            downloadOnlineCheats()
        }

        binding.btnAddCustomCheat.setOnClickListener {
            showAddCustomCheatDialog()
        }

        binding.btnToggleAll.setOnClickListener {
            val anyDisabled = allCheats.any { !it.isEnabled }
            for (c in allCheats) {
                c.isEnabled = anyDisabled
            }
            adapter.notifyDataSetChanged()
            updateSummary()
        }

        binding.editSearchCheats.doOnTextChanged { text, _, _, _ ->
            val query = text?.toString()?.trim() ?: ""
            binding.btnClearSearch.visibility = if (query.isNotEmpty()) View.VISIBLE else View.GONE
            filterCheats(query)
        }

        binding.btnClearSearch.setOnClickListener {
            binding.editSearchCheats.text?.clear()
        }

        loadCheatsFromDisk()
    }

    override fun onStart() {
        super.onStart()
        dialog?.window?.apply {
            setLayout(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
            )
            setBackgroundDrawable(android.graphics.drawable.ColorDrawable(android.graphics.Color.TRANSPARENT))
        }
    }

    private fun getCheatsDir(): File {
        val base = File(DirectoryInitialization.userDirectory ?: YuzuApplication.appContext.filesDir.absolutePath, "cheats")
        val gameDir = File(base, game.programIdHex)
        if (!gameDir.exists()) gameDir.mkdirs()
        return gameDir
    }

    private fun loadCheatsFromDisk() {
        allCheats.clear()
        val cheatsDir = getCheatsDir()
        val prefs = PreferenceManager.getDefaultSharedPreferences(YuzuApplication.appContext)

        val enabledFile = File(cheatsDir, "enabled_cheats.txt")
        val enabledSet = if (enabledFile.exists()) {
            try {
                enabledFile.readLines(Charsets.UTF_8).map { it.trim() }.filter { it.isNotEmpty() }.toSet()
            } catch (_: Exception) {
                prefs.getStringSet("cheats_enabled_${game.programIdHex}", null)
            }
        } else {
            prefs.getStringSet("cheats_enabled_${game.programIdHex}", null)
        }

        val filesToRead = mutableListOf<File>()
        cheatsDir.listFiles()?.filter { it.isFile && it.name.endsWith(".txt", ignoreCase = true) && !it.name.equals("enabled_cheats.txt", ignoreCase = true) }?.let { list ->
            // Sort so specific Build ID files come before generic cheats.txt
            val sorted = list.sortedBy { f ->
                if (f.name.equals("cheats.txt", ignoreCase = true) || f.name.equals("custom.txt", ignoreCase = true)) 1 else 0
            }
            filesToRead.addAll(sorted)
        }

        val legacyFile = File(cheatsDir.parentFile, "${game.programIdHex}.txt")
        if (legacyFile.exists() && legacyFile.isFile) {
            filesToRead.add(legacyFile)
        }

        for (file in filesToRead) {
            try {
                var buildId = file.nameWithoutExtension.uppercase()
                if (buildId == "CHEATS" || buildId == game.programIdHex || buildId == "ALL" || buildId == getString(R.string.cheat_build_all_versions)) {
                    buildId = "ALL"
                }

                val lines = file.readLines(Charsets.UTF_8)
                var currentName = ""
                val currentCode = StringBuilder()

                for (rawLine in lines) {
                    val line = rawLine.trim()
                    if (line.isEmpty() || line.startsWith("#") || line.startsWith("//") || line.startsWith(";")) {
                        continue
                    }

                    if (line.startsWith("[") && line.contains("]")) {
                        if (currentName.isNotEmpty() && currentCode.isNotEmpty()) {
                            addCheatItem(currentName, currentCode.toString().trim(), buildId, enabledSet, file)
                        }
                        val closeIdx = line.indexOf(']')
                        currentName = line.substring(1, closeIdx).trim()
                        currentCode.clear()
                    } else {
                        currentCode.append(line).append("\n")
                    }
                }

                if (currentName.isNotEmpty() && currentCode.isNotEmpty()) {
                    addCheatItem(currentName, currentCode.toString().trim(), buildId, enabledSet, file)
                }
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }

        filterCheats(binding.editSearchCheats.text?.toString()?.trim() ?: "")
    }

    private fun addCheatItem(name: String, code: String, buildId: String, enabledSet: Set<String>?, sourceFile: File) {
        if (allCheats.any { it.name == name }) return
        val isEnabled = if (enabledSet != null) {
            enabledSet.contains(name)
        } else {
            false
        }
        allCheats.add(
            CheatModel(
                name = name,
                code = code,
                buildId = buildId,
                isEnabled = isEnabled,
                isCustom = sourceFile.name.contains("custom", ignoreCase = true),
                sourceFile = sourceFile
            )
        )
    }

    @SuppressLint("NotifyDataSetChanged")
    private fun filterCheats(query: String) {
        displayedCheats.clear()
        if (query.isEmpty()) {
            displayedCheats.addAll(allCheats)
        } else {
            displayedCheats.addAll(allCheats.filter { it.name.contains(query, ignoreCase = true) })
        }
        adapter.notifyDataSetChanged()
        binding.layoutEmptyCheats.visibility = if (displayedCheats.isEmpty()) View.VISIBLE else View.GONE
        binding.recyclerCheats.visibility = if (displayedCheats.isEmpty()) View.GONE else View.VISIBLE
        updateSummary()
    }

    private fun updateSummary() {
        val total = allCheats.size
        val enabled = allCheats.count { it.isEnabled }
        binding.textCheatsSummary.text = getString(R.string.cheats_count_summary, total, enabled)
        binding.btnToggleAll.text = if (allCheats.any { !it.isEnabled }) getString(R.string.select_all) else getString(R.string.deselect_all)
    }

    private fun saveCheatsSilently() {
        val prefs = PreferenceManager.getDefaultSharedPreferences(YuzuApplication.appContext)
        val enabledNames = allCheats.filter { it.isEnabled }.map { it.name }.toSet()
        prefs.edit().putStringSet("cheats_enabled_${game.programIdHex}", enabledNames).apply()

        val rootDir = DirectoryInitialization.userDirectory ?: YuzuApplication.appContext.filesDir.absolutePath
        val cheatsDir = getCheatsDir()
        val loadCheatsDir = File(rootDir, "load/${game.programIdHex}/cheats")
        val atmoCheatsDir = File(rootDir, "sdmc/atmosphere/contents/${game.programIdHex}/cheats")
        val atmoCheatsLowerDir = File(rootDir, "sdmc/atmosphere/contents/${game.programIdHex.lowercase()}/cheats")

        val targetDirs = listOf(cheatsDir, loadCheatsDir, atmoCheatsDir, atmoCheatsLowerDir)
        for (d in targetDirs) {
            if (!d.exists()) d.mkdirs()
        }

        // 1. Write enabled_cheats.txt for direct C++ PatchManager synchronization
        try {
            val enabledLines = mutableListOf<String>()
            for (name in enabledNames) {
                enabledLines.add(name)
                enabledLines.add("[$name]")
            }
            val textToWrite = enabledLines.joinToString("\n")
            for (d in targetDirs) {
                File(d, "enabled_cheats.txt").writeText(textToWrite, Charsets.UTF_8)
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }

        // 2. Also write full list into cheats.txt and per-buildId files
        try {
            val content = StringBuilder()
            val buildMap = mutableMapOf<String, StringBuilder>()

            for (cheat in allCheats) {
                content.append("[${cheat.name}]\n")
                content.append(cheat.code.trim()).append("\n\n")

                if (cheat.buildId.isNotBlank() && cheat.buildId != "ALL" && cheat.buildId != "CHEATS" && cheat.buildId != game.programIdHex && cheat.buildId != getString(R.string.cheat_build_all_versions)) {
                    val bsb = buildMap.getOrPut(cheat.buildId) { StringBuilder() }
                    bsb.append("[${cheat.name}]\n").append(cheat.code.trim()).append("\n\n")
                }
            }

            for (d in targetDirs) {
                File(d, "cheats.txt").writeText(content.toString(), Charsets.UTF_8)
                for ((bId, bContent) in buildMap) {
                    if (bId.length == 16 && bId.all { it.isDigit() || (it in 'A'..'F') || (it in 'a'..'f') }) {
                        File(d, "$bId.txt").writeText(bContent.toString(), Charsets.UTF_8)
                        File(d, "${bId.lowercase()}.txt").writeText(bContent.toString(), Charsets.UTF_8)
                    }
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }

        // 3. Reload in-game memory cheats in real-time if emulation is running
        if (NativeLibrary.isRunning()) {
            try {
                NativeLibrary.reloadCheats()
                NativeLibrary.reloadProfiles()
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }
    }

    private fun saveCheats() {
        saveCheatsSilently()
        if (NativeLibrary.isRunning()) {
            Toast.makeText(requireContext(), getString(R.string.cheat_toast_activated_ingame), Toast.LENGTH_SHORT).show()
        } else {
            Toast.makeText(requireContext(), getString(R.string.cheat_toast_saved), Toast.LENGTH_SHORT).show()
        }
    }

        private fun downloadOnlineCheats() {
        binding.layoutDownloadStatus.visibility = View.VISIBLE
        binding.btnDownloadOnline.isEnabled = false

        lifecycleScope.launch(Dispatchers.IO) {
            val tidUpper = game.programIdHex.uppercase()
            val userAgent = "STORM-SWITCH/16.0.6"
            val timeoutMs = 10000

            // Helper for HTTP GET with timeout and User-Agent
            fun fetchUrl(urlStr: String): String? {
                return try {
                    val url = URL(urlStr)
                    val conn = url.openConnection() as HttpURLConnection
                    conn.requestMethod = "GET"
                    conn.connectTimeout = timeoutMs
                    conn.readTimeout = timeoutMs
                    conn.setRequestProperty("User-Agent", userAgent)
                    if (conn.responseCode == 200) {
                        conn.inputStream.bufferedReader(Charsets.UTF_8).use { it.readText().trim() }
                    } else null
                } catch (_: Exception) {
                    null
                }
            }

            // Discover known local BuildIDs
            val knownBuildIds = mutableSetOf<String>()
            val cheatsDir = getCheatsDir()
            if (cheatsDir.exists()) {
                cheatsDir.listFiles()?.forEach { f ->
                    val name = f.nameWithoutExtension.uppercase()
                    if (name.length == 16 && name.all { it.isDigit() || (it in 'A'..'F') }) {
                        knownBuildIds.add(name)
                    }
                }
            }
            allCheats.forEach { c ->
                val b = c.buildId.uppercase()
                if (b.length == 16 && b.all { it.isDigit() || (it in 'A'..'F') }) {
                    knownBuildIds.add(b)
                }
            }

            // Storage for deduplicated cheats: normalized title -> (name, code, buildId)
            data class OnlineCheat(val name: String, val code: String, var buildId: String)
            val deduplicatedCheats = mutableMapOf<String, OnlineCheat>()

            fun normalizeTitle(title: String): String {
                return title.replace("[", "").replace("]", "").trim().lowercase()
            }

            fun addCheat(name: String, code: String, bid: String) {
                val cleanName = name.trim()
                val cleanCode = code.trim()
                if (cleanName.isEmpty() || cleanCode.isEmpty()) return
                val norm = normalizeTitle(cleanName)
                val targetBid = if (bid.length == 16 && bid.all { it.isDigit() || (it in 'A'..'F') }) bid else "ALL"
                if (!deduplicatedCheats.containsKey(norm)) {
                    deduplicatedCheats[norm] = OnlineCheat(cleanName, cleanCode, targetBid)
                }
            }

            fun parseRawCheatText(text: String, defaultBid: String) {
                var currentName = ""
                val currentCode = StringBuilder()
                for (line in text.lines()) {
                    val trimmed = line.trim()
                    if (trimmed.isEmpty() || trimmed.startsWith("#") || trimmed.startsWith("//") || trimmed.startsWith(";")) {
                        continue
                    }
                    if (trimmed.startsWith("[") && trimmed.contains("]")) {
                        if (currentName.isNotEmpty() && currentCode.isNotEmpty()) {
                            addCheat(currentName, currentCode.toString().trim(), defaultBid)
                        }
                        val closeIdx = trimmed.indexOf(']')
                        currentName = trimmed.substring(1, closeIdx).trim()
                        currentCode.clear()
                    } else {
                        currentCode.append(trimmed).append("\n")
                    }
                }
                if (currentName.isNotEmpty() && currentCode.isNotEmpty()) {
                    addCheat(currentName, currentCode.toString().trim(), defaultBid)
                }
            }

            // 1. HamletDuFromage/switch-cheats-db JSON
            try {
                val jsonUrl = "https://cdn.jsdelivr.net/gh/HamletDuFromage/switch-cheats-db@master/cheats/${tidUpper}.json"
                val jsonText = fetchUrl(jsonUrl)
                if (!jsonText.isNullOrEmpty() && jsonText.startsWith("{")) {
                    val jsonObj = JSONObject(jsonText)
                    val keys = jsonObj.keys()
                    val candidateBids = mutableListOf<String>()
                    while (keys.hasNext()) {
                        val k = keys.next().trim().uppercase()
                        if (k != "ATTRIBUTION" && k.length == 16 && k.all { it.isDigit() || (it in 'A'..'F') }) {
                            candidateBids.add(k)
                        }
                    }
                    for (bidKey in candidateBids) {
                        knownBuildIds.add(bidKey)
                        val bidObj = jsonObj.optJSONObject(bidKey)
                        if (bidObj != null) {
                            val cheatNames = bidObj.keys()
                            while (cheatNames.hasNext()) {
                                val cName = cheatNames.next()
                                val cCode = bidObj.optString(cName, "").trim()
                                if (cCode.isNotEmpty()) {
                                    val formattedName = if (cName.startsWith("[") && cName.endsWith("]")) {
                                        cName.substring(1, cName.length - 1).trim()
                                    } else {
                                        cName
                                    }
                                    addCheat(formattedName, cCode, bidKey)
                                }
                            }
                        }
                    }
                }
            } catch (e: Exception) {
                e.printStackTrace()
            }

            // 2. Tinfoil Media API
            try {
                val tinfoilUrl = "https://tinfoil.media/api/cheats/${tidUpper}"
                val tinfoilText = fetchUrl(tinfoilUrl)
                if (!tinfoilText.isNullOrEmpty() && tinfoilText.startsWith("[")) {
                    val jsonArr = org.json.JSONArray(tinfoilText)
                    for (i in 0 until jsonArr.length()) {
                        val item = jsonArr.optJSONObject(i) ?: continue
                        var cName = item.optString("name", "").trim()
                        val cBid = item.optString("build_id", "").trim().uppercase()
                        val cSrc = item.optString("source", "").trim()
                        if (cName.isEmpty() || cSrc.isEmpty()) continue
                        if (cName.startsWith("[") && cName.endsWith("]")) {
                            cName = cName.substring(1, cName.length - 1).trim()
                        }
                        if (cBid.length == 16 && cBid.all { it.isDigit() || (it in 'A'..'F') }) {
                            knownBuildIds.add(cBid)
                        }
                        addCheat(cName, cSrc, cBid)
                    }
                }
            } catch (e: Exception) {
                e.printStackTrace()
            }

            // 5. GitHub contents API fallback to discover all BuildIDs if knownBuildIds is empty or partial
            try {
                val contentsApis = listOf(
                    "https://api.github.com/repos/ibnux/switch-cheat/contents/atmosphere/titles/${tidUpper}/cheats",
                    "https://api.github.com/repos/ChanseyIsTheBest/NX-60FPS-RES-GFX-Cheats/contents/titles/${tidUpper}/cheats"
                )
                for (api in contentsApis) {
                    val contentsText = fetchUrl(api)
                    if (!contentsText.isNullOrEmpty() && contentsText.startsWith("[")) {
                        val arr = org.json.JSONArray(contentsText)
                        for (i in 0 until arr.length()) {
                            val fObj = arr.optJSONObject(i) ?: continue
                            val fName = fObj.optString("name", "").trim()
                            if (fName.endsWith(".txt", ignoreCase = true)) {
                                val bidName = fName.substringBeforeLast(".").uppercase()
                                if (bidName.length == 16 && bidName.all { it.isDigit() || (it in 'A'..'F') }) {
                                    knownBuildIds.add(bidName)
                                }
                            }
                        }
                    }
                }
            } catch (e: Exception) {
                e.printStackTrace()
            }

            // 3. ChanseyIsTheBest/NX-60FPS-RES-GFX-Cheats (60FPS, Resolution, Graphics cheats)
            for (bid in knownBuildIds) {
                try {
                    val chanseyUrl = "https://cdn.jsdelivr.net/gh/ChanseyIsTheBest/NX-60FPS-RES-GFX-Cheats@main/titles/${tidUpper}/cheats/${bid}.txt"
                    val chanseyText = fetchUrl(chanseyUrl)
                    if (!chanseyText.isNullOrEmpty() && chanseyText.contains("[")) {
                        parseRawCheatText(chanseyText, bid)
                    }
                } catch (e: Exception) {
                    e.printStackTrace()
                }
            }

            // 4. ibnux/switch-cheat
            for (bid in knownBuildIds) {
                try {
                    var ibnuxText = fetchUrl("https://cdn.jsdelivr.net/gh/ibnux/switch-cheat@master/atmosphere/titles/${tidUpper}/cheats/${bid.lowercase()}.txt")
                    if (ibnuxText.isNullOrEmpty()) {
                        ibnuxText = fetchUrl("https://cdn.jsdelivr.net/gh/ibnux/switch-cheat@master/atmosphere/titles/${tidUpper}/cheats/${bid.uppercase()}.txt")
                    }
                    if (!ibnuxText.isNullOrEmpty() && ibnuxText.contains("[")) {
                        parseRawCheatText(ibnuxText, bid)
                    }
                } catch (e: Exception) {
                    e.printStackTrace()
                }
            }

            // Write all discovered deduplicated cheats to disk
            val success = deduplicatedCheats.isNotEmpty()
            if (success) {
                val rootDir = DirectoryInitialization.userDirectory ?: YuzuApplication.appContext.filesDir.absolutePath
                val dir = getCheatsDir()
                val loadCheatsDir = File(rootDir, "load/${tidUpper}/cheats")
                val atmoCheatsDir = File(rootDir, "sdmc/atmosphere/contents/${tidUpper}/cheats")
                val atmoCheatsLowerDir = File(rootDir, "sdmc/atmosphere/contents/${tidUpper.lowercase()}/cheats")
                val allDirs = listOf(dir, loadCheatsDir, atmoCheatsDir, atmoCheatsLowerDir)
                for (d in allDirs) {
                    if (!d.exists()) d.mkdirs()
                }

                val allCheatsContent = StringBuilder()
                val buildMap = mutableMapOf<String, StringBuilder>()

                for ((_, cheat) in deduplicatedCheats) {
                    val entry = "[${cheat.name}]\n${cheat.code.trim()}\n\n"
                    allCheatsContent.append(entry)
                    if (cheat.buildId.length == 16 && cheat.buildId.all { it.isDigit() || (it in 'A'..'F') }) {
                        buildMap.getOrPut(cheat.buildId) { StringBuilder() }.append(entry)
                    }
                }

                for (d in allDirs) {
                    File(d, "cheats.txt").writeText(allCheatsContent.toString(), Charsets.UTF_8)
                    for ((bId, bContent) in buildMap) {
                        File(d, "$bId.txt").writeText(bContent.toString(), Charsets.UTF_8)
                        File(d, "${bId.lowercase()}.txt").writeText(bContent.toString(), Charsets.UTF_8)
                    }
                }
            }

            withContext(Dispatchers.Main) {
                if (_binding == null) return@withContext
                binding.layoutDownloadStatus.visibility = View.GONE
                binding.btnDownloadOnline.isEnabled = true
                if (success) {
                    loadCheatsFromDisk()
                    Toast.makeText(
                        requireContext(),
                        getString(R.string.cheats_download_success, deduplicatedCheats.size),
                        Toast.LENGTH_LONG
                    ).show()
                } else {
                    Toast.makeText(
                        requireContext(),
                        getString(R.string.cheats_not_found),
                        Toast.LENGTH_LONG
                    ).show()
                }
            }
        }
    }

private fun showAddCustomCheatDialog() {
        val context = requireContext()
        val layout = android.widget.LinearLayout(context).apply {
            orientation = android.widget.LinearLayout.VERTICAL
            setPadding(40, 20, 40, 10)
        }

        val nameEdit = EditText(context).apply {
            hint = getString(R.string.cheat_add_name_hint)
            textSize = 14f
        }
        val codeEdit = EditText(context).apply {
            hint = getString(R.string.cheat_add_code_hint)
            textSize = 12f
            minLines = 4
            typeface = android.graphics.Typeface.MONOSPACE
        }

        layout.addView(nameEdit)
        layout.addView(codeEdit)

        MaterialAlertDialogBuilder(context, R.style.EdenMaterialDialog)
            .setTitle(R.string.add_cheat)
            .setView(layout)
            .setPositiveButton(R.string.ok) { _, _ ->
                val name = nameEdit.text.toString().trim()
                val code = codeEdit.text.toString().trim()
                if (name.isNotEmpty() && code.isNotEmpty()) {
                    val cheatsDir = getCheatsDir()
                    val customFile = File(cheatsDir, "custom.txt")
                    try {
                        val currentText = if (customFile.exists()) customFile.readText(Charsets.UTF_8) else ""
                        customFile.writeText("$currentText\n\n[$name]\n$code\n", Charsets.UTF_8)
                        loadCheatsFromDisk()
                        Toast.makeText(context, getString(R.string.cheat_toast_added), Toast.LENGTH_SHORT).show()
                    } catch (e: Exception) {
                        e.printStackTrace()
                    }
                }
            }
            .setNegativeButton(android.R.string.cancel, null)
            .show()
    }

    private fun showEditCheatValueDialog(item: CheatModel) {
        val context = requireContext()
        val layout = android.widget.LinearLayout(context).apply {
            orientation = android.widget.LinearLayout.VERTICAL
            setPadding(48, 24, 48, 12)
        }

        val lines = item.code.lines().filter { it.isNotBlank() }
        var targetLineIdx = -1
        var existingHex = ""

        for (i in lines.indices) {
            val tokens = lines[i].trim().split("\\s+".toRegex())
            if (tokens.size >= 3 && (tokens[0].startsWith("04") || tokens[0].startsWith("08") || tokens[0].startsWith("02") || tokens[0].startsWith("01") || tokens[0].startsWith("58") || tokens[0].startsWith("78"))) {
                targetLineIdx = i
                existingHex = tokens.last()
                break
            } else if (tokens.size == 2) {
                targetLineIdx = i
                existingHex = tokens.last()
                break
            }
        }

        val existingDec = try {
            existingHex.toLong(16)
        } catch (_: Exception) {
            0L
        }

        val titleTv = com.google.android.material.textview.MaterialTextView(context).apply {
            text = getString(R.string.cheat_current_val_fmt, existingDec, existingHex)
            textSize = 13f
            setTextColor(ThemeHelper.getColor(context, androidx.appcompat.R.attr.colorPrimary))
            setPadding(0, 0, 0, 16)
        }
        layout.addView(titleTv)

        val inputLayout = com.google.android.material.textfield.TextInputLayout(context).apply {
            hint = getString(R.string.cheat_new_val_hint)
            boxBackgroundMode = com.google.android.material.textfield.TextInputLayout.BOX_BACKGROUND_OUTLINE
        }
        val inputEdit = com.google.android.material.textfield.TextInputEditText(context).apply {
            inputType = android.text.InputType.TYPE_CLASS_NUMBER
            setText(if (existingDec > 0) existingDec.toString() else "999999")
            textSize = 15f
        }
        inputLayout.addView(inputEdit)
        layout.addView(inputLayout)

        val presetsLayout = android.widget.LinearLayout(context).apply {
            orientation = android.widget.LinearLayout.HORIZONTAL
            setPadding(0, 16, 0, 0)
        }
        val presets = listOf("1000", "99999", "999999", "99999999")
        for (p in presets) {
            val btn = com.google.android.material.button.MaterialButton(context, null, com.google.android.material.R.attr.materialButtonOutlinedStyle).apply {
                text = p
                textSize = 10f
                setPadding(12, 0, 12, 0)
                layoutParams = android.widget.LinearLayout.LayoutParams(0, android.widget.LinearLayout.LayoutParams.WRAP_CONTENT, 1f).apply {
                    setMargins(4, 0, 4, 0)
                }
                setOnClickListener {
                    inputEdit.setText(p)
                }
            }
            presetsLayout.addView(btn)
        }
        layout.addView(presetsLayout)

        MaterialAlertDialogBuilder(context, R.style.EdenMaterialDialog)
            .setTitle(getString(R.string.cheat_edit_val_title_fmt, item.name))
            .setView(layout)
            .setPositiveButton(R.string.apply_driver_now) { _, _ ->
                val entered = inputEdit.text.toString().trim().toLongOrNull() ?: 0L
                val hexLen = if (existingHex.isNotEmpty()) existingHex.length else 8
                val newHex = java.lang.Long.toHexString(entered).uppercase().padStart(hexLen, '0')

                val updatedLines = lines.toMutableList()
                if (targetLineIdx in updatedLines.indices) {
                    val tokens = updatedLines[targetLineIdx].trim().split("\\s+".toRegex()).toMutableList()
                    if (tokens.isNotEmpty()) {
                        tokens[tokens.size - 1] = newHex
                        updatedLines[targetLineIdx] = tokens.joinToString(" ")
                    }
                } else if (updatedLines.isNotEmpty()) {
                    val tokens = updatedLines[0].trim().split("\\s+".toRegex()).toMutableList()
                    tokens[tokens.size - 1] = newHex
                    updatedLines[0] = tokens.joinToString(" ")
                }

                val newCode = updatedLines.joinToString("\n")
                item.code = newCode

                updateCheatInFile(item)
                saveCheats()
                adapter.notifyDataSetChanged()
                Toast.makeText(context, getString(R.string.cheat_toast_val_updated_fmt, entered, newHex), Toast.LENGTH_SHORT).show()
            }
            .setNegativeButton(android.R.string.cancel, null)
            .show()
    }

    private fun updateCheatInFile(item: CheatModel) {
        try {
            val file = item.sourceFile
            if (file != null && file.exists()) {
                val lines = file.readLines(Charsets.UTF_8)
                val out = StringBuilder()
                var inTarget = false
                for (line in lines) {
                    val trimmed = line.trim()
                    if (trimmed.startsWith("[") && trimmed.contains("]")) {
                        val name = trimmed.substring(1, trimmed.indexOf(']')).trim()
                        if (name == item.name) {
                            inTarget = true
                            out.append("[$name]\n").append(item.code).append("\n\n")
                            continue
                        } else {
                            inTarget = false
                        }
                    }
                    if (!inTarget) {
                        out.append(line).append("\n")
                    }
                }
                file.writeText(out.toString().trim() + "\n", Charsets.UTF_8)
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun showCheatActionMenu(item: CheatModel) {
        val options = arrayOf(getString(R.string.cheat_menu_edit_val), getString(R.string.cheat_menu_copy_code), getString(R.string.cheat_menu_delete))
        MaterialAlertDialogBuilder(requireContext(), R.style.EdenMaterialDialog)
            .setTitle(item.name)
            .setItems(options) { _, which ->
                when (which) {
                    0 -> showEditCheatValueDialog(item)
                    1 -> {
                        val cm = requireContext().getSystemService(android.content.Context.CLIPBOARD_SERVICE) as android.content.ClipboardManager
                        cm.setPrimaryClip(android.content.ClipData.newPlainText("Cheat", "[${item.name}]\n${item.code}"))
                        Toast.makeText(requireContext(), getString(R.string.cheat_toast_code_copied), Toast.LENGTH_SHORT).show()
                    }
                    2 -> {
                        deleteCheat(item)
                    }
                }
            }
            .show()
    }

    private fun deleteCheat(item: CheatModel) {
        allCheats.remove(item)
        displayedCheats.remove(item)
        try {
            val file = item.sourceFile
            if (file != null && file.exists()) {
                val lines = file.readLines(Charsets.UTF_8)
                val out = StringBuilder()
                var inTarget = false
                for (line in lines) {
                    val trimmed = line.trim()
                    if (trimmed.startsWith("[") && trimmed.contains("]")) {
                        val name = trimmed.substring(1, trimmed.indexOf(']')).trim()
                        inTarget = (name == item.name)
                    }
                    if (!inTarget) {
                        out.append(line).append("\n")
                    }
                }
                file.writeText(out.toString().trim() + "\n", Charsets.UTF_8)
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
        saveCheats()
        adapter.notifyDataSetChanged()
        updateSummary()
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }

    private inner class CheatsAdapter(
        private val items: List<CheatModel>,
        private val isRunning: Boolean,
        private val onCheckChanged: (CheatModel, Boolean) -> Unit,
        private val onEditValueClicked: (CheatModel) -> Unit,
        private val onLongClicked: (CheatModel) -> Unit
    ) : RecyclerView.Adapter<CheatsAdapter.CheatViewHolder>() {

        inner class CheatViewHolder(val binding: ItemCheatBinding) : RecyclerView.ViewHolder(binding.root)

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): CheatViewHolder {
            val binding = ItemCheatBinding.inflate(LayoutInflater.from(parent.context), parent, false)
            return CheatViewHolder(binding)
        }

        override fun onBindViewHolder(holder: CheatViewHolder, position: Int) {
            val item = items[position]
            val b = holder.binding
            val primaryColor = ThemeHelper.getColor(b.root.context, androidx.appcompat.R.attr.colorPrimary)
            val secondaryTextColor = ThemeHelper.getColor(b.root.context, com.google.android.material.R.attr.colorOnSurfaceVariant)

            b.textCheatName.text = item.name
            b.checkboxCheat.setOnCheckedChangeListener(null)
            b.checkboxCheat.isChecked = item.isEnabled

            b.textBuildIdBadge.text = if (item.buildId.isEmpty() || item.buildId.equals("ALL", ignoreCase = true) || item.buildId.equals("CHEATS", ignoreCase = true) || item.buildId.equals(game.programIdHex, ignoreCase = true)) {
                b.root.context.getString(R.string.cheat_build_all_versions)
            } else {
                item.buildId
            }
            b.textBuildIdBadge.visibility = View.VISIBLE

            if (isRunning && item.isEnabled) {
                b.textCheatStatus.text = b.root.context.getString(R.string.cheat_status_active)
                b.textCheatStatus.setTextColor(primaryColor)
            } else if (item.isEnabled) {
                b.textCheatStatus.text = b.root.context.getString(R.string.cheat_status_enabled)
                b.textCheatStatus.setTextColor(primaryColor)
            } else {
                b.textCheatStatus.text = b.root.context.getString(R.string.cheat_status_disabled)
                b.textCheatStatus.setTextColor(secondaryTextColor)
            }

            val localizedDesc = getLocalizedCheatDescription(b.root.context, item.name, item.code)
            b.textCheatDescription.text = localizedDesc
            b.textCheatDescription.visibility = if (localizedDesc.isNotEmpty()) View.VISIBLE else View.GONE

            val lines = item.code.lines().filter { it.isNotBlank() }
            val firstLine = lines.firstOrNull() ?: ""
            b.textCodePreview.text = if (lines.size > 1) b.root.context.getString(R.string.cheat_code_more_lines_fmt, firstLine, lines.size - 1) else firstLine

            // Show value editor button ONLY for numeric quantity cheats (money, items, ammo, stats, etc.)
            val isEditable = isNumericQuantityCheat(item.name, item.code)
            b.btnEditValue.visibility = if (isEditable && lines.isNotEmpty()) View.VISIBLE else View.GONE
            b.btnEditValue.setOnClickListener {
                onEditValueClicked(item)
            }

            b.checkboxCheat.setOnCheckedChangeListener { _, isChecked ->
                onCheckChanged(item, isChecked)
                if (isRunning && isChecked) {
                    b.textCheatStatus.text = b.root.context.getString(R.string.cheat_status_active)
                    b.textCheatStatus.setTextColor(primaryColor)
                } else if (isChecked) {
                    b.textCheatStatus.text = b.root.context.getString(R.string.cheat_status_enabled)
                    b.textCheatStatus.setTextColor(primaryColor)
                } else {
                    b.textCheatStatus.text = b.root.context.getString(R.string.cheat_status_disabled)
                    b.textCheatStatus.setTextColor(secondaryTextColor)
                }
            }

            b.root.setOnClickListener {
                b.checkboxCheat.isChecked = !b.checkboxCheat.isChecked
            }

            b.root.setOnLongClickListener {
                onLongClicked(item)
                true
            }
        }

        override fun getItemCount(): Int = items.size

        private fun getLocalizedCheatDescription(ctx: android.content.Context, name: String, code: String): String {
            val lower = name.lowercase().trim()
            val baseDesc = when {
                lower.contains("60fps") || lower.contains("60 fps") -> ctx.getString(R.string.cheat_desc_60fps)
                lower.contains("30fps") || lower.contains("30 fps") -> ctx.getString(R.string.cheat_desc_30fps)
                lower.contains("120fps") || lower.contains("120 fps") -> ctx.getString(R.string.cheat_desc_120fps)
                lower.contains("unlock fps") || lower.contains("uncap fps") || lower.contains("no vsync") -> ctx.getString(R.string.cheat_desc_unlock_fps)
                lower.contains("dynamic res") || lower.contains("dynamic resolution") || lower.contains("drs") -> ctx.getString(R.string.cheat_desc_dynamic_res)
                lower.contains("lod") || lower.contains("draw distance") || lower.contains("render distance") -> ctx.getString(R.string.cheat_desc_lod)
                lower.contains("god mode") || lower.contains("godmode") || lower.contains("invincib") || lower.contains("no damage") -> ctx.getString(R.string.cheat_desc_god_mode)
                lower.contains("inf health") || lower.contains("inf hp") || lower.contains("infinite health") || lower.contains("infinite hp") || lower.contains("max hp") -> ctx.getString(R.string.cheat_desc_inf_health)
                lower.contains("inf stam") || lower.contains("infinite stam") || lower.contains("max stam") || lower.contains("stamina") -> ctx.getString(R.string.cheat_desc_inf_stamina)
                lower.contains("inf mana") || lower.contains("infinite mana") || lower.contains("inf mp") || lower.contains("infinite mp") || lower.contains("max mp") -> ctx.getString(R.string.cheat_desc_inf_mana)
                lower.contains("shield") || lower.contains("armor") || lower.contains("defense") || lower.contains("def") -> ctx.getString(R.string.cheat_desc_armor)
                lower.contains("money") || lower.contains("gold") || lower.contains("cash") || lower.contains("coins") || lower.contains("rupees") || lower.contains("zenny") || lower.contains("credits") || lower.contains("currency") -> ctx.getString(R.string.cheat_desc_money)
                lower.contains("ammo") || lower.contains("bullet") || lower.contains("arrows") || lower.contains("shots") -> ctx.getString(R.string.cheat_desc_ammo)
                lower.contains("all items") || lower.contains("inf item") || lower.contains("item x99") || lower.contains("material") || lower.contains("crafting") -> ctx.getString(R.string.cheat_desc_items)
                lower.contains("durability") || lower.contains("inf weapon") || lower.contains("never break") -> ctx.getString(R.string.cheat_desc_durability)
                lower.contains("moon jump") || lower.contains("super jump") || lower.contains("high jump") || lower.contains("inf jump") -> ctx.getString(R.string.cheat_desc_jump)
                lower.contains("no clip") || lower.contains("noclip") || lower.contains("walk through") || lower.contains("fly") || lower.contains("flight") -> ctx.getString(R.string.cheat_desc_noclip)
                lower.contains("speed") || lower.contains("fast move") || lower.contains("movement") || lower.contains("fast run") -> ctx.getString(R.string.cheat_desc_speed)
                lower.contains("one hit") || lower.contains("1 hit") || lower.contains("instant kill") || lower.contains("one shot") -> ctx.getString(R.string.cheat_desc_one_hit)
                lower.contains("attack") || lower.contains("damage") || lower.contains("dmg") || lower.contains("power") || lower.contains("strength") -> ctx.getString(R.string.cheat_desc_attack)
                lower.contains("cooldown") || lower.contains("no cd") || lower.contains("instant skill") -> ctx.getString(R.string.cheat_desc_cooldown)
                lower.contains("skill point") || lower.contains("sp") || lower.contains("ap") || lower.contains("talent") -> ctx.getString(R.string.cheat_desc_sp)
                lower.contains("level") || lower.contains("lvl") || lower.contains("max level") || lower.contains("exp") || lower.contains("experience") -> ctx.getString(R.string.cheat_desc_level)
                lower.contains("fog") || lower.contains("disable fog") || lower.contains("remove fog") || lower.contains("no fog") -> ctx.getString(R.string.cheat_desc_fog)
                lower.contains("widescreen") || lower.contains("21:9") || lower.contains("ultrawide") -> ctx.getString(R.string.cheat_desc_widescreen)
                lower.contains("fov") || lower.contains("camera") || lower.contains("zoom") -> ctx.getString(R.string.cheat_desc_fov)
                lower.contains("drop rate") || lower.contains("100% drop") || lower.contains("max drop") || lower.contains("lucky") -> ctx.getString(R.string.cheat_desc_drop_rate)
                lower.contains("unlock") || lower.contains("all characters") || lower.contains("costumes") || lower.contains("gallery") -> ctx.getString(R.string.cheat_desc_unlock)
                lower.contains("timer") || lower.contains("freeze time") || lower.contains("inf time") -> ctx.getString(R.string.cheat_desc_timer)
                lower.contains("master") || lower.contains("main code") -> ctx.getString(R.string.cheat_desc_master)
                else -> {
                    val firstLine = code.lines().firstOrNull { it.isNotBlank() }?.trim() ?: ""
                    when {
                        firstLine.startsWith("04") -> ctx.getString(R.string.cheat_desc_mem_32)
                        firstLine.startsWith("08") -> ctx.getString(R.string.cheat_desc_mem_64)
                        firstLine.startsWith("01") || firstLine.startsWith("02") -> ctx.getString(R.string.cheat_desc_mem_reg)
                        firstLine.startsWith("58") -> ctx.getString(R.string.cheat_desc_mem_ptr)
                        else -> ctx.getString(R.string.cheat_desc_custom)
                    }
                }
            }

            if (isNumericQuantityCheat(name, code)) {
                val value = extractNumericValue(code)
                if (value > 0) {
                    val prefix = if (baseDesc.isNotEmpty()) baseDesc else name
                    return ctx.getString(R.string.cheat_desc_qty_fmt, prefix, value)
                }
            }

            return baseDesc
        }

        private fun extractNumericValue(code: String): Long {
            val lines = code.lines().filter { it.isNotBlank() }
            for (line in lines) {
                val tokens = line.trim().split("\\s+".toRegex())
                if (tokens.size >= 2) {
                    val lastToken = tokens.last()
                    val dec = try { lastToken.toLong(16) } catch (_: Exception) { 0L }
                    if (dec > 0) return dec
                }
            }
            return 0L
        }

        private fun isNumericQuantityCheat(name: String, code: String): Boolean {
            val lowerName = name.lowercase()

            val quantityKeywords = listOf(
                "money", "gold", "cash", "coin", "rupee", "dollar", "zenny", "gil", "bell", "credit",
                "token", "point", "gem", "shard", "cap", "currency", "деньги", "золото", "монет",
                "hp", "health", "mp", "sp", "mana", "stamina", "exp", "level", "lvl", "stat",
                "atk", "def", "speed", "опыт", "здоровье", "мана", "уровен",
                "item", "amount", "quantity", "count", "ammo", "bullet", "arrow", "bomb", "potion",
                "предмет", "патрон", "стрел", "количеств", "score", "life", "lives"
            )
            if (quantityKeywords.any { lowerName.contains(it) }) {
                return true
            }

            val toggleKeywords = listOf(
                "fps", "framerate", "frame rate", "60fps", "30fps", "120fps", "uncap", "speed limit",
                "resolution", "dynamic res", "widescreen", "aspect", "hdr", "bloom", "fog", "vsync",
                "invincible", "god mode", "godmode", "no damage",
                "moon jump", "noclip", "no clip", "freeze", "unlock all", "all items", "100%", "skip"
            )
            if (toggleKeywords.any { lowerName.contains(it) }) {
                return false
            }

            val lines = code.lines().filter { it.isNotBlank() }
            for (line in lines) {
                val tokens = line.trim().split("\\s+".toRegex())
                if (tokens.size >= 3 && (tokens[0].startsWith("04") || tokens[0].startsWith("08") || tokens[0].startsWith("02") || tokens[0].startsWith("01"))) {
                    return true
                }
            }
            return false
        }
    }
}

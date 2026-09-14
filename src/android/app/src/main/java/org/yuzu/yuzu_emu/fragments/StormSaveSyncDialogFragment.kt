// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.app.Dialog
import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.os.Build
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.core.view.isVisible
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.activityViewModels
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.RequestBody.Companion.toRequestBody
import org.json.JSONArray
import org.json.JSONObject
import org.yuzu.yuzu_emu.NativeLibrary
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.YuzuApplication
import org.yuzu.yuzu_emu.databinding.DialogStormSaveConflictBinding
import org.yuzu.yuzu_emu.databinding.DialogStormSaveSyncBinding
import org.yuzu.yuzu_emu.databinding.ItemStormSaveSyncBinding
import org.yuzu.yuzu_emu.model.GamesViewModel
import org.yuzu.yuzu_emu.utils.DirectoryInitialization
import org.yuzu.yuzu_emu.utils.Log
import java.io.BufferedInputStream
import java.io.BufferedOutputStream
import java.io.ByteArrayOutputStream
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.Inet4Address
import java.net.InetAddress
import java.net.NetworkInterface
import java.net.ServerSocket
import java.net.Socket
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.TimeUnit
import java.util.zip.ZipEntry
import java.util.zip.ZipInputStream
import java.util.zip.ZipOutputStream

enum class AndroidSaveSyncStatus {
    SYNCHRONIZED,
    CONFLICT,
    LOCAL_ONLY,
    REMOTE_ONLY,
    UNKNOWN
}

enum class AndroidConflictAction {
    CANCEL,
    REPLACE_CURRENT,
    REPLACE_REMOTE,
    KEEP_BOTH
}

data class AndroidSaveItem(
    val titleId: String,
    var titleName: String,
    var localTimestamp: Long = 0L,
    var localDateStr: String = "",
    var localSizeBytes: Long = 0L,
    var localFileCount: Int = 0,
    var hasLocal: Boolean = false,

    var remoteTimestamp: Long = 0L,
    var remoteDateStr: String = "",
    var remoteSizeBytes: Long = 0L,
    var remoteFileCount: Int = 0,
    var hasRemote: Boolean = false,

    var status: AndroidSaveSyncStatus = AndroidSaveSyncStatus.UNKNOWN
)

class StormSaveSyncDialogFragment : DialogFragment() {

    private var _binding: DialogStormSaveSyncBinding? = null
    private val binding get() = _binding!!

    private val gamesViewModel: GamesViewModel by activityViewModels()

    private val httpClient = OkHttpClient.Builder()
        .connectTimeout(8, TimeUnit.SECONDS)
        .readTimeout(30, TimeUnit.SECONDS)
        .writeTimeout(30, TimeUnit.SECONDS)
        .build()

    private var serverSocket: ServerSocket? = null
    private var serverJob: Job? = null
    private var udpSocket: DatagramSocket? = null
    private var udpJob: Job? = null

    private var localIp: String = "127.0.0.1"
    private var localPort: Int = 28443
    private var localKey: String = ""

    private var remoteIp: String? = null
    private var remotePort: Int = 28443
    private var remoteDeviceName: String = ""
    private var isConnected: Boolean = false

    private val saveItems = ConcurrentHashMap<String, AndroidSaveItem>()
    private val discoveredDevices = ConcurrentHashMap<String, String>() // key -> name

    private lateinit var adapter: SaveSyncAdapter

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setStyle(STYLE_NORMAL, R.style.Theme_Yuzu_Main)
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = DialogStormSaveSyncBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        setupUI()
        startLocalServer()
        startUdpDiscovery()
        scanLocalSaves()
        updateList()

        // Discover devices in LAN
        broadcastDiscovery()
    }

    override fun onDestroyView() {
        super.onDestroyView()
        stopLocalServer()
        _binding = null
    }

    private fun setupUI() {
        binding.btnClose.setOnClickListener { dismiss() }
        binding.btnRefresh.setOnClickListener {
            scanLocalSaves()
            if (isConnected) {
                fetchRemoteSaves()
            } else {
                updateList()
            }
            broadcastDiscovery()
        }

        binding.btnCopyKey.setOnClickListener {
            if (localKey.isNotEmpty()) {
                val clipboard = requireContext().getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
                clipboard.setPrimaryClip(ClipData.newPlainText("STORM SAVE SYNC Key", localKey))
                Toast.makeText(requireContext(), getString(R.string.storm_save_key_copied), Toast.LENGTH_SHORT).show()
            }
        }

        binding.btnConnect.setOnClickListener {
            val input = binding.editRemoteKey.text.toString().trim()
            if (input.isEmpty()) {
                Toast.makeText(requireContext(), "Введите ключ или IP:порт", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
            val parsed = parseConnectionKey(input)
            if (parsed == null) {
                Toast.makeText(requireContext(), "Неверный формат ключа", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
            connectToRemote(parsed.first, parsed.second)
        }

        binding.btnSearchDevices.setOnClickListener {
            broadcastDiscovery()
            Toast.makeText(requireContext(), "Поиск устройств в сети Wi-Fi...", Toast.LENGTH_SHORT).show()
        }

        binding.btnSyncAll.setOnClickListener {
            if (!isConnected) {
                Toast.makeText(requireContext(), "Сначала подключитесь к устройству", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
            syncAllSaves()
        }

        adapter = SaveSyncAdapter(
            items = mutableListOf(),
            onSyncClick = { item -> handleSyncItem(item) }
        )
        binding.recyclerSaves.layoutManager = LinearLayoutManager(requireContext())
        binding.recyclerSaves.adapter = adapter
    }

    private fun startLocalServer() {
        localIp = getLocalIpAddress() ?: "127.0.0.1"
        localPort = 28443
        localKey = generateConnectionKey(localIp, localPort)

        binding.textMyKey.text = "$localKey ($localIp:$localPort)"

        serverJob = viewLifecycleOwner.lifecycleScope.launch(Dispatchers.IO) {
            try {
                serverSocket = try {
                    ServerSocket(localPort)
                } catch (_: Exception) {
                    localPort = 28445
                    localKey = generateConnectionKey(localIp, localPort)
                    withContext(Dispatchers.Main) {
                        _binding?.textMyKey?.text = "$localKey ($localIp:$localPort)"
                    }
                    ServerSocket(localPort)
                }

                while (isActive && serverSocket != null && !serverSocket!!.isClosed) {
                    val client = serverSocket!!.accept()
                    launch(Dispatchers.IO) {
                        handleClientConnection(client)
                    }
                }
            } catch (e: Exception) {
                Log.error("[StormSaveSync] Server error: ${e.message}")
            }
        }
    }

    private fun stopLocalServer() {
        try {
            serverSocket?.close()
            serverSocket = null
        } catch (_: Exception) {}
        serverJob?.cancel()

        try {
            udpSocket?.close()
            udpSocket = null
        } catch (_: Exception) {}
        udpJob?.cancel()
    }

    private fun startUdpDiscovery() {
        udpJob = viewLifecycleOwner.lifecycleScope.launch(Dispatchers.IO) {
            try {
                udpSocket = DatagramSocket(28444).apply {
                    broadcast = true
                }
                val buffer = ByteArray(1024)
                val packet = DatagramPacket(buffer, buffer.size)

                while (isActive && udpSocket != null && !udpSocket!!.isClosed) {
                    udpSocket!!.receive(packet)
                    val msg = String(packet.data, 0, packet.length).trim()

                    if (msg == "STORM_SYNC_DISCOVER") {
                        val reply = "STORM_SYNC_ANNOUNCE:$localKey:${Build.MANUFACTURER} ${Build.MODEL}:Android"
                        val replyData = reply.toByteArray(Charsets.UTF_8)
                        val replyPacket = DatagramPacket(replyData, replyData.size, packet.address, packet.port)
                        udpSocket?.send(replyPacket)
                    } else if (msg.startsWith("STORM_SYNC_ANNOUNCE:")) {
                        val tokens = msg.split(":")
                        if (tokens.size >= 3) {
                            val rKey = tokens[1]
                            val rName = tokens[2]
                            val rPlatform = if (tokens.size >= 4) tokens[3] else "Device"
                            discoveredDevices[rKey] = "$rName ($rPlatform)"

                            withContext(Dispatchers.Main) {
                                updateDiscoveredSpinner()
                            }
                        }
                    }
                }
            } catch (e: Exception) {
                Log.error("[StormSaveSync] UDP error: ${e.message}")
            }
        }
    }

    private fun broadcastDiscovery() {
        viewLifecycleOwner.lifecycleScope.launch(Dispatchers.IO) {
            try {
                val ping = "STORM_SYNC_DISCOVER".toByteArray(Charsets.UTF_8)
                val broadcastAddr = InetAddress.getByName("255.255.255.255")
                val packet = DatagramPacket(ping, ping.size, broadcastAddr, 28444)
                udpSocket?.send(packet)
            } catch (e: Exception) {
                Log.error("[StormSaveSync] Broadcast error: ${e.message}")
            }
        }
    }

    private fun updateDiscoveredSpinner() {
        val binding = _binding ?: return
        val list = mutableListOf("Обнаруженные устройства в сети (${discoveredDevices.size})")
        val keys = mutableListOf("")

        for ((k, name) in discoveredDevices) {
            list.add("$name — $k")
            keys.add(k)
        }

        val adapter = ArrayAdapter(requireContext(), android.R.layout.simple_spinner_dropdown_item, list)
        binding.spinnerDiscovered.adapter = adapter
        binding.spinnerDiscovered.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                if (position > 0) {
                    val selectedKey = keys[position]
                    binding.editRemoteKey.setText(selectedKey)
                    val parsed = parseConnectionKey(selectedKey)
                    if (parsed != null) {
                        connectToRemote(parsed.first, parsed.second)
                    }
                }
            }
            override fun onNothingSelected(parent: AdapterView<*>?) {}
        }
    }

    private suspend fun handleClientConnection(socket: Socket) {
        try {
            val input = socket.getInputStream()
            val output = socket.getOutputStream()

            val reader = input.bufferedReader(Charsets.UTF_8)
            val firstLine = reader.readLine() ?: return
            val parts = firstLine.split(" ")
            if (parts.size < 2) return

            val method = parts[0]
            val path = parts[1]

            val headers = mutableMapOf<String, String>()
            var line: String? = reader.readLine()
            var contentLength = 0
            while (!line.isNullOrEmpty()) {
                val colon = line.indexOf(':')
                if (colon != -1) {
                    val k = line.substring(0, colon).trim().lowercase()
                    val v = line.substring(colon + 1).trim()
                    headers[k] = v
                    if (k == "content-length") {
                        contentLength = v.toIntOrNull() ?: 0
                    }
                }
                line = reader.readLine()
            }

            fun sendResponse(status: Int, contentType: String, body: ByteArray) {
                val head = "HTTP/1.1 $status OK\r\n" +
                        "Content-Type: $contentType\r\n" +
                        "Content-Length: ${body.size}\r\n" +
                        "Connection: close\r\n\r\n"
                output.write(head.toByteArray(Charsets.UTF_8))
                output.write(body)
                output.flush()
            }

            val cleanPath = path.substringBefore("?")
            val query = if (path.contains("?")) path.substringAfter("?") else ""
            val queryParams = query.split("&").mapNotNull {
                val p = it.split("=")
                if (p.size == 2) p[0] to p[1] else null
            }.toMap()

            when {
                method == "GET" && cleanPath == "/api/status" -> {
                    val obj = JSONObject().apply {
                        put("status", "ok")
                        put("device_name", "${Build.MANUFACTURER} ${Build.MODEL}")
                        put("platform", "android")
                        put("version", "8.5.0")
                    }
                    sendResponse(200, "application/json", obj.toString().toByteArray(Charsets.UTF_8))
                }
                method == "GET" && cleanPath == "/api/saves" -> {
                    scanLocalSaves()
                    val arr = JSONArray()
                    for ((_, item) in saveItems) {
                        if (!item.hasLocal) continue
                        val obj = JSONObject().apply {
                            put("title_id", item.titleId)
                            put("title_name", item.titleName)
                            put("timestamp", item.localTimestamp)
                            put("date_str", item.localDateStr)
                            put("size_bytes", item.localSizeBytes)
                            put("file_count", item.localFileCount)
                        }
                        arr.put(obj)
                    }
                    sendResponse(200, "application/json", arr.toString().toByteArray(Charsets.UTF_8))
                }
                method == "GET" && cleanPath == "/api/save/download" -> {
                    val titleId = queryParams["title_id"]?.uppercase()
                    if (titleId.isNullOrEmpty()) {
                        sendResponse(400, "text/plain", "Missing title_id".toByteArray())
                        return
                    }
                    val saveFolder = getLocalSaveDirForTitle(titleId)
                    if (saveFolder == null || !saveFolder.exists()) {
                        sendResponse(404, "text/plain", "Save not found".toByteArray())
                        return
                    }

                    val baos = ByteArrayOutputStream()
                    ZipOutputStream(BufferedOutputStream(baos)).use { zos ->
                        zipDirectory(saveFolder, saveFolder, zos)
                    }
                    val zipData = baos.toByteArray()
                    sendResponse(200, "application/zip", zipData)
                }
                method == "POST" && cleanPath == "/api/save/upload" -> {
                    val titleId = queryParams["title_id"]?.uppercase()
                    if (titleId.isNullOrEmpty()) {
                        sendResponse(400, "text/plain", "Missing title_id".toByteArray())
                        return
                    }

                    val rawInput = BufferedInputStream(input)
                    val bodyBytes = ByteArray(contentLength)
                    var read = 0
                    while (read < contentLength) {
                        val r = rawInput.read(bodyBytes, read, contentLength - read)
                        if (r == -1) break
                        read += r
                    }

                    val targetDir = getTargetSaveDirForTitle(titleId)
                    targetDir.mkdirs()

                    ZipInputStream(bodyBytes.inputStream()).use { zis ->
                        var entry = zis.nextEntry
                        while (entry != null) {
                            val f = File(targetDir, entry.name)
                            if (entry.isDirectory) {
                                f.mkdirs()
                            } else {
                                f.parentFile?.mkdirs()
                                FileOutputStream(f).use { fos ->
                                    zis.copyTo(fos)
                                }
                            }
                            entry = zis.nextEntry
                        }
                    }

                    scanLocalSaves()
                    withContext(Dispatchers.Main) {
                        updateList()
                    }
                    sendResponse(200, "application/json", "{\"status\":\"ok\"}".toByteArray())
                }
                method == "POST" && cleanPath == "/api/save/backup" -> {
                    val titleId = queryParams["title_id"]?.uppercase()
                    if (titleId.isNullOrEmpty()) {
                        sendResponse(400, "text/plain", "Missing title_id".toByteArray())
                        return
                    }
                    backupLocalSave(titleId)
                    sendResponse(200, "application/json", "{\"status\":\"ok\"}".toByteArray())
                }
                else -> {
                    sendResponse(404, "text/plain", "Not Found".toByteArray())
                }
            }
        } catch (e: Exception) {
            Log.error("[StormSaveSync] Client handler error: ${e.message}")
        } finally {
            try { socket.close() } catch (_: Exception) {}
        }
    }

    private fun connectToRemote(ip: String, port: Int) {
        remoteIp = ip
        remotePort = port
        binding.textConnectionState.text = "Подключение к $ip:$port..."

        viewLifecycleOwner.lifecycleScope.launch(Dispatchers.IO) {
            try {
                val req = Request.Builder()
                    .url("http://$ip:$port/api/status")
                    .build()
                val resp = httpClient.newCall(req).execute()
                if (resp.isSuccessful) {
                    val body = resp.body?.string() ?: "{}"
                    val obj = JSONObject(body)
                    remoteDeviceName = obj.optString("device_name", "Удалённый узел")
                    val platform = obj.optString("platform", "узел")
                    isConnected = true

                    withContext(Dispatchers.Main) {
                        binding.textConnectionState.text = "🟢 Подключено: $remoteDeviceName ($platform) [$ip:$port]"
                        fetchRemoteSaves()
                    }
                    return@launch
                }
            } catch (_: Exception) {}

            withContext(Dispatchers.Main) {
                isConnected = false
                binding.textConnectionState.text = "❌ Ошибка подключения к $ip:$port"
                Toast.makeText(requireContext(), "Не удалось подключиться к $ip:$port", Toast.LENGTH_LONG).show()
            }
        }
    }

    private fun fetchRemoteSaves() {
        val ip = remoteIp ?: return
        val port = remotePort

        viewLifecycleOwner.lifecycleScope.launch(Dispatchers.IO) {
            try {
                val req = Request.Builder()
                    .url("http://$ip:$port/api/saves")
                    .build()
                val resp = httpClient.newCall(req).execute()
                if (resp.isSuccessful) {
                    val body = resp.body?.string() ?: "[]"
                    val arr = JSONArray(body)

                    // Clear previous remote markers
                    for ((_, item) in saveItems) {
                        item.hasRemote = false
                        item.remoteTimestamp = 0L
                        item.remoteSizeBytes = 0L
                        item.remoteFileCount = 0
                        item.remoteDateStr = ""
                    }

                    for (i in 0 until arr.length()) {
                        val obj = arr.getJSONObject(i)
                        val titleId = obj.optString("title_id").uppercase()
                        if (titleId.isEmpty()) continue

                        val item = saveItems.getOrPut(titleId) {
                            AndroidSaveItem(titleId, obj.optString("title_name", titleId))
                        }
                        item.hasRemote = true
                        item.remoteTimestamp = obj.optLong("timestamp", 0L)
                        item.remoteDateStr = obj.optString("date_str", "")
                        item.remoteSizeBytes = obj.optLong("size_bytes", 0L)
                        item.remoteFileCount = obj.optInt("file_count", 0)
                        if (item.titleName.isEmpty() || item.titleName == titleId) {
                            item.titleName = obj.optString("title_name", titleId)
                        }
                    }

                    withContext(Dispatchers.Main) {
                        updateComparisonList()
                        updateList()
                    }
                }
            } catch (e: Exception) {
                Log.error("[StormSaveSync] Fetch remote saves error: ${e.message}")
            }
        }
    }

    private fun scanLocalSaves() {
        val userDirStr = DirectoryInitialization.userDirectory ?: YuzuApplication.appContext.filesDir.absolutePath
        val saveRoot = File(userDirStr, "nand/user/save/0000000000000000")
        if (!saveRoot.exists() || !saveRoot.isDirectory) return

        val userDirs = saveRoot.listFiles { f -> f.isDirectory } ?: return
        val dateFormat = SimpleDateFormat("dd.MM.yyyy HH:mm", Locale.getDefault())

        for (uDir in userDirs) {
            val titleDirs = uDir.listFiles { f -> f.isDirectory && f.name.length == 16 } ?: continue
            for (tDir in titleDirs) {
                val titleId = tDir.name.uppercase()
                var totalBytes = 0L
                var fileCount = 0
                var latestModified = 0L

                tDir.walkTopDown().forEach { file ->
                    if (file.isFile) {
                        totalBytes += file.length()
                        fileCount++
                        if (file.lastModified() > latestModified) {
                            latestModified = file.lastModified()
                        }
                    }
                }

                if (fileCount == 0) continue

                val item = saveItems.getOrPut(titleId) {
                    val resolvedTitle = gamesViewModel.games.value.find {
                        it.programIdHex.equals(titleId, ignoreCase = true)
                    }?.title ?: titleId
                    AndroidSaveItem(titleId, resolvedTitle)
                }

                item.hasLocal = true
                item.localSizeBytes = totalBytes
                item.localFileCount = fileCount
                item.localTimestamp = latestModified / 1000L
                item.localDateStr = if (latestModified > 0) dateFormat.format(Date(latestModified)) else "-"
            }
        }

        updateComparisonList()
    }

    private fun updateComparisonList() {
        for ((_, item) in saveItems) {
            item.status = when {
                item.hasLocal && item.hasRemote -> {
                    val diff = Math.abs(item.localTimestamp - item.remoteTimestamp)
                    if (diff <= 3 && item.localSizeBytes == item.remoteSizeBytes) {
                        AndroidSaveSyncStatus.SYNCHRONIZED
                    } else {
                        AndroidSaveSyncStatus.CONFLICT
                    }
                }
                item.hasLocal -> AndroidSaveSyncStatus.LOCAL_ONLY
                item.hasRemote -> AndroidSaveSyncStatus.REMOTE_ONLY
                else -> AndroidSaveSyncStatus.UNKNOWN
            }
        }
    }

    private fun updateList() {
        val binding = _binding ?: return
        val list = saveItems.values.toList().sortedBy { it.titleName.lowercase() }
        adapter.submitList(list)
        binding.textSavesHeader.text = "Сохранения игр (${list.size})"
        binding.textEmptySaves.isVisible = list.isEmpty()
    }

    private fun handleSyncItem(item: AndroidSaveItem) {
        if (!isConnected) {
            Toast.makeText(requireContext(), "Сначала подключитесь к удалённому устройству", Toast.LENGTH_SHORT).show()
            return
        }

        when (item.status) {
            AndroidSaveSyncStatus.CONFLICT -> {
                showConflictDialog(item)
            }
            AndroidSaveSyncStatus.LOCAL_ONLY -> {
                uploadSave(item.titleId)
            }
            AndroidSaveSyncStatus.REMOTE_ONLY -> {
                downloadSave(item.titleId)
            }
            AndroidSaveSyncStatus.SYNCHRONIZED -> {
                Toast.makeText(requireContext(), "Сохранения полностью идентичны", Toast.LENGTH_SHORT).show()
            }
            else -> {}
        }
    }

    private fun showConflictDialog(item: AndroidSaveItem) {
        val conflictBinding = DialogStormSaveConflictBinding.inflate(layoutInflater)
        val dialog = MaterialAlertDialogBuilder(requireContext())
            .setView(conflictBinding.root)
            .create()

        conflictBinding.textConflictSubtitle.text = "Игра: ${item.titleName} [${item.titleId}]"

        conflictBinding.textLocalDate.text = "📅 Дата и время: ${item.localDateStr}"
        conflictBinding.textLocalSize.text = "📦 Размер данных: ${formatSize(item.localSizeBytes)} • Файлов: ${item.localFileCount}"

        conflictBinding.textRemoteDeviceTitle.text = "Удалённое устройство (${if (remoteDeviceName.isNotEmpty()) remoteDeviceName else "ПК"})"
        conflictBinding.textRemoteDate.text = "📅 Дата и время: ${item.remoteDateStr}"
        conflictBinding.textRemoteSize.text = "📦 Размер данных: ${formatSize(item.remoteSizeBytes)} • Файлов: ${item.remoteFileCount}"

        conflictBinding.btnReplaceCurrent.setOnClickListener {
            dialog.dismiss()
            downloadSave(item.titleId)
        }

        conflictBinding.btnReplaceRemote.setOnClickListener {
            dialog.dismiss()
            uploadSave(item.titleId)
        }

        conflictBinding.btnKeepBoth.setOnClickListener {
            dialog.dismiss()
            backupLocalSave(item.titleId)
            backupRemoteSave(item.titleId) {
                downloadSave(item.titleId)
            }
        }

        conflictBinding.btnCancelConflict.setOnClickListener {
            dialog.dismiss()
        }

        dialog.show()
    }

    private fun downloadSave(titleId: String) {
        val ip = remoteIp ?: return
        val port = remotePort

        binding.progressSync.isVisible = true

        viewLifecycleOwner.lifecycleScope.launch(Dispatchers.IO) {
            try {
                val req = Request.Builder()
                    .url("http://$ip:$port/api/save/download?title_id=$titleId")
                    .build()
                val resp = httpClient.newCall(req).execute()
                if (resp.isSuccessful) {
                    val body = resp.body?.byteStream()
                    if (body != null) {
                        val targetDir = getTargetSaveDirForTitle(titleId)
                        targetDir.mkdirs()

                        ZipInputStream(BufferedInputStream(body)).use { zis ->
                            var entry = zis.nextEntry
                            while (entry != null) {
                                val f = File(targetDir, entry.name)
                                if (entry.isDirectory) {
                                    f.mkdirs()
                                } else {
                                    f.parentFile?.mkdirs()
                                    FileOutputStream(f).use { fos ->
                                        zis.copyTo(fos)
                                    }
                                }
                                entry = zis.nextEntry
                            }
                        }

                        scanLocalSaves()
                        withContext(Dispatchers.Main) {
                            binding.progressSync.isVisible = false
                            updateList()
                            Toast.makeText(requireContext(), "Сохранение успешно загружено!", Toast.LENGTH_SHORT).show()
                        }
                        return@launch
                    }
                }
            } catch (e: Exception) {
                Log.error("[StormSaveSync] Download save error: ${e.message}")
            }

            withContext(Dispatchers.Main) {
                binding.progressSync.isVisible = false
                Toast.makeText(requireContext(), "Ошибка скачивания сохранения", Toast.LENGTH_SHORT).show()
            }
        }
    }

    private fun uploadSave(titleId: String) {
        val ip = remoteIp ?: return
        val port = remotePort
        val saveDir = getLocalSaveDirForTitle(titleId) ?: return

        binding.progressSync.isVisible = true

        viewLifecycleOwner.lifecycleScope.launch(Dispatchers.IO) {
            try {
                val baos = ByteArrayOutputStream()
                ZipOutputStream(BufferedOutputStream(baos)).use { zos ->
                    zipDirectory(saveDir, saveDir, zos)
                }
                val zipData = baos.toByteArray()

                val req = Request.Builder()
                    .url("http://$ip:$port/api/save/upload?title_id=$titleId")
                    .post(zipData.toRequestBody("application/zip".toMediaType()))
                    .build()
                val resp = httpClient.newCall(req).execute()
                if (resp.isSuccessful) {
                    fetchRemoteSaves()
                    withContext(Dispatchers.Main) {
                        binding.progressSync.isVisible = false
                        Toast.makeText(requireContext(), "Сохранение успешно отправлено на удалённое устройство!", Toast.LENGTH_SHORT).show()
                    }
                    return@launch
                }
            } catch (e: Exception) {
                Log.error("[StormSaveSync] Upload save error: ${e.message}")
            }

            withContext(Dispatchers.Main) {
                binding.progressSync.isVisible = false
                Toast.makeText(requireContext(), "Ошибка отправки сохранения", Toast.LENGTH_SHORT).show()
            }
        }
    }

    private fun backupLocalSave(titleId: String) {
        val saveDir = getLocalSaveDirForTitle(titleId) ?: return
        val stamp = SimpleDateFormat("dd.MM.yyyy_HH-mm", Locale.getDefault()).format(Date())
        val backupDir = File(saveDir.parentFile, "${titleId}_backup_$stamp")
        saveDir.copyRecursively(backupDir, overwrite = true)
    }

    private fun backupRemoteSave(titleId: String, onComplete: () -> Unit) {
        val ip = remoteIp ?: return
        val port = remotePort

        viewLifecycleOwner.lifecycleScope.launch(Dispatchers.IO) {
            try {
                val req = Request.Builder()
                    .url("http://$ip:$port/api/save/backup?title_id=$titleId")
                    .post("".toRequestBody(null))
                    .build()
                httpClient.newCall(req).execute()
            } catch (_: Exception) {}

            withContext(Dispatchers.Main) {
                onComplete()
            }
        }
    }

    private fun syncAllSaves() {
        for ((_, item) in saveItems) {
            if (item.status != AndroidSaveSyncStatus.SYNCHRONIZED) {
                handleSyncItem(item)
            }
        }
    }

    private fun getLocalSaveDirForTitle(titleId: String): File? {
        val userDirStr = DirectoryInitialization.userDirectory ?: YuzuApplication.appContext.filesDir.absolutePath
        val saveRoot = File(userDirStr, "nand/user/save/0000000000000000")
        if (!saveRoot.exists()) return null

        val userDirs = saveRoot.listFiles { f -> f.isDirectory } ?: return null
        for (uDir in userDirs) {
            val titleDir = File(uDir, titleId)
            if (titleDir.exists() && titleDir.isDirectory) {
                return titleDir
            }
        }
        return null
    }

    private fun getTargetSaveDirForTitle(titleId: String): File {
        val userDirStr = DirectoryInitialization.userDirectory ?: YuzuApplication.appContext.filesDir.absolutePath
        val saveRoot = File(userDirStr, "nand/user/save/0000000000000000")
        val userDirs = saveRoot.listFiles { f -> f.isDirectory }
        val activeUser = if (!userDirs.isNullOrEmpty()) userDirs[0] else File(saveRoot, "00000000000000010000000000000000")
        return File(activeUser, titleId)
    }

    private fun zipDirectory(root: File, source: File, zos: ZipOutputStream) {
        val files = source.listFiles() ?: return
        for (file in files) {
            if (file.isDirectory) {
                zipDirectory(root, file, zos)
            } else {
                val relPath = file.relativeTo(root).path.replace('\\', '/')
                zos.putNextEntry(ZipEntry(relPath))
                FileInputStream(file).use { fis ->
                    fis.copyTo(zos)
                }
                zos.closeEntry()
            }
        }
    }

    private fun getLocalIpAddress(): String? {
        try {
            val interfaces = NetworkInterface.getNetworkInterfaces()
            while (interfaces.hasMoreElements()) {
                val iface = interfaces.nextElement()
                if (iface.isLoopback || !iface.isUp) continue
                val addresses = iface.inetAddresses
                while (addresses.hasMoreElements()) {
                    val addr = addresses.nextElement()
                    if (addr is Inet4Address && !addr.isLoopbackAddress) {
                        return addr.hostAddress
                    }
                }
            }
        } catch (_: Exception) {}
        return null
    }

    private fun generateConnectionKey(ipStr: String, port: Int): String {
        return try {
            val parts = ipStr.split(".").map { it.toInt() }
            val hexIp = String.format("%02X%02X%02X%02X", parts[0], parts[1], parts[2], parts[3])
            val hexPort = String.format("%04X", port)
            "STORM-$hexIp-$hexPort"
        } catch (_: Exception) {
            "$ipStr:$port"
        }
    }

    private fun parseConnectionKey(rawKey: String): Pair<String, Int>? {
        val key = rawKey.trim()
        val stormRegex = Regex("^STORM-([0-9A-Fa-f]{8})-([0-9A-Fa-f]{4})$", RegexOption.IGNORE_CASE)
        val match = stormRegex.matchEntire(key)
        if (match != null) {
            val hexIp = match.groupValues[1]
            val hexPort = match.groupValues[2]
            val ip = "${hexIp.substring(0, 2).toInt(16)}.${hexIp.substring(2, 4).toInt(16)}.${hexIp.substring(4, 6).toInt(16)}.${hexIp.substring(6, 8).toInt(16)}"
            val port = hexPort.toInt(16)
            return Pair(ip, port)
        }
        if (key.contains(":")) {
            val parts = key.split(":")
            if (parts.size == 2) {
                val p = parts[1].toIntOrNull() ?: 28443
                return Pair(parts[0], p)
            }
        }
        return Pair(key, 28443)
    }

    private fun formatSize(bytes: Long): String {
        return when {
            bytes <= 0L -> "0 Б"
            bytes < 1024L -> "$bytes Б"
            bytes < 1024L * 1024L -> String.format(Locale.US, "%.1f КБ", bytes / 1024.0)
            else -> String.format(Locale.US, "%.2f МБ", bytes / (1024.0 * 1024.0))
        }
    }

    companion object {
        const val TAG = "StormSaveSyncDialogFragment"

        fun newInstance(): StormSaveSyncDialogFragment {
            return StormSaveSyncDialogFragment()
        }
    }

    // Inner Adapter
    class SaveSyncAdapter(
        private var items: List<AndroidSaveItem>,
        private val onSyncClick: (AndroidSaveItem) -> Unit
    ) : RecyclerView.Adapter<SaveSyncAdapter.ViewHolder>() {

        class ViewHolder(val binding: ItemStormSaveSyncBinding) : RecyclerView.ViewHolder(binding.root)

        fun submitList(newItems: List<AndroidSaveItem>) {
            items = newItems
            notifyDataSetChanged()
        }

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
            val binding = ItemStormSaveSyncBinding.inflate(LayoutInflater.from(parent.context), parent, false)
            return ViewHolder(binding)
        }

        override fun getItemCount(): Int = items.size

        override fun onBindViewHolder(holder: ViewHolder, position: Int) {
            val item = items[position]
            val b = holder.binding

            b.textGameTitle.text = item.titleName
            b.textTitleId.text = item.titleId

            b.textLocalSaveInfo.text = if (item.hasLocal) {
                "📱 На этом устройстве: ${item.localDateStr} (${formatSize(item.localSizeBytes)}, ${item.localFileCount} файл.)"
            } else {
                "📱 На этом устройстве: отсутствует"
            }

            b.textRemoteSaveInfo.text = if (item.hasRemote) {
                "💻 На удалённом: ${item.remoteDateStr} (${formatSize(item.remoteSizeBytes)}, ${item.remoteFileCount} файл.)"
            } else {
                "💻 На удалённом: отсутствует"
            }

            when (item.status) {
                AndroidSaveSyncStatus.SYNCHRONIZED -> {
                    b.textSyncBadge.text = "🟢 Синхронизировано"
                    b.textSyncBadge.setTextColor(0xFF10B981.toInt())
                    b.btnActionSync.text = "Синхронизировано"
                    b.btnActionSync.isEnabled = false
                }
                AndroidSaveSyncStatus.CONFLICT -> {
                    b.textSyncBadge.text = "⚠️ Конфликт"
                    b.textSyncBadge.setTextColor(0xFFF59E0B.toInt())
                    b.btnActionSync.text = "Разрешить конфликт"
                    b.btnActionSync.isEnabled = true
                }
                AndroidSaveSyncStatus.LOCAL_ONLY -> {
                    b.textSyncBadge.text = "📤 Только локально"
                    b.textSyncBadge.setTextColor(0xFF3B82F6.toInt())
                    b.btnActionSync.text = "Отправить"
                    b.btnActionSync.isEnabled = true
                }
                AndroidSaveSyncStatus.REMOTE_ONLY -> {
                    b.textSyncBadge.text = "📥 Только на удалённом"
                    b.textSyncBadge.setTextColor(0xFF8B5CF6.toInt())
                    b.btnActionSync.text = "Скачать"
                    b.btnActionSync.isEnabled = true
                }
                else -> {
                    b.textSyncBadge.text = "Неизвестно"
                    b.textSyncBadge.setTextColor(0xFF94A3B8.toInt())
                    b.btnActionSync.text = "Синхронизировать"
                    b.btnActionSync.isEnabled = true
                }
            }

            b.btnActionSync.setOnClickListener {
                onSyncClick(item)
            }
        }

        private fun formatSize(bytes: Long): String {
            return when {
                bytes <= 0L -> "0 Б"
                bytes < 1024L -> "$bytes Б"
                bytes < 1024L * 1024L -> String.format(Locale.US, "%.1f КБ", bytes / 1024.0)
                else -> String.format(Locale.US, "%.2f МБ", bytes / (1024.0 * 1024.0))
            }
        }
    }
}

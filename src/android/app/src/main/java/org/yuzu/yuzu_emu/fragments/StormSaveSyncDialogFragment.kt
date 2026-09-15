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
import android.net.wifi.WifiManager
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.core.view.isVisible
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import coil.load
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
import org.yuzu.yuzu_emu.model.GameFixDatabase
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
    private var multicastLock: WifiManager.MulticastLock? = null
    private var lastDiscoveredKeys: List<String> = emptyList()

    private var localIp: String = "127.0.0.1"
    private val localIps = mutableSetOf<String>()
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

    override fun onStart() {
        super.onStart()
        dialog?.window?.let { window ->
            window.setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT)
            window.setBackgroundDrawableResource(android.R.color.transparent)
        }
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        ViewCompat.setOnApplyWindowInsetsListener(binding.root) { _, insets ->
            val systemBars = insets.getInsets(
                WindowInsetsCompat.Type.systemBars() or
                WindowInsetsCompat.Type.displayCutout()
            )
            val density = resources.displayMetrics.density
            val hPadding = (16 * density).toInt()
            val vPadding = (12 * density).toInt()

            binding.topBar.setPadding(
                hPadding + systemBars.left,
                systemBars.top + vPadding,
                hPadding + systemBars.right,
                vPadding
            )
            binding.scrollContent.setPadding(
                systemBars.left,
                0,
                systemBars.right,
                systemBars.bottom
            )
            insets
        }
        binding.root.requestApplyInsets()

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
            if (isConnected) {
                isConnected = false
                remoteIp = ""
                remotePort = 28443
                remoteDeviceName = ""
                binding.textConnectionState.text = "Статус: не подключено к удалённому устройству"
                binding.btnConnect.text = "Подключить"
                for ((_, it) in saveItems) {
                    it.hasRemote = false
                    it.remoteTimestamp = 0L
                    it.remoteSizeBytes = 0L
                    it.remoteFileCount = 0
                    it.remoteDateStr = ""
                }
                updateComparisonList()
                updateList()
                Toast.makeText(requireContext(), "Отключено от удалённого устройства", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
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

        try {
            if (multicastLock?.isHeld == true) {
                multicastLock?.release()
            }
        } catch (_: Exception) {}
        multicastLock = null
    }

    private fun startUdpDiscovery() {
        try {
            val wifi = requireContext().applicationContext.getSystemService(Context.WIFI_SERVICE) as? WifiManager
            multicastLock = wifi?.createMulticastLock("StormSaveSyncLock")?.apply {
                setReferenceCounted(true)
                acquire()
            }
        } catch (e: Exception) {
            Log.error("[StormSaveSync] MulticastLock acquire error: ${e.message}")
        }

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
                            if (rKey.equals(localKey, ignoreCase = true)) {
                                continue
                            }
                            val rName = tokens[2]
                            val rPlatform = if (tokens.size >= 4) tokens[3] else "Device"
                            val currentName = "$rName ($rPlatform)"
                            if (!discoveredDevices.containsKey(rKey) || discoveredDevices[rKey] != currentName) {
                                discoveredDevices[rKey] = currentName
                                withContext(Dispatchers.Main) {
                                    updateDiscoveredSpinner()
                                }
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
                val broadcastTargets = mutableSetOf<InetAddress>()
                try {
                    broadcastTargets.add(InetAddress.getByName("255.255.255.255"))
                } catch (_: Exception) {}

                try {
                    val interfaces = NetworkInterface.getNetworkInterfaces()
                    while (interfaces.hasMoreElements()) {
                        val iface = interfaces.nextElement()
                        if (!iface.isUp || iface.isLoopback) continue
                        for (addr in iface.interfaceAddresses) {
                            val bcast = addr.broadcast
                            if (bcast != null) {
                                broadcastTargets.add(bcast)
                            }
                        }
                    }
                } catch (e: Exception) {
                    Log.error("[StormSaveSync] Failed to get broadcast interfaces: ${e.message}")
                }

                for (target in broadcastTargets) {
                    try {
                        val packet = DatagramPacket(ping, ping.size, target, 28444)
                        udpSocket?.send(packet)
                    } catch (_: Exception) {}
                }
            } catch (e: Exception) {
                Log.error("[StormSaveSync] Broadcast error: ${e.message}")
            }
        }
    }

    private fun updateDiscoveredSpinner() {
        val binding = _binding ?: return
        val currentKeys = discoveredDevices.keys.toList().sorted()
        if (currentKeys == lastDiscoveredKeys && binding.spinnerDiscovered.adapter != null) {
            return
        }
        lastDiscoveredKeys = currentKeys

        val list = mutableListOf("Обнаруженные устройства в сети (${discoveredDevices.size})")
        val keys = mutableListOf("")

        for (k in currentKeys) {
            val name = discoveredDevices[k] ?: ""
            list.add("$name — $k")
            keys.add(k)
        }

        val currentSelectedKey = binding.editRemoteKey.text.toString().trim()
        val adapter = ArrayAdapter(requireContext(), android.R.layout.simple_spinner_dropdown_item, list)
        binding.spinnerDiscovered.adapter = adapter

        val existingIndex = keys.indexOf(currentSelectedKey)
        if (existingIndex > 0) {
            binding.spinnerDiscovered.setSelection(existingIndex, false)
        }

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
                    val clientIp = queryParams["client_ip"]
                    if (!clientIp.isNullOrEmpty()) {
                        val clientPort = queryParams["client_port"]?.toIntOrNull() ?: 28443
                        val rawClientName = queryParams["client_name"] ?: "Удалённое устройство"
                        val clientName = try { java.net.URLDecoder.decode(rawClientName, "UTF-8") } catch (_: Exception) { rawClientName }
                        remoteIp = clientIp
                        remotePort = clientPort
                        remoteDeviceName = clientName
                        isConnected = true
                        withContext(Dispatchers.Main) {
                            binding.textConnectionState.text = "🟢 Подключено: $clientName [$clientIp:$clientPort]"
                            binding.btnConnect.text = "Отключить"
                            scanLocalSaves()
                            fetchRemoteSaves()
                        }
                    }

                    val obj = JSONObject().apply {
                        put("status", "ok")
                        put("device_name", "${Build.MANUFACTURER} ${Build.MODEL}")
                        put("platform", "android")
                        put("version", "8.6.5")
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
        if ((ip == "127.0.0.1" || ip.equals("localhost", ignoreCase = true) || localIps.contains(ip) || ip == localIp) && port == localPort) {
            binding.textConnectionState.text = "⚠️ Введён ключ этого же устройства"
            Toast.makeText(requireContext(), "Вы указали ключ текущего устройства. Для синхронизации введите ключ другого устройства (ПК или другого смартфона).", Toast.LENGTH_LONG).show()
            return
        }

        remoteIp = ip
        remotePort = port
        binding.textConnectionState.text = "Подключение к $ip:$port..."

        viewLifecycleOwner.lifecycleScope.launch(Dispatchers.IO) {
            try {
                val myName = "${Build.MANUFACTURER} ${Build.MODEL}"
                val encodedName = java.net.URLEncoder.encode(myName, "UTF-8")
                val myIp = localIp ?: "127.0.0.1"
                val myKey = generateConnectionKey(myIp, localPort)
                val statusUrl = "http://$ip:$port/api/status?client_ip=$myIp&client_port=$localPort&client_name=$encodedName&client_key=$myKey"
                val req = Request.Builder()
                    .url(statusUrl)
                    .header("User-Agent", "STORM-SWITCH-SYNC/8.6.5")
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
                        binding.btnConnect.text = "Отключить"
                        fetchRemoteSaves()
                    }
                    return@launch
                }
            } catch (_: Exception) {}

            withContext(Dispatchers.Main) {
                isConnected = false
                binding.btnConnect.text = "Подключить"
                binding.textConnectionState.text = "❌ Ошибка подключения к $ip:$port"
                Toast.makeText(requireContext(), "Не удалось подключиться к $ip:$port. Убедитесь, что на втором устройстве запущен STORM SAVE SYNC и порт $port открыт.", Toast.LENGTH_LONG).show()
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
                            val rName = obj.optString("title_name", "")
                            val finalName = if (rName.isNotBlank() && !rName.equals(titleId, ignoreCase = true)) {
                                rName
                            } else {
                                resolveGameTitle(titleId)
                            }
                            AndroidSaveItem(titleId, finalName)
                        }
                        item.hasRemote = true
                        item.remoteTimestamp = obj.optLong("timestamp", 0L)
                        item.remoteDateStr = obj.optString("date_str", "")
                        item.remoteSizeBytes = obj.optLong("size_bytes", 0L)
                        item.remoteFileCount = obj.optInt("file_count", 0)
                        val remoteName = obj.optString("title_name", "")
                        if (remoteName.isNotBlank() && !remoteName.equals(titleId, ignoreCase = true)) {
                            item.titleName = remoteName
                        } else if (item.titleName.isEmpty() || item.titleName.equals(titleId, ignoreCase = true)) {
                            item.titleName = resolveGameTitle(titleId)
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

    private fun cleanGameTitle(raw: String): String {
        if (raw.isBlank()) return raw
        var t = raw
        t = t.replace(Regex("\\.(nsp|xci|nsz|xcz)$", RegexOption.IGNORE_CASE), "")
        t = t.replace(Regex("\\[[^\\]]*\\]"), "")
        t = t.replace(Regex("\\([^\\)]*\\)"), "")
        t = t.replace(Regex("\\b(v\\d+(\\.\\d+)*)\\b", RegexOption.IGNORE_CASE), "")
        t = t.replace('_', ' ')
        return t.replace(Regex("\\s+"), " ").trim()
    }

    private fun resolveGameTitle(titleId: String): String {
        val cleanTid = titleId.trim().uppercase(Locale.ROOT)

        // 1. Known Switch titles map (clean, verified canonical names)
        SWITCH_KNOWN_TITLES[cleanTid]?.let { return it }

        // 2. GameFixDatabase profile
        val fix = GameFixDatabase.getFix(cleanTid)
        if (fix != null && fix.gameName.isNotBlank()) {
            return fix.gameName
        }

        // 3. Storm World cached catalog
        try {
            val catalog = StormGamesWorldDialogFragment.getCachedCatalog(requireContext())
            catalog.firstOrNull {
                it.serialId.equals(cleanTid, ignoreCase = true)
            }?.let {
                if (it.finalTitle.isNotBlank()) return cleanGameTitle(it.finalTitle)
                if (it.title.isNotBlank()) return cleanGameTitle(it.title)
            }
        } catch (_: Exception) {}

        // 4. Installed game in library (cleaned of file brackets and tags)
        gamesViewModel.games.value.firstOrNull {
            it.programIdHex.equals(cleanTid, ignoreCase = true)
        }?.let {
            if (it.title.isNotBlank()) {
                val cleaned = cleanGameTitle(it.title)
                if (cleaned.isNotBlank()) return cleaned
            }
        }

        // 5. Default fallback
        return cleanTid
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
                    AndroidSaveItem(titleId, resolveGameTitle(titleId))
                }
                if (item.titleName.isEmpty() || item.titleName.equals(titleId, ignoreCase = true)) {
                    item.titleName = resolveGameTitle(titleId)
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
                    val sizeAndCountMatch = (item.localFileCount == item.remoteFileCount) &&
                            (item.localSizeBytes == item.remoteSizeBytes) &&
                            (item.localSizeBytes > 0L)
                    if (sizeAndCountMatch || (diff <= 180 && item.localSizeBytes == item.remoteSizeBytes)) {
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
        localIps.clear()
        val candidates = mutableListOf<Pair<String, Int>>()
        try {
            val interfaces = NetworkInterface.getNetworkInterfaces()
            while (interfaces.hasMoreElements()) {
                val iface = interfaces.nextElement()
                if (iface.isLoopback || !iface.isUp) continue
                val name = iface.name.lowercase(Locale.ROOT)
                val isVirtual = name.contains("dummy") || name.contains("docker") ||
                        name.contains("vbox") || name.contains("vmnet") || name.contains("p2p")

                val addresses = iface.inetAddresses
                while (addresses.hasMoreElements()) {
                    val addr = addresses.nextElement()
                    if (addr is Inet4Address && !addr.isLoopbackAddress) {
                        val host = addr.hostAddress ?: continue
                        localIps.add(host)
                        var score = 10
                        if (name.startsWith("wlan") || name.startsWith("wifi")) score += 100
                        else if (name.startsWith("eth")) score += 90
                        else if (name.startsWith("rmnet")) score += 50
                        else if (name.startsWith("tun") || name.startsWith("tap") || name.startsWith("tailscale")) score += 30

                        if (host.startsWith("192.168.")) score += 60
                        else if (host.startsWith("10.0.") || host.startsWith("10.")) score += 40
                        else if (host.startsWith("172.")) score += 35
                        else if (host.startsWith("100.")) score += 30

                        if (isVirtual) score -= 150
                        candidates.add(host to score)
                    }
                }
            }
        } catch (_: Exception) {}

        candidates.sortByDescending { it.second }
        return candidates.firstOrNull()?.first
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
        var key = rawKey.trim()
        if (key.startsWith("http://", ignoreCase = true)) {
            key = key.substring(7)
        } else if (key.startsWith("https://", ignoreCase = true)) {
            key = key.substring(8)
        }
        while (key.endsWith("/")) {
            key = key.substring(0, key.length - 1).trim()
        }
        if (key.isEmpty()) return null

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
            val lastColon = key.lastIndexOf(':')
            val host = key.substring(0, lastColon).trim()
            val portStr = key.substring(lastColon + 1).trim()
            val p = portStr.toIntOrNull() ?: 28443
            return Pair(host, p)
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

        val SWITCH_KNOWN_TITLES = mapOf(
            "01000B900D8B0000" to "Cadence of Hyrule: Crypt of the NecroDancer featuring The Legend of Zelda",
            "010015100B514000" to "Super Mario Bros. Wonder",
            "01001B300B9BE000" to "Diablo III: Eternal Collection",
            "010020D01AD24000" to "Animal Well",
            "010022201229A000" to "Super Robot Wars 30",
            "010026800E304000" to "Super Robot Wars X",
            "01002DA013484000" to "The Legend of Zelda: Skyward Sword HD",
            "01002EF01A316000" to "Brotato",
            "01002FC00412C000" to "Little Nightmares",
            "0100307018934000" to "Signalis",
            "010040502453E000" to "Vampire Crawlers",
            "010042D00D900000" to "LEGO Star Wars: The Skywalker Saga",
            "010044700DEB0000" to "Assassin’s Creed: The Rebel Collection",
            "010057901E9E6000" to "Underling Uprising",
            "010059D020C26000" to "Marvel Cosmic Invasion",
            "01005CF01E784000" to "Teenage Mutant Ninja Turtles: Splintered Fate",
            "01005EC01E6A4000" to "The Art of Dave the Diver",
            "010063301BD50000" to "Super Robot Wars Y",
            "01006560184E6000" to "Mortal Kombat 1",
            "010066101A55A000" to "Little Nightmares III",
            "0100670014482000" to "Assassin's Creed: The Ezio Collection",
            "01006BB00C6F0000" to "The Legend of Zelda: Link's Awakening",
            "01006C900CC60000" to "Super Robot Wars T",
            "0100726014352000" to "Diablo II: Resurrected",
            "01007EF00011E000" to "The Legend of Zelda: Breath of the Wild",
            "01007F600B134000" to "Assassin's Creed III: Remastered",
            "010089A0197E4000" to "Vampire Survivors",
            "01008BA02525A000" to "Dispatch",
            "01008CF01BAAC000" to "The Legend of Zelda: Echoes of Wisdom",
            "010093801237C000" to "Metroid Dread",
            "010094D023A28000" to "Drill Core",
            "010097100EDD6000" to "Little Nightmares II",
            "010097F018538000" to "Dave the Diver",
            "0100AC300919A000" to "Firewatch",
            "0100B11027658000" to "Defender of the Crown: The Legend Returns",
            "0100BAC01E57E000" to "Ys X: Nordics",
            "0100BDA01AABC000" to "Rift of the NecroDancer",
            "0100C6A0235D4000" to "Devil Jam",
            "0100CA400E300000" to "Super Robot Wars V",
            "0100CEA007D08000" to "Crypt of the NecroDancer",
            "0100D59022590000" to "Scott Pilgrim EX",
            "0100E65002BB8000" to "Stardew Valley",
            "0100EC9010258000" to "Streets of Rage 4",
            "0100F2200C984000" to "Mortal Kombat 11",
            "0100F2C0115B6000" to "The Legend of Zelda: Tears of the Kingdom",
            "0100000000010000" to "Super Mario Odyssey",
            "0100152000022000" to "Mario Kart 8 Deluxe",
            "01000320000CC000" to "Super Smash Bros. Ultimate",
            "01006F8002326000" to "Animal Crossing: New Horizons",
            "010041800C120000" to "Pokemon Sword",
            "01008DB008C2C000" to "Pokemon Shield",
            "0100ABF008968000" to "Pokemon Brilliant Diamond",
            "0100000011D90000" to "Pokemon Shining Pearl",
            "01001F5010DFA000" to "Pokemon Legends: Arceus",
            "01008C5014C52000" to "Pokemon Scarlet",
            "0100A39014C54000" to "Pokemon Violet",
            "0100258002EAE000" to "Luigi's Mansion 3",
            "010028600EBDA000" to "Super Mario 3D World + Bowser's Fury",
            "0100D870045B6000" to "Super Mario 3D All-Stars",
            "010040600C5CE000" to "Super Mario Party",
            "010021C000B36000" to "Splatoon 2",
            "0100C2500FC20000" to "Splatoon 3",
            "01005EE00CDDC000" to "Xenoblade Chronicles 2",
            "01008B3005A30000" to "Xenoblade Chronicles: Definitive Edition",
            "010074600E2A6000" to "Xenoblade Chronicles 3",
            "01001A8005ED6000" to "Fire Emblem: Three Houses",
            "0100C9C017C14000" to "Fire Emblem Engage",
            "01000A10041EA000" to "Kirby Star Allies",
            "010005C013280000" to "Kirby and the Forgotten Land",
            "0100B04011742000" to "Metroid Prime Remastered",
            "0100827003E92000" to "Bayonetta 2",
            "01004A4010F22000" to "Bayonetta 3",
            "010049900F546000" to "Hollow Knight",
            "010012F007A0C000" to "Hades",
            "0100559011740000" to "Persona 5 Royal",
            "010087700E340000" to "Persona 4 Golden",
            "010065701446C000" to "Persona 3 Portable",
            "01009100052C4000" to "Monster Hunter Rise",
            "0100C81014D88000" to "Sonic Frontiers",
            "010077000B410000" to "Crash Bandicoot N. Sane Trilogy",
            "0100D7700B0BE000" to "Spyro Reignited Trilogy",
            "01006F3009AE2000" to "The Witcher 3: Wild Hunt",
            "01003BC0000A0000" to "The Elder Scrolls V: Skyrim",
            "0100165003504000" to "DOOM",
            "010008F005BDE000" to "DOOM Eternal",
            "0100A250097F0000" to "Cuphead",
            "01003C700009C000" to "Minecraft",
            "0100B7D0022EE000" to "Mario + Rabbids Kingdom Battle",
            "010068D01440C000" to "Mario + Rabbids Sparks of Hope",
            "01005D100807A000" to "Donkey Kong Country: Tropical Freeze",
            "01007E3006DDA000" to "Captain Toad: Treasure Tracker",
            "0100490008A20000" to "Yoshi's Crafted World",
            "01008A600CA58000" to "Paper Mario: The Origami King",
            "01001C3018868000" to "Paper Mario: The Thousand-Year Door",
            "010078000BAE6000" to "Mario Golf: Super Rush",
            "0100B5B00D478000" to "Mario Tennis Aces",
            "010080F01358C000" to "Mario Strikers: Battle League",
            "01001B90145B8000" to "Advance Wars 1+2: Re-Boot Camp",
            "01003D200FAA2000" to "Pikmin 3 Deluxe",
            "0100B70012262000" to "Pikmin 4",
            "01007DA00755C000" to "Astral Chain",
            "0100D2800D5C0000" to "Shin Megami Tensei V",
            "0100235017260000" to "Shin Megami Tensei V: Vengeance",
            "01005C8009628000" to "Dragon Quest XI S",
            "0100B96013A02000" to "NieR:Automata The End of YoRHa Edition",
            "010003001886A000" to "Princess Peach: Showtime!",
            "0100DCA01C46E000" to "Super Mario Party Jamboree",
            "01004B201A996000" to "Mario and Luigi: Brothership"
        )
    }

    // Inner Adapter
    inner class SaveSyncAdapter(
        private var items: List<AndroidSaveItem>,
        private val onSyncClick: (AndroidSaveItem) -> Unit
    ) : RecyclerView.Adapter<SaveSyncAdapter.ViewHolder>() {

        inner class ViewHolder(val binding: ItemStormSaveSyncBinding) : RecyclerView.ViewHolder(binding.root)

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

            val tid = item.titleId.uppercase(Locale.ROOT)
            val finalTitle = if (item.titleName.isNotBlank() && !item.titleName.equals(item.titleId, ignoreCase = true)) {
                item.titleName
            } else {
                resolveGameTitle(tid)
            }

            b.textGameTitle.text = finalTitle
            b.textTitleId.text = "ID: $tid"

            // Load Game Icon
            val localGame = gamesViewModel.games.value.firstOrNull {
                it.programIdHex.equals(tid, ignoreCase = true)
            }
            if (localGame != null) {
                org.yuzu.yuzu_emu.utils.GameIconUtils.loadGameIcon(localGame, b.imageGameIcon)
            } else {
                val iconUrl = StormGamesWorldDialogFragment.SWITCH_CDN_ICONS[tid]
                    ?: if (tid.length == 16) "https://raw.githubusercontent.com/blawar/titledb/master/icons/$tid.jpg" else null
                if (!iconUrl.isNullOrBlank()) {
                    b.imageGameIcon.load(iconUrl) {
                        crossfade(true)
                        placeholder(R.drawable.ic_cartridge)
                        error(R.drawable.ic_cartridge)
                    }
                } else {
                    b.imageGameIcon.setImageResource(R.drawable.ic_cartridge)
                }
            }

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

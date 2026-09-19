// SPDX-FileCopyrightText: 2026 STORM SWITCH Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.utils

import okhttp3.Dns
import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.InetAddress
import java.net.URL
import java.net.UnknownHostException

/**
 * Robust DNS resolver for network requests.
 * Uses system DNS by default, but seamlessly falls back to static IP mapping
 * and DNS-over-HTTPS (DoH) if local ISP/cellular DNS fails or blocks resolution.
 */
object SmartDns : Dns {
    private val systemDns = Dns.SYSTEM

    // Static fallback IPs for critical mirrors and GitHub to guarantee resolution
    // even if local cellular DNS is completely poisoned, blocked or unreachable without VPN
    private val staticFallbackHosts = mapOf(
        "ghfast.top" to listOf("23.95.31.220", "107.175.190.200"),
        "gh-proxy.net" to listOf("37.48.77.81"),
        "ghproxy.net" to listOf("5.135.5.132"),
        "gh.con.sh" to listOf("172.66.45.36", "172.66.46.220"),
        "gh.llkk.cc" to listOf("104.21.35.157", "172.67.157.106"),
        "api.github.com" to listOf("140.82.121.6", "140.82.121.5", "140.82.112.6"),
        "github.com" to listOf("140.82.121.4", "140.82.121.3", "140.82.114.3"),
        "raw.githubusercontent.com" to listOf("185.199.108.133", "185.199.109.133", "185.199.110.133", "185.199.111.133"),
        "objects.githubusercontent.com" to listOf("185.199.108.133", "185.199.109.133", "185.199.110.133", "185.199.111.133")
    )

    override fun lookup(hostname: String): List<InetAddress> {
        // 1. Try system DNS first
        try {
            val addresses = systemDns.lookup(hostname)
            if (addresses.isNotEmpty()) {
                return addresses
            }
        } catch (_: UnknownHostException) {
            Log.info("[SmartDns] System DNS lookup failed for $hostname, trying fallback...")
        }

        // 2. Check static host table (zero network overhead, immune to DNS blocking)
        val staticIps = staticFallbackHosts[hostname.lowercase()]
        if (!staticIps.isNullOrEmpty()) {
            val resolved = staticIps.mapNotNull { ip ->
                try {
                    val addrBytes = InetAddress.getByName(ip).address
                    InetAddress.getByAddress(hostname, addrBytes)
                } catch (_: Throwable) {
                    null
                }
            }
            if (resolved.isNotEmpty()) {
                Log.info("[SmartDns] Resolved $hostname via static fallback: $resolved")
                return resolved
            }
        }

        // 3. Fallback to querying public DNS over HTTPS (DoH) via Google or Cloudflare
        val dohIps = queryDoH(hostname)
        if (dohIps.isNotEmpty()) {
            Log.info("[SmartDns] Resolved $hostname via DoH: $dohIps")
            return dohIps
        }

        throw UnknownHostException("Unable to resolve host \"$hostname\": No address associated with hostname")
    }

    private fun queryDoH(hostname: String): List<InetAddress> {
        val dohEndpoints = listOf(
            "https://dns.google/resolve?name=$hostname&type=A",
            "https://cloudflare-dns.com/dns-query?name=$hostname&type=A"
        )
        for (endpoint in dohEndpoints) {
            try {
                val url = URL(endpoint)
                val conn = url.openConnection() as HttpURLConnection
                conn.connectTimeout = 3000
                conn.readTimeout = 3000
                conn.setRequestProperty("Accept", "application/dns-json")
                conn.setRequestProperty("User-Agent", "Mozilla/5.0")
                if (conn.responseCode == 200) {
                    val jsonStr = conn.inputStream.bufferedReader().use { it.readText() }
                    val json = JSONObject(jsonStr)
                    val answers = json.optJSONArray("Answer")
                    if (answers != null && answers.length() > 0) {
                        val result = mutableListOf<InetAddress>()
                        for (i in 0 until answers.length()) {
                            val item = answers.getJSONObject(i)
                            val type = item.optInt("type", 0)
                            if (type == 1) { // A record
                                val ip = item.optString("data", "")
                                if (ip.isNotEmpty()) {
                                    val addrBytes = InetAddress.getByName(ip).address
                                    result.add(InetAddress.getByAddress(hostname, addrBytes))
                                }
                            }
                        }
                        if (result.isNotEmpty()) return result
                    }
                }
            } catch (_: Throwable) {
            }
        }
        return emptyList()
    }
}

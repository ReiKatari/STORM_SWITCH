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
 * Uses system DNS first. If system DNS fails or is blocked, queries DNS-over-HTTPS (DoH)
 * via Cloudflare (1.1.1.1) and Google before falling back to static IP mapping.
 */
object SmartDns : Dns {
    private val systemDns = Dns.SYSTEM

    // Static fallback IPs for hosts with stable unicast IPs.
    // NOTE: objects.githubusercontent.com and release-assets.githubusercontent.com use AWS CloudFront / Azure CDN
    // and must NEVER be mapped to static Fastly IPs.
    private val staticFallbackHosts = mapOf(
        "api.github.com" to listOf("140.82.121.6", "140.82.121.5", "140.82.112.6"),
        "github.com" to listOf("140.82.121.4", "140.82.121.3", "140.82.114.3"),
        "raw.githubusercontent.com" to listOf("185.199.108.133", "185.199.109.133", "185.199.110.133", "185.199.111.133"),
        "stormgamesworld.ru" to listOf("87.255.6.63")
    )

    override fun lookup(hostname: String): List<InetAddress> {
        // 1. Try system DNS first
        try {
            val addresses = systemDns.lookup(hostname)
            if (addresses.isNotEmpty()) {
                return addresses
            }
        } catch (_: UnknownHostException) {
            Log.info("[SmartDns] System DNS lookup failed for $hostname, trying DoH...")
        }

        // 2. Query public DNS-over-HTTPS (DoH) before any static table
        val dohIps = queryDoH(hostname)
        if (dohIps.isNotEmpty()) {
            Log.info("[SmartDns] Resolved $hostname via DoH: $dohIps")
            return dohIps
        }

        // 3. Fallback to static host table if DoH is blocked or unreachable
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

        throw UnknownHostException("Unable to resolve host \"$hostname\": No address associated with hostname")
    }

    private fun queryDoH(hostname: String): List<InetAddress> {
        val dohEndpoints = listOf(
            "https://1.1.1.1/dns-query?name=$hostname&type=A",
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
                conn.setRequestProperty("User-Agent", "STORM-SWITCH-Updater")
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

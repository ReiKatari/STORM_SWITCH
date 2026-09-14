// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.utils

import android.content.Context
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import okhttp3.OkHttpClient
import okhttp3.Request
import org.json.JSONArray
import org.json.JSONObject
import org.yuzu.yuzu_emu.NativeLibrary
import org.yuzu.yuzu_emu.YuzuApplication
import java.io.File
import java.io.FileOutputStream
import java.util.concurrent.TimeUnit
import kotlin.random.Random

data class AmiiboEntry(
    val name: String,
    val character: String,
    val gameSeries: String,
    val amiiboSeries: String,
    val type: String,
    val head: String,
    val tail: String,
    val imageUrl: String,
    val switchGames: List<String> = emptyList()
) {
    val fullId: String get() = "${head}${tail}".uppercase()
}

object AmiiboHelper {
    private const val USER_AGENT =
        "Mozilla/5.0 (Linux; Android 14; Mobile) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Mobile Safari/537.36 STORM-EDEN/4.0.0"

    private val httpClient: OkHttpClient by lazy {
        OkHttpClient.Builder()
            .followRedirects(true)
            .followSslRedirects(true)
            .connectTimeout(20, TimeUnit.SECONDS)
            .readTimeout(30, TimeUnit.SECONDS)
            .build()
    }

    private var cachedAmiibos: List<AmiiboEntry> = emptyList()
    var activeAmiiboName: String? = null

    suspend fun getAmiiboDatabase(forceRefresh: Boolean = false): List<AmiiboEntry> =
        withContext(Dispatchers.IO) {
            if (cachedAmiibos.isNotEmpty() && !forceRefresh) {
                return@withContext cachedAmiibos
            }

            val cacheFile = File(YuzuApplication.appContext.cacheDir, "amiibo_cache.json")
            if (cacheFile.exists() && !forceRefresh) {
                try {
                    val jsonText = cacheFile.readText()
                    val parsed = parseAmiiboJson(jsonText)
                    if (parsed.isNotEmpty()) {
                        cachedAmiibos = parsed
                        return@withContext parsed
                    }
                } catch (e: Exception) {
                    e.printStackTrace()
                }
            }

        val urls = listOf(
            "https://cdn.jsdelivr.net/gh/N3evin/AmiiboAPI@master/database/amiibo.json",
            "https://raw.githubusercontent.com/N3evin/AmiiboAPI/master/database/amiibo.json",
            "https://www.amiiboapi.com/api/amiibo/"
        )

            for (url in urls) {
                try {
                    val request = Request.Builder()
                        .url(url)
                        .header("User-Agent", USER_AGENT)
                        .header("Accept", "application/json, text/plain, */*")
                        .build()

                    val response = httpClient.newCall(request).execute()
                    if (response.isSuccessful) {
                        val bodyString = response.body?.string()
                        if (!bodyString.isNullOrEmpty()) {
                            val parsed = parseAmiiboJson(bodyString)
                            if (parsed.isNotEmpty()) {
                                try {
                                    cacheFile.writeText(bodyString)
                                } catch (_: Exception) {}
                                cachedAmiibos = parsed
                                return@withContext parsed
                            }
                        }
                    }
                } catch (e: Exception) {
                    e.printStackTrace()
                }
            }

            if (cacheFile.exists()) {
                try {
                    cachedAmiibos = parseAmiiboJson(cacheFile.readText())
                } catch (_: Exception) {}
            }

            return@withContext cachedAmiibos
        }

    private fun parseAmiiboJson(jsonText: String): List<AmiiboEntry> {
        val list = mutableListOf<AmiiboEntry>()
        try {
            val root = JSONObject(jsonText)
            val amiiboSeriesMap = root.optJSONObject("amiibo_series")
            val gameSeriesMap = root.optJSONObject("game_series")
            val typesMap = root.optJSONObject("types")
            val charactersMap = root.optJSONObject("characters")

            if (root.has("amiibo")) {
                val array = root.optJSONArray("amiibo") ?: JSONArray()
                for (i in 0 until array.length()) {
                    val obj = array.optJSONObject(i) ?: continue
                    parseAmiiboObject(obj, "", amiiboSeriesMap, gameSeriesMap, typesMap, charactersMap)?.let { list.add(it) }
                }
            } else if (root.has("amiibos")) {
                val amiibosObj = root.optJSONObject("amiibos")
                if (amiibosObj != null) {
                    val keys = amiibosObj.keys()
                    while (keys.hasNext()) {
                        val key = keys.next()
                        val obj = amiibosObj.optJSONObject(key) ?: continue
                        val entry = parseAmiiboObject(obj, key, amiiboSeriesMap, gameSeriesMap, typesMap, charactersMap)
                        if (entry != null) list.add(entry)
                    }
                } else {
                    val arr = root.optJSONArray("amiibos") ?: JSONArray()
                    for (i in 0 until arr.length()) {
                        val obj = arr.optJSONObject(i) ?: continue
                        parseAmiiboObject(obj, "", amiiboSeriesMap, gameSeriesMap, typesMap, charactersMap)?.let { list.add(it) }
                    }
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
        return list
    }

    private fun parseAmiiboObject(
        obj: JSONObject,
        keyFallback: String = "",
        amiiboSeriesMap: JSONObject? = null,
        gameSeriesMap: JSONObject? = null,
        typesMap: JSONObject? = null,
        charactersMap: JSONObject? = null
    ): AmiiboEntry? {
        val name = obj.optString("name", "").ifEmpty { obj.optString("character", "") }
        if (name.isEmpty()) return null

        var head = obj.optString("head", "")
        var tail = obj.optString("tail", "")

        var cleanKey = keyFallback.trim()
        if (cleanKey.startsWith("0x", ignoreCase = true)) {
            cleanKey = cleanKey.substring(2)
        }
        if (head.isEmpty() && cleanKey.length >= 16) {
            head = cleanKey.substring(0, 8)
            tail = cleanKey.substring(8, 16)
        }

        var amiiboSeries = obj.optString("amiiboSeries", "")
        if (amiiboSeries.isEmpty() && tail.length >= 4 && amiiboSeriesMap != null) {
            val seriesId = "0x" + tail.substring(2, 4).lowercase()
            amiiboSeries = amiiboSeriesMap.optString(seriesId, "")
        }
        if (amiiboSeries.isEmpty()) amiiboSeries = "Others"

        var type = obj.optString("type", "")
        if (type.isEmpty() && tail.length >= 8 && typesMap != null) {
            val typeId = "0x" + tail.substring(6, 8).lowercase()
            type = typesMap.optString(typeId, "")
        }
        if (type.isEmpty()) type = "Figure"

        var gameSeries = obj.optString("gameSeries", "")
        if (gameSeries.isEmpty() && head.length >= 3 && gameSeriesMap != null) {
            val gId = "0x" + head.substring(0, 3).lowercase()
            gameSeries = gameSeriesMap.optString(gId, "")
        }
        if (gameSeries.isEmpty()) gameSeries = "Nintendo"

        var character = obj.optString("character", "")
        if (character.isEmpty() && head.length >= 4 && charactersMap != null) {
            val cId = "0x" + head.substring(0, 4).lowercase()
            character = charactersMap.optString(cId, "")
        }
        if (character.isEmpty()) character = name

        var image = obj.optString("image", "")
        if (image.isEmpty() && head.isNotEmpty() && tail.isNotEmpty()) {
            image = "https://cdn.jsdelivr.net/gh/N3evin/AmiiboAPI@master/images/icon_${head.lowercase()}-${tail.lowercase()}.png"
        }

        val switchGamesList = mutableListOf<String>()
        val gamesSwitch = obj.optJSONArray("gamesSwitch")
        if (gamesSwitch != null && gamesSwitch.length() > 0) {
            for (i in 0 until gamesSwitch.length()) {
                val g = gamesSwitch.optJSONObject(i) ?: continue
                val gName = g.optString("gameName", "")
                if (gName.isNotEmpty()) {
                    val usages = g.optJSONArray("amiiboUsage")
                    val usageText = StringBuilder()
                    if (usages != null) {
                        for (j in 0 until usages.length()) {
                            val u = usages.optJSONObject(j) ?: continue
                            val uStr = u.optString("Usage", "")
                            if (uStr.isNotEmpty()) {
                                if (usageText.isNotEmpty()) usageText.append("; ")
                                usageText.append(uStr)
                            }
                        }
                    }
                    if (usageText.isNotEmpty()) {
                        switchGamesList.add("• $gName: $usageText")
                    } else {
                        switchGamesList.add("• $gName")
                    }
                }
            }
        }

        if (switchGamesList.isEmpty()) {
            if (gameSeries.contains("Zelda", ignoreCase = true) || amiiboSeries.contains("Zelda", ignoreCase = true)) {
                switchGamesList.add("• The Legend of Zelda: Tears of the Kingdom: Эксклюзивная ткань параплана, оружие, ресурсы")
                switchGamesList.add("• The Legend of Zelda: Breath of the Wild: Доспехи, оружие, призыв Эпоны / Волка Линка, сундуки")
                switchGamesList.add("• The Legend of Zelda: Echoes of Wisdom: Уникальные костюмы, аксессуары и ресурсы")
                switchGamesList.add("• Super Smash Bros. Ultimate: Обучаемый боец FP (Figure Player)")
                switchGamesList.add("• Mario Kart 8 Deluxe: Гоночный костюм Mii")
            } else if (gameSeries.contains("Mario", ignoreCase = true) || amiiboSeries.contains("Mario", ignoreCase = true)) {
                switchGamesList.add("• Super Mario Odyssey: Уникальные костюмы для Марио и подсказки Лун энергии")
                switchGamesList.add("• Super Mario 3D World + Bowser's Fury: Костюм Белого Тануки Неуязвимости, бонусы")
                switchGamesList.add("• Super Smash Bros. Ultimate: Боец FP с прокачкой 1-50 ур.")
                switchGamesList.add("• Mario Kart 8 Deluxe: Специальный гоночный костюм Mii")
            } else if (gameSeries.contains("Splatoon", ignoreCase = true) || amiiboSeries.contains("Splatoon", ignoreCase = true)) {
                switchGamesList.add("• Splatoon 3 / 2: Эксклюзивные наборы экипировки и совместные фотосессии")
            } else if (gameSeries.contains("Metroid", ignoreCase = true) || amiiboSeries.contains("Metroid", ignoreCase = true)) {
                switchGamesList.add("• Metroid Dread: Дополнительный контейнер энергии (Energy Tank) и ракеты")
            } else if (gameSeries.contains("Animal Crossing", ignoreCase = true) || amiiboSeries.contains("Animal Crossing", ignoreCase = true)) {
                switchGamesList.add("• Animal Crossing: New Horizons: Плакаты, визит жителя на кемпинг, фотостудия")
            } else if (gameSeries.contains("Monster Hunter", ignoreCase = true) || amiiboSeries.contains("Monster Hunter", ignoreCase = true)) {
                switchGamesList.add("• Monster Hunter Rise: Многослойная броня и ежедневная лотерея Кагари")
            } else if (gameSeries.contains("Xenoblade", ignoreCase = true) || amiiboSeries.contains("Xenoblade", ignoreCase = true)) {
                switchGamesList.add("• Xenoblade Chronicles 3: Облик Меча Монадо и ресурсы")
            } else {
                switchGamesList.add("• Super Smash Bros. Ultimate: Обучаемый боец FP или бонусы")
                switchGamesList.add("• Mario Kart 8 Deluxe: Гоночный костюм Mii")
                switchGamesList.add("• Универсальная поддержка: Совместимо со всеми Switch играми с поддержкой Amiibo")
            }
        }

        return AmiiboEntry(
            name = name,
            character = character,
            gameSeries = gameSeries,
            amiiboSeries = amiiboSeries,
            type = type,
            head = head,
            tail = tail,
            imageUrl = image,
            switchGames = switchGamesList
        )
    }

    fun generateAmiiboBin(entry: AmiiboEntry): ByteArray {
        val bin = ByteArray(540)

        // NTAG215 Header (UID + CRC checks conforming to ISO/IEC 14443-3)
        bin[0] = 0x04.toByte()
        bin[1] = Random.nextInt(0x01, 0xFE).toByte()
        bin[2] = Random.nextInt(0x01, 0xFE).toByte()
        bin[3] = (0x88 xor bin[0].toInt() xor bin[1].toInt() xor bin[2].toInt()).toByte()
        bin[4] = Random.nextInt(0x01, 0xFE).toByte()
        bin[5] = Random.nextInt(0x01, 0xFE).toByte()
        bin[6] = Random.nextInt(0x01, 0xFE).toByte()
        bin[7] = 0x00.toByte() // Nintendo ID
        bin[8] = (bin[4].toInt() xor bin[5].toInt() xor bin[6].toInt() xor bin[7].toInt()).toByte()

        // Internal bytes
        bin[9] = 0x48.toByte()

        // Static Lock: 0xE00F (little-endian)
        bin[10] = 0x0F.toByte()
        bin[11] = 0xE0.toByte()

        // Capability Container (CC): 0xEEFF10F1 (little-endian)
        bin[12] = 0xF1.toByte()
        bin[13] = 0x10.toByte()
        bin[14] = 0xFF.toByte()
        bin[15] = 0xEE.toByte()

        // User memory: EncryptedAmiiboFile at offset 0x10
        bin[0x10] = 0xA5.toByte() // Constant value required by Nintendo Amiibo (0xA5)
        bin[0x11] = 0x00.toByte() // Write counter high
        bin[0x12] = 0x01.toByte() // Write counter low
        bin[0x13] = 0x00.toByte() // Amiibo version
        bin[0x14] = 0x10.toByte() // Settings: amiibo_initialized = 1 (bit 4)

        // Nickname UTF-16BE at offset 0x20 (up to 10 characters)
        val cleanName = entry.name.take(10)
        for (i in cleanName.indices) {
            val code = cleanName[i].code
            bin[0x20 + i * 2] = ((code ushr 8) and 0xFF).toByte()
            bin[0x20 + i * 2 + 1] = (code and 0xFF).toByte()
        }

        // Model info at offset 0x54 (12 bytes)
        try {
            val headHex = entry.head.padStart(8, '0')
            val tailHex = entry.tail.padStart(8, '0')

            bin[0x54] = headHex.substring(0, 2).toInt(16).toByte()
            bin[0x55] = headHex.substring(2, 4).toInt(16).toByte()
            bin[0x56] = headHex.substring(4, 6).toInt(16).toByte()
            bin[0x57] = headHex.substring(6, 8).toInt(16).toByte()

            bin[0x58] = tailHex.substring(0, 2).toInt(16).toByte()
            bin[0x59] = tailHex.substring(2, 4).toInt(16).toByte()
            bin[0x5A] = tailHex.substring(4, 6).toInt(16).toByte()
            bin[0x5B] = 0x02.toByte() // tag_type: PackedTagType::Type2 (0x02) required by IsAmiiboValid
        } catch (_: Exception) {
            bin[0x5B] = 0x02.toByte()
        }

        // Dynamic Lock at offset 0x208: 0x0F0001 (little-endian)
        bin[0x208] = 0x01.toByte()
        bin[0x209] = 0x00.toByte()
        bin[0x20A] = 0x0F.toByte()
        bin[0x20B] = 0x00.toByte()

        // CFG0 at offset 0x20C: 0x04000000 (little-endian)
        bin[0x20C] = 0x00.toByte()
        bin[0x20D] = 0x00.toByte()
        bin[0x20E] = 0x00.toByte()
        bin[0x20F] = 0x04.toByte()

        // CFG1 at offset 0x210: 0x5F (little-endian)
        bin[0x210] = 0x5F.toByte()
        bin[0x211] = 0x00.toByte()
        bin[0x212] = 0x00.toByte()
        bin[0x213] = 0x00.toByte()

        return bin
    }

    fun saveAmiiboToStorage(entry: AmiiboEntry): File {
        val userDir = File(DirectoryInitialization.userDirectory, "amiibo")
        userDir.mkdirs()

        val safeName = entry.name.replace(Regex("[^a-zA-Z0-9._ -]"), "_").trim()
        val fileName = if (safeName.isNotEmpty()) "$safeName.bin" else "Amiibo_${entry.fullId}.bin"
        val targetFile = File(userDir, fileName)

        val bytes = generateAmiiboBin(entry)
        FileOutputStream(targetFile).use { it.write(bytes) }
        return targetFile
    }

    data class AmiiboRewardInfo(
        val category: String,
        val icon: String,
        val title: String,
        val description: String
    ) {
        val fullFormatted: String get() = "[$icon $category] $title"
    }

    fun getRewardForGame(entry: AmiiboEntry, gameTitle: String): AmiiboRewardInfo {
        val n = entry.name.lowercase()
        val c = entry.character.lowercase()
        val gs = entry.gameSeries.lowercase()
        val aser = entry.amiiboSeries.lowercase()
        val gn = gameTitle.lowercase()

        // 1. Zelda: Tears of the Kingdom
        if (gn.contains("tears") || gn.contains("totk")) {
            if (n.contains("tears") || (c.contains("link") && n.contains("totk"))) {
                return AmiiboRewardInfo("Ткань параплана и оружие", "🪂", "Новая ткань чемпионов и Меч рыцаря", "Эксклюзивная ткань для параплана «Новая туника чемпионов», крепкий Меч рыцаря, целебные травы и мясо")
            }
            if (n.contains("ocarina") || n.contains("time")) {
                return AmiiboRewardInfo("Оружие, броня и ткань", "🗡️", "Меч Большого Горона и Сет Времени", "Двуручный Меч Большого Горона, Шапка/Туника/Штаны Времени, Ткань маски Лони-Лони")
            }
            if (n.contains("majora")) {
                return AmiiboRewardInfo("Оружие, броня и ткань", "🗡️", "Двуручник и Сет Свирепого Божества", "Маска, Нагрудник и Поножи Свирепого Божества, Двуручник Свирепого Божества, Ткань маски Маджоры")
            }
            if (n.contains("twilight") || (c.contains("link") && aser.contains("smash"))) {
                return AmiiboRewardInfo("Скакун, броня и ткань", "🐺", "Легендарная кобыла Эпона и Сет Сумерек", "Призыв легендарной лошади Эпоны (максимальные характеристики), Шапка/Туника/Штаны Сумерек, Сумеречная ткань")
            }
            if (n.contains("skyward")) {
                return AmiiboRewardInfo("Оружие, броня и ткань", "🗡️", "Белый меч Небес и Сет Неба", "Белый меч Богини, Шапка/Туника/Штаны Неба, ткань Меча Небес")
            }
            if (n.contains("awakening")) {
                return AmiiboRewardInfo("Броня и ткань", "🛡️", "Сет Пробуждения и Ткань Яйца", "Мультяшная маска и туника Пробуждения, эксклюзивная ткань Яйца Ветрорыба, солдатские стрелы")
            }
            if (n.contains("archer")) {
                return AmiiboRewardInfo("Оружие и ткань", "🏹", "Ткань капюшона лучника и Королевский лук", "Эксклюзивная ткань лучника, Королевский лук, сырая дичь и редкая рыба")
            }
            if (n.contains("rider")) {
                return AmiiboRewardInfo("Ткань и снаряжение", "🪂", "Ткань капюшона всадника и Солдатский палаш", "Ткань всадника, оружие всадника, целебные грибы")
            }
            if (c.contains("wolf link") || n.contains("wolf")) {
                return AmiiboRewardInfo("Охотничьи ресурсы", "📦", "Охотничий провиант Линка", "Огромный запас свежего мяса, птицы и дичи для кулинарии")
            }
            if (c.contains("zelda")) {
                return AmiiboRewardInfo("Ткань и оружие", "🪂", "Ткань принцессы Зельды и Королевский лук", "Эксклюзивная ткань принцессы Зельды, древний или королевский лук, драгоценные камни и травы")
            }
            if (c.contains("ganondorf") || n.contains("ganon")) {
                return AmiiboRewardInfo("Оружие и ткань", "🗡️", "Меч и Ткань Демонического Короля", "Эксклюзивная ткань Демонического Короля, Меч Демонического Короля / Меч сумерек, запеченное мясо и самоцветы")
            }
            if (c.contains("sheik")) {
                return AmiiboRewardInfo("Броня и ткань", "🛡️", "Маска Шейха и Ткань клана Шеика", "Маска Шейха (бонус к скрытности), Ткань клана Шеика, ножи и щиты")
            }
            if (c.contains("guardian")) {
                return AmiiboRewardInfo("Ткань и механизмы", "🪂", "Древняя ткань Шеика и Древние стрелы", "Древняя ткань стража, металлические ящики, древние механизмы и древние клинки")
            }
            if (c.contains("bokoblin")) {
                return AmiiboRewardInfo("Ткань и оружие", "🪂", "Ткань Бокоблина и Боко-щиты", "Ткань Бокоблина, шипастый боко-щит, дубина и сырое мясо")
            }
            if (c.contains("daruk") || c.contains("mipha") || c.contains("revali") || c.contains("urbosa")) {
                return AmiiboRewardInfo("Божественный шлем и ткань", "🛡️", "Шлем Божественного чудища и Ткань чемпиона", "Уникальный Божественный шлем со стихийной защитой и персональная ткань чемпиона")
            }
            if (gs.contains("zelda") || aser.contains("zelda")) {
                return AmiiboRewardInfo("Оружие и ткань", "🗡️", "Тематическая ткань параплана и оружие", "Эксклюзивная ткань параплана, сундук с высокоуровневым оружием и самоцветами")
            }
            return AmiiboRewardInfo("Универсальные ресурсы", "📦", "Припасы путешественника", "Сундук со случайным оружием, стрелы, целебные травы, яблоки, мясо и рыба")
        }

        // 2. Zelda: Breath of the Wild
        if (gn.contains("breath") || gn.contains("botw")) {
            if (c.contains("wolf link") || n.contains("wolf")) {
                return AmiiboRewardInfo("Компаньон", "🐺", "Призыв Волка Линка (Wolf Link)", "Волк Линк появляется в мире и сражается на вашей стороне с 20 сердцами здоровья!")
            }
            if (n.contains("twilight") || (c.contains("link") && aser.contains("smash"))) {
                return AmiiboRewardInfo("Скакун и броня", "🐺", "Кобыла Эпона и Сет Сумерек", "Призыв легендарной лошади Эпоны с максимальными характеристиками и Набор Сумерек")
            }
            if (n.contains("majora")) {
                return AmiiboRewardInfo("Оружие и броня", "🗡️", "Двуручник и Сет Свирепого Божества", "Маска, Доспех и Поножи Свирепого Божества, Двуручник Свирепого Божества")
            }
            if (n.contains("ocarina") || n.contains("time")) {
                return AmiiboRewardInfo("Оружие и броня", "🗡️", "Меч Большого Горона и Сет Времени", "Шапка/Туника/Штаны Времени, Меч Большого Горона")
            }
            if (n.contains("archer")) {
                return AmiiboRewardInfo("Оружие", "🏹", "Лук Путешественника и особые стрелы", "Древние, ледяные, огненные и электрические стрелы, редкое мясо")
            }
            if (c.contains("zelda")) {
                return AmiiboRewardInfo("Щит и самоцветы", "🛡️", "Щит Бригадира и Звездный осколок", "Щит Бригадира, редкие рубины, сапфиры, алмазы и целебные травы")
            }
            if (c.contains("daruk") || c.contains("mipha") || c.contains("revali") || c.contains("urbosa")) {
                return AmiiboRewardInfo("Броня", "🛡️", "Шлем Божественного чудища", "Шлемы Ва-Рудания, Ва-Рута, Ва-Медо или Ва-Наборис со стихийной защитой")
            }
            if (gs.contains("zelda") || aser.contains("zelda")) {
                return AmiiboRewardInfo("Оружие и сундук", "🗡️", "Уникальный сундук Хайрула", "Сундук с редким оружием, стрелами и драгоценными металлами")
            }
            return AmiiboRewardInfo("Ресурсы", "📦", "Припасы и сырье", "Сундук с базовым оружием, мясо, грибы, травы и овощи")
        }

        // 3. Zelda: Echoes of Wisdom
        if (gn.contains("echoes of wisdom") || gn.contains("wisdom")) {
            if (c.contains("zelda") || n.contains("zelda")) {
                return AmiiboRewardInfo("Костюм", "👗", "Особое платье принцессы Зельды", "Эксклюзивный наряд принцессы для путешествия по Хайрулу, Камни Силы и редкие смузи ингредиенты")
            }
            if (c.contains("link") || n.contains("link")) {
                return AmiiboRewardInfo("Костюм", "👕", "Одеяния Героя", "Эксклюзивный зелёный наряд Героя Мечника, целебные травы и ценные кристаллы эхо")
            }
            if (c.contains("cat") || n.contains("cat") || n.contains("mario")) {
                return AmiiboRewardInfo("Костюм", "🐱", "Костюм Чёрной кошки", "Уникальный наряд Чёрной кошки, позволяющий общаться с котиками Хайрула!")
            }
            return AmiiboRewardInfo("Ресурсы", "📦", "Набор кристаллов и смузи", "Набор редких ингредиентов для приготовления смузи, яблоки, масло и Камни Силы")
        }

        // 4. Super Mario Odyssey
        if (gn.contains("odyssey") || gn.contains("mario odyssey")) {
            if (n.contains("wedding") && c.contains("mario")) {
                return AmiiboRewardInfo("Костюм и усиление", "👕", "Свадебный смокинг Марио и Неуязвимость", "Свадебный цилиндр и смокинг; Неуязвимость Марио ко всем видам урона на 30 секунд!")
            }
            if (n.contains("wedding") && c.contains("bowser")) {
                return AmiiboRewardInfo("Костюм и подсказки", "👕", "Свадебный смокинг Боузера и Локатор монет", "Свадебный цилиндр Боузера; Подсветка всех региональных фиолетовых монет на карте!")
            }
            if (n.contains("wedding") && c.contains("peach")) {
                return AmiiboRewardInfo("Костюм и здоровье", "👗", "Свадебное свадебное платье Пич и Сердце жизни", "Свадебная фата и платье Пич; Полное восстановление здоровья и Сердце Жизни (+3 HP)")
            }
            if (c.contains("mario")) {
                return AmiiboRewardInfo("Костюм", "👕", "Классический костюм Марио", "Классическая кепка и комбинезон Марио, временная неуязвимость")
            }
            if (c.contains("luigi")) {
                return AmiiboRewardInfo("Костюм", "👕", "Костюм Луиджи", "Кепка и комбинезон Луиджи для Марио")
            }
            if (c.contains("wario") || c.contains("waluigi")) {
                return AmiiboRewardInfo("Костюм", "👕", "Костюм Варио / Валуиджи", "Костюмы Варио и Валуиджи со шляпами")
            }
            return AmiiboRewardInfo("Подсказка дяди Амибо", "⭐", "Поиск Луны энергии", "Дядя Амибо отмечает на карте точное местоположение скрытой Луны энергии!")
        }

        // 5. Super Mario 3D World / Bowser's Fury
        if (gn.contains("3d world") || gn.contains("bowser's fury") || gn.contains("bowsers fury")) {
            if (n.contains("cat mario") || (c.contains("mario") && n.contains("cat"))) {
                return AmiiboRewardInfo("Усилитель", "🌟", "Белый Кот Марио (Неуязвимость)", "Превращает Марио в Белого Кота Марио с постоянной неуязвимостью к врагам!")
            }
            if (n.contains("cat peach") || (c.contains("peach") && n.contains("cat"))) {
                return AmiiboRewardInfo("Бонус", "🐱", "Случайный супер-усилитель", "Дарует случайный редкий бонус: Супер-Колокольчик, Бумеранг или Лист Тануки")
            }
            if (c.contains("bowser")) {
                return AmiiboRewardInfo("Событие", "🔥", "Мгновенное пробуждение Яростного Боузера", "Немедленно активирует Ярость Боузера и шторм на озере Кошачьих Лап")
            }
            if (c.contains("bowser jr")) {
                return AmiiboRewardInfo("Ударная волна", "💥", "Мощная взрывная волна Боузера-младшего", "Уничтожает всех ближайших врагов и открывает секретные блоки")
            }
            return AmiiboRewardInfo("Бонус", "🍄", "Гриб 1-Up или Супер-Гриб", "Дарует дополнительную жизнь или увеличение персонажа")
        }

        // 6. Metroid Dread
        if (gn.contains("metroid") || gn.contains("dread")) {
            if (c.contains("samus") && (n.contains("dread") || aser.contains("metroid"))) {
                return AmiiboRewardInfo("Контейнер энергии", "🔋", "Дополнительный бак энергии (+100 HP)", "Навсегда увеличивает максимальный запас здоровья Самус на один бак (+100 HP) и восстанавливает энергию раз в день")
            }
            if (c.contains("emmi") || n.contains("e.m.m.i") || n.contains("emmi")) {
                return AmiiboRewardInfo("Ракетный комплекс", "🚀", "Дополнительный контейнер ракет (+10)", "Навсегда увеличивает максимальный боезапас ракет Самус на +10 и восполняет ракеты раз в день")
            }
            return AmiiboRewardInfo("Пополнение", "⚡", "Восстановление ресурсов Самус", "Экстренное пополнение запаса здоровья или ракет во время миссии")
        }

        // 7. Splatoon 3 / Splatoon 2
        if (gn.contains("splatoon")) {
            if (aser.contains("splatoon") || gs.contains("splatoon")) {
                return AmiiboRewardInfo("Экипировка", "👕", "Эксклюзивный комплект одежды и обуви", "Уникальный 3-предметный набор брендовой экипировки с эксклюзивными способностями и фотосессия")
            }
            return AmiiboRewardInfo("Бонус", "🎨", "Специальные сувениры и снимки", "Совместные снимки в фотобудке Инкополиса")
        }

        // 8. Monster Hunter Rise
        if (gn.contains("monster hunter") || gn.contains("rise") || gn.contains("sunbreak")) {
            if (gs.contains("monster hunter") || n.contains("magnamalo") || n.contains("palamute") || n.contains("palico") || n.contains("malzeno")) {
                return AmiiboRewardInfo("Многослойная броня", "🛡️", "Особая броня Синра / Малзено", "Эксклюзивный комплект многослойной брони охотника, Палико или Паламута, а также участие в лотерее Кагари")
            }
            return AmiiboRewardInfo("Лотерея", "🎲", "Ежедневный лотерейный билет Кагари", "Бесплатная лотерея у торговца Кагари с ценными зельями, ловушками и сферами доспехов")
        }

        // 9. Xenoblade Chronicles 3
        if (gn.contains("xenoblade")) {
            if (c.contains("shulk")) {
                return AmiiboRewardInfo("Облик оружия", "⚔️", "Легендарный меч Монадо", "Разблокирует легендарный внешний облик клинка Меч Монадо для мечника Ноа!")
            }
            if (n.contains("noah") || n.contains("mio")) {
                return AmiiboRewardInfo("Костюм", "👕", "Особый наряд для спутников", "Уникальная куртка Ноа или наряд Мио для всех членов отряда")
            }
            return AmiiboRewardInfo("Предметы", "📦", "Коллекционные предметы и монеты Нопонов", "Набор редких коллекционных предметов и золотые/серебряные монеты Нопонов")
        }

        // 10. Super Smash Bros. Ultimate
        if (gn.contains("smash") || gn.contains("ssbu")) {
            return AmiiboRewardInfo("Боец FP", "🥊", "Figure Player: Обучаемый боец FP", "Создает уникального бойца FP (1-50 ур.), обучающегося вашему боевому стилю и тактике!")
        }

        // 11. Mario Kart 8 Deluxe
        if (gn.contains("mario kart") || gn.contains("mk8")) {
            return AmiiboRewardInfo("Гоночный костюм", "🏎️", "Гоночный костюм Mii", "Разблокирует уникальный тематический комбинезон и гоночный шлем персонажа для гонщика Mii!")
        }

        // 12. Animal Crossing: New Horizons
        if (gn.contains("animal crossing") || gn.contains("horizons")) {
            return AmiiboRewardInfo("Кемпинг и фото", "🏕️", "Визит на кемпинг и постер жителя", "Приглашает жителя на кемпинг острова, разблокирует памятный постер и фотосессию на острове Харви")
        }

        // 13. Fire Emblem
        if (gn.contains("fire emblem") || gn.contains("engage") || gn.contains("three houses")) {
            return AmiiboRewardInfo("Музыка и костюм", "🎶", "Саундтрек персонажа и билеты беседки", "Разблокирует классический музыкальный трек персонажа, купоны нарядов и ценные печати призыва")
        }

        // 14. Kirby
        if (gn.contains("kirby")) {
            return AmiiboRewardInfo("Усилитель", "⭐", "Особая способность и здоровье", "Дарует мощную способность копирования, звездочки и восстанавливает шкалу здоровья Кирби")
        }

        // 15. Pokemon
        if (gn.contains("pokemon") || gn.contains("pokémon")) {
            return AmiiboRewardInfo("Награда", "🎒", "Припасы тренера и ягоды", "Набор полезных ягод, покеболов и предметов для тренировки покемонов")
        }

        // Universal Fallback
        return AmiiboRewardInfo("Награда", "🎁", "Ежедневный подарочный набор", "Ежедневный набор полезных внутриигровых ресурсов, монет и расходных материалов")
    }

    fun getAmiibosForGame(allAmiibos: List<AmiiboEntry>, titleId: String, gameTitle: String): List<AmiiboEntry> {
        val gameLower = gameTitle.lowercase()
        val cleanTid = titleId.uppercase()
        return allAmiibos.filter { a ->
            when {
                gameLower.contains("zelda") || cleanTid in setOf("01007EF00011E000", "0100F2C0115B6000", "01006BB00C6F0000", "01002DA013484000", "01008CF01BAAC000") ->
                    a.gameSeries.contains("zelda", ignoreCase = true) || a.amiiboSeries.contains("zelda", ignoreCase = true)
                gameLower.contains("mario") || cleanTid in setOf("0100000000010000", "0100152000022000", "010015100B514000", "010028600EBDA000", "0100746002A76000") ->
                    a.gameSeries.contains("mario", ignoreCase = true) || a.amiiboSeries.contains("mario", ignoreCase = true)
                gameLower.contains("smash") || gameLower.contains("ssbu") || cleanTid in setOf("0100030003C60000", "01006A800016E000") ->
                    true
                gameLower.contains("splatoon") || cleanTid in setOf("0100B9300DC88000", "0100C2500FC20000") ->
                    a.gameSeries.contains("splatoon", ignoreCase = true) || a.amiiboSeries.contains("splatoon", ignoreCase = true)
                gameLower.contains("metroid") || cleanTid in setOf("010093801237C000", "0100559011740000") ->
                    a.gameSeries.contains("metroid", ignoreCase = true) || a.amiiboSeries.contains("metroid", ignoreCase = true)
                gameLower.contains("pokemon") || cleanTid in setOf("0100ABF008968000", "01008DB008C2C000", "0100A3D008C5C000", "01008F5008C5E000", "01001F5010DFA000") ->
                    a.gameSeries.contains("pokemon", ignoreCase = true) || a.amiiboSeries.contains("pokemon", ignoreCase = true)
                gameLower.contains("kirby") || cleanTid in setOf("010049900F546000", "010034400EBDA000") ->
                    a.gameSeries.contains("kirby", ignoreCase = true) || a.amiiboSeries.contains("kirby", ignoreCase = true)
                gameLower.contains("fire emblem") || cleanTid in setOf("01003D200E9C2000", "0100C960117E2000") ->
                    a.gameSeries.contains("fire emblem", ignoreCase = true) || a.amiiboSeries.contains("fire emblem", ignoreCase = true)
                gameLower.contains("xenoblade") || cleanTid in setOf("01008F6008C5E000", "0100E95004038000", "010074F013262000") ->
                    a.gameSeries.contains("xenoblade", ignoreCase = true) || a.amiiboSeries.contains("xenoblade", ignoreCase = true)
                gameLower.contains("monster hunter") || cleanTid in setOf("010033B00C30A000", "0100827014606000") ->
                    a.gameSeries.contains("monster hunter", ignoreCase = true) || a.amiiboSeries.contains("monster hunter", ignoreCase = true)
                gameLower.contains("animal crossing") || cleanTid in setOf("01006F8002326000") ->
                    a.gameSeries.contains("animal crossing", ignoreCase = true) || a.amiiboSeries.contains("animal crossing", ignoreCase = true)
                else -> a.amiiboSeries.contains("Smash", ignoreCase = true) || a.amiiboSeries.contains("Zelda", ignoreCase = true) || a.amiiboSeries.contains("Mario", ignoreCase = true)
            }
        }
    }

    fun isAmiiboSaved(entry: AmiiboEntry): Boolean {
        val userDir = File(DirectoryInitialization.userDirectory, "amiibo")
        val safeName = entry.name.replace(Regex("[^a-zA-Z0-9._ -]"), "_").trim()
        val fileName = if (safeName.isNotEmpty()) "$safeName.bin" else "Amiibo_${entry.fullId}.bin"
        return File(userDir, fileName).exists()
    }

    fun loadAmiiboDirectly(entry: AmiiboEntry): Boolean {
        val bytes = generateAmiiboBin(entry)
        val result = NativeLibrary.loadAmiibo(bytes)
        return result == 0
    }

    val imageMemoryCache = androidx.collection.LruCache<String, android.graphics.Bitmap>(300)
    private val fastImageClient = OkHttpClient.Builder()
        .connectTimeout(2, java.util.concurrent.TimeUnit.SECONDS)
        .readTimeout(3, java.util.concurrent.TimeUnit.SECONDS)
        .build()

    suspend fun getAmiiboImage(url: String): android.graphics.Bitmap? = withContext(Dispatchers.IO) {
        if (url.isEmpty()) return@withContext null

        imageMemoryCache.get(url)?.let { return@withContext it }

        val cacheDir = File(YuzuApplication.appContext.cacheDir, "amiibo_images")
        if (!cacheDir.exists()) cacheDir.mkdirs()
        val filename = url.substringAfterLast("/", "img.png").ifEmpty { "img.png" }
        val localFile = File(cacheDir, filename)

        if (localFile.exists() && localFile.length() > 0) {
            try {
                val bmp = android.graphics.BitmapFactory.decodeFile(localFile.absolutePath)
                if (bmp != null) {
                    imageMemoryCache.put(url, bmp)
                    return@withContext bmp
                }
            } catch (_: Exception) {}
        }

        val mirrorUrls = listOf(
            url,
            url.replace("cdn.jsdelivr.net/gh/N3evin/AmiiboAPI@master", "fastly.jsdelivr.net/gh/N3evin/AmiiboAPI@master"),
            url.replace("cdn.jsdelivr.net/gh/N3evin/AmiiboAPI@master", "raw.githubusercontent.com/N3evin/AmiiboAPI/master"),
            url.replace("cdn.jsdelivr.net/gh/N3evin/AmiiboAPI@master", "ghproxy.net/https://raw.githubusercontent.com/N3evin/AmiiboAPI/master"),
            url.replace("raw.githubusercontent.com/N3evin/AmiiboAPI/master", "fastly.jsdelivr.net/gh/N3evin/AmiiboAPI@master")
        ).distinct()

        for (mirror in mirrorUrls) {
            try {
                val req = Request.Builder()
                    .url(mirror)
                    .header("User-Agent", USER_AGENT)
                    .build()
                val resp = fastImageClient.newCall(req).execute()
                if (resp.isSuccessful) {
                    val bytes = resp.body?.bytes()
                    if (bytes != null && bytes.isNotEmpty()) {
                        try { localFile.writeBytes(bytes) } catch (_: Exception) {}
                        val bmp = android.graphics.BitmapFactory.decodeByteArray(bytes, 0, bytes.size)
                        if (bmp != null) {
                            imageMemoryCache.put(url, bmp)
                            return@withContext bmp
                        }
                    }
                }
            } catch (_: Exception) {}
        }
        null
    }
}

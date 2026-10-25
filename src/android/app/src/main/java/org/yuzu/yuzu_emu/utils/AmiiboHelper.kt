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
        "Mozilla/5.0 (Linux; Android 14; Mobile) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Mobile Safari/537.36 StormSwitch/4.8.8"

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
            val isRu = java.util.Locale.getDefault().language in setOf("ru", "uk", "be")
            if (gameSeries.contains("Zelda", ignoreCase = true) || amiiboSeries.contains("Zelda", ignoreCase = true)) {
                switchGamesList.add(if (isRu) "• The Legend of Zelda: Tears of the Kingdom: Эксклюзивная ткань параплана, оружие, ресурсы" else "• The Legend of Zelda: Tears of the Kingdom: Exclusive Paraglider fabric, weapons, materials")
                switchGamesList.add(if (isRu) "• The Legend of Zelda: Breath of the Wild: Доспехи, оружие, призыв Эпоны / Волка Линка, сундуки" else "• The Legend of Zelda: Breath of the Wild: Armor, weapons, summon Epona / Wolf Link, chests")
                switchGamesList.add(if (isRu) "• The Legend of Zelda: Echoes of Wisdom: Уникальные костюмы, аксессуары и ресурсы" else "• The Legend of Zelda: Echoes of Wisdom: Unique outfits, accessories, and materials")
                switchGamesList.add(if (isRu) "• Super Smash Bros. Ultimate: Обучаемый боец FP (Figure Player)" else "• Super Smash Bros. Ultimate: Trainable FP (Figure Player) fighter")
                switchGamesList.add(if (isRu) "• Mario Kart 8 Deluxe: Гоночный костюм Mii" else "• Mario Kart 8 Deluxe: Mii Racing Suit")
            } else if (gameSeries.contains("Mario", ignoreCase = true) || amiiboSeries.contains("Mario", ignoreCase = true)) {
                switchGamesList.add(if (isRu) "• Super Mario Odyssey: Уникальные костюмы для Марио и подсказки Лун энергии" else "• Super Mario Odyssey: Special Mario outfits and Power Moon hints")
                switchGamesList.add(if (isRu) "• Super Mario 3D World + Bowser's Fury: Костюм Белого Тануки Неуязвимости, бонусы" else "• Super Mario 3D World + Bowser's Fury: White Tanooki Invincibility power-ups and bonuses")
                switchGamesList.add(if (isRu) "• Super Smash Bros. Ultimate: Боец FP с прокачкой 1-50 ур." else "• Super Smash Bros. Ultimate: FP fighter with Lv. 1-50 progression")
                switchGamesList.add(if (isRu) "• Mario Kart 8 Deluxe: Специальный гоночный костюм Mii" else "• Mario Kart 8 Deluxe: Special Mii Racing Suit")
            } else if (gameSeries.contains("Splatoon", ignoreCase = true) || amiiboSeries.contains("Splatoon", ignoreCase = true)) {
                switchGamesList.add(if (isRu) "• Splatoon 3 / 2: Эксклюзивные наборы экипировки и совместные фотосессии" else "• Splatoon 3 / 2: Exclusive gear sets and photo shoots")
            } else if (gameSeries.contains("Metroid", ignoreCase = true) || amiiboSeries.contains("Metroid", ignoreCase = true)) {
                switchGamesList.add(if (isRu) "• Metroid Dread: Дополнительный контейнер энергии (Energy Tank) и ракеты" else "• Metroid Dread: Extra Energy Tank and Missile+ Tank")
            } else if (gameSeries.contains("Animal Crossing", ignoreCase = true) || amiiboSeries.contains("Animal Crossing", ignoreCase = true)) {
                switchGamesList.add(if (isRu) "• Animal Crossing: New Horizons: Плакаты, визит жителя на кемпинг, фотостудия" else "• Animal Crossing: New Horizons: Posters, campsite visitor, Photopia studio")
            } else if (gameSeries.contains("Monster Hunter", ignoreCase = true) || amiiboSeries.contains("Monster Hunter", ignoreCase = true)) {
                switchGamesList.add(if (isRu) "• Monster Hunter Rise: Многослойная броня и ежедневная лотерея Кагари" else "• Monster Hunter Rise: Layered Armor and daily Kagero lottery")
            } else if (gameSeries.contains("Xenoblade", ignoreCase = true) || amiiboSeries.contains("Xenoblade", ignoreCase = true)) {
                switchGamesList.add(if (isRu) "• Xenoblade Chronicles 3: Облик Меча Монадо и ресурсы" else "• Xenoblade Chronicles 3: Monado weapon skin and collectibles")
            } else {
                switchGamesList.add(if (isRu) "• Super Smash Bros. Ultimate: Обучаемый боец FP или бонусы" else "• Super Smash Bros. Ultimate: Trainable FP fighter or Spirit bonuses")
                switchGamesList.add(if (isRu) "• Mario Kart 8 Deluxe: Гоночный костюм Mii" else "• Mario Kart 8 Deluxe: Mii Racing Suit")
                switchGamesList.add(if (isRu) "• Универсальная поддержка: Совместимо со всеми Switch играми с поддержкой Amiibo" else "• Universal Support: Compatible with all Amiibo-enabled Nintendo Switch games")
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

    private fun biReward(
        isRu: Boolean,
        catRu: String,
        catEn: String,
        icon: String,
        titleRu: String,
        titleEn: String,
        descRu: String,
        descEn: String
    ): AmiiboRewardInfo = AmiiboRewardInfo(
        category = if (isRu) catRu else catEn,
        icon = icon,
        title = if (isRu) titleRu else titleEn,
        description = if (isRu) descRu else descEn
    )

    fun getRewardForGame(entry: AmiiboEntry, gameTitle: String): AmiiboRewardInfo {
        val isRu = java.util.Locale.getDefault().language in setOf("ru", "uk", "be")
        val n = entry.name.lowercase()
        val c = entry.character.lowercase()
        val gs = entry.gameSeries.lowercase()
        val aser = entry.amiiboSeries.lowercase()
        val gn = gameTitle.lowercase()

        // 1. Zelda: Tears of the Kingdom
        if (gn.contains("tears") || gn.contains("totk")) {
            if (n.contains("tears") || (c.contains("link") && n.contains("totk"))) {
                return biReward(isRu, "Ткань параплана и оружие", "Paraglider Fabric & Weapons", "🪂", "Новая ткань чемпионов и Меч рыцаря", "New Champion's Fabric & Knight's Broadsword", "Эксклюзивная ткань для параплана «Новая туника чемпионов», крепкий Меч рыцаря, целебные травы и мясо", "Exclusive New Champion's Tunic paraglider fabric, sturdy Knight's Broadsword, herbs, and meat")
            }
            if (n.contains("ocarina") || n.contains("time")) {
                return biReward(isRu, "Оружие, броня и ткань", "Weapons, Armor & Fabric", "🗡️", "Меч Большого Горона и Сет Времени", "Biggoron's Sword & Armor of Time", "Двуручный Меч Большого Горона, Шапка/Туника/Штаны Времени, Ткань маски Лони-Лони", "Two-handed Biggoron's Sword, Cap/Tunic/Trousers of Time, Lon Lon Ranch Fabric")
            }
            if (n.contains("majora")) {
                return biReward(isRu, "Оружие, броня и ткань", "Weapons, Armor & Fabric", "🗡️", "Двуручник и Сет Свирепого Божества", "Fierce Deity Sword & Set", "Маска, Нагрудник и Поножи Свирепого Божества, Двуручник Свирепого Божества, Ткань маски Маджоры", "Fierce Deity Mask/Armor/Boots, Fierce Deity Sword, Majora's Mask Fabric")
            }
            if (n.contains("twilight") || (c.contains("link") && aser.contains("smash"))) {
                return biReward(isRu, "Скакун, броня и ткань", "Mount, Armor & Fabric", "🐺", "Легендарная кобыла Эпона и Сет Сумерек", "Legendary Horse Epona & Twilight Set", "Призыв легендарной лошади Эпоны (максимальные характеристики), Шапка/Туника/Штаны Сумерек, Сумеречная ткань", "Summon legendary horse Epona (max stats), Cap/Tunic/Trousers of Twilight, Mirror of Twilight Fabric")
            }
            if (n.contains("skyward")) {
                return biReward(isRu, "Оружие, броня и ткань", "Weapons, Armor & Fabric", "🗡️", "Белый меч Небес и Сет Неба", "White Sword of the Sky & Sky Set", "Белый меч Богини, Шапка/Туника/Штаны Неба, ткань Меча Небес", "White Sword of the Sky, Cap/Tunic/Trousers of the Sky, Sword-Spirit Fabric")
            }
            if (n.contains("awakening")) {
                return biReward(isRu, "Броня и ткань", "Armor & Fabric", "🛡️", "Сет Пробуждения и Ткань Яйца", "Awakening Set & Egg Fabric", "Мультяшная маска и туника Пробуждения, эксклюзивная ткань Яйца Ветрорыба, солдатские стрелы", "Mask/Tunic/Trousers of Awakening, exclusive Egg Fabric, and arrows")
            }
            if (n.contains("archer")) {
                return biReward(isRu, "Оружие и ткань", "Weapons & Fabric", "🏹", "Ткань капюшона лучника и Королевский лук", "Tunic of Memories Fabric & Royal Bow", "Эксклюзивная ткань лучника, Королевский лук, сырая дичь и редкая рыба", "Exclusive archer fabric, Royal/Soldier's Bow, raw meat, and rare fish")
            }
            if (n.contains("rider")) {
                return biReward(isRu, "Ткань и снаряжение", "Fabric & Gear", "🪂", "Ткань капюшона всадника и Солдатский палаш", "Hylian Hood Fabric & Soldier's Broadsword", "Ткань всадника, оружие всадника, целебные грибы", "Hylian Hood fabric, weapons, and healing mushrooms")
            }
            if (c.contains("wolf link") || n.contains("wolf")) {
                return biReward(isRu, "Охотничьи ресурсы", "Hunting Supplies", "📦", "Охотничий провиант Линка", "Link's Hunting Provisions", "Огромный запас свежего мяса, птицы и дичи для кулинарии", "Large supply of fresh prime and gourmet meat for cooking")
            }
            if (c.contains("zelda")) {
                return biReward(isRu, "Ткань и оружие", "Fabric & Weapons", "🪂", "Ткань принцессы Зельды и Королевский лук", "Princess Zelda Fabric & Royal Bow", "Эксклюзивная ткань принцессы Зельды, древний или королевский лук, драгоценные камни и травы", "Exclusive Princess Zelda fabric, bow, gems, and herbs")
            }
            if (c.contains("ganondorf") || n.contains("ganon")) {
                return biReward(isRu, "Оружие и ткань", "Weapons & Fabric", "🗡️", "Меч и Ткань Демонического Короля", "Demon King Fabric & Dusk Claymore", "Эксклюзивная ткань Демонического Короля, Меч Демонического Короля / Меч сумерек, запеченное мясо и самоцветы", "Exclusive Demon King fabric, Dusk Claymore / Gerudo weapons, meat, and gems")
            }
            if (c.contains("sheik")) {
                return biReward(isRu, "Броня и ткань", "Armor & Fabric", "🛡️", "Маска Шейха и Ткань клана Шеика", "Sheik's Mask & Sheikah Fabric", "Маска Шейха (бонус к скрытности), Ткань клана Шеика, ножи и щиты", "Sheik's Mask (Stealth Up), Sheikah Fabric, Eightfold Blades, and shields")
            }
            if (c.contains("guardian")) {
                return biReward(isRu, "Ткань и механизмы", "Fabric & Ancient Parts", "🪂", "Древняя ткань Шеика и Древние стрелы", "Ancient Sheikah Fabric & Ancient Blades", "Древняя ткань стража, металлические ящики, древние механизмы и древние клинки", "Ancient Sheikah Fabric, metal crates, and Ancient Blades")
            }
            if (c.contains("bokoblin")) {
                return biReward(isRu, "Ткань и оружие", "Fabric & Weapons", "🪂", "Ткань Бокоблина и Боко-щиты", "Bokoblin Fabric & Boko Shields", "Ткань Бокоблина, шипастый боко-щит, дубина и сырое мясо", "Bokoblin Fabric, Boko shields, clubs, and raw meat")
            }
            if (c.contains("daruk") || c.contains("mipha") || c.contains("revali") || c.contains("urbosa")) {
                return biReward(isRu, "Божественный шлем и ткань", "Divine Helm & Fabric", "🛡️", "Шлем Божественного чудища и Ткань чемпиона", "Vah Divine Beast Helm & Champion Fabric", "Уникальный Божественный шлем со стихийной защитой и персональная ткань чемпиона", "Unique Divine Beast Helm with elemental resistance and Champion's Fabric")
            }
            if (gs.contains("zelda") || aser.contains("zelda")) {
                return biReward(isRu, "Оружие и ткань", "Weapons & Fabric", "🗡️", "Тематическая ткань параплана и оружие", "Themed Paraglider Fabric & Weapons", "Эксклюзивная ткань параплана, сундук с высокоуровневым оружием и самоцветами", "Exclusive paraglider fabric, chest with high-tier weapons, and gems")
            }
            return biReward(isRu, "Универсальные ресурсы", "Universal Supplies", "📦", "Припасы путешественника", "Traveler's Provisions", "Сундук со случайным оружием, стрелы, целебные травы, яблоки, мясо и рыба", "Chest with random weapons, arrows, herbs, apples, meat, and fish")
        }

        // 2. Zelda: Breath of the Wild
        if (gn.contains("breath") || gn.contains("botw")) {
            if (c.contains("wolf link") || n.contains("wolf")) {
                return biReward(isRu, "Компаньон", "Companion", "🐺", "Призыв Волка Линка (Wolf Link)", "Summon Wolf Link Companion", "Волк Линк появляется в мире и сражается на вашей стороне с 20 сердцами здоровья!", "Wolf Link spawns in the world and fights alongside you with 20 hearts!")
            }
            if (n.contains("twilight") || (c.contains("link") && aser.contains("smash"))) {
                return biReward(isRu, "Скакун и броня", "Mount & Armor", "🐺", "Кобыла Эпона и Сет Сумерек", "Legendary Horse Epona & Twilight Set", "Призыв легендарной лошади Эпоны с максимальными характеристиками и Набор Сумерек", "Summon legendary horse Epona with max stats and Armor of Twilight")
            }
            if (n.contains("majora")) {
                return biReward(isRu, "Оружие и броня", "Weapons & Armor", "🗡️", "Двуручник и Сет Свирепого Божества", "Fierce Deity Sword & Armor Set", "Маска, Доспех и Поножи Свирепого Божества, Двуручник Свирепого Божества", "Fierce Deity Mask, Armor, Boots, and Fierce Deity Sword")
            }
            if (n.contains("ocarina") || n.contains("time")) {
                return biReward(isRu, "Оружие и броня", "Weapons & Armor", "🗡️", "Меч Большого Горона и Сет Времени", "Biggoron's Sword & Armor of Time", "Шапка/Туника/Штаны Времени, Меч Большого Горона", "Cap, Tunic, and Trousers of Time, plus Biggoron's Sword")
            }
            if (n.contains("archer")) {
                return biReward(isRu, "Оружие", "Weapons", "🏹", "Лук Путешественника и особые стрелы", "Falcon/Royal Bow & Elemental Arrows", "Древние, ледяные, огненные и электрические стрелы, редкое мясо", "Ancient, Ice, Fire, and Shock Arrows, plus rare meat")
            }
            if (c.contains("zelda")) {
                return biReward(isRu, "Щит и самоцветы", "Shield & Gems", "🛡️", "Щит Бригадира и Звездный осколок", "Hylian/Royal Shield & Star Fragment", "Щит Бригадира, редкие рубины, сапфиры, алмазы и целебные травы", "Royal Shield, Star Fragments, Rubies, Sapphires, Diamonds, and herbs")
            }
            if (c.contains("daruk") || c.contains("mipha") || c.contains("revali") || c.contains("urbosa")) {
                return biReward(isRu, "Броня", "Armor", "🛡️", "Шлем Божественного чудища", "Vah Divine Beast Helm", "Шлемы Ва-Рудания, Ва-Рута, Ва-Медо или Ва-Наборис со стихийной защитой", "Vah Rudania, Vah Ruta, Vah Medoh, or Vah Naboris Divine Helms")
            }
            if (gs.contains("zelda") || aser.contains("zelda")) {
                return biReward(isRu, "Оружие и сундук", "Weapon & Chest", "🗡️", "Уникальный сундук Хайрула", "Unique Hyrule Treasure Chest", "Сундук с редким оружием, стрелами и драгоценными металлами", "Treasure chest with rare weapons, arrows, and precious gems")
            }
            return biReward(isRu, "Ресурсы", "Supplies", "📦", "Припасы и сырье", "Provisions & Materials", "Сундук с базовым оружием, мясо, грибы, травы и овощи", "Chest with weapons, meat, mushrooms, herbs, and vegetables")
        }

        // 3. Zelda: Echoes of Wisdom
        if (gn.contains("echoes of wisdom") || gn.contains("wisdom")) {
            if (c.contains("zelda") || n.contains("zelda")) {
                return biReward(isRu, "Костюм", "Outfit", "👗", "Особое платье принцессы Зельды", "Blue Attire & Smoothie Ingredients", "Эксклюзивный наряд принцессы для путешествия по Хайрулу, Камни Силы и редкие смузи ингредиенты", "Exclusive Blue Attire for Zelda, Might Crystals, and rare smoothie ingredients")
            }
            if (c.contains("link") || n.contains("link")) {
                return biReward(isRu, "Костюм", "Outfit", "👕", "Одеяния Героя", "Red Tunic & Smoothie Ingredients", "Эксклюзивный зелёный наряд Героя Мечника, целебные травы и ценные кристаллы эхо", "Exclusive Tunic variant, herbs, and rare smoothie ingredients")
            }
            if (c.contains("cat") || n.contains("cat") || n.contains("mario")) {
                return biReward(isRu, "Костюм", "Outfit", "🐱", "Костюм Чёрной кошки", "Black Cat Clothes", "Уникальный наряд Чёрной кошки, позволяющий общаться с котиками Хайрула!", "Unique Black Cat Clothes that let you talk to cats across Hyrule!")
            }
            return biReward(isRu, "Ресурсы", "Supplies", "📦", "Набор кристаллов и смузи", "Smoothie Ingredients Pack", "Набор редких ингредиентов для приготовления смузи, яблоки, масло и Камни Силы", "Pack of rare smoothie ingredients, Golden Eggs, Chilly/Warm Peppers, and Monster Stones")
        }

        // 4. Super Mario Odyssey
        if (gn.contains("odyssey") || gn.contains("mario odyssey")) {
            if (n.contains("wedding") && c.contains("mario")) {
                return biReward(isRu, "Костюм и усиление", "Outfit & Power-Up", "👕", "Свадебный смокинг Марио и Неуязвимость", "Mario's Wedding Tuxedo & Invincibility", "Свадебный цилиндр и смокинг; Неуязвимость Марио ко всем видам урона на 30 секунд!", "Mario's Wedding Top Hat & Tuxedo; grants temporary Invincibility!")
            }
            if (n.contains("wedding") && c.contains("bowser")) {
                return biReward(isRu, "Костюм и подсказки", "Outfit & Radar", "👕", "Свадебный смокинг Боузера и Локатор монет", "Bowser's Wedding Tuxedo & Coin Radar", "Свадебный цилиндр Боузера; Подсветка всех региональных фиолетовых монет на карте!", "Bowser's Wedding Outfit; reveals nearby Purple Regional Coins!")
            }
            if (n.contains("wedding") && c.contains("peach")) {
                return biReward(isRu, "Костюм и здоровье", "Outfit & Life-Up", "👗", "Свадебное платье Пич и Сердце жизни", "Wedding Dress & Life-Up Heart", "Свадебная фата и платье Пич; Полное восстановление здоровья и Сердце Жизни (+3 HP)", "Peach's Wedding Veil & Dress; grants a Life-Up Heart (6 HP)")
            }
            if (c.contains("mario")) {
                return biReward(isRu, "Костюм", "Outfit", "👕", "Классический костюм Марио", "Classic Mario Suit & Invincibility", "Классическая кепка и комбинезон Марио, временная неуязвимость", "Classic Mario Cap & Suit, plus temporary invincibility")
            }
            if (c.contains("luigi")) {
                return biReward(isRu, "Костюм", "Outfit", "👕", "Костюм Луиджи", "Luigi Cap & Suit", "Кепка и комбинезон Луиджи для Марио", "Luigi Cap and Suit for Mario")
            }
            if (c.contains("wario") || c.contains("waluigi")) {
                return biReward(isRu, "Костюм", "Outfit", "👕", "Костюм Варио / Валуиджи", "Wario / Waluigi Cap & Suit", "Костюмы Варио и Валуиджи со шляпами", "Wario and Waluigi Caps and Suits")
            }
            return biReward(isRu, "Подсказка дяди Амибо", "Uncle amiibo Hint", "⭐", "Поиск Луны энергии", "Power Moon Location Hint", "Дядя Амибо отмечает на карте точное местоположение скрытой Луны энергии!", "Uncle amiibo marks the exact location of a hidden Power Moon on your map!")
        }

        // 5. Super Mario 3D World / Bowser's Fury
        if (gn.contains("3d world") || gn.contains("bowser's fury") || gn.contains("bowsers fury")) {
            if (n.contains("cat mario") || (c.contains("mario") && n.contains("cat"))) {
                return biReward(isRu, "Усилитель", "Power-Up", "🌟", "Белый Кот Марио (Неуязвимость)", "White Cat Mario (Invincibility Bell)", "Превращает Марио в Белого Кота Марио с постоянной неуязвимостью к врагам!", "Grants an Invincibility Bell that transforms Mario into White Cat Mario!")
            }
            if (n.contains("cat peach") || (c.contains("peach") && n.contains("cat"))) {
                return biReward(isRu, "Бонус", "Power-Up", "🐱", "Случайный супер-усилитель", "Random Super Power-Up", "Дарует случайный редкий бонус: Супер-Колокольчик, Бумеранг или Лист Тануки", "Spawns a random rare power-up: Super Bell, Boomerang Flower, or Super Leaf")
            }
            if (c.contains("bowser")) {
                return biReward(isRu, "Событие", "Event", "🔥", "Мгновенное пробуждение Яростного Боузера", "Summon Fury Bowser", "Немедленно активирует Ярость Боузера и шторм на озере Кошачьих Лап", "Immediately summons Fury Bowser in Lake Lapcat")
            }
            if (c.contains("bowser jr")) {
                return biReward(isRu, "Ударная волна", "Shockwave", "💥", "Мощная взрывная волна Боузера-младшего", "Bowser Jr. Shockwave Blast", "Уничтожает всех ближайших врагов и открывает секретные блоки", "Unleashes a powerful shockwave that defeats nearby enemies and breaks blocks")
            }
            return biReward(isRu, "Бонус", "Bonus", "🍄", "Гриб 1-Up или Супер-Гриб", "1-Up Mushroom or Super Star", "Дарует дополнительную жизнь или увеличение персонажа", "Grants a 1-Up Mushroom or Super Star")
        }

        // 6. Metroid Dread
        if (gn.contains("metroid") || gn.contains("dread")) {
            if (c.contains("samus") && (n.contains("dread") || aser.contains("metroid"))) {
                return biReward(isRu, "Контейнер энергии", "Energy Tank", "🔋", "Дополнительный бак энергии (+100 HP)", "Extra Energy Tank (+100 HP)", "Навсегда увеличивает максимальный запас здоровья Самус на один бак (+100 HP) и восстанавливает энергию раз в день", "Permanently increases Samus's max health by one Energy Tank (+100 HP) and restores energy daily")
            }
            if (c.contains("emmi") || n.contains("e.m.m.i") || n.contains("emmi")) {
                return biReward(isRu, "Ракетный комплекс", "Missile+ Tank", "🚀", "Дополнительный контейнер ракет (+10)", "Extra Missile+ Tank (+10)", "Навсегда увеличивает максимальный боезапас ракет Самус на +10 и восполняет ракеты раз в день", "Permanently increases Samus's max missile capacity by +10 and replenishes missiles daily")
            }
            return biReward(isRu, "Пополнение", "Replenish", "⚡", "Восстановление ресурсов Самус", "Daily Energy & Missile Replenishment", "Экстренное пополнение запаса здоровья или ракет во время миссии", "Replenishes health and missiles once per day")
        }

        // 7. Splatoon 3 / Splatoon 2
        if (gn.contains("splatoon")) {
            if (aser.contains("splatoon") || gs.contains("splatoon")) {
                return biReward(isRu, "Экипировка", "Exclusive Gear", "👕", "Эксклюзивный комплект одежды и обуви", "3-Piece Exclusive Gear Set", "Уникальный 3-предметный набор брендовой экипировки с эксклюзивными способностями и фотосессия", "Unlocks a 3-piece exclusive gear set (Headgear, Clothing, Shoes) and photo mode poses")
            }
            return biReward(isRu, "Бонус", "Bonus", "🎨", "Специальные сувениры и снимки", "Special Souvenirs & Photos", "Совместные снимки в фотобудке Инкополиса", "Take photos together in Splatsville / Inkopolis")
        }

        // 8. Monster Hunter Rise
        if (gn.contains("monster hunter") || gn.contains("rise") || gn.contains("sunbreak")) {
            if (gs.contains("monster hunter") || n.contains("magnamalo") || n.contains("palamute") || n.contains("palico") || n.contains("malzeno")) {
                return biReward(isRu, "Многослойная броня", "Layered Armor", "🛡️", "Особая броня Синра / Малзено", "Sinister Seal / Formal Dragon Layered Armor", "Эксклюзивный комплект многослойной брони охотника, Палико или Паламута, а также участие в лотерее Кагари", "Unlocks exclusive Layered Armor for Hunter, Palamute, or Palico, plus daily lottery entry")
            }
            return biReward(isRu, "Лотерея", "Lottery", "🎲", "Ежедневный лотерейный билет Кагари", "Daily Kagero Lottery Ticket", "Бесплатная лотерея у торговца Кагари с ценными зельями, ловушками и сферами доспехов", "Free daily lottery spin with Kagero the Merchant for potions, traps, and armor spheres")
        }

        // 9. Xenoblade Chronicles 3
        if (gn.contains("xenoblade")) {
            if (c.contains("shulk")) {
                return biReward(isRu, "Облик оружия", "Weapon Skin", "⚔️", "Легендарный меч Монадо", "Legendary Monado Sword Skin", "Разблокирует легендарный внешний облик клинка Меч Монадо для мечника Ноа!", "Unlocks the legendary Monado blade attachment skin for Swordfighter!")
            }
            if (n.contains("noah") || n.contains("mio")) {
                return biReward(isRu, "Костюм", "Outfit", "👕", "Особый наряд для спутников", "Special Leisure Time Outfits", "Уникальная куртка Ноа или наряд Мио для всех членов отряда", "Unlocks Noah's and Mio's special outfits for party members")
            }
            return biReward(isRu, "Предметы", "Collectibles", "📦", "Коллекционные предметы и монеты Нопонов", "Collectibles & Nopon Coins", "Набор редких коллекционных предметов и золотые/серебряные монеты Нопонов", "Pack of rare collectibles and Silver/Gold Nopon Coins")
        }

        // 10. Super Smash Bros. Ultimate
        if (gn.contains("smash") || gn.contains("ssbu")) {
            return biReward(isRu, "Боец FP", "FP Fighter", "🥊", "Figure Player: Обучаемый боец FP", "Figure Player: Trainable FP Fighter", "Создает уникального бойца FP (1-50 ур.), обучающегося вашему боевому стилю и тактике!", "Creates a trainable Figure Player (Lv. 1-50) that learns from your playstyle, or grants a Spirit!")
        }

        // 11. Mario Kart 8 Deluxe
        if (gn.contains("mario kart") || gn.contains("mk8")) {
            return biReward(isRu, "Гоночный костюм", "Racing Suit", "🏎️", "Гоночный костюм Mii", "Mii Racing Suit", "Разблокирует уникальный тематический комбинезон и гоночный шлем персонажа для гонщика Mii!", "Unlocks a themed character racing suit and helmet for your Mii driver!")
        }

        // 12. Animal Crossing: New Horizons
        if (gn.contains("animal crossing") || gn.contains("horizons")) {
            return biReward(isRu, "Кемпинг и фото", "Campsite & Photo", "🏕️", "Визит на кемпинг и постер жителя", "Campsite Visitor & Character Poster", "Приглашает жителя на кемпинг острова, разблокирует памятный постер и фотосессию на острове Харви", "Invite the character to your Campsite, Roost cafe, or Harv's Island Photopia and unlock their poster")
        }

        // 13. Fire Emblem
        if (gn.contains("fire emblem") || gn.contains("engage") || gn.contains("three houses")) {
            return biReward(isRu, "Музыка и костюм", "Music & Outfit", "🎶", "Саундтрек персонажа и билеты беседки", "Legacy Music Track & Fashion Tickets", "Разблокирует классический музыкальный трек персонажа, купоны нарядов и ценные печати призыва", "Unlocks a classic Fire Emblem music track, fashion tickets, and Relay Tickets at the Amiibo Gazebo")
        }

        // 14. Kirby
        if (gn.contains("kirby")) {
            return biReward(isRu, "Усилитель", "Boost", "⭐", "Особая способность и здоровье", "Healing Items & Star Coins", "Дарует мощную способность копирования, звездочки и восстанавливает шкалу здоровья Кирби", "Grants healing items, Star Coins, and Rare Stones")
        }

        // 15. Pokemon
        if (gn.contains("pokemon") || gn.contains("pokémon")) {
            return biReward(isRu, "Награда", "Reward", "🎒", "Припасы тренера и ягоды", "Trainer Supplies & Berries", "Набор полезных ягод, покеболов и предметов для тренировки покемонов", "Bundle of useful Berries, Poké Balls, and training items")
        }

        // Universal Fallback
        return biReward(isRu, "Награда", "Reward", "🎁", "Ежедневный подарочный набор", "Daily In-Game Gift Pack", "Ежедневный набор полезных внутриигровых ресурсов, монет и расходных материалов", "Daily bundle of useful in-game materials, coins, and consumables")
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

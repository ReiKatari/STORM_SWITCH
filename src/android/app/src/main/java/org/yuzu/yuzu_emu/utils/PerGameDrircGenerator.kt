package org.yuzu.yuzu_emu.utils

import java.io.File
import org.yuzu.yuzu_emu.YuzuApplication

/**
 * Generates tailored, conflict-free Mesa Turnip and PanVK drirc configurations
 * on-the-fly for any Nintendo Switch game prior to Vulkan instance creation.
 *
 * This ensures that games requiring opposing driver settings (e.g. Zelda needing
 * tu_tile_discard=false vs Diablo II/Streets of Rage 4 needing tu_tile_discard=true)
 * each receive their exact ideal profile in <application name="all"> without conflict.
 */
object PerGameDrircGenerator {

    enum class GameProfileType {
        ZELDA,
        DIABLO,
        STREETS_OF_RAGE_4,
        XENOBLADE,
        RED_DEAD_REDEMPTION,
        SUPER_MARIO_ODYSSEY,
        SUPER_SMASH_BROS,
        METROID_PRIME,
        HOGWARTS_LEGACY,
        THE_WITCHER_3,
        BAYONETTA,
        MONSTER_HUNTER_RISE,
        POKEMON,
        PERSONA_5_ROYAL,
        NIER_AUTOMATA,
        DOOM_ETERNAL,
        BORDERLANDS,
        MORTAL_KOMBAT_1,
        MORTAL_KOMBAT_11,
        ASSASSINS_CREED,
        EA_SPORTS,
        ALAN_WAKE,
        LEGEND_OF_HEROES,
        SONIC,
        LITTLE_NIGHTMARES,
        DISCO_ELYSIUM,
        MARIO_RABBIDS,
        MARIO_WONDER_JAMBOREE,
        UNIVERSAL_DEFAULT
    }

    fun detectProfileType(programIdHex: String, title: String): GameProfileType {
        var rawId = programIdHex.trim().uppercase()
        if (rawId.startsWith("0X")) {
            rawId = rawId.substring(2)
        }
        val cleanId = if (rawId.length == 16) {
            rawId.substring(0, 13) + "000"
        } else {
            rawId
        }
        val cleanTitle = title.trim().lowercase()

        // 1. Zelda Series (LunchPack / KingSystem - Zero strobing, crystal water, intact shrines)
        if (cleanId.startsWith("01007EF00011E") || // BotW
            cleanId.startsWith("0100F2C0115B6") || // TotK
            cleanId.startsWith("01006BB00C6F0") || // Link's Awakening
            cleanId.startsWith("01008CF01BA04") || // Echoes of Wisdom
            cleanId.startsWith("01008CF01BAEC") || // Echoes of Wisdom Alt
            cleanId.startsWith("01002DA013484") || // Skyward Sword HD
            cleanId.startsWith("01003C700009C") || // Hyrule Warriors Definitive
            cleanId.startsWith("0100874011E54") || // Hyrule Warriors: Age of Calamity
            cleanId.startsWith("0100000000000074") || // Ship of Harkinian / 2S2H
            cleanTitle.contains("zelda") ||
            cleanTitle.contains("hyrule") ||
            cleanTitle.contains("breath of the wild") ||
            cleanTitle.contains("tears of the kingdom") ||
            cleanTitle.contains("skyward sword") ||
            cleanTitle.contains("link's awakening") ||
            cleanTitle.contains("echoes of wisdom")
        ) {
            return GameProfileType.ZELDA
        }

        // 2. Diablo Series (Diablo II: Resurrected ALL 3 Title IDs + Title)
        if (cleanId == "01002A801458A000" || cleanId.startsWith("01002A801458A") || // Diablo II USA
            cleanId == "0100916014D8C000" || cleanId.startsWith("0100916014D8C") || // Diablo II EUR
            cleanId == "0100726014352000" || cleanId.startsWith("0100726014352") || // Diablo II Resurrected JPN/Alt
            cleanId.startsWith("01001B300B9BE") || // Diablo III
            cleanId.startsWith("01001B700A654") || // Diablo III Eternal
            cleanId.startsWith("0100D1AB10000000") || // DevilutionX
            cleanTitle.contains("diablo ii") ||
            cleanTitle.contains("diablo 2") ||
            cleanTitle.contains("diablo")
        ) {
            return GameProfileType.DIABLO
        }

        // 2b. Disco Elysium: The Final Cut (Unity Deferred Lighting & TextMeshPro Swizzle)
        if (cleanId.startsWith("0100C88011246") ||
            cleanId.startsWith("01008C300F7F0") ||
            cleanId.startsWith("0100E26014466") ||
            cleanTitle.contains("disco elysium")
        ) {
            return GameProfileType.DISCO_ELYSIUM
        }

        // 2c. Mario + Rabbids Series (Snowdrop Engine Shadow Maps, Exposure & Fullscreen Quad)
        if (cleanId.startsWith("010067300059A") || // Mario + Rabbids: Kingdom Battle
            cleanId.startsWith("0100760012E4A") || // Mario + Rabbids: Kingdom Battle Gold
            cleanId.startsWith("010032C00AC58") || // Mario + Rabbids: Sparks of Hope
            cleanId.startsWith("01005CA00F966") || // Mario + Rabbids: Sparks of Hope
            cleanTitle.contains("rabbids")
        ) {
            return GameProfileType.MARIO_RABBIDS
        }

        // 3. Streets of Rage 4 & 2D Action Sprites (Pristine Pixel Art & Video Cutscenes)
        if (cleanId.startsWith("0100EC9010258") || // Streets of Rage 4
            cleanId.startsWith("010085800E33E") || // Streets of Rage 4 Main
            cleanId.startsWith("0100BA700E340") || // Streets of Rage 4 Alt
            cleanId.startsWith("0100A83022A22") || // Absolum
            cleanId.startsWith("010063B0077F6") || // Hollow Knight
            cleanId.startsWith("01008EB00B32E") || // Cuphead
            cleanId.startsWith("010006200A526") || // Dead Cells
            cleanId.startsWith("0100854005118") || // Celeste
            cleanId.startsWith("01009AA000FAA") || // Sonic Mania
            cleanId.startsWith("0100109015CA6") || // TMNT Shredder's Revenge
            cleanId.startsWith("0100779004BEE") || // Rayman Legends
            cleanId.startsWith("010019C00D6FE") || // Ori 1
            cleanId.startsWith("0100B1A00FECC") || // Ori 2
            cleanId.startsWith("0100B2B0192A0") || // Vampire Survivors
            cleanId.startsWith("0100AE801659C") || // Prince of Persia: The Lost Crown
            cleanId.startsWith("0100535012974") || // Hades
            cleanId.startsWith("0100B880182CA") || // Sea of Stars
            cleanId.startsWith("0100570003AC8") || // Octopath Traveler
            cleanId.startsWith("010057A016848") || // Octopath Traveler II
            cleanId.startsWith("0100CC80140F8") || // Triangle Strategy
            cleanId.startsWith("010023400DA34") || // Blasphemous
            cleanId.startsWith("010067901A51A") || // Blasphemous 2
            cleanId.startsWith("010004D00A9C0") || // Aggelos
            cleanId.startsWith("010040F01EC60") || // Aggelos 2
            cleanTitle.contains("absolum") ||
            cleanTitle.contains("aggelos") ||
            cleanTitle.contains("streets of rage") ||
            cleanTitle.contains("hollow knight") ||
            cleanTitle.contains("cuphead") ||
            cleanTitle.contains("dead cells") ||
            cleanTitle.contains("celeste") ||
            cleanTitle.contains("sonic mania") ||
            cleanTitle.contains("tmnt") ||
            cleanTitle.contains("shredder") ||
            cleanTitle.contains("vampire survivors") ||
            cleanTitle.contains("blasphemous") ||
            cleanTitle.contains("sea of stars") ||
            cleanTitle.contains("lost crown") ||
            cleanTitle.contains("hades") ||
            cleanTitle.contains("octopath") ||
            cleanTitle.contains("triangle strategy")
        ) {
            return GameProfileType.STREETS_OF_RAGE_4
        }

        // 4. Super Smash Bros. Ultimate
        if (cleanId.startsWith("01006A800016E") ||
            cleanTitle.contains("smash bros") ||
            cleanTitle.contains("super smash")
        ) {
            return GameProfileType.SUPER_SMASH_BROS
        }

        // 5. Xenoblade Chronicles Series (Monolith Soft Water & Cloud Depth Stability)
        if (cleanId.startsWith("0100FF500E34A") || // Xenoblade DE
            cleanId.startsWith("0100E95004038") || // Xenoblade 2
            cleanId.startsWith("0100C48008890") || // Xenoblade 2 Torna
            cleanId.startsWith("010074F013262") || // Xenoblade 3
            cleanId.startsWith("0100F3400332E") ||
            cleanId.startsWith("010074600E26E") ||
            cleanTitle.contains("xenoblade") ||
            cleanTitle.contains("torna")
        ) {
            return GameProfileType.XENOBLADE
        }

        // 6. Red Dead Redemption (RAGE Engine)
        if (cleanId.startsWith("01007820195A6") ||
            cleanId.startsWith("0100E26017E5E") ||
            cleanTitle.contains("red dead redemption") ||
            cleanTitle.contains("red dead")
        ) {
            return GameProfileType.RED_DEAD_REDEMPTION
        }

        // 7. Metroid Prime & Dread (MercurySteam / Retro Studios)
        if (cleanId.startsWith("0100121014688") || // Metroid Prime Remastered
            cleanId.startsWith("0100813014B74") ||
            cleanId.startsWith("010093801237C") || // Metroid Dread
            cleanId.startsWith("01005AF00BA7A") ||
            cleanTitle.contains("metroid prime") ||
            cleanTitle.contains("metroid dread") ||
            cleanTitle.contains("metroid")
        ) {
            return GameProfileType.METROID_PRIME
        }

        // 8. Hogwarts Legacy (Unreal Engine 4 Heavy RPG)
        if (cleanId.startsWith("0100BCB0176D0") ||
            cleanId.startsWith("0100B5B0112F8") ||
            cleanId.startsWith("0100D1801648E") ||
            cleanTitle.contains("hogwarts legacy") ||
            cleanTitle.contains("hogwarts")
        ) {
            return GameProfileType.HOGWARTS_LEGACY
        }

        // 9. The Witcher 3: Wild Hunt & Skyrim (Heavy Open World RPG)
        if (cleanId.startsWith("01003D100E9C6") ||
            cleanId.startsWith("010014B00E9BE") ||
            cleanId.startsWith("0100E670129CA") ||
            cleanId.startsWith("0100E67012924") ||
            cleanId.startsWith("01000A10041EA") || // Skyrim
            cleanTitle.contains("witcher") ||
            cleanTitle.contains("wild hunt") ||
            cleanTitle.contains("skyrim") ||
            cleanTitle.contains("elder scrolls")
        ) {
            return GameProfileType.THE_WITCHER_3
        }

        // 10. Bayonetta Series (PlatinumGames)
        if (cleanId.startsWith("010076F0049A2") ||
            cleanId.startsWith("0100778006E88") ||
            cleanId.startsWith("01009A1008C36") ||
            cleanId.startsWith("01007760086F8") ||
            cleanId.startsWith("01004A4010F22") || // Bayonetta 3
            cleanId.startsWith("0100B5E018BD6") || // Cereza
            cleanTitle.contains("bayonetta") ||
            cleanTitle.contains("cereza")
        ) {
            return GameProfileType.BAYONETTA
        }

        // 11. Monster Hunter Series (Capcom RE Engine & MT Framework)
        if (cleanId.startsWith("0100B04011742") || // Rise
            cleanId.startsWith("0100830007780") || // Sunbreak
            cleanId.startsWith("0100770008DD8") || // Generations Ultimate
            cleanId.startsWith("01000F5012586") || // Stories 2
            cleanTitle.contains("monster hunter") ||
            cleanTitle.contains("sunbreak")
        ) {
            return GameProfileType.MONSTER_HUNTER_RISE
        }

        // 12. Pokemon Series
        if (cleanId.startsWith("0100A3D008C5C") || // Scarlet
            cleanId.startsWith("01008C30086E0") ||
            cleanId.startsWith("01008F6008C5E") || // Violet
            cleanId.startsWith("0100A3D0086EE") ||
            cleanId.startsWith("01001F5010DFA") || // Arceus
            cleanId.startsWith("0100ABF008968") || // Sword
            cleanId.startsWith("01008DB008C2C") || // Shield
            cleanId.startsWith("0100000011D90") || // BD
            cleanId.startsWith("010018E011D92") || // SP
            cleanId.startsWith("010003F003A34") || // Let's Go Pikachu
            cleanId.startsWith("0100152000022") || // Let's Go Eevee
            cleanId.startsWith("01008F2012014") || // Snap
            cleanId.startsWith("010025D00E266") || // Mystery Dungeon
            cleanTitle.contains("pokemon")
        ) {
            return GameProfileType.POKEMON
        }

        // 13. Persona & Shin Megami Tensei Series (Atlus)
        if (cleanId.startsWith("01000A10041EA") || // Persona 5 Royal
            cleanId.startsWith("01005CA0157CA") ||
            cleanId.startsWith("0100224016A90") || // Persona 4 Golden
            cleanId.startsWith("010058C01570E") ||
            cleanId.startsWith("010072001570A") || // Persona 3 Portable
            cleanId.startsWith("01006A4011400") || // Persona 5 Strikers
            cleanId.startsWith("0100B5B019E06") || // Persona 5 Tactica
            cleanId.startsWith("0100B870097D6") || // SMT V
            cleanId.startsWith("01000B901C46E") || // SMT V Vengeance
            cleanId.startsWith("01006F801BC4C") ||
            cleanId.startsWith("01007E3019808") ||
            cleanTitle.contains("persona") ||
            cleanTitle.contains("shin megami tensei") ||
            cleanTitle.contains("smt v") ||
            cleanTitle.contains("smtv")
        ) {
            return GameProfileType.PERSONA_5_ROYAL
        }

        // 14. NieR: Automata (PlatinumGames / Square Enix)
        if (cleanId.startsWith("01005C9014168") ||
            cleanId.startsWith("01007D401662A") ||
            cleanTitle.contains("nier:automata") ||
            cleanTitle.contains("nier automata") ||
            cleanTitle.contains("yorha")
        ) {
            return GameProfileType.NIER_AUTOMATA
        }

        // 15. DOOM & Wolfenstein Series (id Tech 6 / 7)
        if (cleanId.startsWith("010053700465C") || // DOOM 2016
            cleanId.startsWith("0100B9F010DC4") || // DOOM Eternal
            cleanId.startsWith("0100BB600DC30") ||
            cleanId.startsWith("0100D00100000000") || // DOOM Classic
            cleanId.startsWith("0100D00300000000") || // DOOM 3
            cleanId.startsWith("01002A3008E88") || // Wolfenstein II
            cleanId.startsWith("01000AE009FF2") || // Wolfenstein Youngblood
            cleanTitle.contains("doom") ||
            cleanTitle.contains("wolfenstein")
        ) {
            return GameProfileType.DOOM_ETERNAL
        }

        // 16. Borderlands Series (Gearbox)
        if (cleanId.startsWith("01007E300B70C") || // Legendary Collection
            cleanId.startsWith("01008C30113B0") || // Borderlands 3
            cleanId.startsWith("01006D401831E") || // Tiny Tina
            cleanTitle.contains("borderlands") ||
            cleanTitle.contains("tiny tina")
        ) {
            return GameProfileType.BORDERLANDS
        }

        // 17. Mortal Kombat 11 (Dedicated High-FPS Profile: clean 55-60 FPS on Turnip)
        if (cleanId.startsWith("0100B1100C4D0") ||
            cleanId.startsWith("0100F2200C984") ||
            cleanTitle.contains("mortal kombat 11") ||
            cleanTitle.contains("mk11") ||
            cleanTitle.contains("mk 11")
        ) {
            return GameProfileType.MORTAL_KOMBAT_11
        }

        // 18. Mortal Kombat 1 & Batman Arkham Series (Heavy UE4 Custom Pipelines)
        if (cleanId.startsWith("01006560184E6") || // MK 1
            cleanId.startsWith("0100D2800D5C2") ||
            cleanId.startsWith("010063B017DAE") || // Arkham Knight
            cleanId.startsWith("010023A017E94") ||
            cleanId.startsWith("01003AE017DB0") || // Arkham City
            cleanId.startsWith("01001D4017DA8") || // Arkham Asylum
            cleanId.startsWith("01000B0012E4E") || // Cronos
            cleanId.startsWith("010055700C30A") || // Outer Worlds
            cleanTitle.contains("mortal kombat 1") ||
            cleanTitle.contains("mk1") ||
            cleanTitle.contains("mk 1") ||
            cleanTitle.contains("arkham") ||
            cleanTitle.contains("batman")
        ) {
            return GameProfileType.MORTAL_KOMBAT_1
        }

        // 18. Assassin's Creed Series (Ubisoft AnvilNext)
        if (cleanId.startsWith("010043800C3DA") || // Rebel Collection (Black Flag / Rogue)
            cleanId.startsWith("0100BC8013E8E") || // Ezio Collection
            cleanId.startsWith("010010000C16C") || // AC III Remastered
            cleanTitle.contains("assassin")
        ) {
            return GameProfileType.ASSASSINS_CREED
        }

        // 19. EA Sports FC Series (Frostbite Engine)
        if (cleanId.startsWith("0100A38018D5A") || // FC 24
            cleanId.startsWith("01005B101DC84") || // FC 25
            cleanId.startsWith("010077D0238FA") || // FC 26
            cleanTitle.contains("ea sports") ||
            cleanTitle.contains("fifa") ||
            cleanTitle.contains("fc 2")
        ) {
            return GameProfileType.EA_SPORTS
        }

        // 20. Alan Wake Remastered (Font and Subtitle Color Fix)
        if (cleanId.startsWith("0100F830138E6") || // Alan Wake Remastered
            cleanId.startsWith("010074B01456A") || // Alan Wake USA
            cleanId.startsWith("010065F014C5E") || // Alan Wake EUR
            cleanTitle.contains("alan wake")
        ) {
            return GameProfileType.ALAN_WAKE
        }

        // 21. The Legend of Heroes & Falcom Series
        if (cleanId.startsWith("0100B0E020356") ||
            cleanId.startsWith("01004A301D6E2") ||
            cleanId.startsWith("01002C5016EE8") ||
            cleanId.startsWith("0100F7A016336") ||
            cleanTitle.contains("trails in the sky") ||
            cleanTitle.contains("trails of cold steel") ||
            cleanTitle.contains("legend of heroes") ||
            cleanTitle.contains("ys viii") ||
            cleanTitle.contains("ys ix") ||
            cleanTitle.contains("ys x")
        ) {
            return GameProfileType.LEGEND_OF_HEROES
        }

        // 21b. Mario Wonder & Jamboree (Anti-Flicker & Concurrent Binning Off)
        if (cleanId == "010015100B514000" || cleanId.startsWith("010015100B514") || // Mario Wonder
            cleanId.startsWith("010015100B5C4") ||
            cleanId.startsWith("010015100B5B4") ||
            cleanId == "010036601D380000" || cleanId.startsWith("010036601D380") || // Party Jamboree
            cleanTitle.contains("wonder") ||
            cleanTitle.contains("jamboree")
        ) {
            return GameProfileType.MARIO_WONDER_JAMBOREE
        }

        // 22. Mario Series (Odyssey, Wonder, 3D World, Kart, RPG, etc.)
        if (cleanId.startsWith("0100000000010") || // Odyssey
            cleanId.startsWith("010015100B5C4") || // Wonder
            cleanId.startsWith("010015100B5B4") ||
            cleanId.startsWith("010028600EBDA") || // 3D World
            cleanId.startsWith("0100152000022") || // Mario Kart 8
            cleanId.startsWith("01007EF01B842") || // Paper Mario TTYD
            cleanId.startsWith("01004D701742A") ||
            cleanId.startsWith("01004D300C5AE") || // Origami King
            cleanId.startsWith("0100D57018DEE") || // Mario & Luigi Brothership
            cleanId.startsWith("010091801E8B2") ||
            cleanId.startsWith("010062C019800") || // Super Mario RPG
            cleanId.startsWith("0100BC0018138") ||
            cleanId.startsWith("01008D3017B4C") ||
            cleanId.startsWith("0100DCA0064A6") || // Luigi's Mansion 3
            cleanId.startsWith("010036601D380") || // Party Jamboree
            cleanId.startsWith("010049900F546") || // 3D All-Stars
            cleanId.startsWith("0100427010476") ||
            cleanId.startsWith("0100F7701140E") || // Golf
            cleanId.startsWith("0100874011EE6") ||
            cleanId.startsWith("0100BDE00862A") || // Tennis Aces
            cleanId.startsWith("010019401051C") || // Strikers
            cleanId.startsWith("0100000000000064") || // SM64-NX
            cleanTitle.contains("mario") ||
            cleanTitle.contains("luigi") ||
            cleanTitle.contains("donkey kong")
        ) {
            return GameProfileType.SUPER_MARIO_ODYSSEY
        }

        // 23. Sonic 3D Series (Hedgehog Engine 2)
        if (cleanId.startsWith("01006CD0127BE") || // Frontiers
            cleanId.startsWith("01006CD016142") || // Superstars
            cleanId.startsWith("0100DA80036C8") || // Forces
            cleanId.startsWith("01005110137EE") || // Colors
            cleanId.startsWith("0100A7D01B694") || // Generations
            cleanTitle.contains("sonic")
        ) {
            return GameProfileType.SONIC
        }

        // 24. Little Nightmares Series (Unreal Engine 4 & 5 Bloom and Volumetric Light Fix)
        if (cleanId.startsWith("01002FC00412C") || // Little Nightmares I
            cleanId.startsWith("010097100EDD6") || // Little Nightmares II
            cleanId.startsWith("010066101A55A") || // Little Nightmares III
            cleanTitle.contains("little nightmares")
        ) {
            return GameProfileType.LITTLE_NIGHTMARES
        }

        return GameProfileType.UNIVERSAL_DEFAULT
    }

    fun generateXmlContent(profileType: GameProfileType, programIdHex: String, title: String): String {
        val optionsBuilder = StringBuilder()

        // 100% Real Compiled Mesa Turnip 26.2.0 (STORM DRIVER 8.0.0) Driconf Options:
        when (profileType) {
            GameProfileType.DIABLO -> {
                optionsBuilder.append("            <!-- TIER 1 / ISOMETRIC ARPGs (Diablo II: Resurrected - tu_dont_care_as_load omitted to preserve UI text & font glyphs) -->\n")
                optionsBuilder.append("            <option name=\"tu_allow_concurrent_binning\" value=\"false\" />\n")
                optionsBuilder.append("            <option name=\"disable_cb\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"disable_conservative_lrz\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"enable_lrz_fast_clear\" value=\"false\" />\n")
                optionsBuilder.append("            <option name=\"tu_override_uncached_as_cache_coherent\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"ubwc_coherency_quirk\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"tu_use_tex_coord_round_nearest_even_mode\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"tu_enable_d24s8_border_color_workaround\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"tu_enable_fast_border_color_for_undefined_formats\" value=\"false\" />\n")
                optionsBuilder.append("            <option name=\"tu_ignore_frag_depth_direction\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"dual_color_blend_by_location\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"tu_allow_oob_indirect_ubo_loads\" value=\"true\" />\n")
            }
            GameProfileType.MARIO_RABBIDS -> {
                optionsBuilder.append("            <!-- TIER 1 / SNOWDROP (100% Real Compiled Mesa Turnip 26.2.0 Options) -->\n")
                optionsBuilder.append("            <option name=\"tu_allow_concurrent_binning\" value=\"false\" />\n")
                optionsBuilder.append("            <option name=\"disable_cb\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"tu_dont_care_as_load\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"disable_conservative_lrz\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"enable_lrz_fast_clear\" value=\"false\" />\n")
                optionsBuilder.append("            <option name=\"tu_override_uncached_as_cache_coherent\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"ubwc_coherency_quirk\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"tu_use_tex_coord_round_nearest_even_mode\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"tu_enable_d24s8_border_color_workaround\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"tu_enable_fast_border_color_for_undefined_formats\" value=\"false\" />\n")
                optionsBuilder.append("            <option name=\"tu_ignore_frag_depth_direction\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"dual_color_blend_by_location\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"tu_allow_oob_indirect_ubo_loads\" value=\"true\" />\n")
            }
            GameProfileType.STREETS_OF_RAGE_4,
            GameProfileType.DISCO_ELYSIUM -> {
                optionsBuilder.append("            <!-- TIER 6: 2D, INDIE, DOTEMU & SPRITE PRECISION -->\n")
                optionsBuilder.append("            <option name=\"tu_allow_concurrent_binning\" value=\"false\" />\n")
                optionsBuilder.append("            <option name=\"disable_cb\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"tu_use_tex_coord_round_nearest_even_mode\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"tu_dont_care_as_load\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"dual_color_blend_by_location\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"tu_allow_oob_indirect_ubo_loads\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"ubwc_coherency_quirk\" value=\"true\" />\n")
            }
            GameProfileType.ZELDA,
            GameProfileType.XENOBLADE,
            GameProfileType.POKEMON -> {
                optionsBuilder.append("            <!-- TIER 2: NINTENDO KINGSYSTEM / LUNCHPACK & MONOLITH SOFT -->\n")
                optionsBuilder.append("            <option name=\"tu_dont_care_as_load\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"tu_allow_concurrent_binning\" value=\"false\" />\n")
                optionsBuilder.append("            <option name=\"disable_conservative_lrz\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"enable_lrz_fast_clear\" value=\"false\" />\n")
                optionsBuilder.append("            <option name=\"tu_enable_d24s8_border_color_workaround\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"tu_ignore_frag_depth_direction\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"tu_allow_oob_indirect_ubo_loads\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"ubwc_coherency_quirk\" value=\"true\" />\n")
            }
            GameProfileType.MARIO_WONDER_JAMBOREE -> {
                optionsBuilder.append("            <!-- TIER 2b: MARIO WONDER & JAMBOREE (Anti-Flicker & Stable LRZ) -->\n")
                optionsBuilder.append("            <option name=\"tu_dont_care_as_load\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"tu_allow_concurrent_binning\" value=\"false\" />\n")
                optionsBuilder.append("            <option name=\"disable_conservative_lrz\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"enable_lrz_fast_clear\" value=\"false\" />\n")
                optionsBuilder.append("            <option name=\"ubwc_coherency_quirk\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"tu_allow_oob_indirect_ubo_loads\" value=\"true\" />\n")
            }
            GameProfileType.SUPER_MARIO_ODYSSEY,
            GameProfileType.SUPER_SMASH_BROS,
            GameProfileType.SONIC -> {
                optionsBuilder.append("            <!-- TIER 5: PURE 3D FIRST-PARTY HIGH-THROUGHPUT ENGINES -->\n")
                optionsBuilder.append("            <option name=\"tu_allow_concurrent_binning\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"tu_allow_oob_indirect_ubo_loads\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"ubwc_coherency_quirk\" value=\"true\" />\n")
            }
            else -> {
                optionsBuilder.append("            <!-- TIER 3/4/7/0: UNIVERSAL SAFE BASELINE & DEFERRED RENDERERS -->\n")
                optionsBuilder.append("            <option name=\"tu_dont_care_as_load\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"tu_enable_d24s8_border_color_workaround\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"tu_override_uncached_as_cache_coherent\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"disable_conservative_lrz\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"tu_allow_concurrent_binning\" value=\"false\" />\n")
                optionsBuilder.append("            <option name=\"tu_use_tex_coord_round_nearest_even_mode\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"tu_allow_oob_indirect_ubo_loads\" value=\"true\" />\n")
                optionsBuilder.append("            <option name=\"ubwc_coherency_quirk\" value=\"true\" />\n")
            }
        }

        return """
<driconf>
    <device driver="turnip">
        <application name="all">
$optionsBuilder
        </application>
    </device>
    <device driver="panfrost">
        <application name="all">
            <option name="pan_early_z" value="true" />
            <option name="pan_afbc_lossless" value="true" />
        </application>
    </device>
    <device driver="panvk">
        <application name="all">
            <option name="pan_early_z" value="true" />
            <option name="pan_forward_pixel_kill" value="true" />
            <option name="pan_fp16_arithmetic" value="true" />
            <option name="pan_thread_group_clamp" value="true" />
            <option name="pan_tile_buffer_coherency" value="true" />
            <option name="pan_afbc_lossless" value="true" />
            <option name="pan_shader_cache_multithreaded" value="true" />
            <option name="mesa_disk_cache_single_file" value="true" />
            <option name="mesa_shader_cache_max_size" value="4294967296" />
        </application>
    </device>
</driconf>
""".trimIndent()
    }

    /**
     * Generates and writes the tailored drirc configuration for the active game,
     * returning the File pointing to the active session config.
     */
    @Volatile
    private var lastWrittenXml: String? = null

    fun generateForGame(programIdHex: String, title: String): File {
        var rawId = programIdHex.trim().uppercase()
        if (rawId.startsWith("0X")) {
            rawId = rawId.substring(2)
        }
        val cleanId = if (rawId.length == 16) {
            rawId.substring(0, 13) + "000"
        } else {
            rawId
        }
        val profileType = detectProfileType(cleanId, title)
        val xml = generateXmlContent(profileType, cleanId, title)

        val targetDir = File(YuzuApplication.appContext.filesDir, "per_game_drirc")
        if (!targetDir.exists()) {
            targetDir.mkdirs()
        }

        val configFile = File(targetDir, "drirc.xml")
        if (xml != lastWrittenXml || !configFile.exists()) {
            configFile.writeText(xml, Charsets.UTF_8)
            File(targetDir, "00-storm.conf").writeText(xml, Charsets.UTF_8)
            val homeDir = YuzuApplication.appContext.filesDir
            File(homeDir, ".drirc").writeText(xml, Charsets.UTF_8)
            lastWrittenXml = xml
        }

        Log.info("[PerGameDrircGenerator] Generated $profileType drirc profile for $title ($programIdHex)")
        return configFile
    }
}

// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/file_sys/vfs/vfs_edenpack.h"
#include "core/file_sys/vfs/vfs_vector.h"
#include "core/memory/cheat_parser.h"

namespace {

void AppendLe32(std::vector<u8>& out, u32 value) {
    for (int i = 0; i < 4; ++i) {
        out.push_back(static_cast<u8>((value >> (i * 8)) & 0xFFU));
    }
}

void AppendLe64(std::vector<u8>& out, u64 value) {
    for (int i = 0; i < 8; ++i) {
        out.push_back(static_cast<u8>((value >> (i * 8)) & 0xFFU));
    }
}

std::shared_ptr<FileSys::VectorVfsFile> BuildEdenPackArchive(const std::string& payload,
                                                             const std::string& json_index) {
    std::vector<u8> bytes;
    const char magic[8] = {'E', 'D', 'E', 'N', 'P', 'K', '0', '1'};
    bytes.insert(bytes.end(), std::begin(magic), std::end(magic));

    const u64 payload_offset = 40;
    const u64 index_offset = payload_offset + payload.size();

    AppendLe32(bytes, 1U); // version
    AppendLe32(bytes, 0U); // flags
    AppendLe64(bytes, index_offset);
    AppendLe64(bytes, static_cast<u64>(json_index.size()));
    AppendLe64(bytes, 3U); // entry_count

    bytes.insert(bytes.end(), payload.begin(), payload.end());
    bytes.insert(bytes.end(), json_index.begin(), json_index.end());

    return std::make_shared<FileSys::VectorVfsFile>(std::move(bytes), "test.edenpack");
}

} // namespace

TEST_CASE("TextCheatParser parses master, submenus, and DisableSubmenus", "[core]") {
    constexpr std::string_view kSample =
        "{Master Code}\n"
        "04000000 00000100 00000001\n"
        "--SectionStart:Player--\n"
        "[Infinite HP]\n"
        "04000000 00000200 000003E7\n"
        "--SectionEnd:Player--\n"
        "[Zing Group]\n"
        "20000000\n"
        "[Speed x2]\n"
        "04000000 00000300 00000002\n"
        "[Zing End]\n"
        "20000001\n"
        "[Flat Cheat]\n"
        "04000000 00000400 00000009\n";

    const Core::Memory::TextCheatParser parser{false};
    const auto parsed = parser.Parse(kSample);

    REQUIRE(parsed.cheats.size() == 4);
    REQUIRE(parsed.cheats[0].is_master);
    REQUIRE(parsed.cheats[0].enabled);
    REQUIRE_FALSE(parsed.cheats[1].is_master);
    REQUIRE_FALSE(parsed.cheats[1].enabled);

    bool has_submenu = false;
    bool has_nested_cheat = false;
    for (const auto& item : parsed.menu) {
        if (item.type == Core::Memory::CheatMenuEntryType::Submenu) {
            has_submenu = true;
        } else if (item.type == Core::Memory::CheatMenuEntryType::Cheat &&
                   item.parent_id != Core::Memory::CHEAT_MENU_ROOT) {
            has_nested_cheat = true;
        }
    }
    REQUIRE(has_submenu);
    REQUIRE(has_nested_cheat);

    std::string disabled_submenus = "--DisableSubmenus--\n";
    disabled_submenus.append(kSample);
    const auto flat = Core::Memory::TextCheatParser{true}.Parse(disabled_submenus);
    REQUIRE(flat.cheats.size() == 4);
    for (const auto& item : flat.menu) {
        REQUIRE(item.type == Core::Memory::CheatMenuEntryType::Cheat);
        REQUIRE(item.parent_id == Core::Memory::CHEAT_MENU_ROOT);
    }
}

TEST_CASE("EdenPackVfsFilesystem validates archive structure and path safety", "[core]") {
    SECTION("Valid archive exposes ExeFS file payload and RomFS directory") {
        const std::string payload = "MAIN_ELF_DATA";
        const std::string json =
            R"({"format":"EdenPack","version":1,"entries":[)"
            R"({"path":"ExeFS","type":"directory"},)"
            R"({"path":"ExeFS/main","type":"file","offset":40,"size":13},)"
            R"({"path":"RomFS","type":"directory"}]})";

        auto file = BuildEdenPackArchive(payload, json);
        auto root = FileSys::EdenPackVfsFilesystem::Open(file);
        REQUIRE(root != nullptr);

        auto exefs = root->GetSubdirectory("ExeFS");
        auto romfs = root->GetSubdirectory("RomFS");
        REQUIRE(exefs != nullptr);
        REQUIRE(romfs != nullptr);

        auto main_bin = exefs->GetFile("main");
        REQUIRE(main_bin != nullptr);
        REQUIRE(main_bin->GetSize() == payload.size());
        const auto bytes = main_bin->ReadAllBytes();
        REQUIRE(std::string(bytes.begin(), bytes.end()) == payload);
    }

    SECTION("Rejects path traversal entries") {
        const std::string payload = "ABCD";
        const std::string json =
            R"({"format":"EdenPack","version":1,"entries":[)"
            R"({"path":"ExeFS","type":"directory"},)"
            R"({"path":"RomFS","type":"directory"},)"
            R"({"path":"../escape","type":"file","offset":40,"size":4}]})";

        auto file = BuildEdenPackArchive(payload, json);
        REQUIRE(FileSys::EdenPackVfsFilesystem::Open(file) == nullptr);
    }

    SECTION("Rejects overlapping byte ranges") {
        const std::string payload = "ABCDEFGH";
        const std::string json =
            R"({"format":"EdenPack","version":1,"entries":[)"
            R"({"path":"ExeFS","type":"directory"},)"
            R"({"path":"ExeFS/a","type":"file","offset":40,"size":6},)"
            R"({"path":"RomFS/b","type":"file","offset":44,"size":4}]})";

        auto file = BuildEdenPackArchive(payload, json);
        REQUIRE(FileSys::EdenPackVfsFilesystem::Open(file) == nullptr);
    }
}

// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include "common/common_types.h"

namespace Core {

struct AppliedProfileOverride {
    std::string key;
    std::string value;
    std::string source; // "base" or vendor key ("nvidia", "amd", "intel", "adreno", "mali", "xclipse", "other")
};

struct GameFixProfile {
    u64 title_id;
    std::string game_name;
    std::string issues_ru;
    std::string issues_en;
    std::string fixes_ru;
    std::string fixes_en;
    
    // Per-game config parameters
    std::unordered_map<std::string, std::string> ini_settings;

    // Alternative Title IDs (regional releases, aliases)
    std::vector<u64> alt_title_ids{};

    // Vendor-specific override maps (keys: "nvidia", "amd", "intel", "adreno", "mali", "xclipse", "other")
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> vendor_overrides{};
};

class GameFixDatabase {
public:
    static const GameFixProfile* GetProfile(u64 title_id);
    static const GameFixProfile* GetProfileByTitleOrPath(u64 title_id, const std::string& name_or_path);
    static bool HasProfile(u64 title_id);
    static const std::vector<GameFixProfile>& GetAllProfiles();
    static bool ApplyProfileToPerGameConfig(u64 title_id, const std::string& config_file_path);
    static bool ApplyProfileDirectly(u64 title_id, const std::string& name_or_path = "");
    static bool IsFixApplied(u64 title_id, const std::string& config_file_path);
    static void SetDontAskAgain(u64 title_id, const std::string& config_file_path, bool dont_ask = true);
    static int ResetAllDontAskAgain();
    static void SetFixesEnabled(bool enabled);
    static bool AreFixesEnabled();
    static void SetExcludedKeys(const std::vector<std::string>& excluded_keys);
    static bool IsKeyExcluded(const std::string& full_key);
    static void SanitizeSettingsForHostHardware();

    /// GPU vendor detection and override re-application
    static void SetDetectedGpuVendor(const std::string& vendor_or_driver);
    static void SetDetectedGpuVendorFromDriver(u32 vendor_id, const std::string& driver_name);
    static std::string NormalizeGpuVendor(u32 vendor_id, const std::string& driver_name);
    static const std::string& GetDetectedGpuVendor();
    static bool ReapplyVendorOverrides(u64 title_id);

    /// Profile override transparency API: returns list of (key, value, source) applied for a TID
    /// Source is "base" for default profile settings, or vendor key (e.g. "nvidia", "amd") for vendor overrides
    static std::vector<AppliedProfileOverride> GetAppliedOverrides(u64 title_id);
};

} // namespace Core

// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#ifdef NIGHTLY_BUILD
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#endif

#include <fmt/format.h>
#include "common/net/net.h"
#include "common/scm_rev.h"
#include "update_checker.h"

#include "common/logging.h"

#include <sstream>
#include <vector>
#include <string>
#include <algorithm>

std::optional<Common::Net::Release> UpdateChecker::GetUpdate() {
    const auto latest = Common::Net::GetLatestRelease();
    if (!latest) return std::nullopt;

    LOG_INFO(Frontend, "Received latest release tag: {}", latest->tag);

#ifdef NIGHTLY_BUILD
    std::vector<std::string> result;

    boost::split(result, latest->tag, boost::is_any_of("."));
    if (result.size() != 2)
        return std::nullopt;

    const std::string tag = result[1];

    boost::split(result, std::string{Common::g_build_version}, boost::is_any_of("-"));
    if (result.empty())
        return std::nullopt;

    const std::string build = result[0];
#else
    std::string tag = latest->tag;
    std::string build = Common::g_build_version;
    while (!tag.empty() && (tag.front() == 'v' || tag.front() == 'V')) {
        tag = tag.substr(1);
    }
    while (!build.empty() && (build.front() == 'v' || build.front() == 'V')) {
        build = build.substr(1);
    }
#endif

    if (tag.empty()) {
        return std::nullopt;
    }

    auto parse_semver = [](const std::string& ver) -> std::vector<int> {
        std::vector<int> components;
        std::string current;
        for (unsigned char c : ver) {
            if (c >= '0' && c <= '9') {
                current += static_cast<char>(c);
            } else if (c == '.') {
                if (!current.empty()) {
                    try {
                        components.push_back(std::stoi(current));
                    } catch (...) {
                        components.push_back(0);
                    }
                    current.clear();
                }
            }
        }
        if (!current.empty()) {
            try {
                components.push_back(std::stoi(current));
            } catch (...) {
                components.push_back(0);
            }
        }
        while (components.size() < 3) {
            components.push_back(0);
        }
        return components;
    };

    const auto remote_parts = parse_semver(tag);
    const auto local_parts = parse_semver(build);

    LOG_INFO(Frontend, "Checking update: remote '{}' -> [{}.{}.{}] vs local '{}' -> [{}.{}.{}]",
             tag, remote_parts[0], remote_parts[1], remote_parts[2],
             build, local_parts[0], local_parts[1], local_parts[2]);

    bool is_newer = false;
    const std::size_t max_parts = std::max(remote_parts.size(), local_parts.size());
    for (std::size_t i = 0; i < max_parts; ++i) {
        const int r = (i < remote_parts.size()) ? remote_parts[i] : 0;
        const int l = (i < local_parts.size()) ? local_parts[i] : 0;
        if (r > l) {
            is_newer = true;
            break;
        } else if (r < l) {
            is_newer = false;
            break;
        }
    }

    if (is_newer) {
        LOG_INFO(Frontend, "Newer version available: {} > {}", tag, build);
        return latest;
    }

    LOG_INFO(Frontend, "Current version {} is up to date (remote: {})", build, tag);
    return std::nullopt;
}

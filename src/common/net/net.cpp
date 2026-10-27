// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <optional>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>

#include <fmt/format.h>
#include "common/scm_rev.h"
#include "net.h"

#include "common/logging.h"

#include "common/httplib.h"

#ifdef YUZU_BUNDLED_OPENSSL
#include <openssl/cert.h>
#endif

#define QT_TR_NOOP(x) x

namespace Common::Net {

std::vector<Asset> Release::GetPlatformAssets() const {
    // TODO(crueter): Need better handling for this as a whole.
#ifdef NIGHTLY_BUILD
    std::vector<std::string> result;
    boost::algorithm::split(result, tag, boost::is_any_of("."));
    if (result.size() != 2)
        return {};
    const auto ref = result.at(1);
#else
    const auto ref = tag;
#endif

    std::vector<Asset> found_assets;

    // FIXME: This is mildly inefficient.
    // Finds assets based on a hierarchy of regex search strings.
    const auto find_asset = [&found_assets, ref, this](const std::string& name,
                                                       const std::vector<std::string>& suffixes) {
        for (const std::string& asset : assets) {
            for (const auto& suffix : suffixes) {
                if (asset.ends_with(suffix)) {
                    const std::string_view asset_sv = asset;
                    const size_t pos = asset_sv.find_last_of('/');
                    const std::string_view filename =
                        (pos != std::string_view::npos) ? asset_sv.substr(pos + 1) : asset_sv;

                    found_assets.emplace_back(Asset{
                        .name = name,
                        .url = host,
                        .path = asset,
                        .filename = std::string{filename},
                    });
                    return;
                }
            }
        }
    };

#ifdef _WIN32
    find_asset("Standard", {"win-x64.zip", ".zip", ".7z", ".exe"});
#elif defined(__APPLE__)
#ifdef ARCHITECTURE_arm64
    find_asset("Standard", {"standard.dmg", "standard.tar.gz", ".dmg", ".tar.gz"});
    find_asset("PGO", {"pgo.dmg", "pgo.tar.gz"});
#endif // ARCHITECTURE_arm64
#elif defined(__ANDROID__)
    find_asset("Standard", {"Android.apk", ".apk"});
#endif // __APPLE__
    return found_assets;
}

static inline u64 ParseIsoTimestamp(const std::string& iso) {
    if (iso.empty())
        return 0;

    std::string buf = iso;
    if (buf.back() == 'Z')
        buf.pop_back();

    std::tm tm{};
    std::istringstream ss(buf);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (ss.fail())
        return 0;

#ifdef _WIN32
    return static_cast<u64>(_mkgmtime(&tm));
#else
    return static_cast<u64>(timegm(&tm));
#endif
}

std::optional<Release> Release::FromJson(const nlohmann::json& json, const std::string& host,
                                         const std::string& repo) {
    Release rel;
    if (!json.is_object())
        return std::nullopt;

    rel.tag = json.value("tag_name", std::string{});
    if (rel.tag.empty())
        return std::nullopt;

    rel.title = json.value("name", rel.tag);
    rel.id = json.value("id", std::hash<std::string>{}(rel.title));

    rel.published = ParseIsoTimestamp(json.value("published_at", std::string{}));
    rel.prerelease = json.value("prerelease", false);

    auto body = json.value("body", rel.title);
    boost::replace_all(body, "\\r", "");
    boost::replace_all(body, "\\n", "\n");
    rel.body = body;

    rel.host = host;

    const auto release_base =
        fmt::format("{}/{}/releases", Common::g_build_auto_update_website, repo);
    const auto fallback_html = fmt::format("{}/tag/{}", release_base, rel.tag);
    rel.html_url = json.value("html_url", fallback_html);

    // This is our own "fake" API.
    if (json.contains("base")) {
        const auto base = json.value("base", fmt::format("https://{}", Common::g_build_auto_update_api));
        rel.base_download_url = fmt::format("{}/{}", base, rel.tag);

        // Assets are easy :)
        rel.assets = json.value("assets", std::vector<std::string>{});
    } else {
        const auto base_download_url = fmt::format("/{}/releases/download/{}", repo, rel.tag);

        rel.base_download_url = base_download_url;

        // assets are a bit more complex here. :(
        std::vector<std::string> assets;
        const nlohmann::json& arr = json["assets"];
        for (const auto &obj : arr) {
            const auto url = obj.value("browser_download_url", std::string{});
            assets.emplace_back(url);
        }

        rel.assets = assets;
    }

    return rel;
}

std::optional<Release> Release::FromJson(const std::string_view& json, const std::string& host,
                                         const std::string& repo) {
    try {
        return FromJson(nlohmann::json::parse(json), host, repo);
    } catch (std::exception& e) {
        LOG_WARNING(Common, "Failed to parse JSON: {}", e.what());
    }

    return {};
}

std::vector<Release> Release::ListFromJson(const nlohmann::json& json, const std::string& host,
                                           const std::string& repo) {
    if (!json.is_array())
        return {};

    std::vector<Release> releases;
    for (const auto& obj : json) {
        auto rel = Release::FromJson(obj, host, repo);
        if (rel)
            releases.emplace_back(rel.value());
    }
    return releases;
}

std::vector<Release> Release::ListFromJson(const std::string_view& json, const std::string& host,
                                           const std::string& repo) {
    try {
        return ListFromJson(nlohmann::json::parse(json), host, repo);
    } catch (std::exception& e) {
        LOG_WARNING(Common, "Failed to parse JSON: {}", e.what());
    }

    return {};
}

std::optional<std::string> MakeRequest(const std::string& url, const std::string& path) {
    try {
        constexpr std::size_t timeout_seconds = 10;

        std::unique_ptr<httplib::Client> client = std::make_unique<httplib::Client>(url);
        client->set_connection_timeout(timeout_seconds);
        client->set_read_timeout(timeout_seconds);
        client->set_write_timeout(timeout_seconds);

#ifdef YUZU_BUNDLED_OPENSSL
        client->load_ca_cert_store(kCert, sizeof(kCert));
#endif

        if (client == nullptr) {
            LOG_ERROR(Common, "Invalid URL {}{}", url, path);
            return {};
        }

        httplib::Headers headers = {
            {"User-Agent", fmt::format("STORM-SWITCH/{}", Common::g_build_version)},
            {"Accept", "application/vnd.github.v3+json, application/json, */*"},
        };

        httplib::Request request{
            .method = "GET",
            .path = path,
            .headers = headers,
        };

        client->set_follow_location(true);
        httplib::Result result = client->send(request);

        if (!result) {
            LOG_DEBUG(Common, "GET to {}{} returned null", url, path);
            return {};
        }

        const auto& response = result.value();
        if (response.status >= 400) {
            if (response.status == 403 || response.status == 429 || response.status == 404) {
                LOG_DEBUG(Common, "GET to {}{} returned status code: {}", url, path, response.status);
            } else {
                LOG_ERROR(Common, "GET to {}{} returned error status code: {}", url, path,
                          response.status);
            }
            return {};
        }
        if (response.body.empty()) {
            LOG_DEBUG(Common, "GET to {}{} returned empty body", url, path);
            return {};
        }

        return response.body;
    } catch (std::exception& e) {
        LOG_DEBUG(Common, "GET to {}{} failed during update check: {}", url, path, e.what());
        return std::nullopt;
    }
}

static std::vector<Release> ParseStormGamesWorldReleases(const std::string& body_str) {
    std::vector<Release> out;
    try {
        const auto json = nlohmann::json::parse(body_str);
        if (!json.contains("repositories") || !json["repositories"].is_array() || json["repositories"].empty())
            return out;
        const auto& repo = json["repositories"][0];
        if (!repo.contains("releases") || !repo["releases"].is_array())
            return out;
        for (const auto& rj : repo["releases"]) {
            if (!rj.is_object())
                continue;
            Release rel{};
            rel.tag = rj.value("version", std::string{});
            if (!rel.tag.empty() && (rel.tag[0] == 'v' || rel.tag[0] == 'V'))
                rel.tag = rel.tag.substr(1);
            if (rel.tag.empty())
                continue;
            rel.title = rj.value("title", "STORM SWITCH " + rel.tag);
            rel.id = std::hash<std::string>{}(rel.title);
            rel.published = ParseIsoTimestamp(rj.value("publishedAt", std::string{}));
            rel.prerelease = (rj.value("channel", std::string{"stable"}) != "stable");
            rel.body = rj.value("notes", rel.title);
            boost::replace_all(rel.body, "\r", "");
            rel.host = "https://stormgamesworld.ru";
            rel.html_url = "https://stormgamesworld.ru/programs?repo=STORM_SWITCH";
            rel.base_download_url = "https://stormgamesworld.ru/uploads/programs/STORM_SWITCH";
            if (rj.contains("assets") && rj["assets"].is_array()) {
                for (const auto& aj : rj["assets"]) {
                    const auto name = aj.value("name", std::string{});
                    if (!name.empty()) {
                        rel.assets.emplace_back("https://stormgamesworld.ru/uploads/programs/STORM_SWITCH/" + name);
                    }
                }
            }
            out.emplace_back(std::move(rel));
        }
    } catch (...) {
    }
    return out;
}

static bool IsVerNewer(const std::string& remote_v, const std::string& local_v) {
    auto parse_parts = [](const std::string& s) {
        std::vector<int> parts;
        std::string cur;
        for (char c : s) {
            if (c >= '0' && c <= '9') {
                cur.push_back(c);
            } else if (!cur.empty()) {
                parts.push_back(std::atoi(cur.c_str()));
                cur.clear();
            }
        }
        if (!cur.empty())
            parts.push_back(std::atoi(cur.c_str()));
        return parts;
    };
    const auto r = parse_parts(remote_v);
    const auto l = parse_parts(local_v);
    const size_t n = std::max(r.size(), l.size());
    for (size_t i = 0; i < n; ++i) {
        const int rv = i < r.size() ? r[i] : 0;
        const int lv = i < l.size() ? l[i] : 0;
        if (rv != lv)
            return rv > lv;
    }
    return false;
}

std::vector<Release> GetReleases() {
    std::vector<Release> gh_releases;
    const auto body = GetReleasesBody();
    if (body) {
        const std::string_view body_str = body.value();
        const auto url = fmt::format("https://{}", Common::g_build_auto_update_stable_api);
        gh_releases = Release::ListFromJson(body_str, url, Common::g_build_auto_update_stable_repo);
    }

    const auto sgw_body = MakeRequest("https://stormgamesworld.ru", "/api/programs?repo=STORM_SWITCH");
    if (sgw_body) {
        auto sgw_releases = ParseStormGamesWorldReleases(sgw_body.value());
        if (!sgw_releases.empty()) {
            if (gh_releases.empty() || IsVerNewer(sgw_releases.front().tag, gh_releases.front().tag)) {
                return sgw_releases;
            }
        }
    }

    return gh_releases;
}

std::optional<Release> GetLatestRelease() {
    std::optional<Release> gh_rel;
    const auto releases_path = Common::g_build_auto_update_api_path;
    const auto url = fmt::format("https://{}", Common::g_build_auto_update_api);

    const auto body = MakeRequest(url, releases_path);
    if (body) {
        const std::string_view body_str = body.value();
        gh_rel = Release::FromJson(body_str, url, Common::g_build_auto_update_repo);
    }

    const auto sgw_body = MakeRequest("https://stormgamesworld.ru", "/api/programs?repo=STORM_SWITCH");
    if (sgw_body) {
        auto sgw_releases = ParseStormGamesWorldReleases(sgw_body.value());
        if (!sgw_releases.empty()) {
            if (!gh_rel.has_value() || IsVerNewer(sgw_releases.front().tag, gh_rel->tag)) {
                return sgw_releases.front();
            }
        }
    }

    return gh_rel;
}

std::optional<std::string> GetReleasesBody() {
    const auto releases_path =
        fmt::format("/{}/{}/releases", Common::g_build_auto_update_stable_api_path,
                    Common::g_build_auto_update_stable_repo);
    const auto url = fmt::format("https://{}", Common::g_build_auto_update_stable_api);

    return MakeRequest(url, releases_path);
}

} // namespace Common::Net

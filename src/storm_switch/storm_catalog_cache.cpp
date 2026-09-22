// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "storm_switch/storm_catalog_cache.h"
#include <algorithm>
#include <thread>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include "common/fs/fs.h"
#include "common/fs/path_util.h"

StormCatalogCache& StormCatalogCache::Instance() {
    static StormCatalogCache instance;
    return instance;
}

StormCatalogCache::StormCatalogCache() {
    std::thread(&StormCatalogCache::LoadCache, this).detach();
}

QString StormCatalogCache::GetCacheFilePath() const {
    const auto cache_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir);
    const QString cache_dir_q = QString::fromStdString(Common::FS::PathToUTF8String(cache_dir));
    return QDir(cache_dir_q).filePath(QStringLiteral("storm_games_world_catalog.json"));
}

void StormCatalogCache::LoadCache() {
    bool expected = false;
    if (!m_loading.compare_exchange_strong(expected, true)) {
        return;
    }
    const QString path = GetCacheFilePath();
    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        m_loading = false;
        return;
    }
    const QByteArray data = file.readAll();
    file.close();
    ParseCatalogJson(data);
    m_loaded = true;
    m_loading = false;
}

void StormCatalogCache::SaveCache(const QByteArray& json_data) {
    const QString path = GetCacheFilePath();
    QFileInfo fi(path);
    QDir().mkpath(fi.absolutePath());

    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(json_data);
        file.close();
    }
    ParseCatalogJson(json_data);
    m_loaded = true;
}

QByteArray StormCatalogCache::GetRawCacheData() const {
    const QString path = GetCacheFilePath();
    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    const QByteArray data = file.readAll();
    file.close();
    return data;
}

void StormCatalogCache::ParseCatalogJson(const QByteArray& data) {
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        return;
    }

    QMap<u64, QString> temp_versions;
    const QJsonArray arr = doc.array();

    for (const auto& val : arr) {
        if (!val.isObject()) continue;
        const QJsonObject obj = val.toObject();

        const QString platform = obj[QStringLiteral("platformName")].toString();
        const QString platform_type = obj[QStringLiteral("platformTypeName")].toString();
        const bool file_exists = obj[QStringLiteral("fileExists")].toBool();
        const bool has_file = obj[QStringLiteral("hasFile")].toBool();

        if (platform == QStringLiteral("Nintendo Switch") &&
            platform_type == QStringLiteral("CONSOLES") &&
            file_exists && has_file) {

            const QString serial_id = obj[QStringLiteral("serialId")].toString().trimmed();
            const QString ver = obj[QStringLiteral("version")].toString().trimmed();

            if (!serial_id.isEmpty() && !ver.isEmpty()) {
                bool ok = false;
                const u64 tid = serial_id.toULongLong(&ok, 16);
                if (ok && tid != 0) {
                    const u64 base_tid = (tid & ~0x800ULL);
                    if (!temp_versions.contains(base_tid) ||
                        CompareVersions(ver, temp_versions[base_tid]) > 0) {
                        temp_versions[base_tid] = ver;
                    }
                    if (!temp_versions.contains(tid) ||
                        CompareVersions(ver, temp_versions[tid]) > 0) {
                        temp_versions[tid] = ver;
                    }
                }
            }
        }
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_latest_versions = std::move(temp_versions);
}

bool StormCatalogCache::HasUpdate(u64 title_id, const QString& installed_ver) const {
    if (title_id == 0 || installed_ver.trimmed().isEmpty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    QString catalog_ver;
    if (m_latest_versions.contains(title_id)) {
        catalog_ver = m_latest_versions.value(title_id);
    } else {
        const u64 base_tid = (title_id & ~0x800ULL);
        if (m_latest_versions.contains(base_tid)) {
            catalog_ver = m_latest_versions.value(base_tid);
        }
    }

    if (catalog_ver.isEmpty()) {
        return false;
    }

    return CompareVersions(catalog_ver, installed_ver) > 0;
}

QString StormCatalogCache::GetLatestVersion(u64 title_id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_latest_versions.contains(title_id)) {
        return m_latest_versions.value(title_id);
    }
    const u64 base_tid = (title_id & ~0x800ULL);
    return m_latest_versions.value(base_tid, QString());
}

static std::vector<int> ParseSemanticNumbers(const QString& v) {
    QString clean = v.trimmed();
    if (clean.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        clean.remove(0, 1);
    }
    const auto paren_idx = clean.indexOf(QLatin1Char('('));
    if (paren_idx > 0) clean = clean.left(paren_idx).trimmed();
    const auto bracket_idx = clean.indexOf(QLatin1Char('['));
    if (bracket_idx > 0) clean = clean.left(bracket_idx).trimmed();

    const QStringList parts = clean.split(QLatin1Char('.'));
    std::vector<int> nums;
    for (const auto& p : parts) {
        bool ok = false;
        int n = p.toInt(&ok);
        if (ok) {
            nums.push_back(n);
        }
    }
    return nums;
}

int StormCatalogCache::CompareVersions(const QString& v1, const QString& v2) {
    if (v1.trimmed().compare(v2.trimmed(), Qt::CaseInsensitive) == 0) {
        return 0;
    }

    const auto nums1 = ParseSemanticNumbers(v1);
    const auto nums2 = ParseSemanticNumbers(v2);

    if (!nums1.empty() && !nums2.empty()) {
        if (nums1.size() == 1 && nums1[0] > 1000 && nums2.size() > 1) {
            const int b2 = (nums2.size() >= 3) ? (nums2[0] * 655360 + nums2[1] * 655360 + nums2[2] * 65536) : (nums2[0] * 655360);
            if (nums1[0] > b2) return 1;
            if (nums1[0] < b2) return -1;
            return 0;
        }
        if (nums2.size() == 1 && nums2[0] > 1000 && nums1.size() > 1) {
            const int b1 = (nums1.size() >= 3) ? (nums1[0] * 655360 + nums1[1] * 655360 + nums1[2] * 65536) : (nums1[0] * 655360);
            if (b1 > nums2[0]) return 1;
            if (b1 < nums2[0]) return -1;
            return 0;
        }

        const size_t max_len = std::max(nums1.size(), nums2.size());
        for (size_t i = 0; i < max_len; ++i) {
            const int n1 = (i < nums1.size()) ? nums1[i] : 0;
            const int n2 = (i < nums2.size()) ? nums2[i] : 0;
            if (n1 > n2) return 1;
            if (n1 < n2) return -1;
        }
        return 0;
    }

    return v1.compare(v2, Qt::CaseInsensitive);
}

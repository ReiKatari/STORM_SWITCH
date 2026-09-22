// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <mutex>
#include <tuple>
#include <vector>
#include <QMap>
#include <QString>
#include "common/common_types.h"

class StormCatalogCache {
public:
    static StormCatalogCache& Instance();

    void LoadCache();
    void SaveCache(const QByteArray& json_data);
    QByteArray GetRawCacheData() const;

    bool HasUpdate(u64 title_id, const QString& installed_ver) const;
    QString GetLatestVersion(u64 title_id) const;

    static int CompareVersions(const QString& v1, const QString& v2);

private:
    StormCatalogCache();
    QString GetCacheFilePath() const;
    void ParseCatalogJson(const QByteArray& data);

    mutable std::mutex m_mutex;
    QMap<u64, QString> m_latest_versions;
    std::atomic<bool> m_loaded{false};
    std::atomic<bool> m_loading{false};
};

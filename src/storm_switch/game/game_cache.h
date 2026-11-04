// SPDX-FileCopyrightText: 2026 STORM SOFT
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <QImage>
#include <QObject>
#include <QPersistentModelIndex>
#include <QPixmap>
#include <QStandardItemModel>
#include <QString>

#include "common/common_types.h"

/**
 * Asynchronous, non-blocking cover / thumbnail loader with QPixmapCache integration.
 * Ensures that loading or decoding covers NEVER blocks the Qt UI main thread.
 */
class GameCoverLoader : public QObject {
    Q_OBJECT

public:
    static GameCoverLoader& Instance();

    /// Request asynchronous background loading and caching of game cover
    void RequestCover(u64 program_id, const QString& icon_path, int size,
                      const QPersistentModelIndex& index, QAbstractItemModel* model);

    /// Get cover from memory cache if available (0 ms)
    bool TryGetCover(u64 program_id, int size, QPixmap* out_pixmap) const;

    /// Clear all pending requests and memory caches
    void Clear();

private:
    GameCoverLoader();
    ~GameCoverLoader() override;

    static QString MakeCacheKey(u64 program_id, int size);

    mutable std::mutex m_mutex;
    std::unordered_set<u64> m_pending_requests;
};

// SPDX-FileCopyrightText: 2026 STORM SOFT
// SPDX-License-Identifier: GPL-3.0-or-later

#include "storm_switch/game/game_cache.h"

#include <QDir>
#include <QFile>
#include <QImageReader>
#include <QMetaObject>
#include <QPixmapCache>
#include <QThreadPool>

#include <fmt/format.h>

#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "qt_common/game_list/game_list_p.h"

GameCoverLoader& GameCoverLoader::Instance() {
    static GameCoverLoader s_instance;
    return s_instance;
}

GameCoverLoader::GameCoverLoader() {
    // Increase QPixmapCache limit to 64 MB for silky smooth 60 FPS scrolling
    if (QPixmapCache::cacheLimit() < 64 * 1024) {
        QPixmapCache::setCacheLimit(64 * 1024);
    }
}

GameCoverLoader::~GameCoverLoader() {
    Clear();
}

QString GameCoverLoader::MakeCacheKey(u64 program_id, int size) {
    return QStringLiteral("storm_cover_%1_%2").arg(program_id, 16, 16, QLatin1Char('0')).arg(size);
}

bool GameCoverLoader::TryGetCover(u64 program_id, int size, QPixmap* out_pixmap) const {
    if (program_id == 0) {
        return false;
    }
    const QString key = MakeCacheKey(program_id, size);
    return QPixmapCache::find(key, out_pixmap);
}

void GameCoverLoader::Clear() {
    std::scoped_lock lock(m_mutex);
    m_pending_requests.clear();
}

void GameCoverLoader::RequestCover(u64 program_id, const QString& icon_path, int size,
                                   const QPersistentModelIndex& index, QAbstractItemModel* model) {
    if (program_id == 0 || !model || !index.isValid()) {
        return;
    }

    const QString cache_key = MakeCacheKey(program_id, size);
    QPixmap existing;
    if (QPixmapCache::find(cache_key, &existing)) {
        model->setData(index, existing, GameListItemPath::ScaledPixmapRole);
        model->setData(index, existing, Qt::DecorationRole);
        return;
    }

    {
        std::scoped_lock lock(m_mutex);
        if (m_pending_requests.contains(program_id)) {
            return; // Already being loaded asynchronously in thread pool
        }
        m_pending_requests.insert(program_id);
    }

    // Launch low-overhead task in global thread pool
    QThreadPool::globalInstance()->start([this, program_id, icon_path, size, index, model, cache_key]() {
        QImage loaded_image;

        // 1. Check pre-scaled thumbnail in icons/ directory
        const auto cache_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) / "game_list";
        const auto pre_thumb = cache_dir / "icons" / fmt::format("{:016X}.png", program_id);
        const auto legacy_jpeg = cache_dir / fmt::format("{:016X}.jpeg", program_id);

        const QString pre_thumb_str = QString::fromStdString(Common::FS::PathToUTF8String(pre_thumb));
        const QString legacy_jpeg_str = QString::fromStdString(Common::FS::PathToUTF8String(legacy_jpeg));

        if (QFile::exists(pre_thumb_str)) {
            QImageReader reader(pre_thumb_str);
            loaded_image = reader.read();
        } else if (!icon_path.isEmpty() && QFile::exists(icon_path)) {
            QImageReader reader(icon_path);
            loaded_image = reader.read();
        } else if (QFile::exists(legacy_jpeg_str)) {
            QImageReader reader(legacy_jpeg_str);
            loaded_image = reader.read();
        }

        QPixmap pixmap;
        if (!loaded_image.isNull()) {
            if (loaded_image.width() != size || loaded_image.height() != size) {
                loaded_image = loaded_image.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
            pixmap = QPixmap::fromImage(loaded_image);
        }

        // Return to main thread via QMetaObject::invokeMethod
        QMetaObject::invokeMethod(model, [this, program_id, cache_key, pixmap, index, model]() {
            {
                std::scoped_lock lock(m_mutex);
                m_pending_requests.erase(program_id);
            }

            if (!pixmap.isNull()) {
                QPixmapCache::insert(cache_key, pixmap);
                if (index.isValid()) {
                    model->setData(index, pixmap, GameListItemPath::ScaledPixmapRole);
                    model->setData(index, pixmap, Qt::DecorationRole);
                }
            }
        }, Qt::QueuedConnection);
    });
}

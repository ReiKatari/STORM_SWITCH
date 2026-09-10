// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <QString>
#include <QStringList>

namespace QtCommon::Mod {

QStringList GetModFolders(const QString& root, const QString& fallbackName);

const QString ExtractMod(const QString& path);

bool OrganizeModStructure(const std::filesystem::path& source_dir, const std::filesystem::path& target_dir);

} // namespace QtCommon::Mod

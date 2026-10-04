// SPDX-FileCopyrightText: 2026 STORM SOFT
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <QLocale>
#include <QString>
#include "storm_switch/uisettings.h"

inline QString StormLang(const QString& ru, const QString& en,
                         const QString& de = QString(), const QString& fr = QString(),
                         const QString& zh = QString(), const QString& ja = QString(),
                         const QString& ar = QString(), const QString& es = QString()) {
    std::string lang = UISettings::values.language.GetValue();
    if (lang.empty()) {
        lang = QLocale::system().name().toStdString();
    }
    if (lang.rfind("ru", 0) == 0) return ru;
    if (lang.rfind("ar", 0) == 0 && !ar.isEmpty()) return ar;
    if (lang.rfind("es", 0) == 0 && !es.isEmpty()) return es;
    if (lang.rfind("de", 0) == 0 && !de.isEmpty()) return de;
    if (lang.rfind("fr", 0) == 0 && !fr.isEmpty()) return fr;
    if (lang.rfind("zh", 0) == 0 && !zh.isEmpty()) return zh;
    if (lang.rfind("ja", 0) == 0 && !ja.isEmpty()) return ja;
    return en;
}

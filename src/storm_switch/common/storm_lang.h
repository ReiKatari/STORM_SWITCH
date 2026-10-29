// SPDX-FileCopyrightText: 2026 STORM SOFT
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <QLocale>
#include <QString>
#include "qt_common/config/uisettings.h"

QString StormLookupMultiLang(const QString& en_key, const std::string& lang);

inline QString StormLang(const QString& ru, const QString& en,
                         const QString& de = QString(), const QString& fr = QString(),
                         const QString& zh = QString(), const QString& ja = QString(),
                         const QString& ar = QString(), const QString& es = QString()) {
    std::string lang = UISettings::values.language.GetValue();
    if (lang.empty()) {
        lang = QLocale::system().name().toStdString();
    }
    if (lang.rfind("ru", 0) == 0) return ru;
    if (lang.rfind("en", 0) == 0) return en;
    if (lang.rfind("ar", 0) == 0 && !ar.isEmpty()) return ar;
    if (lang.rfind("es", 0) == 0 && !es.isEmpty()) return es;
    if (lang.rfind("de", 0) == 0 && !de.isEmpty()) return de;
    if (lang.rfind("fr", 0) == 0 && !fr.isEmpty()) return fr;
    if (lang.rfind("zh", 0) == 0 && !zh.isEmpty()) return zh;
    if (lang.rfind("ja", 0) == 0 && !ja.isEmpty()) return ja;

    const QString ml = StormLookupMultiLang(en, lang);
    if (!ml.isEmpty()) {
        return ml;
    }
    return en;
}

inline QString StormLang11(const QString& ru, const QString& en,
                           const QString& de, const QString& fr,
                           const QString& es, const QString& it,
                           const QString& pt, const QString& zh,
                           const QString& ja, const QString& ko,
                           const QString& ar) {
    std::string lang = UISettings::values.language.GetValue();
    if (lang.empty()) {
        lang = QLocale::system().name().toStdString();
    }
    if (lang.rfind("ru", 0) == 0) return ru;
    if (lang.rfind("en", 0) == 0) return en;
    if (lang.rfind("de", 0) == 0 && !de.isEmpty()) return de;
    if (lang.rfind("fr", 0) == 0 && !fr.isEmpty()) return fr;
    if (lang.rfind("es", 0) == 0 && !es.isEmpty()) return es;
    if (lang.rfind("it", 0) == 0 && !it.isEmpty()) return it;
    if (lang.rfind("pt", 0) == 0 && !pt.isEmpty()) return pt;
    if (lang.rfind("zh", 0) == 0 && !zh.isEmpty()) return zh;
    if (lang.rfind("ja", 0) == 0 && !ja.isEmpty()) return ja;
    if (lang.rfind("ko", 0) == 0 && !ko.isEmpty()) return ko;
    if (lang.rfind("ar", 0) == 0 && !ar.isEmpty()) return ar;
    return en;
}

inline QString StormCancelText() {
    std::string lang = UISettings::values.language.GetValue();
    if (lang.empty()) {
        lang = QLocale::system().name().toStdString();
    }
    if (lang.rfind("ru", 0) == 0 || lang.rfind("be", 0) == 0) return QStringLiteral("Отмена");
    if (lang.rfind("uk", 0) == 0) return QStringLiteral("Скасувати");
    if (lang.rfind("de", 0) == 0) return QStringLiteral("Abbrechen");
    if (lang.rfind("fr", 0) == 0) return QStringLiteral("Annuler");
    if (lang.rfind("es", 0) == 0) return QStringLiteral("Cancelar");
    if (lang.rfind("it", 0) == 0) return QStringLiteral("Annulla");
    if (lang.rfind("pt", 0) == 0) return QStringLiteral("Cancelar");
    if (lang.rfind("zh", 0) == 0) return QStringLiteral("取消");
    if (lang.rfind("ja", 0) == 0) return QStringLiteral("キャンセル");
    if (lang.rfind("ko", 0) == 0) return QStringLiteral("취소");
    if (lang.rfind("pl", 0) == 0) return QStringLiteral("Anuluj");
    if (lang.rfind("ar", 0) == 0) return QStringLiteral("إلغاء");
    return QStringLiteral("Cancel");
}

inline QString StormCancelButtonStyle() {
    return QStringLiteral(
        "QPushButton, QPushButton[isCancelButton=\"true\"], QDialogButtonBox QPushButton[isCancelButton=\"true\"] {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #EF4444, stop:1 #DC2626) !important;"
        "    color: #FFFFFF !important;"
        "    font-weight: 700 !important;"
        "    font-size: 12px !important;"
        "    padding: 6px 18px !important;"
        "    border-radius: 7px !important;"
        "    border: 1px solid #F87171 !important;"
        "    min-height: 24px !important;"
        "}"
        "QPushButton:hover, QPushButton[isCancelButton=\"true\"]:hover, QDialogButtonBox QPushButton[isCancelButton=\"true\"]:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #F87171, stop:1 #EF4444) !important;"
        "    border: 1px solid #FCA5A5 !important;"
        "    color: #FFFFFF !important;"
        "}"
        "QPushButton:pressed, QPushButton[isCancelButton=\"true\"]:pressed, QDialogButtonBox QPushButton[isCancelButton=\"true\"]:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #B91C1C, stop:1 #7F1D1D) !important;"
        "    border: 1px solid #991B1B !important;"
        "    color: #FFFFFF !important;"
        "}"
    );
}
inline QString StormCancelButtonQssRule() {
    return QStringLiteral(
        "\nQPushButton[isCancelButton=\"true\"], QDialogButtonBox QPushButton[isCancelButton=\"true\"] {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #EF4444, stop:1 #DC2626) !important;"
        "    color: #FFFFFF !important;"
        "    font-weight: 700 !important;"
        "    font-size: 12px !important;"
        "    padding: 6px 18px !important;"
        "    border-radius: 7px !important;"
        "    border: 1px solid #F87171 !important;"
        "    min-height: 24px !important;"
        "}"
        "\nQPushButton[isCancelButton=\"true\"]:hover, QDialogButtonBox QPushButton[isCancelButton=\"true\"]:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #F87171, stop:1 #EF4444) !important;"
        "    border: 1px solid #FCA5A5 !important;"
        "    color: #FFFFFF !important;"
        "}"
        "\nQPushButton[isCancelButton=\"true\"]:pressed, QDialogButtonBox QPushButton[isCancelButton=\"true\"]:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #B91C1C, stop:1 #7F1D1D) !important;"
        "    border: 1px solid #991B1B !important;"
        "    color: #FFFFFF !important;"
        "}\n"
    );
}

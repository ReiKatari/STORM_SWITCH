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
                           const QString& ar, const QString& hi = QString()) {
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
    if (lang.rfind("hi", 0) == 0 && !hi.isEmpty()) return hi;
    if (lang.rfind("ar", 0) == 0 && !ar.isEmpty()) return ar;

    const QString ml = StormLookupMultiLang(en, lang);
    if (!ml.isEmpty()) {
        return ml;
    }
    return en;
}

// 11 Mandatory STORM Languages: ru, en, de, fr, zh, ko, ja, es, it, pt, hi
inline QString StormLangStorm11(const QString& ru, const QString& en,
                                const QString& de, const QString& fr,
                                const QString& zh, const QString& ko,
                                const QString& ja, const QString& es,
                                const QString& it, const QString& pt,
                                const QString& hi) {
    std::string lang = UISettings::values.language.GetValue();
    if (lang.empty()) {
        lang = QLocale::system().name().toStdString();
    }
    if (lang.rfind("ru", 0) == 0) return ru;
    if (lang.rfind("en", 0) == 0) return en;
    if (lang.rfind("de", 0) == 0 && !de.isEmpty()) return de;
    if (lang.rfind("fr", 0) == 0 && !fr.isEmpty()) return fr;
    if (lang.rfind("zh", 0) == 0 && !zh.isEmpty()) return zh;
    if (lang.rfind("ko", 0) == 0 && !ko.isEmpty()) return ko;
    if (lang.rfind("ja", 0) == 0 && !ja.isEmpty()) return ja;
    if (lang.rfind("es", 0) == 0 && !es.isEmpty()) return es;
    if (lang.rfind("it", 0) == 0 && !it.isEmpty()) return it;
    if (lang.rfind("pt", 0) == 0 && !pt.isEmpty()) return pt;
    if (lang.rfind("hi", 0) == 0 && !hi.isEmpty()) return hi;

    const QString ml = StormLookupMultiLang(en, lang);
    if (!ml.isEmpty()) {
        return ml;
    }
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
    if (lang.rfind("hi", 0) == 0) return QStringLiteral("रद्द करें");
    if (lang.rfind("ar", 0) == 0) return QStringLiteral("إلغاء");
    return QStringLiteral("Cancel");
}

inline QString StormCancelButtonStyle() {
    return QStringLiteral(
        "QPushButton, QPushButton[isCancelButton=\"true\"], QDialogButtonBox QPushButton[isCancelButton=\"true\"] {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f87171, stop:0.48 #ef4444, stop:0.52 #dc2626, stop:1 #b91c1c) !important;"
        "    color: #FFFFFF !important;"
        "    font-weight: 700 !important;"
        "    font-size: 12px !important;"
        "    padding: 6px 18px !important;"
        "    border-radius: 8px !important;"
        "    border-top: 1px solid rgba(255, 255, 255, 0.45) !important;"
        "    border-left: 1px solid rgba(255, 255, 255, 0.20) !important;"
        "    border-right: 1px solid rgba(255, 255, 255, 0.20) !important;"
        "    border-bottom: 3px solid #7f1d1d !important;"
        "    min-height: 24px !important;"
        "}"
        "QPushButton:hover, QPushButton[isCancelButton=\"true\"]:hover, QDialogButtonBox QPushButton[isCancelButton=\"true\"]:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #fca5a5, stop:0.48 #f87171, stop:0.52 #ef4444, stop:1 #dc2626) !important;"
        "    border-top: 1px solid rgba(255, 255, 255, 0.65) !important;"
        "    border-left: 1px solid rgba(255, 255, 255, 0.30) !important;"
        "    border-right: 1px solid rgba(255, 255, 255, 0.30) !important;"
        "    border-bottom: 3px solid #991b1b !important;"
        "    color: #FFFFFF !important;"
        "}"
        "QPushButton:pressed, QPushButton[isCancelButton=\"true\"]:pressed, QDialogButtonBox QPushButton[isCancelButton=\"true\"]:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #991b1b, stop:0.5 #b91c1c, stop:1 #ef4444) !important;"
        "    border-top: 2px solid rgba(0, 0, 0, 0.55) !important;"
        "    border-left: 1px solid rgba(0, 0, 0, 0.35) !important;"
        "    border-right: 1px solid rgba(0, 0, 0, 0.35) !important;"
        "    border-bottom: 1px solid rgba(255, 255, 255, 0.20) !important;"
        "    padding-top: 8px !important;"
        "    padding-bottom: 4px !important;"
        "    color: #FFFFFF !important;"
        "}"
    );
}
inline QString StormCancelButtonQssRule() {
    return QStringLiteral(
        "\nQPushButton[isCancelButton=\"true\"], QDialogButtonBox QPushButton[isCancelButton=\"true\"] {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f87171, stop:0.48 #ef4444, stop:0.52 #dc2626, stop:1 #b91c1c) !important;"
        "    color: #FFFFFF !important;"
        "    font-weight: 700 !important;"
        "    font-size: 12px !important;"
        "    padding: 6px 18px !important;"
        "    border-radius: 8px !important;"
        "    border-top: 1px solid rgba(255, 255, 255, 0.45) !important;"
        "    border-left: 1px solid rgba(255, 255, 255, 0.20) !important;"
        "    border-right: 1px solid rgba(255, 255, 255, 0.20) !important;"
        "    border-bottom: 3px solid #7f1d1d !important;"
        "    min-height: 24px !important;"
        "}"
        "\nQPushButton[isCancelButton=\"true\"]:hover, QDialogButtonBox QPushButton[isCancelButton=\"true\"]:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #fca5a5, stop:0.48 #f87171, stop:0.52 #ef4444, stop:1 #dc2626) !important;"
        "    border-top: 1px solid rgba(255, 255, 255, 0.65) !important;"
        "    border-left: 1px solid rgba(255, 255, 255, 0.30) !important;"
        "    border-right: 1px solid rgba(255, 255, 255, 0.30) !important;"
        "    border-bottom: 3px solid #991b1b !important;"
        "    color: #FFFFFF !important;"
        "}"
        "\nQPushButton[isCancelButton=\"true\"]:pressed, QDialogButtonBox QPushButton[isCancelButton=\"true\"]:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #991b1b, stop:0.5 #b91c1c, stop:1 #ef4444) !important;"
        "    border-top: 2px solid rgba(0, 0, 0, 0.55) !important;"
        "    border-left: 1px solid rgba(0, 0, 0, 0.35) !important;"
        "    border-right: 1px solid rgba(0, 0, 0, 0.35) !important;"
        "    border-bottom: 1px solid rgba(255, 255, 255, 0.20) !important;"
        "    padding-top: 8px !important;"
        "    padding-bottom: 4px !important;"
        "    color: #FFFFFF !important;"
        "}\n"
    );
}

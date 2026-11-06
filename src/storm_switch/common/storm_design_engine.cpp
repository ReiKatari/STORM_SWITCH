// SPDX-FileCopyrightText: 2026 STORM SOFT
// SPDX-License-Identifier: GPL-3.0-or-later

#include "storm_switch/common/storm_design_engine.h"

#include <algorithm>
#include <QAbstractButton>
#include <QAbstractItemView>
#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QEasingCurve>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QLineEdit>
#include <QPointer>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QTabWidget>
#include <QVariantAnimation>
#include <QWidget>

#include "qt_common/config/uisettings.h"
#include "storm_switch/common/storm_lang.h"
#include "storm_switch/common/storm_theme.h"

namespace StormDesign {

namespace {

QString RgbaCss(const QColor& c, int alpha_255) {
    const QColor valid = c.isValid() ? c : QColor(139, 92, 246);
    return QStringLiteral("rgba(%1, %2, %3, %4)")
        .arg(valid.red())
        .arg(valid.green())
        .arg(valid.blue())
        .arg(std::clamp(alpha_255, 0, 255));
}

QColor MixColors(const QColor& a, const QColor& b, double t) {
    const double k = std::clamp(t, 0.0, 1.0);
    return QColor(
        static_cast<int>(a.red() * (1.0 - k) + b.red() * k),
        static_cast<int>(a.green() * (1.0 - k) + b.green() * k),
        static_cast<int>(a.blue() * (1.0 - k) + b.blue() * k));
}

struct DerivedTokens {
    QColor bg;
    QColor surface;
    QColor card;
    QColor cardAlt;
    QColor border;
    QColor accent;
    QColor accentLight;
    QColor accentDark;
    QColor deepShadow;
    QColor deepWell;

    QString windowBackdrop;
    QString cardRadius;
    QString btnRadius;
    QString inputRadius;
    QString tabRadius;

    QString btnNormal;
    QString btnHover;
    QString btnPressed;
    QString btnPrimaryNormal;
    QString btnPrimaryHover;
    QString btnPrimaryPressed;

    QString inputNormal;
    QString inputHover;
    QString inputFocus;

    QString groupBoxRule;
    QString groupTitleRule;
    QString tabPaneRule;
    QString tabRule;
    QString selectorListRule;
};

DerivedTokens ComputeTokens(const StormTheme::Palette& pal, Preset preset) {
    DerivedTokens t;
    t.bg = QColor(pal.bg);
    t.surface = QColor(pal.surface);
    t.card = QColor(pal.card);
    t.cardAlt = QColor(pal.cardAlt);
    t.border = QColor(pal.border);
    t.accent = QColor(pal.accent);
    if (!t.accent.isValid()) {
        t.accent = QColor(QStringLiteral("#8B5CF6"));
    }
    t.accentLight = t.accent.lighter(pal.is_light ? 115 : 132);
    t.accentDark = t.accent.darker(pal.is_light ? 125 : 145);
    t.deepShadow = pal.is_light ? t.border.darker(145) : t.bg.darker(235);
    t.deepWell = pal.is_light ? t.cardAlt.darker(115) : t.bg.darker(185);

    const QColor btnTop = pal.is_light ? t.surface : MixColors(t.cardAlt, t.accent, 0.14).lighter(118);
    const QColor btnMid = pal.is_light ? t.card : MixColors(t.card, t.surface, 0.35);
    const QColor btnBot = pal.is_light ? t.cardAlt : t.surface.darker(122);
    const QColor hoverTop = MixColors(btnTop, t.accentLight, pal.is_light ? 0.22 : 0.34);
    const QColor hoverBot = MixColors(btnBot, t.accentDark, pal.is_light ? 0.16 : 0.28);

    const QString specBright = pal.is_light ? QStringLiteral("rgba(255, 255, 255, 0.92)")
                                            : QStringLiteral("rgba(255, 255, 255, 0.34)");
    const QString specSoft = pal.is_light ? QStringLiteral("rgba(255, 255, 255, 0.72)")
                                          : QStringLiteral("rgba(255, 255, 255, 0.16)");
    const QString accentTintSoft = RgbaCss(t.accent, pal.is_light ? 28 : 42);
    const QString accentTintMid = RgbaCss(t.accent, pal.is_light ? 48 : 78);
    const QString accentTintStrong = RgbaCss(t.accent, pal.is_light ? 80 : 125);
    const QString cardGlass = RgbaCss(t.card, pal.is_light ? 235 : 205);
    const QString surfGlass = RgbaCss(t.surface, pal.is_light ? 242 : 218);

    switch (preset) {
    case Preset::HyperionGlass:
    default: {
        t.cardRadius = QStringLiteral("13px");
        t.btnRadius = QStringLiteral("10px");
        t.inputRadius = QStringLiteral("9px");
        t.tabRadius = QStringLiteral("9px");
        t.windowBackdrop = QStringLiteral(
            "qradialgradient(cx:0.18, cy:0.10, radius:1.18, fx:0.18, fy:0.10, "
            "stop:0 %1, stop:0.46 %2, stop:1 %3)")
            .arg(MixColors(t.surface, t.accent, pal.is_light ? 0.10 : 0.18).name(),
                 pal.surface, pal.bg);

        t.btnNormal = QStringLiteral(
            "background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:0.48 %2, stop:1 %3);"
            "border-top: 1.5px solid %4;"
            "border-left: 1px solid %5;"
            "border-right: 1px solid %5;"
            "border-bottom: 3px solid %6;"
            "border-radius: 10px;"
            "padding: 6px 16px;")
            .arg(btnTop.name(), btnMid.name(), btnBot.name(), specBright, RgbaCss(t.border, 195), t.deepShadow.name());
        t.btnHover = QStringLiteral(
            "background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:0.50 %2, stop:1 %3);"
            "border-top: 1.5px solid %4;"
            "border-left: 1px solid %5;"
            "border-right: 1px solid %5;"
            "border-bottom: 3px solid %6;")
            .arg(hoverTop.name(), btnTop.name(), hoverBot.name(), t.accentLight.name(), pal.accent, t.accentDark.name());
        t.btnPressed = QStringLiteral(
            "background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);"
            "border: 1px solid %3;"
            "border-top: 2px solid %4;"
            "padding: 8px 16px 4px 16px;")
            .arg(btnBot.name(), btnMid.name(), pal.accent, t.deepShadow.name());

        t.inputNormal = QStringLiteral(
            "background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);"
            "border-top: 2px solid %3;"
            "border-left: 1px solid %4;"
            "border-right: 1px solid %4;"
            "border-bottom: 1px solid %5;"
            "border-radius: 9px;")
            .arg(t.deepWell.name(), pal.inputBg, t.deepShadow.name(), pal.border, specSoft);
        t.inputHover = QStringLiteral(
            "border-top: 1.5px solid %1; border-left: 1px solid %1; border-right: 1px solid %1; border-bottom: 1.5px solid %2; background-color: %3;")
            .arg(pal.accent, t.accentLight.name(), pal.surface);
        t.inputFocus = QStringLiteral(
            "border: 1.5px solid %1; background-color: %2;")
            .arg(t.accentLight.name(), pal.surface);

        t.groupBoxRule = QStringLiteral(
            "background-color: %1;"
            "border-top: 1.5px solid %2;"
            "border-left: 1px solid %3;"
            "border-right: 1px solid %3;"
            "border-bottom: 3px solid %4;"
            "border-radius: 13px;"
            "margin-top: 14px;"
            "padding: 18px 14px 13px 14px;")
            .arg(cardGlass, specSoft, RgbaCss(t.accent, 68), t.deepShadow.name());
        t.groupTitleRule = QStringLiteral(
            "subcontrol-origin: margin; subcontrol-position: top left; top: 1px; left: 14px;"
            "padding: 2px 10px; border-radius: 7px; font-weight: 800; font-size: 12px;"
            "color: %1; background-color: %2; border: 1px solid %3;")
            .arg(pal.is_light ? pal.accent : t.accentLight.name(), pal.surface, RgbaCss(t.accent, 95));
        break;
    }

    case Preset::CyberDeck: {
        t.cardRadius = QStringLiteral("4px");
        t.btnRadius = QStringLiteral("3px");
        t.inputRadius = QStringLiteral("3px");
        t.tabRadius = QStringLiteral("3px");
        t.windowBackdrop = QStringLiteral(
            "qlineargradient(x1:0, y1:0, x2:1, y2:1, "
            "stop:0 %1, stop:0.52 %2, stop:0.86 %3, stop:1 %1)")
            .arg(pal.bg, pal.surface, MixColors(t.bg, t.accent, pal.is_light ? 0.08 : 0.15).name());

        t.btnNormal = QStringLiteral(
            "background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);"
            "border-left: 3.5px solid %3;"
            "border-top: 1px solid %4;"
            "border-right: 1px solid %5;"
            "border-bottom: 3.5px solid %6;"
            "border-radius: 3px;"
            "padding: 6px 16px;")
            .arg(btnTop.name(), btnBot.name(), pal.accent, RgbaCss(t.accentLight, 120), pal.border, t.deepShadow.name());
        t.btnHover = QStringLiteral(
            "background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 %1, stop:0.35 %2, stop:1 %3);"
            "border-left: 4px solid %4;"
            "border-top: 1px solid %4;"
            "border-right: 1px solid %5;"
            "border-bottom: 3.5px solid %6;")
            .arg(accentTintMid, hoverTop.name(), btnMid.name(), t.accentLight.name(), pal.accent, t.deepShadow.name());
        t.btnPressed = QStringLiteral(
            "background-color: %1;"
            "border-left: 4px solid %2;"
            "border-top: 2px solid %3;"
            "border-right: 1px solid %2;"
            "border-bottom: 1px solid %2;"
            "padding: 8px 16px 4px 16px;")
            .arg(btnBot.name(), pal.accent, t.deepShadow.name());

        t.inputNormal = QStringLiteral(
            "background-color: %1;"
            "border-left: 3px solid %2;"
            "border-top: 2px solid %3;"
            "border-right: 1px solid %4;"
            "border-bottom: 1px solid %4;"
            "border-radius: 3px;")
            .arg(pal.inputBg, pal.accent, t.deepShadow.name(), pal.border);
        t.inputHover = QStringLiteral(
            "border-left: 3.5px solid %1; border-top: 1px solid %1; border-right: 1px solid %1; border-bottom: 1.5px solid %1; background-color: %2;")
            .arg(t.accentLight.name(), pal.surface);
        t.inputFocus = QStringLiteral(
            "border-left: 4px solid %1; border-top: 1.5px solid %1; border-right: 1.5px solid %1; border-bottom: 1.5px solid %1; background-color: %2;")
            .arg(t.accentLight.name(), pal.surface);

        t.groupBoxRule = QStringLiteral(
            "background-color: %1;"
            "border-left: 3.5px solid %2;"
            "border-top: 1px solid %3;"
            "border-right: 1px solid %3;"
            "border-bottom: 3px solid %4;"
            "border-radius: 4px;"
            "margin-top: 14px;"
            "padding: 18px 14px 13px 14px;")
            .arg(surfGlass, pal.accent, pal.border, t.deepShadow.name());
        t.groupTitleRule = QStringLiteral(
            "subcontrol-origin: margin; subcontrol-position: top left; top: 1px; left: 10px;"
            "padding: 2px 10px; border-radius: 2px; font-weight: 800; font-size: 12px;"
            "color: %1; background-color: %2; border-left: 3px solid %3; border-top: 1px solid %3; border-right: 1px solid %4; border-bottom: 1px solid %4;")
            .arg(pal.textPrimary, pal.card, pal.accent, pal.border);
        break;
    }

    case Preset::AuroraQuantum: {
        t.cardRadius = QStringLiteral("18px");
        t.btnRadius = QStringLiteral("15px");
        t.inputRadius = QStringLiteral("14px");
        t.tabRadius = QStringLiteral("13px");
        t.windowBackdrop = QStringLiteral(
            "qlineargradient(x1:0, y1:0, x2:1, y2:0.88, "
            "stop:0 %1, stop:0.36 %2, stop:0.70 %3, stop:1 %4)")
            .arg(MixColors(t.bg, t.accent, pal.is_light ? 0.12 : 0.22).name(),
                 pal.bg, pal.surface,
                 MixColors(t.surface, t.accentLight, pal.is_light ? 0.10 : 0.16).name());

        const QColor metalUpper = MixColors(btnTop, t.accentLight, 0.18);
        const QColor metalHorizon = MixColors(btnMid, t.accent, 0.10);
        t.btnNormal = QStringLiteral(
            "background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:0.49 %2, stop:0.52 %3, stop:1 %4);"
            "border-top: 1.5px solid %5;"
            "border-left: 1px solid %6;"
            "border-right: 1px solid %6;"
            "border-bottom: 3px solid %7;"
            "border-radius: 15px;"
            "padding: 6px 18px;")
            .arg(metalUpper.name(), metalHorizon.name(), btnMid.darker(106).name(), btnBot.name(),
                 RgbaCss(t.accentLight, 175), RgbaCss(t.accent, 90), t.deepShadow.name());
        t.btnHover = QStringLiteral(
            "background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:0.50 %2, stop:1 %3);"
            "border-top: 1.5px solid %4;"
            "border-left: 1.5px solid %5;"
            "border-right: 1.5px solid %5;"
            "border-bottom: 3px solid %6;")
            .arg(hoverTop.name(), MixColors(btnTop, t.accent, 0.30).name(), hoverBot.name(),
                 specBright, t.accentLight.name(), t.accentDark.name());
        t.btnPressed = QStringLiteral(
            "background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);"
            "border: 1.5px solid %3;"
            "padding: 8px 18px 4px 18px;")
            .arg(btnBot.name(), metalHorizon.name(), pal.accent);

        t.inputNormal = QStringLiteral(
            "background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);"
            "border-top: 2px solid %3;"
            "border-left: 1px solid %4;"
            "border-right: 1px solid %4;"
            "border-bottom: 1.5px solid %5;"
            "border-radius: 14px;")
            .arg(t.deepWell.name(), pal.inputBg, t.deepShadow.name(), RgbaCss(t.accent, 72), RgbaCss(t.accentLight, 95));
        t.inputHover = QStringLiteral(
            "border: 1.5px solid %1; background-color: %2;")
            .arg(pal.accent, pal.surface);
        t.inputFocus = QStringLiteral(
            "border: 2px solid %1; background-color: %2;")
            .arg(t.accentLight.name(), pal.surface);

        t.groupBoxRule = QStringLiteral(
            "background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 %2);"
            "border-top: 1.5px solid %3;"
            "border-left: 1px solid %4;"
            "border-right: 1px solid %4;"
            "border-bottom: 3px solid %5;"
            "border-radius: 18px;"
            "margin-top: 15px;"
            "padding: 19px 15px 14px 15px;")
            .arg(cardGlass, surfGlass, RgbaCss(t.accentLight, 135), RgbaCss(t.accent, 75), t.deepShadow.name());
        t.groupTitleRule = QStringLiteral(
            "subcontrol-origin: margin; subcontrol-position: top left; top: 1px; left: 16px;"
            "padding: 2px 12px; border-radius: 10px; font-weight: 800; font-size: 12px;"
            "color: #FFFFFF; background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 %1, stop:1 %2);"
            "border: 1px solid %3;")
            .arg(t.accentDark.name(), pal.accent, t.accentLight.name());
        break;
    }

    case Preset::OrbitalSpatial: {
        t.cardRadius = QStringLiteral("16px");
        t.btnRadius = QStringLiteral("12px");
        t.inputRadius = QStringLiteral("11px");
        t.tabRadius = QStringLiteral("10px");
        t.windowBackdrop = QStringLiteral(
            "qradialgradient(cx:0.50, cy:0.02, radius:1.15, fx:0.50, fy:0.02, "
            "stop:0 %1, stop:0.42 %2, stop:1 %3)")
            .arg(MixColors(t.card, t.accent, pal.is_light ? 0.12 : 0.22).name(),
                 pal.surface, pal.bg);

        t.btnNormal = QStringLiteral(
            "background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);"
            "border-top: 1.5px solid %3;"
            "border-left: 1px solid %4;"
            "border-right: 1px solid %4;"
            "border-bottom: 3.5px solid %5;"
            "border-radius: 12px;"
            "padding: 6px 16px;")
            .arg(RgbaCss(btnTop, 235), RgbaCss(btnBot, 235), specBright, specSoft, t.deepShadow.name());
        t.btnHover = QStringLiteral(
            "background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);"
            "border-top: 1.5px solid #FFFFFF;"
            "border-left: 1px solid %3;"
            "border-right: 1px solid %3;"
            "border-bottom: 3.5px solid %4;")
            .arg(hoverTop.name(), hoverBot.name(), t.accentLight.name(), t.accentDark.name());
        t.btnPressed = QStringLiteral(
            "background-color: %1;"
            "border-top: 2px solid %2;"
            "border-left: 1px solid %3;"
            "border-right: 1px solid %3;"
            "border-bottom: 1px solid %3;"
            "padding: 8px 16px 4px 16px;")
            .arg(btnBot.name(), t.deepShadow.name(), pal.accent);

        t.inputNormal = QStringLiteral(
            "background-color: %1;"
            "border-top: 2px solid %2;"
            "border-left: 1px solid %3;"
            "border-right: 1px solid %3;"
            "border-bottom: 1.5px solid %4;"
            "border-radius: 11px;")
            .arg(RgbaCss(QColor(pal.inputBg), 230), t.deepShadow.name(), pal.border, specSoft);
        t.inputHover = QStringLiteral(
            "border: 1.5px solid %1; background-color: %2;")
            .arg(pal.accent, pal.surface);
        t.inputFocus = QStringLiteral(
            "border: 1.5px solid %1; border-top: 1.5px solid #FFFFFF; background-color: %2;")
            .arg(t.accentLight.name(), pal.surface);

        t.groupBoxRule = QStringLiteral(
            "background-color: %1;"
            "border-top: 1.5px solid %2;"
            "border-left: 1px solid %3;"
            "border-right: 1px solid %3;"
            "border-bottom: 3.5px solid %4;"
            "border-radius: 16px;"
            "margin-top: 14px;"
            "padding: 18px 14px 14px 14px;")
            .arg(cardGlass, specBright, specSoft, t.deepShadow.name());
        t.groupTitleRule = QStringLiteral(
            "subcontrol-origin: margin; subcontrol-position: top left; top: 1px; left: 16px;"
            "padding: 2px 11px; border-radius: 9px; font-weight: 800; font-size: 12px;"
            "color: %1; background-color: %2; border-top: 1px solid %3; border-bottom: 2px solid %4; border-left: 1px solid %5; border-right: 1px solid %5;")
            .arg(pal.is_light ? pal.accent : t.accentLight.name(), pal.surface, specBright, t.deepShadow.name(), RgbaCss(t.accent, 85));
        break;
    }

    case Preset::ChronoNexus: {
        t.cardRadius = QStringLiteral("8px");
        t.btnRadius = QStringLiteral("6px");
        t.inputRadius = QStringLiteral("6px");
        t.tabRadius = QStringLiteral("6px");
        t.windowBackdrop = QStringLiteral(
            "qlineargradient(x1:0, y1:0, x2:0, y2:1, "
            "stop:0 %1, stop:0.20 %2, stop:0.84 %2, stop:1 %3)")
            .arg(pal.surface, pal.bg, MixColors(t.bg, t.accent, pal.is_light ? 0.08 : 0.16).name());

        t.btnNormal = QStringLiteral(
            "background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);"
            "border-top: 2px solid %3;"
            "border-left: 1.5px solid %4;"
            "border-right: 1.5px solid %5;"
            "border-bottom: 4px solid %5;"
            "border-radius: 6px;"
            "padding: 6px 16px;"
            "font-weight: 700;")
            .arg(btnTop.name(), btnBot.name(), RgbaCss(t.accentLight, 150), pal.border, t.deepShadow.name());
        t.btnHover = QStringLiteral(
            "background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);"
            "border-top: 2px solid %3;"
            "border-left: 1.5px solid %4;"
            "border-right: 1.5px solid %5;"
            "border-bottom: 4px solid %5;")
            .arg(hoverTop.name(), hoverBot.name(), t.accentLight.name(), pal.accent, t.deepShadow.name());
        t.btnPressed = QStringLiteral(
            "background-color: %1;"
            "border-top: 3px solid %2;"
            "border-left: 1.5px solid %2;"
            "border-right: 1px solid %3;"
            "border-bottom: 1px solid %3;"
            "padding: 8px 16px 4px 16px;")
            .arg(btnBot.name(), t.deepShadow.name(), pal.accent);

        t.inputNormal = QStringLiteral(
            "background-color: %1;"
            "border-top: 2.5px solid %2;"
            "border-left: 2px solid %2;"
            "border-right: 1px solid %3;"
            "border-bottom: 1.5px solid %4;"
            "border-radius: 6px;")
            .arg(pal.inputBg, t.deepShadow.name(), pal.border, RgbaCss(t.accentLight, 95));
        t.inputHover = QStringLiteral(
            "border-top: 2px solid %1; border-left: 1.5px solid %2; border-right: 1.5px solid %2; border-bottom: 2px solid %2; background-color: %3;")
            .arg(t.deepShadow.name(), pal.accent, pal.surface);
        t.inputFocus = QStringLiteral(
            "border: 2px solid %1; background-color: %2;")
            .arg(t.accentLight.name(), pal.surface);

        t.groupBoxRule = QStringLiteral(
            "background-color: %1;"
            "border-top: 2px solid %2;"
            "border-left: 1px solid %3;"
            "border-right: 1.5px solid %4;"
            "border-bottom: 3.5px solid %4;"
            "border-radius: 8px;"
            "margin-top: 14px;"
            "padding: 18px 14px 13px 14px;")
            .arg(pal.card, pal.accent, pal.border, t.deepShadow.name());
        t.groupTitleRule = QStringLiteral(
            "subcontrol-origin: margin; subcontrol-position: top left; top: 0px; left: 12px;"
            "padding: 2px 10px; border-radius: 4px; font-weight: 800; font-size: 12px;"
            "color: %1; background-color: %2; border-top: 2px solid %3; border-bottom: 2px solid %4; border-left: 1px solid %3; border-right: 1px solid %3;")
            .arg(pal.is_light ? pal.accent : t.accentLight.name(), pal.surface, pal.accent, t.deepShadow.name());
        break;
    }
    }

    t.btnPrimaryNormal = QStringLiteral(
        "background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);"
        "color: #FFFFFF;"
        "border-top: 1.5px solid %3;"
        "border-left: 1px solid %4;"
        "border-right: 1px solid %4;"
        "border-bottom: 3.5px solid %5;"
        "border-radius: %6;"
        "padding: 6px 18px;"
        "font-weight: 800;")
        .arg(t.accentLight.name(), pal.accent, specBright, t.accentLight.name(), t.accentDark.darker(135).name(), t.btnRadius);

    t.btnPrimaryHover = QStringLiteral(
        "background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);"
        "color: #FFFFFF;"
        "border-top: 1.5px solid #FFFFFF;"
        "border-left: 1px solid %1;"
        "border-right: 1px solid %1;"
        "border-bottom: 3.5px solid %3;")
        .arg(t.accentLight.lighter(115).name(), t.accentLight.name(), t.accentDark.name());

    t.btnPrimaryPressed = QStringLiteral(
        "background-color: %1;"
        "color: #FFFFFF;"
        "border: 1.5px solid %2;"
        "padding: 8px 18px 4px 18px;")
        .arg(t.accentDark.name(), t.accentLight.name());

    t.tabPaneRule = QStringLiteral(
        "border: 1px solid %1; border-bottom: 3px solid %2; border-radius: %3; background-color: %4; top: -1px;")
        .arg(pal.border, t.deepShadow.name(), t.cardRadius, pal.bg);

    if (preset == Preset::AuroraQuantum || preset == Preset::OrbitalSpatial) {
        t.tabRule = QStringLiteral(
            "QTabBar::tab {"
            "  background-color: %1; color: %2; border: 1px solid %3; border-bottom: 2px solid %4;"
            "  border-radius: %5; padding: 7px 16px; margin: 2px 4px 4px 0px; font-weight: 600;"
            "}"
            "QTabBar::tab:hover {"
            "  background-color: %6; color: %7; border: 1px solid %8;"
            "}"
            "QTabBar::tab:selected {"
            "  background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 %8, stop:1 %9);"
            "  color: #FFFFFF; border-top: 1.5px solid %10; border-bottom: 2.5px solid %4; font-weight: 800;"
            "}")
            .arg(pal.card, pal.textSecondary, pal.border, t.deepShadow.name(), t.tabRadius,
                 accentTintSoft, pal.textPrimary, pal.accent, t.accentDark.name(), specBright);
    } else if (preset == Preset::CyberDeck) {
        t.tabRule = QStringLiteral(
            "QTabBar::tab {"
            "  background-color: %1; color: %2; border: 1px solid %3; border-left: 2px solid %3;"
            "  border-top-left-radius: 3px; border-top-right-radius: 3px; padding: 8px 16px; margin-right: 3px; font-weight: 700;"
            "}"
            "QTabBar::tab:hover {"
            "  background-color: %4; color: %5; border-left: 3px solid %6;"
            "}"
            "QTabBar::tab:selected {"
            "  background-color: %7; color: %6; border-left: 3.5px solid %6; border-top: 1.5px solid %6; border-right: 1px solid %3; border-bottom: 1px solid %7; font-weight: 800;"
            "}")
            .arg(pal.bg, pal.textSecondary, pal.border, accentTintSoft, pal.textPrimary, pal.accent, pal.surface);
    } else {
        t.tabRule = QStringLiteral(
            "QTabBar::tab {"
            "  background-color: %1; color: %2; border: 1px solid %3; border-bottom: none;"
            "  border-top-left-radius: %4; border-top-right-radius: %4; padding: 8px 16px; margin-right: 3px; font-weight: 600;"
            "}"
            "QTabBar::tab:hover {"
            "  background-color: %5; color: %6; border-top: 2px solid %7;"
            "}"
            "QTabBar::tab:selected {"
            "  background-color: %8; color: %9; border-top: 2.5px solid %7; border-left: 1px solid %3; border-right: 1px solid %3; border-bottom: 1px solid %8; font-weight: 800;"
            "}")
            .arg(pal.bg, pal.textSecondary, pal.border, t.tabRadius,
                 pal.card, pal.textPrimary, pal.accent, pal.surface,
                 pal.is_light ? pal.accent : t.accentLight.name());
    }

    t.selectorListRule = QStringLiteral(
        "QListWidget#selectorList, QListWidget#perGameSelectorList {"
        "  background-color: %1; border: 1px solid %2; border-bottom: 3px solid %3; border-radius: %4; padding: 6px; outline: none;"
        "}"
        "QListWidget#selectorList::item, QListWidget#perGameSelectorList::item {"
        "  color: %5; background: transparent; border: 1px solid transparent; border-radius: %6;"
        "  padding: 10px 14px; font-size: 13px; font-weight: 600; margin-bottom: 4px;"
        "}"
        "QListWidget#selectorList::item:hover, QListWidget#perGameSelectorList::item:hover {"
        "  color: %5; background-color: %7; border: 1px solid %8;"
        "}"
        "QListWidget#selectorList::item:selected, QListWidget#perGameSelectorList::item:selected {"
        "  color: #FFFFFF;"
        "  background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 %9, stop:1 %10);"
        "  border-top: 1px solid %11; border-left: 3px solid %12; border-right: 1px solid %9; border-bottom: 2.5px solid %3;"
        "  font-weight: 800;"
        "}")
        .arg(surfGlass, pal.border, t.deepShadow.name(), t.cardRadius,
             pal.textPrimary, t.btnRadius,
             accentTintSoft, RgbaCss(t.accent, 110),
             pal.accent, t.accentDark.name(), specBright, t.accentLight.name());

    return t;
}

QString BuildCoreControlsQss(const StormTheme::Palette& pal, const DerivedTokens& t) {
    QString qss;
    qss.reserve(8192);

    qss += QStringLiteral(
        "QLabel, QCheckBox, QRadioButton { color: %1; background: transparent; background-color: transparent; }"
        "QWidget#StormSettingRow, QGroupBox QLabel, QGroupBox QCheckBox, QGroupBox QRadioButton { background: transparent; background-color: transparent; }"
        "QTabWidget, QTabWidget > QStackedWidget, QTabWidget QStackedWidget, QStackedWidget {"
        "  background: %2; background-color: %2; color: %1;"
        "}"
        "QScrollArea, QScrollArea > QWidget, QScrollArea > QWidget > QWidget, "
        "QWidget#qt_scrollarea_viewport, QWidget#scrollAreaWidgetContents {"
        "  background: %2; background-color: %2; color: %1;"
        "}"
    ).arg(pal.textPrimary, pal.bg);

    qss += QStringLiteral("QGroupBox { %1 color: %2; font-weight: bold; }")
               .arg(t.groupBoxRule, pal.textPrimary);
    qss += QStringLiteral("QGroupBox::title { %1 }").arg(t.groupTitleRule);

    qss += QStringLiteral("QTabWidget::pane { %1 }").arg(t.tabPaneRule);
    qss += t.tabRule;
    qss += t.selectorListRule;

    // 3D Volumetric Inputs, SpinBoxes & Dropdowns
    qss += QStringLiteral(
        "QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox, QDateTimeEdit {"
        "  %1"
        "  color: %2;"
        "  padding-top: 0px; padding-bottom: 0px; padding-left: 11px; padding-right: 28px;"
        "  min-height: 32px; font-size: 12px;"
        "  selection-background-color: %3; selection-color: #FFFFFF;"
        "}"
        "QSpinBox QLineEdit, QDoubleSpinBox QLineEdit {"
        "  padding: 0px; margin: 0px; border: none; background: transparent;"
        "}"
        "QLineEdit:hover, QSpinBox:hover, QDoubleSpinBox:hover, QComboBox:hover, QDateTimeEdit:hover {"
        "  %4"
        "}"
        "QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus, QComboBox:on, QDateTimeEdit:focus {"
        "  %5"
        "}"
        "QComboBox::drop-down {"
        "  subcontrol-origin: border; subcontrol-position: top right; width: 24px;"
        "  margin: 1px 1px 1px 0px; border-left: 1px solid %6;"
        "  border-top-right-radius: %7; border-bottom-right-radius: %7;"
        "  background-color: %8;"
        "}"
        "QComboBox::down-arrow { image: url(:/icons/down_arrow.svg); width: 12px; height: 12px; }"
        "QComboBoxPrivateContainer, QComboBox QListView, QComboBox QAbstractItemView {"
        "  background-color: %9; color: %2; border: 1.5px solid %3; border-radius: %7;"
        "  selection-background-color: %3; selection-color: #FFFFFF; outline: none; padding: 4px;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "  min-height: 26px; padding: 4px 10px; border-radius: 5px; color: %2;"
        "}"
        "QComboBox QAbstractItemView::item:selected, QComboBox QAbstractItemView::item:hover {"
        "  background-color: %3; color: #FFFFFF;"
        "}")
        .arg(t.inputNormal, pal.textPrimary, pal.accent, t.inputHover, t.inputFocus,
             pal.border, t.inputRadius, RgbaCss(t.accent, 28), pal.surface);

    // 3D Volumetric Sliders & Indicators
    qss += QStringLiteral(
        "QSlider::groove:horizontal {"
        "  border: 1px solid %1; border-top: 2px solid %2; height: 7px; background: %3; border-radius: 4px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 %4, stop:1 %5); border-radius: 4px;"
        "}"
        "QSlider::thumb:horizontal {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #FFFFFF, stop:1 %5);"
        "  border: 2px solid %4; width: 16px; margin-top: -6px; margin-bottom: -6px; border-radius: 8px;"
        "}"
        "QSlider::thumb:horizontal:hover { background: #FFFFFF; border: 2px solid %5; }"
        "QCheckBox::indicator:unchecked, QGroupBox::indicator:unchecked, QTreeView::indicator:unchecked, QListView::indicator:unchecked, QTableView::indicator:unchecked {"
        "  width: 16px; height: 16px; border: 1.5px solid #EF4444; border-radius: 4px; background: rgba(239, 68, 68, 0.18); image: none;"
        "}"
        "QCheckBox::indicator:checked, QGroupBox::indicator:checked, QTreeView::indicator:checked, QListView::indicator:checked, QTableView::indicator:checked {"
        "  width: 16px; height: 16px; background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #34D399, stop:1 #10B981); border: 1.5px solid #10B981; border-radius: 4px; image: none;"
        "}")
        .arg(pal.border, t.deepShadow.name(), pal.inputBg, pal.accent, t.accentLight.name());

    // Tables, Headers (Centered Bold) & Corner Button (Zero White Square!)
    qss += QStringLiteral(
        "QTableWidget, QTableView, QTreeWidget, QTreeView, QListWidget {"
        "  background-color: %1; alternate-background-color: %2; color: %3;"
        "  border: 1px solid %4; border-radius: %5; gridline-color: %4;"
        "  selection-background-color: %6; selection-color: #FFFFFF;"
        "}"
        "QHeaderView::section {"
        "  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %2, stop:1 %7);"
        "  color: %3; border: none; border-right: 1px solid %4; border-bottom: 2px solid %6;"
        "  padding: 6px 10px; font-weight: 800; text-align: center; qproperty-alignment: AlignCenter;"
        "}"
        "QTableCornerButton::section, QTableView QTableCornerButton::section, QTableWidget QTableCornerButton::section {"
        "  background-color: %7; border: none; border-right: 1px solid %4; border-bottom: 2px solid %6;"
        "}")
        .arg(pal.inputBg, pal.card, pal.textPrimary, pal.border, t.inputRadius, pal.accent, pal.surface);

    // Themed ScrollBars (Law 12)
    qss += QStringLiteral(
        "QScrollBar:vertical {"
        "  background: transparent; width: 9px; margin: 2px 2px 2px 0px; border-radius: 4px; border: none;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: %1; min-height: 28px; border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical:hover { background: %2; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; background: none; border: none; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
        "QScrollBar:horizontal {"
        "  background: transparent; height: 9px; margin: 0px 2px 2px 2px; border-radius: 4px; border: none;"
        "}"
        "QScrollBar::handle:horizontal {"
        "  background: %1; min-width: 28px; border-radius: 4px;"
        "}"
        "QScrollBar::handle:horizontal:hover { background: %2; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; background: none; border: none; }"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }")
        .arg(RgbaCss(t.accent, 115), pal.accent);

    // 3D Volumetric Tactile Buttons
    qss += QStringLiteral(
        "QPushButton, QDialogButtonBox QPushButton, QToolButton, QCommandLinkButton, QWizard QPushButton, QMessageBox QPushButton, QPushButton[flat=\"true\"], QToolButton[flat=\"true\"] {"
        "  %1 color: %2; font-weight: 600; min-height: 24px;"
        "}"
        "QPushButton:hover, QDialogButtonBox QPushButton:hover, QToolButton:hover, QCommandLinkButton:hover, QWizard QPushButton:hover, QMessageBox QPushButton:hover, QPushButton[flat=\"true\"]:hover, QToolButton[flat=\"true\"]:hover {"
        "  %3 color: %2;"
        "}"
        "QPushButton:pressed, QDialogButtonBox QPushButton:pressed, QToolButton:pressed, QCommandLinkButton:pressed, QWizard QPushButton:pressed, QMessageBox QPushButton:pressed, QPushButton[flat=\"true\"]:pressed, QToolButton[flat=\"true\"]:pressed {"
        "  %4 color: %2;"
        "}"
        "QPushButton:checked, QDialogButtonBox QPushButton:checked, QToolButton:checked, QCommandLinkButton:checked {"
        "  %5"
        "}"
        "QPushButton:disabled, QDialogButtonBox QPushButton:disabled, QToolButton:disabled {"
        "  background-color: %6; color: %7; border: 1px solid %8; border-radius: %9; padding: 6px 16px;"
        "}"
        "QPushButton:default, QDialogButtonBox QPushButton:default, QWizard QPushButton:default, QMessageBox QPushButton:default, QPushButton[primary=\"true\"], QToolButton[primary=\"true\"] {"
        "  %5"
        "}"
        "QPushButton:default:hover, QDialogButtonBox QPushButton:default:hover, QWizard QPushButton:default:hover, QMessageBox QPushButton:default:hover, QPushButton[primary=\"true\"]:hover, QToolButton[primary=\"true\"]:hover {"
        "  %10"
        "}"
        "QPushButton:default:pressed, QDialogButtonBox QPushButton:default:pressed, QWizard QPushButton:default:pressed, QMessageBox QPushButton:default:pressed, QPushButton[primary=\"true\"]:pressed, QToolButton[primary=\"true\"]:pressed {"
        "  %11"
        "}")
        .arg(t.btnNormal, pal.textPrimary, t.btnHover, t.btnPressed, t.btnPrimaryNormal,
             pal.inputBg, pal.textSecondary, pal.border, t.btnRadius,
             t.btnPrimaryHover, t.btnPrimaryPressed);

    return qss;
}

// -----------------------------------------------------------------------------
// 60/120 FPS Interactive 3D Hover, Press & Spatial Transition Animator
// -----------------------------------------------------------------------------
class GlobalDesignAnimator final : public QObject {
public:
    explicit GlobalDesignAnimator(QObject* parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (!watched || !event) {
            return QObject::eventFilter(watched, event);
        }

        const auto type = event->type();

        // Smooth spatial cross-fade when switching tabs inside dialogs
        if (type == QEvent::Show) {
            if (auto* tab_widget = qobject_cast<QTabWidget*>(watched)) {
                if (!tab_widget->property("_stormTabAnimHooked").toBool()) {
                    tab_widget->setProperty("_stormTabAnimHooked", true);
                    connect(tab_widget, &QTabWidget::currentChanged, this, [tab_widget](int idx) {
                        QWidget* page = tab_widget->widget(idx);
                        if (!page || !page->isVisible()) {
                            return;
                        }
                        if (page->graphicsEffect() != nullptr) {
                            return;
                        }
                        auto* fx = new QGraphicsOpacityEffect(page);
                        fx->setOpacity(0.58);
                        page->setGraphicsEffect(fx);
                        auto* anim = new QVariantAnimation(page);
                        anim->setDuration(140);
                        anim->setStartValue(0.58);
                        anim->setEndValue(1.0);
                        anim->setEasingCurve(QEasingCurve::OutCubic);
                        QPointer<QWidget> safe_page(page);
                        QPointer<QGraphicsOpacityEffect> safe_fx(fx);
                        connect(anim, &QVariantAnimation::valueChanged, page, [safe_fx](const QVariant& v) {
                            if (safe_fx) {
                                safe_fx->setOpacity(v.toReal());
                            }
                        });
                        connect(anim, &QVariantAnimation::finished, page, [safe_page, safe_fx]() {
                            if (safe_page && safe_page->graphicsEffect() == safe_fx) {
                                safe_page->setGraphicsEffect(nullptr);
                            }
                        });
                        anim->start(QAbstractAnimation::DeleteWhenStopped);
                    });
                }
            }
            return QObject::eventFilter(watched, event);
        }

        // Clean up any stale QGraphicsDropShadowEffect if attached to controls (Law 17 & 23)
        if (auto* widget = qobject_cast<QWidget*>(watched)) {
            if (widget->graphicsEffect() != nullptr &&
                widget->graphicsEffect()->property("_stormAnimShadow").toBool()) {
                widget->setGraphicsEffect(nullptr);
            }
        }

        return QObject::eventFilter(watched, event);
    }
};

} // namespace

Preset GetActivePreset(const std::string& explicit_id) {
    const std::string id = explicit_id.empty() ? GetActiveDesignId() : explicit_id;
    if (id == "cyber_deck") {
        return Preset::CyberDeck;
    }
    if (id == "aurora_quantum") {
        return Preset::AuroraQuantum;
    }
    if (id == "orbital_spatial") {
        return Preset::OrbitalSpatial;
    }
    if (id == "chrono_nexus") {
        return Preset::ChronoNexus;
    }
    return Preset::HyperionGlass;
}

std::string GetActiveDesignId() {
    const std::string val = UISettings::values.ui_design.GetValue();
    for (const auto& [id, _] : kDesigns) {
        if (val == id) {
            return val;
        }
    }
    return "hyperion_glass";
}

QString GetDesignSelectorLabel() {
    return StormLangStorm11(
        QStringLiteral("Дизайн интерфейса:"),
        QStringLiteral("UI Design:"),
        QStringLiteral("UI-Design:"),
        QStringLiteral("Design de l'interface :"),
        QStringLiteral("界面设计架构："),
        QStringLiteral("인터페이스 디자인:"),
        QStringLiteral("UIデザイン："),
        QStringLiteral("Diseño de interfaz:"),
        QStringLiteral("Design dell'interfaccia:"),
        QStringLiteral("Design da interface:"),
        QStringLiteral("इंटरफ़ेस डिज़ाइन:"));
}

QString GetLocalizedDesignName(const std::string& design_id) {
    if (design_id == "cyber_deck") {
        return StormLangStorm11(
            QStringLiteral("STORM CYBER-DECK TACTILE — Тактильная кибер-дека"),
            QStringLiteral("STORM CYBER-DECK TACTILE — Tactile Cyber-Deck"),
            QStringLiteral("STORM CYBER-DECK TACTILE — Taktiles Cyber-Deck"),
            QStringLiteral("STORM CYBER-DECK TACTILE — Cyber-Deck tactile"),
            QStringLiteral("STORM CYBER-DECK TACTILE — 战术赛博甲板"),
            QStringLiteral("STORM CYBER-DECK TACTILE — 택틸 사이버 덱"),
            QStringLiteral("STORM CYBER-DECK TACTILE — タクティカル・サイバーデッキ"),
            QStringLiteral("STORM CYBER-DECK TACTILE — Cyber-Deck táctil"),
            QStringLiteral("STORM CYBER-DECK TACTILE — Cyber-Deck tattile"),
            QStringLiteral("STORM CYBER-DECK TACTILE — Cyber-Deck tátil"),
            QStringLiteral("STORM CYBER-DECK TACTILE — टैक्टाइल साइबर-डेक"));
    }
    if (design_id == "aurora_quantum") {
        return StormLangStorm11(
            QStringLiteral("STORM AURORA QUANTUM — Жидкий металл и Аврора"),
            QStringLiteral("STORM AURORA QUANTUM — Liquid Metal & Aurora"),
            QStringLiteral("STORM AURORA QUANTUM — Flüssigmetall & Aurora"),
            QStringLiteral("STORM AURORA QUANTUM — Métal liquide & Aurore"),
            QStringLiteral("STORM AURORA QUANTUM — 液态金属与极光"),
            QStringLiteral("STORM AURORA QUANTUM — 리퀴드 메탈 & 오로라"),
            QStringLiteral("STORM AURORA QUANTUM — リキッドメタル＆オーロラ"),
            QStringLiteral("STORM AURORA QUANTUM — Metal líquido y Aurora"),
            QStringLiteral("STORM AURORA QUANTUM — Metallo liquido e Aurora"),
            QStringLiteral("STORM AURORA QUANTUM — Metal líquido e Aurora"),
            QStringLiteral("STORM AURORA QUANTUM — लिक्विड मेटल और ऑरोरा"));
    }
    if (design_id == "orbital_spatial") {
        return StormLangStorm11(
            QStringLiteral("STORM ORBITAL SPATIAL UI — Пространственный 3D-интерфейс"),
            QStringLiteral("STORM ORBITAL SPATIAL UI — Spatial 3D Vision UI"),
            QStringLiteral("STORM ORBITAL SPATIAL UI — Räumliches 3D-Interface"),
            QStringLiteral("STORM ORBITAL SPATIAL UI — Interface spatiale 3D"),
            QStringLiteral("STORM ORBITAL SPATIAL UI — 空间立体 3D 界面"),
            QStringLiteral("STORM ORBITAL SPATIAL UI — 공간형 3D 인터페이스"),
            QStringLiteral("STORM ORBITAL SPATIAL UI — 空間 3D インターフェース"),
            QStringLiteral("STORM ORBITAL SPATIAL UI — Interfaz espacial 3D"),
            QStringLiteral("STORM ORBITAL SPATIAL UI — Interfaccia spaziale 3D"),
            QStringLiteral("STORM ORBITAL SPATIAL UI — Interface espacial 3D"),
            QStringLiteral("STORM ORBITAL SPATIAL UI — स्पेशियल 3D इंटरफ़ेस"));
    }
    if (design_id == "chrono_nexus") {
        return StormLangStorm11(
            QStringLiteral("STORM CHRONO-NEXUS COMMANDER — Командный центр киберспорта"),
            QStringLiteral("STORM CHRONO-NEXUS COMMANDER — Esports Command Center"),
            QStringLiteral("STORM CHRONO-NEXUS COMMANDER — E-Sport-Kommandozentrale"),
            QStringLiteral("STORM CHRONO-NEXUS COMMANDER — Centre de commandement Esports"),
            QStringLiteral("STORM CHRONO-NEXUS COMMANDER — 电竞指挥中心"),
            QStringLiteral("STORM CHRONO-NEXUS COMMANDER — 이스포츠 커맨드 센터"),
            QStringLiteral("STORM CHRONO-NEXUS COMMANDER — eスポーツ・コマンドセンター"),
            QStringLiteral("STORM CHRONO-NEXUS COMMANDER — Centro de mando Esports"),
            QStringLiteral("STORM CHRONO-NEXUS COMMANDER — Centro di comando Esports"),
            QStringLiteral("STORM CHRONO-NEXUS COMMANDER — Centro de comando Esports"),
            QStringLiteral("STORM CHRONO-NEXUS COMMANDER — ई-स्पोर्ट्स कमांड सेंटर"));
    }
    return StormLangStorm11(
        QStringLiteral("STORM HYPERION GLASS 3D — Голографическое 3D-стекло"),
        QStringLiteral("STORM HYPERION GLASS 3D — Holographic 3D Glass"),
        QStringLiteral("STORM HYPERION GLASS 3D — Holografisches 3D-Glas"),
        QStringLiteral("STORM HYPERION GLASS 3D — Verre 3D holographique"),
        QStringLiteral("STORM HYPERION GLASS 3D — 全息 3D 玻璃"),
        QStringLiteral("STORM HYPERION GLASS 3D — 홀로그래픽 3D 글래스"),
        QStringLiteral("STORM HYPERION GLASS 3D — ホログラフィック 3D グラス"),
        QStringLiteral("STORM HYPERION GLASS 3D — Cristal 3D holográfico"),
        QStringLiteral("STORM HYPERION GLASS 3D — Vetro 3D olografico"),
        QStringLiteral("STORM HYPERION GLASS 3D — Vidro 3D holográfico"),
        QStringLiteral("STORM HYPERION GLASS 3D — होलोग्राफिक 3D ग्लास"));
}

QString GetLocalizedDesignTooltip() {
    return StormLangStorm11(
        QStringLiteral("Полная архитектурная смена 3D-оформления интерфейса, формы объёмных кнопок, полей ввода, фасок, теней, фонов и плавных 60/120 FPS анимаций. Полностью совместима со всеми 9 цветовыми темами и 13 акцентами."),
        QStringLiteral("Complete architectural UI transformation changing 3D button geometry, input wells, bevels, shadows, ambient backdrops, and smooth 60/120 FPS animations. Works seamlessly across all 9 color themes and 13 accent colors."),
        QStringLiteral("Vollständige 3D-UI-Architektur für Tastenformen, Eingabefelder, Schatten, Hintergründe und flüssige Animationen — kompatibel mit allen 9 Farbthemen."),
        QStringLiteral("Architecture complète d'interface 3D transformant boutons, champs, ombres, arrière-plans et animations fluides — compatible avec les 9 thèmes."),
        QStringLiteral("完整切换 3D 立体按钮几何造型、输入框凹槽、阴影、动态背景与流畅动画，完美兼容全部 9 款配色主题与 13 种强调色。"),
        QStringLiteral("버튼 입체 형상, 입력창, 그림자, 배경 및 부드러운 애니메이션을 완전히 전환하며 모든 9개 컬러 테마와 완벽 호환됩니다."),
        QStringLiteral("3Dボタン形状、入力欄、陰影、背景、滑らかなアニメーションを全面的に切り替えます（全9カラーテーマ対応）。"),
        QStringLiteral("Transformación completa de la arquitectura 3D de botones, campos, sombras, fondos y animaciones fluidas, compatible con los 9 temas."),
        QStringLiteral("Trasformazione completa dell'architettura 3D di pulsanti, campi, ombre, sfondi e animazioni fluide, compatibile con tutti i 9 temi."),
        QStringLiteral("Transformação completa da arquitetura 3D de botões, campos, sombras, fundos e animações suaves, compatível com todos os 9 temas."),
        QStringLiteral("बटन, इनपुट फ़ील्ड, शैडो, बैकग्राउंड और स्मूथ एनिमेशन का पूर्ण 3D डिज़ाइन बदलाव — सभी 9 थीम के साथ संगत।"));
}

QString BuildDialogStylesheet(const StormTheme::Palette& pal, const std::string& explicit_design) {
    const Preset preset = GetActivePreset(explicit_design);
    const DerivedTokens t = ComputeTokens(pal, preset);
    QString qss = QStringLiteral(
        "QDialog {"
        "  background: %1;"
        "  background-color: %2;"
        "  color: %3;"
        "}")
        .arg(t.windowBackdrop, pal.bg, pal.textPrimary);
    const QString accentHex = pal.accent.isEmpty() ? QStringLiteral("#8B5CF6") : pal.accent;
    const QColor accCol(accentHex);
    const QString accentRgba15 = RgbaCss(accCol, 38);
    const QString accentRgba30 = RgbaCss(accCol, 76);

    qss += QStringLiteral(
        "QLineEdit#SettingsSearchEdit, QLineEdit[stormSearchBox=\"true\"] {"
        "  background-color: %1; border: 1px solid %2; border-radius: 8px;"
        "  padding: 6px 14px; color: %3; font-size: 12.5px; min-height: 32px;"
        "}"
        "QLineEdit#SettingsSearchEdit:focus, QLineEdit[stormSearchBox=\"true\"]:focus {"
        "  border: 1.5px solid %4; background-color: %5;"
        "}"
        "QListWidget#SettingsSearchResultsList {"
        "  background-color: %5; color: %3; border: 1.5px solid %4; border-radius: 7px; padding: 4px;"
        "}"
        "QListWidget#SettingsSearchResultsList::item {"
        "  padding: 6px 10px; border-radius: 5px; color: %3;"
        "}"
        "QListWidget#SettingsSearchResultsList::item:hover, QListWidget#SettingsSearchResultsList::item:selected {"
        "  background-color: %4; color: #FFFFFF;"
        "}"
        "QLabel#StormSectionBadge, QLabel[stormSectionBadge=\"true\"] {"
        "  background: %6; border: 1px solid %4; color: %4; border-radius: 4px; padding: 2px 6px; font-weight: bold; font-size: 8.5pt;"
        "}"
        "QPushButton#StormGamePill, QPushButton[stormGamePill=\"true\"] {"
        "  background: rgba(255, 255, 255, 0.08); border: 1px solid %2; color: %3; border-radius: 12px; padding: 3px 10px; font-weight: 600; font-size: 8.5pt;"
        "}"
        "QPushButton#StormGamePill:hover, QPushButton[stormGamePill=\"true\"]:hover {"
        "  background: %7; border-color: %4; color: %4;"
        "}"
        "QGroupBox[stormCard=\"true\"], QGroupBox#groupBox[stormHeroCard=\"true\"], QGroupBox#BentoGameHeaderCard {"
        "  background-color: %8; border: 1px solid %2; border-radius: 10px; margin-top: 14px; padding: 24px 14px 14px 14px;"
        "}"
        "QGroupBox[stormCard=\"true\"]::title, QGroupBox#groupBox[stormHeroCard=\"true\"]::title, QGroupBox#BentoGameHeaderCard::title {"
        "  subcontrol-origin: margin; subcontrol-position: top left; left: 14px; padding: 2px 10px;"
        "  color: %4; font-size: 12px; font-weight: bold; background-color: %5; border: 1px solid %2; border-radius: 4px;"
        "}"
        "QWidget[stormOverrideRow=\"true\"], ConfigurationShared--Widget[stormOverrideRow=\"true\"] {"
        "  border-left: 3px solid %4; background-color: %6; border-radius: 4px;"
        "}"
    ).arg(pal.inputBg, pal.border, pal.textPrimary, accentHex, pal.surface,
         accentRgba15, accentRgba30, RgbaCss(QColor(pal.surface), 200));

    qss += BuildCoreControlsQss(pal, t);
    return qss;
}

QString BuildConfigDialogStylesheet(const StormTheme::Palette& pal, bool is_per_game,
                                    const std::string& explicit_design) {
    const Preset preset = GetActivePreset(explicit_design);
    const DerivedTokens t = ComputeTokens(pal, preset);

    QString qss = QStringLiteral(
        "QDialog#ConfigureDialog, QDialog#ConfigurePerGame {"
        "  background: %1;"
        "  background-color: %2;"
        "  color: %3;"
        "}")
        .arg(t.windowBackdrop, pal.bg, pal.textPrimary);

    if (is_per_game) {
        qss += QStringLiteral(
            "QFrame#gameInfoCard {"
            "  background-color: %1; border-top: 1.5px solid %2; border-left: 1px solid %3; border-right: 1px solid %3; border-bottom: 3px solid %4; border-radius: %5;"
            "}"
            "QLineEdit#display_name, QTextEdit#display_filename {"
            "  background-color: %6; border: 1px solid %3; border-radius: %7; padding: 4px 8px; color: %8; font-size: 12px;"
            "}"
            "QLineEdit#display_developer, QLineEdit#display_version, QLineEdit#display_format, QLineEdit#display_size {"
            "  background-color: %6; border: 1px solid %3; border-radius: %7; padding: 3px 8px; color: %8; font-size: 11.5px;"
            "}"
            "QLineEdit#display_title_id {"
            "  background-color: %6; border: 1px solid %3; border-radius: %7; padding: 3px 8px; color: %9;"
            "  font-family: 'Consolas', 'Courier New', monospace; font-size: 11.5px; font-weight: 700;"
            "}")
            .arg(pal.surface, RgbaCss(t.accentLight, 120), pal.border, t.deepShadow.name(), t.cardRadius,
                 pal.inputBg, t.inputRadius, pal.textPrimary, pal.accent);
    }

    qss += BuildCoreControlsQss(pal, t);
    return qss;
}

QString BuildMainWindowDesignOverlay(const StormTheme::Palette& pal, const std::string& explicit_design) {
    const Preset preset = GetActivePreset(explicit_design);
    const DerivedTokens t = ComputeTokens(pal, preset);

    return QStringLiteral(
        "\n/* === STORM 3D UI DESIGN ARCHITECTURE OVERLAY === */\n"
        "QMainWindow, QWidget#centralwidget {"
        "  background: %1;"
        "  background-color: %2;"
        "  color: %3;"
        "}"
        "QMenuBar {"
        "  background-color: %4;"
        "  color: %3;"
        "  border-bottom: 1.5px solid %5;"
        "  padding: 2px 6px;"
        "}"
        "QMenuBar::item {"
        "  padding: 5px 11px;"
        "  border-radius: %6;"
        "  background: transparent;"
        "  font-weight: 600;"
        "}"
        "QMenuBar::item:selected {"
        "  background-color: %7;"
        "  color: #FFFFFF;"
        "}"
        "QToolBar {"
        "  background-color: %4;"
        "  border: none;"
        "  border-bottom: 2px solid %8;"
        "  spacing: 6px;"
        "  padding: 4px 8px;"
        "}"
        "QToolBar QToolButton {"
        "  %9"
        "  color: %3;"
        "}"
        "QToolBar QToolButton:hover {"
        "  %10"
        "  color: %3;"
        "}"
        "QToolBar QToolButton:pressed {"
        "  %11"
        "  color: %3;"
        "}"
        "QStatusBar {"
        "  background-color: %4;"
        "  color: %3;"
        "  border-top: 1.5px solid %5;"
        "}"
        "QHeaderView::section {"
        "  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %12, stop:1 %4);"
        "  color: %3;"
        "  border: none;"
        "  border-right: 1px solid %5;"
        "  border-bottom: 2px solid %13;"
        "  padding: 6px 10px;"
        "  font-weight: 800;"
        "  text-align: center;"
        "  qproperty-alignment: AlignCenter;"
        "}"
        "QTableCornerButton::section, QTableView QTableCornerButton::section, QTableWidget QTableCornerButton::section {"
        "  background-color: %4;"
        "  border: none;"
        "  border-right: 1px solid %5;"
        "  border-bottom: 2px solid %13;"
        "}\n")
        .arg(t.windowBackdrop, pal.bg, pal.textPrimary, pal.surface, pal.border,
             t.btnRadius, RgbaCss(t.accent, 145), t.deepShadow.name(),
             t.btnNormal, t.btnHover, t.btnPressed, pal.card, pal.accent);
}

void InstallGlobalAnimator() {
    if (!qApp) {
        return;
    }
    if (qApp->property("_stormDesignAnimatorInstalled").toBool()) {
        return;
    }
    qApp->setProperty("_stormDesignAnimatorInstalled", true);
    qApp->installEventFilter(new GlobalDesignAnimator(qApp));
}

} // namespace StormDesign

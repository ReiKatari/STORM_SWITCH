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
    case Preset::TitaniumCyberDeck: {
        t.cardRadius = QStringLiteral("6px");
        t.btnRadius = QStringLiteral("4px");
        t.inputRadius = QStringLiteral("14px");
        t.tabRadius = QStringLiteral("4px");
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
            "border-radius: 4px;"
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
            "border-radius: 14px;")
            .arg(pal.inputBg, pal.accent, t.deepShadow.name(), pal.border);
        t.inputHover = QStringLiteral(
            "border-left: 3.5px solid %1; border-top: 1px solid %1; border-right: 1px solid %1; border-bottom: 1.5px solid %1; background-color: %2; border-radius: 14px;")
            .arg(t.accentLight.name(), pal.surface);
        t.inputFocus = QStringLiteral(
            "border: 1.5px solid %1; border-left: 4px solid %1; background-color: %2; border-radius: 14px;")
            .arg(t.accentLight.name(), pal.surface);

        t.groupBoxRule = QStringLiteral(
            "background-color: %1;"
            "border-left: 3.5px solid %2;"
            "border-top: 1px solid %3;"
            "border-right: 1px solid %3;"
            "border-bottom: 3px solid %4;"
            "border-radius: 6px;"
            "margin-top: 14px;"
            "padding: 18px 14px 13px 14px;")
            .arg(surfGlass, pal.accent, pal.border, t.deepShadow.name());
        t.groupTitleRule = QStringLiteral(
            "subcontrol-origin: margin; subcontrol-position: top left; top: 1px; left: 10px;"
            "padding: 2px 10px; border-radius: 4px; font-weight: 800; font-size: 12px;"
            "color: %1; background-color: %2; border-left: 3px solid %3; border-top: 1px solid %3; border-right: 1px solid %4; border-bottom: 1px solid %4;")
            .arg(pal.textPrimary, pal.card, pal.accent, pal.border);
        break;
    }

    case Preset::HyperionDockedConsole:
    default: {
        t.cardRadius = QStringLiteral("14px");
        t.btnRadius = QStringLiteral("10px");
        t.inputRadius = QStringLiteral("14px");
        t.tabRadius = QStringLiteral("10px");
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
            "border-radius: 14px;")
            .arg(t.deepWell.name(), pal.inputBg, t.deepShadow.name(), pal.border, specSoft);
        t.inputHover = QStringLiteral(
            "border: 1.5px solid %1; background-color: %2; border-radius: 14px;")
            .arg(pal.accent, pal.surface);
        t.inputFocus = QStringLiteral(
            "border: 1.5px solid %1; background-color: %2; border-radius: 14px;")
            .arg(t.accentLight.name(), pal.surface);

        t.groupBoxRule = QStringLiteral(
            "background-color: %1;"
            "border-top: 1.5px solid %2;"
            "border-left: 1px solid %3;"
            "border-right: 1px solid %3;"
            "border-bottom: 3px solid %4;"
            "border-radius: 14px;"
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

    case Preset::ValkyrieTacticalRig: {
        t.cardRadius = QStringLiteral("4px");
        t.btnRadius = QStringLiteral("4px");
        t.inputRadius = QStringLiteral("14px");
        t.tabRadius = QStringLiteral("4px");
        t.windowBackdrop = QStringLiteral(
            "qlineargradient(x1:0, y1:0, x2:1, y2:1, "
            "stop:0 #0B0F14, stop:0.50 #121820, stop:1 #080B0F)");

        t.btnNormal = QStringLiteral(
            "background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);"
            "border-left: 3px solid #10B981;"
            "border-top: 1px solid %3;"
            "border-right: 1px solid %3;"
            "border-bottom: 3px solid #065F46;"
            "border-radius: 4px;"
            "padding: 6px 16px;")
            .arg(btnTop.name(), btnBot.name(), pal.border);
        t.btnHover = QStringLiteral(
            "background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 %1, stop:1 %2);"
            "border-left: 3.5px solid #34D399;"
            "border-top: 1px solid #34D399;"
            "border-right: 1px solid #059669;"
            "border-bottom: 3px solid #065F46;")
            .arg(hoverTop.name(), btnMid.name());
        t.btnPressed = QStringLiteral(
            "background-color: %1;"
            "border: 1.5px solid #10B981;"
            "padding: 8px 16px 4px 16px;")
            .arg(btnBot.name());

        t.inputNormal = QStringLiteral(
            "background-color: %1;"
            "border: 1px solid %2;"
            "border-left: 3px solid #10B981;"
            "border-radius: 14px;")
            .arg(pal.inputBg, pal.border);
        t.inputHover = QStringLiteral(
            "border: 1.5px solid #34D399; background-color: %1; border-radius: 14px;")
            .arg(pal.surface);
        t.inputFocus = QStringLiteral(
            "border: 1.5px solid #34D399; border-left: 4px solid #10B981; background-color: %1; border-radius: 14px;")
            .arg(pal.surface);

        t.groupBoxRule = QStringLiteral(
            "background-color: %1;"
            "border-left: 3px solid #10B981;"
            "border-top: 1px solid %2;"
            "border-right: 1px solid %2;"
            "border-bottom: 3px solid #065F46;"
            "border-radius: 4px;"
            "margin-top: 14px;"
            "padding: 18px 14px 13px 14px;")
            .arg(surfGlass, pal.border);
        t.groupTitleRule = QStringLiteral(
            "subcontrol-origin: margin; subcontrol-position: top left; top: 1px; left: 10px;"
            "padding: 2px 10px; border-radius: 3px; font-weight: 800; font-size: 12px;"
            "color: #34D399; background-color: %1; border: 1px solid #10B981;")
            .arg(pal.surface);
        break;
    }

    case Preset::SolarisSwitch2Pro: {
        t.cardRadius = QStringLiteral("16px");
        t.btnRadius = QStringLiteral("12px");
        t.inputRadius = QStringLiteral("14px");
        t.tabRadius = QStringLiteral("12px");
        t.windowBackdrop = QStringLiteral(
            "qlineargradient(x1:0, y1:0, x2:1, y2:1, "
            "stop:0 #0F1219, stop:0.48 #141824, stop:1 #0A0D14)");

        t.btnNormal = QStringLiteral(
            "background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);"
            "border-top: 1.5px solid rgba(255, 59, 48, 0.65);"
            "border-left: 1px solid %3;"
            "border-right: 1px solid %3;"
            "border-bottom: 3px solid rgba(0, 229, 255, 0.75);"
            "border-radius: 12px;"
            "padding: 6px 16px;")
            .arg(btnTop.name(), btnBot.name(), pal.border);
        t.btnHover = QStringLiteral(
            "background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);"
            "border: 1.5px solid #00E5FF;"
            "border-top: 2px solid #FF3B30;")
            .arg(hoverTop.name(), btnMid.name());
        t.btnPressed = QStringLiteral(
            "background-color: %1;"
            "border: 1.5px solid #FF3B30;"
            "padding: 8px 16px 4px 16px;")
            .arg(btnBot.name());

        t.inputNormal = QStringLiteral(
            "background-color: %1;"
            "border: 1.5px solid rgba(0, 229, 255, 0.35);"
            "border-radius: 14px;")
            .arg(pal.inputBg);
        t.inputHover = QStringLiteral(
            "border: 1.5px solid #00E5FF; background-color: %1; border-radius: 14px;")
            .arg(pal.surface);
        t.inputFocus = QStringLiteral(
            "border: 1.5px solid #00E5FF; border-left: 3px solid #FF3B30; background-color: %1; border-radius: 14px;")
            .arg(pal.surface);

        t.groupBoxRule = QStringLiteral(
            "background-color: %1;"
            "border: 1px solid rgba(0, 229, 255, 0.25);"
            "border-top: 2px solid rgba(255, 59, 48, 0.75);"
            "border-radius: 16px;"
            "margin-top: 14px;"
            "padding: 18px 14px 13px 14px;")
            .arg(cardGlass);
        t.groupTitleRule = QStringLiteral(
            "subcontrol-origin: margin; subcontrol-position: top left; top: 1px; left: 14px;"
            "padding: 2px 10px; border-radius: 8px; font-weight: 800; font-size: 12px;"
            "color: #00E5FF; background-color: %1; border: 1px solid rgba(0, 229, 255, 0.45);")
            .arg(pal.surface);
        break;
    }

    case Preset::AetherFrostedStudio: {
        t.cardRadius = QStringLiteral("12px");
        t.btnRadius = QStringLiteral("8px");
        t.inputRadius = QStringLiteral("14px");
        t.tabRadius = QStringLiteral("8px");
        t.windowBackdrop = QStringLiteral(
            "qradialgradient(cx:0.5, cy:0.2, radius:1.2, fx:0.5, fy:0.2, "
            "stop:0 #1E293B, stop:0.55 #0F172A, stop:1 #020617)");

        t.btnNormal = QStringLiteral(
            "background-color: rgba(30, 41, 59, 0.70);"
            "border: 1px solid rgba(148, 163, 184, 0.25);"
            "border-top: 1.5px solid rgba(255, 255, 255, 0.35);"
            "border-bottom: 2.5px solid rgba(15, 23, 42, 0.85);"
            "border-radius: 8px;"
            "padding: 6px 16px;")
            .arg(btnTop.name(), btnBot.name());
        t.btnHover = QStringLiteral(
            "background-color: rgba(51, 65, 85, 0.85);"
            "border: 1px solid %1;"
            "border-top: 1.5px solid #FFFFFF;")
            .arg(pal.accent);
        t.btnPressed = QStringLiteral(
            "background-color: rgba(15, 23, 42, 0.95);"
            "border: 1px solid %1;"
            "padding: 8px 16px 4px 16px;")
            .arg(pal.accent);

        t.inputNormal = QStringLiteral(
            "background-color: rgba(15, 23, 42, 0.65);"
            "border: 1px solid rgba(148, 163, 184, 0.25);"
            "border-radius: 14px;")
            .arg(pal.inputBg);
        t.inputHover = QStringLiteral(
            "border: 1.5px solid %1; background-color: rgba(30, 41, 59, 0.75); border-radius: 14px;")
            .arg(pal.accent);
        t.inputFocus = QStringLiteral(
            "border: 1.5px solid %1; background-color: rgba(30, 41, 59, 0.85); border-radius: 14px;")
            .arg(t.accentLight.name());

        t.groupBoxRule = QStringLiteral(
            "background-color: rgba(15, 23, 42, 0.65);"
            "border: 1px solid rgba(148, 163, 184, 0.20);"
            "border-top: 1.5px solid rgba(255, 255, 255, 0.25);"
            "border-radius: 12px;"
            "margin-top: 14px;"
            "padding: 18px 14px 13px 14px;");
        t.groupTitleRule = QStringLiteral(
            "subcontrol-origin: margin; subcontrol-position: top left; top: 1px; left: 12px;"
            "padding: 2px 10px; border-radius: 6px; font-weight: 800; font-size: 12px;"
            "color: #93C5FD; background-color: #0F172A; border: 1px solid rgba(148, 163, 184, 0.30);");
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
    if (id == "titanium_cyber_deck" || id == "cyber_deck") {
        return Preset::TitaniumCyberDeck;
    }
    if (id == "hyperion_docked_console" || id == "hyperion_glass") {
        return Preset::HyperionDockedConsole;
    }
    if (id == "valkyrie_tactical_rig" || id == "orbital_spatial") {
        return Preset::ValkyrieTacticalRig;
    }
    if (id == "solaris_switch2_pro" || id == "chrono_nexus") {
        return Preset::SolarisSwitch2Pro;
    }
    if (id == "aether_frosted_studio" || id == "aurora_quantum") {
        return Preset::AetherFrostedStudio;
    }
    return Preset::TitaniumCyberDeck;
}

std::string GetActiveDesignId() {
    const std::string val = UISettings::values.ui_design.GetValue();
    for (const auto& [id, _] : kDesigns) {
        if (val == id) {
            return val;
        }
    }
    // Backward compatibility mappings
    if (val == "cyber_deck") {
        return "titanium_cyber_deck";
    }
    if (val == "hyperion_glass") {
        return "hyperion_docked_console";
    }
    if (val == "orbital_spatial") {
        return "valkyrie_tactical_rig";
    }
    if (val == "chrono_nexus") {
        return "solaris_switch2_pro";
    }
    if (val == "aurora_quantum") {
        return "aether_frosted_studio";
    }
    return "titanium_cyber_deck";
}

QString GetDesignSelectorLabel() {
    return StormLangStorm11(
        QStringLiteral("Вид интерфейса:"),
        QStringLiteral("UI Layout View:"),
        QStringLiteral("UI-Layout-Ansicht:"),
        QStringLiteral("Vue de l'interface :"),
        QStringLiteral("界面布局视图："),
        QStringLiteral("인터페이스 레이아웃 뷰:"),
        QStringLiteral("UIレイアウトビュー："),
        QStringLiteral("Vista de diseño de UI:"),
        QStringLiteral("Vista layout interfaccia:"),
        QStringLiteral("Visualização do layout da UI:"),
        QStringLiteral("इंटरफ़ेस लेआउट दृश्य:"));
}

QString GetLocalizedDesignName(const std::string& design_id) {
    if (design_id == "titanium_cyber_deck" || design_id == "cyber_deck") {
        return StormLangStorm11(
            QStringLiteral("TITANIUM CYBER DECK — Двухпалубный командный мост и досье"),
            QStringLiteral("TITANIUM CYBER DECK — Dual-Deck Command Bridge & Dossier"),
            QStringLiteral("TITANIUM CYBER DECK — Doppeldeck-Kommandobrücke & Dossier"),
            QStringLiteral("TITANIUM CYBER DECK — Pont de commandement double & dossier"),
            QStringLiteral("TITANIUM CYBER DECK — 双层指挥桥与档案库"),
            QStringLiteral("TITANIUM CYBER DECK — 듀얼 데크 지휘 브릿지 & 도시에"),
            QStringLiteral("TITANIUM CYBER DECK — デュアルデッキ司令ブリッジ＆ドシエ"),
            QStringLiteral("TITANIUM CYBER DECK — Puente de mando de doble cubierta y expediente"),
            QStringLiteral("TITANIUM CYBER DECK — Ponte di comando a doppio ponte e dossier"),
            QStringLiteral("TITANIUM CYBER DECK — Ponte de comando de convés duplo e dossiê"),
            QStringLiteral("TITANIUM CYBER DECK — डुअल-डेक कमांड ब्रिज और डोज़ियर"));
    }
    if (design_id == "hyperion_docked_console" || design_id == "hyperion_glass") {
        return StormLangStorm11(
            QStringLiteral("HYPERION DOCKED CONSOLE — Консольный стиль и Bento-плитки"),
            QStringLiteral("HYPERION DOCKED CONSOLE — Docked Console & Bento Tiles"),
            QStringLiteral("HYPERION DOCKED CONSOLE — Docked-Konsole & Bento-Kacheln"),
            QStringLiteral("HYPERION DOCKED CONSOLE — Console dockée & tuiles Bento"),
            QStringLiteral("HYPERION DOCKED CONSOLE — 主机基座模式与 Bento 磁贴"),
            QStringLiteral("HYPERION DOCKED CONSOLE — 독형 콘솔 & 벤토 타일"),
            QStringLiteral("HYPERION DOCKED CONSOLE — ドック型コンソール＆Bentoタイル"),
            QStringLiteral("HYPERION DOCKED CONSOLE — Consola acoplada y mosaicos Bento"),
            QStringLiteral("HYPERION DOCKED CONSOLE — Console docked e riquadri Bento"),
            QStringLiteral("HYPERION DOCKED CONSOLE — Console acoplado e blocos Bento"),
            QStringLiteral("HYPERION DOCKED CONSOLE — डॉक्ड कंसोल और बेंटो टाइलें"));
    }
    if (design_id == "valkyrie_tactical_rig" || design_id == "orbital_spatial") {
        return StormLangStorm11(
            QStringLiteral("VALKYRIE TACTICAL RIG — Киберспортивный тактический риг"),
            QStringLiteral("VALKYRIE TACTICAL RIG — Esports Tactical Carbon Rig"),
            QStringLiteral("VALKYRIE TACTICAL RIG — E-Sport Taktik-Rig"),
            QStringLiteral("VALKYRIE TACTICAL RIG — Rig tactique d'esport"),
            QStringLiteral("VALKYRIE TACTICAL RIG — 电子竞技战术碳纤维装备"),
            QStringLiteral("VALKYRIE TACTICAL RIG — e스포츠 전술 카본 리그"),
            QStringLiteral("VALKYRIE TACTICAL RIG — eスポーツ・タクティカルリグ"),
            QStringLiteral("VALKYRIE TACTICAL RIG — Rig táctico de esports"),
            QStringLiteral("VALKYRIE TACTICAL RIG — Rig tattico per esports"),
            QStringLiteral("VALKYRIE TACTICAL RIG — Rig tático de esports"),
            QStringLiteral("VALKYRIE TACTICAL RIG — ई-स्पोर्ट्स सामरिक कार्बन रिग"));
    }
    if (design_id == "solaris_switch2_pro" || design_id == "chrono_nexus") {
        return StormLangStorm11(
            QStringLiteral("SOLARIS SWITCH 2 PRO — Флагманская консольная эстетика Switch 2"),
            QStringLiteral("SOLARIS SWITCH 2 PRO — Flagship Switch 2 Pro Console Aesthetics"),
            QStringLiteral("SOLARIS SWITCH 2 PRO — Flaggschiff Switch 2 Pro Ästhetik"),
            QStringLiteral("SOLARIS SWITCH 2 PRO — Esthétique de console Switch 2 Pro"),
            QStringLiteral("SOLARIS SWITCH 2 PRO — 旗舰 Switch 2 Pro 主机美学"),
            QStringLiteral("SOLARIS SWITCH 2 PRO — 플래그십 Switch 2 Pro 콘솔 미학"),
            QStringLiteral("SOLARIS SWITCH 2 PRO — フラッグシップ Switch 2 Pro コンソール美学"),
            QStringLiteral("SOLARIS SWITCH 2 PRO — Estética de consola Switch 2 Pro"),
            QStringLiteral("SOLARIS SWITCH 2 PRO — Estetica console Switch 2 Pro"),
            QStringLiteral("SOLARIS SWITCH 2 PRO — Estética de console Switch 2 Pro"),
            QStringLiteral("SOLARIS SWITCH 2 PRO — फ्लैगशिप स्विच 2 प्रो कंसोल सौंदर्य"));
    }
    return StormLangStorm11(
        QStringLiteral("AETHER FROSTED STUDIO — Сапфировое матовое стекло и инспектор"),
        QStringLiteral("AETHER FROSTED STUDIO — Sapphire Frosted Glass & Master Inspector"),
        QStringLiteral("AETHER FROSTED STUDIO — Saphir-Mattglas & Master-Inspektor"),
        QStringLiteral("AETHER FROSTED STUDIO — Verre dépoli saphir & inspecteur maître"),
        QStringLiteral("AETHER FROSTED STUDIO — 蓝宝石磨砂玻璃与主监视器"),
        QStringLiteral("AETHER FROSTED STUDIO — 사파이어 프로스트 글래스 & 마스터 인스펙터"),
        QStringLiteral("AETHER FROSTED STUDIO — サファイアすりガラス＆マスターインスペクター"),
        QStringLiteral("AETHER FROSTED STUDIO — Cristal esmerilado de zafiro e inspector maestro"),
        QStringLiteral("AETHER FROSTED STUDIO — Vetro smerigliato zaffiro e ispettore master"),
        QStringLiteral("AETHER FROSTED STUDIO — Vidro fosco de safira e inspetor master"),
        QStringLiteral("AETHER FROSTED STUDIO — नीलम फ्रॉस्टेड ग्लास और मास्टर इंस्पेक्टर"));
}

QString GetLocalizedDesignTooltip() {
    return StormLangStorm11(
        QStringLiteral("Мгновенное переключение одного из 5 утверждённых архитектурных видов интерфейса (Titanium Cyber Deck, Hyperion Docked Console, Valkyrie Tactical Rig, Solaris Switch 2 Pro, Aether Frosted Studio) в реальном времени без перезапуска приложения."),
        QStringLiteral("Instant real-time switching between the 5 approved architectural UI views (Titanium Cyber Deck, Hyperion Docked Console, Valkyrie Tactical Rig, Solaris Switch 2 Pro, Aether Frosted Studio) on the fly without restarting."),
        QStringLiteral("Sofortiges Umschalten zwischen den 5 genehmigten UI-Architekturansichten in Echtzeit ohne Neustart."),
        QStringLiteral("Bascule instantanée en temps réel entre les 5 vues d'architecture d'interface sans redémarrer."),
        QStringLiteral("在 5 种官方 UI 架构视图之间即时平滑切换，完全无需重启即可生效。"),
        QStringLiteral("재시작 없이 5가지 공인 인터페이스 아키텍처 뷰 간을 실시간으로 즉시 전환합니다。"),
        QStringLiteral("再起動なしで5つの認定UIアーキテクチャビューをリアルタイムで瞬時に切り替えます。"),
        QStringLiteral("Cambio instantáneo en tiempo real entre las 5 vistas de arquitectura de UI sin reiniciar."),
        QStringLiteral("Passaggio istantaneo in tempo reale tra le 5 viste di architettura dell'interfaccia senza riavviare."),
        QStringLiteral("Alternância instantânea em tempo real entre as 5 visualizações de arquitetura de UI sem reiniciar."),
        QStringLiteral("बिना पुनरारंभ किए 5 स्वीकृत इंटरफ़ेस आर्किटेक्चर दृश्यों के बीच त्वरित रीयल-टाइम स्विचिंग।"));
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
        /* Recessed 3D Search Capsule (Zero Double Border) */
        "QWidget#SearchCapsuleWidget {"
        "  background-color: %4;"
        "  border: 1.5px solid %5;"
        "  border-radius: 14px;"
        "  padding: 3px 10px;"
        "  min-height: 32px;"
        "}"
        "QWidget#SearchCapsuleWidget:focus-within {"
        "  border: 1.5px solid %13;"
        "  background-color: %12;"
        "}"
        "QLineEdit#SearchCapsuleEdit {"
        "  border: none !important;"
        "  outline: none !important;"
        "  background: transparent !important;"
        "  color: %3;"
        "  font-size: 13px;"
        "  padding: 2px 4px;"
        "}"
        "QToolButton#SearchCapsuleClear {"
        "  border: none;"
        "  background: transparent;"
        "  padding: 2px;"
        "  border-radius: 8px;"
        "}"
        "QToolButton#SearchCapsuleClear:hover {"
        "  background-color: rgba(239, 68, 68, 0.25);"
        "}"

        /* STORM Architecture Panels */
        "QFrame#StormDossierPanel {"
        "  background-color: %4;"
        "  border-left: 2px solid %13;"
        "  border-top: none;"
        "  border-right: none;"
        "  border-bottom: none;"
        "}"
        "QFrame#StormTelemetryBar {"
        "  background-color: %4;"
        "  border-bottom: 1.5px solid %5;"
        "  padding: 4px 12px;"
        "}"
        "QFrame#StormHeroBlock {"
        "  background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %12, stop:1 %4);"
        "  border-bottom: 2px solid %13;"
        "  padding: 12px 20px;"
        "}"
        "QFrame#StormConsoleBottomBar {"
        "  background-color: %4;"
        "  border-top: 1.5px solid %5;"
        "  padding: 6px 16px;"
        "}"

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

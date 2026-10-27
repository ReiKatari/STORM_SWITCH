// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QEasingCurve>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QRadialGradient>
#include <QScreen>
#include "qt_common/config/uisettings.h"
#include "storm_switch/common/storm_lang.h"
#include "storm_switch/in_game_notification.h"

namespace {

struct NotificationThemePalette {
    QColor bg_top;
    QColor bg_bottom;
    QColor card_inner_border;
    QColor title_color;
    QColor subtitle_color;
    QColor badge_bg;
    QColor badge_text;
    QColor theme_accent;
    bool is_light{false};
};

NotificationThemePalette ResolveActiveThemePalette(const QColor& requested_accent) {
    const QString theme = QString::fromStdString(UISettings::values.theme).toLower();
    NotificationThemePalette pal;

    if (theme.startsWith(QStringLiteral("light"))) {
        pal.is_light = true;
        pal.bg_top = QColor(248, 250, 255, 248);
        pal.bg_bottom = QColor(235, 241, 251, 252);
        pal.card_inner_border = QColor(15, 23, 42, 28);
        pal.title_color = QColor(15, 23, 42);
        pal.subtitle_color = QColor(51, 65, 85);
        pal.theme_accent = QColor(2, 132, 199);
    } else if (theme.startsWith(QStringLiteral("solarized"))) {
        pal.is_light = false;
        pal.bg_top = QColor(0, 43, 54, 246);
        pal.bg_bottom = QColor(7, 54, 66, 252);
        pal.card_inner_border = QColor(147, 161, 161, 42);
        pal.title_color = QColor(253, 246, 227);
        pal.subtitle_color = QColor(147, 161, 161);
        pal.theme_accent = QColor(42, 161, 152);
    } else if (theme.startsWith(QStringLiteral("cyberpunk"))) {
        pal.bg_top = QColor(22, 9, 41, 246);
        pal.bg_bottom = QColor(11, 4, 24, 252);
        pal.card_inner_border = QColor(255, 0, 127, 55);
        pal.title_color = QColor(253, 242, 248);
        pal.subtitle_color = QColor(244, 114, 182);
        pal.theme_accent = QColor(255, 0, 127);
    } else if (theme.startsWith(QStringLiteral("emerald"))) {
        pal.bg_top = QColor(6, 32, 24, 246);
        pal.bg_bottom = QColor(3, 18, 14, 252);
        pal.card_inner_border = QColor(16, 185, 129, 50);
        pal.title_color = QColor(236, 253, 245);
        pal.subtitle_color = QColor(167, 243, 208);
        pal.theme_accent = QColor(16, 185, 129);
    } else if (theme.startsWith(QStringLiteral("blood_moon"))) {
        pal.bg_top = QColor(36, 8, 14, 246);
        pal.bg_bottom = QColor(20, 4, 8, 252);
        pal.card_inner_border = QColor(244, 63, 94, 52);
        pal.title_color = QColor(255, 241, 242);
        pal.subtitle_color = QColor(254, 205, 211);
        pal.theme_accent = QColor(244, 63, 94);
    } else if (theme.startsWith(QStringLiteral("nord"))) {
        pal.bg_top = QColor(46, 52, 64, 246);
        pal.bg_bottom = QColor(36, 41, 51, 252);
        pal.card_inner_border = QColor(136, 192, 208, 48);
        pal.title_color = QColor(236, 239, 244);
        pal.subtitle_color = QColor(216, 222, 233);
        pal.theme_accent = QColor(136, 192, 208);
    } else if (theme.startsWith(QStringLiteral("sakura"))) {
        pal.bg_top = QColor(34, 14, 28, 246);
        pal.bg_bottom = QColor(21, 8, 18, 252);
        pal.card_inner_border = QColor(244, 114, 182, 52);
        pal.title_color = QColor(253, 242, 248);
        pal.subtitle_color = QColor(251, 207, 232);
        pal.theme_accent = QColor(244, 114, 182);
    } else if (theme.startsWith(QStringLiteral("obsidian")) ||
               theme.startsWith(QStringLiteral("dark"))) {
        pal.bg_top = QColor(18, 18, 24, 248);
        pal.bg_bottom = QColor(9, 9, 13, 252);
        pal.card_inner_border = QColor(148, 163, 184, 38);
        pal.title_color = QColor(248, 250, 252);
        pal.subtitle_color = QColor(203, 213, 225);
        pal.theme_accent = QColor(99, 102, 241);
    } else {
        // Default / Midnight Aurora
        pal.bg_top = QColor(13, 22, 40, 246);
        pal.bg_bottom = QColor(7, 12, 24, 252);
        pal.card_inner_border = QColor(56, 189, 248, 44);
        pal.title_color = QColor(241, 245, 249);
        pal.subtitle_color = QColor(203, 213, 225);
        pal.theme_accent = QColor(0, 229, 255);
    }

    // Blend requested semantic accent with theme accent so status colors (e.g. warning/error/save)
    // stay distinct while harmonizing with the active STORM theme.
    if (requested_accent.isValid()) {
        if (requested_accent == QColor(0, 229, 255)) {
            pal.theme_accent = pal.theme_accent;
        } else {
            pal.theme_accent = QColor(
                (requested_accent.red() * 3 + pal.theme_accent.red()) / 4,
                (requested_accent.green() * 3 + pal.theme_accent.green()) / 4,
                (requested_accent.blue() * 3 + pal.theme_accent.blue()) / 4);
        }
    }

    if (pal.is_light) {
        pal.badge_bg = QColor(pal.theme_accent.red(), pal.theme_accent.green(), pal.theme_accent.blue(), 32);
        pal.badge_text = pal.theme_accent.darker(145);
    } else {
        pal.badge_bg = QColor(pal.theme_accent.red(), pal.theme_accent.green(), pal.theme_accent.blue(), 46);
        pal.badge_text = pal.theme_accent.lighter(135);
    }

    return pal;
}

QString DefaultLocalizedBadgeText() {
    return StormLang11(
        QStringLiteral("STORM SWITCH • УВЕДОМЛЕНИЕ"),
        QStringLiteral("STORM SWITCH • NOTIFICATION"),
        QStringLiteral("STORM SWITCH • HINWEIS"),
        QStringLiteral("STORM SWITCH • NOTIFICATION"),
        QStringLiteral("STORM SWITCH • NOTIFICACIÓN"),
        QStringLiteral("STORM SWITCH • NOTIFICA"),
        QStringLiteral("STORM SWITCH • NOTIFICAÇÃO"),
        QStringLiteral("STORM SWITCH • 系统通知"),
        QStringLiteral("STORM SWITCH • 通知"),
        QStringLiteral("STORM SWITCH • 알림"),
        QStringLiteral("STORM SWITCH • إشعار"));
}

} // namespace

InGameNotificationOverlay::InGameNotificationOverlay(QWidget* parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                          Qt::WindowDoesNotAcceptFocus) {
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::NoFocus);

    setFixedSize(580, 86);

    m_fade_anim = new QPropertyAnimation(this, "panelOpacity", this);
    m_fade_anim->setDuration(220);
    m_fade_anim->setEasingCurve(QEasingCurve::OutCubic);

    m_progress_anim = new QPropertyAnimation(this, "progressValue", this);
    m_progress_anim->setEasingCurve(QEasingCurve::Linear);

    m_hide_timer.setSingleShot(true);
    connect(&m_hide_timer, &QTimer::timeout, this, [this]() {
        m_fade_anim->stop();
        m_fade_anim->disconnect(this);
        m_fade_anim->setDuration(240);
        m_fade_anim->setStartValue(m_panel_opacity);
        m_fade_anim->setEndValue(0.0);
        connect(m_fade_anim, &QPropertyAnimation::finished, this, &QWidget::hide,
                Qt::UniqueConnection);
        m_fade_anim->start();
    });

    hide();
}

void InGameNotificationOverlay::RepositionToParent() {
    if (QWidget* pw = parentWidget()) {
        const QPoint top_left = pw->mapToGlobal(QPoint(0, 0));
        const int x = top_left.x() + (pw->width() - width()) / 2;
        const int y = top_left.y() + 44;
        move(x, y);
    } else if (QScreen* scr = QGuiApplication::primaryScreen()) {
        const QRect sg = scr->availableGeometry();
        move(sg.x() + (sg.width() - width()) / 2, sg.y() + 48);
    }
}

void InGameNotificationOverlay::ShowNotification(const QString& title, const QString& message,
                                                 const QColor& accent, const QString& iconText,
                                                 int displayDurationMs,
                                                 const QString& badgeText) {
    m_title = title;
    m_message = message;
    m_accent_color = accent;
    m_icon = iconText.isEmpty() ? QStringLiteral("⚡") : iconText;
    m_badge_text = badgeText.isEmpty() ? DefaultLocalizedBadgeText() : badgeText;

    RepositionToParent();

    m_hide_timer.stop();
    m_fade_anim->stop();
    m_fade_anim->disconnect(this);
    m_progress_anim->stop();

    m_panel_opacity = 1.0;
    m_progress_value = 1.0;

    m_progress_anim->setDuration(std::max(400, displayDurationMs));
    m_progress_anim->setStartValue(1.0);
    m_progress_anim->setEndValue(0.0);
    m_progress_anim->start();

    show();
    raise();
    repaint();

    QTimer::singleShot(50, this, [this]() {
        if (isVisible()) {
            RepositionToParent();
            raise();
            update();
        }
    });
    QTimer::singleShot(180, this, [this]() {
        if (isVisible()) {
            RepositionToParent();
            raise();
            update();
        }
    });

    m_hide_timer.start(displayDurationMs);
}

void InGameNotificationOverlay::paintEvent(QPaintEvent* /*event*/) {
    if (m_panel_opacity <= 0.01) {
        return;
    }

    const NotificationThemePalette pal = ResolveActiveThemePalette(m_accent_color);
    const QColor accent = pal.theme_accent;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    p.setOpacity(m_panel_opacity);

    const QRect r = rect().adjusted(4, 4, -4, -4);

    // Outer rounded card
    QPainterPath bgPath;
    bgPath.addRoundedRect(r, 16, 16);

    // Theme-matched glassmorphic surface gradient
    QLinearGradient bgGrad(0, r.top(), 0, r.bottom());
    bgGrad.setColorAt(0.0, pal.bg_top);
    bgGrad.setColorAt(1.0, pal.bg_bottom);
    p.fillPath(bgPath, bgGrad);

    // Ambient left radial/horizontal glow wash in theme accent
    QLinearGradient washGrad(r.left(), 0, r.left() + 240, 0);
    QColor washStart = accent;
    washStart.setAlpha(pal.is_light ? 28 : 48);
    QColor washMid = accent;
    washMid.setAlpha(pal.is_light ? 10 : 16);
    washGrad.setColorAt(0.0, washStart);
    washGrad.setColorAt(0.55, washMid);
    washGrad.setColorAt(1.0, QColor(0, 0, 0, 0));
    p.fillPath(bgPath, washGrad);

    // Inner subtle glass highlight border
    QPainterPath innerPath;
    innerPath.addRoundedRect(r.adjusted(1, 1, -1, -1), 15, 15);
    p.setPen(QPen(pal.card_inner_border, 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawPath(innerPath);

    // Outer neon accent border
    QColor outerBorderColor = accent;
    outerBorderColor.setAlpha(pal.is_light ? 190 : 225);
    p.setPen(QPen(outerBorderColor, 1.8));
    p.drawPath(bgPath);

    // Top central glowing accent bar
    QPainterPath topBar;
    topBar.addRoundedRect(QRect(r.x() + 32, r.y(), r.width() - 64, 3), 1.5, 1.5);
    p.fillPath(topBar, accent);

    // Bottom animated countdown progress bar
    const int progFullWidth = r.width() - 36;
    const int progCurWidth = std::max(0, static_cast<int>(progFullWidth * m_progress_value));
    if (progCurWidth > 0) {
        QPainterPath progTrack;
        progTrack.addRoundedRect(QRect(r.x() + 18, r.bottom() - 4, progFullWidth, 3), 1.5, 1.5);
        QColor trackCol = accent;
        trackCol.setAlpha(pal.is_light ? 30 : 40);
        p.fillPath(progTrack, trackCol);

        QPainterPath progFill;
        progFill.addRoundedRect(QRect(r.x() + 18, r.bottom() - 4, progCurWidth, 3), 1.5, 1.5);
        QLinearGradient progGrad(r.x() + 18, 0, r.x() + 18 + progFullWidth, 0);
        progGrad.setColorAt(0.0, accent.lighter(120));
        progGrad.setColorAt(1.0, accent);
        p.fillPath(progFill, progGrad);
    }

    // 3D Volumetric Left Icon Badge
    const QRect iconRect(r.x() + 16, r.y() + (r.height() - 48) / 2, 48, 48);
    QPainterPath iconBadge;
    iconBadge.addRoundedRect(iconRect, 13, 13);

    QLinearGradient badgeGrad(0, iconRect.top(), 0, iconRect.bottom());
    QColor badgeTop = accent;
    badgeTop.setAlpha(pal.is_light ? 45 : 75);
    QColor badgeBot = accent.darker(130);
    badgeBot.setAlpha(pal.is_light ? 25 : 42);
    badgeGrad.setColorAt(0.0, badgeTop);
    badgeGrad.setColorAt(1.0, badgeBot);
    p.fillPath(iconBadge, badgeGrad);

    p.setPen(QPen(accent, 1.5));
    p.drawPath(iconBadge);

    // Icon glyph
    p.setPen(pal.is_light ? accent.darker(150) : QColor(255, 255, 255));
    QFont iconFont(QStringLiteral("Segoe UI Emoji"), 17, QFont::Bold);
    p.setFont(iconFont);
    p.drawText(iconRect, Qt::AlignCenter, m_icon);

    const int textLeft = iconRect.right() + 15;
    const int textMaxWidth = r.right() - textLeft - 16;

    // Top-right / inline category pill badge + Title row
    QFont badgeFont(QStringLiteral("Segoe UI"), 7, QFont::Bold);
    badgeFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.7);
    QFontMetrics badgeFm(badgeFont);
    const QString badgeStr = m_badge_text.toUpper();
    const int badgeW = std::min(195, badgeFm.horizontalAdvance(badgeStr) + 16);
    const QRect badgeRect(r.right() - badgeW - 14, r.y() + 10, badgeW, 18);

    QPainterPath pillPath;
    pillPath.addRoundedRect(badgeRect, 9, 9);
    p.fillPath(pillPath, pal.badge_bg);
    QColor pillBorder = accent;
    pillBorder.setAlpha(110);
    p.setPen(QPen(pillBorder, 1.0));
    p.drawPath(pillPath);

    p.setFont(badgeFont);
    p.setPen(pal.badge_text);
    p.drawText(badgeRect, Qt::AlignCenter,
               badgeFm.elidedText(badgeStr, Qt::ElideRight, badgeRect.width() - 8));

    // Title
    const QRect titleRect(textLeft, r.y() + 11, textMaxWidth - badgeW - 10, 24);
    p.setPen(pal.is_light ? accent.darker(145) : accent.lighter(135));
    QFont titleFont(QStringLiteral("Segoe UI"), 10, QFont::ExtraBold);
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.4);
    p.setFont(titleFont);
    const QString elided_title =
        QFontMetrics(titleFont).elidedText(m_title, Qt::ElideRight, titleRect.width());
    p.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, elided_title);

    // Subtitle / Message
    const QRect msgRect(textLeft, r.y() + 38, textMaxWidth, 26);
    p.setPen(pal.title_color);
    QFont msgFont(QStringLiteral("Segoe UI"), 9, QFont::DemiBold);
    p.setFont(msgFont);
    const QString elided_msg =
        QFontMetrics(msgFont).elidedText(m_message, Qt::ElideRight, msgRect.width());
    p.drawText(msgRect, Qt::AlignLeft | Qt::AlignVCenter, elided_msg);
}

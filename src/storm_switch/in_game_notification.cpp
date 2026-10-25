// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QFontMetrics>
#include <QGuiApplication>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include "storm_switch/in_game_notification.h"

InGameNotificationOverlay::InGameNotificationOverlay(QWidget* parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                          Qt::WindowDoesNotAcceptFocus) {
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::NoFocus);

    setFixedSize(540, 76);

    m_fade_anim = new QPropertyAnimation(this, "panelOpacity", this);
    m_fade_anim->setDuration(220);

    m_hide_timer.setSingleShot(true);
    connect(&m_hide_timer, &QTimer::timeout, this, [this]() {
        m_fade_anim->stop();
        m_fade_anim->disconnect(this);
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
                                                 int displayDurationMs) {
    m_title = title;
    m_message = message;
    m_accent_color = accent;
    m_icon = iconText;

    RepositionToParent();

    m_hide_timer.stop();
    m_fade_anim->stop();
    m_fade_anim->disconnect(this);

    if (m_panel_opacity < 0.15) {
        m_panel_opacity = 0.15;
    }

    show();
    raise();

    m_fade_anim->setStartValue(m_panel_opacity);
    m_fade_anim->setEndValue(1.0);
    m_fade_anim->start();

    update();
    m_hide_timer.start(displayDurationMs);
}

void InGameNotificationOverlay::paintEvent(QPaintEvent* /*event*/) {
    if (m_panel_opacity <= 0.01) {
        return;
    }

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setOpacity(m_panel_opacity);

    const QRect r = rect().adjusted(3, 3, -3, -3);

    // Outer pill shape
    QPainterPath bgPath;
    bgPath.addRoundedRect(r, 14, 14);

    // Dark Aurora cyber glassmorphism gradient
    QLinearGradient bgGrad(0, r.top(), 0, r.bottom());
    bgGrad.setColorAt(0.0, QColor(13, 21, 38, 244));
    bgGrad.setColorAt(1.0, QColor(7, 11, 22, 250));
    p.fillPath(bgPath, bgGrad);

    // Subtle radial/horizontal accent wash on the left
    QLinearGradient washGrad(r.left(), 0, r.left() + 180, 0);
    QColor washColor = m_accent_color;
    washColor.setAlpha(38);
    washGrad.setColorAt(0.0, washColor);
    washGrad.setColorAt(1.0, QColor(0, 0, 0, 0));
    p.fillPath(bgPath, washGrad);

    // Neon accent border
    QPen borderPen(m_accent_color, 1.8);
    p.setPen(borderPen);
    p.setBrush(Qt::NoBrush);
    p.drawPath(bgPath);

    // Glowing top bar accent
    QPainterPath topBar;
    topBar.addRoundedRect(QRect(r.x() + 28, r.y(), r.width() - 56, 3), 1.5, 1.5);
    p.fillPath(topBar, m_accent_color);

    // Left Icon badge
    const QRect iconRect(r.x() + 14, r.y() + (r.height() - 42) / 2, 42, 42);
    QPainterPath iconBadge;
    iconBadge.addRoundedRect(iconRect, 11, 11);
    QColor badgeBg = m_accent_color;
    badgeBg.setAlpha(52);
    p.fillPath(iconBadge, badgeBg);
    p.setPen(QPen(m_accent_color, 1.4));
    p.drawPath(iconBadge);

    // Draw icon symbol
    p.setPen(QColor(255, 255, 255));
    QFont iconFont(QStringLiteral("Segoe UI Emoji"), 16, QFont::Bold);
    p.setFont(iconFont);
    p.drawText(iconRect, Qt::AlignCenter, m_icon);

    // Title
    const QRect titleRect(iconRect.right() + 14, r.y() + 10, r.width() - 84, 24);
    p.setPen(m_accent_color.lighter(135));
    QFont titleFont(QStringLiteral("Segoe UI"), 10, QFont::ExtraBold);
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.6);
    p.setFont(titleFont);
    const QString elided_title =
        QFontMetrics(titleFont).elidedText(m_title, Qt::ElideRight, titleRect.width());
    p.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, elided_title);

    // Subtitle Message
    const QRect msgRect(iconRect.right() + 14, r.y() + 35, r.width() - 84, 24);
    p.setPen(QColor(226, 232, 240));
    QFont msgFont(QStringLiteral("Segoe UI"), 9, QFont::DemiBold);
    p.setFont(msgFont);
    const QString elided_msg =
        QFontMetrics(msgFont).elidedText(m_message, Qt::ElideRight, msgRect.width());
    p.drawText(msgRect, Qt::AlignLeft | Qt::AlignVCenter, elided_msg);
}

// SPDX-FileCopyrightText: 2026 STORM SOFT
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cmath>
#include <QColor>
#include <QIcon>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QRadialGradient>

namespace StormVectorIcons {

inline QIcon CreateSearchIcon(int w = 18, int h = 18, const QColor& accent = QColor(0, 210, 255)) {
    const int scale = 2;
    QPixmap pix(w * scale, h * scale);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);

        const qreal sw = w * scale;
        const qreal sh = h * scale;

        // Lens circle
        const qreal cx = sw * 0.40;
        const qreal cy = sh * 0.40;
        const qreal r = sw * 0.28;

        QLinearGradient grad(cx - r, cy - r, cx + r, cy + r);
        grad.setColorAt(0.0, accent);
        grad.setColorAt(1.0, accent.lighter(130));

        QPen pen(grad, sw * 0.12);
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen);
        p.setBrush(QColor(accent.red(), accent.green(), accent.blue(), 25));
        p.drawEllipse(QPointF(cx, cy), r, r);

        // Specular highlight on lens
        QPainterPath highlight;
        highlight.arcMoveTo(QRectF(cx - r * 0.65, cy - r * 0.65, r * 1.3, r * 1.3), 60);
        highlight.arcTo(QRectF(cx - r * 0.65, cy - r * 0.65, r * 1.3, r * 1.3), 60, 60);
        p.setPen(QPen(QColor(255, 255, 255, 180), sw * 0.06, Qt::SolidLine, Qt::RoundCap));
        p.drawPath(highlight);

        // Handle
        const qreal hx1 = cx + r * 0.707;
        const qreal hy1 = cy + r * 0.707;
        const qreal hx2 = sw * 0.88;
        const qreal hy2 = sh * 0.88;

        QLinearGradient handleGrad(hx1, hy1, hx2, hy2);
        handleGrad.setColorAt(0.0, accent);
        handleGrad.setColorAt(1.0, accent.darker(140));

        QPen handlePen(handleGrad, sw * 0.14, Qt::SolidLine, Qt::RoundCap);
        p.setPen(handlePen);
        p.drawLine(QPointF(hx1, hy1), QPointF(hx2, hy2));
    }
    pix.setDevicePixelRatio(scale);
    return QIcon(pix);
}

inline QIcon CreateClearCrossIcon(int w = 16, int h = 16, const QColor& color = QColor(248, 113, 113)) {
    const int scale = 2;
    QPixmap pix(w * scale, h * scale);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);

        const qreal sw = w * scale;
        const qreal sh = h * scale;
        const qreal pad = sw * 0.24;

        // Background subtle circle
        p.setBrush(QColor(color.red(), color.green(), color.blue(), 35));
        p.setPen(Qt::NoPen);
        p.drawEllipse(QRectF(sw * 0.05, sh * 0.05, sw * 0.90, sh * 0.90));

        // Cross strokes
        QPen pen(color, sw * 0.12, Qt::SolidLine, Qt::RoundCap);
        p.setPen(pen);
        p.drawLine(QPointF(pad, pad), QPointF(sw - pad, sh - pad));
        p.drawLine(QPointF(sw - pad, pad), QPointF(pad, sh - pad));
    }
    pix.setDevicePixelRatio(scale);
    return QIcon(pix);
}

inline QIcon CreateLayoutViewIcon(int w = 18, int h = 18, const QColor& accent = QColor(168, 85, 247)) {
    const int scale = 2;
    QPixmap pix(w * scale, h * scale);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);

        const qreal sw = w * scale;
        const qreal sh = h * scale;

        // Upper deck card
        QLinearGradient g1(0, 0, sw, sh * 0.5);
        g1.setColorAt(0.0, accent.lighter(130));
        g1.setColorAt(1.0, accent);
        p.setPen(QPen(accent.lighter(140), sw * 0.06));
        p.setBrush(g1);
        p.drawRoundedRect(QRectF(sw * 0.10, sh * 0.12, sw * 0.80, sh * 0.32), sw * 0.08, sw * 0.08);

        // Lower split decks
        p.setBrush(QColor(accent.red(), accent.green(), accent.blue(), 180));
        p.drawRoundedRect(QRectF(sw * 0.10, sh * 0.54, sw * 0.48, sh * 0.34), sw * 0.08, sw * 0.08);

        // Right inspector miniature
        QLinearGradient g2(sw * 0.64, sh * 0.54, sw * 0.90, sh * 0.88);
        g2.setColorAt(0.0, QColor(0, 210, 255));
        g2.setColorAt(1.0, accent);
        p.setBrush(g2);
        p.drawRoundedRect(QRectF(sw * 0.64, sh * 0.54, sw * 0.26, sh * 0.34), sw * 0.08, sw * 0.08);
    }
    pix.setDevicePixelRatio(scale);
    return QIcon(pix);
}

inline QIcon CreateSortIcon(int w = 18, int h = 18, const QColor& accent = QColor(139, 92, 246)) {
    const int scale = 2;
    QPixmap pix(w * scale, h * scale);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);

        const qreal sw = w * scale;
        const qreal sh = h * scale;

        // Up arrow
        QPainterPath up;
        up.moveTo(sw * 0.30, sh * 0.20);
        up.lineTo(sw * 0.14, sh * 0.42);
        up.lineTo(sw * 0.46, sh * 0.42);
        up.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(accent.lighter(120));
        p.drawPath(up);
        p.setPen(QPen(accent.lighter(120), sw * 0.08, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(sw * 0.30, sh * 0.35), QPointF(sw * 0.30, sh * 0.78));

        // Down arrow
        QPainterPath down;
        down.moveTo(sw * 0.70, sh * 0.80);
        down.lineTo(sw * 0.54, sh * 0.58);
        down.lineTo(sw * 0.86, sh * 0.58);
        down.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(accent);
        p.drawPath(down);
        p.setPen(QPen(accent, sw * 0.08, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(sw * 0.70, sh * 0.65), QPointF(sw * 0.70, sh * 0.22));
    }
    pix.setDevicePixelRatio(scale);
    return QIcon(pix);
}

inline QIcon CreateGridViewIcon(int w = 18, int h = 18, const QColor& accent = QColor(56, 189, 248)) {
    const int scale = 2;
    QPixmap pix(w * scale, h * scale);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);

        const qreal sw = w * scale;
        const qreal sh = h * scale;
        const qreal sz = sw * 0.34;
        const qreal r = sw * 0.08;

        p.setPen(QPen(accent, sw * 0.05));
        p.setBrush(QColor(accent.red(), accent.green(), accent.blue(), 190));
        p.drawRoundedRect(QRectF(sw * 0.12, sh * 0.12, sz, sz), r, r);
        p.drawRoundedRect(QRectF(sw * 0.54, sh * 0.12, sz, sz), r, r);
        p.drawRoundedRect(QRectF(sw * 0.12, sh * 0.54, sz, sz), r, r);
        p.drawRoundedRect(QRectF(sw * 0.54, sh * 0.54, sz, sz), r, r);
    }
    pix.setDevicePixelRatio(scale);
    return QIcon(pix);
}

inline QIcon CreateTreeViewIcon(int w = 18, int h = 18, const QColor& accent = QColor(56, 189, 248)) {
    const int scale = 2;
    QPixmap pix(w * scale, h * scale);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);

        const qreal sw = w * scale;
        const qreal sh = h * scale;

        // Tree node dots and horizontal list bars
        for (int i = 0; i < 3; ++i) {
            const qreal y = sh * (0.22 + i * 0.28);
            p.setPen(Qt::NoPen);
            p.setBrush(accent.lighter(130));
            p.drawEllipse(QPointF(sw * 0.20, y), sw * 0.08, sw * 0.08);

            p.setPen(QPen(accent, sw * 0.08, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(QPointF(sw * 0.38, y), QPointF(sw * 0.85, y));
        }
    }
    pix.setDevicePixelRatio(scale);
    return QIcon(pix);
}

inline QIcon CreatePlayIcon(int w = 18, int h = 18, const QColor& accent = QColor(52, 211, 153)) {
    const int scale = 2;
    QPixmap pix(w * scale, h * scale);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);

        const qreal sw = w * scale;
        const qreal sh = h * scale;

        QPainterPath path;
        path.moveTo(sw * 0.26, sh * 0.18);
        path.lineTo(sw * 0.84, sh * 0.50);
        path.lineTo(sw * 0.26, sh * 0.82);
        path.closeSubpath();

        QLinearGradient grad(sw * 0.26, sh * 0.18, sw * 0.84, sh * 0.82);
        grad.setColorAt(0.0, accent.lighter(130));
        grad.setColorAt(1.0, accent);

        p.setPen(QPen(accent.lighter(140), sw * 0.06, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(grad);
        p.drawPath(path);
    }
    pix.setDevicePixelRatio(scale);
    return QIcon(pix);
}

inline QIcon CreateSettingsIcon(int w = 18, int h = 18, const QColor& accent = QColor(148, 163, 184)) {
    const int scale = 2;
    QPixmap pix(w * scale, h * scale);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);

        const qreal sw = w * scale;
        const qreal sh = h * scale;
        const qreal cx = sw * 0.5;
        const qreal cy = sh * 0.5;
        const qreal r_outer = sw * 0.36;
        const qreal r_inner = sw * 0.28;
        const qreal r_hole = sw * 0.14;

        QPainterPath cog;
        constexpr int teeth = 8;
        for (int i = 0; i < teeth; ++i) {
            const double a1 = (i * 2.0 * M_PI) / teeth;
            const double a2 = a1 + (M_PI / teeth) * 0.5;
            const double a3 = a1 + (M_PI / teeth);
            const double a4 = a1 + (M_PI / teeth) * 1.5;

            const QPointF p1(cx + r_inner * std::cos(a1), cy + r_inner * std::sin(a1));
            const QPointF p2(cx + r_outer * std::cos(a2), cy + r_outer * std::sin(a2));
            const QPointF p3(cx + r_outer * std::cos(a3), cy + r_outer * std::sin(a3));
            const QPointF p4(cx + r_inner * std::cos(a4), cy + r_inner * std::sin(a4));

            if (i == 0) cog.moveTo(p1);
            else cog.lineTo(p1);
            cog.lineTo(p2);
            cog.lineTo(p3);
            cog.lineTo(p4);
        }
        cog.closeSubpath();

        QPainterPath hole;
        hole.addEllipse(QPointF(cx, cy), r_hole, r_hole);
        QPainterPath final_cog = cog.subtracted(hole);

        QLinearGradient grad(0, 0, sw, sh);
        grad.setColorAt(0.0, accent.lighter(130));
        grad.setColorAt(1.0, accent);

        p.setPen(QPen(accent.lighter(140), sw * 0.04));
        p.setBrush(grad);
        p.drawPath(final_cog);
    }
    pix.setDevicePixelRatio(scale);
    return QIcon(pix);
}

inline QIcon CreateCheatsIcon(int w = 18, int h = 18, const QColor& accent = QColor(245, 158, 11)) {
    const int scale = 2;
    QPixmap pix(w * scale, h * scale);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);

        const qreal sw = w * scale;
        const qreal sh = h * scale;

        QPainterPath bolt;
        bolt.moveTo(sw * 0.56, sh * 0.10);
        bolt.lineTo(sw * 0.24, sh * 0.52);
        bolt.lineTo(sw * 0.52, sh * 0.52);
        bolt.lineTo(sw * 0.44, sh * 0.90);
        bolt.lineTo(sw * 0.76, sh * 0.44);
        bolt.lineTo(sw * 0.48, sh * 0.44);
        bolt.closeSubpath();

        QLinearGradient grad(sw * 0.5, sh * 0.10, sw * 0.5, sh * 0.90);
        grad.setColorAt(0.0, QColor(254, 240, 138));
        grad.setColorAt(1.0, accent);

        p.setPen(QPen(accent.lighter(130), sw * 0.05, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(grad);
        p.drawPath(bolt);
    }
    pix.setDevicePixelRatio(scale);
    return QIcon(pix);
}

inline QIcon CreateSaveFolderIcon(int w = 18, int h = 18, const QColor& accent = QColor(59, 130, 246)) {
    const int scale = 2;
    QPixmap pix(w * scale, h * scale);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);

        const qreal sw = w * scale;
        const qreal sh = h * scale;

        // Folder back tab
        QPainterPath tab;
        tab.moveTo(sw * 0.12, sh * 0.28);
        tab.lineTo(sw * 0.42, sh * 0.28);
        tab.lineTo(sw * 0.52, sh * 0.38);
        tab.lineTo(sw * 0.88, sh * 0.38);
        tab.lineTo(sw * 0.88, sh * 0.82);
        tab.lineTo(sw * 0.12, sh * 0.82);
        tab.closeSubpath();
        p.setPen(QPen(accent.darker(120), sw * 0.05));
        p.setBrush(accent.darker(130));
        p.drawPath(tab);

        // Folder front body
        QPainterPath body;
        body.moveTo(sw * 0.12, sh * 0.44);
        body.lineTo(sw * 0.88, sh * 0.44);
        body.lineTo(sw * 0.80, sh * 0.84);
        body.lineTo(sw * 0.12, sh * 0.84);
        body.closeSubpath();

        QLinearGradient grad(sw * 0.12, sh * 0.44, sw * 0.88, sh * 0.84);
        grad.setColorAt(0.0, accent.lighter(130));
        grad.setColorAt(1.0, accent);

        p.setPen(QPen(accent.lighter(140), sw * 0.05));
        p.setBrush(grad);
        p.drawPath(body);
    }
    pix.setDevicePixelRatio(scale);
    return QIcon(pix);
}

// OSD Quick Action Icons
inline QIcon CreateRewindIcon(int w = 16, int h = 16, const QColor& accent = QColor(56, 189, 248)) {
    const int scale = 2;
    QPixmap pix(w * scale, h * scale);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);
        const qreal sw = w * scale;
        const qreal sh = h * scale;

        auto draw_tri = [&](qreal x1, qreal x2) {
            QPainterPath tri;
            tri.moveTo(x1, sh * 0.50);
            tri.lineTo(x2, sh * 0.20);
            tri.lineTo(x2, sh * 0.80);
            tri.closeSubpath();
            p.setPen(QPen(accent.lighter(130), sw * 0.04));
            p.setBrush(accent);
            p.drawPath(tri);
        };
        draw_tri(sw * 0.10, sw * 0.48);
        draw_tri(sw * 0.48, sw * 0.86);
    }
    pix.setDevicePixelRatio(scale);
    return QIcon(pix);
}

inline QIcon CreateFastForwardIcon(int w = 16, int h = 16, const QColor& accent = QColor(245, 158, 11)) {
    const int scale = 2;
    QPixmap pix(w * scale, h * scale);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);
        const qreal sw = w * scale;
        const qreal sh = h * scale;

        auto draw_tri = [&](qreal x1, qreal x2) {
            QPainterPath tri;
            tri.moveTo(x2, sh * 0.50);
            tri.lineTo(x1, sh * 0.20);
            tri.lineTo(x1, sh * 0.80);
            tri.closeSubpath();
            p.setPen(QPen(accent.lighter(130), sw * 0.04));
            p.setBrush(accent);
            p.drawPath(tri);
        };
        draw_tri(sw * 0.14, sw * 0.52);
        draw_tri(sw * 0.52, sw * 0.90);
    }
    pix.setDevicePixelRatio(scale);
    return QIcon(pix);
}

inline QIcon CreateFrameStepIcon(int w = 16, int h = 16, const QColor& accent = QColor(168, 85, 247)) {
    const int scale = 2;
    QPixmap pix(w * scale, h * scale);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);
        const qreal sw = w * scale;
        const qreal sh = h * scale;

        QPainterPath tri;
        tri.moveTo(sw * 0.60, sh * 0.50);
        tri.lineTo(sw * 0.20, sh * 0.20);
        tri.lineTo(sw * 0.20, sh * 0.80);
        tri.closeSubpath();
        p.setPen(QPen(accent.lighter(130), sw * 0.04));
        p.setBrush(accent);
        p.drawPath(tri);

        p.setPen(QPen(accent.lighter(140), sw * 0.12, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(sw * 0.78, sh * 0.20), QPointF(sw * 0.78, sh * 0.80));
    }
    pix.setDevicePixelRatio(scale);
    return QIcon(pix);
}

inline QIcon CreateQuickSaveIcon(int w = 16, int h = 16, const QColor& accent = QColor(52, 211, 153)) {
    const int scale = 2;
    QPixmap pix(w * scale, h * scale);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);
        const qreal sw = w * scale;
        const qreal sh = h * scale;

        // Tray bracket
        p.setPen(QPen(accent, sw * 0.10, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(sw * 0.15, sh * 0.60), QPointF(sw * 0.15, sh * 0.85));
        p.drawLine(QPointF(sw * 0.15, sh * 0.85), QPointF(sw * 0.85, sh * 0.85));
        p.drawLine(QPointF(sw * 0.85, sh * 0.85), QPointF(sw * 0.85, sh * 0.60));

        // Downward arrow
        p.drawLine(QPointF(sw * 0.50, sh * 0.15), QPointF(sw * 0.50, sh * 0.62));
        QPainterPath arrow;
        arrow.moveTo(sw * 0.50, sh * 0.66);
        arrow.lineTo(sw * 0.30, sh * 0.44);
        arrow.lineTo(sw * 0.70, sh * 0.44);
        arrow.closeSubpath();
        p.setBrush(accent.lighter(130));
        p.setPen(Qt::NoPen);
        p.drawPath(arrow);
    }
    pix.setDevicePixelRatio(scale);
    return QIcon(pix);
}

inline QIcon CreateQuickLoadIcon(int w = 16, int h = 16, const QColor& accent = QColor(0, 229, 255)) {
    const int scale = 2;
    QPixmap pix(w * scale, h * scale);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);
        const qreal sw = w * scale;
        const qreal sh = h * scale;

        // Tray bracket
        p.setPen(QPen(accent, sw * 0.10, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(sw * 0.15, sh * 0.60), QPointF(sw * 0.15, sh * 0.85));
        p.drawLine(QPointF(sw * 0.15, sh * 0.85), QPointF(sw * 0.85, sh * 0.85));
        p.drawLine(QPointF(sw * 0.85, sh * 0.85), QPointF(sw * 0.85, sh * 0.60));

        // Upward arrow
        p.drawLine(QPointF(sw * 0.50, sh * 0.65), QPointF(sw * 0.50, sh * 0.22));
        QPainterPath arrow;
        arrow.moveTo(sw * 0.50, sh * 0.16);
        arrow.lineTo(sw * 0.30, sh * 0.38);
        arrow.lineTo(sw * 0.70, sh * 0.38);
        arrow.closeSubpath();
        p.setBrush(accent.lighter(130));
        p.setPen(Qt::NoPen);
        p.drawPath(arrow);
    }
    pix.setDevicePixelRatio(scale);
    return QIcon(pix);
}

inline QIcon CreateAiTranslateIcon(int w = 16, int h = 16, const QColor& accent = QColor(236, 72, 153)) {
    const int scale = 2;
    QPixmap pix(w * scale, h * scale);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);
        const qreal sw = w * scale;
        const qreal sh = h * scale;

        // Rounded speech bubble
        p.setPen(QPen(accent, sw * 0.08));
        p.setBrush(QColor(accent.red(), accent.green(), accent.blue(), 30));
        p.drawRoundedRect(QRectF(sw * 0.12, sh * 0.15, sw * 0.76, sh * 0.62), sw * 0.16, sw * 0.16);

        // 'A' letter in the bubble
        QFont f = p.font();
        f.setBold(true);
        f.setPixelSize(static_cast<int>(sw * 0.44));
        p.setFont(f);
        p.setPen(accent.lighter(140));
        p.drawText(QRectF(sw * 0.12, sh * 0.15, sw * 0.76, sh * 0.62), Qt::AlignCenter, QStringLiteral("A"));
    }
    pix.setDevicePixelRatio(scale);
    return QIcon(pix);
}

inline QIcon CreateShaderFixIcon(int w = 16, int h = 16, const QColor& accent = QColor(129, 140, 248)) {
    const int scale = 2;
    QPixmap pix(w * scale, h * scale);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);
        const qreal sw = w * scale;
        const qreal sh = h * scale;

        // Healing / Magic Sparkle 4-point star
        QPainterPath star;
        const qreal cx = sw * 0.50;
        const qreal cy = sh * 0.50;
        const qreal r1 = sw * 0.42;
        const qreal r2 = sw * 0.12;
        for (int i = 0; i < 4; ++i) {
            const double a1 = i * M_PI * 0.5;
            const double a2 = a1 + M_PI * 0.25;
            if (i == 0) star.moveTo(cx + r1 * std::cos(a1), cy + r1 * std::sin(a1));
            else star.lineTo(cx + r1 * std::cos(a1), cy + r1 * std::sin(a1));
            star.lineTo(cx + r2 * std::cos(a2), cy + r2 * std::sin(a2));
        }
        star.closeSubpath();

        QLinearGradient grad(0, 0, sw, sh);
        grad.setColorAt(0.0, QColor(255, 255, 255));
        grad.setColorAt(1.0, accent);

        p.setPen(QPen(accent.lighter(140), sw * 0.04));
        p.setBrush(grad);
        p.drawPath(star);
    }
    pix.setDevicePixelRatio(scale);
    return QIcon(pix);
}

inline QIcon CreateTurboIcon(int w = 16, int h = 16, const QColor& accent = QColor(239, 68, 68)) {
    const int scale = 2;
    QPixmap pix(w * scale, h * scale);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);
        const qreal sw = w * scale;
        const qreal sh = h * scale;

        // Gauge arc
        p.setPen(QPen(QColor(accent.red(), accent.green(), accent.blue(), 80), sw * 0.10, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(QRectF(sw * 0.15, sh * 0.15, sw * 0.70, sh * 0.70), 0, 180 * 16);

        // Active turbo arc (red/amber)
        p.setPen(QPen(accent, sw * 0.12, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(QRectF(sw * 0.15, sh * 0.15, sw * 0.70, sh * 0.70), 45 * 16, 90 * 16);

        // Needle pointing to max
        p.setPen(QPen(QColor(255, 255, 255), sw * 0.08, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(sw * 0.50, sh * 0.55), QPointF(sw * 0.72, sh * 0.32));
    }
    pix.setDevicePixelRatio(scale);
    return QIcon(pix);
}

inline QIcon CreatePauseCoolingIcon(int w = 16, int h = 16, const QColor& accent = QColor(56, 189, 248)) {
    const int scale = 2;
    QPixmap pix(w * scale, h * scale);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);
        const qreal sw = w * scale;
        const qreal sh = h * scale;

        // Dual cooling bars
        QLinearGradient g(0, 0, 0, sh);
        g.setColorAt(0.0, QColor(186, 230, 253));
        g.setColorAt(1.0, accent);

        p.setPen(QPen(accent.lighter(130), sw * 0.04));
        p.setBrush(g);
        p.drawRoundedRect(QRectF(sw * 0.22, sh * 0.18, sw * 0.20, sh * 0.64), sw * 0.06, sw * 0.06);
        p.drawRoundedRect(QRectF(sw * 0.58, sh * 0.18, sw * 0.20, sh * 0.64), sw * 0.06, sw * 0.06);
    }
    pix.setDevicePixelRatio(scale);
    return QIcon(pix);
}

} // namespace StormVectorIcons

namespace StormVectorIcon {

inline QIcon LayoutSwitcher(const QColor& accent = QColor(168, 85, 247), int size = 18) {
    return StormVectorIcons::CreateLayoutViewIcon(size, size, accent);
}
inline QIcon LayoutSwitcher(const QString& hex, int size = 18) {
    return StormVectorIcons::CreateLayoutViewIcon(size, size, QColor(hex));
}

inline QIcon GridView(const QColor& accent = QColor(56, 189, 248), int size = 18) {
    return StormVectorIcons::CreateGridViewIcon(size, size, accent);
}
inline QIcon GridView(const QString& hex, int size = 18) {
    return StormVectorIcons::CreateGridViewIcon(size, size, QColor(hex));
}

inline QIcon TreeView(const QColor& accent = QColor(56, 189, 248), int size = 18) {
    return StormVectorIcons::CreateTreeViewIcon(size, size, accent);
}
inline QIcon TreeView(const QString& hex, int size = 18) {
    return StormVectorIcons::CreateTreeViewIcon(size, size, QColor(hex));
}

inline QIcon Sort(const QColor& accent = QColor(139, 92, 246), int size = 18) {
    return StormVectorIcons::CreateSortIcon(size, size, accent);
}
inline QIcon Sort(const QString& hex, int size = 18) {
    return StormVectorIcons::CreateSortIcon(size, size, QColor(hex));
}

inline QIcon Search(const QColor& accent = QColor(0, 210, 255), int size = 18) {
    return StormVectorIcons::CreateSearchIcon(size, size, accent);
}
inline QIcon Search(const QString& hex, int size = 18) {
    return StormVectorIcons::CreateSearchIcon(size, size, QColor(hex));
}

inline QIcon Settings(const QColor& accent = QColor(148, 163, 184), int size = 18) {
    return StormVectorIcons::CreateSettingsIcon(size, size, accent);
}
inline QIcon Settings(const QString& hex, int size = 18) {
    return StormVectorIcons::CreateSettingsIcon(size, size, QColor(hex));
}

inline QIcon Play(const QColor& accent = QColor(52, 211, 153), int size = 18) {
    return StormVectorIcons::CreatePlayIcon(size, size, accent);
}
inline QIcon Play(const QString& hex, int size = 18) {
    return StormVectorIcons::CreatePlayIcon(size, size, QColor(hex));
}

inline QIcon Cheats(const QColor& accent = QColor(245, 158, 11), int size = 18) {
    return StormVectorIcons::CreateCheatsIcon(size, size, accent);
}
inline QIcon Cheats(const QString& hex, int size = 18) {
    return StormVectorIcons::CreateCheatsIcon(size, size, QColor(hex));
}

inline QIcon SaveFolder(const QColor& accent = QColor(59, 130, 246), int size = 18) {
    return StormVectorIcons::CreateSaveFolderIcon(size, size, accent);
}
inline QIcon SaveFolder(const QString& hex, int size = 18) {
    return StormVectorIcons::CreateSaveFolderIcon(size, size, QColor(hex));
}

} // namespace StormVectorIcon

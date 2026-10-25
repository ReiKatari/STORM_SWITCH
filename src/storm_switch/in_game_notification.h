// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QPropertyAnimation>
#include <QTimer>
#include <QWidget>

class InGameNotificationOverlay : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal panelOpacity READ panelOpacity WRITE setPanelOpacity)

public:
    explicit InGameNotificationOverlay(QWidget* parent = nullptr);
    ~InGameNotificationOverlay() override = default;

    void ShowNotification(const QString& title, const QString& message,
                          const QColor& accent = QColor(0, 229, 255),
                          const QString& iconText = QStringLiteral("?"),
                          int displayDurationMs = 3000);
    void RepositionToParent();

    qreal panelOpacity() const {
        return m_panel_opacity;
    }
    void setPanelOpacity(qreal value) {
        m_panel_opacity = value;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_title;
    QString m_message;
    QString m_icon;
    QColor m_accent_color{0, 229, 255};
    qreal m_panel_opacity{0.0};
    QTimer m_hide_timer;
    QPropertyAnimation* m_fade_anim{nullptr};
};

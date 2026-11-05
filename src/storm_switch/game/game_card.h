// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QStyledItemDelegate>

/**
 * A stylized "card"-like delegate for the game grid view.
 * Zero-computation in paint(): all badges and scaled covers come directly from model roles.
 */
class GameCard : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit GameCard(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    QRect getCardRect(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    bool hitTest(const QPoint& point, const QModelIndex& index,
                 const QWidget* widget, const QRect& cellRect) const;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setSize(const QSize& newSize, const QSize& contentSize, const int padding, const int columns);

private:
    void DrawVersionBadge(QPainter* painter, const QRect& iconRect, const QString& version) const;
    void DrawGearBadge(QPainter* painter, const QRect& iconRect) const;
    void DrawUpdateBadge(QPainter* painter, const QRect& iconRect) const;
    void DrawDlcBadge(QPainter* painter, const QRect& iconRect, const QString& text) const;
    void DrawDlcBadge(QPainter* painter, const QRect& iconRect, int dlc_count) const;
    void DrawSwitch2Badge(QPainter* painter, const QRect& iconRect) const;

    static constexpr int cardMargin = 8;

    QSize m_size;
    QSize m_contentSize;
    int m_padding = 0;
    int m_columns = 1;
};

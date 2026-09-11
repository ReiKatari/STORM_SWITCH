// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/perf_stats.h"
#include "performance_overlay.h"
#include "ui_performance_overlay.h"

#include "main_window.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

class FpsGraphWidget : public QWidget {
public:
    explicit FpsGraphWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setMinimumHeight(100);
        setAttribute(Qt::WA_OpaquePaintEvent, false);
    }

    void setSamples(const std::deque<double>& samples, double max_fps) {
        m_samples = samples;
        m_max_fps = std::max(30.0, max_fps);
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        const int w = width();
        const int h = height();

        // Dark background matching STORM theme
        painter.fillRect(rect(), QColor(10, 12, 16, 220));

        // Grid lines
        painter.setPen(QPen(QColor(45, 55, 72, 180), 1, Qt::DashLine));
        painter.drawLine(0, h / 2, w, h / 2);
        painter.drawLine(0, h / 4, w, h / 4);
        painter.drawLine(0, 3 * h / 4, w, 3 * h / 4);

        if (m_samples.size() < 2) {
            return;
        }

        // Compute polyline points
        QVector<QPointF> points;
        points.reserve(static_cast<int>(m_samples.size()));

        const double step = static_cast<double>(w) / static_cast<double>(NUM_FPS_SAMPLES_GRAPH);
        const double startX = w - static_cast<double>(m_samples.size() - 1) * step;

        for (size_t i = 0; i < m_samples.size(); ++i) {
            const double x = startX + static_cast<double>(i) * step;
            const double norm = std::clamp(m_samples[i] / m_max_fps, 0.0, 1.0);
            const double y = static_cast<double>(h) - (norm * static_cast<double>(h - 8)) - 4.0;
            points.append(QPointF(x, y));
        }

        // Smooth gradient fill under the FPS line
        if (!points.isEmpty()) {
            QPainterPath fillPath;
            fillPath.moveTo(points.first().x(), static_cast<double>(h));
            for (const auto& pt : points) {
                fillPath.lineTo(pt);
            }
            fillPath.lineTo(points.last().x(), static_cast<double>(h));
            fillPath.closeSubpath();

            QLinearGradient fillGrad(0, 0, 0, h);
            fillGrad.setColorAt(0.0, QColor(0, 210, 255, 80));
            fillGrad.setColorAt(1.0, QColor(0, 210, 255, 5));
            painter.fillPath(fillPath, fillGrad);
        }

        // Draw neon Cyan polyline
        QPen linePen(QColor(0, 210, 255), 2);
        painter.setPen(linePen);
        painter.drawPolyline(points.data(), points.size());
    }

private:
    static constexpr size_t NUM_FPS_SAMPLES_GRAPH = 120;
    std::deque<double> m_samples;
    double m_max_fps = 60.0;
};

PerformanceOverlay::PerformanceOverlay(MainWindow* parent)
    : QWidget(parent), m_mainWindow{parent}, ui(new Ui::PerformanceOverlay) {
    ui->setupUi(this);

    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    raise();

    m_fpsGraph = new FpsGraphWidget(this);
    ui->verticalLayout->addWidget(m_fpsGraph, 1);

    QFont font = ui->fps->font();
    font.setWeight(QFont::DemiBold);

    ui->fps->setFont(font);
    ui->frametime->setFont(font);

    // pos/stats
    resetPosition(m_mainWindow->pos());
    connect(parent, &MainWindow::positionChanged, this, &PerformanceOverlay::resetPosition);
    connect(m_mainWindow, &MainWindow::statsUpdated, this, &PerformanceOverlay::updateStats);
}

PerformanceOverlay::~PerformanceOverlay() {
    delete ui;
}

void PerformanceOverlay::resetPosition(const QPoint& _) {
    auto pos = m_mainWindow->pos();
    move(pos.x() + m_offset.x(), pos.y() + m_offset.y());
}

void PerformanceOverlay::updateStats(const Core::PerfStatsResults& results,
                                     const VideoCore::ShaderNotify& shaders) {
    auto fps = results.average_game_fps;
    if (!std::isnan(fps)) {
        static constexpr double FPS_SAMPLE_THRESHOLD = 3.0;

        QString fpsText = tr("%1 fps").arg(std::round(fps), 0, 'f', 0);
        ui->fps->setText(fpsText);

        if (fps > FPS_SAMPLE_THRESHOLD) {
            m_fpsSamples.push_back(fps);
        }

        if (m_fpsSamples.size() > NUM_FPS_SAMPLES) {
            m_fpsSamples.pop_front();
        }

        if (m_fpsSamples.size() >= 2) {
            const int back_search = static_cast<int>(std::min(size_t(10), m_fpsSamples.size() - 1));
            double sum = std::accumulate(m_fpsSamples.end() - back_search, m_fpsSamples.end(), 0.0);
            double avg = sum / back_search;

            ui->fps_avg->setText(tr("Avg: %1").arg(avg, 0, 'f', 0));
        }

        if (!m_fpsSamples.empty()) {
            auto [min_it, max_it] = std::minmax_element(m_fpsSamples.begin(), m_fpsSamples.end());
            double min_fps = *min_it;
            double max_fps = *max_it;

            ui->fps_min->setText(tr("Min: %1").arg(min_fps, 0, 'f', 0));
            ui->fps_max->setText(tr("Max: %1").arg(max_fps, 0, 'f', 0));

            m_fpsGraph->setSamples(m_fpsSamples, max_fps);
        }
    }

    auto ft = results.frametime;
    if (!std::isnan(ft)) {
        static constexpr double FT_SAMPLE_THRESHOLD = 500.0;

        double ft_ms = results.frametime * 1000.0;
        ui->frametime->setText(tr("%1 ms").arg(ft_ms, 0, 'f', 2));

        if (ft_ms <= FT_SAMPLE_THRESHOLD)
            m_frametimeSamples.push_back(ft_ms);

        if (m_frametimeSamples.size() > NUM_FRAMETIME_SAMPLES)
            m_frametimeSamples.pop_front();

        if (!m_frametimeSamples.empty()) {
            auto [min_it, max_it] =
                std::minmax_element(m_frametimeSamples.begin(), m_frametimeSamples.end());
            ui->ft_min->setText(tr("Min: %1").arg(*min_it, 0, 'f', 1));
            ui->ft_max->setText(tr("Max: %1").arg(*max_it, 0, 'f', 1));
        }

        if (m_frametimeSamples.size() >= 2) {
            const int back_search = static_cast<int>(std::min(size_t(10), m_frametimeSamples.size() - 1));
            double sum = std::accumulate(m_frametimeSamples.end() - back_search,
                                         m_frametimeSamples.end(), 0.0);
            double avg = sum / back_search;

            ui->ft_avg->setText(tr("Avg: %1").arg(avg, 0, 'f', 1));
        }
    }
}

void PerformanceOverlay::paintEvent(QPaintEvent* event) {
    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing);

    painter.setBrush(m_background);
    painter.setPen(Qt::NoPen);

    painter.drawRoundedRect(rect(), 10.0, 10.0);
}

void PerformanceOverlay::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_drag_start_pos = event->pos();
    }
}

void PerformanceOverlay::mouseMoveEvent(QMouseEvent* event) {
    // drag
    if (event->buttons() & Qt::LeftButton) {
        QPoint new_global_pos = event->globalPosition().toPoint() - m_drag_start_pos;
        m_offset = new_global_pos - m_mainWindow->pos();
        move(new_global_pos);
    }
}

void PerformanceOverlay::closeEvent(QCloseEvent* event) {
    emit closed();
}

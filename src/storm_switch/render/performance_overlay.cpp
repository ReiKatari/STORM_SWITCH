// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <cmath>
#include <numeric>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTabWidget>
#include <QGroupBox>
#include <QKeyEvent>
#include <QSlider>
#include <QTextStream>
#include "storm_switch/common/storm_lang.h"
#include "storm_switch/common/storm_theme.h"
#include "storm_switch/common/storm_design_engine.h"
#include <QVBoxLayout>

#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/settings.h"
#include "core/arm/dynarmic/storm_tier2_compiler.h"
#include "core/core.h"
#include "core/dynamic_performance_scaler.h"
#include "core/memory/cheat_engine.h"
#include "core/perf_stats.h"
#include "core/save_state.h"
#include "input_common/drivers/virtual_amiibo.h"
#include "input_common/main.h"
#include "qt_common/abstract/frontend.h"
#include "storm_switch/main_window.h"
#include "storm_switch/render/performance_overlay.h"
#include "video_core/gpu.h"
#include "video_core/shader_notify.h"
#include <QIcon>
#include <QPixmap>

namespace {
QIcon MakeVectorCloseIcon(int size = 16, const QColor& color = QColor(148, 163, 184)) {
    QPixmap px(size, size);
    px.fill(Qt::transparent);
    QPainter p(&px);
    p.setRenderHint(QPainter::Antialiasing);
    QPen pen(color, 1.8, Qt::SolidLine, Qt::RoundCap);
    p.setPen(pen);
    const qreal m = size * 0.25;
    const qreal s = size - m;
    p.drawLine(QPointF(m, m), QPointF(s, s));
    p.drawLine(QPointF(s, m), QPointF(m, s));
    return QIcon(px);
}
} // namespace

// Custom lightweight FPS / Frametime Graph Widget
class FpsGraphWidget : public QWidget {
public:
    explicit FpsGraphWidget(QWidget* parent = nullptr, int min_height = 80)
        : QWidget(parent) {
        setMinimumHeight(min_height);
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

        // Dark volumetric cyber background
        painter.fillRect(rect(), QColor(10, 14, 22, 220));

        // Grid lines
        painter.setPen(QPen(QColor(30, 41, 59, 180), 1, Qt::DashLine));
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

        // Gradient under FPS polyline
        if (!points.isEmpty()) {
            QPainterPath fillPath;
            fillPath.moveTo(points.first().x(), static_cast<double>(h));
            for (const auto& pt : points) {
                fillPath.lineTo(pt);
            }
            fillPath.lineTo(points.last().x(), static_cast<double>(h));
            fillPath.closeSubpath();

            QLinearGradient fillGrad(0, 0, 0, h);
            fillGrad.setColorAt(0.0, QColor(0, 210, 255, 90));
            fillGrad.setColorAt(1.0, QColor(0, 210, 255, 5));
            painter.fillPath(fillPath, fillGrad);
        }

        // Draw neon Cyan line
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
    : QWidget(parent), m_mainWindow{parent} {
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    raise();

    SetupUI();

    resetPosition(m_mainWindow->pos());
    connect(parent, &MainWindow::positionChanged, this, &PerformanceOverlay::resetPosition);
    connect(m_mainWindow, &MainWindow::statsUpdated, this, &PerformanceOverlay::updateStats);
}

PerformanceOverlay::~PerformanceOverlay() = default;

void PerformanceOverlay::SetupUI() {
    const auto po_pal = StormTheme::GetThemePalette();
    auto* root_layout = new QVBoxLayout(this);
    root_layout->setContentsMargins(0, 0, 0, 0);
    root_layout->setSpacing(0);

    // Global OSD Stylesheet matching dynamic STORM palette
    setStyleSheet(QStringLiteral(
        "QWidget { color: %1; font-size: 12px; }"
        "QTabWidget::pane { border: 1px solid %2; background: %3; border-radius: 8px; }"
        "QTabBar::tab { background: %4; color: %1; padding: 7px 14px; border: 1px solid %2; "
        "border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px; font-weight: bold; }"
        "QTabBar::tab:selected { background: %2; color: %5; border-bottom: 2px solid %5; }"
        "QPushButton { background: %6; border: 1px solid %2; border-radius: 6px; color: %1; padding: 6px 12px; font-weight: 500; }"
        "QPushButton:hover { border: 1px solid %5; color: %5; background: %2; }"
        "QPushButton:pressed { background: %5; color: #FFFFFF; }"
        "QPushButton:disabled { background: %4; border: 1px solid %2; color: %7; }"
        "QListWidget { background: %3; border: 1px solid %2; border-radius: 6px; padding: 4px; color: %1; }"
        "QListWidget::item { padding: 5px; border-radius: 4px; margin-bottom: 2px; }"
        "QListWidget::item:hover { background: %2; color: %5; }"
        "QComboBox { background: %4; border: 1px solid %2; border-radius: 6px; padding: 5px 10px; color: %1; }"
        "QComboBox::drop-down { border: none; width: 20px; }"
        "QCheckBox { spacing: 8px; color: %1; }"
        "QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid %2; border-radius: 4px; background: %4; }"
        "QCheckBox::indicator:checked { background: %5; border: 1px solid %5; }"
    ).arg(
        po_pal.textPrimary, po_pal.border, po_pal.card, po_pal.surface, po_pal.accent,
        po_pal.buttonBg, po_pal.textSecondary
    ));

    m_stack = new QStackedWidget(this);

    // ==========================================
    // 1. Compact Mode Widget (Mini HUD)
    // ==========================================
    m_compact_widget = new QWidget(this);
    m_compact_widget->setFixedSize(280, 185);
    auto* compact_layout = new QVBoxLayout(m_compact_widget);
    compact_layout->setContentsMargins(10, 8, 10, 8);
    compact_layout->setSpacing(6);

    auto* compact_header = new QHBoxLayout();
    auto* compact_title = new QLabel(QStringLiteral("STORM OSD"), m_compact_widget);
    compact_title->setStyleSheet(QStringLiteral("color: #00D2FF; font-weight: bold; font-size: 11px;"));
    compact_header->addWidget(compact_title);
    compact_header->addStretch(1);

    auto* btn_storm_sidebar = new QPushButton(StormLang(QStringLiteral("F10"), QStringLiteral("F10")), m_compact_widget);
    btn_storm_sidebar->setCursor(Qt::PointingHandCursor);
    btn_storm_sidebar->setToolTip(StormLang(QStringLiteral("Быстрые настройки (F10)"), QStringLiteral("Quick Settings (F10)")));
    btn_storm_sidebar->setStyleSheet(QStringLiteral(
        "QPushButton { background: rgba(0, 210, 255, 0.2); border: 1px solid #00D2FF; border-radius: 4px; "
        "color: #FFFFFF; font-weight: bold; font-size: 10px; padding: 2px 6px; min-height: 18px; }"
        "QPushButton:hover { background: #00D2FF; color: #000000; }"));
    compact_header->addWidget(btn_storm_sidebar);
    connect(btn_storm_sidebar, &QPushButton::clicked, this, &PerformanceOverlay::ToggleSidebar);

    auto* btn_to_expanded = new QPushButton(StormLang(QStringLiteral("Меню (F6)"), QStringLiteral("Menu (F6)")), m_compact_widget);
    btn_to_expanded->setStyleSheet(QStringLiteral("padding: 2px 8px; font-size: 10px; font-weight: bold; border-color: #00D2FF; color: #00D2FF;"));
    compact_header->addWidget(btn_to_expanded);

    auto* btn_compact_close = new QPushButton(m_compact_widget);
    btn_compact_close->setIcon(MakeVectorCloseIcon(16));
    btn_compact_close->setFixedSize(20, 20);
    btn_compact_close->setStyleSheet(QStringLiteral("padding: 0; font-size: 11px;"));
    compact_header->addWidget(btn_compact_close);
    compact_layout->addLayout(compact_header);

    auto* compact_stats_row = new QHBoxLayout();
    m_compact_fps = new QLabel(tr("0 fps"), m_compact_widget);
    m_compact_fps->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: bold; color: #38BDF8;"));
    compact_stats_row->addWidget(m_compact_fps);

    m_compact_ft = new QLabel(tr("0.0 ms"), m_compact_widget);
    m_compact_ft->setStyleSheet(QStringLiteral("font-size: 13px; font-weight: bold; color: %1;").arg(po_pal.accent));
    compact_stats_row->addWidget(m_compact_ft);

    m_compact_dps = new QLabel(m_compact_widget);
    m_compact_dps->setStyleSheet(QStringLiteral("font-size: 11px; font-weight: bold; color: #34D399;"));
    compact_stats_row->addWidget(m_compact_dps);
    compact_stats_row->addStretch(1);
    compact_layout->addLayout(compact_stats_row);

    m_compact_fpsGraph = new FpsGraphWidget(m_compact_widget, 90);
    compact_layout->addWidget(m_compact_fpsGraph, 1);

    connect(btn_to_expanded, &QPushButton::clicked, this, &PerformanceOverlay::OnToggleExpand);
    connect(btn_compact_close, &QPushButton::clicked, this, &QWidget::close);

    // ==========================================
    // 2. Expanded Mode Widget (Full Tesla OSD Menu)
    // ==========================================
    m_expanded_widget = new QWidget(this);
    m_expanded_widget->setFixedSize(650, 470);
    auto* exp_layout = new QVBoxLayout(m_expanded_widget);
    exp_layout->setContentsMargins(12, 10, 12, 10);
    exp_layout->setSpacing(8);

    // Header bar
    auto* exp_header = new QHBoxLayout();
    auto* exp_title = new QLabel(tr("STORM SWITCH - Tesla OSD"), m_expanded_widget);
    exp_title->setStyleSheet(QStringLiteral("font-size: 14px; font-weight: bold; color: #00D2FF;"));
    exp_header->addWidget(exp_title);

    m_game_title_lbl = new QLabel(m_expanded_widget);
    m_game_title_lbl->setStyleSheet(QStringLiteral("font-size: 11px; color: %1;").arg(po_pal.textPrimary));
    exp_header->addWidget(m_game_title_lbl);
    exp_header->addStretch(1);

    auto* btn_to_compact = new QPushButton(tr("Свернуть в HUD"), m_expanded_widget);
    btn_to_compact->setStyleSheet(QStringLiteral("padding: 3px 10px; font-size: 11px;"));
    exp_header->addWidget(btn_to_compact);

    auto* btn_exp_close = new QPushButton(m_expanded_widget);
    btn_exp_close->setIcon(MakeVectorCloseIcon(18));
    btn_exp_close->setFixedSize(24, 24);
    btn_exp_close->setStyleSheet(QStringLiteral("padding: 0; font-size: 12px; font-weight: bold;"));
    exp_header->addWidget(btn_exp_close);
    exp_layout->addLayout(exp_header);

    connect(btn_to_compact, &QPushButton::clicked, this, &PerformanceOverlay::OnToggleExpand);
    connect(btn_exp_close, &QPushButton::clicked, this, &QWidget::close);

    // Tab widget
    m_tabs = new QTabWidget(m_expanded_widget);

    auto* tab_monitoring = new QWidget();
    SetupMonitoringTab(tab_monitoring);
    m_tabs->addTab(tab_monitoring, tr("Мониторинг"));

    auto* tab_cheats = new QWidget();
    SetupCheatsTab(tab_cheats);
    m_tabs->addTab(tab_cheats, tr("Читы"));

    auto* tab_saves = new QWidget();
    SetupSaveStatesTab(tab_saves);
    m_tabs->addTab(tab_saves, tr("Сохранения"));

    auto* tab_amiibo = new QWidget();
    SetupAmiiboTab(tab_amiibo);
    m_tabs->addTab(tab_amiibo, tr("Amiibo"));

    auto* tab_cfw = new QWidget();
    SetupCfwTab(tab_cfw);
    m_tabs->addTab(tab_cfw, tr("CFW твики"));

    exp_layout->addWidget(m_tabs, 1);

    // Add to stack
    m_stack->addWidget(m_compact_widget);
    m_stack->addWidget(m_expanded_widget);

    SetupSidebarUI();

    m_stack->setCurrentWidget(m_compact_widget);

    root_layout->addWidget(m_stack);
    adjustSize();
}

void PerformanceOverlay::SetupMonitoringTab(QWidget* tab) {
    const auto po_pal = StormTheme::GetThemePalette();
    auto* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    // Row 1: Frametime
    auto* ft_box = new QHBoxLayout();
    auto* ft_title = new QLabel(tr("Frametime:"), tab);
    ft_title->setStyleSheet(QStringLiteral("font-weight: bold; color: %1; font-size: 13px;").arg(po_pal.accent));
    ft_box->addWidget(ft_title);

    m_lbl_ft = new QLabel(tr("0.00 ms"), tab);
    m_lbl_ft->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 14px; color: %1;").arg(po_pal.textPrimary));
    ft_box->addWidget(m_lbl_ft);

    ft_box->addSpacing(15);
    m_lbl_ft_min = new QLabel(tr("Min: 0.0"), tab);
    m_lbl_ft_max = new QLabel(tr("Max: 0.0"), tab);
    m_lbl_ft_avg = new QLabel(tr("Avg: 0.0"), tab);
    m_lbl_ft_min->setStyleSheet(QStringLiteral("color: #FFFFFF;"));
    m_lbl_ft_max->setStyleSheet(QStringLiteral("color: #FFFFFF;"));
    m_lbl_ft_avg->setStyleSheet(QStringLiteral("color: #FFFFFF;"));
    ft_box->addWidget(m_lbl_ft_min);
    ft_box->addWidget(m_lbl_ft_max);
    ft_box->addWidget(m_lbl_ft_avg);
    ft_box->addStretch(1);
    layout->addLayout(ft_box);

    // Row 2: FPS + 1% Low
    auto* fps_box = new QHBoxLayout();
    auto* fps_title = new QLabel(tr("FPS игры:"), tab);
    fps_title->setStyleSheet(QStringLiteral("font-weight: bold; color: #38BDF8; font-size: 13px;"));
    fps_box->addWidget(fps_title);

    m_lbl_fps = new QLabel(tr("0 fps"), tab);
    m_lbl_fps->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 15px; color: #00D2FF;"));
    fps_box->addWidget(m_lbl_fps);

    fps_box->addSpacing(12);
    m_lbl_fps_min = new QLabel(tr("Min: 0"), tab);
    m_lbl_fps_max = new QLabel(tr("Max: 0"), tab);
    m_lbl_fps_avg = new QLabel(tr("Avg: 0"), tab);
    m_lbl_fps_1low = new QLabel(tr("1% Low: 0"), tab);
    m_lbl_fps_min->setStyleSheet(QStringLiteral("color: #FFFFFF;"));
    m_lbl_fps_max->setStyleSheet(QStringLiteral("color: #FFFFFF;"));
    m_lbl_fps_avg->setStyleSheet(QStringLiteral("color: #FFFFFF;"));
    m_lbl_fps_1low->setStyleSheet(QStringLiteral("font-weight: bold; color: #F59E0B;"));
    fps_box->addWidget(m_lbl_fps_min);
    fps_box->addWidget(m_lbl_fps_max);
    fps_box->addWidget(m_lbl_fps_avg);
    fps_box->addWidget(m_lbl_fps_1low);
    fps_box->addStretch(1);
    layout->addLayout(fps_box);

    // Row 3: DPS, Resolution, LSFG & Shaders
    auto* info_box = new QHBoxLayout();
    m_lbl_dps = new QLabel(tr("DPS: Выкл"), tab);
    m_lbl_dps->setStyleSheet(QStringLiteral("font-weight: bold; color: #34D399;"));
    info_box->addWidget(m_lbl_dps);

    m_lbl_res = new QLabel(tr("Масштаб: 1.0x"), tab);
    m_lbl_res->setStyleSheet(QStringLiteral("color: %1;").arg(po_pal.textPrimary));
    info_box->addWidget(m_lbl_res);

    m_lbl_lsfg = new QLabel(tr("LSFG: Выкл"), tab);
    m_lbl_lsfg->setStyleSheet(QStringLiteral("font-weight: bold; color: #38BDF8;"));
    info_box->addWidget(m_lbl_lsfg);

    m_lbl_shaders = new QLabel(tr("Шейдеры: 0"), tab);
    m_lbl_shaders->setStyleSheet(QStringLiteral("color: %1;").arg(po_pal.accent));
    info_box->addWidget(m_lbl_shaders);

    info_box->addStretch(1);
    layout->addLayout(info_box);

    // FPS Graph
    m_fpsGraph = new FpsGraphWidget(tab, 160);
    layout->addWidget(m_fpsGraph, 1);
}

void PerformanceOverlay::SetupCheatsTab(QWidget* tab) {
    auto* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto* top_row = new QHBoxLayout();
    m_cheat_status = new QLabel(tr("Чит-коды для текущей игры:"), tab);
    m_cheat_status->setStyleSheet(QStringLiteral("font-weight: bold; color: #38BDF8;"));
    top_row->addWidget(m_cheat_status);
    top_row->addStretch(1);

    auto* btn_reload = new QPushButton(tr("Перезагрузить читы"), tab);
    btn_reload->setStyleSheet(QStringLiteral("padding: 4px 10px; font-size: 11px;"));
    connect(btn_reload, &QPushButton::clicked, this, &PerformanceOverlay::OnReloadCheats);
    top_row->addWidget(btn_reload);
    layout->addLayout(top_row);

    m_cheat_list = new QListWidget(tab);
    layout->addWidget(m_cheat_list, 1);

    connect(m_cheat_list, &QListWidget::itemChanged, this, [this](QListWidgetItem* item) {
        int row = m_cheat_list->row(item);
        OnCheatToggled(row);
    });
}

void PerformanceOverlay::SetupSaveStatesTab(QWidget* tab) {
    auto* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto* info_lbl = new QLabel(tr("Быстрые сохранения работают мгновенно без выхода из игры (F5 - сохранить, F7 - загрузить):"), tab);
    info_lbl->setStyleSheet(QStringLiteral("color: #FFFFFF; font-size: 11px;"));
    layout->addWidget(info_lbl);

    for (int i = 0; i < 5; ++i) {
        auto* row = new QHBoxLayout();
        row->setSpacing(8);

        m_slots[i].label = new QLabel(tr("Слот %1: [Пусто]").arg(i + 1), tab);
        m_slots[i].label->setStyleSheet(QStringLiteral("font-weight: 500; min-width: 280px;"));
        row->addWidget(m_slots[i].label, 1);

        m_slots[i].btn_save = new QPushButton(tr("Сохранить"), tab);
        m_slots[i].btn_save->setStyleSheet(QStringLiteral("padding: 4px 12px; font-weight: bold;"));
        connect(m_slots[i].btn_save, &QPushButton::clicked, this, [this, i]() { OnSaveSlot(i + 1); });
        row->addWidget(m_slots[i].btn_save);

        m_slots[i].btn_load = new QPushButton(tr("Загрузить"), tab);
        m_slots[i].btn_load->setStyleSheet(QStringLiteral("padding: 4px 12px; font-weight: bold; color: #38BDF8;"));
        connect(m_slots[i].btn_load, &QPushButton::clicked, this, [this, i]() { OnLoadSlot(i + 1); });
        row->addWidget(m_slots[i].btn_load);

        layout->addLayout(row);
    }

    m_save_status = new QLabel(tab);
    m_save_status->setStyleSheet(QStringLiteral("font-weight: bold; color: #34D399; font-size: 12px;"));
    layout->addWidget(m_save_status);
    layout->addStretch(1);
}

void PerformanceOverlay::SetupAmiiboTab(QWidget* tab) {
    auto* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    auto* desc = new QLabel(tr("Инжектор виртуальных фигурок Amiibo:"), tab);
    desc->setStyleSheet(QStringLiteral("font-weight: bold; color: #F59E0B;"));
    layout->addWidget(desc);

    auto* select_row = new QHBoxLayout();
    m_amiibo_combo = new QComboBox(tab);
    m_amiibo_combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    select_row->addWidget(m_amiibo_combo, 1);

    m_btn_browse_amiibo = new QPushButton(tr("Обзор..."), tab);
    connect(m_btn_browse_amiibo, &QPushButton::clicked, this, &PerformanceOverlay::OnBrowseAmiibo);
    select_row->addWidget(m_btn_browse_amiibo);
    layout->addLayout(select_row);

    auto* btn_row = new QHBoxLayout();
    m_btn_inject_amiibo = new QPushButton(tr("Инжектировать Amiibo"), tab);
    m_btn_inject_amiibo->setStyleSheet(QStringLiteral("background: #0284C7; font-weight: bold; padding: 7px 16px;"));
    connect(m_btn_inject_amiibo, &QPushButton::clicked, this, &PerformanceOverlay::OnInjectAmiibo);
    btn_row->addWidget(m_btn_inject_amiibo);

    m_btn_eject_amiibo = new QPushButton(tr("Извлечь Amiibo"), tab);
    m_btn_eject_amiibo->setStyleSheet(QStringLiteral("padding: 7px 16px;"));
    connect(m_btn_eject_amiibo, &QPushButton::clicked, this, &PerformanceOverlay::OnEjectAmiibo);
    btn_row->addWidget(m_btn_eject_amiibo);
    btn_row->addStretch(1);
    layout->addLayout(btn_row);

    m_amiibo_status = new QLabel(tr("Amiibo не подключен"), tab);
    m_amiibo_status->setStyleSheet(QStringLiteral("font-size: 12px; color: #FFFFFF;"));
    layout->addWidget(m_amiibo_status);
    layout->addStretch(1);
}

void PerformanceOverlay::SetupCfwTab(QWidget* tab) {
    auto* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(8);

    // 1. ReverseNX (Docked / Handheld on the fly)
    auto* docked_box = new QHBoxLayout();
    m_docked_status = new QLabel(tr("Режим Switch: ТВ (Docked)"), tab);
    m_docked_status->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 13px; color: #38BDF8;"));
    docked_box->addWidget(m_docked_status, 1);

    m_btn_toggle_docked = new QPushButton(tr("Переключить (ReverseNX)"), tab);
    m_btn_toggle_docked->setStyleSheet(QStringLiteral("padding: 5px 14px; font-weight: bold; border-color: #00D2FF; color: #00D2FF;"));
    connect(m_btn_toggle_docked, &QPushButton::clicked, this, &PerformanceOverlay::OnToggleDocked);
    docked_box->addWidget(m_btn_toggle_docked);
    layout->addLayout(docked_box);

    // 2. Universal Aspect Ratio Selector (All 17 formats)
    auto* aspect_box = new QHBoxLayout();
    auto* aspect_lbl = new QLabel(tr("Формат экрана (Все режимы):"), tab);
    aspect_lbl->setStyleSheet(QStringLiteral("font-weight: 500;"));
    aspect_box->addWidget(aspect_lbl);

    m_aspect_combo = new QComboBox(tab);
    m_aspect_combo->addItem(tr("16:9 (Стандарт)"), static_cast<u32>(Settings::AspectRatio::R16_9));
    m_aspect_combo->addItem(tr("4:3 (Классический)"), static_cast<u32>(Settings::AspectRatio::R4_3));
    m_aspect_combo->addItem(tr("21:9 (UltraWide)"), static_cast<u32>(Settings::AspectRatio::R21_9));
    m_aspect_combo->addItem(tr("16:10 (Steam Deck / WUXGA)"), static_cast<u32>(Settings::AspectRatio::R16_10));
    m_aspect_combo->addItem(tr("Растянуть во всё окно (Stretch)"), static_cast<u32>(Settings::AspectRatio::Stretch));
    m_aspect_combo->addItem(tr("32:9 (Super UltraWide)"), static_cast<u32>(Settings::AspectRatio::R32_9));
    m_aspect_combo->addItem(tr("1:1 (Квадрат)"), static_cast<u32>(Settings::AspectRatio::R1_1));
    m_aspect_combo->addItem(tr("3:2 (Surface / Портатив)"), static_cast<u32>(Settings::AspectRatio::R3_2));
    m_aspect_combo->addItem(tr("5:4 (SXGA)"), static_cast<u32>(Settings::AspectRatio::R5_4));
    m_aspect_combo->addItem(tr("18:9 (2:1 Univisium)"), static_cast<u32>(Settings::AspectRatio::R18_9));
    m_aspect_combo->addItem(tr("19.5:9 (Смартфоны)"), static_cast<u32>(Settings::AspectRatio::R19_5_9));
    m_aspect_combo->addItem(tr("20:9 (Киноформат)"), static_cast<u32>(Settings::AspectRatio::R20_9));
    m_aspect_combo->addItem(tr("21:10 (Широкий 16:10)"), static_cast<u32>(Settings::AspectRatio::R21_10));
    m_aspect_combo->addItem(tr("24:10 (3840x1600 UltraWide+)"), static_cast<u32>(Settings::AspectRatio::R24_10));
    m_aspect_combo->addItem(tr("32:10 (Super UltraWide 16:10)"), static_cast<u32>(Settings::AspectRatio::R32_10));
    m_aspect_combo->addItem(tr("48:9 (Три монитора 3x16:9)"), static_cast<u32>(Settings::AspectRatio::R48_9));
    m_aspect_combo->addItem(tr("Авто по размеру окна (3D Fit)"), static_cast<u32>(Settings::AspectRatio::Auto_Window));
    connect(m_aspect_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PerformanceOverlay::OnAspectRatioChanged);
    aspect_box->addWidget(m_aspect_combo, 1);
    layout->addLayout(aspect_box);

    // 3. FPS Locker / Speed limit
    auto* fps_limit_box = new QHBoxLayout();
    auto* fps_limit_lbl = new QLabel(tr("Лимит частоты кадров (FPS Locker):"), tab);
    fps_limit_lbl->setStyleSheet(QStringLiteral("font-weight: 500;"));
    fps_limit_box->addWidget(fps_limit_lbl);

    m_speed_limit_combo = new QComboBox(tab);
    m_speed_limit_combo->addItem(tr("60 FPS (100%)"), 100);
    m_speed_limit_combo->addItem(tr("30 FPS (50%)"), 50);
    m_speed_limit_combo->addItem(tr("120 FPS (200%)"), 200);
    m_speed_limit_combo->addItem(tr("Разблокирован (Без лимита)"), 0);
    connect(m_speed_limit_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PerformanceOverlay::OnSpeedLimitChanged);
    fps_limit_box->addWidget(m_speed_limit_combo, 1);
    layout->addLayout(fps_limit_box);

    // 4. Toggles Grid (DPS, Auto-Aspect 3D Patch, Auto 60 FPS Unlock, LSFG, HDR10)
    m_chk_auto_aspect = new QCheckBox(tr("Авто-патч 3D-камеры под формат экрана (без растяжения)"), tab);
    m_chk_auto_aspect->setChecked(Settings::values.auto_aspect_ratio_patch.GetValue());
    m_chk_auto_aspect->setStyleSheet(QStringLiteral("font-weight: bold; color: #38BDF8;"));
    connect(m_chk_auto_aspect, &QCheckBox::toggled, this, &PerformanceOverlay::OnAutoAspectPatchToggled);
    layout->addWidget(m_chk_auto_aspect);

    m_chk_60fps_unlock = new QCheckBox(tr("Авто-разблокировка 60/120 FPS (Снятие лимита 30 FPS VSync)"), tab);
    m_chk_60fps_unlock->setChecked(Settings::values.auto_60fps_unlock.GetValue());
    m_chk_60fps_unlock->setStyleSheet(QStringLiteral("font-weight: bold; color: %1;").arg(StormTheme::GetThemePalette().accent));
    connect(m_chk_60fps_unlock, &QCheckBox::toggled, this, &PerformanceOverlay::OnAuto60FpsToggled);
    layout->addWidget(m_chk_60fps_unlock);

    auto* row_toggles = new QHBoxLayout();
    m_chk_dps = new QCheckBox(tr("Динамическое разрешение (DPS)"), tab);
    m_chk_dps->setChecked(Settings::values.dynamic_performance_scaler.GetValue());
    m_chk_dps->setStyleSheet(QStringLiteral("font-weight: bold; color: #34D399;"));
    connect(m_chk_dps, &QCheckBox::toggled, this, &PerformanceOverlay::OnDpsToggled);
    row_toggles->addWidget(m_chk_dps);

    m_chk_lsfg = new QCheckBox(tr("Генерация кадров LSFG"), tab);
    m_chk_lsfg->setChecked(Settings::values.frame_gen.GetValue());
    m_chk_lsfg->setStyleSheet(QStringLiteral("font-weight: bold; color: #F59E0B;"));
    connect(m_chk_lsfg, &QCheckBox::toggled, this, &PerformanceOverlay::OnLsfgToggled);
    row_toggles->addWidget(m_chk_lsfg);

    m_chk_hdr10 = new QCheckBox(tr("Auto-HDR10"), tab);
    m_chk_hdr10->setChecked(Settings::values.enable_hdr10.GetValue());
    m_chk_hdr10->setStyleSheet(QStringLiteral("font-weight: bold; color: #F472B6;"));
    connect(m_chk_hdr10, &QCheckBox::toggled, this, &PerformanceOverlay::OnHdr10Toggled);
    row_toggles->addWidget(m_chk_hdr10);
    layout->addLayout(row_toggles);

    // 5. GPU Accuracy & 1-Click Artifact Auto-Healer
    auto* gpu_acc_box = new QHBoxLayout();
    auto* gpu_acc_lbl = new QLabel(tr("Точность GPU:"), tab);
    gpu_acc_lbl->setStyleSheet(QStringLiteral("font-weight: 500;"));
    gpu_acc_box->addWidget(gpu_acc_lbl);

    m_btn_gpu_acc = new QPushButton(tr("Точность GPU: High"), tab);
    connect(m_btn_gpu_acc, &QPushButton::clicked, this, &PerformanceOverlay::OnToggleGpuAccuracy);
    gpu_acc_box->addWidget(m_btn_gpu_acc, 1);

    m_btn_heal_gpu = new QPushButton(tr("Автолечение артефактов"), tab);
    m_btn_heal_gpu->setToolTip(tr("Мгновенно устраняет полосы, засветы и битые текстуры (GPU High + Safe DMA + Accurate Barriers + Reactive Flushing)"));
    m_btn_heal_gpu->setStyleSheet(QStringLiteral(
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0F766E, stop:1 #0284C7); "
        "border: 1px solid #38BDF8; border-radius: 6px; color: #FFFFFF; font-weight: bold; padding: 6px 12px; }"
        "QPushButton:hover { background: #00D2FF; color: #000000; }"));
    connect(m_btn_heal_gpu, &QPushButton::clicked, this, &PerformanceOverlay::OnAutoHealGraphicsArtifacts);
    gpu_acc_box->addWidget(m_btn_heal_gpu, 1);
    layout->addLayout(gpu_acc_box);

    layout->addStretch(1);
}

void PerformanceOverlay::SetMode(OverlayMode mode) {
    m_mode = mode;
    m_is_expanded = (m_mode != OverlayMode::Compact);

    if (m_mode == OverlayMode::Sidebar) {
        if (m_sidebar_widget) {
            m_stack->setCurrentWidget(m_sidebar_widget);
        }
        const int h = std::clamp(m_mainWindow ? m_mainWindow->height() - 40 : 640, 520, 920);
        setFixedSize(350, h);
        if (m_mainWindow) {
            const auto m_pos = m_mainWindow->pos();
            move(m_pos.x() + m_mainWindow->width() - 365, m_pos.y() + 30);
        }
        RefreshSidebarState();
    } else if (m_mode == OverlayMode::Expanded) {
        m_stack->setCurrentWidget(m_expanded_widget);
        setFixedSize(670, 500);
        RefreshCheats();
        RefreshSaveSlots();
        RefreshAmiiboList();
        RefreshCfwInfo();
    } else {
        m_stack->setCurrentWidget(m_compact_widget);
        setFixedSize(280, 185);
    }
    update();
}

void PerformanceOverlay::ToggleSidebar() {
    if (isVisible() && m_mode == OverlayMode::Sidebar) {
        SetMode(OverlayMode::Compact);
    } else {
        SetMode(OverlayMode::Sidebar);
        show();
        raise();
        activateWindow();
    }
}

void PerformanceOverlay::SetExpanded(bool expanded) {
    SetMode(expanded ? OverlayMode::Expanded : OverlayMode::Compact);
}

void PerformanceOverlay::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_F10 || event->key() == Qt::Key_F9) {
        if (m_mode == OverlayMode::Sidebar) {
            SetMode(OverlayMode::Compact);
            event->accept();
            return;
        }
    }
    QWidget::keyPressEvent(event);
}

void PerformanceOverlay::OnToggleExpand() {
    SetExpanded(!m_is_expanded);
}

void PerformanceOverlay::resetPosition(const QPoint& _) {
    auto pos = m_mainWindow->pos();
    move(pos.x() + m_offset.x(), pos.y() + m_offset.y());
}

void PerformanceOverlay::updateStats(const Core::PerfStatsResults& results,
                                     const VideoCore::ShaderNotify& shaders) {
    if (!isVisible()) return;

    auto fps = results.average_game_fps;
    if (!std::isnan(fps)) {
        static constexpr double FPS_SAMPLE_THRESHOLD = 3.0;

        QString fpsText = tr("%1 fps").arg(std::round(fps), 0, 'f', 0);
        m_compact_fps->setText(fpsText);
        if (m_lbl_fps) m_lbl_fps->setText(fpsText);

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

            if (m_lbl_fps_avg) m_lbl_fps_avg->setText(tr("Avg: %1").arg(avg, 0, 'f', 0));
        }

        if (!m_fpsSamples.empty()) {
            auto [min_it, max_it] = std::minmax_element(m_fpsSamples.begin(), m_fpsSamples.end());
            double min_fps = *min_it;
            double max_fps = *max_it;

            if (m_lbl_fps_min) m_lbl_fps_min->setText(tr("Min: %1").arg(min_fps, 0, 'f', 0));
            if (m_lbl_fps_max) m_lbl_fps_max->setText(tr("Max: %1").arg(max_fps, 0, 'f', 0));

            if (m_lbl_fps_1low) {
                std::vector<double> sorted_fps(m_fpsSamples.begin(), m_fpsSamples.end());
                std::sort(sorted_fps.begin(), sorted_fps.end());
                const size_t low_count = std::max<size_t>(1, sorted_fps.size() / 10);
                double low_sum = 0.0;
                for (size_t i = 0; i < low_count; ++i) {
                    low_sum += sorted_fps[i];
                }
                m_lbl_fps_1low->setText(tr("1% Low: %1").arg(low_sum / static_cast<double>(low_count), 0, 'f', 0));
            }

            m_compact_fpsGraph->setSamples(m_fpsSamples, max_fps);
            if (m_fpsGraph) m_fpsGraph->setSamples(m_fpsSamples, max_fps);
        }
    }

    auto ft = results.frametime;
    if (!std::isnan(ft)) {
        static constexpr double FT_SAMPLE_THRESHOLD = 500.0;

        double ft_ms = results.frametime * 1000.0;
        QString ftText = tr("%1 ms").arg(ft_ms, 0, 'f', 2);
        m_compact_ft->setText(ftText);
        if (m_lbl_ft) m_lbl_ft->setText(ftText);

        if (ft_ms <= FT_SAMPLE_THRESHOLD)
            m_frametimeSamples.push_back(ft_ms);

        if (m_frametimeSamples.size() > NUM_FRAMETIME_SAMPLES)
            m_frametimeSamples.pop_front();

        if (!m_frametimeSamples.empty()) {
            auto [min_it, max_it] =
                std::minmax_element(m_frametimeSamples.begin(), m_frametimeSamples.end());
            if (m_lbl_ft_min) m_lbl_ft_min->setText(tr("Min: %1").arg(*min_it, 0, 'f', 1));
            if (m_lbl_ft_max) m_lbl_ft_max->setText(tr("Max: %1").arg(*max_it, 0, 'f', 1));
        }

        if (m_frametimeSamples.size() >= 2) {
            const int back_search = static_cast<int>(std::min(size_t(10), m_frametimeSamples.size() - 1));
            double sum = std::accumulate(m_frametimeSamples.end() - back_search,
                                         m_frametimeSamples.end(), 0.0);
            double avg = sum / back_search;

            if (m_lbl_ft_avg) m_lbl_ft_avg->setText(tr("Avg: %1").arg(avg, 0, 'f', 1));
        }
    }

    // Dynamic Performance Scaler status (live adaptive scale factor)
    if (Settings::values.dynamic_performance_scaler.GetValue()) {
        const float scale = Core::DynamicPerformanceScaler::GetEffectiveScaleFactor();
        QString dpsText = tr("DPS: %1x").arg(scale, 0, 'f', 2);
        m_compact_dps->setText(dpsText);
        m_compact_dps->setVisible(true);
        if (m_lbl_dps) m_lbl_dps->setText(dpsText);
    } else {
        m_compact_dps->setVisible(false);
        if (m_lbl_dps) m_lbl_dps->setText(tr("DPS: Выкл"));
    }

    // Resolution scale info
    if (m_lbl_res) {
        const auto& ri = Settings::values.resolution_info;
        const float res_scale = static_cast<float>(ri.up_scale) / static_cast<float>(1U << ri.down_shift);
        m_lbl_res->setText(tr("Масштаб: %1x").arg(res_scale, 0, 'f', 1));
    }

    if (m_lbl_lsfg) {
        if (Settings::values.frame_gen.GetValue()) {
            m_lbl_lsfg->setText(tr("LSFG: %1x").arg(Settings::values.frame_gen_multiplier.GetValue()));
        } else {
            m_lbl_lsfg->setText(tr("LSFG: Выкл"));
        }
    }

    if (m_lbl_shaders) {
        const int building = const_cast<VideoCore::ShaderNotify&>(shaders).ShadersBuilding();
        const u64 jit_opt = Core::ARM::StormTier2Compiler::GetRewrittenCount() +
                            Core::ARM::StormTier2Compiler::GetSuperblockOptCount();
        if (building > 0) {
            m_lbl_shaders->setText(tr("Шейдеры: +%1 | JIT: %2").arg(building).arg(jit_opt));
        } else {
            m_lbl_shaders->setText(tr("Шейдеры: OK | JIT Tier-3: %1").arg(jit_opt));
        }
    }
}

void PerformanceOverlay::RefreshCheats() {
    if (!QtCommon::system || !QtCommon::system->IsPoweredOn()) return;
    const u64 title_id = QtCommon::system->GetApplicationProcessProgramID();
    if (title_id == 0) return;

    if (m_game_title_lbl) {
        m_game_title_lbl->setText(QStringLiteral("(ID: 0x%1)").arg(title_id, 16, 16, QLatin1Char('0')).toUpper());
    }

    m_cheats.clear();
    m_cheat_list->blockSignals(true);
    m_cheat_list->clear();

    const auto& disabled = Settings::values.disabled_addons[title_id];
    const bool has_explicit_enabled = std::any_of(disabled.begin(), disabled.end(), [](const std::string& s) {
        return s.rfind("__ENABLED__:", 0) == 0;
    });

    QStringList search_dirs;
    const QString load_dir = QString::fromStdString(Common::FS::PathToUTF8String(
        Common::FS::GetEdenPath(Common::FS::EdenPath::LoadDir)));
    search_dirs << QDir(load_dir).filePath(QStringLiteral("%1/cheats").arg(title_id, 16, 16, QLatin1Char('0')).toUpper());

    const QString sdmc_dir = QString::fromStdString(Common::FS::PathToUTF8String(
        Common::FS::GetEdenPath(Common::FS::EdenPath::SDMCDir)));
    search_dirs << QDir(sdmc_dir).filePath(QStringLiteral("atmosphere/contents/%1/cheats").arg(title_id, 16, 16, QLatin1Char('0')).toUpper());

    for (const auto& dir_path : search_dirs) {
        QDir dir(dir_path);
        if (!dir.exists()) continue;

        const auto files = dir.entryInfoList(QStringList{QStringLiteral("*.txt")}, QDir::Files);
        for (const auto& fi : files) {
            QFile file(fi.absoluteFilePath());
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

            QTextStream stream(&file);
            QString cur_name;
            QString cur_code;

            while (!stream.atEnd()) {
                QString line = stream.readLine().trimmed();
                if (line.isEmpty() || line.startsWith(QStringLiteral("#")) || line.startsWith(QStringLiteral("//")) || line.startsWith(QStringLiteral(";"))) continue;

                if (line.startsWith(QStringLiteral("[")) && line.contains(QStringLiteral("]"))) {
                    if (!cur_name.isEmpty() && !cur_code.isEmpty()) {
                        bool enabled = has_explicit_enabled
                            ? (std::find(disabled.begin(), disabled.end(), "__ENABLED__:" + cur_name.toStdString()) != disabled.end())
                            : false;
                        m_cheats.push_back({cur_name, cur_code.trimmed(), enabled});
                    }
                    int close_idx = line.indexOf(QLatin1Char(']'));
                    cur_name = line.mid(1, close_idx - 1).trimmed();
                    cur_code.clear();
                } else {
                    cur_code += line + QStringLiteral("\n");
                }
            }
            if (!cur_name.isEmpty() && !cur_code.isEmpty()) {
                bool enabled = has_explicit_enabled
                    ? (std::find(disabled.begin(), disabled.end(), "__ENABLED__:" + cur_name.toStdString()) != disabled.end())
                    : false;
                m_cheats.push_back({cur_name, cur_code.trimmed(), enabled});
            }
        }
    }

    int active_count = 0;
    for (const auto& c : m_cheats) {
        auto* item = new QListWidgetItem(c.name, m_cheat_list);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(c.enabled ? Qt::Checked : Qt::Unchecked);
        if (c.enabled) active_count++;
    }

    m_cheat_list->blockSignals(false);

    if (m_cheats.empty()) {
        m_cheat_status->setText(tr("Для данной игры читы не найдены в load/ или SDMC."));
        m_cheat_status->setStyleSheet(QStringLiteral("color: #FFFFFF;"));
    } else {
        m_cheat_status->setText(tr("Активно читов: %1 из %2 (Применяются на лету)").arg(active_count).arg(m_cheats.size()));
        m_cheat_status->setStyleSheet(QStringLiteral("font-weight: bold; color: #34D399;"));
    }
}

void PerformanceOverlay::OnCheatToggled(int row) {
    if (row < 0 || row >= static_cast<int>(m_cheats.size())) return;
    auto* item = m_cheat_list->item(row);
    if (!item) return;

    m_cheats[row].enabled = (item->checkState() == Qt::Checked);

    if (!QtCommon::system || !QtCommon::system->IsPoweredOn()) return;
    const u64 title_id = QtCommon::system->GetApplicationProcessProgramID();
    if (title_id == 0) return;

    auto& disabled = Settings::values.disabled_addons[title_id];
    std::vector<std::string> new_disabled;
    for (const auto& entry : disabled) {
        if (entry.rfind("__ENABLED__:", 0) != 0) {
            bool is_this_cheat = std::any_of(m_cheats.begin(), m_cheats.end(), [&](const CheatEntryUI& c) {
                return c.name.toStdString() == entry;
            });
            if (!is_this_cheat) new_disabled.push_back(entry);
        }
    }

    std::vector<Core::Memory::CheatEntry> active_cheat_entries;
    const Core::Memory::TextCheatParser parser;
    int active_count = 0;

    for (const auto& c : m_cheats) {
        if (!c.enabled) {
            new_disabled.push_back(c.name.toStdString());
        } else {
            active_count++;
            new_disabled.push_back("__ENABLED__:" + c.name.toStdString());
            std::string formatted = fmt::format("[{}]\n{}\n", c.name.toStdString(), c.code.toStdString());
            auto parsed = parser.Parse(formatted);
            for (auto& p : parsed) {
                active_cheat_entries.push_back(std::move(p));
            }
        }
    }

    disabled = std::move(new_disabled);

    // Dynamically apply to running game!
    QtCommon::system->ReloadCheatList(active_cheat_entries);
    m_cheat_status->setText(tr("Читы обновлены на лету! Активно: %1 из %2").arg(active_count).arg(m_cheats.size()));
    m_cheat_status->setStyleSheet(QStringLiteral("font-weight: bold; color: #00D2FF;"));
}

void PerformanceOverlay::OnReloadCheats() {
    RefreshCheats();
}

void PerformanceOverlay::RefreshSaveSlots() {
    if (!QtCommon::system || !QtCommon::system->IsPoweredOn()) return;
    const u64 title_id = QtCommon::system->GetApplicationProcessProgramID();
    if (title_id == 0) return;

    for (int slot = 1; slot <= 5; ++slot) {
        const auto path = Core::GetSaveStatePath(title_id, slot);
        Core::SaveStateHeader header{};
        const bool exists = Core::GetSaveStateInfo(path, header);

        auto& ui_slot = m_slots[slot - 1];
        if (exists && header.timestamp > 0) {
            const QDateTime dt = QDateTime::fromSecsSinceEpoch(header.timestamp);
            const double mb = static_cast<double>(header.dram_compressed_size) / (1024.0 * 1024.0);
            ui_slot.label->setText(tr("Слот %1: %2 (%3 МБ)").arg(slot).arg(dt.toString(QStringLiteral("dd.MM.yyyy HH:mm"))).arg(mb, 0, 'f', 1));
            ui_slot.label->setStyleSheet(QStringLiteral("font-weight: bold; color: #38BDF8;"));
            ui_slot.btn_load->setEnabled(true);
        } else {
            ui_slot.label->setText(tr("Слот %1: [Пусто]").arg(slot));
            ui_slot.label->setStyleSheet(QStringLiteral("color: #FFFFFF;"));
            ui_slot.btn_load->setEnabled(false);
        }
    }
}

void PerformanceOverlay::OnSaveSlot(int slot) {
    if (m_mainWindow) {
        m_mainWindow->OnSaveStateSlot(slot);
        RefreshSaveSlots();
        return;
    }
    if (!QtCommon::system || !QtCommon::system->IsPoweredOn()) return;
    const u64 title_id = QtCommon::system->GetApplicationProcessProgramID();
    if (title_id == 0) return;

    const auto path = Core::GetSaveStatePath(title_id, slot);
    auto res = Core::CreateSaveState(*QtCommon::system, path);

    if (res == Core::SaveStateResult::Success) {
        m_save_status->setText(tr("Состояние игры успешно сохранено в Слот %1!").arg(slot));
        m_save_status->setStyleSheet(QStringLiteral("font-weight: bold; color: #34D399;"));
        RefreshSaveSlots();
    } else {
        m_save_status->setText(tr("Ошибка при сохранении в Слот %1.").arg(slot));
        m_save_status->setStyleSheet(QStringLiteral("font-weight: bold; color: #EF4444;"));
    }
}

void PerformanceOverlay::OnLoadSlot(int slot) {
    if (m_mainWindow) {
        m_mainWindow->OnLoadStateSlot(slot);
        return;
    }
    if (!QtCommon::system || !QtCommon::system->IsPoweredOn()) return;
    const u64 title_id = QtCommon::system->GetApplicationProcessProgramID();
    if (title_id == 0) return;

    const auto path = Core::GetSaveStatePath(title_id, slot);
    auto res = Core::LoadSaveState(*QtCommon::system, path);

    if (res == Core::SaveStateResult::Success) {
        m_save_status->setText(tr("Состояние игры успешно загружено из Слота %1!").arg(slot));
        m_save_status->setStyleSheet(QStringLiteral("font-weight: bold; color: #34D399;"));
    } else {
        m_save_status->setText(tr("Ошибка при загрузке Слота %1.").arg(slot));
        m_save_status->setStyleSheet(QStringLiteral("font-weight: bold; color: #EF4444;"));
    }
}

void PerformanceOverlay::RefreshAmiiboList() {
    m_amiibo_combo->clear();
    const QString amiibo_dir = QString::fromStdString(Common::FS::PathToUTF8String(
        Common::FS::GetEdenPath(Common::FS::EdenPath::AmiiboDir)));

    QDir dir(amiibo_dir);
    if (dir.exists()) {
        const auto files = dir.entryInfoList(QStringList{QStringLiteral("*.bin")}, QDir::Files, QDir::Name);
        for (const auto& fi : files) {
            m_amiibo_combo->addItem(fi.baseName(), fi.absoluteFilePath());
        }
    }

    if (m_mainWindow && m_mainWindow->IsAmiiboActive()) {
        m_amiibo_status->setText(tr("Виртуальный Amiibo активен в игре"));
        m_amiibo_status->setStyleSheet(QStringLiteral("font-weight: bold; color: #34D399;"));
    } else {
        m_amiibo_status->setText(tr("Amiibo не подключен"));
        m_amiibo_status->setStyleSheet(QStringLiteral("color: #FFFFFF;"));
    }
}

void PerformanceOverlay::OnInjectAmiibo() {
    if (m_amiibo_combo->currentIndex() < 0) return;
    const QString path = m_amiibo_combo->currentData().toString();
    if (path.isEmpty() || !QFile::exists(path)) return;

    if (m_mainWindow) {
        m_mainWindow->LoadAmiibo(path);
        RefreshAmiiboList();
    }
}

void PerformanceOverlay::OnEjectAmiibo() {
    if (m_mainWindow) {
        m_mainWindow->EjectAmiibo();
        RefreshAmiiboList();
    }
}

void PerformanceOverlay::OnBrowseAmiibo() {
    const QString path = QFileDialog::getOpenFileName(this, tr("Выберите файл Amiibo"), {}, tr("Файлы Amiibo (*.bin);;Все файлы (*.*)"));
    if (path.isEmpty()) return;

    m_amiibo_combo->addItem(QFileInfo(path).baseName(), path);
    m_amiibo_combo->setCurrentIndex(m_amiibo_combo->count() - 1);
    OnInjectAmiibo();
}

void PerformanceOverlay::RefreshCfwInfo() {
    const bool is_docked = Settings::values.use_docked_mode.GetValue() == Settings::ConsoleMode::Docked;
    m_docked_status->setText(is_docked ? tr("Режим Switch: ТВ (Docked)") : tr("Режим Switch: Портативный (Handheld)"));
    m_docked_status->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 13px; color: %1;").arg(is_docked ? QStringLiteral("#38BDF8") : QStringLiteral("#34D399")));

    if (m_aspect_combo) {
        m_aspect_combo->blockSignals(true);
        const u32 cur_aspect = static_cast<u32>(Settings::values.aspect_ratio.GetValue());
        const int idx = m_aspect_combo->findData(cur_aspect);
        if (idx >= 0) {
            m_aspect_combo->setCurrentIndex(idx);
        }
        m_aspect_combo->blockSignals(false);
    }

    const bool has_speed_limit = Settings::values.use_speed_limit.GetValue();
    const u16 speed = Settings::values.speed_limit.GetValue();

    m_speed_limit_combo->blockSignals(true);
    if (!has_speed_limit) {
        m_speed_limit_combo->setCurrentIndex(3); // Unlocked
    } else if (speed == 50) {
        m_speed_limit_combo->setCurrentIndex(1); // 30 FPS
    } else if (speed == 200) {
        m_speed_limit_combo->setCurrentIndex(2); // 120 FPS
    } else {
        m_speed_limit_combo->setCurrentIndex(0); // 60 FPS
    }
    m_speed_limit_combo->blockSignals(false);

    if (m_chk_auto_aspect) {
        m_chk_auto_aspect->blockSignals(true);
        m_chk_auto_aspect->setChecked(Settings::values.auto_aspect_ratio_patch.GetValue());
        m_chk_auto_aspect->blockSignals(false);
    }
    if (m_chk_60fps_unlock) {
        m_chk_60fps_unlock->blockSignals(true);
        m_chk_60fps_unlock->setChecked(Settings::values.auto_60fps_unlock.GetValue());
        m_chk_60fps_unlock->blockSignals(false);
    }
    m_chk_dps->blockSignals(true);
    m_chk_dps->setChecked(Settings::values.dynamic_performance_scaler.GetValue());
    m_chk_dps->blockSignals(false);

    if (m_chk_lsfg) {
        m_chk_lsfg->blockSignals(true);
        m_chk_lsfg->setChecked(Settings::values.frame_gen.GetValue());
        m_chk_lsfg->blockSignals(false);
    }
    if (m_chk_hdr10) {
        m_chk_hdr10->blockSignals(true);
        m_chk_hdr10->setChecked(Settings::values.enable_hdr10.GetValue());
        m_chk_hdr10->blockSignals(false);
    }

    const auto gpu_acc = Settings::values.gpu_accuracy.GetValue();
    m_btn_gpu_acc->setText(gpu_acc == Settings::GpuAccuracy::High ? tr("Точность GPU: High") : tr("Точность GPU: Normal"));
}

void PerformanceOverlay::OnToggleDocked() {
    if (m_mainWindow) {
        m_mainWindow->OnToggleDockedMode();
        RefreshCfwInfo();
    }
}

void PerformanceOverlay::OnAspectRatioChanged(int index) {
    if (!m_aspect_combo || index < 0) return;
    const u32 val = m_aspect_combo->itemData(index).toUInt();
    Settings::values.aspect_ratio.SetValue(static_cast<Settings::AspectRatio>(val));
    if (m_mainWindow) {
        m_mainWindow->OnDynamicSettingChangedFromUI();
    }
}

void PerformanceOverlay::OnSpeedLimitChanged(int index) {
    switch (index) {
    case 0: // 60 FPS (100%)
        Settings::values.use_speed_limit.SetValue(true);
        Settings::values.speed_limit.SetValue(100);
        break;
    case 1: // 30 FPS (50%)
        Settings::values.use_speed_limit.SetValue(true);
        Settings::values.speed_limit.SetValue(50);
        break;
    case 2: // 120 FPS (200%)
        Settings::values.use_speed_limit.SetValue(true);
        Settings::values.speed_limit.SetValue(200);
        break;
    case 3: // Unlocked
        Settings::values.use_speed_limit.SetValue(false);
        break;
    }
}

void PerformanceOverlay::OnDpsToggled(bool checked) {
    Settings::values.dynamic_performance_scaler.SetValue(checked);
}

void PerformanceOverlay::OnAutoAspectPatchToggled(bool checked) {
    Settings::values.auto_aspect_ratio_patch.SetValue(checked);
}

void PerformanceOverlay::OnAuto60FpsToggled(bool checked) {
    Settings::values.auto_60fps_unlock.SetValue(checked);
}

void PerformanceOverlay::OnLsfgToggled(bool checked) {
    Settings::values.frame_gen.SetValue(checked);
}

void PerformanceOverlay::OnHdr10Toggled(bool checked) {
    Settings::values.enable_hdr10.SetValue(checked);
}

void PerformanceOverlay::OnToggleGpuAccuracy() {
    if (m_mainWindow) {
        m_mainWindow->OnToggleGpuAccuracy();
        RefreshCfwInfo();
    }
}

void PerformanceOverlay::OnAutoHealGraphicsArtifacts() {
    if (m_mainWindow) {
        m_mainWindow->OnHealGraphicsArtifacts();
        RefreshCfwInfo();
        if (m_btn_heal_gpu) {
            m_btn_heal_gpu->setText(tr("Артефакты устранены!"));
        }
    }
}

void PerformanceOverlay::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Cyber translucent background with neon cyan border
    painter.setBrush(QColor(11, 15, 25, 240));
    painter.setPen(QPen(QColor(0, 210, 255, 120), 1.5));
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 10.0, 10.0);
}

void PerformanceOverlay::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_drag_start_pos = event->pos();
    }
}

void PerformanceOverlay::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        QPoint new_global_pos = event->globalPosition().toPoint() - m_drag_start_pos;
        m_offset = new_global_pos - m_mainWindow->pos();
        move(new_global_pos);
    }
}

void PerformanceOverlay::closeEvent(QCloseEvent* event) {
    emit closed();
}


// =========================================================================
// In-Game Quick Settings Sidebar (F10)
// =========================================================================

void PerformanceOverlay::SetupSidebarUI() {
    m_sidebar_widget = new QWidget(this);
    m_sidebar_widget->setFixedWidth(350);
    auto* layout = new QVBoxLayout(m_sidebar_widget);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(6);

    const auto pal = StormTheme::GetThemePalette();
    const QString accent = pal.accent.isEmpty() ? QStringLiteral("#00D2FF") : pal.accent;

    // Header bar
    auto* header = new QHBoxLayout();
    header->setSpacing(8);

    auto* title_lbl = new QLabel(StormLang(QStringLiteral("БЫСТРЫЕ НАСТРОЙКИ"), QStringLiteral("QUICK SETTINGS")), m_sidebar_widget);
    title_lbl->setStyleSheet(QStringLiteral("font-weight: 800; font-size: 13px; color: %1; letter-spacing: 0.5px;").arg(accent));
    header->addWidget(title_lbl);

    header->addStretch(1);

    auto* btn_to_hud = new QPushButton(StormLang(QStringLiteral("В HUD"), QStringLiteral("To HUD")), m_sidebar_widget);
    btn_to_hud->setCursor(Qt::PointingHandCursor);
    btn_to_hud->setStyleSheet(QStringLiteral("padding: 3px 8px; font-size: 10.5px; font-weight: bold; border-radius: 5px;"));
    connect(btn_to_hud, &QPushButton::clicked, this, [this]() {
        SetMode(OverlayMode::Compact);
    });
    header->addWidget(btn_to_hud);

    auto* btn_close = new QPushButton(m_sidebar_widget);
    btn_close->setIcon(MakeVectorCloseIcon(16));
    btn_close->setFixedSize(22, 22);
    btn_close->setCursor(Qt::PointingHandCursor);
    btn_close->setStyleSheet(QStringLiteral("padding: 0; font-size: 11px; font-weight: bold; border-radius: 4px;"));
    connect(btn_close, &QPushButton::clicked, this, [this]() {
        SetMode(OverlayMode::Compact);
    });
    header->addWidget(btn_close);

    layout->addLayout(header);

    m_sidebar_game_title = new QLabel(StormLang(QStringLiteral("Сессия эмулятора"), QStringLiteral("Emulation Session")), m_sidebar_widget);
    m_sidebar_game_title->setStyleSheet(QStringLiteral("font-size: 11px; color: %1; font-weight: 600; padding-bottom: 2px;").arg(pal.textSecondary.isEmpty() ? QStringLiteral("#94A3B8") : pal.textSecondary));
    layout->addWidget(m_sidebar_game_title);

    // Scroll Area for the Bento Blocks
    auto* scroll = new QScrollArea(m_sidebar_widget);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("QScrollArea { background: transparent; border: none; } QWidget#SidebarScrollContents { background: transparent; }"));

    auto* scroll_contents = new QWidget(scroll);
    scroll_contents->setObjectName(QStringLiteral("SidebarScrollContents"));
    auto* blocks_layout = new QVBoxLayout(scroll_contents);
    blocks_layout->setContentsMargins(0, 4, 4, 4);
    blocks_layout->setSpacing(10);

    const QString bento_style = QStringLiteral(
        "QGroupBox {"
        "  background: rgba(15, 23, 42, 0.72);"
        "  border: 1px solid rgba(255, 255, 255, 0.10);"
        "  border-radius: 8px;"
        "  margin-top: 14px;"
        "  padding: 18px 10px 10px 10px;"
        "  font-weight: bold;"
        "  color: %1;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top left;"
        "  left: 10px;"
        "  padding: 2px 8px;"
        "  color: %1;"
        "  font-size: 11px;"
        "  font-weight: bold;"
        "  background: #0F172A;"
        "  border: 1px solid rgba(255, 255, 255, 0.12);"
        "  border-radius: 4px;"
        "}"
    ).arg(accent);

    // -------------------------------------------------------------
    // Bento Block 1: Сохранения (SESS)
    // -------------------------------------------------------------
    auto* group_sess = new QGroupBox(StormLang(QStringLiteral("Быстрые сохранения (SESS)"), QStringLiteral("Quick Saves (SESS)")), scroll_contents);
    group_sess->setStyleSheet(bento_style);
    auto* sess_layout = new QVBoxLayout(group_sess);
    sess_layout->setSpacing(6);
    sess_layout->setContentsMargins(6, 8, 6, 6);

    auto* slot_row = new QHBoxLayout();
    auto* slot_lbl = new QLabel(StormLang(QStringLiteral("Слот:"), QStringLiteral("Slot:")), group_sess);
    slot_lbl->setStyleSheet(QStringLiteral("font-weight: 600; font-size: 11px;"));
    slot_row->addWidget(slot_lbl);

    m_sidebar_slot_combo = new QComboBox(group_sess);
    m_sidebar_slot_combo->setCursor(Qt::PointingHandCursor);
    for (int i = 1; i <= 10; ++i) {
        m_sidebar_slot_combo->addItem(StormLang(QStringLiteral("Слот %1").arg(i), QStringLiteral("Slot %1").arg(i)), i);
    }
    slot_row->addWidget(m_sidebar_slot_combo, 1);
    sess_layout->addLayout(slot_row);

    auto* sess_btns_row = new QHBoxLayout();
    sess_btns_row->setSpacing(6);
    m_btn_sidebar_save = new QPushButton(StormLang(QStringLiteral("Сохранить (F5)"), QStringLiteral("Quick Save (F5)")), group_sess);
    m_btn_sidebar_save->setCursor(Qt::PointingHandCursor);
    connect(m_btn_sidebar_save, &QPushButton::clicked, this, &PerformanceOverlay::OnSidebarSaveClicked);
    sess_btns_row->addWidget(m_btn_sidebar_save);

    m_btn_sidebar_load = new QPushButton(StormLang(QStringLiteral("Загрузить (F7)"), QStringLiteral("Quick Load (F7)")), group_sess);
    m_btn_sidebar_load->setCursor(Qt::PointingHandCursor);
    connect(m_btn_sidebar_load, &QPushButton::clicked, this, &PerformanceOverlay::OnSidebarLoadClicked);
    sess_btns_row->addWidget(m_btn_sidebar_load);
    sess_layout->addLayout(sess_btns_row);

    m_btn_sidebar_manager = new QPushButton(StormLang(QStringLiteral("Менеджер сохранений..."), QStringLiteral("Save State Manager...")), group_sess);
    m_btn_sidebar_manager->setCursor(Qt::PointingHandCursor);
    connect(m_btn_sidebar_manager, &QPushButton::clicked, this, &PerformanceOverlay::OnSidebarSaveManagerClicked);
    sess_layout->addWidget(m_btn_sidebar_manager);

    m_sidebar_save_status = new QLabel(StormLang(QStringLiteral("Слоты 1..10 готовы"), QStringLiteral("Slots 1..10 ready")), group_sess);
    m_sidebar_save_status->setStyleSheet(QStringLiteral("font-size: 10px; color: %1;").arg(pal.textSecondary.isEmpty() ? QStringLiteral("#94A3B8") : pal.textSecondary));
    sess_layout->addWidget(m_sidebar_save_status);

    blocks_layout->addWidget(group_sess);

    // -------------------------------------------------------------
    // Bento Block 2: Быстрые настройки (Графика и Производительность)
    // -------------------------------------------------------------
    auto* group_settings = new QGroupBox(StormLang(QStringLiteral("Быстрые настройки"), QStringLiteral("Quick Settings")), scroll_contents);
    group_settings->setStyleSheet(bento_style);
    auto* set_layout = new QVBoxLayout(group_settings);
    set_layout->setSpacing(6);
    set_layout->setContentsMargins(6, 8, 6, 6);

    // Resolution Scale
    auto* res_row = new QHBoxLayout();
    auto* res_lbl = new QLabel(StormLang(QStringLiteral("Разрешение:"), QStringLiteral("Resolution:")), group_settings);
    res_lbl->setStyleSheet(QStringLiteral("font-size: 11px; font-weight: 600;"));
    res_row->addWidget(res_lbl);

    m_sidebar_resolution_combo = new QComboBox(group_settings);
    m_sidebar_resolution_combo->setCursor(Qt::PointingHandCursor);
    m_sidebar_resolution_combo->addItem(QStringLiteral("0.5x (360p/540p)"), static_cast<int>(Settings::ResolutionSetup::Res1_2X));
    m_sidebar_resolution_combo->addItem(QStringLiteral("0.75x (540p/810p)"), static_cast<int>(Settings::ResolutionSetup::Res3_4X));
    m_sidebar_resolution_combo->addItem(QStringLiteral("1x (720p/1080p)"), static_cast<int>(Settings::ResolutionSetup::Res1X));
    m_sidebar_resolution_combo->addItem(QStringLiteral("1.5x (1080p/1620p)"), static_cast<int>(Settings::ResolutionSetup::Res3_2X));
    m_sidebar_resolution_combo->addItem(QStringLiteral("2x (1440p/2160p)"), static_cast<int>(Settings::ResolutionSetup::Res2X));
    const auto cur_res = Settings::values.resolution_setup.GetValue();
    const int cur_res_idx = m_sidebar_resolution_combo->findData(static_cast<int>(cur_res));
    if (cur_res_idx >= 0) {
        m_sidebar_resolution_combo->setCurrentIndex(cur_res_idx);
    }
    connect(m_sidebar_resolution_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PerformanceOverlay::OnSidebarResolutionChanged);
    res_row->addWidget(m_sidebar_resolution_combo, 1);
    set_layout->addLayout(res_row);

    // Scaling Filter
    auto* scale_row = new QHBoxLayout();
    auto* scale_lbl = new QLabel(StormLang(QStringLiteral("Фильтр:"), QStringLiteral("Filter:")), group_settings);
    scale_lbl->setStyleSheet(QStringLiteral("font-size: 11px; font-weight: 600;"));
    scale_row->addWidget(scale_lbl);

    m_sidebar_scaling_combo = new QComboBox(group_settings);
    m_sidebar_scaling_combo->setCursor(Qt::PointingHandCursor);
    m_sidebar_scaling_combo->addItem(QStringLiteral("Nearest (Ближайший)"), static_cast<int>(Settings::ScalingFilter::NearestNeighbor));
    m_sidebar_scaling_combo->addItem(QStringLiteral("Bilinear (Билинейный)"), static_cast<int>(Settings::ScalingFilter::Bilinear));
    m_sidebar_scaling_combo->addItem(QStringLiteral("Bicubic (Бикубический)"), static_cast<int>(Settings::ScalingFilter::Bicubic));
    m_sidebar_scaling_combo->addItem(QStringLiteral("Gaussian (Гауссов)"), static_cast<int>(Settings::ScalingFilter::Gaussian));
    m_sidebar_scaling_combo->addItem(QStringLiteral("ScaleForce"), static_cast<int>(Settings::ScalingFilter::ScaleForce));
    m_sidebar_scaling_combo->addItem(QStringLiteral("AMD FSR"), static_cast<int>(Settings::ScalingFilter::Fsr));
    const auto cur_filter = Settings::values.scaling_filter.GetValue();
    const int cur_filter_idx = m_sidebar_scaling_combo->findData(static_cast<int>(cur_filter));
    if (cur_filter_idx >= 0) {
        m_sidebar_scaling_combo->setCurrentIndex(cur_filter_idx);
    }
    connect(m_sidebar_scaling_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PerformanceOverlay::OnSidebarScalingChanged);
    scale_row->addWidget(m_sidebar_scaling_combo, 1);
    set_layout->addLayout(scale_row);

    // Speed Limit & VSync row
    auto* perf_btns_row = new QHBoxLayout();
    perf_btns_row->setSpacing(6);

    m_btn_sidebar_speed_limit = new QPushButton(StormLang(QStringLiteral("Лимит: 100%"), QStringLiteral("Limit: 100%")), group_settings);
    m_btn_sidebar_speed_limit->setCursor(Qt::PointingHandCursor);
    connect(m_btn_sidebar_speed_limit, &QPushButton::clicked, this, &PerformanceOverlay::OnSidebarSpeedLimitToggled);
    perf_btns_row->addWidget(m_btn_sidebar_speed_limit);

    m_btn_sidebar_vsync = new QPushButton(StormLang(QStringLiteral("VSync: Вкл"), QStringLiteral("VSync: On")), group_settings);
    m_btn_sidebar_vsync->setCursor(Qt::PointingHandCursor);
    connect(m_btn_sidebar_vsync, &QPushButton::clicked, this, &PerformanceOverlay::OnSidebarVsyncToggled);
    perf_btns_row->addWidget(m_btn_sidebar_vsync);
    set_layout->addLayout(perf_btns_row);

    // Docked mode
    m_btn_sidebar_docked = new QPushButton(StormLang(QStringLiteral("Режим: В доке"), QStringLiteral("Mode: Docked")), group_settings);
    m_btn_sidebar_docked->setCursor(Qt::PointingHandCursor);
    connect(m_btn_sidebar_docked, &QPushButton::clicked, this, &PerformanceOverlay::OnSidebarDockedToggled);
    set_layout->addWidget(m_btn_sidebar_docked);

    // Volume
    m_sidebar_volume_label = new QLabel(StormLang(QStringLiteral("Громкость: 100%"), QStringLiteral("Volume: 100%")), group_settings);
    m_sidebar_volume_label->setStyleSheet(QStringLiteral("font-size: 11px; font-weight: 600;"));
    set_layout->addWidget(m_sidebar_volume_label);

    auto* vol_slider = new QSlider(Qt::Horizontal, group_settings);
    vol_slider->setRange(0, 100);
    vol_slider->setValue(static_cast<int>(Settings::values.volume.GetValue()));
    vol_slider->setCursor(Qt::PointingHandCursor);
    connect(vol_slider, &QSlider::valueChanged, this, &PerformanceOverlay::OnSidebarVolumeChanged);
    set_layout->addWidget(vol_slider);

    blocks_layout->addWidget(group_settings);

    // -------------------------------------------------------------
    // Bento Block 3: Читы и Amiibo
    // -------------------------------------------------------------
    auto* group_tools = new QGroupBox(StormLang(QStringLiteral("Читы и Amiibo"), QStringLiteral("Cheats & Amiibo")), scroll_contents);
    group_tools->setStyleSheet(bento_style);
    auto* tools_layout = new QVBoxLayout(group_tools);
    tools_layout->setSpacing(6);
    tools_layout->setContentsMargins(6, 8, 6, 6);

    m_btn_sidebar_cheats = new QPushButton(StormLang(QStringLiteral("Управление читами..."), QStringLiteral("Cheats Manager...")), group_tools);
    m_btn_sidebar_cheats->setCursor(Qt::PointingHandCursor);
    connect(m_btn_sidebar_cheats, &QPushButton::clicked, this, &PerformanceOverlay::OnSidebarCheatsClicked);
    tools_layout->addWidget(m_btn_sidebar_cheats);

    auto* amiibo_row = new QHBoxLayout();
    amiibo_row->setSpacing(6);
    m_btn_sidebar_amiibo_browse = new QPushButton(StormLang(QStringLiteral("Amiibo база..."), QStringLiteral("Browse Amiibo...")), group_tools);
    m_btn_sidebar_amiibo_browse->setCursor(Qt::PointingHandCursor);
    connect(m_btn_sidebar_amiibo_browse, &QPushButton::clicked, this, &PerformanceOverlay::OnSidebarAmiiboBrowseClicked);
    amiibo_row->addWidget(m_btn_sidebar_amiibo_browse, 1);

    m_btn_sidebar_amiibo_eject = new QPushButton(StormLang(QStringLiteral("Извлечь"), QStringLiteral("Eject")), group_tools);
    m_btn_sidebar_amiibo_eject->setCursor(Qt::PointingHandCursor);
    connect(m_btn_sidebar_amiibo_eject, &QPushButton::clicked, this, &PerformanceOverlay::OnSidebarAmiiboEjectClicked);
    amiibo_row->addWidget(m_btn_sidebar_amiibo_eject);
    tools_layout->addLayout(amiibo_row);

    blocks_layout->addWidget(group_tools);

    // -------------------------------------------------------------
    // Bento Block 4: Сессия
    // -------------------------------------------------------------
    auto* group_session = new QGroupBox(StormLang(QStringLiteral("Управление сессией"), QStringLiteral("Session Controls")), scroll_contents);
    group_session->setStyleSheet(bento_style);
    auto* sess_ctrl_layout = new QVBoxLayout(group_session);
    sess_ctrl_layout->setSpacing(6);
    sess_ctrl_layout->setContentsMargins(6, 8, 6, 6);

    auto* row_ctrl1 = new QHBoxLayout();
    row_ctrl1->setSpacing(6);
    m_btn_sidebar_pause = new QPushButton(StormLang(QStringLiteral("Пауза / Пуск"), QStringLiteral("Pause / Resume")), group_session);
    m_btn_sidebar_pause->setCursor(Qt::PointingHandCursor);
    connect(m_btn_sidebar_pause, &QPushButton::clicked, this, &PerformanceOverlay::OnSidebarPauseClicked);
    row_ctrl1->addWidget(m_btn_sidebar_pause);

    m_btn_sidebar_restart = new QPushButton(StormLang(QStringLiteral("Перезапуск"), QStringLiteral("Restart")), group_session);
    m_btn_sidebar_restart->setCursor(Qt::PointingHandCursor);
    connect(m_btn_sidebar_restart, &QPushButton::clicked, this, &PerformanceOverlay::OnSidebarRestartClicked);
    row_ctrl1->addWidget(m_btn_sidebar_restart);
    sess_ctrl_layout->addLayout(row_ctrl1);

    auto* row_ctrl2 = new QHBoxLayout();
    row_ctrl2->setSpacing(6);
    m_btn_sidebar_configure = new QPushButton(StormLang(QStringLiteral("Все настройки..."), QStringLiteral("All Settings...")), group_session);
    m_btn_sidebar_configure->setCursor(Qt::PointingHandCursor);
    connect(m_btn_sidebar_configure, &QPushButton::clicked, this, &PerformanceOverlay::OnSidebarConfigureClicked);
    row_ctrl2->addWidget(m_btn_sidebar_configure);

    m_btn_sidebar_stop = new QPushButton(StormLang(QStringLiteral("Стоп"), QStringLiteral("Stop")), group_session);
    m_btn_sidebar_stop->setCursor(Qt::PointingHandCursor);
    m_btn_sidebar_stop->setStyleSheet(QStringLiteral("color: #EF4444; border-color: rgba(239, 68, 68, 0.4);"));
    connect(m_btn_sidebar_stop, &QPushButton::clicked, this, &PerformanceOverlay::OnSidebarStopClicked);
    row_ctrl2->addWidget(m_btn_sidebar_stop);
    sess_ctrl_layout->addLayout(row_ctrl2);

    blocks_layout->addWidget(group_session);

    blocks_layout->addStretch(1);
    scroll->setWidget(scroll_contents);
    layout->addWidget(scroll, 1);

    m_stack->addWidget(m_sidebar_widget);
}

void PerformanceOverlay::RefreshSidebarState() {
    if (m_btn_sidebar_docked) {
        const bool docked = (Settings::values.use_docked_mode.GetValue() == Settings::ConsoleMode::Docked);
        m_btn_sidebar_docked->setText(docked
            ? StormLang(QStringLiteral("Режим: В доке"), QStringLiteral("Mode: Docked"))
            : StormLang(QStringLiteral("Режим: Портативный"), QStringLiteral("Mode: Handheld")));
    }
    if (m_sidebar_resolution_combo) {
        const auto cur_res = Settings::values.resolution_setup.GetValue();
        const int idx = m_sidebar_resolution_combo->findData(static_cast<int>(cur_res));
        if (idx >= 0 && m_sidebar_resolution_combo->currentIndex() != idx) {
            const QSignalBlocker blocker(m_sidebar_resolution_combo);
            m_sidebar_resolution_combo->setCurrentIndex(idx);
        }
    }
    if (m_sidebar_scaling_combo) {
        const auto sf = Settings::values.scaling_filter.GetValue();
        const int idx = m_sidebar_scaling_combo->findData(static_cast<int>(sf));
        if (idx >= 0 && m_sidebar_scaling_combo->currentIndex() != idx) {
            const QSignalBlocker blocker(m_sidebar_scaling_combo);
            m_sidebar_scaling_combo->setCurrentIndex(idx);
        }
    }
    if (m_btn_sidebar_speed_limit) {
        const bool limit = Settings::values.use_speed_limit.GetValue();
        m_btn_sidebar_speed_limit->setText(limit
            ? StormLang(QStringLiteral("Лимит: 100%"), QStringLiteral("Limit: 100%"))
            : StormLang(QStringLiteral("Лимит: Выкл"), QStringLiteral("Limit: Off")));
    }
    if (m_btn_sidebar_vsync) {
        const auto vsync = Settings::values.vsync_mode.GetValue();
        const bool on = (vsync == Settings::VSyncMode::Fifo || vsync == Settings::VSyncMode::FifoRelaxed);
        m_btn_sidebar_vsync->setText(on
            ? StormLang(QStringLiteral("VSync: Вкл"), QStringLiteral("VSync: On"))
            : StormLang(QStringLiteral("VSync: Выкл"), QStringLiteral("VSync: Off")));
    }
    if (m_sidebar_volume_label) {
        const int vol = static_cast<int>(Settings::values.volume.GetValue());
        m_sidebar_volume_label->setText(StormLang(
            QStringLiteral("Громкость: %1%").arg(vol),
            QStringLiteral("Volume: %1%").arg(vol)));
    }
    if (m_sidebar_game_title && QtCommon::system && QtCommon::system->IsPoweredOn()) {
        const u64 tid = QtCommon::system->GetApplicationProcessProgramID();
        if (tid != 0) {
            m_sidebar_game_title->setText(QStringLiteral("ID: %1").arg(StormFormat::FormatTitleId(tid)));
        } else {
            m_sidebar_game_title->setText(StormLang(QStringLiteral("Сессия эмулятора"), QStringLiteral("Emulation Session")));
        }
    }
}

void PerformanceOverlay::OnSidebarSaveClicked() {
    if (!m_mainWindow) return;
    const int slot = m_sidebar_slot_combo ? (m_sidebar_slot_combo->currentIndex() + 1) : 1;
    m_mainWindow->OnSaveStateSlot(slot);
    if (m_sidebar_save_status) {
        m_sidebar_save_status->setText(StormLang(
            QStringLiteral("Слот %1 сохранен").arg(slot),
            QStringLiteral("Slot %1 saved").arg(slot)));
    }
}

void PerformanceOverlay::OnSidebarLoadClicked() {
    if (!m_mainWindow) return;
    const int slot = m_sidebar_slot_combo ? (m_sidebar_slot_combo->currentIndex() + 1) : 1;
    m_mainWindow->OnLoadStateSlot(slot);
    if (m_sidebar_save_status) {
        m_sidebar_save_status->setText(StormLang(
            QStringLiteral("Слот %1 загружен").arg(slot),
            QStringLiteral("Slot %1 loaded").arg(slot)));
    }
}

void PerformanceOverlay::OnSidebarSaveManagerClicked() {
    if (!m_mainWindow) return;
    m_mainWindow->OnSaveStateManager();
}

void PerformanceOverlay::OnSidebarResolutionChanged(int idx) {
    if (idx < 0 || !m_sidebar_resolution_combo) return;
    const auto res = static_cast<Settings::ResolutionSetup>(m_sidebar_resolution_combo->itemData(idx).toInt());
    Settings::values.resolution_setup.SetValue(res);
    if (m_mainWindow) {
        m_mainWindow->OnDynamicSettingChangedFromUI();
    }
}

void PerformanceOverlay::OnSidebarScalingChanged(int idx) {
    if (idx < 0 || !m_sidebar_scaling_combo) return;
    const auto filter = static_cast<Settings::ScalingFilter>(m_sidebar_scaling_combo->itemData(idx).toInt());
    Settings::values.scaling_filter.SetValue(filter);
    if (m_mainWindow) {
        m_mainWindow->OnDynamicSettingChangedFromUI();
    }
}

void PerformanceOverlay::OnSidebarSpeedLimitToggled() {
    const bool current = Settings::values.use_speed_limit.GetValue();
    Settings::values.use_speed_limit.SetValue(!current);
    if (m_mainWindow) {
        m_mainWindow->OnDynamicSettingChangedFromUI();
    }
    RefreshSidebarState();
}

void PerformanceOverlay::OnSidebarVsyncToggled() {
    const auto current = Settings::values.vsync_mode.GetValue();
    const auto next_mode = (current == Settings::VSyncMode::Fifo || current == Settings::VSyncMode::FifoRelaxed)
                               ? Settings::VSyncMode::Mailbox
                               : Settings::VSyncMode::Fifo;
    Settings::values.vsync_mode.SetValue(next_mode);
    if (m_mainWindow) {
        m_mainWindow->OnDynamicSettingChangedFromUI();
    }
    RefreshSidebarState();
}

void PerformanceOverlay::OnSidebarDockedToggled() {
    if (!m_mainWindow) return;
    m_mainWindow->OnToggleDockedMode();
    RefreshSidebarState();
}

void PerformanceOverlay::OnSidebarVolumeChanged(int val) {
    Settings::values.volume.SetValue(static_cast<u8>(val));
    if (m_sidebar_volume_label) {
        m_sidebar_volume_label->setText(StormLang(
            QStringLiteral("Громкость: %1%").arg(val),
            QStringLiteral("Volume: %1%").arg(val)));
    }
    if (m_mainWindow) {
        m_mainWindow->OnDynamicSettingChangedFromUI();
    }
}

void PerformanceOverlay::OnSidebarHudToggled() {
    SetMode(OverlayMode::Compact);
}

void PerformanceOverlay::OnSidebarCheatsClicked() {
    if (!m_mainWindow) return;
    m_mainWindow->OnCheatsDialog();
    if (QtCommon::system && QtCommon::system->IsPoweredOn()) {
        RefreshCheats();
    }
}

void PerformanceOverlay::OnSidebarAmiiboBrowseClicked() {
    if (!m_mainWindow) return;
    m_mainWindow->OnAmiiboOnlineDatabase();
}

void PerformanceOverlay::OnSidebarAmiiboEjectClicked() {
    if (!m_mainWindow) return;
    m_mainWindow->EjectAmiibo();
}

void PerformanceOverlay::OnSidebarPauseClicked() {
    if (!m_mainWindow) return;
    m_mainWindow->OnPauseContinueGame();
    RefreshSidebarState();
}

void PerformanceOverlay::OnSidebarRestartClicked() {
    if (!m_mainWindow) return;
    m_mainWindow->OnRestartGame();
}

void PerformanceOverlay::OnSidebarConfigureClicked() {
    if (!m_mainWindow) return;
    m_mainWindow->OnConfigure();
}

void PerformanceOverlay::OnSidebarStopClicked() {
    if (!m_mainWindow) return;
    m_mainWindow->OnStopGame();
    SetMode(OverlayMode::Compact);
    hide();
}

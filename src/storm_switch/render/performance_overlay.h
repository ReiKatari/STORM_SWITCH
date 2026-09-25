// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <deque>
#include <memory>
#include <vector>
#include <QPoint>
#include <QWidget>

namespace VideoCore {
class ShaderNotify;
}
namespace Core {
struct PerfStatsResults;
}

class MainWindow;
class FpsGraphWidget;
class QTabWidget;
class QLabel;
class QPushButton;
class QListWidget;
class QComboBox;
class QCheckBox;
class QStackedWidget;

class PerformanceOverlay : public QWidget {
    Q_OBJECT

public:
    explicit PerformanceOverlay(MainWindow* parent = nullptr);
    ~PerformanceOverlay() override;

    void SetExpanded(bool expanded);
    bool IsExpanded() const { return m_is_expanded; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void OnToggleExpand();
    void OnCheatToggled(int row);
    void OnReloadCheats();
    void OnSaveSlot(int slot);
    void OnLoadSlot(int slot);
    void OnInjectAmiibo();
    void OnEjectAmiibo();
    void OnBrowseAmiibo();
    void OnToggleDocked();
    void OnSpeedLimitChanged(int index);
    void OnDpsToggled(bool checked);
    void OnToggleGpuAccuracy();

private:
    void SetupUI();
    void SetupMonitoringTab(QWidget* tab);
    void SetupCheatsTab(QWidget* tab);
    void SetupSaveStatesTab(QWidget* tab);
    void SetupAmiiboTab(QWidget* tab);
    void SetupCfwTab(QWidget* tab);

    void resetPosition(const QPoint& pos);
    void updateStats(const Core::PerfStatsResults& results, const VideoCore::ShaderNotify& shaders);

    void RefreshCheats();
    void RefreshSaveSlots();
    void RefreshAmiiboList();
    void RefreshCfwInfo();

    MainWindow* m_mainWindow = nullptr;
    bool m_is_expanded = false;

    // Layout components
    QStackedWidget* m_stack = nullptr;

    // Compact Mode UI
    QWidget* m_compact_widget = nullptr;
    QLabel* m_compact_fps = nullptr;
    QLabel* m_compact_ft = nullptr;
    QLabel* m_compact_dps = nullptr;
    FpsGraphWidget* m_compact_fpsGraph = nullptr;

    // Expanded Mode UI (Tesla OSD)
    QWidget* m_expanded_widget = nullptr;
    QLabel* m_game_title_lbl = nullptr;
    QTabWidget* m_tabs = nullptr;

    // Monitoring tab
    QLabel* m_lbl_fps = nullptr;
    QLabel* m_lbl_fps_min = nullptr;
    QLabel* m_lbl_fps_max = nullptr;
    QLabel* m_lbl_fps_avg = nullptr;
    QLabel* m_lbl_ft = nullptr;
    QLabel* m_lbl_ft_min = nullptr;
    QLabel* m_lbl_ft_max = nullptr;
    QLabel* m_lbl_ft_avg = nullptr;
    QLabel* m_lbl_dps = nullptr;
    QLabel* m_lbl_res = nullptr;
    FpsGraphWidget* m_fpsGraph = nullptr;

    // Cheats tab
    QListWidget* m_cheat_list = nullptr;
    QLabel* m_cheat_status = nullptr;
    struct CheatEntryUI {
        QString name;
        QString code;
        bool enabled;
    };
    std::vector<CheatEntryUI> m_cheats;

    // Save states tab
    struct SaveSlotUI {
        QLabel* label = nullptr;
        QPushButton* btn_save = nullptr;
        QPushButton* btn_load = nullptr;
    };
    std::array<SaveSlotUI, 5> m_slots{};
    QLabel* m_save_status = nullptr;

    // Amiibo tab
    QComboBox* m_amiibo_combo = nullptr;
    QPushButton* m_btn_browse_amiibo = nullptr;
    QPushButton* m_btn_inject_amiibo = nullptr;
    QPushButton* m_btn_eject_amiibo = nullptr;
    QLabel* m_amiibo_status = nullptr;

    // CFW Tweaks tab
    QLabel* m_docked_status = nullptr;
    QPushButton* m_btn_toggle_docked = nullptr;
    QComboBox* m_speed_limit_combo = nullptr;
    QCheckBox* m_chk_dps = nullptr;
    QPushButton* m_btn_gpu_acc = nullptr;

    // Dragging
    QPoint m_offset{25, 75};
    QPoint m_drag_start_pos;

    // Graph samples
    static constexpr size_t NUM_FRAMETIME_SAMPLES = 300;
    std::deque<double> m_frametimeSamples;
    static constexpr size_t NUM_FPS_SAMPLES = 120;
    std::deque<double> m_fpsSamples;

signals:
    void closed();
};

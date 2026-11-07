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

enum class OverlayVisualPreset {
    StormAuroraGlass3D = 0,
    Switch2ProMagnetic = 1,
    CyberpunkEdgeNeon = 2,
    OledGhostWireframe = 3,
    TitaniumMechaTactical = 4,
    HyruleRelicGold = 5,
    SanctuaryBloodRune = 6,
    ArcadeCandyTactile3D = 7,
    LiquidQuartzFrost = 8,
    RetroPixelMatrix = 9,
};

class PerformanceOverlay : public QWidget {
    Q_OBJECT

public:
    explicit PerformanceOverlay(MainWindow* parent = nullptr);
    ~PerformanceOverlay() override;

    void SetExpanded(bool expanded);
    bool IsExpanded() const { return m_is_expanded; }

    void SetVisualPreset(OverlayVisualPreset preset);
    OverlayVisualPreset GetVisualPreset() const { return m_visual_preset; }
    void ApplyVisualPreset();

    void OnSaveSlot(int slot);
    void OnLoadSlot(int slot);
    void RefreshSaveSlots();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void OnToggleExpand();
    void OnCheatToggled(int row);
    void OnReloadCheats();
    void OnInjectAmiibo();
    void OnEjectAmiibo();
    void OnBrowseAmiibo();
    void OnToggleDocked();
    void OnSpeedLimitChanged(int index);
    void OnAspectRatioChanged(int index);
    void OnDpsToggled(bool checked);
    void OnAutoAspectPatchToggled(bool checked);
    void OnAuto60FpsToggled(bool checked);
    void OnLsfgToggled(bool checked);
    void OnHdr10Toggled(bool checked);
    void OnToggleGpuAccuracy();
    void OnAutoHealGraphicsArtifacts();
    void OnQuickRewind();
    void OnQuickFastForward();
    void OnQuickFrameStep();
    void OnQuickSaveAction();
    void OnQuickLoadAction();
    void OnQuickAiTranslate();
    void OnQuickShaderFix();
    void OnQuickTurbo();
    void OnQuickPauseCooling();
    void OnStyleComboChanged(int index);

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
    void RefreshAmiiboList();
    void RefreshCfwInfo();

    MainWindow* m_mainWindow = nullptr;
    OverlayVisualPreset m_visual_preset = OverlayVisualPreset::StormAuroraGlass3D;

    // Quick actions panel in Tesla OSD
    QWidget* m_quick_actions_panel = nullptr;
    QPushButton* m_btn_rewind = nullptr;
    QPushButton* m_btn_fast_forward = nullptr;
    QPushButton* m_btn_frame_step = nullptr;
    QPushButton* m_btn_quick_save = nullptr;
    QPushButton* m_btn_quick_load = nullptr;
    QPushButton* m_btn_ai_translate = nullptr;
    QPushButton* m_btn_shader_fix = nullptr;
    QPushButton* m_btn_turbo = nullptr;
    QPushButton* m_btn_pause_cooling = nullptr;
    QComboBox* m_style_combo = nullptr;
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
    QLabel* m_lbl_fps_1low = nullptr;
    QLabel* m_lbl_ft = nullptr;
    QLabel* m_lbl_ft_min = nullptr;
    QLabel* m_lbl_ft_max = nullptr;
    QLabel* m_lbl_ft_avg = nullptr;
    QLabel* m_lbl_dps = nullptr;
    QLabel* m_lbl_res = nullptr;
    QLabel* m_lbl_lsfg = nullptr;
    QLabel* m_lbl_shaders = nullptr;
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
    QComboBox* m_aspect_combo = nullptr;
    QCheckBox* m_chk_dps = nullptr;
    QCheckBox* m_chk_auto_aspect = nullptr;
    QCheckBox* m_chk_60fps_unlock = nullptr;
    QCheckBox* m_chk_lsfg = nullptr;
    QCheckBox* m_chk_hdr10 = nullptr;
    QPushButton* m_btn_gpu_acc = nullptr;
    QPushButton* m_btn_heal_gpu = nullptr;

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

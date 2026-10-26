// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// Static Qt on macOS doesn't use Vulkan
// Other platforms do, and thus have conflicting VulkanMemoryAllocator symbols
#if defined(QT_STATICPLUGIN) && !defined(__APPLE__)
#undef VMA_IMPLEMENTATION
#endif

#include <fstream>
#include <QRegularExpression>
#include <boost/algorithm/string/split.hpp>
#include "common/cityhash.h"
#include "common/fs/path_util.h"
#include "common/settings.h"
#include "common/settings_enums.h"
#include "frontend_common/settings_generator.h"
#include "render/performance_overlay.h"
#include "core/save_state.h"
#include "updater/update_dialog.h"

#include <QDesktopServices>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QTextEdit>
#include <QPlainTextEdit>
#include "common/fs/ryujinx_compat.h"
#include "main_window.h"
#include "network/network.h"
#include "qt_common/discord/discord.h"
#include "qt_common/titledb.h"
#include "qt_common/game_list/game_list_p.h"
#include "ui_main.h"

// Other Yuzu stuff //
#include "debugger/console.h"
#include "debugger/controller.h"

#include "about_dialog.h"
#include "amiibo_browser_dialog.h"
#include "cheats_dialog.h"
#include "mod_manager_dialog.h"
#include "crash_report_dialog.h"
#include "nand_manager_dialog.h"
#include "data_dialog.h"
#include "deps_dialog.h"
#include "install_dialog.h"
#include "storm_switch/storm_games_world_dialog.h"
#include "storm_switch/configuration/retroarch_shader_catalog.h"
#include "storm_switch/storm_save_sync_dialog.h"
#include "storm_switch/log_viewer_dialog.h"
#include "translator/floating_translate_button.h"
#include "translator/game_translator.h"
#include "in_game_notification.h"
#include "online_tools_dialog.h"

#include "bootmanager.h"
#include "loading_screen.h"
#include "qt_common/util/vk.h"
#include "ryujinx_dialog.h"
#include "set_play_time_dialog.h"
#include "util/util.h"
#include "storm_switch/game/game_list.h"

#include "applets/qt_amiibo_settings.h"
#include "applets/qt_controller.h"
#include "applets/qt_error.h"
#include "applets/qt_profile_select.h"
#include "applets/qt_software_keyboard.h"
#include "applets/qt_web_browser.h"

#include "configuration/configure_dialog.h"
#include "configuration/shared_widget.h"
#include "configuration/configure_input.h"
#include "configuration/configure_per_game.h"
#include "configuration/configure_tas.h"

#include "util/clickable_label.h"
#include "util/controller_navigation.h"
#include "util/overlay_dialog.h"

#include "multiplayer/state.h"
#include "core/hle/service/game_fix_database.h"

// Qt Stuff //
#define QT_NO_OPENGL

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QStyleHints>
#endif

#include <QAbstractScrollArea>
#include <QActionGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QInputDialog>
#include <QMimeData>
#include <QPalette>
#include <QProgressDialog>
#include <QScreen>
#include <QScrollArea>
#include <QShortcut>
#include <QStatusBar>
#include <QSystemTrayIcon>
#include <QTableWidget>
#include <QListWidget>
#include <QHeaderView>
#include <QtConcurrentRun>

// Qt Common //
#include "qt_common/config/shared_translation.h"
#include "qt_common/config/uisettings.h"

#include "qt_common/abstract/frontend.h"

#include "qt_common/qt_common.h"
#include "qt_common/qt_string_lookup.h"

#include "qt_common/util/content.h"
#include "qt_common/util/fs.h"
#include "qt_common/util/meta.h"
#include "qt_common/util/mod.h"
#include "qt_common/util/path.h"

#include "qt_common/render/emu_thread.h"

// These are wrappers to avoid the calls to CreateDirectory and CreateFile because of the Windows
// defines.
static FileSys::VirtualDir VfsFilesystemCreateDirectoryWrapper(const std::string& path,
                                                               FileSys::OpenMode mode) {
    return QtCommon::vfs->CreateDirectory(path, mode);
}

static FileSys::VirtualFile VfsDirectoryCreateFileWrapper(const FileSys::VirtualDir& dir,
                                                          const std::string& path) {
    return dir->CreateFile(path);
}

// Frontend //
#include "frontend_common/play_time_manager.h"

#ifdef ENABLE_UPDATE_CHECKER
#include "frontend_common/update_checker.h"
#endif

// Common //
#include "common/fs/fs.h"
#include "common/logging.h"
#include "common/memory_detect.h"
#include "common/scm_rev.h"
#include "common/scope_exit.h"
#include "common/string_util.h"

#ifdef ARCHITECTURE_x86_64
#include "common/cpu_features.h"
#endif

// Core //
#include "core/frontend/applets/general.h"
#include "core/frontend/applets/mii_edit.h"
#include "core/frontend/applets/software_keyboard.h"

#include "core/hle/kernel/k_process.h"
#include "core/hle/service/acc/profile_manager.h"
#include "core/hle/service/am/applet_manager.h"
#include "core/hle/service/am/frontend/applet_web_browser_types.h"

#include "core/file_sys/card_image.h"
#include "core/file_sys/romfs.h"
#include "core/file_sys/savedata_factory.h"

#include "core/tools/renderdoc.h"

#include "core/perf_stats.h"

#include "core/crypto/key_manager.h"

// Input //
#include "hid_core/frontend/emulated_controller.h"
#include "hid_core/hid_core.h"
#include "input_common/drivers/virtual_amiibo.h"

// Video Core //
#include "video_core/gpu.h"
#include "video_core/renderer_base.h"
#include "video_core/shader_notify.h"

#include <SDL3/SDL.h>

#include <boost/container/flat_set.hpp>

// Platform stuff //
#include <boost/container/flat_set.hpp>

#ifdef __APPLE__
#include <unistd.h> // for chdir
#endif

#ifdef __unix__

#include <csignal>
#include <QSocketNotifier>
#include <sys/socket.h>
#include "qt_common/gui_settings.h"

#endif

#include "qt_common/gamemode.h"

#ifdef _WIN32
#include "common/windows/timer_resolution.h"
#include "core/core_timing.h"

#include <QPlatformSurfaceEvent>
#include <QSettings>
#include <dwmapi.h>
#include <windows.h>
#ifdef _MSC_VER
#pragma comment(lib, "Dwmapi.lib")
#endif

static inline void ApplyWindowsTitleBarDarkMode(HWND hwnd, bool enabled) {
    if (!hwnd)
        return;
    BOOL val = enabled ? TRUE : FALSE;
    // 20 = Win11/21H2+
    if (SUCCEEDED(DwmSetWindowAttribute(hwnd, 20, &val, sizeof(val))))
        return;
    // 19 = pre-21H2
    DwmSetWindowAttribute(hwnd, 19, &val, sizeof(val));
}

static inline void ApplyDarkToTopLevel(QWidget* w, bool on) {
    if (!w || !w->isWindow())
        return;
    const auto wid = w->internalWinId();
    if (!wid)
        return;
    ApplyWindowsTitleBarDarkMode(reinterpret_cast<HWND>(wid), on);
}

#include <QFontMetrics>
#include <QGroupBox>

static inline void EnsureModalDialogScalableAndStyled(QWidget* w) {
    auto* dlg = qobject_cast<QDialog*>(w);
    if (!dlg || !dlg->isWindow()) {
        return;
    }
    if (dlg->inherits("InGameNotification") || dlg->inherits("PerformanceOverlay") ||
        dlg->inherits("FloatingTranslateButton")) {
        return;
    }

    // 1. Ensure unified STORM square checkbox/groupbox/radio indicator and QGroupBox::title styling
    const QString cur_qss = dlg->styleSheet();
    if (!cur_qss.isEmpty() && !cur_qss.contains(QStringLiteral("QCheckBox::indicator"))) {
        dlg->setStyleSheet(cur_qss + QStringLiteral(
            "\nQGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; top: 0px; left: 12px; padding: 0 6px; }"
            "\nQCheckBox::indicator, QGroupBox::indicator, QRadioButton::indicator, QTreeView::indicator, QListView::indicator, QTableView::indicator {"
            " width: 16px; height: 16px; border: 1px solid #475569; border-radius: 4px; background: #121826; image: none; }"
            "\nQCheckBox::indicator:hover, QGroupBox::indicator:hover, QRadioButton::indicator:hover, QTreeView::indicator:hover, QListView::indicator:hover, QTableView::indicator:hover {"
            " border: 1px solid #00D2FF; }"
            "\nQCheckBox::indicator:checked, QGroupBox::indicator:checked, QRadioButton::indicator:checked, QTreeView::indicator:checked, QListView::indicator:checked, QTableView::indicator:checked {"
            " background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #00F0FF, stop:1 #0088CC); border: 1px solid #00F0FF; image: none; }"
            "\nQCheckBox::indicator:unchecked, QGroupBox::indicator:unchecked, QRadioButton::indicator:unchecked, QTreeView::indicator:unchecked, QListView::indicator:unchecked, QTableView::indicator:unchecked {"
            " background: #121826; border: 1px solid #475569; image: none; }"));
    }

    QScreen* scr = dlg->screen();
    if (!scr && dlg->parentWidget()) {
        scr = dlg->parentWidget()->screen();
    }
    if (!scr) {
        scr = QGuiApplication::primaryScreen();
    }
    const QRect avail = scr ? scr->availableGeometry() : QRect(0, 0, 1920, 1040);
    const int max_w = std::max(480, avail.width() - 36);
    const int max_h = std::max(360, avail.height() - 56);

    for (auto* sa : dlg->findChildren<QScrollArea*>()) {
        sa->setWidgetResizable(true);
        sa->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
    }

    // 2. Ensure QGroupBox titles are never clipped horizontally
    for (auto* gb : dlg->findChildren<QGroupBox*>()) {
        if (!gb->title().isEmpty()) {
            QFont f = gb->font();
            f.setBold(true);
            const QFontMetrics fm(f);
            const int extra = gb->isCheckable() ? 58 : 38;
            const int needed = std::min(fm.horizontalAdvance(gb->title()) + extra, std::max(200, max_w - 60));
            if (gb->minimumWidth() < needed && gb->maximumWidth() >= needed) {
                gb->setMinimumWidth(needed);
            }
        }
    }

    // 3. Clamp minimumSize, maximumSize, and current size to available screen geometry
    const bool is_fixed = (dlg->minimumSize() == dlg->maximumSize()) && (dlg->minimumWidth() > 0);
    if (!is_fixed) {
        dlg->setSizeGripEnabled(true);
        if (dlg->minimumWidth() > max_w || dlg->minimumHeight() > max_h) {
            dlg->setMinimumSize(std::min(dlg->minimumWidth(), max_w),
                                std::min(dlg->minimumHeight(), max_h));
        }
        if (dlg->maximumHeight() > max_h || dlg->maximumWidth() > max_w) {
            dlg->setMaximumSize(std::min(dlg->maximumWidth(), max_w),
                                std::min(dlg->maximumHeight(), max_h));
        }
        if (!dlg->isMaximized() && !dlg->isFullScreen()) {
            int target_w = std::min(dlg->width(), max_w);
            int target_h = std::min(dlg->height(), max_h);
            const QString obj_name = dlg->objectName();
            if (obj_name == QStringLiteral("ConfigureDialog") ||
                obj_name == QStringLiteral("ConfigurePerGame")) {
                if (QWidget* main_win = dlg->parentWidget() ? dlg->parentWidget()->window() : nullptr) {
                    if (main_win->isVisible() && !main_win->isMinimized()) {
                        if (avail.height() <= 920) {
                            target_h = std::clamp(std::min(main_win->height() - 16, avail.height() - 68),
                                                  std::min(460, max_h), max_h);
                        } else {
                            target_h = std::clamp(main_win->height(), std::min(540, max_h), max_h);
                        }
                    }
                } else if (avail.height() <= 920) {
                    target_h = std::clamp(avail.height() - 68, std::min(460, max_h), max_h);
                }
            }
            if (target_w != dlg->width() || target_h != dlg->height()) {
                dlg->resize(target_w, target_h);
            }
            int nx = dlg->x();
            int ny = dlg->y();
            if (QWidget* main_win = dlg->parentWidget() ? dlg->parentWidget()->window() : nullptr) {
                if (main_win->isVisible() && !main_win->isMinimized() &&
                    (obj_name == QStringLiteral("ConfigureDialog") ||
                     obj_name == QStringLiteral("ConfigurePerGame"))) {
                    nx = main_win->x() + (main_win->width() - target_w) / 2;
                    ny = main_win->y() + (main_win->height() - target_h) / 2;
                }
            }
            const int frame_extra_h = std::max(32, dlg->frameGeometry().height() - dlg->height());
            const int frame_extra_w = std::max(8, dlg->frameGeometry().width() - dlg->width());
            if (nx + target_w + frame_extra_w > avail.right()) {
                nx = avail.right() - target_w - frame_extra_w - 4;
            }
            if (ny + target_h + frame_extra_h > avail.bottom()) {
                ny = avail.bottom() - target_h - frame_extra_h - 4;
            }
            if (nx < avail.left() + 4) {
                nx = avail.left() + 4;
            }
            if (ny < avail.top() + 4) {
                ny = avail.top() + 4;
            }
            if (nx != dlg->x() || ny != dlg->y()) {
                dlg->move(nx, ny);
            }
        }
    }
}

namespace {
struct TitlebarFilter final : QObject {
    bool dark;
    bool in_filter{false};
    explicit TitlebarFilter(bool is_dark) : QObject(qApp), dark(is_dark) {}

    void setDark(bool is_dark) {
        dark = is_dark;
    }

    void onFocusChanged(QWidget*, QWidget* now) {
        if (now && !in_filter) {
            in_filter = true;
            ApplyDarkToTopLevel(now->window(), dark);
            in_filter = false;
        }
    }

    bool eventFilter(QObject* obj, QEvent* ev) override {
        if (in_filter) {
            return QObject::eventFilter(obj, ev);
        }
        if (auto* w = qobject_cast<QWidget*>(obj)) {
            switch (ev->type()) {
            case QEvent::Show:
                in_filter = true;
                ApplyDarkToTopLevel(w, dark);
                EnsureModalDialogScalableAndStyled(w);
                in_filter = false;
                break;
            case QEvent::WindowStateChange:
                in_filter = true;
                ApplyDarkToTopLevel(w, dark);
                in_filter = false;
                break;
            default:
                break;
            }
        }
        return QObject::eventFilter(obj, ev);
    }
};

static TitlebarFilter* g_filter = nullptr;
static QMetaObject::Connection g_focusConn;

} // namespace

static void ApplyGlobalDarkTitlebar(bool is_dark) {
    static bool in_apply = false;
    if (in_apply)
        return;
    in_apply = true;
    if (!g_filter) {
        g_filter = new TitlebarFilter(is_dark);
        qApp->installEventFilter(g_filter);
        g_focusConn = QObject::connect(qApp, &QApplication::focusChanged, g_filter,
                                       &TitlebarFilter::onFocusChanged);
    } else {
        g_filter->setDark(is_dark);
    }
    for (QWidget* w : QApplication::topLevelWidgets()) {
        ApplyDarkToTopLevel(w, is_dark);
    }
    in_apply = false;
}

static void RemoveTitlebarFilter() {
    if (!g_filter)
        return;
    qApp->removeEventFilter(g_filter);
    QObject::disconnect(g_focusConn);
    delete g_filter;
    g_filter = nullptr;
}

#endif

#ifdef YUZU_CRASH_DUMPS
#include "storm_switch/breakpad.h"
#endif

using namespace Common::Literals;

#include "qt_common/discord/discord.h"

#ifdef USE_DISCORD_PRESENCE
#include "qt_common/discord/discord_impl.h"
#endif

#ifdef QT_STATICPLUGIN
#include <QtPlugin>

#if defined(_WIN32)
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
#elif defined(__APPLE__)
Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin)
#endif

#endif

#ifdef _WIN32
#include <windows.h>
extern "C" {
// tells Nvidia and AMD drivers to use the dedicated GPU by default on laptops with switchable
// graphics
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

constexpr int default_mouse_hide_timeout = 2500;
constexpr int default_input_update_timeout = 16;

constexpr size_t CopyBufferSize = 1_MiB;

/**
 * "Callouts" are one-time instructional messages shown to the user. In the config settings, there
 * is a bitfield "callout_flags" options, used to track if a message has already been shown to the
 * user. This is 32-bits - if we have more than 32 callouts, we should retire and recycle old ones.
 */
enum class CalloutFlag : uint32_t {
    DRDDeprecation = 0x2,
};

const int MainWindow::max_recent_files_item;

namespace {

constexpr std::array<std::pair<u32, const char*>, 5> default_game_icon_sizes{
    std::make_pair(0, QT_TRANSLATE_NOOP("MainWindow", "None")),
    std::make_pair(32, QT_TRANSLATE_NOOP("MainWindow", "Small (32x32)")),
    std::make_pair(64, QT_TRANSLATE_NOOP("MainWindow", "Standard (64x64)")),
    std::make_pair(128, QT_TRANSLATE_NOOP("MainWindow", "Large (128x128)")),
    std::make_pair(256, QT_TRANSLATE_NOOP("MainWindow", "Full Size (256x256)")),
};

QString GetTranslatedGameIconSize(size_t index) {
    return QCoreApplication::translate("MainWindow", default_game_icon_sizes[index].second);
}

bool GetActivePerGameConfigName(const QString& current_game_path,
                                const QString& last_filename_booted,
                                std::string& out_config_name) {
    if (!QtCommon::system || !QtCommon::system->IsPoweredOn()) {
        return false;
    }
    const u64 title_id = QtCommon::system->GetApplicationProcessProgramID();
    if (title_id == 0) {
        return false;
    }
    const QString& game_file = !current_game_path.isEmpty() ? current_game_path : last_filename_booted;
    const std::filesystem::path custom_path =
        Common::FS::GetEdenPath(Common::FS::EdenPath::ConfigDir) / "custom";
    if (!game_file.isEmpty()) {
        const auto utf8_str = game_file.toUtf8();
        const auto file_path_hash =
            Common::CityHash64(utf8_str.constData(), static_cast<std::size_t>(utf8_str.size()));
        const auto specific_config = fmt::format("{:016X}_{:016X}", title_id, file_path_hash);
        if (std::filesystem::exists(custom_path / (specific_config + ".ini"))) {
            out_config_name = specific_config;
            return true;
        }
    }
    const auto legacy_config = fmt::format("{:016X}", title_id);
    if (std::filesystem::exists(custom_path / (legacy_config + ".ini"))) {
        out_config_name = legacy_config;
        return true;
    }
    return false;
}

template <typename SettingType, typename ValueType, typename BackupPairType>
void ApplyStatusBarSwitchableSetting(SettingType& setting, const ValueType& value,
                                     bool session_backup_active, BackupPairType& backup_pair,
                                     const QString& current_game_path = {},
                                     const QString& last_filename_booted = {}) {
    std::string per_game_config_name;
    const bool has_per_game_file =
        GetActivePerGameConfigName(current_game_path, last_filename_booted, per_game_config_name);
    if (has_per_game_file || !setting.UsingGlobal()) {
        setting.SetGlobal(false);
        setting.SetValue(value);
    } else {
        setting.SetGlobal(true);
        setting.SetValue(value);
        if (session_backup_active) {
            backup_pair = {value, true};
        }
    }
}

template <typename SettingType, typename ValueType, typename SessionBackupType, typename BackupPairType>
    requires (!std::is_same_v<std::decay_t<SessionBackupType>, bool>)
void ApplyStatusBarSwitchableSetting(SettingType& setting, const ValueType& value,
                                     const SessionBackupType& session_backup, BackupPairType& backup_pair,
                                     const QString& current_game_path = {},
                                     const QString& last_filename_booted = {}) {
    ApplyStatusBarSwitchableSetting(setting, value, session_backup.is_active, backup_pair,
                                    current_game_path, last_filename_booted);
}

template <typename SettingType, typename ValueType>
void ApplyStatusBarSwitchableSettingNoBackup(SettingType& setting, const ValueType& value,
                                             const QString& current_game_path = {},
                                             const QString& last_filename_booted = {}) {
    std::string per_game_config_name;
    const bool has_per_game_file =
        GetActivePerGameConfigName(current_game_path, last_filename_booted, per_game_config_name);
    if (has_per_game_file || !setting.UsingGlobal()) {
        setting.SetGlobal(false);
        setting.SetValue(value);
    } else {
        setting.SetGlobal(true);
        setting.SetValue(value);
    }
}

} // namespace

#ifndef _WIN32
// TODO(crueter): carboxyl does this, is it needed in qml?
inline static bool isDarkMode() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const auto scheme = QGuiApplication::styleHints()->colorScheme();
    return scheme == Qt::ColorScheme::Dark;
#else
    const QPalette defaultPalette;
    const auto text = defaultPalette.color(QPalette::WindowText);
    const auto window = defaultPalette.color(QPalette::Window);
    return text.lightness() > window.lightness();
#endif // QT_VERSION
}
#endif // _WIN32

static MainWindow* s_main_window_instance = nullptr;

MainWindow* MainWindow::GetInstance() {
    return s_main_window_instance;
}

QString MainWindow::GetGameTitleByProgramId(u64 program_id) const {
    if (!game_list) {
        return QString();
    }
    auto* model = game_list->GetModel();
    if (!model) {
        return QString();
    }
    auto* root = model->invisibleRootItem();
    if (!root) {
        return QString();
    }

    std::function<QString(QStandardItem*)> searchItem = [&](QStandardItem* item) -> QString {
        if (!item) return QString();
        const auto prog_id_val = item->data(GameListItemPath::ProgramIdRole);
        if (prog_id_val.isValid() && prog_id_val.toULongLong() == program_id) {
            const QString title = item->data(GameListItemPath::TitleRole).toString().trimmed();
            if (!title.isEmpty()) {
                return title;
            }
        }
        for (int r = 0; r < item->rowCount(); ++r) {
            auto* child = item->child(r, 0);
            if (child) {
                const QString found = searchItem(child);
                if (!found.isEmpty()) {
                    return found;
                }
            }
        }
        return QString();
    };

    return searchItem(root);
}

MainWindow::MainWindow(bool has_broken_vulkan)
    : ui{std::make_unique<Ui::MainWindow>()},
      input_subsystem{std::make_shared<InputCommon::InputSubsystem>()}, user_data_migrator{this} {
    s_main_window_instance = this;
    ConfigurationShared::SetGlobalSettingChangeCallback([this]() {
        this->OnDynamicSettingChangedFromUI();
    });
    QtCommon::Init(this);

    Common::FS::CreateEdenPaths();
    std::error_code cfg_ec;
    const auto global_cfg_file = Common::FS::GetEdenPath(Common::FS::EdenPath::ConfigDir) / "qt-config.ini";
    const bool had_existing_config_file =
        std::filesystem::exists(global_cfg_file, cfg_ec) &&
        std::filesystem::file_size(global_cfg_file, cfg_ec) > 64;
    this->config = std::make_unique<QtConfig>();

    // Initialize core emulation settings to Zero-Regression Baseline on fresh install, preserving existing user settings on upgrade
    static constexpr std::string_view CURRENT_BUILD_VERSION = "12.7.2";
    if (UISettings::values.config_version.GetValue().empty() && !had_existing_config_file) {
        LOG_INFO(Frontend, "Fresh configuration detected. Initializing core emulation settings to Zero-Regression Baseline (version '{}')...",
                 CURRENT_BUILD_VERSION);

        Settings::values.gpu_accuracy.SetValue(Settings::GpuAccuracy::High);
        Settings::values.astc_recompression.SetValue(Settings::AstcRecompression::Bc1);
        Settings::values.accelerate_astc.SetValue(Settings::AstcDecodeMode::Gpu);
        Settings::values.nvdec_emulation.SetValue(Settings::NvdecEmulation::Gpu);
        Settings::values.use_asynchronous_shaders.SetValue(true);
        Settings::values.use_asynchronous_gpu_emulation.SetValue(true);
        Settings::values.async_presentation.SetValue(true);
        Settings::values.eco_thermal_mode.SetValue(false);
        Settings::values.eco_frame_pacing.SetValue(true);
        Settings::values.smart_shader_throttle.SetValue(false);
        Settings::values.cpu_affinity_pinning.SetValue(true);
        Settings::values.use_vulkan_driver_pipeline_cache.SetValue(true);
        Settings::values.vram_garbage_collection.SetValue(true);
        Settings::values.early_release_fences.SetValue(false);
        Settings::values.use_reactive_flushing.SetValue(false);
        Settings::values.sync_memory_operations.SetValue(false);
        Settings::values.enable_gpu_buffer_readback.SetValue(false);
        Settings::values.dma_accuracy.SetValue(Settings::DmaAccuracy::Default);
        Settings::values.gpu_fence_behavior.SetValue(Settings::GpuFenceBehavior::Default);
        Settings::values.barrier_feedback_loops.SetValue(true);
        Settings::values.drs_resolution_lock.SetValue(false);
        Settings::values.vram_usage_mode.SetValue(Settings::VramUsageMode::Normal);
        Settings::values.vram_budget_governor.SetValue(false);
        Settings::values.storm_lowend_turbo.SetValue(false);
        Settings::values.storm_thermal_governor.SetValue(true);
        Settings::values.cpuopt_ignore_memory_aborts.SetValue(true);
        Settings::values.cpuopt_fastmem.SetValue(true);
        Settings::values.skip_cpu_inner_invalidation.SetValue(false);
        Settings::values.renderer_force_max_clock.SetValue(false);

        UISettings::values.config_version.SetValue(std::string(CURRENT_BUILD_VERSION));
        config->SaveAllValues();
    } else if (UISettings::values.config_version.GetValue() != CURRENT_BUILD_VERSION) {
        UISettings::values.config_version.SetValue(std::string(CURRENT_BUILD_VERSION));
        config->SaveAllValues();
    }

    if (user_data_migrator.migrated) {
        // Sort-of hack whereby we only move the old dir if it's a subfolder of the user dir

        using namespace Common::FS;

        static constexpr const std::array<const EdenPath, 4> paths = {
            EdenPath::NANDDir, EdenPath::SDMCDir, EdenPath::DumpDir, EdenPath::LoadDir};

        for (const EdenPath& path : paths) {
            std::string str_path = Common::FS::GetEdenPathString(path);
            if (str_path.starts_with(user_data_migrator.selected_emu.get_user_dir())) {
                boost::replace_all(
                    str_path, user_data_migrator.selected_emu.lower_name().toStdString(), "storm_switch");
                Common::FS::SetEdenPath(path, str_path);
            }
        }
    }

#ifdef __unix__
    SetupSigInterrupts();
#endif

    // Multi-NAND Profile support: apply active NAND profile if configured
    const std::string active_nand_p = GetActiveNandProfileName();
    if (!active_nand_p.empty() && active_nand_p != "Default") {
        const auto custom_nand_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::EdenDir) / "nand_profiles" / active_nand_p;
        if (std::filesystem::exists(custom_nand_dir)) {
            Common::FS::SetEdenPath(Common::FS::EdenPath::NANDDir, custom_nand_dir);
            LOG_INFO(Frontend, "Using custom active NAND profile: '{}' at '{}'", active_nand_p, custom_nand_dir.string());
        }
    }

    SetGamemodeEnabled(UISettings::values.enable_gamemode.GetValue());

    UISettings::RestoreWindowState(config);

    LoadTranslation();
    FrontendCommon::GenerateSettings();

    setAcceptDrops(true);
    ui->setupUi(this);
    statusBar()->hide();

    startup_icon_theme = QIcon::themeName();
    // fallback can only be set once, colorful theme icons are okay on both light/dark
    QIcon::setFallbackThemeName(QStringLiteral("colorful"));
    QIcon::setFallbackSearchPaths(QStringList(QStringLiteral(":/icons")));

    default_theme_paths = QIcon::themeSearchPaths();
    UpdateUITheme();

    SetDiscordEnabled(UISettings::values.enable_discord_presence.GetValue());
    discord_rpc->Update();

    play_time_manager = std::make_unique<PlayTime::PlayTimeManager>();
    in_game_notification = new InGameNotificationOverlay(this);

    InitializeWidgets();
    InitializeDebugWidgets();
    InitializeRecentFileMenuActions();
    SetupMenuIcons();
    InitializeHotkeys();

    SetDefaultUIGeometry();
    RestoreUIState();

    ConnectMenuEvents();
    ConnectWidgetEvents();

    QtCommon::SetupHID();
    controller_dialog->refreshConfiguration();

    UpdateWindowTitle();

    show();

#ifdef ENABLE_UPDATE_CHECKER
    if (UISettings::values.check_for_updates) {
        update_future = QtConcurrent::run(
            []() -> std::optional<Common::Net::Release> { return UpdateChecker::GetUpdate(); });
        update_watcher.connect(&update_watcher, &QFutureWatcher<QString>::finished, this,
                               &MainWindow::OnEmulatorUpdateAvailable);
        update_watcher.setFuture(update_future);
    }
#endif

    // Setup content providers.
    QtCommon::SetupContentProviders();

    // Gen keys if necessary
    OnCheckFirmwareDecryption();

#ifdef __unix__
    OnCheckGraphicsBackend();
#endif

    // Check for orphaned profiles and reset profile data if necessary
    QtCommon::Content::FixProfiles();

    if (Settings::values.use_dev_keys.GetValue()) {
        Core::Crypto::KeyManager::Instance().ReloadKeys();
    }
    game_list->PopulateAsync(UISettings::values.game_dirs);
    QTimer::singleShot(2500, this, &MainWindow::StartSilentCheatsSync);

    // Set up game list mode checkboxes.
    SetGameListMode(UISettings::values.game_list_mode.GetValue());

    // make sure menubar has the arrow cursor instead of inheriting from this
    ui->menubar->setCursor(QCursor());
    statusBar()->setCursor(QCursor());

    mouse_hide_timer.setInterval(default_mouse_hide_timeout);
    connect(&mouse_hide_timer, &QTimer::timeout, this, &MainWindow::HideMouseCursor);
    connect(ui->menubar, &QMenuBar::hovered, this, &MainWindow::ShowMouseCursor);

    update_input_timer.setInterval(default_input_update_timeout);
    connect(&update_input_timer, &QTimer::timeout, this, &MainWindow::UpdateInputDrivers);
    update_input_timer.start();

    if (has_broken_vulkan) {
        UISettings::values.has_broken_vulkan = true;

        QMessageBox::warning(this, tr("Broken Vulkan Installation Detected"),
                             tr("Vulkan initialization failed during boot."));
#ifdef HAS_OPENGL
        Settings::values.renderer_backend = Settings::RendererBackend::OpenGL_GLSL;
#else
        Settings::values.renderer_backend = Settings::RendererBackend::Null;
#endif

        UpdateAPIText();
        renderer_status_button->setDisabled(true);
        renderer_status_button->setChecked(false);
    } else {
        VkDeviceInfo::PopulateRecords(vk_device_records, this->window()->windowHandle());
    }

#if !defined(_WIN32)
    SDL_InitSubSystem(SDL_INIT_VIDEO);

    // Set a screensaver inhibition reason string. Currently passed to DBus by SDL and visible to
    // the user through their desktop environment.
    //: TRANSLATORS: This string is shown to the user to explain why yuzu needs to prevent the
    //: computer from sleeping
    QByteArray wakelock_reason = tr("Running a game").toUtf8();
    SDL_SetHint(SDL_HINT_SCREENSAVER_INHIBIT_ACTIVITY_NAME, wakelock_reason.data());

    // SDL disables the screen saver by default, and setting the hint
    // SDL_HINT_VIDEO_ALLOW_SCREENSAVER doesn't seem to work, so we just enable the screen saver
    // for now.
    SDL_EnableScreenSaver();
#endif

    SetupPrepareForSleep();

    // Pre-load TitleDB database in background
    TitleDB::TitleDatabase::Instance().EnsureLoaded();

    // Some moron added a race condition to the status bar
    // so now we have to make this completely unnecessary call
    // to prevent the UI from blowing up.
    UpdateUITheme();
    UpdateStatusButtons();
    InitializeSystemTray();

    // Ensure all explicit StormLang translations are applied immediately at startup
    OnLanguageChanged(QString::fromStdString(UISettings::values.language.GetValue()));

    QTimer::singleShot(2500, this, [this] {
        OnCheckUpdates(false);
    });

    QStringList args = QApplication::arguments();
    LOG_INFO(Frontend, "Processing command line args (count={}): {}", args.size(), args.join(QStringLiteral(" | ")).toStdString());

    if (args.size() < 2) {
        return;
    }

    QString game_path;
    bool should_launch_qlaunch = false;
    bool should_launch_hlaunch = false;
    bool should_launch_setup = false;
    bool has_gamepath = false;
    bool is_fullscreen = false;

    auto is_known_option = [](const QString& arg) {
        return arg == QStringLiteral("-f") ||
               arg == QStringLiteral("-u") ||
               arg == QStringLiteral("-g") ||
               arg == QStringLiteral("-input-profile") ||
               arg == QStringLiteral("-qlaunch") ||
               arg == QStringLiteral("-hlaunch") ||
               arg == QStringLiteral("-setup") ||
               arg.startsWith(QStringLiteral("--"));
    };

    // Preserves drag/drop functionality
    for (int i = 1; i < args.size(); ++i) {
        if (args[i] == QStringLiteral("-f")) {
            // Launch game in fullscreen mode
            is_fullscreen = true;
        } else if (args[i] == QStringLiteral("-u") && i < args.size() - 1) {
            // Launch game with a specific user
            int user_arg_idx = ++i;
            bool argument_ok;
            std::size_t selected_user = args[user_arg_idx].toUInt(&argument_ok);
            if (!argument_ok) {
                // try to look it up by username, only finds the first username that matches.
                std::string const user_arg_str = args[user_arg_idx].toStdString();
                auto const user_idx =
                    QtCommon::system->GetProfileManager().GetUserIndex(user_arg_str);
                if (user_idx != std::nullopt) {
                    selected_user = user_idx.value();
                } else {
                    LOG_ERROR(Frontend, "Invalid user argument '{}'", user_arg_str);
                    continue;
                }
            }
            if (QtCommon::system->GetProfileManager().UserExistsIndex(selected_user)) {
                Settings::values.current_user = s32(selected_user);
                user_flag_cmd_line = true;
            } else {
                LOG_ERROR(Frontend, "Selected user {} doesn't exist", selected_user);
            }
        } else if (args[i] == QStringLiteral("-g") && i < args.size() - 1) {
            // Launch game at path
            game_path = args[++i];
            while (i + 1 < args.size() && !is_known_option(args[i + 1])) {
                game_path += QStringLiteral(" ") + args[++i];
            }
            game_path.remove(QLatin1Char('\"'));
            has_gamepath = true;
        } else if (args[i] == QStringLiteral("-input-profile") && i < args.size() - 1) {
            auto& players = Settings::values.players.GetValue();
            players[0].profile_name = args[++i].toStdString();
        } else if (args[i] == QStringLiteral("-qlaunch")) {
            should_launch_qlaunch = true;
        } else if (args[i] == QStringLiteral("-hlaunch")) {
            should_launch_hlaunch = true;
        } else if (args[i] == QStringLiteral("-setup")) {
            should_launch_setup = true;
        } else if (!is_known_option(args[i])) {
            if (game_path.isEmpty()) {
                game_path = args[i];
            } else {
                game_path += QStringLiteral(" ") + args[i];
            }
            game_path.remove(QLatin1Char('\"'));
            has_gamepath = true;
        }
    }

    // Override fullscreen setting if gamepath or argument is provided
    if (has_gamepath || is_fullscreen) {
        ui->action_Fullscreen->setChecked(is_fullscreen);
    }

    if (should_launch_setup) {
        LaunchFirmwareApplet(u64(Service::AM::AppletProgramId::Starter), std::nullopt);
    } else {
        if (!game_path.isEmpty()) {
            m_is_cmd_line_launch = true;
            LOG_INFO(Frontend, "Identified game path to boot: '{}'", game_path.toStdString());
            BootGame(game_path, ApplicationAppletParameters());
        } else if (should_launch_qlaunch) {
            LaunchFirmwareApplet(u64(Service::AM::AppletProgramId::QLaunch), std::nullopt);
        } else if (should_launch_hlaunch) {
            std::filesystem::path const sd_dir =
                Common::FS::GetEdenPathString(Common::FS::EdenPath::SDMCDir);
            auto const hbl_path = (sd_dir / "atmosphere" / "hbl.nsp").string();
            BootGame(
                QString::fromStdString(hbl_path),
                LibraryAppletParameters(0x010000000000100Dull, Service::AM::AppletId::QLaunch));
        }
    }
}

MainWindow::~MainWindow() {
    if (game_list) {
        game_list->CancelPopulate();
    }
    ConfigurationShared::SetGlobalSettingChangeCallback(nullptr);
    if (s_main_window_instance == this) {
        s_main_window_instance = nullptr;
    }
    // will get automatically deleted otherwise
    if (render_window->parent() == nullptr) {
        delete render_window;
    }

#ifdef _WIN32
    RemoveTitlebarFilter();
#endif
#ifdef __unix__
    ::close(sig_interrupt_fds[0]);
    ::close(sig_interrupt_fds[1]);
#endif
}

void MainWindow::AmiiboSettingsShowDialog(const Core::Frontend::CabinetParameters& parameters,
                                          std::shared_ptr<Service::NFC::NfcDevice> nfp_device) {
    cabinet_applet =
        new QtAmiiboSettingsDialog(this, parameters, input_subsystem.get(), nfp_device);
    SCOPE_EXIT {
        cabinet_applet->deleteLater();
        cabinet_applet = nullptr;
    };

    cabinet_applet->setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint |
                                   Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
    cabinet_applet->setWindowModality(Qt::WindowModal);

    if (cabinet_applet->exec() == QDialog::Rejected) {
        emit AmiiboSettingsFinished(false, {});
        return;
    }

    emit AmiiboSettingsFinished(true, cabinet_applet->GetName());
}

void MainWindow::AmiiboSettingsRequestExit() {
    if (cabinet_applet) {
        cabinet_applet->reject();
    }
}

void MainWindow::ControllerSelectorReconfigureControllers(
    const Core::Frontend::ControllerParameters& parameters) {
    const bool was_fullscreen = isFullScreen();
    if (was_fullscreen) {
        showNormal();
    }
    controller_applet =
        new QtControllerSelectorDialog(this, parameters, input_subsystem.get(), *QtCommon::system);
    SCOPE_EXIT {
        controller_applet->deleteLater();
        controller_applet = nullptr;
        if (was_fullscreen) {
            showFullScreen();
        }
    };

    controller_applet->setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint |
                                      Qt::WindowStaysOnTopHint | Qt::WindowTitleHint |
                                      Qt::WindowSystemMenuHint);
    controller_applet->setWindowModality(Qt::WindowModal);
    bool is_success = controller_applet->exec() != QDialog::Rejected;

    // Don't forget to apply settings.
    QtCommon::system->HIDCore().DisableAllControllerConfiguration();
    QtCommon::system->ApplySettings();
    config->SaveAllValues();

    UpdateStatusButtons();

    emit ControllerSelectorReconfigureFinished(is_success);
}

void MainWindow::ControllerSelectorRequestExit() {
    if (controller_applet) {
        controller_applet->reject();
    }
}

void MainWindow::ProfileSelectorSelectProfile(
    const Core::Frontend::ProfileSelectParameters& parameters) {
    const bool was_fullscreen = isFullScreen();
    if (was_fullscreen) {
        showNormal();
    }
    profile_select_applet = new QtProfileSelectionDialog(*QtCommon::system, this, parameters);
    SCOPE_EXIT {
        profile_select_applet->deleteLater();
        profile_select_applet = nullptr;
        if (was_fullscreen) {
            showFullScreen();
        }
    };

    profile_select_applet->setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint |
                                          Qt::WindowStaysOnTopHint | Qt::WindowTitleHint |
                                          Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
    profile_select_applet->setWindowModality(Qt::WindowModal);
    if (profile_select_applet->exec() == QDialog::Rejected) {
        emit ProfileSelectorFinishedSelection(std::nullopt);
        return;
    }

    const auto uuid = QtCommon::system->GetProfileManager().GetUser(
        static_cast<std::size_t>(profile_select_applet->GetIndex()));
    if (!uuid.has_value()) {
        emit ProfileSelectorFinishedSelection(std::nullopt);
        return;
    }

    emit ProfileSelectorFinishedSelection(uuid);
}

void MainWindow::ProfileSelectorRequestExit() {
    if (profile_select_applet) {
        profile_select_applet->reject();
    }
}

void MainWindow::SoftwareKeyboardInitialize(
    bool is_inline, Core::Frontend::KeyboardInitializeParameters initialize_parameters) {
    if (software_keyboard) {
        LOG_ERROR(Frontend, "The software keyboard is already initialized!");
        return;
    }

    software_keyboard = new QtSoftwareKeyboardDialog(render_window, *QtCommon::system, is_inline,
                                                     std::move(initialize_parameters));

    if (is_inline) {
        connect(
            software_keyboard, &QtSoftwareKeyboardDialog::SubmitInlineText, this,
            [this](Service::AM::Frontend::SwkbdReplyType reply_type, std::u16string submitted_text,
                   s32 cursor_position) {
                emit SoftwareKeyboardSubmitInlineText(reply_type, submitted_text, cursor_position);
            },
            Qt::QueuedConnection);
    } else {
        connect(
            software_keyboard, &QtSoftwareKeyboardDialog::SubmitNormalText, this,
            [this](Service::AM::Frontend::SwkbdResult result, std::u16string submitted_text,
                   bool confirmed) {
                emit SoftwareKeyboardSubmitNormalText(result, submitted_text, confirmed);
            },
            Qt::QueuedConnection);
    }
}

void MainWindow::SoftwareKeyboardShowNormal() {
    if (!software_keyboard) {
        LOG_ERROR(Frontend, "The software keyboard is not initialized!");
        return;
    }

    const auto& layout = render_window->GetFramebufferLayout();

    const auto x = layout.screen.left;
    const auto y = layout.screen.top;
    const auto w = layout.screen.GetWidth();
    const auto h = layout.screen.GetHeight();
    const auto scale_ratio = devicePixelRatioF();

    software_keyboard->ShowNormalKeyboard(render_window->mapToGlobal(QPoint(x, y) / scale_ratio),
                                          QSize(w, h) / scale_ratio);
}

void MainWindow::SoftwareKeyboardShowTextCheck(
    Service::AM::Frontend::SwkbdTextCheckResult text_check_result,
    std::u16string text_check_message) {
    if (!software_keyboard) {
        LOG_ERROR(Frontend, "The software keyboard is not initialized!");
        return;
    }

    software_keyboard->ShowTextCheckDialog(text_check_result, text_check_message);
}

void MainWindow::SoftwareKeyboardShowInline(
    Core::Frontend::InlineAppearParameters appear_parameters) {
    if (!software_keyboard) {
        LOG_ERROR(Frontend, "The software keyboard is not initialized!");
        return;
    }

    const auto& layout = render_window->GetFramebufferLayout();

    const auto x =
        static_cast<int>(layout.screen.left + (0.5f * layout.screen.GetWidth() *
                                               ((2.0f * appear_parameters.key_top_translate_x) +
                                                (1.0f - appear_parameters.key_top_scale_x))));
    const auto y =
        static_cast<int>(layout.screen.top + (layout.screen.GetHeight() *
                                              ((2.0f * appear_parameters.key_top_translate_y) +
                                               (1.0f - appear_parameters.key_top_scale_y))));
    const auto w = static_cast<int>(layout.screen.GetWidth() * appear_parameters.key_top_scale_x);
    const auto h = static_cast<int>(layout.screen.GetHeight() * appear_parameters.key_top_scale_y);
    const auto scale_ratio = devicePixelRatioF();

    software_keyboard->ShowInlineKeyboard(std::move(appear_parameters),
                                          render_window->mapToGlobal(QPoint(x, y) / scale_ratio),
                                          QSize(w, h) / scale_ratio);
}

void MainWindow::SoftwareKeyboardHideInline() {
    if (!software_keyboard) {
        LOG_ERROR(Frontend, "The software keyboard is not initialized!");
        return;
    }

    software_keyboard->HideInlineKeyboard();
}

void MainWindow::SoftwareKeyboardInlineTextChanged(
    Core::Frontend::InlineTextParameters text_parameters) {
    if (!software_keyboard) {
        LOG_ERROR(Frontend, "The software keyboard is not initialized!");
        return;
    }

    software_keyboard->InlineTextChanged(std::move(text_parameters));
}

void MainWindow::SoftwareKeyboardExit() {
    if (!software_keyboard) {
        return;
    }

    software_keyboard->ExitKeyboard();

    software_keyboard = nullptr;
}

void MainWindow::WebBrowserOpenWebPage(const std::string& main_url,
                                       const std::string& additional_args, bool is_local) {
#ifdef YUZU_USE_QT_WEB_ENGINE

    // Raw input breaks with the web applet, Disable web applets if enabled
    if (Settings::values.disable_web_applet || Settings::values.enable_raw_input) {
        emit WebBrowserClosed(Service::AM::Frontend::WebExitReason::WindowClosed,
                              "http://localhost/");
        return;
    }

    web_applet = new QtNXWebEngineView(this, *QtCommon::system, input_subsystem.get());

    ui->action_Pause->setEnabled(false);
    ui->action_Restart->setEnabled(false);
    ui->action_Stop->setEnabled(false);

    {
        QProgressDialog loading_progress(this);
        loading_progress.setLabelText(tr("Loading Web Applet..."));
        loading_progress.setRange(0, 3);
        loading_progress.setValue(0);

        if (is_local && !Common::FS::Exists(main_url)) {
            loading_progress.show();

            auto future = QtConcurrent::run([this] { emit WebBrowserExtractOfflineRomFS(); });

            while (!future.isFinished()) {
                QCoreApplication::processEvents();

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        loading_progress.setValue(1);

        if (is_local) {
            web_applet->LoadLocalWebPage(main_url, additional_args);
        } else {
            web_applet->LoadExternalWebPage(main_url, additional_args);
        }

        if (render_window->IsLoadingComplete()) {
            render_window->hide();
        }

        const auto& layout = render_window->GetFramebufferLayout();
        const auto scale_ratio = devicePixelRatioF();
        web_applet->resize(layout.screen.GetWidth() / scale_ratio,
                           layout.screen.GetHeight() / scale_ratio);
        web_applet->move(layout.screen.left / scale_ratio,
                         (layout.screen.top / scale_ratio) + menuBar()->height());
        web_applet->setZoomFactor(static_cast<qreal>(layout.screen.GetWidth() / scale_ratio) /
                                  static_cast<qreal>(Layout::ScreenUndocked::Width));

        web_applet->setFocus();
        web_applet->show();

        loading_progress.setValue(2);

        QCoreApplication::processEvents();

        loading_progress.setValue(3);
    }

    bool exit_check = false;

    // TODO (Morph): Remove this
    QAction* exit_action = new QAction(tr("Disable Web Applet"), this);
    connect(exit_action, &QAction::triggered, this, [this] {
        const auto result = QMessageBox::warning(
            this, tr("Disable Web Applet"),
            tr("Disabling the web applet can lead to undefined behavior and should only be used "
               "with Super Mario 3D All-Stars. Are you sure you want to disable the web "
               "applet?\n(This can be re-enabled in the Debug settings.)"),
            QMessageBox::Yes | QMessageBox::No);
        if (result == QMessageBox::Yes) {
            Settings::values.disable_web_applet = true;
            web_applet->SetFinished(true);
        }
    });
    ui->menubar->addAction(exit_action);

    while (!web_applet->IsFinished()) {
        QCoreApplication::processEvents();

        if (!exit_check) {
            web_applet->page()->runJavaScript(
                QStringLiteral("end_applet;"), [&](const QVariant& variant) {
                    exit_check = false;
                    if (variant.toBool()) {
                        web_applet->SetFinished(true);
                        web_applet->SetExitReason(
                            Service::AM::Frontend::WebExitReason::EndButtonPressed);
                    }
                });

            exit_check = true;
        }

        if (web_applet->GetCurrentURL().contains(QStringLiteral("localhost"))) {
            if (!web_applet->IsFinished()) {
                web_applet->SetFinished(true);
                web_applet->SetExitReason(Service::AM::Frontend::WebExitReason::CallbackURL);
            }

            web_applet->SetLastURL(web_applet->GetCurrentURL().toStdString());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    const auto exit_reason = web_applet->GetExitReason();
    const auto last_url = web_applet->GetLastURL();

    web_applet->hide();

    render_window->setFocus();

    if (render_window->IsLoadingComplete()) {
        render_window->show();
    }

    ui->action_Pause->setEnabled(true);
    ui->action_Restart->setEnabled(true);
    ui->action_Stop->setEnabled(true);

    ui->menubar->removeAction(exit_action);

    QCoreApplication::processEvents();

    emit WebBrowserClosed(exit_reason, last_url);

#else

    // Utilize the same fallback as the default web browser applet.
    emit WebBrowserClosed(Service::AM::Frontend::WebExitReason::WindowClosed, "http://localhost/");

#endif
}

void MainWindow::WebBrowserRequestExit() {
#ifdef YUZU_USE_QT_WEB_ENGINE
    if (web_applet) {
        web_applet->SetExitReason(Service::AM::Frontend::WebExitReason::ExitRequested);
        web_applet->SetFinished(true);
    }
#endif
}

void MainWindow::InitializeWidgets() {
    render_window = new GRenderWindow(this, input_subsystem);
    render_window->hide();

    game_list = new GameList(QtCommon::vfs, QtCommon::provider.get(), *play_time_manager,
                             *QtCommon::system, this);
    ui->horizontalLayout->addWidget(game_list);

    game_list_placeholder = new GameListPlaceholder(this);
    ui->horizontalLayout->addWidget(game_list_placeholder);
    game_list_placeholder->setVisible(false);

    loading_screen = new LoadingScreen(ui->centralwidget);
    loading_screen->hide();
    connect(loading_screen, &LoadingScreen::Hidden, this, [&] {
        loading_screen->Clear();
    });

    multiplayer_state = new MultiplayerState(this, game_list->GetModel(), ui->action_Leave_Room,
                                             ui->action_Show_Room, *QtCommon::system);
    multiplayer_state->setVisible(false);

    // Create status bar
    message_label = new QLabel();
    // Configured separately for left alignment
    message_label->setFrameStyle(QFrame::NoFrame);
    message_label->setContentsMargins(4, 0, 4, 0);
    message_label->setAlignment(Qt::AlignLeft);
    statusBar()->addPermanentWidget(message_label, 1);

    shader_building_label = new QLabel();
    shader_building_label->setToolTip(tr("The amount of shaders currently being built"));
    res_scale_label = new QLabel();
    res_scale_label->setToolTip(tr("The current selected resolution scaling multiplier."));
    emu_speed_label = new QLabel();
    emu_speed_label->setToolTip(
        tr("Current emulation speed. Values higher or lower than 100% "
           "indicate emulation is running faster or slower than a Switch."));
    game_fps_label = new QLabel();
    game_fps_label->setToolTip(tr("How many frames per second the game is currently displaying. "
                                  "This will vary from game to game and scene to scene."));
    emu_frametime_label = new QLabel();
    emu_frametime_label->setToolTip(
        tr("Time taken to emulate a Switch frame, not counting framelimiting or v-sync. For "
           "full-speed emulation this should be at most 16.67 ms."));

    for (auto& label : {shader_building_label, res_scale_label, emu_speed_label, game_fps_label,
                        emu_frametime_label}) {
        label->setVisible(false);
        label->setFrameStyle(QFrame::NoFrame);
        label->setContentsMargins(4, 0, 4, 0);
        statusBar()->addPermanentWidget(label);
    }

    // Purge legacy auto-fix configuration files
    try {
        const auto custom_config_path = Common::FS::GetEdenPath(Common::FS::EdenPath::ConfigDir) / "custom";
        if (std::filesystem::exists(custom_config_path)) {
            for (const auto bitand entry : std::filesystem::directory_iterator(custom_config_path)) {
                if (entry.is_regular_file() and entry.path().extension() == ".ini") {
                    std::ifstream f(entry.path());
                    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                    f.close();
                    if (content.find("storm_fix_applied") != std::string::npos or content.find("storm_fix_dont_ask") != std::string::npos) {
                        std::error_code ec;
                        std::filesystem::remove(entry.path(), ec);
                    }
                }
            }
        }
    } catch (...) {}

    firmware_label = new QLabel();
    firmware_label->setObjectName(QStringLiteral("FirmwareLabel"));
    firmware_label->setVisible(false);
    firmware_label->setContentsMargins(4, 0, 4, 0);
    firmware_label->setFocusPolicy(Qt::NoFocus);
    firmware_label->setCursor(Qt::PointingHandCursor);

    class FirmwareLabelClickFilter : public QObject {
    public:
        explicit FirmwareLabelClickFilter(MainWindow* mw) : QObject(mw), m_mw(mw) {}
    protected:
        bool eventFilter(QObject* obj, QEvent* ev) override {
            if (ev->type() == QEvent::MouseButtonPress) {
                m_mw->ShowFirmwareContextMenu();
                return true;
            }
            return QObject::eventFilter(obj, ev);
        }
    private:
        MainWindow* m_mw;
    };
    firmware_label->installEventFilter(new FirmwareLabelClickFilter(this));
    statusBar()->addPermanentWidget(firmware_label);

    statusBar()->addPermanentWidget(multiplayer_state->GetStatusText(), 0);
    statusBar()->addPermanentWidget(multiplayer_state->GetStatusIcon(), 0);

    tas_label = new QLabel();
    tas_label->setObjectName(QStringLiteral("TASlabel"));
    tas_label->setFocusPolicy(Qt::NoFocus);
    statusBar()->insertPermanentWidget(0, tas_label);

    volume_popup = new QWidget(this);
    volume_popup->setWindowFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::Popup);
    auto* pop_layout = new QVBoxLayout(volume_popup);
    pop_layout->setContentsMargins(10, 8, 10, 8);
    pop_layout->setSpacing(6);
    volume_popup->setStyleSheet(QStringLiteral(
        "QWidget { background-color: #090a10; color: #e0e6ed; font-family: 'Segoe UI', sans-serif; border: 2px solid #00f0ff; border-radius: 6px; }"
        "QSlider::groove:horizontal { height: 6px; background: #121624; border-radius: 3px; border: 1px solid #00f0ff; }"
        "QSlider::sub-page:horizontal { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ff0055, stop:1 #ffee00); border-radius: 3px; }"
        "QSlider::handle:horizontal { background: #ffee00; border: 1px solid #000000; width: 14px; margin-top: -5px; margin-bottom: -5px; border-radius: 7px; }"
        "QSlider::handle:horizontal:hover { background: #00f0ff; }"
        "QPushButton { background-color: #121624; color: #00f0ff; border: 1px solid #00f0ff; border-radius: 3px; padding: 2px 6px; font-size: 8pt; font-weight: bold; }"
        "QPushButton:hover { background-color: #ffee00; color: #000000; }"
    ));

    volume_val_label = new QLabel(tr("Volume: 100%"), volume_popup);
    volume_val_label->setStyleSheet(QStringLiteral("font-weight: bold; color: #ffee00; font-size: 9.5pt; border: none; background: transparent;"));
    volume_val_label->setAlignment(Qt::AlignCenter);
    pop_layout->addWidget(volume_val_label);

    volume_slider = new QSlider(Qt::Horizontal, volume_popup);
    volume_slider->setObjectName(QStringLiteral("volume_slider"));
    volume_slider->setMaximum(200);
    volume_slider->setPageStep(5);
    volume_slider->setTickPosition(QSlider::TicksBelow);
    volume_slider->setTickInterval(25);
    pop_layout->addWidget(volume_slider);

    auto* scale_layout = new QHBoxLayout();
    scale_layout->setContentsMargins(0, 0, 0, 0);
    const QStringList ticks = {QStringLiteral("0%"), QStringLiteral("50%"), QStringLiteral("100%"), QStringLiteral("150%"), QStringLiteral("200%")};
    for (const QString& tick : ticks) {
        auto* lbl = new QLabel(tick, volume_popup);
        lbl->setStyleSheet(QStringLiteral("color: #7b8fa3; font-size: 7.5pt; border: none; background: transparent;"));
        lbl->setAlignment(Qt::AlignCenter);
        scale_layout->addWidget(lbl);
    }
    pop_layout->addLayout(scale_layout);

    auto* preset_layout = new QHBoxLayout();
    preset_layout->setContentsMargins(0, 2, 0, 0);
    preset_layout->setSpacing(4);
    const std::vector<int> presets = {0, 50, 100, 150, 200};
    for (int p_val : presets) {
        auto* btn = new QPushButton(QStringLiteral("%1%").arg(p_val), volume_popup);
        btn->setFocusPolicy(Qt::NoFocus);
        connect(btn, &QPushButton::clicked, this, [this, p_val] {
            Settings::values.audio_muted.SetValue(false);
            Settings::values.volume.SetValue(static_cast<u8>(p_val));
            UpdateVolumeUI();
            UpdateMuteButton();
            ApplyDynamicSettingChange();
        });
        preset_layout->addWidget(btn);
    }
    pop_layout->addLayout(preset_layout);

    volume_button = new VolumeButton();
    volume_button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    volume_button->setFocusPolicy(Qt::NoFocus);
    volume_button->setCheckable(true);
    UpdateVolumeUI();
    connect(volume_slider, &QSlider::valueChanged, this, [this](int percentage) {
        Settings::values.audio_muted.SetValue(false);
        const auto volume = static_cast<u8>(percentage);
        Settings::values.volume.SetValue(volume);
        UpdateVolumeUI();
        UpdateMuteButton();
        ApplyDynamicSettingChange();
    });
    connect(volume_button, &QPushButton::clicked, this, [this] {
        UpdateVolumeUI();
        const bool will_show = !volume_popup->isVisible();
        volume_popup->setVisible(will_show);
        if (will_show) {
            const QPoint btn_top_left = volume_button->mapToGlobal(QPoint(0, 0));
            const int popup_width = 240;
            const int popup_height = 115;
            const int popup_x = btn_top_left.x() + (volume_button->width() - popup_width) / 2;
            const int popup_y = btn_top_left.y() - popup_height - 6;
            volume_popup->setGeometry(popup_x, popup_y, popup_width, popup_height);
            volume_popup->raise();
        }
    });
    volume_button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(volume_button, &QPushButton::customContextMenuRequested,
            [this](const QPoint& menu_location) {
                QMenu context_menu(this);
                const bool muted = Settings::values.audio_muted.GetValue();
                context_menu.addAction(muted
                    ? StormLang(QStringLiteral("Включить звук"), QStringLiteral("Unmute"), QStringLiteral("Ton einschalten"), QStringLiteral("Activer le son"), QStringLiteral("取消静音"), QStringLiteral("ミュート解除"))
                    : StormLang(QStringLiteral("Отключить звук"), QStringLiteral("Mute"), QStringLiteral("Stummschalten"), QStringLiteral("Couper le son"), QStringLiteral("静音"), QStringLiteral("ミュート")),
                    [this] {
                        OnMute();
                        ApplyDynamicSettingChange();
                    });

                context_menu.addAction(StormLang(
                    QStringLiteral("Сбросить громкость (100%)"),
                    QStringLiteral("Reset volume (100%)"),
                    QStringLiteral("Lautstärke zurücksetzen (100%)"),
                    QStringLiteral("Réinitialiser le volume (100 %)"),
                    QStringLiteral("重置音量 (100%)"),
                    QStringLiteral("音量をリセット (100%)")),
                    [this] {
                        Settings::values.volume.SetValue(100);
                        UpdateVolumeUI();
                        ApplyDynamicSettingChange();
                    });

                ShowMenuAtWidget(context_menu, volume_button);
                volume_button->repaint();
            });
    connect(volume_button, &VolumeButton::VolumeChanged, this, &MainWindow::UpdateVolumeUI);

    // setup AA button
    aa_status_button = new QPushButton();
    aa_status_button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    aa_status_button->setFocusPolicy(Qt::NoFocus);
    aa_status_button->setCheckable(true);
    aa_status_button->setChecked(true);
    UpdateAAText();
    auto show_aa_menu = [this]() {
        QMenu context_menu(this);
        const auto cur_aa = Settings::values.anti_aliasing.GetValue();
        const std::vector<std::pair<Settings::AntiAliasing, QString>> aa_options = {
            {Settings::AntiAliasing::None, StormLang(QStringLiteral("Отключено"), QStringLiteral("Disabled"), QStringLiteral("Deaktiviert"), QStringLiteral("Désactivé"), QStringLiteral("已禁用"), QStringLiteral("無効"))},
            {Settings::AntiAliasing::Fxaa, QStringLiteral("FXAA")},
            {Settings::AntiAliasing::Smaa, QStringLiteral("SMAA")},
        };
        for (const auto& opt : aa_options) {
            auto* act = context_menu.addAction(opt.second, [this, opt] {
                ApplyStatusBarSwitchableSetting(Settings::values.anti_aliasing, opt.first,
                                                m_session_backup.is_active,
                                                m_session_backup.anti_aliasing, current_game_path,
                                                last_filename_booted);
                UpdateAAText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(opt.first == cur_aa);
        }
        ShowMenuAtWidget(context_menu, aa_status_button);
        aa_status_button->repaint();
    };
    connect(aa_status_button, &QPushButton::clicked, show_aa_menu);
    aa_status_button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(aa_status_button, &QPushButton::customContextMenuRequested, show_aa_menu);

    // Setup Filter button
    filter_status_button = new QPushButton();
    filter_status_button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    filter_status_button->setFocusPolicy(Qt::NoFocus);
    filter_status_button->setCheckable(true);
    filter_status_button->setChecked(true);
    UpdateFilterText();
    auto show_filter_menu = [this]() {
        QMenu context_menu(this);
        const auto cur_filter = Settings::values.scaling_filter.GetValue();
        const std::vector<std::pair<Settings::ScalingFilter, QString>> filter_options = {
            {Settings::ScalingFilter::NearestNeighbor, StormLang(QStringLiteral("Ближайший сосед"), QStringLiteral("Nearest Neighbor"), QStringLiteral("Nächster Nachbar"), QStringLiteral("Plus proche voisin"), QStringLiteral("最近邻"), QStringLiteral("最近傍"))},
            {Settings::ScalingFilter::Bilinear, StormLang(QStringLiteral("Билинейный"), QStringLiteral("Bilinear"), QStringLiteral("Bilinear"), QStringLiteral("Bilinéaire"), QStringLiteral("双线性"), QStringLiteral("バイリニア"))},
            {Settings::ScalingFilter::Bicubic, StormLang(QStringLiteral("Бикубический"), QStringLiteral("Bicubic"), QStringLiteral("Bikubisch"), QStringLiteral("Bicubique"), QStringLiteral("双三次"), QStringLiteral("バイキュービック"))},
            {Settings::ScalingFilter::Gaussian, StormLang(QStringLiteral("Гаусс"), QStringLiteral("Gaussian"), QStringLiteral("Gauß"), QStringLiteral("Gauss"), QStringLiteral("高斯"), QStringLiteral("ガウス"))},
            {Settings::ScalingFilter::Lanczos, StormLang(QStringLiteral("Ланцош"), QStringLiteral("Lanczos"), QStringLiteral("Lanczos"), QStringLiteral("Lanczos"), QStringLiteral("兰索斯"), QStringLiteral("ランツォシュ"))},
            {Settings::ScalingFilter::ScaleForce, QStringLiteral("ScaleForce")},
            {Settings::ScalingFilter::Fsr, QStringLiteral("FSR")},
            {Settings::ScalingFilter::Area, QStringLiteral("Area")},
            {Settings::ScalingFilter::ZeroTangent, QStringLiteral("Zero-Tangent")},
            {Settings::ScalingFilter::BSpline, QStringLiteral("B-Spline")},
            {Settings::ScalingFilter::Mitchell, QStringLiteral("Mitchell")},
            {Settings::ScalingFilter::Spline1, QStringLiteral("Spline-1")},
            {Settings::ScalingFilter::Mmpx, QStringLiteral("MMPX")},
            {Settings::ScalingFilter::Sgsr, QStringLiteral("SGSR")},
            {Settings::ScalingFilter::SgsrEdge, QStringLiteral("SGSR EdgeDir")},
        };
        for (const auto& opt : filter_options) {
            auto* act = context_menu.addAction(opt.second, [this, opt] {
                ApplyStatusBarSwitchableSetting(Settings::values.scaling_filter, opt.first,
                                                m_session_backup.is_active,
                                                m_session_backup.scaling_filter, current_game_path,
                                                last_filename_booted);
                UpdateFilterText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(opt.first == cur_filter);
        }
        ShowMenuAtWidget(context_menu, filter_status_button);
        filter_status_button->repaint();
    };
    connect(filter_status_button, &QPushButton::clicked, show_filter_menu);
    filter_status_button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(filter_status_button, &QPushButton::customContextMenuRequested, show_filter_menu);

    // Setup Post-Processing Shader button (RetroArch 75-preset catalog)
    StormShaders::SyncShaderFromQSettings();
    shader_status_button = new QPushButton();
    shader_status_button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    shader_status_button->setFocusPolicy(Qt::NoFocus);
    shader_status_button->setCheckable(true);
    UpdateShaderText();
    auto show_shader_menu = [this]() {
        const int cur_shader = Settings::values.post_processing_shader.GetValue();
        StormShaders::ShowSearchableShaderPopup(shader_status_button, cur_shader, [this](int chosen_idx) {
            StormShaders::ApplyShaderByIndex(chosen_idx);
            ApplyStatusBarSwitchableSettingNoBackup(
                Settings::values.post_processing_shader, chosen_idx,
                current_game_path, last_filename_booted);
            UpdateShaderText();
            UpdateFilterText();
            ApplyDynamicSettingChange();
            const auto& cat = StormShaders::GetRetroArchShaderCatalog();
            if (chosen_idx >= 0 && chosen_idx < static_cast<int>(cat.size())) {
                const QString sname = StormShaders::GetLocalizedName(cat[static_cast<std::size_t>(chosen_idx)]);
                statusBar()->showMessage(StormLang(
                    QStringLiteral("🎨 Активный шейдер постобработки: %1"),
                    QStringLiteral("🎨 Active post-processing shader: %1"),
                    QStringLiteral("🎨 Aktiver Post-Processing-Shader: %1"),
                    QStringLiteral("🎨 Shader de post-traitement actif : %1"),
                    QStringLiteral("🎨 当前后处理着色器：%1"),
                    QStringLiteral("🎨 有効なポストプロセスシェーダー: %1"),
                    QStringLiteral("🎨 مظلل المعالجة اللاحقة النشط: %1"),
                    QStringLiteral("🎨 Sombreador de postprocesado activo: %1")
                ).arg(sname), 4000);
            }
        });
        shader_status_button->repaint();
    };
    connect(shader_status_button, &QPushButton::clicked, show_shader_menu);
    shader_status_button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(shader_status_button, &QPushButton::customContextMenuRequested, show_shader_menu);

    // Setup Dock button
    dock_status_button = new QPushButton();
    dock_status_button->setObjectName(QStringLiteral("DockingStatusBarButton"));
    dock_status_button->setFocusPolicy(Qt::NoFocus);
    dock_status_button->setCheckable(true);
    UpdateDockedButton();
    auto show_dock_menu = [this]() {
        QMenu context_menu(this);
        const auto cur_dock = Settings::values.use_docked_mode.GetValue();
        const std::vector<std::pair<Settings::ConsoleMode, QString>> dock_options = {
            {Settings::ConsoleMode::Docked, StormLang(QStringLiteral("В док-станции"), QStringLiteral("Docked"), QStringLiteral("Im Dock"), QStringLiteral("En station d'accueil"), QStringLiteral("底座模式"), QStringLiteral("ドックモード"))},
            {Settings::ConsoleMode::Handheld, StormLang(QStringLiteral("Портативный"), QStringLiteral("Handheld"), QStringLiteral("Handheld"), QStringLiteral("Portable"), QStringLiteral("掌机模式"), QStringLiteral("携帯モード"))},
        };
        for (const auto& opt : dock_options) {
            auto* act = context_menu.addAction(opt.second, [this, opt] {
                if (opt.first != Settings::values.use_docked_mode.GetValue()) {
                    OnToggleDockedMode();
                }
            });
            act->setCheckable(true);
            act->setChecked(opt.first == cur_dock);
        }
        ShowMenuAtWidget(context_menu, dock_status_button);
        dock_status_button->repaint();
    };
    connect(dock_status_button, &QPushButton::clicked, show_dock_menu);
    dock_status_button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(dock_status_button, &QPushButton::customContextMenuRequested, show_dock_menu);

    // Setup GPU Accuracy button
    gpu_accuracy_button = new QPushButton();
    gpu_accuracy_button->setObjectName(QStringLiteral("GPUStatusBarButton"));
    gpu_accuracy_button->setCheckable(true);
    gpu_accuracy_button->setFocusPolicy(Qt::NoFocus);
    UpdateGPUAccuracyButton();
    auto show_gpu_menu = [this]() {
        QMenu context_menu(this);
        const auto cur_gpu = Settings::values.gpu_accuracy.GetValue();
        const std::vector<std::pair<Settings::GpuAccuracy, QString>> gpu_options = {
            {Settings::GpuAccuracy::Low, StormLang(QStringLiteral("Быстрый (низкая точность)"), QStringLiteral("Fast (low accuracy)"), QStringLiteral("Schnell (niedrige Genauigkeit)"), QStringLiteral("Rapide (faible précision)"), QStringLiteral("快速 (低精度)"), QStringLiteral("高速 (低精度)"))},
            {Settings::GpuAccuracy::High, StormLang(QStringLiteral("Высокая точность"), QStringLiteral("High accuracy"), QStringLiteral("Hohe Genauigkeit"), QStringLiteral("Haute précision"), QStringLiteral("高精度"), QStringLiteral("高精度"))},
        };
        for (const auto& opt : gpu_options) {
            auto* act = context_menu.addAction(opt.second, [this, opt] {
                ApplyStatusBarSwitchableSetting(Settings::values.gpu_accuracy, opt.first,
                                                m_session_backup.is_active,
                                                m_session_backup.gpu_accuracy, current_game_path,
                                                last_filename_booted);
                UpdateGPUAccuracyButton();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(opt.first == cur_gpu);
        }
        ShowMenuAtWidget(context_menu, gpu_accuracy_button);
        gpu_accuracy_button->repaint();
    };
    connect(gpu_accuracy_button, &QPushButton::clicked, show_gpu_menu);
    gpu_accuracy_button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(gpu_accuracy_button, &QPushButton::customContextMenuRequested, show_gpu_menu);

    // Setup Renderer API button
    renderer_status_button = new QPushButton();
    renderer_status_button->setObjectName(QStringLiteral("RendererStatusBarButton"));
    renderer_status_button->setCheckable(true);
    renderer_status_button->setFocusPolicy(Qt::NoFocus);
    UpdateAPIText();
    renderer_status_button->setChecked(Settings::values.renderer_backend.GetValue() ==
                                       Settings::RendererBackend::Vulkan);
    auto show_renderer_menu = [this]() {
        QMenu context_menu(this);
        const auto cur_api = Settings::values.renderer_backend.GetValue();
        const std::vector<std::pair<Settings::RendererBackend, QString>> api_options = {
            {Settings::RendererBackend::Vulkan, QStringLiteral("Vulkan")},
            {Settings::RendererBackend::OpenGL_GLSL, QStringLiteral("OpenGL GLSL")},
            {Settings::RendererBackend::OpenGL_SPIRV, QStringLiteral("OpenGL SPIRV")},
            {Settings::RendererBackend::OpenGL_GLASM, QStringLiteral("OpenGL GLASM")},
        };
        for (const auto& opt : api_options) {
            auto* act = context_menu.addAction(opt.second, [this, opt] {
                ApplyStatusBarSwitchableSettingNoBackup(Settings::values.renderer_backend,
                                                        opt.first, current_game_path,
                                                        last_filename_booted);
                UpdateAPIText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(opt.first == cur_api);
        }
        ShowMenuAtWidget(context_menu, renderer_status_button);
        renderer_status_button->repaint();
    };
    connect(renderer_status_button, &QPushButton::clicked, show_renderer_menu);
    renderer_status_button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(renderer_status_button, &QPushButton::customContextMenuRequested, show_renderer_menu);

    // Setup Aspect Ratio button
    aspect_ratio_button = new QPushButton();
    aspect_ratio_button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    aspect_ratio_button->setFocusPolicy(Qt::NoFocus);
    UpdateAspectText();
    auto show_aspect_menu = [this]() {
        QMenu context_menu(this);
        const auto cur_aspect = Settings::values.aspect_ratio.GetValue();
        const std::vector<std::pair<Settings::AspectRatio, QString>> items = {
            {Settings::AspectRatio::R16_9, StormLang(QStringLiteral("16:9 (Стандарт HD)"), QStringLiteral("16:9 (Standard HD)"), QStringLiteral("16:9 (Standard-HD)"), QStringLiteral("16:9 (Standard HD)"), QStringLiteral("16:9 (标准 HD)"), QStringLiteral("16:9 (標準 HD)"), QStringLiteral("16:9 (قياسي HD)"), QStringLiteral("16:9 (Estándar HD)"))},
            {Settings::AspectRatio::R4_3, StormLang(QStringLiteral("4:3 (Классический)"), QStringLiteral("4:3 (Classic)"), QStringLiteral("4:3 (Klassisch)"), QStringLiteral("4:3 (Classique)"), QStringLiteral("4:3 (经典)"), QStringLiteral("4:3 (クラシック)"), QStringLiteral("4:3 (كلاسيكي)"), QStringLiteral("4:3 (Clásico)"))},
            {Settings::AspectRatio::R21_9, StormLang(QStringLiteral("21:9 (Ультраширокий)"), QStringLiteral("21:9 (Ultrawide)"), QStringLiteral("21:9 (Ultrawide)"), QStringLiteral("21:9 (Ultra-large)"), QStringLiteral("21:9 (超宽屏)"), QStringLiteral("21:9 (ウルトラワイド)"), QStringLiteral("21:9 (عريض جداً)"), QStringLiteral("21:9 (Ultraancho)"))},
            {Settings::AspectRatio::R16_10, StormLang(QStringLiteral("16:10 (Ноутбуки / Steam Deck)"), QStringLiteral("16:10 (Laptops / Steam Deck)"), QStringLiteral("16:10 (Laptops / Steam Deck)"), QStringLiteral("16:10 (PC portables / Steam Deck)"), QStringLiteral("16:10 (笔记本 / Steam Deck)"), QStringLiteral("16:10 (ノートPC / Steam Deck)"), QStringLiteral("16:10 (لابتوب / Steam Deck)"), QStringLiteral("16:10 (Portátiles / Steam Deck)"))},
            {Settings::AspectRatio::Stretch, StormLang(QStringLiteral("Растянуть по окну"), QStringLiteral("Stretch to Window"), QStringLiteral("An Fenster anpassen"), QStringLiteral("Étirer à la fenêtre"), QStringLiteral("拉伸至窗口"), QStringLiteral("ウィンドウに合わせる"), QStringLiteral("تمديد للنافذة"), QStringLiteral("Estirar a la ventana"))},
            {Settings::AspectRatio::R32_9, StormLang(QStringLiteral("32:9 (Супер-ультраширокий)"), QStringLiteral("32:9 (Super Ultrawide)"), QStringLiteral("32:9 (Super-Ultrawide)"), QStringLiteral("32:9 (Super ultra-large)"), QStringLiteral("32:9 (超级超宽屏)"), QStringLiteral("32:9 (スーパーウルトラワイド)"), QStringLiteral("32:9 (فائق العرض)"), QStringLiteral("32:9 (Súper ultraancho)"))},
            {Settings::AspectRatio::R1_1, StormLang(QStringLiteral("1:1 (Квадратный)"), QStringLiteral("1:1 (Square)"), QStringLiteral("1:1 (Quadratisch)"), QStringLiteral("1:1 (Carré)"), QStringLiteral("1:1 (正方形)"), QStringLiteral("1:1 (正方形)"), QStringLiteral("1:1 (مربع)"), QStringLiteral("1:1 (Cuadrado)"))},
            {Settings::AspectRatio::R3_2, StormLang(QStringLiteral("3:2 (Surface / Планшеты)"), QStringLiteral("3:2 (Surface / Tablets)"), QStringLiteral("3:2 (Surface / Tablets)"), QStringLiteral("3:2 (Surface / Tablettes)"), QStringLiteral("3:2 (Surface / 平板)"), QStringLiteral("3:2 (Surface / タブレット)"), QStringLiteral("3:2 (Surface / الأجهزة اللوحية)"), QStringLiteral("3:2 (Surface / Tabletas)"))},
            {Settings::AspectRatio::R5_4, StormLang(QStringLiteral("5:4 (Мониторы SXGA)"), QStringLiteral("5:4 (SXGA Monitors)"), QStringLiteral("5:4 (SXGA-Monitore)"), QStringLiteral("5:4 (Moniteurs SXGA)"), QStringLiteral("5:4 (SXGA 显示器)"), QStringLiteral("5:4 (SXGA モニター)"), QStringLiteral("5:4 (شاشات SXGA)"), QStringLiteral("5:4 (Monitores SXGA)"))},
            {Settings::AspectRatio::R18_9, StormLang(QStringLiteral("18:9 / 2:1 (Смартфоны Univisium)"), QStringLiteral("18:9 / 2:1 (Univisium Mobile)"), QStringLiteral("18:9 / 2:1 (Univisium Mobil)"), QStringLiteral("18:9 / 2:1 (Mobile Univisium)"), QStringLiteral("18:9 / 2:1 (Univisium 手机)"), QStringLiteral("18:9 / 2:1 (Univisium モバイル)"), QStringLiteral("18:9 / 2:1 (هواتف Univisium)"), QStringLiteral("18:9 / 2:1 (Móviles Univisium)"))},
            {Settings::AspectRatio::R19_5_9, StormLang(QStringLiteral("19.5:9 (Современные смартфоны)"), QStringLiteral("19.5:9 (Modern Smartphones)"), QStringLiteral("19.5:9 (Moderne Smartphones)"), QStringLiteral("19.5:9 (Smartphones modernes)"), QStringLiteral("19.5:9 (现代智能手机)"), QStringLiteral("19.5:9 (最新スマートフォン)"), QStringLiteral("19.5:9 (الهواتف الذكية الحديثة)"), QStringLiteral("19.5:9 (Smartphones modernos)"))},
            {Settings::AspectRatio::R20_9, StormLang(QStringLiteral("20:9 (Игровые смартфоны)"), QStringLiteral("20:9 (Gaming Smartphones)"), QStringLiteral("20:9 (Gaming-Smartphones)"), QStringLiteral("20:9 (Smartphones gaming)"), QStringLiteral("20:9 (游戏手机)"), QStringLiteral("20:9 (ゲーミングスマホ)"), QStringLiteral("20:9 (هواتف الألعاب)"), QStringLiteral("20:9 (Smartphones gaming)"))},
            {Settings::AspectRatio::R21_10, StormLang(QStringLiteral("21:10 (Широкоформатный)"), QStringLiteral("21:10 (Widescreen)"), QStringLiteral("21:10 (Breitbild)"), QStringLiteral("21:10 (Écran large)"), QStringLiteral("21:10 (宽屏)"), QStringLiteral("21:10 (ワイドスクリーン)"), QStringLiteral("21:10 (شاشة عريضة)"), QStringLiteral("21:10 (Pantalla ancha)"))},
            {Settings::AspectRatio::R24_10, StormLang(QStringLiteral("24:10 / 2.4:1 (Киноформат)"), QStringLiteral("24:10 / 2.4:1 (Cinema)"), QStringLiteral("24:10 / 2.4:1 (Kinoformat)"), QStringLiteral("24:10 / 2.4:1 (Cinéma)"), QStringLiteral("24:10 / 2.4:1 (电影宽银幕)"), QStringLiteral("24:10 / 2.4:1 (シネマ)"), QStringLiteral("24:10 / 2.4:1 (سينمائي)"), QStringLiteral("24:10 / 2.4:1 (Cine)"))},
            {Settings::AspectRatio::R32_10, StormLang(QStringLiteral("32:10 (Двойной 16:10)"), QStringLiteral("32:10 (Dual 16:10)"), QStringLiteral("32:10 (Dual 16:10)"), QStringLiteral("32:10 (Double 16:10)"), QStringLiteral("32:10 (双 16:10)"), QStringLiteral("32:10 (デュアル 16:10)"), QStringLiteral("32:10 (مزدوج 16:10)"), QStringLiteral("32:10 (Doble 16:10)"))},
            {Settings::AspectRatio::R48_9, StormLang(QStringLiteral("48:9 (Три монитора 16:9)"), QStringLiteral("48:9 (Triple 16:9 Surround)"), QStringLiteral("48:9 (Dreifach-16:9 Surround)"), QStringLiteral("48:9 (Triple écran 16:9)"), QStringLiteral("48:9 (三联屏 16:9)"), QStringLiteral("48:9 (トリプル 16:9)"), QStringLiteral("48:9 (ثلاث شاشات 16:9)"), QStringLiteral("48:9 (Triple monitor 16:9)"))},
            {Settings::AspectRatio::Auto_Window, StormLang(QStringLiteral("Авто по пропорциям окна (3D без растяжения)"), QStringLiteral("Auto Window Aspect (3D Native)"), QStringLiteral("Automatisch nach Fenstergröße (3D Nativ)"), QStringLiteral("Auto selon la fenêtre (3D natif)"), QStringLiteral("按窗口比例自动适配 (3D 原生无拉伸)"), QStringLiteral("ウィンドウ比率に自動適応 (3D ネイティブ)"), QStringLiteral("تلقائي حسب أبعاد النافذة (3D أصلي)"), QStringLiteral("Auto según ventana (3D nativo)"))},
        };
        for (const auto& item : items) {
            auto* act = context_menu.addAction(item.second, [this, item] {
                ApplyStatusBarSwitchableSetting(Settings::values.aspect_ratio, item.first,
                                                m_session_backup.is_active,
                                                m_session_backup.aspect_ratio, current_game_path,
                                                last_filename_booted);
                UpdateAspectText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(item.first == cur_aspect);
        }
        ShowMenuAtWidget(context_menu, aspect_ratio_button);
    };
    connect(aspect_ratio_button, &QPushButton::clicked, show_aspect_menu);
    aspect_ratio_button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(aspect_ratio_button, &QPushButton::customContextMenuRequested, show_aspect_menu);

    // Setup DMA Accuracy button
    dma_accuracy_button = new QPushButton();
    dma_accuracy_button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    dma_accuracy_button->setFocusPolicy(Qt::NoFocus);
    UpdateDmaText();
    auto show_dma_menu = [this]() {
        QMenu context_menu(this);
        const auto cur_dma = Settings::values.dma_accuracy.GetValue();
        const std::vector<std::pair<Settings::DmaAccuracy, QString>> items = {
            {Settings::DmaAccuracy::Default, StormLang(QStringLiteral("По умолчанию"), QStringLiteral("Default"), QStringLiteral("Standard"), QStringLiteral("Par défaut"), QStringLiteral("默认"), QStringLiteral("デフォルト"))},
            {Settings::DmaAccuracy::Normal, StormLang(QStringLiteral("Нормально"), QStringLiteral("Normal"), QStringLiteral("Normal"), QStringLiteral("Normal"), QStringLiteral("正常"), QStringLiteral("通常"))},
            {Settings::DmaAccuracy::Unsafe, StormLang(QStringLiteral("Небезопасно"), QStringLiteral("Unsafe"), QStringLiteral("Unsicher"), QStringLiteral("Non sécurisé"), QStringLiteral("不安全"), QStringLiteral("非安全"))},
            {Settings::DmaAccuracy::Safe, StormLang(QStringLiteral("Безопасно"), QStringLiteral("Safe"), QStringLiteral("Sicher"), QStringLiteral("Sécurisé"), QStringLiteral("安全"), QStringLiteral("安全"))},
        };
        for (const auto& item : items) {
            auto* act = context_menu.addAction(item.second, [this, item] {
                ApplyStatusBarSwitchableSetting(Settings::values.dma_accuracy, item.first,
                                                m_session_backup.is_active,
                                                m_session_backup.dma_accuracy, current_game_path,
                                                last_filename_booted);
                UpdateDmaText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(item.first == cur_dma);
        }
        ShowMenuAtWidget(context_menu, dma_accuracy_button);
    };
    connect(dma_accuracy_button, &QPushButton::clicked, show_dma_menu);
    dma_accuracy_button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(dma_accuracy_button, &QPushButton::customContextMenuRequested, show_dma_menu);

    // Setup GPU Fence button
    gpu_fence_button = new QPushButton();
    gpu_fence_button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    gpu_fence_button->setFocusPolicy(Qt::NoFocus);
    UpdateGpuFenceText();
    auto show_gpu_fence_menu = [this]() {
        QMenu context_menu(this);
        const auto cur_fence = Settings::values.gpu_fence_behavior.GetValue();
        const std::vector<std::pair<Settings::GpuFenceBehavior, QString>> items = {
            {Settings::GpuFenceBehavior::Default, StormLang(QStringLiteral("По умолчанию"), QStringLiteral("Default"), QStringLiteral("Standard"), QStringLiteral("Par défaut"), QStringLiteral("默认"), QStringLiteral("デフォルト"))},
            {Settings::GpuFenceBehavior::Immediate, StormLang(QStringLiteral("Мгновенно"), QStringLiteral("Immediate"), QStringLiteral("Sofort"), QStringLiteral("Immédiat"), QStringLiteral("立即"), QStringLiteral("即時"))},
            {Settings::GpuFenceBehavior::Balanced, StormLang(QStringLiteral("Сбалансированно"), QStringLiteral("Balanced"), QStringLiteral("Ausgewogen"), QStringLiteral("Équilibré"), QStringLiteral("平衡"), QStringLiteral("バランス"))},
            {Settings::GpuFenceBehavior::Accurate, StormLang(QStringLiteral("Точно"), QStringLiteral("Accurate"), QStringLiteral("Präzise"), QStringLiteral("Précis"), QStringLiteral("精确"), QStringLiteral("高精度"))},
            {Settings::GpuFenceBehavior::Strict, StormLang(QStringLiteral("Строго"), QStringLiteral("Strict"), QStringLiteral("Strikt"), QStringLiteral("Strict"), QStringLiteral("严格"), QStringLiteral("厳格"))},
        };
        for (const auto& item : items) {
            auto* act = context_menu.addAction(item.second, [this, item] {
                ApplyStatusBarSwitchableSetting(Settings::values.gpu_fence_behavior, item.first,
                                                m_session_backup.is_active,
                                                m_session_backup.gpu_fence_behavior,
                                                current_game_path, last_filename_booted);
                UpdateGpuFenceText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(item.first == cur_fence);
        }
        ShowMenuAtWidget(context_menu, gpu_fence_button);
    };
    connect(gpu_fence_button, &QPushButton::clicked, show_gpu_fence_menu);
    gpu_fence_button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(gpu_fence_button, &QPushButton::customContextMenuRequested, show_gpu_fence_menu);

    // Setup VRAM Mode button
    vram_mode_button = new QPushButton();
    vram_mode_button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    vram_mode_button->setFocusPolicy(Qt::NoFocus);
    UpdateVramText();
    auto show_vram_menu = [this]() {
        QMenu context_menu(this);
        const auto cur_vram = Settings::values.vram_usage_mode.GetValue();
        const std::vector<std::pair<Settings::VramUsageMode, QString>> options = {
            {Settings::VramUsageMode::Conservative, StormLang(QStringLiteral("Консервативный"), QStringLiteral("Conservative"), QStringLiteral("Konservativ"), QStringLiteral("Conservateur"), QStringLiteral("保守"), QStringLiteral("控えめ"))},
            {Settings::VramUsageMode::Normal, StormLang(QStringLiteral("Нормальный"), QStringLiteral("Normal"), QStringLiteral("Normal"), QStringLiteral("Normal"), QStringLiteral("正常"), QStringLiteral("通常"))},
            {Settings::VramUsageMode::Aggressive, StormLang(QStringLiteral("Агрессивный"), QStringLiteral("Aggressive"), QStringLiteral("Aggressiv"), QStringLiteral("Agressif"), QStringLiteral("激进"), QStringLiteral("積極的"))},
        };
        for (const auto& opt : options) {
            auto* act = context_menu.addAction(opt.second, [this, opt] {
                ApplyStatusBarSwitchableSetting(Settings::values.vram_usage_mode, opt.first,
                                                m_session_backup.is_active,
                                                m_session_backup.vram_usage_mode, current_game_path,
                                                last_filename_booted);
                UpdateVramText();
                ApplyDynamicSettingChange();
                if (QtCommon::system && QtCommon::system->IsPoweredOn()) {
                    statusBar()->showMessage(StormLang(
                        QStringLiteral("Режим использования VRAM изменен. Полный бюджет видеопамяти применится при следующем запуске игры."),
                        QStringLiteral("VRAM usage mode changed. Full GPU memory budget will apply upon next game launch."),
                        QStringLiteral("VRAM-Nutzungsmodus geändert. Das vollständige GPU-Speicherbudget gilt beim nächsten Spielstart."),
                        QStringLiteral("Mode d'utilisation de la VRAM modifié. Le budget mémoire s'appliquera au prochain lancement."),
                        QStringLiteral("显存使用模式已更改。完整的显存预算将在下次启动游戏时生效。"),
                        QStringLiteral("VRAM使用モードを変更しました。完全なGPUメモリ割り当ては次回のゲーム起動時に適用されます。")
                    ), 6000);
                }
            });
            act->setCheckable(true);
            act->setChecked(opt.first == cur_vram);
        }
        ShowMenuAtWidget(context_menu, vram_mode_button);
    };
    connect(vram_mode_button, &QPushButton::clicked, show_vram_menu);
    vram_mode_button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(vram_mode_button, &QPushButton::customContextMenuRequested, show_vram_menu);

    // Setup Anisotropy button
    anisotropy_button = new QPushButton();
    anisotropy_button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    anisotropy_button->setFocusPolicy(Qt::NoFocus);
    UpdateAnisotropyText();
    auto show_anisotropy_menu = [this]() {
        QMenu context_menu(this);
        const auto cur_aniso = Settings::values.max_anisotropy.GetValue();
        const std::vector<std::pair<Settings::AnisotropyMode, QString>> options = {
            {Settings::AnisotropyMode::Automatic, StormLang(QStringLiteral("Автоматически"), QStringLiteral("Automatic"), QStringLiteral("Automatisch"), QStringLiteral("Automatique"), QStringLiteral("自动"), QStringLiteral("自動"))},
            {Settings::AnisotropyMode::Default, StormLang(QStringLiteral("По умолчанию"), QStringLiteral("Default"), QStringLiteral("Standard"), QStringLiteral("Par défaut"), QStringLiteral("默认"), QStringLiteral("デフォルト"))},
            {Settings::AnisotropyMode::X2, QStringLiteral("2x")},
            {Settings::AnisotropyMode::X4, QStringLiteral("4x")},
            {Settings::AnisotropyMode::X8, QStringLiteral("8x")},
            {Settings::AnisotropyMode::X16, QStringLiteral("16x")},
            {Settings::AnisotropyMode::X32, QStringLiteral("32x")},
            {Settings::AnisotropyMode::X64, QStringLiteral("64x")},
            {Settings::AnisotropyMode::None, StormLang(QStringLiteral("Отключено"), QStringLiteral("Disabled"), QStringLiteral("Deaktiviert"), QStringLiteral("Désactivé"), QStringLiteral("已禁用"), QStringLiteral("無効"))},
        };
        for (const auto& opt : options) {
            auto* act = context_menu.addAction(opt.second, [this, opt] {
                ApplyStatusBarSwitchableSetting(Settings::values.max_anisotropy, opt.first,
                                                m_session_backup.is_active,
                                                m_session_backup.max_anisotropy, current_game_path,
                                                last_filename_booted);
                UpdateAnisotropyText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(opt.first == cur_aniso);
        }
        ShowMenuAtWidget(context_menu, anisotropy_button);
    };
    connect(anisotropy_button, &QPushButton::clicked, show_anisotropy_menu);
    anisotropy_button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(anisotropy_button, &QPushButton::customContextMenuRequested, show_anisotropy_menu);

    // Setup ASTC Decode button
    astc_decode_button = new QPushButton();
    astc_decode_button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    astc_decode_button->setFocusPolicy(Qt::NoFocus);
    UpdateAstcDecodeText();
    auto show_astc_decode_menu = [this]() {
        QMenu context_menu(this);
        const auto cur_dec = Settings::values.accelerate_astc.GetValue();
        const std::vector<std::pair<Settings::AstcDecodeMode, QString>> options = {
            {Settings::AstcDecodeMode::CpuAsynchronous, StormLang(QStringLiteral("ЦП асинхронно"), QStringLiteral("CPU asynchronous"), QStringLiteral("CPU asynchron"), QStringLiteral("CPU asynchrone"), QStringLiteral("CPU 异步"), QStringLiteral("CPU 非同期"))},
            {Settings::AstcDecodeMode::Cpu, StormLang(QStringLiteral("ЦП"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"))},
            {Settings::AstcDecodeMode::Gpu, StormLang(QStringLiteral("ГПУ"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"))},
            {Settings::AstcDecodeMode::Hybrid, StormLang(QStringLiteral("Гибридный"), QStringLiteral("Hybrid"), QStringLiteral("Hybrid"), QStringLiteral("Hybride"), QStringLiteral("混合"), QStringLiteral("ハイブリッド"))},
        };
        for (const auto& opt : options) {
            auto* act = context_menu.addAction(opt.second, [this, opt] {
                ApplyStatusBarSwitchableSetting(Settings::values.accelerate_astc, opt.first,
                                                m_session_backup.is_active,
                                                m_session_backup.accelerate_astc, current_game_path,
                                                last_filename_booted);
                UpdateAstcDecodeText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(opt.first == cur_dec);
        }
        ShowMenuAtWidget(context_menu, astc_decode_button);
    };
    connect(astc_decode_button, &QPushButton::clicked, show_astc_decode_menu);
    astc_decode_button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(astc_decode_button, &QPushButton::customContextMenuRequested, show_astc_decode_menu);

    // Setup ASTC Recompress button
    astc_recompress_button = new QPushButton();
    astc_recompress_button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    astc_recompress_button->setFocusPolicy(Qt::NoFocus);
    UpdateAstcRecompressText();
    auto show_astc_recompress_menu = [this]() {
        QMenu context_menu(this);
        const auto cur_rec = Settings::values.astc_recompression.GetValue();
        const std::vector<std::pair<Settings::AstcRecompression, QString>> options = {
            {Settings::AstcRecompression::Uncompressed, StormLang(QStringLiteral("Без сжатия (лучшее качество)"), QStringLiteral("Uncompressed (best quality)"), QStringLiteral("Unkomprimiert (beste Qualität)"), QStringLiteral("Non compressé (meilleure qualité)"), QStringLiteral("未压缩 (最佳画质)"), QStringLiteral("非圧縮 (最高品質)"))},
            {Settings::AstcRecompression::Bc1, StormLang(QStringLiteral("BC1 (низкое качество)"), QStringLiteral("BC1 (low quality)"), QStringLiteral("BC1 (niedrige Qualität)"), QStringLiteral("BC1 (faible qualité)"), QStringLiteral("BC1 (低画质)"), QStringLiteral("BC1 (低品質)"))},
            {Settings::AstcRecompression::Bc3, StormLang(QStringLiteral("BC3 (среднее качество)"), QStringLiteral("BC3 (medium quality)"), QStringLiteral("BC3 (mittlere Qualität)"), QStringLiteral("BC3 (qualité moyenne)"), QStringLiteral("BC3 (中画质)"), QStringLiteral("BC3 (中品質)"))},
            {Settings::AstcRecompression::Bc5, StormLang(QStringLiteral("BC5 (высокое качество)"), QStringLiteral("BC5 (high quality)"), QStringLiteral("BC5 (hohe Qualität)"), QStringLiteral("BC5 (haute qualité)"), QStringLiteral("BC5 (高画质)"), QStringLiteral("BC5 (高品質)"))},
        };
        for (const auto& opt : options) {
            auto* act = context_menu.addAction(opt.second, [this, opt] {
                ApplyStatusBarSwitchableSetting(Settings::values.astc_recompression, opt.first,
                                                m_session_backup.is_active,
                                                m_session_backup.astc_recompression,
                                                current_game_path, last_filename_booted);
                UpdateAstcRecompressText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(opt.first == cur_rec);
        }
        ShowMenuAtWidget(context_menu, astc_recompress_button);
    };
    connect(astc_recompress_button, &QPushButton::clicked, show_astc_recompress_menu);
    astc_recompress_button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(astc_recompress_button, &QPushButton::customContextMenuRequested, show_astc_recompress_menu);

    // Setup Addons button
    addons_status_button = new QPushButton();
    addons_status_button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    addons_status_button->setFocusPolicy(Qt::NoFocus);
    UpdateAddonsStatusButton();
    connect(addons_status_button, &QPushButton::clicked, this, [this] {
        if (m_current_addons_title_id == 0 && game_list) {
            const auto [tid, path] = game_list->GetSelectedGameInfo();
            if (tid != 0) {
                m_current_addons_title_id = tid;
                m_current_addons_game_path = path.toStdString();
                m_current_addons_game_name = QFileInfo(path).completeBaseName();
                UpdateAddonsStatusButton(m_current_addons_title_id, m_current_addons_game_name);
            }
        }
        ShowDLCDialog(m_current_addons_title_id, m_current_addons_game_name);
    });
    auto show_addons_menu = [this]() {
        if (m_current_addons_title_id == 0 && game_list) {
            const auto [tid, path] = game_list->GetSelectedGameInfo();
            if (tid != 0) {
                m_current_addons_title_id = tid;
                m_current_addons_game_path = path.toStdString();
                m_current_addons_game_name = QFileInfo(path).completeBaseName();
                UpdateAddonsStatusButton(m_current_addons_title_id, m_current_addons_game_name);
            }
        }
        QMenu context_menu(this);
        if (m_current_addons_title_id == 0) {
            auto* act = context_menu.addAction(StormLang(
                QStringLiteral("⚠️ Игра не выбрана или не запущена"),
                QStringLiteral("⚠️ No game selected or running"),
                QStringLiteral("⚠️ Kein Spiel ausgewählt oder wird ausgeführt"),
                QStringLiteral("⚠️ Aucun jeu sélectionné ou en cours d'exécution"),
                QStringLiteral("⚠️ 未选择或未运行游戏"),
                QStringLiteral("⚠️ ゲームが選択されていないか実行されていません")
            ));
            act->setEnabled(false);
        } else {
            const FileSys::PatchManager patch_manager(m_current_addons_title_id, QtCommon::system->GetFileSystemController(), QtCommon::system->GetContentProvider());
            auto patches = patch_manager.GetPatches();

            auto* title_act = context_menu.addAction(StormLang(
                QStringLiteral("🎮 Дополнения и патчи (ID: 0x%1)"),
                QStringLiteral("🎮 Add-ons and Patches (ID: 0x%1)"),
                QStringLiteral("🎮 Add-ons und Patches (ID: 0x%1)"),
                QStringLiteral("🎮 Extensions et patchs (ID: 0x%1)"),
                QStringLiteral("🎮 附加组件和补丁 (ID: 0x%1)"),
                QStringLiteral("🎮 アドオンとパッチ (ID: 0x%1)")
            ).arg(QStringLiteral("%1").arg(m_current_addons_title_id, 16, 16, QLatin1Char('0')).toUpper()));
            title_act->setEnabled(false);
            context_menu.addSeparator();

            context_menu.addAction(StormLang(
                QStringLiteral("📋 Открыть менеджер дополнений..."),
                QStringLiteral("📋 Open Add-ons Manager..."),
                QStringLiteral("📋 Add-ons-Manager öffnen..."),
                QStringLiteral("📋 Ouvrir le gestionnaire d'extensions..."),
                QStringLiteral("📋 打开附加组件管理器..."),
                QStringLiteral("📋 アドオンマネージャーを開く...")
            ), [this] {
                ShowDLCDialog(m_current_addons_title_id, m_current_addons_game_name);
            });

            context_menu.addAction(StormLang(
                QStringLiteral("📑 Скопировать список дополнений"),
                QStringLiteral("📑 Copy Add-ons List"),
                QStringLiteral("📑 Add-ons-Liste kopieren"),
                QStringLiteral("📑 Copier la liste des extensions"),
                QStringLiteral("📑 复制附加组件列表"),
                QStringLiteral("📑 アドオンリストをコピー")
            ), [this] {
                const FileSys::PatchManager pm(m_current_addons_title_id, QtCommon::system->GetFileSystemController(), QtCommon::system->GetContentProvider());
                const auto pts = pm.GetPatches();
                QStringList lines;
                lines << QStringLiteral("STORM SWITCH - ") + StormLang(
                    QStringLiteral("Список дополнений"), QStringLiteral("Add-ons List"),
                    QStringLiteral("Add-ons-Liste"), QStringLiteral("Liste des extensions"),
                    QStringLiteral("附加组件列表"), QStringLiteral("アドオンリスト"));
                lines << QStringLiteral("%1: %2 (ID: 0x%3)")
                    .arg(StormLang(QStringLiteral("Игра"), QStringLiteral("Game"), QStringLiteral("Spiel"), QStringLiteral("Jeu"), QStringLiteral("游戏"), QStringLiteral("ゲーム")),
                         m_current_addons_game_name, QStringLiteral("%1").arg(m_current_addons_title_id, 16, 16, QLatin1Char('0')).toUpper());
                lines << QStringLiteral("------------------------------------------------------------");
                int idx = 1;
                for (const auto& p : pts) {
                    if (p.type == FileSys::PatchType::DLC || p.type == FileSys::PatchType::Mod || p.type == FileSys::PatchType::Update) {
                        QString ptype = (p.type == FileSys::PatchType::Update) ? StormLang(QStringLiteral("Обновление"), QStringLiteral("Update"), QStringLiteral("Update"), QStringLiteral("Mise à jour"), QStringLiteral("更新"), QStringLiteral("更新")) :
                                        (p.type == FileSys::PatchType::DLC) ? QStringLiteral("DLC") : QStringLiteral("Mod");
                        lines << QStringLiteral("%1. 0x%2 - [%3] %4 (%5)")
                            .arg(QString::number(idx++), QStringLiteral("%1").arg(p.title_id, 16, 16, QLatin1Char('0')).toUpper(),
                                 ptype, QString::fromStdString(p.name), p.enabled ?
                                 StormLang(QStringLiteral("Включено"), QStringLiteral("Enabled"), QStringLiteral("Aktiviert"), QStringLiteral("Activé"), QStringLiteral("已启用"), QStringLiteral("有効")) :
                                 StormLang(QStringLiteral("Отключено"), QStringLiteral("Disabled"), QStringLiteral("Deaktiviert"), QStringLiteral("Désactivé"), QStringLiteral("已禁用"), QStringLiteral("無効")));
                    }
                }
                QGuiApplication::clipboard()->setText(lines.join(QLatin1Char('\n')));
                statusBar()->showMessage(StormLang(
                    QStringLiteral("Список дополнений скопирован в буфер обмена."),
                    QStringLiteral("Add-ons list copied to clipboard."),
                    QStringLiteral("Add-ons-Liste in die Zwischenablage kopiert."),
                    QStringLiteral("Liste des extensions copiée dans le presse-papiers."),
                    QStringLiteral("附加组件列表已复制到剪贴板。"),
                    QStringLiteral("アドオンリストをクリップボードにコピーしました。")
                ), 3000);
            });

            context_menu.addSeparator();

            int addon_count = 0;
            for (const auto& patch : patches) {
                QString name = QString::fromStdString(patch.name);
                QString ver = QString::fromStdString(patch.version);
                QString icon = QStringLiteral("📦");
                if (patch.type == FileSys::PatchType::Update) {
                    icon = QStringLiteral("🆙");
                    name = StormLang(QStringLiteral("Обновление"), QStringLiteral("Update"), QStringLiteral("Update"), QStringLiteral("Mise à jour"), QStringLiteral("更新"), QStringLiteral("更新"));
                } else if (patch.type == FileSys::PatchType::DLC) {
                    icon = QStringLiteral("🧩");
                    name = QStringLiteral("DLC");
                } else if (patch.type == FileSys::PatchType::Mod) {
                    icon = QStringLiteral("⚡");
                }

                QString text = ver.isEmpty() ? QStringLiteral("%1  %2").arg(icon, name)
                                             : QStringLiteral("%1  %2: %3").arg(icon, name, ver);
                QAction* act = context_menu.addAction(text);
                act->setCheckable(true);
                act->setChecked(patch.enabled);
                act->setEnabled(false);
                addon_count++;
            }

            if (addon_count == 0) {
                auto* empty_act = context_menu.addAction(StormLang(
                    QStringLiteral("Нет установленных дополнений"),
                    QStringLiteral("No add-ons installed"),
                    QStringLiteral("Keine Add-ons installiert"),
                    QStringLiteral("Aucune extension installée"),
                    QStringLiteral("未安装附加组件"),
                    QStringLiteral("アドオンがインストールされていません")
                ));
                empty_act->setEnabled(false);
            }

            context_menu.addSeparator();
            context_menu.addAction(StormLang(
                QStringLiteral("⚙️ Свойства игры (Управление дополнениями)..."),
                QStringLiteral("⚙️ Game properties (Manage add-ons)..."),
                QStringLiteral("⚙️ Spieleigenschaften (Add-ons verwalten)..."),
                QStringLiteral("⚙️ Propriétés du jeu (Gérer les extensions)..."),
                QStringLiteral("⚙️ 游戏属性 (管理附加组件)..."),
                QStringLiteral("⚙️ ゲームのプロパティ (アドオンの管理)...")
            ), [this] {
                OpenPerGameConfiguration(m_current_addons_title_id, m_current_addons_game_path);
            });
        }
        ShowMenuAtWidget(context_menu, addons_status_button);
    };
    addons_status_button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(addons_status_button, &QPushButton::customContextMenuRequested, show_addons_menu);

    // Setup GameFix / Quick Settings button
    gamefix_status_button = new QPushButton();
    gamefix_status_button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    gamefix_status_button->setFocusPolicy(Qt::NoFocus);
    UpdateGameFixStatusButton();
    auto show_gamefix_menu = [this]() {
        if (m_current_addons_title_id == 0 && game_list) {
            const auto [tid, path] = game_list->GetSelectedGameInfo();
            if (tid != 0) {
                m_current_addons_title_id = tid;
                m_current_addons_game_path = path.toStdString();
                m_current_addons_game_name = QFileInfo(path).completeBaseName();
            }
        }
        QMenu context_menu(this);
        const auto* profile = Core::GameFixDatabase::GetProfileByTitleOrPath(
            m_current_addons_title_id, m_current_addons_game_path);
        const QString header_text = profile != nullptr
            ? StormLang(QStringLiteral("🛡️ Профиль: %1"), QStringLiteral("🛡️ Profile: %1"),
                        QStringLiteral("🛡️ Profil: %1"), QStringLiteral("🛡️ Profil : %1"),
                        QStringLiteral("🛡️ 配置文件: %1"), QStringLiteral("🛡️ プロファイル: %1"))
                  .arg(QString::fromStdString(profile->game_name))
            : StormLang(QStringLiteral("⚡ Быстрые настройки и Авто-фикс"),
                        QStringLiteral("⚡ Quick Settings & Auto-Fix"),
                        QStringLiteral("⚡ Schnelleinstellungen & Auto-Fix"),
                        QStringLiteral("⚡ Paramètres rapides & Auto-Fix"),
                        QStringLiteral("⚡ 快捷设置与自动修复"),
                        QStringLiteral("⚡ クイック設定と自動修正"));
        auto* hdr = context_menu.addAction(header_text);
        hdr->setEnabled(false);
        context_menu.addSeparator();

        if (profile != nullptr) {
            context_menu.addAction(StormLang(
                QStringLiteral("🛡️ Открыть диалог авто-исправления игры..."),
                QStringLiteral("🛡️ Open Game Auto-Fix Dialog..."),
                QStringLiteral("🛡️ Spiel-Auto-Fix-Dialog öffnen..."),
                QStringLiteral("🛡️ Ouvrir le dialogue d'auto-correction..."),
                QStringLiteral("🛡️ 打开游戏自动修复对话框..."),
                QStringLiteral("🛡️ ゲーム自動修正ダイアログを開く...")
            ), [this] {
                ShowGameFixDialog(m_current_addons_title_id,
                                  QString::fromStdString(m_current_addons_game_path), true);
                UpdateStatusButtons();
            });
            context_menu.addSeparator();
        }

        auto* act_fg = context_menu.addAction(StormLang(
            QStringLiteral("🚀 Генерация кадров LSFG (x2 FPS)"),
            QStringLiteral("🚀 LSFG Frame Generation (x2 FPS)"),
            QStringLiteral("🚀 LSFG-Bildgenerierung (x2 FPS)"),
            QStringLiteral("🚀 Génération d'images LSFG (x2 FPS)"),
            QStringLiteral("🚀 LSFG 帧生成 (x2 FPS)"),
            QStringLiteral("🚀 LSFG フレーム生成 (x2 FPS)")
        ), [this] {
            ApplyStatusBarSwitchableSetting(Settings::values.frame_gen,
                                            !Settings::values.frame_gen.GetValue(),
                                            m_session_backup.is_active, m_session_backup.frame_gen,
                                            current_game_path, last_filename_booted);
            UpdateStatusButtons();
            ApplyDynamicSettingChange();
        });
        act_fg->setCheckable(true);
        act_fg->setChecked(Settings::values.frame_gen.GetValue());

        auto* act_drs = context_menu.addAction(StormLang(
            QStringLiteral("🔒 Блокировка динамического разрешения (100% DRS)"),
            QStringLiteral("🔒 Dynamic Resolution Lock (100% DRS)"),
            QStringLiteral("🔒 Dynamische Auflösungssperre (100% DRS)"),
            QStringLiteral("🔒 Verrouillage de résolution dynamique (100% DRS)"),
            QStringLiteral("🔒 动态分辨率锁定 (100% DRS)"),
            QStringLiteral("🔒 動的解像度ロック (100% DRS)")
        ), [this] {
            ApplyStatusBarSwitchableSetting(Settings::values.drs_resolution_lock,
                                            !Settings::values.drs_resolution_lock.GetValue(),
                                            m_session_backup.is_active,
                                            m_session_backup.drs_resolution_lock, current_game_path,
                                            last_filename_booted);
            UpdateStatusButtons();
            ApplyDynamicSettingChange();
        });
        act_drs->setCheckable(true);
        act_drs->setChecked(Settings::values.drs_resolution_lock.GetValue());

        auto* act_rf = context_menu.addAction(StormLang(
            QStringLiteral("⚡ Реактивная очистка буферов (Reactive Flushing)"),
            QStringLiteral("⚡ Reactive Buffer Flushing"),
            QStringLiteral("⚡ Reaktives Puffer-Flushing"),
            QStringLiteral("⚡ Vidage réactif des tampons"),
            QStringLiteral("⚡ 响应式缓冲刷新 (Reactive Flushing)"),
            QStringLiteral("⚡ リアクティブフラッシング")
        ), [this] {
            ApplyStatusBarSwitchableSetting(Settings::values.use_reactive_flushing,
                                            !Settings::values.use_reactive_flushing.GetValue(),
                                            m_session_backup.is_active,
                                            m_session_backup.use_reactive_flushing,
                                            current_game_path, last_filename_booted);
            UpdateStatusButtons();
            ApplyDynamicSettingChange();
        });
        act_rf->setCheckable(true);
        act_rf->setChecked(Settings::values.use_reactive_flushing.GetValue());

        auto* act_rb = context_menu.addAction(StormLang(
            QStringLiteral("🔄 Обратное чтение буферов ГПУ (GPU Readback)"),
            QStringLiteral("🔄 GPU Buffer Readback"),
            QStringLiteral("🔄 GPU-Puffer-Readback"),
            QStringLiteral("🔄 Lecture inverse des tampons GPU"),
            QStringLiteral("🔄 GPU 缓冲回读 (Readback)"),
            QStringLiteral("🔄 GPU バッファリードバック")
        ), [this] {
            ApplyStatusBarSwitchableSetting(Settings::values.enable_gpu_buffer_readback,
                                            !Settings::values.enable_gpu_buffer_readback.GetValue(),
                                            m_session_backup.is_active,
                                            m_session_backup.enable_gpu_buffer_readback,
                                            current_game_path, last_filename_booted);
            UpdateStatusButtons();
            ApplyDynamicSettingChange();
        });
        act_rb->setCheckable(true);
        act_rb->setChecked(Settings::values.enable_gpu_buffer_readback.GetValue());

        auto* act_bloom = context_menu.addAction(StormLang(
            QStringLiteral("✨ Защита свечения HDR (Fix Bloom Effects)"),
            QStringLiteral("✨ HDR Bloom Protection (Fix Bloom Effects)"),
            QStringLiteral("✨ HDR-Bloom-Schutz (Fix Bloom Effects)"),
            QStringLiteral("✨ Protection HDR Bloom"),
            QStringLiteral("✨ HDR 泛光修复 (Fix Bloom Effects)"),
            QStringLiteral("✨ HDR ブルーム保護")
        ), [this] {
            ApplyStatusBarSwitchableSetting(Settings::values.fix_bloom_effects,
                                            !Settings::values.fix_bloom_effects.GetValue(),
                                            m_session_backup.is_active,
                                            m_session_backup.fix_bloom_effects, current_game_path,
                                            last_filename_booted);
            UpdateStatusButtons();
            ApplyDynamicSettingChange();
        });
        act_bloom->setCheckable(true);
        act_bloom->setChecked(Settings::values.fix_bloom_effects.GetValue());

        context_menu.addSeparator();
        auto* act_ar_patch = context_menu.addAction(StormLang(
            QStringLiteral("📐 Авто-патч 3D-камеры под формат экрана (Auto Aspect Patch)"),
            QStringLiteral("📐 Auto-Patch 3D Camera Aspect Ratio"),
            QStringLiteral("📐 3D-Kamera-Seitenverhältnis automatisch patchen"),
            QStringLiteral("📐 Patch auto du format d'image 3D"),
            QStringLiteral("📐 自动修补 3D 摄像机宽高比（无拉伸）"),
            QStringLiteral("📐 3D カメラアスペクト比自動パッチ"),
            QStringLiteral("📐 تصحيح تلقائي لنسبة أبعاد الكاميرا 3D"),
            QStringLiteral("📐 Auto-parche de relación de aspecto 3D")
        ), [this] {
            ApplyStatusBarSwitchableSettingNoBackup(
                Settings::values.auto_aspect_ratio_patch,
                !Settings::values.auto_aspect_ratio_patch.GetValue(),
                current_game_path, last_filename_booted);
            UpdateStatusButtons();
            ApplyDynamicSettingChange();
        });
        act_ar_patch->setCheckable(true);
        act_ar_patch->setChecked(Settings::values.auto_aspect_ratio_patch.GetValue());

        auto* act_fps_unlock = context_menu.addAction(StormLang(
            QStringLiteral("🚀 Авто-снятие лимита 30 FPS -> 60/120 FPS (Auto FPS Unlock)"),
            QStringLiteral("🚀 Auto-Unlock 30 FPS Cap -> 60/120 FPS"),
            QStringLiteral("🚀 30-FPS-Limit automatisch auf 60/120 FPS entsperren"),
            QStringLiteral("🚀 Déverrouillage auto 30 FPS -> 60/120 FPS"),
            QStringLiteral("🚀 自动解锁 30 FPS 限制 -> 60/120 FPS"),
            QStringLiteral("🚀 30 FPS 制限を 60/120 FPS に自動解除"),
            QStringLiteral("🚀 فتح قيد 30 إطاراً تلقائياً إلى 60/120 FPS"),
            QStringLiteral("🚀 Auto-desbloqueo de 30 FPS -> 60/120 FPS")
        ), [this] {
            const bool next = !Settings::values.auto_60fps_unlock.GetValue();
            ApplyStatusBarSwitchableSettingNoBackup(
                Settings::values.auto_60fps_unlock, next,
                current_game_path, last_filename_booted);
            if (next) {
                Settings::values.use_speed_limit.SetValue(true);
                if (Settings::values.speed_limit.GetValue() < 100) {
                    Settings::values.speed_limit.SetValue(100);
                }
            }
            UpdateStatusButtons();
            ApplyDynamicSettingChange();
        });
        act_fps_unlock->setCheckable(true);
        act_fps_unlock->setChecked(Settings::values.auto_60fps_unlock.GetValue());

        auto* act_hdr = context_menu.addAction(StormLang(
            QStringLiteral("🌈 Авто-HDR10 (Inverse Tone Mapping)"),
            QStringLiteral("🌈 Auto-HDR10 (Inverse Tone Mapping)"),
            QStringLiteral("🌈 Auto-HDR10 (Inverses Tone Mapping)"),
            QStringLiteral("🌈 Auto-HDR10 (Tone Mapping inverse)"),
            QStringLiteral("🌈 自动 HDR10（逆色调映射）"),
            QStringLiteral("🌈 自動 HDR10（逆トーンマッピング）"),
            QStringLiteral("🌈 تقنية Auto-HDR10 التلقائية"),
            QStringLiteral("🌈 Auto-HDR10 (Mapeo de tonos inverso)")
        ), [this] {
            ApplyStatusBarSwitchableSettingNoBackup(
                Settings::values.enable_hdr10,
                !Settings::values.enable_hdr10.GetValue(),
                current_game_path, last_filename_booted);
            UpdateStatusButtons();
            ApplyDynamicSettingChange();
        });
        act_hdr->setCheckable(true);
        act_hdr->setChecked(Settings::values.enable_hdr10.GetValue());

        if (m_session_backup.is_active) {
            context_menu.addSeparator();
            context_menu.addAction(StormLang(
                QStringLiteral("♻️ Сбросить настройки сессии к глобальным"),
                QStringLiteral("♻️ Reset Session Settings to Global"),
                QStringLiteral("♻️ Sitzungseinstellungen auf Global zurücksetzen"),
                QStringLiteral("♻️ Réinitialiser la session aux paramètres globaux"),
                QStringLiteral("♻️ 将会话设置重置为全局"),
                QStringLiteral("♻️ セッション設定をグローバルにリセット")
            ), [this] {
                m_session_backup.Restore();
                m_session_backup.Capture();
                Core::GameFixDatabase::SetFixesEnabled(false);
                UpdateStatusButtons();
                ApplyDynamicSettingChange();
                statusBar()->showMessage(StormLang(
                    QStringLiteral("♻️ Настройки текущей сессии сброшены к глобальным"),
                    QStringLiteral("♻️ Current session settings reset to global"),
                    QStringLiteral("♻️ Aktuelle Sitzungseinstellungen auf Global zurückgesetzt"),
                    QStringLiteral("♻️ Paramètres de session réinitialisés aux valeurs globales"),
                    QStringLiteral("♻️ 当前会话设置已重置为全局"),
                    QStringLiteral("♻️ 現在のセッション設定をグローバルにリセットしました")
                ), 5000);
            });
        }

        ShowMenuAtWidget(context_menu, gamefix_status_button);
    };
    connect(gamefix_status_button, &QPushButton::clicked, show_gamefix_menu);
    gamefix_status_button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(gamefix_status_button, &QPushButton::customContextMenuRequested, show_gamefix_menu);

    // Setup Resolution Scale button
    res_scale_button = new QPushButton();
    res_scale_button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    res_scale_button->setFocusPolicy(Qt::NoFocus);
    UpdateResScaleText();
    auto show_res_menu = [this]() {
        QMenu context_menu(this);
        const auto cur_res = Settings::values.resolution_setup.GetValue();
        const bool is_docked = Settings::values.use_docked_mode.GetValue() == Settings::ConsoleMode::Docked;
        const std::vector<std::pair<Settings::ResolutionSetup, QString>> res_options = {
            {Settings::ResolutionSetup::Res1_4X, is_docked ? QStringLiteral("0.25X (270p)") : QStringLiteral("0.25X (180p)")},
            {Settings::ResolutionSetup::Res1_2X, is_docked ? QStringLiteral("0.5X (540p)") : QStringLiteral("0.5X (360p)")},
            {Settings::ResolutionSetup::Res3_4X, is_docked ? QStringLiteral("0.75X (810p)") : QStringLiteral("0.75X (540p)")},
            {Settings::ResolutionSetup::Res1X, is_docked ? QStringLiteral("1X (1080p)") : QStringLiteral("1X (720p)")},
            {Settings::ResolutionSetup::Res5_4X, is_docked ? QStringLiteral("1.25X (1350p)") : QStringLiteral("1.25X (900p)")},
            {Settings::ResolutionSetup::Res3_2X, is_docked ? QStringLiteral("1.5X (1620p)") : QStringLiteral("1.5X (1080p)")},
            {Settings::ResolutionSetup::Res2X, is_docked ? QStringLiteral("2X (2160p / 4K)") : QStringLiteral("2X (1440p / 2K)")},
            {Settings::ResolutionSetup::Res3X, is_docked ? QStringLiteral("3X (3240p / 6K)") : QStringLiteral("3X (2160p / 4K)")},
            {Settings::ResolutionSetup::Res4X, is_docked ? QStringLiteral("4X (4320p / 8K)") : QStringLiteral("4X (2880p)")},
            {Settings::ResolutionSetup::Res5X, is_docked ? QStringLiteral("5X (5400p)") : QStringLiteral("5X (3600p)")},
            {Settings::ResolutionSetup::Res6X, is_docked ? QStringLiteral("6X (6480p)") : QStringLiteral("6X (4320p)")},
            {Settings::ResolutionSetup::Res7X, is_docked ? QStringLiteral("7X (7560p)") : QStringLiteral("7X (5040p)")},
            {Settings::ResolutionSetup::Res8X, is_docked ? QStringLiteral("8X (8640p)") : QStringLiteral("8X (5760p)")},
        };
        for (const auto& opt : res_options) {
            auto* act = context_menu.addAction(opt.second, [this, opt] {
                ApplyStatusBarSwitchableSetting(Settings::values.resolution_setup, opt.first,
                                                m_session_backup.is_active,
                                                m_session_backup.resolution_setup,
                                                current_game_path, last_filename_booted);
                UpdateResScaleText();
                ApplyDynamicSettingChange();
                if (QtCommon::system && QtCommon::system->IsPoweredOn()) {
                    statusBar()->showMessage(StormLang(
                        QStringLiteral("Масштаб разрешения изменен. Для предотвращения графических артефактов рекомендуется перезапуск игры."),
                        QStringLiteral("Resolution scale changed. Restarting the game is recommended to avoid visual artifacts."),
                        QStringLiteral("Auflösungsskalierung geändert. Ein Neustart des Spiels wird empfohlen."),
                        QStringLiteral("Échelle de résolution modifiée. Redémarrer le jeu est recommandé."),
                        QStringLiteral("分辨率缩放已更改。建议重新启动游戏以避免视觉伪影。"),
                        QStringLiteral("解像度スケールを変更しました。視覚的なアーティファクトを避けるためゲームの再起動をお勧めします。")
                    ), 6000);
                }
            });
            act->setCheckable(true);
            act->setChecked(opt.first == cur_res);
        }
        ShowMenuAtWidget(context_menu, res_scale_button);
    };
    connect(res_scale_button, &QPushButton::clicked, show_res_menu);
    res_scale_button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(res_scale_button, &QPushButton::customContextMenuRequested, show_res_menu);

    // Setup Refresh Button
    refresh_button = new QPushButton();
    refresh_button->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    refresh_button->setObjectName(QStringLiteral("RefreshButton"));
    refresh_button->setFocusPolicy(Qt::NoFocus);
    UpdateRefreshButton();
    connect(refresh_button, &QPushButton::clicked, this, &MainWindow::OnGameListRefresh);

    // Setup Airplane Mode button
    airplane_mode_button = new QPushButton();
    airplane_mode_button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    airplane_mode_button->setFocusPolicy(Qt::NoFocus);
    UpdateAirplaneModeButton();
    connect(airplane_mode_button, &QPushButton::clicked, this, [this] {
        ApplyStatusBarSwitchableSetting(Settings::values.airplane_mode,
                                        !Settings::values.airplane_mode.GetValue(),
                                        m_session_backup.is_active, m_session_backup.airplane_mode,
                                        current_game_path, last_filename_booted);
        UpdateAirplaneModeButton();
        ApplyDynamicSettingChange();
    });
    airplane_mode_button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(airplane_mode_button, &QPushButton::customContextMenuRequested, [this] {
        ApplyStatusBarSwitchableSetting(Settings::values.airplane_mode,
                                        !Settings::values.airplane_mode.GetValue(),
                                        m_session_backup.is_active, m_session_backup.airplane_mode,
                                        current_game_path, last_filename_booted);
        UpdateAirplaneModeButton();
        ApplyDynamicSettingChange();
    });

    // Setup VSync button
    vsync_mode_button = new QPushButton();
    vsync_mode_button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    vsync_mode_button->setFocusPolicy(Qt::NoFocus);
    UpdateVSyncText();
    auto show_vsync_menu = [this]() {
        QMenu context_menu(this);
        const auto cur_vsync = Settings::values.vsync_mode.GetValue();
        const std::vector<std::pair<Settings::VSyncMode, QString>> vsync_options = {
            {Settings::VSyncMode::Fifo, QStringLiteral("FIFO (VSync)")},
            {Settings::VSyncMode::FifoRelaxed, QStringLiteral("FIFO Relaxed")},
            {Settings::VSyncMode::Mailbox, QStringLiteral("Mailbox")},
            {Settings::VSyncMode::Immediate, StormLang(QStringLiteral("Immediate (Без VSync)"), QStringLiteral("Immediate (No VSync)"), QStringLiteral("Immediate (Ohne VSync)"), QStringLiteral("Immediate (Sans VSync)"), QStringLiteral("Immediate (无垂直同步)"), QStringLiteral("Immediate (垂直同期なし)"))},
        };
        for (const auto& opt : vsync_options) {
            auto* act = context_menu.addAction(opt.second, [this, opt] {
                ApplyStatusBarSwitchableSettingNoBackup(Settings::values.vsync_mode, opt.first,
                                                        current_game_path, last_filename_booted);
                UpdateVSyncText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(opt.first == cur_vsync);
        }
        ShowMenuAtWidget(context_menu, vsync_mode_button);
    };
    connect(vsync_mode_button, &QPushButton::clicked, show_vsync_menu);
    vsync_mode_button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(vsync_mode_button, &QPushButton::customContextMenuRequested, show_vsync_menu);

    // Setup Speed Limit button
    speed_limit_button = new QPushButton();
    speed_limit_button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    speed_limit_button->setFocusPolicy(Qt::NoFocus);
    UpdateSpeedLimitText();
    auto show_speed_menu = [this]() {
        QMenu context_menu(this);
        const bool use_limit = Settings::values.use_speed_limit.GetValue();
        const u16 speed_val = Settings::values.speed_limit.GetValue();

        auto* act100 = context_menu.addAction(QStringLiteral("100%"), [this] {
            Settings::values.use_speed_limit.SetValue(true);
            Settings::values.speed_limit.SetValue(100);
            UpdateSpeedLimitText();
            ApplyDynamicSettingChange();
        });
        act100->setCheckable(true);
        act100->setChecked(use_limit && speed_val == 100);

        auto* act150 = context_menu.addAction(QStringLiteral("150%"), [this] {
            Settings::values.use_speed_limit.SetValue(true);
            Settings::values.speed_limit.SetValue(150);
            UpdateSpeedLimitText();
            ApplyDynamicSettingChange();
        });
        act150->setCheckable(true);
        act150->setChecked(use_limit && speed_val == 150);

        auto* act200 = context_menu.addAction(QStringLiteral("200%"), [this] {
            Settings::values.use_speed_limit.SetValue(true);
            Settings::values.speed_limit.SetValue(200);
            UpdateSpeedLimitText();
            ApplyDynamicSettingChange();
        });
        act200->setCheckable(true);
        act200->setChecked(use_limit && speed_val == 200);

        auto* act300 = context_menu.addAction(QStringLiteral("300%"), [this] {
            Settings::values.use_speed_limit.SetValue(true);
            Settings::values.speed_limit.SetValue(300);
            UpdateSpeedLimitText();
            ApplyDynamicSettingChange();
        });
        act300->setCheckable(true);
        act300->setChecked(use_limit && speed_val == 300);

        auto* act_unlimit = context_menu.addAction(StormLang(
            QStringLiteral("Без лимита скорости"),
            QStringLiteral("No speed limit"),
            QStringLiteral("Keine Geschwindigkeitsbegrenzung"),
            QStringLiteral("Aucune limite de vitesse"),
            QStringLiteral("无速度限制"),
            QStringLiteral("速度制限なし")
        ), [this] {
            Settings::values.use_speed_limit.SetValue(false);
            UpdateSpeedLimitText();
            ApplyDynamicSettingChange();
        });
        act_unlimit->setCheckable(true);
        act_unlimit->setChecked(!use_limit);

        ShowMenuAtWidget(context_menu, speed_limit_button);
    };
    connect(speed_limit_button, &QPushButton::clicked, show_speed_menu);
    speed_limit_button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(speed_limit_button, &QPushButton::customContextMenuRequested, show_speed_menu);

    // Setup NVDEC button
    nvdec_status_button = new QPushButton();
    nvdec_status_button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    nvdec_status_button->setFocusPolicy(Qt::NoFocus);
    UpdateNvdecText();
    auto show_nvdec_menu = [this]() {
        QMenu context_menu(this);
        const auto cur_nvdec = Settings::values.nvdec_emulation.GetValue();
        const std::vector<std::pair<Settings::NvdecEmulation, QString>> nvdec_options = {
            {Settings::NvdecEmulation::Gpu, StormLang(QStringLiteral("ГПУ"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"))},
            {Settings::NvdecEmulation::Hybrid, StormLang(QStringLiteral("Гибридный"), QStringLiteral("Hybrid"), QStringLiteral("Hybrid"), QStringLiteral("Hybride"), QStringLiteral("混合"), QStringLiteral("ハイブリッド"))},
            {Settings::NvdecEmulation::Cpu, StormLang(QStringLiteral("ЦП"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"))},
            {Settings::NvdecEmulation::Off, StormLang(QStringLiteral("Отключено"), QStringLiteral("Disabled"), QStringLiteral("Deaktiviert"), QStringLiteral("Désactivé"), QStringLiteral("已禁用"), QStringLiteral("無効"))},
        };
        for (const auto& opt : nvdec_options) {
            auto* act = context_menu.addAction(opt.second, [this, opt] {
                ApplyStatusBarSwitchableSetting(Settings::values.nvdec_emulation, opt.first,
                                                m_session_backup.is_active,
                                                m_session_backup.nvdec_emulation, current_game_path,
                                                last_filename_booted);
                UpdateNvdecText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(opt.first == cur_nvdec);
        }
        ShowMenuAtWidget(context_menu, nvdec_status_button);
    };
    connect(nvdec_status_button, &QPushButton::clicked, show_nvdec_menu);
    nvdec_status_button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(nvdec_status_button, &QPushButton::customContextMenuRequested, show_nvdec_menu);

    // Setup CPU Accuracy button
    cpu_accuracy_button = new QPushButton();
    cpu_accuracy_button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    cpu_accuracy_button->setFocusPolicy(Qt::NoFocus);
    UpdateCpuAccuracyText();
    auto show_cpu_menu = [this]() {
        QMenu context_menu(this);
        const auto cur_cpu = Settings::values.cpu_accuracy.GetValue();
        const std::vector<std::pair<Settings::CpuAccuracy, QString>> cpu_options = {
            {Settings::CpuAccuracy::Auto, StormLang(QStringLiteral("Авто"), QStringLiteral("Auto"), QStringLiteral("Automatisch"), QStringLiteral("Auto"), QStringLiteral("自动"), QStringLiteral("自動"))},
            {Settings::CpuAccuracy::Accurate, StormLang(QStringLiteral("Точно"), QStringLiteral("Accurate"), QStringLiteral("Genau"), QStringLiteral("Précis"), QStringLiteral("准确"), QStringLiteral("正確"))},
            {Settings::CpuAccuracy::Unsafe, StormLang(QStringLiteral("Небезопасно"), QStringLiteral("Unsafe"), QStringLiteral("Unsicher"), QStringLiteral("Non sécurisé"), QStringLiteral("不安全"), QStringLiteral("非安全"))},
        };
        for (const auto& opt : cpu_options) {
            auto* act = context_menu.addAction(opt.second, [this, opt] {
                ApplyStatusBarSwitchableSetting(Settings::values.cpu_accuracy, opt.first,
                                                m_session_backup.is_active,
                                                m_session_backup.cpu_accuracy, current_game_path,
                                                last_filename_booted);
                UpdateCpuAccuracyText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(opt.first == cur_cpu);
        }
        ShowMenuAtWidget(context_menu, cpu_accuracy_button);
    };
    connect(cpu_accuracy_button, &QPushButton::clicked, show_cpu_menu);
    cpu_accuracy_button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(cpu_accuracy_button, &QPushButton::customContextMenuRequested, show_cpu_menu);

    // Setup Disk Shader Cache button
    disk_cache_button = new QPushButton();
    disk_cache_button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    disk_cache_button->setFocusPolicy(Qt::NoFocus);
    UpdateDiskCacheText();
    connect(disk_cache_button, &QPushButton::clicked, this, [this] {
        ApplyStatusBarSwitchableSettingNoBackup(
            Settings::values.use_disk_shader_cache,
            !Settings::values.use_disk_shader_cache.GetValue(), current_game_path,
            last_filename_booted);
        UpdateDiskCacheText();
        ApplyDynamicSettingChange();
    });
    disk_cache_button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(disk_cache_button, &QPushButton::customContextMenuRequested, [this] {
        ApplyStatusBarSwitchableSettingNoBackup(
            Settings::values.use_disk_shader_cache,
            !Settings::values.use_disk_shader_cache.GetValue(), current_game_path,
            last_filename_booted);
        UpdateDiskCacheText();
        ApplyDynamicSettingChange();
    });

    // Setup Fullscreen button
    fullscreen_button = new QPushButton();
    fullscreen_button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    fullscreen_button->setFocusPolicy(Qt::NoFocus);
    UpdateFullscreenButton();
    connect(fullscreen_button, &QPushButton::clicked, this, [this] {
        ui->action_Fullscreen->setChecked(!ui->action_Fullscreen->isChecked());
        ToggleFullscreen();
        UpdateFullscreenButton();
    });

    // Setup Mute button
    mute_button = new QPushButton();
    mute_button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    mute_button->setFocusPolicy(Qt::NoFocus);
    UpdateMuteButton();
    connect(mute_button, &QPushButton::clicked, this, &MainWindow::OnMute);

    // Setup Footer Customize button
    footer_customize_button = new QPushButton(QStringLiteral("⚙"));
    footer_customize_button->setObjectName(QStringLiteral("FooterCustomizeButton"));
    footer_customize_button->setToolTip(StormLang(
        QStringLiteral("Настройка отображения разделов и кнопок подвала"),
        QStringLiteral("Customize footer sections and buttons display"),
        QStringLiteral("Fußzeilenabschnitte und Schaltflächen anpassen"),
        QStringLiteral("Personnaliser l'affichage des sections et boutons du pied de page"),
        QStringLiteral("自定义底部栏分区与按钮显示"),
        QStringLiteral("フッターのセクションとボタンの表示をカスタマイズ")
    ));
    footer_customize_button->setFocusPolicy(Qt::NoFocus);
    footer_customize_button->setFixedWidth(28);
    footer_customize_button->setMinimumHeight(32);
    footer_customize_button->setStyleSheet(QStringLiteral(
        "QPushButton#FooterCustomizeButton {"
        "  background-color: rgba(255, 255, 255, 0.08);"
        "  color: #00f2fe;"
        "  border: 1px solid rgba(255, 255, 255, 0.15);"
        "  border-radius: 4px;"
        "  font-size: 11pt;"
        "  font-weight: bold;"
        "  padding: 0px;"
        "}"
        "QPushButton#FooterCustomizeButton:hover {"
        "  background-color: #00f2fe;"
        "  color: #000000;"
        "  border-color: #00f2fe;"
        "}"
    ));
    connect(footer_customize_button, &QPushButton::clicked, this, &MainWindow::ShowFooterCustomizeMenu);

    // ============================================================
    // GROUPED STATUS BAR LAYOUT
    // ============================================================

    m_status_groups.clear();
    m_status_group_headers.clear();

    auto createGroup = [this](int group_index, const QString& title, const QString& color, const QList<QWidget*>& widgets) -> QWidget* {
        auto* container = new QWidget();
        container->setObjectName(QStringLiteral("StatusBarGroup"));
        container->setStyleSheet(QStringLiteral(
            "QWidget#StatusBarGroup {"
            "  background-color: #0e1320;"
            "  border: 1px solid rgba(255, 255, 255, 0.10);"
            "  border-top: 2px solid %1;"
            "  border-radius: 5px;"
            "  margin: 1px 1px;"
            "  padding: 1px 2px;"
            "}"
            "QWidget#StatusBarGroup:hover {"
            "  background-color: #141b2e;"
            "  border: 1px solid %1;"
            "  border-top: 2px solid %1;"
            "}"
            "QPushButton#StatusBarGroupLabel {"
            "  background-color: rgba(255, 255, 255, 0.07);"
            "  color: %1;"
            "  font-size: 6.8pt;"
            "  font-weight: 800;"
            "  letter-spacing: 0.8px;"
            "  padding: 1px 8px;"
            "  margin: 0px 1px 2px 1px;"
            "  border: 1px solid rgba(255, 255, 255, 0.08);"
            "  border-radius: 3px;"
            "  min-height: 12px;"
            "}"
            "QPushButton#StatusBarGroupLabel:hover {"
            "  background-color: %1;"
            "  color: #000000;"
            "  border: 1px solid %1;"
            "}"
            "QPushButton#TogglableStatusBarButton, QPushButton {"
            "  background-color: #121826;"
            "  color: #ffffff;"
            "  border: 1px solid rgba(255, 255, 255, 0.13);"
            "  border-radius: 4px;"
            "  padding: 2px 3px;"
            "  font-size: 6.8pt;"
            "  font-weight: 700;"
            "  min-height: 26px;"
            "  min-width: 36px;"
            "}"
            "QPushButton#TogglableStatusBarButton:hover, QPushButton:hover {"
            "  background-color: rgba(255, 255, 255, 0.16);"
            "  color: %1;"
            "  border: 1px solid %1;"
            "}"
            "QPushButton#TogglableStatusBarButton:pressed, QPushButton:pressed {"
            "  background-color: %1;"
            "  color: #000000;"
            "}"
            "QLabel {"
            "  color: #ffffff;"
            "  font-size: 6.8pt;"
            "  font-weight: 700;"
            "}"
            "QLabel:hover {"
            "  color: %1;"
            "  background-color: rgba(255, 255, 255, 0.10);"
            "  border-radius: 2px;"
            "}"
        ).arg(color));

        auto* layout = new QVBoxLayout(container);
        layout->setContentsMargins(2, 1, 2, 1);
        layout->setSpacing(1);
        layout->setAlignment(Qt::AlignCenter);

        auto* headerBtn = new QPushButton(title);
        headerBtn->setObjectName(QStringLiteral("StatusBarGroupLabel"));
        headerBtn->setCursor(Qt::PointingHandCursor);
        headerBtn->setFocusPolicy(Qt::NoFocus);
        headerBtn->setFlat(true);
        headerBtn->setToolTip(StormLang(
            QStringLiteral("Нажмите для быстрого меню раздела «%1»").arg(title),
            QStringLiteral("Click for quick menu of \"%1\" section").arg(title),
            QStringLiteral("Klicken für Schnellmenü des Abschnitts \"%1\"").arg(title),
            QStringLiteral("Cliquer pour le menu rapide de la section \"%1\"").arg(title),
            QStringLiteral("点击打开“%1”分区的快捷菜单").arg(title),
            QStringLiteral("クリックして「%1」セクションのクイックメニューを開く").arg(title)
        ));
        auto show_grp = [this, group_index, container]() {
            ShowGroupMenu(group_index, container);
        };
        connect(headerBtn, &QPushButton::clicked, show_grp);
        headerBtn->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(headerBtn, &QPushButton::customContextMenuRequested, show_grp);
        layout->addWidget(headerBtn);

        auto* btnRow = new QWidget();
        btnRow->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
        auto* btnLayout = new QHBoxLayout(btnRow);
        btnLayout->setContentsMargins(0, 0, 0, 0);
        btnLayout->setSpacing(2);
        btnLayout->setAlignment(Qt::AlignCenter);
        for (auto* w : widgets) {
            w->setMinimumHeight(26);
            btnLayout->addWidget(w);
        }
        layout->addWidget(btnRow);

        container->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(container, &QWidget::customContextMenuRequested, show_grp);

        m_status_groups.push_back(container);
        m_status_group_headers.push_back(headerBtn);
        return container;
    };

    // --- Group 0: CONTROLS (Green accent) ---
    statusBar()->insertPermanentWidget(0, createGroup(
        0, StormLang(QStringLiteral("УПРАВЛЕНИЕ"), QStringLiteral("CONTROLS"), QStringLiteral("STEUERUNG"), QStringLiteral("COMMANDES"), QStringLiteral("控制"), QStringLiteral("操作")),
        QStringLiteral("#00e676"),
        {refresh_button, fullscreen_button}));

    // --- Group 1: ADD-ONS (Purple/Magenta accent) ---
    statusBar()->insertPermanentWidget(1, createGroup(
        1, StormLang(QStringLiteral("ДОПОЛНЕНИЯ"), QStringLiteral("ADD-ONS"), QStringLiteral("ADD-ONS"), QStringLiteral("EXTENSIONS"), QStringLiteral("附加组件"), QStringLiteral("アドオン")),
        QStringLiteral("#e040fb"),
        {addons_status_button, gamefix_status_button}));

    // --- Group 2: RENDER (Cyan accent) ---
    statusBar()->insertPermanentWidget(2, createGroup(
        2, StormLang(QStringLiteral("РЕНДЕР"), QStringLiteral("RENDER"), QStringLiteral("RENDER"), QStringLiteral("RENDU"), QStringLiteral("渲染"), QStringLiteral("レンダー")),
        QStringLiteral("#00e5ff"),
        {renderer_status_button, gpu_accuracy_button, cpu_accuracy_button, vsync_mode_button, dma_accuracy_button, gpu_fence_button, nvdec_status_button}));

    // --- Group 3: GRAPHICS (Yellow accent) ---
    statusBar()->insertPermanentWidget(3, createGroup(
        3, StormLang(QStringLiteral("ГРАФИКА"), QStringLiteral("GRAPHICS"), QStringLiteral("GRAFIK"), QStringLiteral("GRAPHISMES"), QStringLiteral("图形"), QStringLiteral("グラフィックス")),
        QStringLiteral("#ffca28"),
        {aa_status_button, filter_status_button, shader_status_button, aspect_ratio_button, res_scale_button, vram_mode_button, anisotropy_button, disk_cache_button}));

    // --- Group 4: ASTC (Pink accent) ---
    statusBar()->insertPermanentWidget(4, createGroup(
        4, QStringLiteral("ASTC"), QStringLiteral("#ff4081"),
        {astc_decode_button, astc_recompress_button}));

    // --- Group 5: MODE (Purple accent) ---
    statusBar()->insertPermanentWidget(5, createGroup(
        5, StormLang(QStringLiteral("РЕЖИМ"), QStringLiteral("MODE"), QStringLiteral("MODUS"), QStringLiteral("MODE"), QStringLiteral("模式"), QStringLiteral("モード")),
        QStringLiteral("#b388ff"),
        {dock_status_button, airplane_mode_button, speed_limit_button, volume_button, mute_button}));

    // --- Group 6: SYSTEM (Orange accent) ---
    auto* sys_group = createGroup(
        6, StormLang(QStringLiteral("СИСТЕМА"), QStringLiteral("SYSTEM"), QStringLiteral("SYSTEM"), QStringLiteral("SYSTÈME"), QStringLiteral("系统"), QStringLiteral("システム")),
        QStringLiteral("#ff9100"),
        {firmware_label});
    sys_group->setMinimumWidth(90);
    firmware_label->setMinimumWidth(80);
    statusBar()->insertPermanentWidget(6, sys_group);

    // --- Group 7: NETWORK (Blue/Cyan accent) ---
    auto* net_group = createGroup(
        7, StormLang(QStringLiteral("СЕТЬ"), QStringLiteral("NETWORK"), QStringLiteral("NETZWERK"), QStringLiteral("RÉSEAU"), QStringLiteral("网络"), QStringLiteral("ネットワーク")),
        QStringLiteral("#00b0ff"),
        {multiplayer_state->GetStatusIcon(), multiplayer_state->GetStatusText()});
    net_group->setMinimumWidth(90);
    statusBar()->insertPermanentWidget(7, net_group);

    statusBar()->insertPermanentWidget(8, footer_customize_button);
    statusBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(statusBar(), &QStatusBar::customContextMenuRequested, this, &MainWindow::ShowFooterCustomizeMenu);

    LoadFooterSettings();
    UpdateStatusButtons();

    statusBar()->setVisible(true);
    setStyleSheet(QStringLiteral("QStatusBar::item{border: none;}"));
}

void MainWindow::InitializeDebugWidgets() {
    QMenu* debug_menu = ui->menu_View_Debugging;

    controller_dialog = new ControllerDialog(QtCommon::system->HIDCore(), input_subsystem, this);
    controller_dialog->hide();
    debug_menu->addAction(controller_dialog->toggleViewAction());
}

void MainWindow::InitializeRecentFileMenuActions() {
    for (int i = 0; i < max_recent_files_item; ++i) {
        actions_recent_files[i] = new QAction(this);
        actions_recent_files[i]->setVisible(false);
        connect(actions_recent_files[i], &QAction::triggered, this, &MainWindow::OnMenuRecentFile);

        ui->menu_recent_files->addAction(actions_recent_files[i]);
    }
    ui->menu_recent_files->addSeparator();
    QAction* action_clear_recent_files = new QAction(this);
    action_clear_recent_files->setText(tr("&Clear Recent Files"));
    connect(action_clear_recent_files, &QAction::triggered, this, [this] {
        UISettings::values.recent_files.clear();
        UpdateRecentFiles();
    });
    ui->menu_recent_files->addAction(action_clear_recent_files);

    UpdateRecentFiles();
}

static QIcon CreateVectorMenuIcon(const QString& icon_type, const QColor& accent_color) {
    QPixmap pixmap(18, 18);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QPen pen(accent_color, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    if (icon_type == QStringLiteral("nand") || icon_type == QStringLiteral("save")) {
        QPainterPath path;
        path.moveTo(3, 2);
        path.lineTo(12, 2);
        path.lineTo(15, 5);
        path.lineTo(15, 16);
        path.lineTo(3, 16);
        path.closeSubpath();
        painter.drawPath(path);
        painter.drawRect(5, 2, 6, 4);
        painter.fillRect(5, 9, 8, 5, accent_color);
    } else if (icon_type == QStringLiteral("folder") || icon_type == QStringLiteral("folder_open")) {
        QPainterPath path;
        path.moveTo(2, 4);
        path.lineTo(6, 4);
        path.lineTo(8, 6);
        path.lineTo(16, 6);
        path.lineTo(16, 15);
        path.lineTo(2, 15);
        path.closeSubpath();
        painter.setBrush(accent_color.lighter(110));
        painter.drawPath(path);
        painter.setBrush(Qt::NoBrush);
        if (icon_type == QStringLiteral("folder_open")) {
            painter.setPen(QPen(Qt::white, 1.4));
            painter.drawLine(5, 11, 13, 11);
            painter.drawLine(9, 8, 9, 14);
        }
    } else if (icon_type == QStringLiteral("clock")) {
        painter.drawEllipse(2, 2, 14, 14);
        painter.drawLine(9, 9, 9, 5);
        painter.drawLine(9, 9, 13, 9);
    } else if (icon_type == QStringLiteral("tag") || icon_type == QStringLiteral("amiibo")) {
        QPainterPath path;
        path.moveTo(3, 8);
        path.lineTo(8, 3);
        path.lineTo(15, 3);
        path.lineTo(15, 15);
        path.lineTo(3, 15);
        path.closeSubpath();
        painter.drawPath(path);
        painter.drawEllipse(10, 5, 2, 2);
    } else if (icon_type == QStringLiteral("globe") || icon_type == QStringLiteral("web")) {
        painter.drawEllipse(2, 2, 14, 14);
        painter.drawLine(2, 9, 16, 9);
        painter.drawEllipse(5, 2, 8, 14);
    } else if (icon_type == QStringLiteral("door") || icon_type == QStringLiteral("exit")) {
        painter.drawRect(2, 2, 8, 14);
        painter.drawLine(10, 9, 16, 9);
        painter.drawLine(13, 6, 16, 9);
        painter.drawLine(13, 12, 16, 9);
    } else if (icon_type == QStringLiteral("pause")) {
        painter.fillRect(4, 3, 3, 12, accent_color);
        painter.fillRect(11, 3, 3, 12, accent_color);
    } else if (icon_type == QStringLiteral("stop")) {
        painter.fillRect(3, 3, 12, 12, accent_color);
    } else if (icon_type == QStringLiteral("play")) {
        QPolygon polygon;
        polygon << QPoint(5, 3) << QPoint(14, 9) << QPoint(5, 15);
        painter.setBrush(accent_color);
        painter.drawPolygon(polygon);
    } else if (icon_type == QStringLiteral("refresh") || icon_type == QStringLiteral("restart")) {
        painter.drawArc(2, 2, 14, 14, 45 * 16, 250 * 16);
        painter.drawLine(14, 5, 16, 9);
        painter.drawLine(11, 8, 16, 9);
    } else if (icon_type == QStringLiteral("gear") || icon_type == QStringLiteral("config")) {
        painter.drawEllipse(6, 6, 6, 6);
        for (int i = 0; i < 8; ++i) {
            painter.save();
            painter.translate(9, 9);
            painter.rotate(i * 45);
            painter.fillRect(-1, -7, 2, 3, accent_color);
            painter.restore();
        }
    } else if (icon_type == QStringLiteral("gamepad")) {
        painter.drawRoundedRect(2, 4, 14, 10, 3, 3);
        painter.drawLine(5, 9, 8, 9);
        painter.drawLine(6.5, 7.5, 6.5, 10.5);
        painter.drawPoint(12, 7.5);
        painter.drawPoint(13.5, 9);
        painter.drawPoint(12, 10.5);
        painter.drawPoint(10.5, 9);
    } else if (icon_type == QStringLiteral("fullscreen")) {
        painter.drawLine(2, 6, 2, 2);
        painter.drawLine(2, 2, 6, 2);
        painter.drawLine(16, 6, 16, 2);
        painter.drawLine(16, 2, 12, 2);
        painter.drawLine(2, 12, 2, 16);
        painter.drawLine(2, 16, 6, 16);
        painter.drawLine(16, 12, 16, 16);
        painter.drawLine(16, 16, 12, 16);
    } else if (icon_type == QStringLiteral("window")) {
        painter.drawRect(2, 3, 14, 12);
        painter.drawLine(2, 6, 16, 6);
    } else if (icon_type == QStringLiteral("display") || icon_type == QStringLiteral("screen")) {
        painter.drawRect(2, 3, 14, 10);
        painter.drawLine(9, 13, 9, 16);
        painter.drawLine(6, 16, 12, 16);
    } else if (icon_type == QStringLiteral("grid")) {
        painter.drawRect(2, 2, 5, 5);
        painter.drawRect(11, 2, 5, 5);
        painter.drawRect(2, 11, 5, 5);
        painter.drawRect(11, 11, 5, 5);
    } else if (icon_type == QStringLiteral("list") || icon_type == QStringLiteral("tree")) {
        painter.drawLine(6, 4, 16, 4);
        painter.drawLine(6, 9, 16, 9);
        painter.drawLine(6, 14, 16, 14);
        painter.drawEllipse(2, 3, 2, 2);
        painter.drawEllipse(2, 8, 2, 2);
        painter.drawEllipse(2, 13, 2, 2);
    } else if (icon_type == QStringLiteral("carousel")) {
        painter.drawRect(5, 2, 8, 14);
        painter.drawLine(2, 4, 2, 14);
        painter.drawLine(16, 4, 16, 14);
    } else if (icon_type == QStringLiteral("search")) {
        painter.drawEllipse(3, 3, 8, 8);
        painter.drawLine(10, 10, 15, 15);
    } else if (icon_type == QStringLiteral("chart") || icon_type == QStringLiteral("stats")) {
        painter.fillRect(3, 10, 3, 5, accent_color);
        painter.fillRect(7, 6, 3, 9, accent_color);
        painter.fillRect(11, 3, 3, 12, accent_color);
    } else if (icon_type == QStringLiteral("key")) {
        painter.drawEllipse(3, 6, 5, 5);
        painter.drawLine(8, 8.5, 15, 8.5);
        painter.drawLine(12, 8.5, 12, 11.5);
        painter.drawLine(15, 8.5, 15, 11);
    } else if (icon_type == QStringLiteral("check") || icon_type == QStringLiteral("verify")) {
        QPainterPath path;
        path.moveTo(9, 2);
        path.lineTo(15, 4);
        path.lineTo(15, 10);
        path.quadTo(15, 15, 9, 16.5);
        path.quadTo(3, 15, 3, 10);
        path.lineTo(3, 4);
        path.closeSubpath();
        painter.drawPath(path);
        painter.drawLine(6, 9, 8, 12);
        painter.drawLine(8, 12, 13, 6);
    } else if (icon_type == QStringLiteral("database") || icon_type == QStringLiteral("data")) {
        painter.drawEllipse(3, 3, 12, 4);
        painter.drawArc(3, 6, 12, 4, 180 * 16, 180 * 16);
        painter.drawArc(3, 10, 12, 4, 180 * 16, 180 * 16);
        painter.drawLine(3, 5, 3, 12);
        painter.drawLine(15, 5, 15, 12);
    } else if (icon_type == QStringLiteral("edit") || icon_type == QStringLiteral("pencil")) {
        painter.drawLine(3, 15, 5, 15);
        painter.drawLine(3, 15, 3, 13);
        painter.drawLine(4, 12, 12, 4);
        painter.drawLine(6, 14, 14, 6);
        painter.drawLine(12, 4, 14, 6);
    } else if (icon_type == QStringLiteral("trash")) {
        painter.drawLine(2, 4, 16, 4);
        painter.drawLine(6, 2, 12, 2);
        painter.drawRect(4, 5, 10, 11);
        painter.drawLine(7, 8, 7, 13);
        painter.drawLine(11, 8, 11, 13);
    } else if (icon_type == QStringLiteral("applet")) {
        painter.drawRoundedRect(2, 2, 5, 5, 1, 1);
        painter.drawRoundedRect(11, 2, 5, 5, 1, 1);
        painter.drawRoundedRect(2, 11, 5, 5, 1, 1);
        painter.drawRoundedRect(11, 11, 5, 5, 1, 1);
    } else if (icon_type == QStringLiteral("photo")) {
        painter.drawRect(2, 2, 14, 14);
        painter.drawEllipse(5, 5, 2, 2);
        painter.drawLine(3, 13, 7, 9);
        painter.drawLine(7, 9, 11, 12);
        painter.drawLine(11, 12, 15, 8);
    } else if (icon_type == QStringLiteral("user") || icon_type == QStringLiteral("mii")) {
        painter.drawEllipse(6, 3, 6, 6);
        painter.drawArc(3, 10, 12, 8, 0, 180 * 16);
    } else if (icon_type == QStringLiteral("home")) {
        QPainterPath path;
        path.moveTo(9, 3);
        path.lineTo(15, 8);
        path.lineTo(13, 8);
        path.lineTo(13, 15);
        path.lineTo(5, 15);
        path.lineTo(5, 8);
        path.lineTo(3, 8);
        path.closeSubpath();
        painter.drawPath(path);
        painter.drawRect(7, 10, 4, 5);
    } else if (icon_type == QStringLiteral("link") || icon_type == QStringLiteral("shortcut")) {
        painter.drawRoundedRect(3, 6, 7, 6, 3, 3);
        painter.drawRoundedRect(8, 6, 7, 6, 3, 3);
    } else if (icon_type == QStringLiteral("camera") || icon_type == QStringLiteral("screenshot")) {
        painter.drawRoundedRect(2, 5, 14, 11, 2, 2);
        painter.drawRect(6, 3, 6, 2);
        painter.drawEllipse(6, 7, 6, 6);
    } else if (icon_type == QStringLiteral("translate")) {
        painter.drawRoundedRect(2, 2, 9, 9, 2, 2);
        painter.drawRoundedRect(7, 7, 9, 9, 2, 2);
        QFont f = painter.font();
        f.setPixelSize(7);
        f.setBold(true);
        painter.setFont(f);
        painter.drawText(QRect(2, 2, 9, 9), Qt::AlignCenter, QStringLiteral("A"));
        painter.drawText(QRect(7, 7, 9, 9), Qt::AlignCenter, QStringLiteral("文"));
    } else if (icon_type == QStringLiteral("lightning") || icon_type == QStringLiteral("tas")) {
        QPolygon poly;
        poly << QPoint(10, 2) << QPoint(4, 9) << QPoint(9, 9) << QPoint(8, 16) << QPoint(14, 8) << QPoint(9, 8);
        painter.setBrush(accent_color);
        painter.drawPolygon(poly);
    } else if (icon_type == QStringLiteral("record")) {
        painter.drawEllipse(2, 2, 14, 14);
        painter.setBrush(QColor(239, 68, 68));
        painter.drawEllipse(5, 5, 8, 8);
    } else if (icon_type == QStringLiteral("plus") || icon_type == QStringLiteral("add")) {
        painter.drawLine(9, 3, 9, 15);
        painter.drawLine(3, 9, 15, 9);
    } else if (icon_type == QStringLiteral("users") || icon_type == QStringLiteral("lobby")) {
        painter.drawEllipse(6, 3, 5, 5);
        painter.drawArc(3, 9, 10, 7, 0, 180 * 16);
        painter.drawEllipse(12, 5, 4, 4);
        painter.drawArc(10, 10, 7, 6, 0, 180 * 16);
    } else if (icon_type == QStringLiteral("book") || icon_type == QStringLiteral("guide")) {
        painter.drawLine(9, 3, 9, 15);
        painter.drawArc(3, 3, 12, 4, 180 * 16, 180 * 16);
        painter.drawLine(3, 5, 3, 15);
        painter.drawLine(15, 5, 15, 15);
        painter.drawArc(3, 13, 12, 4, 180 * 16, 180 * 16);
    } else if (icon_type == QStringLiteral("faq") || icon_type == QStringLiteral("question")) {
        painter.drawEllipse(2, 2, 14, 14);
        QFont f = painter.font();
        f.setPixelSize(11);
        f.setBold(true);
        painter.setFont(f);
    } else if (icon_type == QStringLiteral("puzzle") || icon_type == QStringLiteral("mod")) {
        painter.drawRoundedRect(3, 3, 12, 12, 2, 2);
        painter.drawEllipse(7, 1, 4, 4);
        painter.drawEllipse(13, 7, 4, 4);
        painter.drawLine(9, 3, 9, 15);
        painter.drawLine(3, 9, 15, 9);
    } else if (icon_type == QStringLiteral("info") || icon_type == QStringLiteral("about")) {
        painter.drawEllipse(2, 2, 14, 14);
        QFont f = painter.font();
        f.setPixelSize(11);
        f.setBold(true);
        painter.setFont(f);
        painter.drawText(QRect(0, 0, 18, 18), Qt::AlignCenter, QStringLiteral("i"));
    } else if (icon_type == QStringLiteral("download")) {
        painter.drawLine(9, 3, 9, 12);
        painter.drawLine(6, 9, 9, 12);
        painter.drawLine(12, 9, 9, 12);
        painter.drawLine(4, 15, 14, 15);
    } else {
        painter.drawRect(5, 5, 8, 8);
    }

    return QIcon(pixmap);
}

void MainWindow::SetupMenuIcons() {
    QFont menu_font = ui->menubar->font();
    menu_font.setBold(true);
    ui->menubar->setFont(menu_font);

    auto clean_action_text = [](QAction* act) {
        if (!act) return;
        QString text = act->text();
        static const QRegularExpression emoji_re(QStringLiteral(R"(^[\p{Emoji}\p{Symbol}\s]+)"), QRegularExpression::UseUnicodePropertiesOption);
        text.remove(emoji_re);
        act->setText(text.trimmed());
    };

    auto apply_action = [&](QAction* act, const QString& icon_type, const QColor& color) {
        if (!act) return;
        clean_action_text(act);
        act->setIcon(CreateVectorMenuIcon(icon_type, color));
    };

    auto apply_menu = [&](QMenu* menu, const QString& icon_type, const QColor& color) {
        if (!menu) return;
        clean_action_text(menu->menuAction());
        menu->menuAction()->setIcon(CreateVectorMenuIcon(icon_type, color));
    };

    // Color definitions (vibrant standalone palette matching Screenshot 1)
    const QColor col_yellow(255, 193, 7);
    const QColor col_amber(255, 167, 38);
    const QColor col_blue(66, 165, 245);
    const QColor col_cyan(0, 229, 255);
    const QColor col_teal(38, 198, 218);
    const QColor col_green(102, 187, 106);
    const QColor col_lime(156, 204, 101);
    const QColor col_red(239, 83, 80);
    const QColor col_pink(236, 64, 122);
    const QColor col_purple(171, 71, 188);
    const QColor col_indigo(92, 107, 192);
    const QColor col_grey(176, 190, 197);

    // File Menu
    if (!quick_save_action) {
        ui->menu_File->insertSeparator(ui->action_Load_Amiibo);

        quick_save_action = new QAction(StormLang(
            QStringLiteral("Быстрое сохранение (Слот 1)"),
            QStringLiteral("Quick Save (Slot 1)"),
            QStringLiteral("Schnellspeichern (Slot 1)"),
            QStringLiteral("Sauvegarde rapide (Emplacement 1)"),
            QStringLiteral("快速保存 (槽位 1)"),
            QStringLiteral("クイックセーブ (スロット 1)")
        ), this);
        connect(quick_save_action, &QAction::triggered, this, &MainWindow::OnQuickSave);
        ui->menu_File->insertAction(ui->action_Load_Amiibo, quick_save_action);

        quick_load_action = new QAction(StormLang(
            QStringLiteral("Быстрая загрузка (Слот 1)"),
            QStringLiteral("Quick Load (Slot 1)"),
            QStringLiteral("Schnellladen (Slot 1)"),
            QStringLiteral("Chargement rapide (Emplacement 1)"),
            QStringLiteral("快速读取 (槽位 1)"),
            QStringLiteral("クイックロード (スロット 1)")
        ), this);
        connect(quick_load_action, &QAction::triggered, this, &MainWindow::OnQuickLoad);
        ui->menu_File->insertAction(ui->action_Load_Amiibo, quick_load_action);

        save_state_slots_menu = new QMenu(StormLang(
            QStringLiteral("Сохранить состояние в слот"),
            QStringLiteral("Save State to Slot"),
            QStringLiteral("Status in Slot speichern"),
            QStringLiteral("Sauvegarder l'état dans l'emplacement"),
            QStringLiteral("保存状态到槽位"),
            QStringLiteral("スロットにステートを保存")
        ), ui->menu_File);
        ui->menu_File->insertMenu(ui->action_Load_Amiibo, save_state_slots_menu);

        load_state_slots_menu = new QMenu(StormLang(
            QStringLiteral("Загрузить состояние из слота"),
            QStringLiteral("Load State from Slot"),
            QStringLiteral("Status aus Slot laden"),
            QStringLiteral("Charger l'état depuis l'emplacement"),
            QStringLiteral("从槽位读取状态"),
            QStringLiteral("スロットからステートをロード")
        ), ui->menu_File);
        ui->menu_File->insertMenu(ui->action_Load_Amiibo, load_state_slots_menu);

        for (int i = 0; i < 5; ++i) {
            const int slot_num = i + 1;
            save_slot_actions[i] = save_state_slots_menu->addAction(
                tr("Слот %1").arg(slot_num), this, [this, slot_num]() {
                    OnSaveStateSlot(slot_num);
                });
            load_slot_actions[i] = load_state_slots_menu->addAction(
                tr("Слот %1").arg(slot_num), this, [this, slot_num]() {
                    OnLoadStateSlot(slot_num);
                });
        }
        save_state_slots_menu->addSeparator();
        load_state_slots_menu->addSeparator();
        save_state_slots_menu->addAction(
            StormLang(QStringLiteral("🗂️ Менеджер быстрых сохранений..."),
                      QStringLiteral("🗂️ Save State Manager..."),
                      QStringLiteral("🗂️ Schnellspeicher-Manager..."),
                      QStringLiteral("🗂️ Gestionnaire de sauvegardes rapides..."),
                      QStringLiteral("🗂️ 快速保存管理器..."),
                      QStringLiteral("🗂️ クイックセーブマネージャー...")),
            this, &MainWindow::OnSaveStateManager);
        load_state_slots_menu->addAction(
            StormLang(QStringLiteral("🗂️ Менеджер быстрых сохранений..."),
                      QStringLiteral("🗂️ Save State Manager..."),
                      QStringLiteral("🗂️ Schnellspeicher-Manager..."),
                      QStringLiteral("🗂️ Gestionnaire de sauvegardes rapides..."),
                      QStringLiteral("🗂️ 快速保存管理器..."),
                      QStringLiteral("🗂️ クイックセーブマネージャー...")),
            this, &MainWindow::OnSaveStateManager);

        connect(save_state_slots_menu, &QMenu::aboutToShow, this, &MainWindow::RefreshSaveStateMenus);
        connect(load_state_slots_menu, &QMenu::aboutToShow, this, &MainWindow::RefreshSaveStateMenus);

        save_state_manager_action = new QAction(StormLang(
            QStringLiteral("Менеджер быстрых сохранений..."),
            QStringLiteral("Save State Manager..."),
            QStringLiteral("Schnellspeicher-Manager..."),
            QStringLiteral("Gestionnaire de sauvegardes rapides..."),
            QStringLiteral("快速保存管理器..."),
            QStringLiteral("クイックセーブマネージャー...")
        ), this);
        save_state_manager_action->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+M")));
        connect(save_state_manager_action, &QAction::triggered, this, &MainWindow::OnSaveStateManager);
        ui->menu_File->insertAction(ui->action_Load_Amiibo, save_state_manager_action);

        ui->menu_File->insertSeparator(ui->action_Load_Amiibo);
        RefreshSaveStateMenus();
    }

    apply_action(ui->action_Install_File_NAND, QStringLiteral("nand"), col_blue);
    apply_action(ui->action_Load_File, QStringLiteral("folder_open"), col_yellow);
    apply_action(ui->action_Load_Folder, QStringLiteral("folder"), col_yellow);
    apply_menu(ui->menu_recent_files, QStringLiteral("clock"), col_teal);
    apply_action(quick_save_action, QStringLiteral("save"), col_green);
    apply_action(quick_load_action, QStringLiteral("restart"), col_cyan);
    apply_menu(save_state_slots_menu, QStringLiteral("save"), col_teal);
    apply_menu(load_state_slots_menu, QStringLiteral("folder_open"), col_blue);
    apply_action(save_state_manager_action, QStringLiteral("database"), col_amber);
    apply_action(ui->action_Load_Amiibo, QStringLiteral("amiibo"), col_amber);
    apply_action(ui->action_Amiibo_Online_Database, QStringLiteral("globe"), col_cyan);
    apply_menu(ui->menuOpen_Eden_Folders, QStringLiteral("folder"), col_yellow);
    apply_action(ui->action_Root_Data_Folder, QStringLiteral("folder"), col_yellow);
    apply_action(ui->action_NAND_Folder, QStringLiteral("nand"), col_blue);
    apply_action(ui->action_SDMC_Folder, QStringLiteral("database"), col_amber);
    apply_action(ui->action_Mod_Folder, QStringLiteral("gear"), col_purple);
    apply_action(ui->action_Log_Folder, QStringLiteral("list"), col_grey);
    apply_action(ui->action_Exit, QStringLiteral("exit"), col_red);

    // Emulation Menu
    apply_action(ui->action_Pause, QStringLiteral("pause"), col_amber);
    apply_action(ui->action_Stop, QStringLiteral("stop"), col_red);
    apply_action(ui->action_Restart, QStringLiteral("restart"), col_cyan);
    apply_action(ui->action_Configure, QStringLiteral("gear"), col_amber);
    apply_action(ui->action_Configure_Current_Game, QStringLiteral("gamepad"), col_green);

    // View Menu
    apply_action(ui->action_Fullscreen, QStringLiteral("fullscreen"), col_indigo);
    apply_action(ui->action_Single_Window_Mode, QStringLiteral("window"), col_blue);
    apply_menu(ui->menu_Reset_Window_Size, QStringLiteral("display"), col_blue);
    apply_action(ui->action_Reset_Window_Size_720, QStringLiteral("display"), col_blue);
    apply_action(ui->action_Reset_Window_Size_900, QStringLiteral("display"), col_indigo);
    apply_action(ui->action_Reset_Window_Size_1080, QStringLiteral("display"), col_purple);
    apply_menu(ui->menu_View_Debugging, QStringLiteral("gear"), col_amber);
    apply_menu(ui->menu_Game_List_Mode, QStringLiteral("grid"), col_indigo);
    apply_action(ui->action_Tree_View, QStringLiteral("tree"), col_blue);
    apply_action(ui->action_Grid_View, QStringLiteral("grid"), col_indigo);
    apply_action(ui->action_Carousel_View, QStringLiteral("carousel"), col_pink);
    apply_menu(ui->menuGame_Icon_Size, QStringLiteral("stats"), col_cyan);
    apply_action(ui->action_Show_Filter_Bar, QStringLiteral("search"), col_cyan);
    apply_action(ui->action_Show_Status_Bar, QStringLiteral("stats"), col_lime);
    apply_action(ui->action_Show_Performance_Overlay, QStringLiteral("chart"), col_amber);
    apply_action(ui->action_Nand_Manager, QStringLiteral("database"), col_cyan);

    // Tools Menu
    apply_menu(ui->menuInstall_Keys, QStringLiteral("key"), col_cyan);
    apply_action(ui->action_Install_Keys_Online, QStringLiteral("network"), col_cyan);
    apply_action(ui->action_Install_Keys_From_ZIP, QStringLiteral("save"), col_blue);
    apply_action(ui->action_Install_Keys, QStringLiteral("file"), col_amber);
    apply_menu(ui->menuInstall_Firmware, QStringLiteral("nand"), col_purple);
    apply_action(ui->action_Firmware_Online, QStringLiteral("network"), col_cyan);
    apply_action(ui->action_Firmware_From_Folder, QStringLiteral("folder"), col_yellow);
    apply_action(ui->action_Firmware_From_ZIP, QStringLiteral("save"), col_blue);
    apply_action(ui->action_Verify_installed_contents, QStringLiteral("verify"), col_green);
    apply_action(ui->action_Data_Manager, QStringLiteral("data"), col_teal);
    apply_menu(ui->menu_cabinet_applet, QStringLiteral("edit"), col_cyan);
    apply_action(ui->action_Launch_Cabinet_Nickname_Owner, QStringLiteral("edit"), col_cyan);
    apply_action(ui->action_Launch_Cabinet_Eraser, QStringLiteral("trash"), col_red);
    apply_action(ui->action_Launch_Cabinet_Restorer, QStringLiteral("restart"), col_green);
    apply_action(ui->action_Launch_Cabinet_Formatter, QStringLiteral("lightning"), col_amber);
    apply_menu(ui->menu_Applets, QStringLiteral("applet"), col_purple);
    apply_action(ui->action_Launch_PhotoViewer, QStringLiteral("photo"), col_purple);
    apply_action(ui->action_Launch_MiiEdit, QStringLiteral("mii"), col_indigo);
    apply_action(ui->action_Launch_Controller, QStringLiteral("gamepad"), col_green);
    apply_action(ui->action_Launch_QLaunch, QStringLiteral("home"), col_teal);
    apply_action(ui->action_Enable_Overlay_Applet, QStringLiteral("window"), col_cyan);
    apply_menu(ui->menu_Create_Shortcuts, QStringLiteral("shortcut"), col_blue);
    apply_action(ui->action_Desktop, QStringLiteral("display"), col_blue);
    apply_action(ui->action_Application_Menu, QStringLiteral("list"), col_indigo);
    apply_action(ui->action_Capture_Screenshot, QStringLiteral("screenshot"), col_pink);
    apply_action(ui->action_Translate_Screen, QStringLiteral("translate"), col_cyan);
    apply_action(ui->action_Mod_Manager, QStringLiteral("puzzle"), col_purple);
    apply_action(ui->action_Cheats, QStringLiteral("lightning"), col_cyan);
    apply_menu(ui->menuTAS, QStringLiteral("tas"), col_lime);
    apply_action(ui->action_TAS_Start, QStringLiteral("play"), col_green);
    apply_action(ui->action_TAS_Record, QStringLiteral("record"), col_red);
    apply_action(ui->action_TAS_Reset, QStringLiteral("restart"), col_amber);
    apply_action(ui->action_Configure_Tas, QStringLiteral("gear"), col_grey);

    if (!storm_games_world_action) {
        storm_games_world_action = new QAction(StormLang(
            QStringLiteral("Каталог и менеджер игр STORM GAMES WORLD..."),
            QStringLiteral("STORM GAMES WORLD Game Catalog and Manager..."),
            QStringLiteral("STORM GAMES WORLD Spielekatalog und Manager..."),
            QStringLiteral("Catalogue et gestionnaire de jeux STORM GAMES WORLD..."),
            QStringLiteral("STORM GAMES WORLD 游戏目录与管理器..."),
            QStringLiteral("STORM GAMES WORLD ゲームカタログとマネージャー...")
        ), this);
        storm_games_world_action->setShortcut(QKeySequence(QStringLiteral("Ctrl+G")));
        ui->menu_Tools->addAction(storm_games_world_action);
    }
    apply_action(storm_games_world_action, QStringLiteral("download"), col_green);

    if (!storm_save_sync_action) {
        storm_save_sync_action = new QAction(StormLang(
            QStringLiteral("Синхронизация сохранений (STORM SAVE SYNC)..."),
            QStringLiteral("Save Synchronization (STORM SAVE SYNC)..."),
            QStringLiteral("Spielstand-Synchronisierung (STORM SAVE SYNC)..."),
            QStringLiteral("Synchronisation des sauvegardes (STORM SAVE SYNC)..."),
            QStringLiteral("存档同步 (STORM SAVE SYNC)..."),
            QStringLiteral("セーブデータ同期 (STORM SAVE SYNC)...")
        ), this);
        storm_save_sync_action->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+S")));
        ui->menu_Tools->addAction(storm_save_sync_action);
    }
    apply_action(storm_save_sync_action, QStringLiteral("refresh"), col_cyan);

    if (!log_viewer_action) {
        log_viewer_action = new QAction(StormLang(
            QStringLiteral("Журнал работы (Логи)..."),
            QStringLiteral("Log Viewer (Logs)..."),
            QStringLiteral("Ereignisprotokoll (Logs)..."),
            QStringLiteral("Journal des opérations (Logs)..."),
            QStringLiteral("运行日志 (Logs)..."),
            QStringLiteral("動作ログ (Logs)...")
        ), this);
        log_viewer_action->setShortcut(QKeySequence(QStringLiteral("Ctrl+L")));
        ui->menu_Tools->addAction(log_viewer_action);
    }
    apply_action(log_viewer_action, QStringLiteral("book"), col_blue);

    if (!reset_gamefix_action) {
        reset_gamefix_action = new QAction(StormLang(
            QStringLiteral("Сбросить скрытые диалоги авто-исправлений..."),
            QStringLiteral("Reset Hidden Auto-Fix Dialogs..."),
            QStringLiteral("Ausgeblendete Auto-Fix-Dialoge zurücksetzen..."),
            QStringLiteral("Réinitialiser les dialogues d'auto-correction masqués..."),
            QStringLiteral("重置隐藏的自动修复对话框..."),
            QStringLiteral("非表示の自動修正ダイアログをリセット...")
        ), this);
        ui->menu_Tools->addAction(reset_gamefix_action);
        connect(reset_gamefix_action, &QAction::triggered, this, &MainWindow::OnResetGameFixSuppression);
    }
    apply_action(reset_gamefix_action, QStringLiteral("shield"), col_cyan);

    // Multiplayer Menu
    apply_action(ui->action_View_Lobby, QStringLiteral("globe"), col_cyan);
    apply_action(ui->action_Start_Room, QStringLiteral("plus"), col_green);
    apply_action(ui->action_Connect_To_Room, QStringLiteral("link"), col_blue);
    apply_action(ui->action_Show_Room, QStringLiteral("users"), col_indigo);
    apply_action(ui->action_Leave_Room, QStringLiteral("exit"), col_red);

    // Help Menu
    apply_action(ui->action_Check_Updates, QStringLiteral("refresh"), col_green);
    apply_action(ui->action_Open_Quickstart_Guide, QStringLiteral("guide"), col_blue);
    apply_action(ui->action_Open_FAQ, QStringLiteral("faq"), col_amber);
    apply_action(ui->action_Open_Mods_Page, QStringLiteral("gear"), col_purple);
    apply_action(ui->action_Eden_Dependencies, QStringLiteral("book"), col_teal);
    apply_action(ui->action_About, QStringLiteral("about"), col_cyan);
}

void MainWindow::LinkActionShortcut(QAction* action, const QString& action_name,
                                    const bool tas_allowed) {
    static const auto main_window = std::string("Main Window");
    action->setShortcut(hotkey_registry.GetKeySequence(main_window, action_name.toStdString()));
    action->setShortcutContext(
        hotkey_registry.GetShortcutContext(main_window, action_name.toStdString()));
    action->setAutoRepeat(false);

    this->addAction(action);

    auto* controller =
        QtCommon::system->HIDCore().GetEmulatedController(Core::HID::NpadIdType::Player1);
    const auto* controller_hotkey =
        hotkey_registry.GetControllerHotkey(main_window, action_name.toStdString(), controller);
    connect(
        controller_hotkey, &ControllerShortcut::Activated, this,
        [action, tas_allowed, this] {
            auto [tas_status, current_tas_frame, total_tas_frames] =
                input_subsystem->GetTas()->GetStatus();
            if (tas_allowed || tas_status == InputCommon::TasInput::TasState::Stopped) {
                action->trigger();
            }
        },
        Qt::QueuedConnection);
}

void MainWindow::InitializeHotkeys() {
    hotkey_registry.LoadHotkeys();

    LinkActionShortcut(ui->action_Load_File, QStringLiteral("Load File"));
    LinkActionShortcut(ui->action_Load_Amiibo, QStringLiteral("Load/Remove Amiibo"));
    if (quick_save_action) {
        LinkActionShortcut(quick_save_action, QStringLiteral("Quick Save"));
    }
    if (quick_load_action) {
        LinkActionShortcut(quick_load_action, QStringLiteral("Quick Load"));
    }
    LinkActionShortcut(ui->action_Exit, QStringLiteral("Exit STORM SWITCH"));
    LinkActionShortcut(ui->action_Restart, QStringLiteral("Restart Emulation"));
    LinkActionShortcut(ui->action_Pause, QStringLiteral("Continue/Pause Emulation"));
    LinkActionShortcut(ui->action_Stop, QStringLiteral("Stop Emulation"));
    LinkActionShortcut(ui->action_Show_Filter_Bar, QStringLiteral("Toggle Filter Bar"));
    LinkActionShortcut(ui->action_Show_Status_Bar, QStringLiteral("Toggle Status Bar"));
    LinkActionShortcut(ui->action_Show_Performance_Overlay,
                       QStringLiteral("Toggle Performance Overlay"));
    LinkActionShortcut(ui->action_Fullscreen, QStringLiteral("Fullscreen"));
    LinkActionShortcut(ui->action_Capture_Screenshot, QStringLiteral("Capture Screenshot"));
    LinkActionShortcut(ui->action_Translate_Screen, QStringLiteral("Translate Screen"));
    LinkActionShortcut(ui->action_TAS_Start, QStringLiteral("TAS Start/Stop"), true);
    LinkActionShortcut(ui->action_TAS_Record, QStringLiteral("TAS Record"), true);
    LinkActionShortcut(ui->action_TAS_Reset, QStringLiteral("TAS Reset"), true);
    LinkActionShortcut(ui->action_View_Lobby, QStringLiteral("Browse Public Game Lobby"));
    LinkActionShortcut(ui->action_Start_Room, QStringLiteral("Create Room"));
    LinkActionShortcut(ui->action_Connect_To_Room, QStringLiteral("Direct Connect to Room"));
    LinkActionShortcut(ui->action_Show_Room, QStringLiteral("Show Current Room"));
    LinkActionShortcut(ui->action_Leave_Room, QStringLiteral("Leave Room"));
    LinkActionShortcut(ui->action_Configure, QStringLiteral("Configure"));
    LinkActionShortcut(ui->action_Configure_Current_Game, QStringLiteral("Configure Current Game"));

    auto tesla_toggle = [this]() {
        if (!perf_overlay) return;
        if (!perf_overlay->isVisible()) {
            perf_overlay->SetExpanded(true);
            ui->action_Show_Performance_Overlay->setChecked(true);
            perf_overlay->show();
        } else if (!perf_overlay->IsExpanded()) {
            perf_overlay->SetExpanded(true);
        } else {
            ui->action_Show_Performance_Overlay->setChecked(false);
            perf_overlay->hide();
        }
    };

    auto* tesla_f6 = new QShortcut(QKeySequence(Qt::Key_F6), this);
    connect(tesla_f6, &QShortcut::activated, this, tesla_toggle);

    auto* tesla_ctrl_f1 = new QShortcut(QKeySequence(QStringLiteral("Ctrl+F1")), this);
    connect(tesla_ctrl_f1, &QShortcut::activated, this, tesla_toggle);

    // Quick Save (F5) and Quick Load (F7) are managed via HotkeyRegistry & LinkActionShortcut

    static const QString main_window = QStringLiteral("Main Window");
    const auto connect_shortcut = [&]<typename Fn>(const QString& action_name, const Fn& function) {
        const auto* hotkey =
            hotkey_registry.GetHotkey(main_window.toStdString(), action_name.toStdString(), this);
        auto* controller =
            QtCommon::system->HIDCore().GetEmulatedController(Core::HID::NpadIdType::Player1);
        const auto* controller_hotkey = hotkey_registry.GetControllerHotkey(
            main_window.toStdString(), action_name.toStdString(), controller);
        connect(hotkey, &QShortcut::activated, this, function);
        connect(controller_hotkey, &ControllerShortcut::Activated, this, function,
                Qt::QueuedConnection);
    };

    connect_shortcut(QStringLiteral("Exit Fullscreen"), [&] {
        if (emulation_running && ui->action_Fullscreen->isChecked()) {
            ui->action_Fullscreen->setChecked(false);
            ToggleFullscreen();
        }
    });
    connect_shortcut(QStringLiteral("Change Adapting Filter"), &MainWindow::OnToggleAdaptingFilter);
    connect_shortcut(QStringLiteral("Change Docked Mode"), &MainWindow::OnToggleDockedMode);
    connect_shortcut(QStringLiteral("Change GPU Mode"), &MainWindow::OnToggleGpuAccuracy);
    connect_shortcut(QStringLiteral("Audio Mute/Unmute"), &MainWindow::OnMute);
    connect_shortcut(QStringLiteral("Audio Volume Down"), &MainWindow::OnDecreaseVolume);
    connect_shortcut(QStringLiteral("Audio Volume Up"), &MainWindow::OnIncreaseVolume);

    connect_shortcut(QStringLiteral("Toggle Framerate Limit"), [this] {
        Settings::ToggleStandardMode();
        SetFPSSuffix();
    });

    connect_shortcut(QStringLiteral("Toggle Turbo Speed"), [this] {
        Settings::ToggleTurboMode();
        SetFPSSuffix();
    });

    connect_shortcut(QStringLiteral("Toggle Slow Speed"), [this] {
        Settings::ToggleSlowMode();
        SetFPSSuffix();
    });

    connect_shortcut(QStringLiteral("Toggle Renderdoc Capture"), [] {
        if (Settings::values.enable_renderdoc_hotkey) {
            QtCommon::system->GetRenderdocAPI().ToggleCapture();
        }
    });
    connect_shortcut(QStringLiteral("Toggle Mouse Panning"), [&] {
        Settings::values.mouse_panning = !Settings::values.mouse_panning;
        if (Settings::values.mouse_panning) {
            render_window->installEventFilter(render_window);
            render_window->setAttribute(Qt::WA_Hover, true);
        }
    });
}

void MainWindow::SetDefaultUIGeometry() {
    // geometry: 53% of the window contents are in the upper screen half, 47% in the lower half
    const QRect screenRect = QGuiApplication::primaryScreen()->geometry();

    const int w = screenRect.width() * 2 / 3;
    const int h = screenRect.height() * 2 / 3;
    const int x = (screenRect.x() + screenRect.width()) / 2 - w / 2;
    const int y = (screenRect.y() + screenRect.height()) / 2 - h * 53 / 100;

    setGeometry(x, y, w, h);
}

void MainWindow::RestoreUIState() {
    setWindowFlags(windowFlags() & ~Qt::FramelessWindowHint);
    restoreGeometry(UISettings::values.geometry);
    // Work-around because the games list isn't supposed to be full screen
    if (isFullScreen()) {
        showNormal();
    }
    restoreState(UISettings::values.state);
    render_window->setWindowFlags(render_window->windowFlags() & ~Qt::FramelessWindowHint);
    render_window->restoreGeometry(UISettings::values.renderwindow_geometry);

    game_list->LoadInterfaceLayout();

    ui->action_Single_Window_Mode->setChecked(UISettings::values.single_window_mode.GetValue());
    ToggleWindowMode();

    ui->action_Fullscreen->setChecked(UISettings::values.fullscreen.GetValue());

    ui->action_Enable_Overlay_Applet->setChecked(Settings::values.enable_overlay.GetValue());

    ui->action_Show_Filter_Bar->setChecked(UISettings::values.show_filter_bar.GetValue());
    game_list->SetFilterVisible(ui->action_Show_Filter_Bar->isChecked());

    ui->action_Show_Status_Bar->setChecked(UISettings::values.show_status_bar.GetValue());
    statusBar()->setVisible(ui->action_Show_Status_Bar->isChecked());

    ui->action_Show_Performance_Overlay->setChecked(
        UISettings::values.show_perf_overlay.GetValue());
    if (perf_overlay)
        perf_overlay->setVisible(ui->action_Show_Performance_Overlay->isChecked());
    Debugger::ToggleConsole();
}

void MainWindow::OnAppFocusStateChanged(Qt::ApplicationState state) {
    if (state != Qt::ApplicationHidden && state != Qt::ApplicationInactive &&
        state != Qt::ApplicationActive) {
        LOG_DEBUG(Frontend, "ApplicationState unusual flag: {} ", state);
    }
    if (!emulation_running) {
        return;
    }
    if (UISettings::values.pause_when_in_background) {
        if (QtCommon::emu_thread->IsRunning() &&
            (state & (Qt::ApplicationHidden | Qt::ApplicationInactive))) {
            auto_paused = true;
            OnPauseGame();
        } else if (!QtCommon::emu_thread->IsRunning() && auto_paused &&
                   (state & Qt::ApplicationActive)) {
            auto_paused = false;
            OnStartGame();
        }
    }
    if (UISettings::values.mute_when_in_background) {
        if (!Settings::values.audio_muted.GetValue() &&
            (state & (Qt::ApplicationHidden | Qt::ApplicationInactive))) {
            Settings::values.audio_muted.SetValue(true);
            auto_muted = true;
        } else if (auto_muted && (state & Qt::ApplicationActive)) {
            Settings::values.audio_muted.SetValue(false);
            auto_muted = false;
        }
        UpdateVolumeUI();
        UpdateMuteButton();
    }
}

void MainWindow::ConnectWidgetEvents() {
    connect(game_list, &GameList::BootGame, this, &MainWindow::BootGameFromList);
    connect(game_list, &GameList::GameSelected, this, [this](u64 program_id, const QString& game_path) {
        if (program_id != 0) {
            m_current_addons_title_id = program_id;
            m_current_addons_game_path = game_path.toStdString();
            m_current_addons_game_name = QFileInfo(game_path).completeBaseName();
            UpdateAddonsStatusButton(m_current_addons_title_id, m_current_addons_game_name);
        }
    });
    connect(game_list, &GameList::GameChosen, this, [this](const QString& game_path, u64 program_id) {
        m_current_addons_game_path = game_path.toStdString();
        UpdateAddonsStatusButton(program_id, QFileInfo(game_path).completeBaseName());
        OnGameListLoadFile(game_path, program_id);
    });
    connect(game_list, &GameList::OpenDirectory, this, &MainWindow::OnGameListOpenDirectory);
    connect(game_list, &GameList::OpenFolderRequested, this, &MainWindow::OnGameListOpenFolder);
    connect(game_list, &GameList::OpenSaveSyncRequested, this,
            [this](u64 program_id) { OnOpenStormSaveSync(program_id); });
    connect(game_list, &GameList::OpenGameFixRequested, this,
            [this](u64 program_id, const QString& game_path) {
                ShowGameFixDialog(program_id, game_path, true /* force_show */);
            });
    connect(game_list, &GameList::OpenModManagerRequested, this,
            [this](u64 program_id, const QString& game_path) {
                const QString game_name = QFileInfo(game_path).completeBaseName();
                ModManagerDialog dialog(this, *QtCommon::system, program_id, game_path, game_name);
                dialog.exec();
            });
    connect(game_list, &GameList::OpenCheatsRequested, this,
            [this](u64 program_id, const QString& game_path) {
                CheatsDialog dialog(this, *QtCommon::system, program_id, game_path);
                dialog.exec();
            });
    connect(game_list, &GameList::OpenTransferableShaderCacheRequested, this,
            [this](u64 program_id) { QtCommon::Path::OpenShaderCache(program_id, this); });
    connect(game_list, &GameList::RemoveInstalledEntryRequested, this,
            &MainWindow::OnGameListRemoveInstalledEntry);
    connect(game_list, &GameList::RemoveFileRequested, this, &MainWindow::OnGameListRemoveFile);
    connect(game_list, &GameList::RemovePlayTimeRequested, this,
            &MainWindow::OnGameListRemovePlayTimeData);
    connect(game_list, &GameList::SetPlayTimeRequested, this, &MainWindow::OnGameListSetPlayTime);
    connect(game_list, &GameList::DumpRomFSRequested, this, &MainWindow::OnGameListDumpRomFS);
    connect(game_list, &GameList::VerifyIntegrityRequested, this,
            &MainWindow::OnGameListVerifyIntegrity);
    connect(game_list, &GameList::CopyTIDRequested, this, &MainWindow::OnGameListCopyTID);
    connect(game_list, &GameList::CreateShortcut, this, &MainWindow::OnGameListCreateShortcut);
    connect(game_list, &GameList::AddDirectory, this, &MainWindow::OnGameListAddDirectory);
    connect(game_list_placeholder, &GameListPlaceholder::AddDirectory, this,
            &MainWindow::OnGameListAddDirectory);
    connect(game_list, &GameList::ShowList, this, &MainWindow::OnGameListShowList);
    connect(game_list, &GameList::PopulatingCompleted,
            [this] { multiplayer_state->UpdateGameList(game_list->GetModel()); });
    connect(game_list, &GameList::SaveConfig, this, &MainWindow::OnSaveConfig);

    connect(game_list, &GameList::OpenPerGameGeneralRequested, this,
            &MainWindow::OnGameListOpenPerGameProperties);
    connect(game_list, &GameList::LinkToRyujinxRequested, this, &MainWindow::OnLinkToRyujinx);

    connect(this, &MainWindow::UpdateInstallProgress, this, &MainWindow::IncrementInstallProgress);

    // Software Keyboard Applet
    connect(this, &MainWindow::EmulationStarting, this, &MainWindow::SoftwareKeyboardExit);
    connect(this, &MainWindow::EmulationStopping, this, &MainWindow::SoftwareKeyboardExit);

    connect(&status_bar_update_timer, &QTimer::timeout, this, &MainWindow::UpdateStatusBar);

    connect(this, &MainWindow::UpdateThemedIcons, multiplayer_state,
            &MultiplayerState::UpdateThemedIcons);
}

void MainWindow::ConnectMenuEvents() {
    const auto connect_menu = [&]<typename Fn>(QAction* action, const Fn& event_fn) {
        connect(action, &QAction::triggered, this, event_fn);
        // Add actions to this window so that hiding menus in fullscreen won't disable them
        addAction(action);
        // Add actions to the render window so that they work outside of single window mode
        render_window->addAction(action);
    };

    // File
    connect_menu(ui->action_Load_File, &MainWindow::OnMenuLoadFile);
    connect_menu(ui->action_Load_Folder, &MainWindow::OnMenuLoadFolder);
    connect_menu(ui->action_Install_File_NAND, &MainWindow::OnMenuInstallToNAND);
    connect_menu(ui->action_Exit, &QMainWindow::close);
    connect_menu(ui->action_Load_Amiibo, &MainWindow::OnLoadAmiibo);

    // Emulation
    connect_menu(ui->action_Pause, &MainWindow::OnPauseContinueGame);
    connect_menu(ui->action_Stop, &MainWindow::OnStopGame);
    connect_menu(ui->action_Open_Mods_Page, &MainWindow::OnOpenModsPage);
    connect_menu(ui->action_Open_Quickstart_Guide, &MainWindow::OnOpenQuickstartGuide);
    connect_menu(ui->action_Open_FAQ, &MainWindow::OnOpenFAQ);
    connect_menu(ui->action_Restart, &MainWindow::OnRestartGame);
    connect_menu(ui->action_Configure, &MainWindow::OnConfigure);
    connect_menu(ui->action_Configure_Current_Game, &MainWindow::OnConfigurePerGame);
    connect_menu(ui->action_Mod_Manager, &MainWindow::OnModManagerDialog);
    connect_menu(ui->action_Cheats, &MainWindow::OnCheatsDialog);
    connect_menu(ui->action_Nand_Manager, &MainWindow::OnNandManagerDialog);

    // View
    connect_menu(ui->action_Fullscreen, &MainWindow::ToggleFullscreen);
    connect_menu(ui->action_Single_Window_Mode, &MainWindow::ToggleWindowMode);
    connect_menu(ui->action_Show_Filter_Bar, &MainWindow::OnToggleFilterBar);
    connect_menu(ui->action_Show_Status_Bar, &MainWindow::OnToggleStatusBar);
    connect_menu(ui->action_Show_Performance_Overlay, &MainWindow::OnTogglePerfOverlay);

    connect_menu(ui->action_Reset_Window_Size_720, &MainWindow::ResetWindowSize720);
    connect_menu(ui->action_Reset_Window_Size_900, &MainWindow::ResetWindowSize900);
    connect_menu(ui->action_Reset_Window_Size_1080, &MainWindow::ResetWindowSize1080);
    ui->menu_Reset_Window_Size->addActions({ui->action_Reset_Window_Size_720,
                                            ui->action_Reset_Window_Size_900,
                                            ui->action_Reset_Window_Size_1080});

    connect_menu(ui->action_Grid_View, &MainWindow::SetGridView);
    connect_menu(ui->action_Tree_View, &MainWindow::SetTreeView);
    connect_menu(ui->action_Carousel_View, &MainWindow::SetCarouselView);

    game_size_actions = new QActionGroup(this);
    game_size_actions->setExclusive(true);

    for (size_t i = 0; i < default_game_icon_sizes.size(); i++) {
        const auto current_size = UISettings::values.game_icon_size.GetValue();
        const auto size = default_game_icon_sizes[i].first;
        QAction* action = ui->menuGame_Icon_Size->addAction(GetTranslatedGameIconSize(i));
        action->setCheckable(true);

        if (current_size == size)
            action->setChecked(true);

        game_size_actions->addAction(action);

        connect(action, &QAction::triggered, this, [this, size](bool checked) {
            if (checked) {
                UISettings::values.game_icon_size.SetValue(size);
                CheckIconSize();

                game_list->UpdateIconSizes();
                game_list->RefreshGameDirectory();
            }
        });
    }

    CheckIconSize();

    ui->action_Show_Game_Name->setChecked(UISettings::values.show_game_name.GetValue());
    connect(ui->action_Show_Game_Name, &QAction::triggered, this, &MainWindow::ToggleShowGameName);

    // Multiplayer
    connect(ui->action_View_Lobby, &QAction::triggered, multiplayer_state,
            &MultiplayerState::OnViewLobby);
    connect(ui->action_Start_Room, &QAction::triggered, multiplayer_state,
            &MultiplayerState::OnCreateRoom);
    connect(ui->action_Leave_Room, &QAction::triggered, multiplayer_state,
            &MultiplayerState::OnCloseRoom);
    connect(ui->action_Connect_To_Room, &QAction::triggered, multiplayer_state,
            &MultiplayerState::OnDirectConnectToRoom);
    connect(ui->action_Show_Room, &QAction::triggered, multiplayer_state,
            &MultiplayerState::OnOpenNetworkRoom);
    connect(multiplayer_state, &MultiplayerState::SaveConfig, this, &MainWindow::OnSaveConfig);

    // Tools
    connect_menu(ui->action_Launch_PhotoViewer, [this] {
        LaunchFirmwareApplet(u64(Service::AM::AppletProgramId::PhotoViewer), std::nullopt);
    });
    connect_menu(ui->action_Launch_MiiEdit, [this] {
        LaunchFirmwareApplet(u64(Service::AM::AppletProgramId::MiiEdit), std::nullopt);
    });
    connect_menu(ui->action_Launch_Controller, [this] {
        LaunchFirmwareApplet(u64(Service::AM::AppletProgramId::Controller), std::nullopt);
    });
    connect_menu(ui->action_Launch_QLaunch, [this] {
        LaunchFirmwareApplet(u64(Service::AM::AppletProgramId::QLaunch), std::nullopt);
    });
    // Tools (cabinet)
    connect_menu(ui->action_Launch_Cabinet_Nickname_Owner, [this] {
        LaunchFirmwareApplet(u64(Service::AM::AppletProgramId::Cabinet),
                             {Service::NFP::CabinetMode::StartNicknameAndOwnerSettings});
    });
    connect_menu(ui->action_Launch_Cabinet_Eraser, [this] {
        LaunchFirmwareApplet(u64(Service::AM::AppletProgramId::Cabinet),
                             {Service::NFP::CabinetMode::StartGameDataEraser});
    });
    connect_menu(ui->action_Launch_Cabinet_Restorer, [this] {
        LaunchFirmwareApplet(u64(Service::AM::AppletProgramId::Cabinet),
                             {Service::NFP::CabinetMode::StartRestorer});
    });
    connect_menu(ui->action_Launch_Cabinet_Formatter, [this] {
        LaunchFirmwareApplet(u64(Service::AM::AppletProgramId::Cabinet),
                             {Service::NFP::CabinetMode::StartFormatter});
    });
    connect_menu(ui->action_Amiibo_Online_Database, &MainWindow::OnAmiiboOnlineDatabase);

    connect_menu(ui->action_Desktop, &MainWindow::OnCreateHomeMenuDesktopShortcut);
    connect_menu(ui->action_Application_Menu, &MainWindow::OnCreateHomeMenuApplicationMenuShortcut);
    connect_menu(ui->action_Capture_Screenshot, &MainWindow::OnCaptureScreenshot);
    connect_menu(ui->action_Translate_Screen, &MainWindow::OnTranslateScreen);

    // TAS
    connect_menu(ui->action_TAS_Start, &MainWindow::OnTasStartStop);
    connect_menu(ui->action_TAS_Record, &MainWindow::OnTasRecord);
    connect_menu(ui->action_TAS_Reset, &MainWindow::OnTasReset);
    connect_menu(ui->action_Configure_Tas, &MainWindow::OnConfigureTas);

    // Help
    connect_menu(ui->action_Root_Data_Folder, &MainWindow::OnOpenRootDataFolder);
    connect_menu(ui->action_NAND_Folder, &MainWindow::OnOpenNANDFolder);
    connect_menu(ui->action_SDMC_Folder, &MainWindow::OnOpenSDMCFolder);
    connect_menu(ui->action_Mod_Folder, &MainWindow::OnOpenModFolder);
    connect_menu(ui->action_Log_Folder, &MainWindow::OnOpenLogFolder);

    connect_menu(ui->action_Verify_installed_contents, &MainWindow::OnVerifyInstalledContents);
    connect_menu(ui->action_Firmware_Online, &MainWindow::OnInstallFirmwareOnline);
    connect_menu(ui->action_Firmware_From_Folder, &MainWindow::OnInstallFirmware);
    connect_menu(ui->action_Firmware_From_ZIP, &MainWindow::OnInstallFirmwareFromZIP);
    connect_menu(ui->action_Install_Keys_Online, &MainWindow::OnInstallKeysOnline);
    connect_menu(ui->action_Install_Keys_From_ZIP, &MainWindow::OnInstallKeysFromZIP);
    connect_menu(ui->action_Install_Keys, &MainWindow::OnInstallDecryptionKeys);
    connect_menu(ui->action_Check_Updates, [this] { OnCheckUpdates(true); });
    connect_menu(ui->action_About, &MainWindow::OnAbout);
    connect_menu(ui->action_Eden_Dependencies, &MainWindow::OnEdenDependencies);
    connect_menu(ui->action_Data_Manager, &MainWindow::OnDataDialog);

    if (storm_games_world_action) {
        connect_menu(storm_games_world_action, &MainWindow::OnOpenStormGamesWorld);
    }
    if (storm_save_sync_action) {
        connect_menu(storm_save_sync_action, [this] { OnOpenStormSaveSync(0); });
    }
    if (log_viewer_action) {
        connect_menu(log_viewer_action, &MainWindow::OnOpenLogViewer);
    }
}

void MainWindow::UpdateMenuState() {
    const bool is_paused = QtCommon::emu_thread == nullptr || !QtCommon::emu_thread->IsRunning();
    const bool is_firmware_available = CheckFirmwarePresence();

    const std::array running_actions{
        ui->action_Stop,
        ui->action_Restart,
        ui->action_Configure_Current_Game,
        ui->action_Load_Amiibo,
        ui->action_Pause,
    };

    const std::array applet_actions{
        ui->action_Launch_PhotoViewer,       ui->action_Launch_Cabinet_Nickname_Owner,
        ui->action_Launch_Cabinet_Eraser,    ui->action_Launch_Cabinet_Restorer,
        ui->action_Launch_Cabinet_Formatter, ui->action_Launch_MiiEdit,
        ui->action_Launch_QLaunch,           ui->action_Launch_Controller};

    for (QAction* action : running_actions) {
        action->setEnabled(emulation_running);
    }

    ui->action_Firmware_Online->setEnabled(!emulation_running);
    ui->action_Firmware_From_Folder->setEnabled(!emulation_running);
    ui->action_Firmware_From_ZIP->setEnabled(!emulation_running);
    ui->menuInstall_Keys->setEnabled(!emulation_running);
    ui->action_Install_Keys_Online->setEnabled(!emulation_running);
    ui->action_Install_Keys_From_ZIP->setEnabled(!emulation_running);
    ui->action_Install_Keys->setEnabled(!emulation_running);

    for (QAction* action : applet_actions) {
        action->setEnabled(is_firmware_available && !emulation_running);
    }

    ui->action_Capture_Screenshot->setEnabled(emulation_running && !is_paused);

    if (emulation_running && is_paused) {
        ui->action_Pause->setText(tr("&Continue"));
    } else {
        ui->action_Pause->setText(tr("&Pause"));
    }

    multiplayer_state->UpdateNotificationStatus();
}

void MainWindow::SetupPrepareForSleep() {
#ifdef __unix__
    if (auto bus = QDBusConnection::systemBus(); bus.isConnected()) {
        // See https://github.com/ConsoleKit2/ConsoleKit2/issues/150
#ifdef __linux__
        const auto dbus_logind_service = QStringLiteral("org.freedesktop.login1");
        const auto dbus_logind_path = QStringLiteral("/org/freedesktop/login1");
        const auto dbus_logind_manager_if = QStringLiteral("org.freedesktop.login1.Manager");
        // const auto dbus_logind_session_if = QStringLiteral("org.freedesktop.login1.Session");
#else
        const auto dbus_logind_service = QStringLiteral("org.freedesktop.ConsoleKit");
        const auto dbus_logind_path = QStringLiteral("/org/freedesktop/ConsoleKit/Manager");
        const auto dbus_logind_manager_if = QStringLiteral("org.freedesktop.ConsoleKit.Manager");
        // const auto dbus_logind_session_if = QStringLiteral("org.freedesktop.ConsoleKit.Session");
#endif
        const bool success = bus.connect(dbus_logind_service, dbus_logind_path,
                                         dbus_logind_manager_if, QStringLiteral("PrepareForSleep"),
                                         QStringLiteral("b"), this, SLOT(OnPrepareForSleep(bool)));
        if (!success)
            LOG_WARNING(Frontend, "Couldn't register PrepareForSleep signal");
    } else {
        LOG_WARNING(Frontend, "QDBusConnection system bus is not connected");
    }
#endif // __unix__
}

void MainWindow::OnPrepareForSleep(bool prepare_sleep) {
    if (QtCommon::emu_thread == nullptr)
        return;

    if (prepare_sleep) {
        if (QtCommon::emu_thread->IsRunning()) {
            auto_paused = true;
            OnPauseGame();
        }
    } else {
        if (!QtCommon::emu_thread->IsRunning() && auto_paused) {
            auto_paused = false;
            OnStartGame();
        }
    }
}

#ifdef __unix__
std::array<int, 3> MainWindow::sig_interrupt_fds{0, 0, 0};

void MainWindow::SetupSigInterrupts() {
    if (sig_interrupt_fds[2] == 1) {
        return;
    }
    socketpair(AF_UNIX, SOCK_STREAM, 0, sig_interrupt_fds.data());
    sig_interrupt_fds[2] = 1;

    struct sigaction sa;
    sa.sa_handler = &MainWindow::HandleSigInterrupt;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    sig_interrupt_notifier = new QSocketNotifier(sig_interrupt_fds[1], QSocketNotifier::Read, this);
    connect(sig_interrupt_notifier, &QSocketNotifier::activated, this,
            &MainWindow::OnSigInterruptNotifierActivated);
    connect(this, &MainWindow::SigInterrupt, this, &MainWindow::close);
}

void MainWindow::HandleSigInterrupt(int sig) {
    if (sig == SIGINT) {
        _exit(1);
    }

    // Calling into Qt directly from a signal handler is not safe,
    // so wake up a QSocketNotifier with this hacky write call instead.
    char a = 1;
    int ret = write(sig_interrupt_fds[0], &a, sizeof(a));
    (void)ret;
}

void MainWindow::OnSigInterruptNotifierActivated() {
    sig_interrupt_notifier->setEnabled(false);

    char a;
    int ret = read(sig_interrupt_fds[1], &a, sizeof(a));
    (void)ret;

    sig_interrupt_notifier->setEnabled(true);

    emit SigInterrupt();
}
#endif // __unix__

void MainWindow::PreventOSSleep() {
#ifdef _WIN32
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
#else
    SDL_DisableScreenSaver();
#endif
}

void MainWindow::AllowOSSleep() {
#ifdef _WIN32
    SetThreadExecutionState(ES_CONTINUOUS);
#else
    SDL_EnableScreenSaver();
#endif
}

bool MainWindow::LoadROM(const QString& filename, Service::AM::FrontendAppletParameters params) {
    // Shutdown previous session if the emu thread is still active...
    if (QtCommon::emu_thread != nullptr)
        ShutdownGame();

    if (!render_window->InitRenderTarget())
        return false;

    QtCommon::system->SetFilesystem(QtCommon::vfs);

    if (params.launch_type == Service::AM::LaunchType::FrontendInitiated)
        QtCommon::system->GetUserChannel().clear();

    QtCommon::system->SetFrontendAppletSet({
        std::make_unique<QtAmiiboSettings>(*this), // Amiibo Settings
        (UISettings::values.controller_applet_disabled.GetValue() == true)
            ? nullptr
            : std::make_unique<QtControllerSelector>(*this), // Controller Selector
        std::make_unique<QtErrorDisplay>(*this),             // Error Display
        nullptr,                                             // Mii Editor
        nullptr,                                             // Parental Controls
        nullptr,                                             // Photo Viewer
        std::make_unique<QtProfileSelector>(*this),          // Profile Selector
        std::make_unique<QtSoftwareKeyboard>(*this),         // Software Keyboard
        std::make_unique<QtWebBrowser>(*this),               // Web Browser
        nullptr,                                             // Net Connect
    });

    /** firmware check */

    if (!QtCommon::Content::CheckGameFirmware(params.program_id))
        return false;

    /** Exec */
    const Core::SystemResultStatus result{
        QtCommon::system->Load(*render_window, filename.toStdString(), params)};

    const auto drd_callout = (UISettings::values.callout_flags.GetValue() &
                              static_cast<u32>(CalloutFlag::DRDDeprecation)) == 0;

    if (result == Core::SystemResultStatus::Success &&
        QtCommon::system->GetAppLoader().GetFileType() ==
            Loader::FileType::DeconstructedRomDirectory &&
        drd_callout) {
        UISettings::values.callout_flags = UISettings::values.callout_flags.GetValue() |
                                           static_cast<u32>(CalloutFlag::DRDDeprecation);
        QMessageBox::warning(
            this, tr("Warning: Outdated Game Format"),
            tr("You are using the deconstructed ROM directory format for this game, which is an "
               "outdated format that has been superseded by others such as NCA, NAX, XCI, or "
               "NSP. Deconstructed ROM directories lack icons, metadata, and update "
               "support.<br>For an explanation of the various Switch formats STORM SWITCH supports, "
               "out our user handbook. This message will not be shown again."));
    }

    if (result != Core::SystemResultStatus::Success) {
        switch (result) {
        case Core::SystemResultStatus::ErrorGetLoader:
            LOG_CRITICAL(Frontend, "Failed to obtain loader for {}!", filename.toStdString());
            QMessageBox::critical(this, tr("Ошибка при загрузке игры!"),
                                  tr("Формат файла игры не поддерживается."));
            break;
        case Core::SystemResultStatus::ErrorVideoCore:
            QMessageBox::critical(
                this, tr("Ошибка инициализации видеоядра."),
                tr("STORM SWITCH столкнулся с ошибкой при запуске видеоядра GPU. "
                   "Обычно это вызвано устаревшими драйверами видеокарты. "
                   "Пожалуйста, обновите графические драйверы."));
            break;
        default:
            if (result > Core::SystemResultStatus::ErrorLoader) {
                const u16 loader_id = static_cast<u16>(Core::SystemResultStatus::ErrorLoader);
                const u16 error_id = static_cast<u16>(result) - loader_id;
                const std::string error_code = fmt::format("({:04X}-{:04X})", loader_id, error_id);
                LOG_CRITICAL(Frontend, "Failed to load ROM! {}", error_code);

                const auto title =
                    tr("Ошибка при загрузке игры! %1", "%1 signifies a numeric error code.")
                        .arg(QString::fromStdString(error_code));
                const auto description =
                    tr("%1<br><br>Пожалуйста, проверьте наличие свежих ключей prod.keys или пересоздайте дамп файла игры.",
                       "%1 signifies an error string.")
                        .arg(QString::fromStdString(
                            GetResultStatusString(static_cast<Loader::ResultStatus>(error_id))));

                QMessageBox::critical(this, title, description);
            } else {
                QMessageBox::critical(
                    this, tr("Ошибка при загрузке игры!"),
                    tr("Произошла неизвестная ошибка при запуске. Подробности смотрите в логе."));
            }
            break;
        }
        return false;
    }
    current_game_path = filename;

    return true;
}

bool MainWindow::SelectAndSetCurrentUser(
    const Core::Frontend::ProfileSelectParameters& parameters) {
    QtProfileSelectionDialog dialog(*QtCommon::system, this, parameters);
    dialog.setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint |
                          Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
    dialog.setWindowModality(Qt::WindowModal);

    if (dialog.exec() == QDialog::Rejected) {
        return false;
    }

    Settings::values.current_user = dialog.GetIndex();
    return true;
}

void MainWindow::StormSessionBackup::Capture() {
    auto capture_switchable = [](const auto& setting) {
        return std::make_pair(setting.GetValue(true), setting.UsingGlobal());
    };
    resolution_setup = capture_switchable(Settings::values.resolution_setup);
    gpu_accuracy = capture_switchable(Settings::values.gpu_accuracy);
    astc_recompression = capture_switchable(Settings::values.astc_recompression);
    accelerate_astc = capture_switchable(Settings::values.accelerate_astc);
    use_asynchronous_shaders = capture_switchable(Settings::values.use_asynchronous_shaders);
    async_presentation = capture_switchable(Settings::values.async_presentation);
    use_reactive_flushing = capture_switchable(Settings::values.use_reactive_flushing);
    sync_memory_operations = capture_switchable(Settings::values.sync_memory_operations);
    gpu_clock = capture_switchable(Settings::values.gpu_clock);
    eco_frame_pacing = capture_switchable(Settings::values.eco_frame_pacing);
    max_anisotropy = capture_switchable(Settings::values.max_anisotropy);
    anti_aliasing = capture_switchable(Settings::values.anti_aliasing);
    scaling_filter = capture_switchable(Settings::values.scaling_filter);
    fsr_sharpening_slider = capture_switchable(Settings::values.fsr_sharpening_slider);
    nvdec_emulation = capture_switchable(Settings::values.nvdec_emulation);
    dma_accuracy = capture_switchable(Settings::values.dma_accuracy);
    cpu_accuracy = capture_switchable(Settings::values.cpu_accuracy);
    cpuopt_fastmem = capture_switchable(Settings::values.cpuopt_fastmem);
    cpuopt_ignore_memory_aborts = capture_switchable(Settings::values.cpuopt_ignore_memory_aborts);
    cpuopt_recompile_exclusives = capture_switchable(Settings::values.cpuopt_recompile_exclusives);
    cpuopt_fastmem_exclusives = capture_switchable(Settings::values.cpuopt_fastmem_exclusives);
    airplane_mode = capture_switchable(Settings::values.airplane_mode);
    memory_layout_mode = capture_switchable(Settings::values.memory_layout_mode);
    use_docked_mode = capture_switchable(Settings::values.use_docked_mode);
    enable_gpu_buffer_readback = capture_switchable(Settings::values.enable_gpu_buffer_readback);
    early_release_fences = capture_switchable(Settings::values.early_release_fences);
    gpu_fence_behavior = capture_switchable(Settings::values.gpu_fence_behavior);
    barrier_feedback_loops = capture_switchable(Settings::values.barrier_feedback_loops);
    smart_shader_throttle = capture_switchable(Settings::values.smart_shader_throttle);
    vram_garbage_collection = capture_switchable(Settings::values.vram_garbage_collection);
    drs_resolution_lock = capture_switchable(Settings::values.drs_resolution_lock);
    rescale_hack = capture_switchable(Settings::values.rescale_hack);
    fix_bloom_effects = capture_switchable(Settings::values.fix_bloom_effects);
    aspect_ratio = capture_switchable(Settings::values.aspect_ratio);
    vram_usage_mode = capture_switchable(Settings::values.vram_usage_mode);
    enable_frame_skipping = capture_switchable(Settings::values.enable_frame_skipping);
    frame_gen = capture_switchable(Settings::values.frame_gen);
    vram_budget_governor = capture_switchable(Settings::values.vram_budget_governor);
    storm_lowend_turbo = capture_switchable(Settings::values.storm_lowend_turbo);
    storm_thermal_governor = capture_switchable(Settings::values.storm_thermal_governor);
    renderer_force_max_clock = capture_switchable(Settings::values.renderer_force_max_clock);
    skip_cpu_inner_invalidation = capture_switchable(Settings::values.skip_cpu_inner_invalidation);
    cpu_clock = capture_switchable(Settings::values.cpu_clock);
    use_custom_cpu_ticks = capture_switchable(Settings::values.use_custom_cpu_ticks);
    cpu_ticks = capture_switchable(Settings::values.cpu_ticks);
    cpu_backend = capture_switchable(Settings::values.cpu_backend);
    frame_pacing_mode = capture_switchable(Settings::values.frame_pacing_mode);
    dynamic_performance_scaler = capture_switchable(Settings::values.dynamic_performance_scaler);
    use_fast_gpu_time = capture_switchable(Settings::values.use_fast_gpu_time);
    dyna_state = capture_switchable(Settings::values.dyna_state);
    emulate_bgr565 = capture_switchable(Settings::values.emulate_bgr565);
    use_video_framerate = capture_switchable(Settings::values.use_video_framerate);
    use_vulkan_driver_pipeline_cache = capture_switchable(Settings::values.use_vulkan_driver_pipeline_cache);
    use_disk_shader_cache = capture_switchable(Settings::values.use_disk_shader_cache);
    enable_compute_pipelines = capture_switchable(Settings::values.enable_compute_pipelines);
    vertex_input_dynamic_state = capture_switchable(Settings::values.vertex_input_dynamic_state);
    disable_macro_hle = capture_switchable(Settings::values.disable_macro_hle);
    sync_core_speed = capture_switchable(Settings::values.sync_core_speed);
    language_index = capture_switchable(Settings::values.language_index);
    region_index = capture_switchable(Settings::values.region_index);
    disable_macro_jit = Settings::values.disable_macro_jit.GetValue();
    use_auto_stub = Settings::values.use_auto_stub.GetValue();
    disable_web_applet = Settings::values.disable_web_applet.GetValue();
    is_active = true;
}

void MainWindow::StormSessionBackup::Restore() {
    if (!is_active) return;
    auto restore_switchable = [](auto& setting, const auto& pair) {
        setting.SetGlobal(pair.second);
        setting.SetValue(pair.first);
    };
    restore_switchable(Settings::values.resolution_setup, resolution_setup);
    restore_switchable(Settings::values.gpu_accuracy, gpu_accuracy);
    restore_switchable(Settings::values.astc_recompression, astc_recompression);
    restore_switchable(Settings::values.accelerate_astc, accelerate_astc);
    restore_switchable(Settings::values.use_asynchronous_shaders, use_asynchronous_shaders);
    restore_switchable(Settings::values.async_presentation, async_presentation);
    restore_switchable(Settings::values.use_reactive_flushing, use_reactive_flushing);
    restore_switchable(Settings::values.sync_memory_operations, sync_memory_operations);
    restore_switchable(Settings::values.gpu_clock, gpu_clock);
    restore_switchable(Settings::values.eco_frame_pacing, eco_frame_pacing);
    restore_switchable(Settings::values.max_anisotropy, max_anisotropy);
    restore_switchable(Settings::values.anti_aliasing, anti_aliasing);
    restore_switchable(Settings::values.scaling_filter, scaling_filter);
    restore_switchable(Settings::values.fsr_sharpening_slider, fsr_sharpening_slider);
    restore_switchable(Settings::values.nvdec_emulation, nvdec_emulation);
    restore_switchable(Settings::values.dma_accuracy, dma_accuracy);
    restore_switchable(Settings::values.cpu_accuracy, cpu_accuracy);
    restore_switchable(Settings::values.cpuopt_fastmem, cpuopt_fastmem);
    restore_switchable(Settings::values.cpuopt_ignore_memory_aborts, cpuopt_ignore_memory_aborts);
    restore_switchable(Settings::values.cpuopt_recompile_exclusives, cpuopt_recompile_exclusives);
    restore_switchable(Settings::values.cpuopt_fastmem_exclusives, cpuopt_fastmem_exclusives);
    restore_switchable(Settings::values.airplane_mode, airplane_mode);
    restore_switchable(Settings::values.memory_layout_mode, memory_layout_mode);
    restore_switchable(Settings::values.use_docked_mode, use_docked_mode);
    restore_switchable(Settings::values.enable_gpu_buffer_readback, enable_gpu_buffer_readback);
    restore_switchable(Settings::values.early_release_fences, early_release_fences);
    restore_switchable(Settings::values.gpu_fence_behavior, gpu_fence_behavior);
    restore_switchable(Settings::values.barrier_feedback_loops, barrier_feedback_loops);
    restore_switchable(Settings::values.smart_shader_throttle, smart_shader_throttle);
    restore_switchable(Settings::values.vram_garbage_collection, vram_garbage_collection);
    restore_switchable(Settings::values.drs_resolution_lock, drs_resolution_lock);
    restore_switchable(Settings::values.rescale_hack, rescale_hack);
    restore_switchable(Settings::values.fix_bloom_effects, fix_bloom_effects);
    restore_switchable(Settings::values.aspect_ratio, aspect_ratio);
    restore_switchable(Settings::values.vram_usage_mode, vram_usage_mode);
    restore_switchable(Settings::values.enable_frame_skipping, enable_frame_skipping);
    restore_switchable(Settings::values.frame_gen, frame_gen);
    restore_switchable(Settings::values.vram_budget_governor, vram_budget_governor);
    restore_switchable(Settings::values.storm_lowend_turbo, storm_lowend_turbo);
    restore_switchable(Settings::values.storm_thermal_governor, storm_thermal_governor);
    restore_switchable(Settings::values.renderer_force_max_clock, renderer_force_max_clock);
    restore_switchable(Settings::values.skip_cpu_inner_invalidation, skip_cpu_inner_invalidation);
    restore_switchable(Settings::values.cpu_clock, cpu_clock);
    restore_switchable(Settings::values.use_custom_cpu_ticks, use_custom_cpu_ticks);
    restore_switchable(Settings::values.cpu_ticks, cpu_ticks);
    restore_switchable(Settings::values.cpu_backend, cpu_backend);
    restore_switchable(Settings::values.frame_pacing_mode, frame_pacing_mode);
    restore_switchable(Settings::values.dynamic_performance_scaler, dynamic_performance_scaler);
    restore_switchable(Settings::values.use_fast_gpu_time, use_fast_gpu_time);
    restore_switchable(Settings::values.dyna_state, dyna_state);
    restore_switchable(Settings::values.emulate_bgr565, emulate_bgr565);
    restore_switchable(Settings::values.use_video_framerate, use_video_framerate);
    restore_switchable(Settings::values.use_vulkan_driver_pipeline_cache, use_vulkan_driver_pipeline_cache);
    restore_switchable(Settings::values.use_disk_shader_cache, use_disk_shader_cache);
    restore_switchable(Settings::values.enable_compute_pipelines, enable_compute_pipelines);
    restore_switchable(Settings::values.vertex_input_dynamic_state, vertex_input_dynamic_state);
    restore_switchable(Settings::values.disable_macro_hle, disable_macro_hle);
    restore_switchable(Settings::values.sync_core_speed, sync_core_speed);
    restore_switchable(Settings::values.language_index, language_index);
    restore_switchable(Settings::values.region_index, region_index);
    Settings::values.disable_macro_jit.SetValue(disable_macro_jit);
    Settings::values.use_auto_stub.SetValue(use_auto_stub);
    Settings::values.disable_web_applet.SetValue(disable_web_applet);

    Settings::UpdateGPUAccuracy();
    Settings::UpdateRescalingInfo();
    is_active = false;
}

void MainWindow::RestoreSessionSettings() {
    if (m_session_backup.is_active) {
        Settings::RestoreGlobalState(false);
        m_session_backup.Restore();
        if (config) {
            config->SaveAllValues();
        }
        Core::GameFixDatabase::SetFixesEnabled(false);
        UpdateStatusButtons();
        ConfigurationShared::ReloadAllActiveWidgets();
    }
}

void MainWindow::OnOpenStormGamesWorld() {
    StormGamesWorldDialog dialog(this);
    dialog.exec();
}

void MainWindow::OnOpenStormSaveSync(u64 target_program_id) {
    StormSaveSyncDialog dialog(this, target_program_id);
    dialog.exec();
}

void MainWindow::OnOpenLogViewer() {
    LogViewerDialog dialog(this);
    dialog.exec();
}

namespace {

QString TranslateFixBoolValue(const std::string& v) {
    const bool on = (v == "true" || v == "1");
    return on
        ? StormLang(QStringLiteral("Включено"), QStringLiteral("Enabled"),
                    QStringLiteral("Aktiviert"), QStringLiteral("Activé"),
                    QStringLiteral("已启用"), QStringLiteral("有効"),
                    QStringLiteral("مفعل"), QStringLiteral("Activado"))
        : StormLang(QStringLiteral("Отключено"), QStringLiteral("Disabled"),
                    QStringLiteral("Deaktiviert"), QStringLiteral("Désactivé"),
                    QStringLiteral("已禁用"), QStringLiteral("無効"),
                    QStringLiteral("معطل"), QStringLiteral("Desactivado"));
}

QString FormatLocalizedIniParam(const std::string& k, const std::string& v) {
    auto bval = [&]() { return TranslateFixBoolValue(v); };

    if (k == "Renderer\\gpu_accuracy" || k == "gpu_accuracy") {
        QString lvl = (v == "0")
            ? StormLang(QStringLiteral("Обычная (Быстро)"), QStringLiteral("Normal (Fast)"), QStringLiteral("Normal (Schnell)"), QStringLiteral("Normale (Rapide)"), QStringLiteral("普通 (快速)"), QStringLiteral("通常 (高速)"), QStringLiteral("عادي (سريع)"), QStringLiteral("Normal (Rápido)"))
            : (v == "1")
            ? StormLang(QStringLiteral("Высокая"), QStringLiteral("High"), QStringLiteral("Hoch"), QStringLiteral("Élevée"), QStringLiteral("高"), QStringLiteral("高"), QStringLiteral("عالي"), QStringLiteral("Alta"))
            : StormLang(QStringLiteral("Экстремальная"), QStringLiteral("Extreme"), QStringLiteral("Extrem"), QStringLiteral("Extrême"), QStringLiteral("极高"), QStringLiteral("最高"), QStringLiteral("قصوى"), QStringLiteral("Extrema"));
        return StormLang(QStringLiteral("Точность ГП: %1"), QStringLiteral("GPU Accuracy: %1"), QStringLiteral("GPU-Genauigkeit: %1"), QStringLiteral("Précision GPU : %1"), QStringLiteral("GPU 精度: %1"), QStringLiteral("GPU 精度: %1"), QStringLiteral("دقة GPU: %1"), QStringLiteral("Precisión GPU: %1")).arg(lvl);
    }
    if (k == "Renderer\\drs_resolution_lock" || k == "drs_resolution_lock") {
        return StormLang(QStringLiteral("Блокировка динам. разрешения (DRS): %1"), QStringLiteral("Dynamic Resolution Lock (DRS): %1"), QStringLiteral("Dynamische Auflösungssperre (DRS): %1"), QStringLiteral("Verrouillage résolution dyn. (DRS) : %1"), QStringLiteral("动态分辨率锁定 (DRS): %1"), QStringLiteral("動的解像度ロック (DRS): %1"), QStringLiteral("قفل الدقة الديناميكية (DRS): %1"), QStringLiteral("Bloqueo de resolución dinámica (DRS): %1")).arg(bval());
    }
    if (k == "Renderer\\rescale_hack" || k == "rescale_hack") {
        return StormLang(QStringLiteral("Патч масштабирования рендера: %1"), QStringLiteral("Render Rescale Patch: %1"), QStringLiteral("Render-Skalierungs-Patch: %1"), QStringLiteral("Patch de mise à l'échelle : %1"), QStringLiteral("渲染重缩放补丁: %1"), QStringLiteral("レンダースケールパッチ: %1"), QStringLiteral("تصحيح تحجيم العرض: %1"), QStringLiteral("Parche de reescalado: %1")).arg(bval());
    }
    if (k == "Renderer\\dynamic_performance_scaler" || k == "dynamic_performance_scaler") {
        return StormLang(QStringLiteral("Динамический скейлер FPS: %1"), QStringLiteral("Dynamic FPS Performance Scaler: %1"), QStringLiteral("Dynamischer FPS-Skalierer: %1"), QStringLiteral("Ajusteur dynamique de FPS : %1"), QStringLiteral("动态帧率缩放器: %1"), QStringLiteral("動的FPSスケーラー: %1"), QStringLiteral("موازن الأداء الديناميكي: %1"), QStringLiteral("Escalador dinámico de FPS: %1")).arg(bval());
    }
    if (k == "Renderer\\barrier_feedback_loops" || k == "barrier_feedback_loops") {
        return StormLang(QStringLiteral("Барьеры циклов обратной связи: %1"), QStringLiteral("Feedback Loop Barriers: %1"), QStringLiteral("Feedback-Loop-Barrieren: %1"), QStringLiteral("Barrières de boucle de rétroaction : %1"), QStringLiteral("反馈循环屏障: %1"), QStringLiteral("フィードバックループバリア: %1"), QStringLiteral("حواجز حلقات التغذية الراجعة: %1"), QStringLiteral("Barreras de bucle de retorno: %1")).arg(bval());
    }
    if (k == "Renderer\\use_reactive_flushing" || k == "use_reactive_flushing") {
        return StormLang(QStringLiteral("Реактивная очистка буферов: %1"), QStringLiteral("Reactive Buffer Flushing: %1"), QStringLiteral("Reaktives Puffer-Flushing: %1"), QStringLiteral("Vidage réactif des tampons : %1"), QStringLiteral("响应式缓冲刷新: %1"), QStringLiteral("リアクティブフラッシング: %1"), QStringLiteral("تفريغ الذاكرة التفاعلي: %1"), QStringLiteral("Vaciado reactivo de búferes: %1")).arg(bval());
    }
    if (k == "Renderer\\astc_recompression" || k == "astc_recompression") {
        QString mode = (v == "0")
            ? StormLang(QStringLiteral("Без сжатия"), QStringLiteral("Uncompressed"), QStringLiteral("Unkomprimiert"), QStringLiteral("Non compressé"), QStringLiteral("未压缩"), QStringLiteral("非圧縮"), QStringLiteral("غير مضغوط"), QStringLiteral("Sin comprimir"))
            : (v == "1") ? QStringLiteral("BC1") : QStringLiteral("BC3");
        return StormLang(QStringLiteral("Пересжатие ASTC: %1"), QStringLiteral("ASTC Recompression: %1"), QStringLiteral("ASTC-Rekompression: %1"), QStringLiteral("Recompression ASTC : %1"), QStringLiteral("ASTC 重压缩: %1"), QStringLiteral("ASTC 再圧縮: %1"), QStringLiteral("إعادة ضغط ASTC: %1"), QStringLiteral("Recompresión ASTC: %1")).arg(mode);
    }
    if (k == "Renderer\\use_asynchronous_shaders" || k == "use_asynchronous_shaders") {
        return StormLang(QStringLiteral("Асинхронная компиляция шейдеров: %1"), QStringLiteral("Asynchronous Shader Building: %1"), QStringLiteral("Asynchrone Shader-Kompilierung: %1"), QStringLiteral("Compilation asynchrone des shaders : %1"), QStringLiteral("异步着色器编译: %1"), QStringLiteral("非同期シェーダー構築: %1"), QStringLiteral("بناء المظللات غير المتزامن: %1"), QStringLiteral("Compilación asíncrona de shaders: %1")).arg(bval());
    }
    if (k == "Renderer\\enable_gpu_buffer_readback" || k == "enable_gpu_buffer_readback") {
        return StormLang(QStringLiteral("Обратное чтение буферов ГП: %1"), QStringLiteral("GPU Buffer Readback: %1"), QStringLiteral("GPU-Puffer-Readback: %1"), QStringLiteral("Lecture inverse des tampons GPU : %1"), QStringLiteral("GPU 缓冲回读: %1"), QStringLiteral("GPU バッファリードバック: %1"), QStringLiteral("قراءة مخازن GPU: %1"), QStringLiteral("Lectura inversa de búfer GPU: %1")).arg(bval());
    }
    if (k == "Renderer\\early_release_fences" || k == "early_release_fences") {
        return StormLang(QStringLiteral("Раннее освобождение барьеров ГП: %1"), QStringLiteral("Early Fence Release: %1"), QStringLiteral("Frühe Fence-Freigabe: %1"), QStringLiteral("Libération anticipée des barrières : %1"), QStringLiteral("提前释放同步栅栏: %1"), QStringLiteral("フェンス早期解放: %1"), QStringLiteral("تحرير الحواجز المبكر: %1"), QStringLiteral("Liberación temprana de vallas: %1")).arg(bval());
    }
    if (k == "Renderer\\sync_memory_operations" || k == "sync_memory_operations") {
        return StormLang(QStringLiteral("Синхронизация операций памяти: %1"), QStringLiteral("Sync Memory Operations: %1"), QStringLiteral("Speicheroperationen synchronisieren: %1"), QStringLiteral("Synchroniser les opérations mémoire : %1"), QStringLiteral("同步内存操作: %1"), QStringLiteral("メモリ操作の同期: %1"), QStringLiteral("مزامنة عمليات الذاكرة: %1"), QStringLiteral("Sincronizar operaciones de memoria: %1")).arg(bval());
    }
    if (k == "Renderer\\use_fast_gpu_time" || k == "use_fast_gpu_time") {
        return StormLang(QStringLiteral("Быстрый таймер ГП: %1"), QStringLiteral("Fast GPU Time: %1"), QStringLiteral("Schnelle GPU-Zeit: %1"), QStringLiteral("Temps GPU rapide : %1"), QStringLiteral("快速 GPU 时钟: %1"), QStringLiteral("高速 GPU 時間: %1"), QStringLiteral("توقيت GPU السريع: %1"), QStringLiteral("Tiempo de GPU rápido: %1")).arg(bval());
    }
    if (k == "Renderer\\nvdec_emulation" || k == "nvdec_emulation") {
        QString mode = (v == "0") ? TranslateFixBoolValue("false")
            : (v == "1") ? StormLang(QStringLiteral("ЦП (CPU)"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"))
            : (v == "2") ? StormLang(QStringLiteral("ГП (GPU)"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"))
            : StormLang(QStringLiteral("Гибридный"), QStringLiteral("Hybrid"), QStringLiteral("Hybrid"), QStringLiteral("Hybride"), QStringLiteral("混合"), QStringLiteral("ハイブリッド"), QStringLiteral("هجين"), QStringLiteral("Híbrido"));
        return StormLang(QStringLiteral("Видеодекодер NVDEC: %1"), QStringLiteral("NVDEC Video Decoding: %1"), QStringLiteral("NVDEC-Videodekodierung: %1"), QStringLiteral("Décodage vidéo NVDEC : %1"), QStringLiteral("NVDEC 视频解码: %1"), QStringLiteral("NVDEC 映像デコード: %1"), QStringLiteral("فك تشفير الفيديو NVDEC: %1"), QStringLiteral("Decodificación NVDEC: %1")).arg(mode);
    }
    if (k == "Renderer\\async_presentation" || k == "async_presentation") {
        return StormLang(QStringLiteral("Асинхронная презентация: %1"), QStringLiteral("Asynchronous Presentation: %1"), QStringLiteral("Asynchrone Präsentation: %1"), QStringLiteral("Présentation asynchrone : %1"), QStringLiteral("异步画面呈现: %1"), QStringLiteral("非同期プレゼンテーション: %1"), QStringLiteral("العرض غير المتزامن: %1"), QStringLiteral("Presentación asíncrona: %1")).arg(bval());
    }
    if (k == "Renderer\\accelerate_astc" || k == "accelerate_astc") {
        QString mode = (v == "0") ? StormLang(QStringLiteral("ЦП"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"))
            : (v == "1") ? StormLang(QStringLiteral("ГП"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"))
            : (v == "2") ? StormLang(QStringLiteral("ЦП асинхронно"), QStringLiteral("CPU Async"), QStringLiteral("CPU Asynchron"), QStringLiteral("CPU Asynchrone"), QStringLiteral("CPU 异步"), QStringLiteral("CPU 非同期"), QStringLiteral("CPU غير متزامن"), QStringLiteral("CPU Asíncrono"))
            : StormLang(QStringLiteral("Гибридный"), QStringLiteral("Hybrid"), QStringLiteral("Hybrid"), QStringLiteral("Hybride"), QStringLiteral("混合"), QStringLiteral("ハイブリッド"), QStringLiteral("هجين"), QStringLiteral("Híbrido"));
        return StormLang(QStringLiteral("Декодирование ASTC: %1"), QStringLiteral("ASTC Decoding: %1"), QStringLiteral("ASTC-Dekodierung: %1"), QStringLiteral("Décodage ASTC : %1"), QStringLiteral("ASTC 纹理加速: %1"), QStringLiteral("ASTC デコード: %1"), QStringLiteral("فك تشفير ASTC: %1"), QStringLiteral("Decodificación ASTC: %1")).arg(mode);
    }
    if (k == "Renderer\\gpu_fence_behavior" || k == "gpu_fence_behavior") {
        QString mode = (v == "1") ? StormLang(QStringLiteral("Мгновенно"), QStringLiteral("Immediate"), QStringLiteral("Sofort"), QStringLiteral("Immédiat"), QStringLiteral("立即"), QStringLiteral("即時"), QStringLiteral("فوري"), QStringLiteral("Inmediato"))
            : (v == "2") ? StormLang(QStringLiteral("Сбалансированно"), QStringLiteral("Balanced"), QStringLiteral("Ausgewogen"), QStringLiteral("Équilibré"), QStringLiteral("平衡"), QStringLiteral("バランス"), QStringLiteral("متوازن"), QStringLiteral("Equilibrado"))
            : (v == "3") ? StormLang(QStringLiteral("Точно"), QStringLiteral("Accurate"), QStringLiteral("Präzise"), QStringLiteral("Précis"), QStringLiteral("精确"), QStringLiteral("正確"), QStringLiteral("دقيق"), QStringLiteral("Preciso"))
            : (v == "4") ? StormLang(QStringLiteral("Строго"), QStringLiteral("Strict"), QStringLiteral("Strikt"), QStringLiteral("Strict"), QStringLiteral("严格"), QStringLiteral("厳格"), QStringLiteral("صارم"), QStringLiteral("Estricto"))
            : StormLang(QStringLiteral("По умолчанию"), QStringLiteral("Default"), QStringLiteral("Standard"), QStringLiteral("Par défaut"), QStringLiteral("默认"), QStringLiteral("デフォルト"), QStringLiteral("افتراضي"), QStringLiteral("Predeterminado"));
        return StormLang(QStringLiteral("Барьеры ГП (Fence): %1"), QStringLiteral("GPU Fence Behavior: %1"), QStringLiteral("GPU-Fence-Verhalten: %1"), QStringLiteral("Comportement GPU Fence : %1"), QStringLiteral("GPU 栅栏行为: %1"), QStringLiteral("GPU フェンス動作: %1"), QStringLiteral("سلوك حواجز GPU: %1"), QStringLiteral("Barreras GPU (Fence): %1")).arg(mode);
    }
    if (k == "Renderer\\dyna_state" || k == "dyna_state") {
        QString mode = (v == "0") ? TranslateFixBoolValue("false") : QStringLiteral("EDS %1").arg(QString::fromStdString(v));
        return StormLang(QStringLiteral("Динамическое состояние Vulkan: %1"), QStringLiteral("Vulkan Dynamic State: %1"), QStringLiteral("Dynamischer Vulkan-Status: %1"), QStringLiteral("État dynamique Vulkan : %1"), QStringLiteral("Vulkan 动态状态: %1"), QStringLiteral("Vulkan 動的ステート: %1"), QStringLiteral("الحالة الديناميكية لـ Vulkan: %1"), QStringLiteral("Estado dinámico Vulkan: %1")).arg(mode);
    }
    if (k == "Renderer\\vram_usage_mode" || k == "vram_usage_mode") {
        QString mode = (v == "0") ? StormLang(QStringLiteral("Консервативный"), QStringLiteral("Conservative"), QStringLiteral("Konservativ"), QStringLiteral("Conservateur"), QStringLiteral("保守"), QStringLiteral("控えめ"), QStringLiteral("محافظ"), QStringLiteral("Conservador"))
            : (v == "2") ? StormLang(QStringLiteral("Агрессивный"), QStringLiteral("Aggressive"), QStringLiteral("Aggressiv"), QStringLiteral("Agressif"), QStringLiteral("激进"), QStringLiteral("積極的"), QStringLiteral("مكثف"), QStringLiteral("Agresivo"))
            : StormLang(QStringLiteral("Обычный"), QStringLiteral("Normal"), QStringLiteral("Normal"), QStringLiteral("Normal"), QStringLiteral("正常"), QStringLiteral("通常"), QStringLiteral("عادي"), QStringLiteral("Normal"));
        return StormLang(QStringLiteral("Режим видеопамяти (VRAM): %1"), QStringLiteral("VRAM Usage Mode: %1"), QStringLiteral("VRAM-Nutzungsmodus: %1"), QStringLiteral("Mode d'utilisation VRAM : %1"), QStringLiteral("显存使用模式: %1"), QStringLiteral("VRAM 使用モード: %1"), QStringLiteral("وضع استهلاك VRAM: %1"), QStringLiteral("Modo de uso de VRAM: %1")).arg(mode);
    }
    if (k == "Renderer\\fix_bloom_effects" || k == "fix_bloom_effects") {
        return StormLang(QStringLiteral("Защита свечения (Fix Bloom): %1"), QStringLiteral("Fix Bloom Effects: %1"), QStringLiteral("Bloom-Effekte korrigieren: %1"), QStringLiteral("Corriger les effets de Bloom : %1"), QStringLiteral("修复泛光特效 (Bloom): %1"), QStringLiteral("ブルーム効果修正: %1"), QStringLiteral("إصلاح توهج Bloom: %1"), QStringLiteral("Corregir efectos Bloom: %1")).arg(bval());
    }
    if (k == "Renderer\\eco_frame_pacing" || k == "eco_frame_pacing") {
        return StormLang(QStringLiteral("Эко-синхронизация кадров: %1"), QStringLiteral("Eco Frame Pacing: %1"), QStringLiteral("Eco-Frame-Pacing: %1"), QStringLiteral("Rythme d'images éco : %1"), QStringLiteral("节能帧率平滑: %1"), QStringLiteral("エコフレームペーシング: %1"), QStringLiteral("تنظيم الإطارات الاقتصادي: %1"), QStringLiteral("Ritmo de fotogramas Eco: %1")).arg(bval());
    }
    if (k == "Renderer\\emulate_bgr565" || k == "emulate_bgr565") {
        return StormLang(QStringLiteral("Эмуляция формата BGR565: %1"), QStringLiteral("Emulate BGR565 Format: %1"), QStringLiteral("BGR565-Format emulieren: %1"), QStringLiteral("Émuler le format BGR565 : %1"), QStringLiteral("模拟 BGR565 格式: %1"), QStringLiteral("BGR565 形式エミュレート: %1"), QStringLiteral("محاكاة تنسيق BGR565: %1"), QStringLiteral("Emular formato BGR565: %1")).arg(bval());
    }
    if (k == "Renderer\\use_video_framerate" || k == "use_video_framerate") {
        return StormLang(QStringLiteral("Частота кадров видеороликов: %1"), QStringLiteral("Use Video Framerate: %1"), QStringLiteral("Video-Bildrate verwenden: %1"), QStringLiteral("Utiliser la fréquence vidéo : %1"), QStringLiteral("使用视频原生帧率: %1"), QStringLiteral("動画フレームレート使用: %1"), QStringLiteral("استخدام معدل إطارات الفيديو: %1"), QStringLiteral("Usar tasa de fotogramas de vídeo: %1")).arg(bval());
    }
    if (k == "Renderer\\use_vulkan_driver_pipeline_cache" || k == "use_vulkan_driver_pipeline_cache") {
        return StormLang(QStringLiteral("Кэш конвейеров драйвера Vulkan: %1"), QStringLiteral("Vulkan Driver Pipeline Cache: %1"), QStringLiteral("Vulkan-Treiber-Pipeline-Cache: %1"), QStringLiteral("Cache pipeline pilote Vulkan : %1"), QStringLiteral("Vulkan 驱动管线缓存: %1"), QStringLiteral("Vulkan ドライバパイプラインキャッシュ: %1"), QStringLiteral("ذاكرة خطوط أنابيب Vulkan: %1"), QStringLiteral("Caché de tuberías Vulkan: %1")).arg(bval());
    }
    if (k == "Renderer\\use_disk_shader_cache" || k == "use_disk_shader_cache") {
        return StormLang(QStringLiteral("Дисковый кэш шейдеров: %1"), QStringLiteral("Disk Shader Cache: %1"), QStringLiteral("Festplatten-Shader-Cache: %1"), QStringLiteral("Cache de shaders sur disque : %1"), QStringLiteral("磁盘着色器缓存: %1"), QStringLiteral("ディスクシェーダーキャッシュ: %1"), QStringLiteral("ذاكرة المظللات على القرص: %1"), QStringLiteral("Caché de shaders en disco: %1")).arg(bval());
    }
    if (k == "Renderer\\enable_compute_pipelines" || k == "enable_compute_pipelines") {
        return StormLang(QStringLiteral("Вычислительные конвейеры Vulkan: %1"), QStringLiteral("Vulkan Compute Pipelines: %1"), QStringLiteral("Vulkan-Compute-Pipelines: %1"), QStringLiteral("Pipelines de calcul Vulkan : %1"), QStringLiteral("Vulkan 计算管线: %1"), QStringLiteral("Vulkan コンピュートパイプライン: %1"), QStringLiteral("خطوط الحوسبة Vulkan: %1"), QStringLiteral("Tuberías de cómputo Vulkan: %1")).arg(bval());
    }
    if (k == "Renderer\\dma_accuracy" || k == "dma_accuracy") {
        QString mode = (v == "1") ? StormLang(QStringLiteral("Обычная"), QStringLiteral("Normal"), QStringLiteral("Normal"), QStringLiteral("Normale"), QStringLiteral("正常"), QStringLiteral("通常"), QStringLiteral("عادي"), QStringLiteral("Normal"))
            : (v == "2") ? StormLang(QStringLiteral("Небезопасно"), QStringLiteral("Unsafe"), QStringLiteral("Unsicher"), QStringLiteral("Non sécurisé"), QStringLiteral("不安全"), QStringLiteral("非安全"), QStringLiteral("غير آمن"), QStringLiteral("Inseguro"))
            : (v == "3") ? StormLang(QStringLiteral("Безопасно"), QStringLiteral("Safe"), QStringLiteral("Sicher"), QStringLiteral("Sécurisé"), QStringLiteral("安全"), QStringLiteral("安全"), QStringLiteral("آمن"), QStringLiteral("Seguro"))
            : StormLang(QStringLiteral("По умолчанию"), QStringLiteral("Default"), QStringLiteral("Standard"), QStringLiteral("Par défaut"), QStringLiteral("默认"), QStringLiteral("デフォルト"), QStringLiteral("افتراضي"), QStringLiteral("Predeterminado"));
        return StormLang(QStringLiteral("Точность DMA: %1"), QStringLiteral("DMA Accuracy: %1"), QStringLiteral("DMA-Genauigkeit: %1"), QStringLiteral("Précision DMA : %1"), QStringLiteral("DMA 精度: %1"), QStringLiteral("DMA 精度: %1"), QStringLiteral("دقة DMA: %1"), QStringLiteral("Precisión DMA: %1")).arg(mode);
    }
    if (k == "Renderer\\vram_garbage_collection" || k == "vram_garbage_collection") {
        return StormLang(QStringLiteral("Очистка видеопамяти (VRAM GC): %1"), QStringLiteral("VRAM Garbage Collection: %1"), QStringLiteral("VRAM-Speicherbereinigung: %1"), QStringLiteral("Nettoyage mémoire VRAM : %1"), QStringLiteral("显存垃圾回收 (VRAM GC): %1"), QStringLiteral("VRAM ガベージコレクション: %1"), QStringLiteral("تنظيف ذاكرة VRAM: %1"), QStringLiteral("Limpieza de VRAM (GC): %1")).arg(bval());
    }
    if (k == "Renderer\\force_max_clock" || k == "force_max_clock") {
        return StormLang(QStringLiteral("Максимальная частота ГП (Turbo): %1"), QStringLiteral("Force Max GPU Clock (Turbo): %1"), QStringLiteral("Maximaler GPU-Takt (Turbo): %1"), QStringLiteral("Fréquence GPU max (Turbo) : %1"), QStringLiteral("强制最高 GPU 频率 (Turbo): %1"), QStringLiteral("最大 GPU クロック強制 (Turbo): %1"), QStringLiteral("أقصى تردد لـ GPU (تربو): %1"), QStringLiteral("Frecuencia máxima de GPU (Turbo): %1")).arg(bval());
    }
    if (k == "Renderer\\enable_frame_skipping" || k == "enable_frame_skipping") {
        return StormLang(QStringLiteral("Пропуск кадров: %1"), QStringLiteral("Frame Skipping: %1"), QStringLiteral("Frame-Skipping: %1"), QStringLiteral("Saut d'images : %1"), QStringLiteral("跳帧优化: %1"), QStringLiteral("フレームスキップ: %1"), QStringLiteral("تخطي الإطارات: %1"), QStringLiteral("Salto de fotogramas: %1")).arg(bval());
    }
    if (k == "Renderer\\vertex_input_dynamic_state" || k == "vertex_input_dynamic_state") {
        return StormLang(QStringLiteral("Динамический ввод вершин: %1"), QStringLiteral("Vertex Input Dynamic State: %1"), QStringLiteral("Dynamischer Vertex-Input: %1"), QStringLiteral("État dynamique d'entrée sommet : %1"), QStringLiteral("顶点输入动态状态: %1"), QStringLiteral("頂点入力動的ステート: %1"), QStringLiteral("حالة إدخال الرؤوس الديناميكية: %1"), QStringLiteral("Estado dinámico de vértices: %1")).arg(bval());
    }
    if (k.find("airplane_mode") != std::string::npos) {
        return StormLang(QStringLiteral("Режим полёта (Офлайн-сеть): %1"), QStringLiteral("Airplane Mode (Offline LAN): %1"), QStringLiteral("Flugmodus (Offline): %1"), QStringLiteral("Mode avion (Hors-ligne) : %1"), QStringLiteral("飞行模式 (离线防卡死): %1"), QStringLiteral("機内モード (オフライン): %1"), QStringLiteral("وضع الطيران (دون اتصال): %1"), QStringLiteral("Modo avión (Sin conexión): %1")).arg(bval());
    }
    if (k.find("memory_layout_mode") != std::string::npos) {
        QString gb = (v == "0") ? QStringLiteral("4 GB") : (v == "1") ? QStringLiteral("6 GB") : (v == "2") ? QStringLiteral("8 GB") : QStringLiteral("12 GB");
        return StormLang(QStringLiteral("Объём памяти консоли: %1"), QStringLiteral("Console RAM Layout: %1"), QStringLiteral("Konsolen-RAM-Layout: %1"), QStringLiteral("Mémoire RAM console : %1"), QStringLiteral("主机内存布局: %1"), QStringLiteral("本体メモリ構成: %1"), QStringLiteral("حجم ذاكرة النظام: %1"), QStringLiteral("Memoria RAM de consola: %1")).arg(gb);
    }
    if (k.find("cpu_accuracy") != std::string::npos) {
        QString mode = (v == "0") ? StormLang(QStringLiteral("Авто"), QStringLiteral("Auto"), QStringLiteral("Auto"), QStringLiteral("Auto"), QStringLiteral("自动"), QStringLiteral("自動"), QStringLiteral("تلقائي"), QStringLiteral("Auto"))
            : (v == "1") ? StormLang(QStringLiteral("Точно"), QStringLiteral("Accurate"), QStringLiteral("Genau"), QStringLiteral("Précis"), QStringLiteral("精确"), QStringLiteral("正確"), QStringLiteral("دقيق"), QStringLiteral("Preciso"))
            : (v == "2") ? StormLang(QStringLiteral("Небезопасно"), QStringLiteral("Unsafe"), QStringLiteral("Unsicher"), QStringLiteral("Non sécurisé"), QStringLiteral("不安全"), QStringLiteral("非安全"), QStringLiteral("غير آمن"), QStringLiteral("Inseguro"))
            : StormLang(QStringLiteral("Параноидально"), QStringLiteral("Paranoid"), QStringLiteral("Paranoid"), QStringLiteral("Paranoïaque"), QStringLiteral("极严"), QStringLiteral("高精度"), QStringLiteral("شديد الدقة"), QStringLiteral("Paranoico"));
        return StormLang(QStringLiteral("Точность ЦП: %1"), QStringLiteral("CPU Accuracy: %1"), QStringLiteral("CPU-Genauigkeit: %1"), QStringLiteral("Précision CPU : %1"), QStringLiteral("CPU 精度: %1"), QStringLiteral("CPU 精度: %1"), QStringLiteral("دقة CPU: %1"), QStringLiteral("Precisión CPU: %1")).arg(mode);
    }
    if (k.find("cpu_backend") != std::string::npos) {
        QString mode = (v == "1") ? QStringLiteral("NCE (Native)") : QStringLiteral("Dynarmic JIT");
        return StormLang(QStringLiteral("Движок ЦП: %1"), QStringLiteral("CPU Backend: %1"), QStringLiteral("CPU-Backend: %1"), QStringLiteral("Moteur CPU : %1"), QStringLiteral("CPU 后端: %1"), QStringLiteral("CPU バックエンド: %1"), QStringLiteral("محرك CPU: %1"), QStringLiteral("Motor CPU: %1")).arg(mode);
    }
    if (k.find("cpuopt_fastmem_exclusives") != std::string::npos) {
        return StormLang(QStringLiteral("Fastmem для эксклюзивных атомиков: %1"), QStringLiteral("Fastmem Exclusives: %1"), QStringLiteral("Fastmem-Exclusives: %1"), QStringLiteral("Fastmem exclusifs : %1"), QStringLiteral("独占原子操作 Fastmem: %1"), QStringLiteral("排他アクセス Fastmem: %1"), QStringLiteral("ذاكرة سريعة للعمليات الحصرية: %1"), QStringLiteral("Fastmem exclusivos: %1")).arg(bval());
    }
    if (k.find("cpuopt_fastmem") != std::string::npos) {
        return StormLang(QStringLiteral("Быстрая память ЦП (Fastmem): %1"), QStringLiteral("CPU Fastmem: %1"), QStringLiteral("CPU-Fastmem: %1"), QStringLiteral("Fastmem CPU : %1"), QStringLiteral("CPU 快速内存 (Fastmem): %1"), QStringLiteral("CPU Fastmem: %1"), QStringLiteral("الذاكرة السريعة لـ CPU: %1"), QStringLiteral("Memoria rápida CPU (Fastmem): %1")).arg(bval());
    }
    if (k.find("cpuopt_ignore_memory_aborts") != std::string::npos) {
        return StormLang(QStringLiteral("Игнорировать сбои страниц памяти: %1"), QStringLiteral("Ignore Memory Aborts: %1"), QStringLiteral("Speicherabbrüche ignorieren: %1"), QStringLiteral("Ignorer les erreurs mémoire : %1"), QStringLiteral("忽略内存访问异常: %1"), QStringLiteral("メモリ中断を無視: %1"), QStringLiteral("تجاهل أخطاء الذاكرة: %1"), QStringLiteral("Ignorar abortos de memoria: %1")).arg(bval());
    }
    if (k.find("cpuopt_recompile_exclusives") != std::string::npos) {
        return StormLang(QStringLiteral("Рекомпиляция атомарных инструкций: %1"), QStringLiteral("Recompile Exclusives: %1"), QStringLiteral("Exclusives rekompilieren: %1"), QStringLiteral("Recompiler les exclusifs : %1"), QStringLiteral("重编译独占指令: %1"), QStringLiteral("排他命令の再コンパイル: %1"), QStringLiteral("إعادة ترجمة التعليمات الحصرية: %1"), QStringLiteral("Recompilar exclusivos: %1")).arg(bval());
    }
    if (k.find("cpuopt_unsafe_unfuse_fma") != std::string::npos) {
        return StormLang(QStringLiteral("Разделение инструкций FMA: %1"), QStringLiteral("Unfuse FMA Instructions: %1"), QStringLiteral("FMA-Instruktionen trennen: %1"), QStringLiteral("Séparer les instructions FMA : %1"), QStringLiteral("拆分 FMA 浮点指令: %1"), QStringLiteral("FMA 命令の分離: %1"), QStringLiteral("فصل تعليمات FMA: %1"), QStringLiteral("Separar instrucciones FMA: %1")).arg(bval());
    }
    if (k.find("cpuopt_unsafe_host_mmu") != std::string::npos) {
        return StormLang(QStringLiteral("Аппаратный MMU хоста: %1"), QStringLiteral("Host MMU Acceleration: %1"), QStringLiteral("Host-MMU-Beschleunigung: %1"), QStringLiteral("Accélération MMU hôte : %1"), QStringLiteral("主机硬件 MMU 加速: %1"), QStringLiteral("ホスト MMU 高速化: %1"), QStringLiteral("تسريع MMU المضيف: %1"), QStringLiteral("Aceleración MMU del host: %1")).arg(bval());
    }
    if (k.find("cpuopt_unsafe_reduce_fp_error") != std::string::npos) {
        return StormLang(QStringLiteral("Точность чисел с плавающей точкой: %1"), QStringLiteral("Reduce Floating-Point Error: %1"), QStringLiteral("Gleitkommafehler reduzieren: %1"), QStringLiteral("Réduire l'erreur flottante : %1"), QStringLiteral("降低浮点运算误差: %1"), QStringLiteral("浮動小数点誤差の低減: %1"), QStringLiteral("تقليل خطأ الفاصلة العائمة: %1"), QStringLiteral("Reducir error de coma flotante: %1")).arg(bval());
    }
    if (k.find("cpuopt_unsafe_ignore_standard_fpcr") != std::string::npos) {
        return StormLang(QStringLiteral("Ускорение регистра FPCR: %1"), QStringLiteral("Fast FPCR Register Mode: %1"), QStringLiteral("Schneller FPCR-Modus: %1"), QStringLiteral("Mode registre FPCR rapide : %1"), QStringLiteral("快速 FPCR 寄存器模式: %1"), QStringLiteral("高速 FPCR レジスタ: %1"), QStringLiteral("تسريع سجل FPCR: %1"), QStringLiteral("Modo rápido FPCR: %1")).arg(bval());
    }
    if (k.find("use_storm_tier2_compiler") != std::string::npos) {
        return StormLang(QStringLiteral("Компилятор STORM Tier-2: %1"), QStringLiteral("STORM Tier-2 Compiler: %1"), QStringLiteral("STORM Tier-2-Compiler: %1"), QStringLiteral("Compilateur STORM Tier-2 : %1"), QStringLiteral("STORM Tier-2 编译器: %1"), QStringLiteral("STORM Tier-2 コンパイラ: %1"), QStringLiteral("مترجم STORM Tier-2: %1"), QStringLiteral("Compilador STORM Tier-2: %1")).arg(bval());
    }
    if (k.find("sync_core_speed") != std::string::npos) {
        return StormLang(QStringLiteral("Синхронизация частоты ядер: %1"), QStringLiteral("Sync Core Speed: %1"), QStringLiteral("Kerngeschwindigkeit synchronisieren: %1"), QStringLiteral("Synchroniser la vitesse des cœurs : %1"), QStringLiteral("同步核心速度: %1"), QStringLiteral("コア速度の同期: %1"), QStringLiteral("مزامنة سرعة النواة: %1"), QStringLiteral("Sincronizar velocidad de núcleos: %1")).arg(bval());
    }
    if (k.find("disable_macro_hle") != std::string::npos) {
        return StormLang(QStringLiteral("Отключить макро-HLE ГП: %1"), QStringLiteral("Disable GPU Macro HLE: %1"), QStringLiteral("GPU-Makro-HLE deaktivieren: %1"), QStringLiteral("Désactiver Macro HLE GPU : %1"), QStringLiteral("禁用 GPU 宏 HLE: %1"), QStringLiteral("GPU マクロ HLE 無効化: %1"), QStringLiteral("تعطيل ماكرو GPU HLE: %1"), QStringLiteral("Desactivar Macro HLE de GPU: %1")).arg(bval());
    }
    if (k.find("disable_macro_jit") != std::string::npos) {
        return StormLang(QStringLiteral("Отключить макро-JIT ГП: %1"), QStringLiteral("Disable GPU Macro JIT: %1"), QStringLiteral("GPU-Makro-JIT deaktivieren: %1"), QStringLiteral("Désactiver Macro JIT GPU : %1"), QStringLiteral("禁用 GPU 宏 JIT: %1"), QStringLiteral("GPU マクロ JIT 無効化: %1"), QStringLiteral("تعطيل ماكرو GPU JIT: %1"), QStringLiteral("Desactivar Macro JIT de GPU: %1")).arg(bval());
    }
    if (k.find("disable_web_applet") != std::string::npos) {
        return StormLang(QStringLiteral("Блокировка встроенного веб-апплета: %1"), QStringLiteral("Disable Web Applet: %1"), QStringLiteral("Web-Applet deaktivieren: %1"), QStringLiteral("Désactiver l'applet Web : %1"), QStringLiteral("禁用内置网页小程序: %1"), QStringLiteral("Web アプレット無効化: %1"), QStringLiteral("تعطيل تطبيق الويب المصغر: %1"), QStringLiteral("Desactivar applet web: %1")).arg(bval());
    }
    if (k.find("use_auto_stub") != std::string::npos) {
        return StormLang(QStringLiteral("Авто-заглушки сервисов HLE: %1"), QStringLiteral("HLE Service Auto-Stub: %1"), QStringLiteral("HLE-Service Auto-Stub: %1"), QStringLiteral("Auto-Stub des services HLE : %1"), QStringLiteral("HLE 服务自动桩: %1"), QStringLiteral("HLE サービス自動スタブ: %1"), QStringLiteral("محاكاة خدمات HLE التلقائية: %1"), QStringLiteral("Auto-Stub de servicios HLE: %1")).arg(bval());
    }

    // Fallback: strip section prefix and localize boolean value
    std::string clean_k = k;
    const auto slash = clean_k.find('\\');
    if (slash != std::string::npos) {
        clean_k = clean_k.substr(slash + 1);
    }
    for (char& ch : clean_k) {
        if (ch == '_') ch = ' ';
    }
    if (!clean_k.empty()) {
        clean_k[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(clean_k[0])));
    }
    const QString disp_v = (v == "true" || v == "false" || v == "1" || v == "0")
        ? bval()
        : QString::fromStdString(v);
    return QStringLiteral("%1: %2").arg(QString::fromStdString(clean_k), disp_v);
}

} // anonymous namespace

MainWindow::GameFixDialogResult MainWindow::ShowGameFixDialog(u64 title_id, const QString& game_path, bool force_show) {
    if (title_id == 0) {
        static const QRegularExpression tid_regex(QStringLiteral(R"(([0-9a-fA-F]{16}))"));
        const auto match = tid_regex.match(game_path);
        if (match.hasMatch()) {
            bool ok = false;
            const u64 parsed = match.captured(1).toULongLong(&ok, 16);
            if (ok && parsed != 0) {
                title_id = parsed;
            }
        }
    }

    const auto* profile = Core::GameFixDatabase::GetProfileByTitleOrPath(title_id, game_path.toStdString());
    if (profile != nullptr && title_id == 0) {
        title_id = profile->title_id;
    }
    if (profile == nullptr && title_id != 0) {
        profile = Core::GameFixDatabase::GetProfile(title_id);
    }

    Core::GameFixDatabase::SetExcludedKeys({});

    if (profile == nullptr) {
        return GameFixDialogResult::LaunchWithoutChanges;
    }

    QByteArray utf8_str = game_path.toUtf8();
    const auto file_path_hash = Common::CityHash64(utf8_str.constData(), static_cast<std::size_t>(utf8_str.size()));
    const auto specific_config = fmt::format("{:016X}_{:016X}", title_id, file_path_hash);
    const auto legacy_config = fmt::format("{:016X}", title_id);

    std::filesystem::path custom_path = Common::FS::GetEdenPath(Common::FS::EdenPath::ConfigDir) / "custom";
    std::string target_ini = (custom_path / (specific_config + ".ini")).string();
    std::string check_ini = target_ini;
    if (!std::filesystem::exists(check_ini) && std::filesystem::exists(custom_path / (legacy_config + ".ini"))) {
        check_ini = (custom_path / (legacy_config + ".ini")).string();
    }

    bool dont_ask = false;
    if (std::filesystem::exists(check_ini)) {
        std::ifstream f(check_ini);
        std::string l;
        while (std::getline(f, l)) {
            if (l.find("storm_fix_dont_ask=true") != std::string::npos || l.find("storm_fix_dont_ask = true") != std::string::npos) {
                dont_ask = true;
                break;
            }
        }
    }

    if ((dont_ask || m_is_cmd_line_launch) && !force_show) {
        Core::GameFixDatabase::SetFixesEnabled(true);
        return GameFixDialogResult::ApplyAndLaunch;
    }

    QString clean_game_name = QString::fromStdString(profile->game_name);
    clean_game_name.remove(QRegularExpression(QStringLiteral(R"(\s*\((?:Alt|Rev|v|Build)[^)]*\))"), QRegularExpression::CaseInsensitiveOption));
    clean_game_name.remove(QRegularExpression(QStringLiteral(R"(\s*\[[^\]]*\])")));
    clean_game_name = clean_game_name.trimmed();
    if (clean_game_name.isEmpty()) {
        clean_game_name = StormLang(QStringLiteral("Игра"), QStringLiteral("Game"), QStringLiteral("Spiel"), QStringLiteral("Jeu"), QStringLiteral("游戏"), QStringLiteral("ゲーム"), QStringLiteral("لعبة"), QStringLiteral("Juego"));
    }

    QDialog fixDialog(this);
    fixDialog.setWindowTitle(StormLang(
        QStringLiteral("🛡️ Авто-исправление: %1"),
        QStringLiteral("🛡️ Auto-Fix Profile: %1"),
        QStringLiteral("🛡️ Auto-Fix-Profil: %1"),
        QStringLiteral("🛡️ Profil d'auto-correction : %1"),
        QStringLiteral("🛡️ 自动修复配置: %1"),
        QStringLiteral("🛡️ 自動修正プロファイル: %1"),
        QStringLiteral("🛡️ الإصلاح التلقائي: %1"),
        QStringLiteral("🛡️ Auto-corrección: %1")).arg(clean_game_name));
    fixDialog.setWindowFlags(fixDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    fixDialog.setMinimumSize(820, 490);
    fixDialog.setMaximumWidth(960);
    fixDialog.resize(860, 520);

    fixDialog.setStyleSheet(QStringLiteral(
        "QDialog {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #111622, stop:0.6 #0C101A, stop:1 #080B12);"
        "    border: 1.5px solid rgba(0, 210, 255, 0.40);"
        "    border-radius: 12px;"
        "}"
        "QLabel { color: #E2E8F0; font-family: 'Segoe UI', sans-serif; }"
    ));

    // 2-COLUMN MASTER LAYOUT: Left = Info + Scroll + Checkbox, Right = Action Panel with Buttons
    auto* main_hlayout = new QHBoxLayout(&fixDialog);
    main_hlayout->setContentsMargins(16, 14, 16, 14);
    main_hlayout->setSpacing(14);

    auto* left_col = new QVBoxLayout();
    left_col->setContentsMargins(0, 0, 0, 0);
    left_col->setSpacing(10);

    // 1. Header Card with Game Title (Left Column Top)
    auto* headerCard = new QFrame(&fixDialog);
    headerCard->setStyleSheet(QStringLiteral(
        "QFrame {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(0, 210, 255, 0.16), stop:1 rgba(15, 23, 42, 0.65));"
        "    border: 1px solid rgba(0, 210, 255, 0.35);"
        "    border-radius: 10px;"
        "}"
    ));
    auto* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 10, 14, 10);
    headerLayout->setSpacing(12);

    auto* gameIconLabel = new QLabel(headerCard);
    gameIconLabel->setText(QStringLiteral("🛡️"));
    gameIconLabel->setStyleSheet(QStringLiteral("font-size: 24px; background: transparent; border: none;"));
    headerLayout->addWidget(gameIconLabel);

    auto* titleContainer = new QVBoxLayout();
    titleContainer->setContentsMargins(0, 0, 0, 0);
    titleContainer->setSpacing(2);

    auto* gameTitleLabel = new QLabel(clean_game_name, headerCard);
    gameTitleLabel->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: 800; color: #FFFFFF; background: transparent; border: none;"));
    titleContainer->addWidget(gameTitleLabel);

    if (title_id != 0) {
        const QString tid_str = QString::asprintf("%016llX", static_cast<unsigned long long>(title_id));
        auto* tidLabel = new QLabel(StormLang(
            QStringLiteral("ID игры: %1"),
            QStringLiteral("Title ID: %1"),
            QStringLiteral("Titel-ID: %1"),
            QStringLiteral("ID du titre : %1"),
            QStringLiteral("游戏 ID: %1"),
            QStringLiteral("タイトル ID: %1"),
            QStringLiteral("معرّف اللعبة: %1"),
            QStringLiteral("ID del título: %1")).arg(tid_str), headerCard);
        tidLabel->setStyleSheet(QStringLiteral("font-size: 11.5px; color: #38BDF8; font-weight: 600; background: transparent; border: none; font-family: 'Consolas', monospace;"));
        titleContainer->addWidget(tidLabel);
    }
    headerLayout->addLayout(titleContainer, 1);
    left_col->addWidget(headerCard);

    // 2. Scroll Area for Issue Card, Fix Card, and Localized Parameters (Left Column Middle)
    auto* scrollArea = new QScrollArea(&fixDialog);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet(QStringLiteral(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollArea::corner { background: transparent; border: none; }"
        "QScrollBar:vertical {"
        "    background: rgba(15, 23, 42, 0.65);"
        "    width: 8px;"
        "    margin: 2px 0px 2px 0px;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: rgba(0, 210, 255, 0.50);"
        "    min-height: 28px;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background: #00E5FF;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; background: none; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
    ));

    auto* scrollWidget = new QWidget();
    scrollWidget->setStyleSheet(QStringLiteral("background: transparent;"));
    auto* scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->setContentsMargins(0, 0, 8, 0);
    scrollLayout->setSpacing(10);

    std::string ui_lang = UISettings::values.language.GetValue();
    if (ui_lang.empty()) {
        ui_lang = QLocale::system().name().toStdString();
    }
    const bool is_russian = ui_lang.rfind("ru", 0) == 0 || ui_lang.rfind("uk", 0) == 0 || ui_lang.rfind("be", 0) == 0;
    QString issues_raw = QString::fromStdString(is_russian ? profile->issues_ru : profile->issues_en);
    if (issues_raw.isEmpty()) {
        issues_raw = QString::fromStdString(profile->issues_en.empty() ? profile->issues_ru : profile->issues_en);
    }
    QString issues_formatted = issues_raw;
    issues_formatted.replace(QStringLiteral("\n"), QStringLiteral("<br>"));

    auto* issueCard = new QFrame(scrollWidget);
    issueCard->setStyleSheet(QStringLiteral(
        "QFrame {"
        "    background: rgba(239, 68, 68, 0.09);"
        "    border: 1px solid rgba(239, 68, 68, 0.38);"
        "    border-radius: 9px;"
        "}"
    ));
    auto* issueLayout = new QVBoxLayout(issueCard);
    issueLayout->setContentsMargins(12, 9, 12, 9);
    issueLayout->setSpacing(5);

    auto* issueHeader = new QLabel(StormLang(
        QStringLiteral("⚠️ <b>Обнаружены известные проблемы в игре:</b>"),
        QStringLiteral("⚠️ <b>Known Game Issues Detected:</b>"),
        QStringLiteral("⚠️ <b>Bekannte Spielprobleme erkannt:</b>"),
        QStringLiteral("⚠️ <b>Problèmes connus détectés dans le jeu :</b>"),
        QStringLiteral("⚠️ <b>检测到已知游戏问题：</b>"),
        QStringLiteral("⚠️ <b>既知のゲーム問題が検出されました:</b>"),
        QStringLiteral("⚠️ <b>تم اكتشاف مشاكل معروفة في اللعبة:</b>"),
        QStringLiteral("⚠️ <b>Problemas conocidos detectados en el juego:</b>")), issueCard);
    issueHeader->setStyleSheet(QStringLiteral("color: #F87171; font-size: 12.5px; background: transparent; border: none;"));
    issueLayout->addWidget(issueHeader);

    auto* issueText = new QLabel(issues_formatted, issueCard);
    issueText->setTextFormat(Qt::RichText);
    issueText->setWordWrap(true);
    issueText->setStyleSheet(QStringLiteral("color: #FCA5A5; font-size: 11.5px; line-height: 1.38; background: transparent; border: none;"));
    issueLayout->addWidget(issueText);
    scrollLayout->addWidget(issueCard);

    // 3. Recommended Fixes Card
    QString fixes_raw = QString::fromStdString(is_russian ? profile->fixes_ru : profile->fixes_en);
    if (fixes_raw.isEmpty()) {
        fixes_raw = QString::fromStdString(profile->fixes_en.empty() ? profile->fixes_ru : profile->fixes_en);
    }
    QString fixes_formatted = fixes_raw;
    if (!fixes_formatted.contains(QStringLiteral("<b>"))) {
        fixes_formatted.replace(QRegularExpression(QStringLiteral(R"((•|\-)\s*([^:\n<]+):)")), QStringLiteral("• <b>\\2:</b>"));
        fixes_formatted.replace(QRegularExpression(QStringLiteral(R"(^(🎮|⚙️|🌐|⚡|🛠️)\s*([^:\n<]+):)"), QRegularExpression::MultilineOption), QStringLiteral("\\1 <b>\\2:</b>"));
    }
    fixes_formatted.replace(QStringLiteral("\n"), QStringLiteral("<br>"));

    auto* fixCard = new QFrame(scrollWidget);
    fixCard->setStyleSheet(QStringLiteral(
        "QFrame {"
        "    background: rgba(0, 210, 255, 0.07);"
        "    border: 1px solid rgba(0, 210, 255, 0.36);"
        "    border-radius: 9px;"
        "}"
    ));
    auto* fixLayout = new QVBoxLayout(fixCard);
    fixLayout->setContentsMargins(12, 9, 12, 9);
    fixLayout->setSpacing(6);

    auto* fixHeader = new QLabel(StormLang(
        QStringLiteral("🛡️ <b>Параметры авто-исправления (совместимость и стабильность):</b>"),
        QStringLiteral("🛡️ <b>Auto-Fix Parameters (Compatibility & Stability):</b>"),
        QStringLiteral("🛡️ <b>Auto-Fix-Parameter (Kompatibilität & Stabilität):</b>"),
        QStringLiteral("🛡️ <b>Paramètres d'auto-correction (Compatibilité et stabilité) :</b>"),
        QStringLiteral("🛡️ <b>自动修复参数（兼容性与稳定性）：</b>"),
        QStringLiteral("🛡️ <b>自動修正パラメータ（互換性と安定性）:</b>"),
        QStringLiteral("🛡️ <b>معلمات الإصلاح التلقائي (التوافق والاستقرار):</b>"),
        QStringLiteral("🛡️ <b>Parámetros de auto-corrección (Compatibilidad y estabilidad):</b>")), fixCard);
    fixHeader->setStyleSheet(QStringLiteral("color: #00E5FF; font-size: 12.5px; background: transparent; border: none;"));
    fixLayout->addWidget(fixHeader);

    auto* fixText = new QLabel(fixes_formatted, fixCard);
    fixText->setTextFormat(Qt::RichText);
    fixText->setWordWrap(true);
    fixText->setStyleSheet(QStringLiteral("color: #E2E8F0; font-size: 11.5px; line-height: 1.42; background: transparent; border: none;"));
    fixLayout->addWidget(fixText);

    // Interactive per-parameter checkboxes (100% localized human-readable names & values)
    std::vector<std::pair<std::string, QCheckBox*>> param_checkboxes;
    if (!profile->ini_settings.empty()) {
        auto* paramsContainer = new QFrame(fixCard);
        paramsContainer->setStyleSheet(QStringLiteral(
            "QFrame {"
            "    background: rgba(15, 23, 42, 0.65);"
            "    border: 1px solid rgba(56, 189, 248, 0.25);"
            "    border-radius: 7px;"
            "    margin-top: 4px;"
            "}"
        ));
        auto* paramsLayout = new QVBoxLayout(paramsContainer);
        paramsLayout->setContentsMargins(10, 8, 10, 8);
        paramsLayout->setSpacing(6);

        auto* paramsTitle = new QLabel(StormLang(
            QStringLiteral("⚙️ <b>Выбор применяемых параметров (можно отключить отдельные пункты):</b>"),
            QStringLiteral("⚙️ <b>Select Applied Parameters (individual items can be toggled):</b>"),
            QStringLiteral("⚙️ <b>Anzuwendende Parameter auswählen (einzelne Punkte abwählbar):</b>"),
            QStringLiteral("⚙️ <b>Sélection des paramètres appliqués (ajustables individuellement) :</b>"),
            QStringLiteral("⚙️ <b>选择要应用的参数（可单独取消勾选）：</b>"),
            QStringLiteral("⚙️ <b>適用するパラメータの選択（個別に切り替え可能）:</b>"),
            QStringLiteral("⚙️ <b>تحديد المعلمات المطبقة (يمكن تعطيل عناصر فردية):</b>"),
            QStringLiteral("⚙️ <b>Selección de parámetros aplicados (se pueden desmarcar):</b>")), paramsContainer);
        paramsTitle->setStyleSheet(QStringLiteral("color: #94A3B8; font-size: 11.5px; background: transparent; border: none;"));
        paramsLayout->addWidget(paramsTitle);

        auto* paramsGrid = new QGridLayout();
        paramsGrid->setContentsMargins(0, 2, 0, 0);
        paramsGrid->setHorizontalSpacing(14);
        paramsGrid->setVerticalSpacing(5);

        int idx = 0;
        for (const auto& [k, v] : profile->ini_settings) {
            if (k == "aspect_ratio" || k == "Renderer\\aspect_ratio" ||
                k == "resolution_setup" || k == "Renderer\\resolution_setup" ||
                k == "use_docked_mode" || k == "System\\use_docked_mode" ||
                k == "anti_aliasing" || k == "Renderer\\anti_aliasing" ||
                k == "scaling_filter" || k == "Renderer\\scaling_filter" ||
                k == "fsr_sharpening_slider" || k == "Renderer\\fsr_sharpening_slider" ||
                k == "max_anisotropy" || k == "Renderer\\max_anisotropy" ||
                k == "frame_pacing_mode" || k == "Renderer\\frame_pacing_mode" ||
                k == "backend" || k == "Renderer\\backend") {
                continue;
            }
            const QString localized_param = FormatLocalizedIniParam(k, v);
            auto* cb = new QCheckBox(localized_param, paramsContainer);
            cb->setToolTip(QStringLiteral("%1 = %2").arg(QString::fromStdString(k), QString::fromStdString(v)));
            cb->setChecked(true);
            cb->setStyleSheet(QStringLiteral(
                "QCheckBox {"
                "    color: #E2E8F0;"
                "    font-size: 11.5px;"
                "    font-family: 'Segoe UI', sans-serif;"
                "    spacing: 7px;"
                "    background: transparent;"
                "    border: none;"
                "}"
                "QCheckBox:hover { color: #38BDF8; }"
                "QCheckBox::indicator {"
                "    width: 14px;"
                "    height: 14px;"
                "    border-radius: 3px;"
                "    border: 1px solid rgba(0, 210, 255, 0.55);"
                "    background: rgba(15, 23, 42, 0.85);"
                "}"
                "QCheckBox::indicator:checked {"
                "    background: #00D2FF;"
                "    border: 1px solid #00F0FF;"
                "}"
            ));
            paramsGrid->addWidget(cb, idx / 2, idx % 2);
            param_checkboxes.emplace_back(k, cb);
            ++idx;
        }
        paramsLayout->addLayout(paramsGrid);
        if (!param_checkboxes.empty()) {
            fixLayout->addWidget(paramsContainer);
        } else {
            paramsContainer->deleteLater();
        }
    }

    scrollLayout->addWidget(fixCard);
    scrollLayout->addStretch(1);

    scrollArea->setWidget(scrollWidget);
    left_col->addWidget(scrollArea, 1);

    // 4. Bottom of Left Column: Prompt + "Do not show again" Checkbox
    auto* leftFooterCard = new QFrame(&fixDialog);
    leftFooterCard->setStyleSheet(QStringLiteral(
        "QFrame {"
        "    background: rgba(15, 23, 42, 0.72);"
        "    border: 1px solid rgba(148, 163, 184, 0.22);"
        "    border-radius: 8px;"
        "}"
    ));
    auto* leftFooterLayout = new QHBoxLayout(leftFooterCard);
    leftFooterLayout->setContentsMargins(12, 8, 12, 8);
    leftFooterLayout->setSpacing(12);

    auto* dont_ask_cb = new QCheckBox(StormLang(
        QStringLiteral("Больше не показывать для этой игры"),
        QStringLiteral("Do not show again for this game"),
        QStringLiteral("Für dieses Spiel nicht mehr anzeigen"),
        QStringLiteral("Ne plus afficher pour ce jeu"),
        QStringLiteral("不再对此游戏显示"),
        QStringLiteral("このゲームでは今後表示しない"),
        QStringLiteral("عدم الإظهار مرة أخرى لهذه اللعبة"),
        QStringLiteral("No volver a mostrar para este juego")), leftFooterCard);
    dont_ask_cb->setChecked(false);
    dont_ask_cb->setCursor(Qt::PointingHandCursor);
    dont_ask_cb->setStyleSheet(QStringLiteral(
        "QCheckBox {"
        "    color: #CBD5E1;"
        "    font-size: 12px;"
        "    font-weight: 600;"
        "    spacing: 8px;"
        "    background: transparent;"
        "    border: none;"
        "}"
        "QCheckBox:hover { color: #FFFFFF; }"
        "QCheckBox::indicator {"
        "    width: 16px;"
        "    height: 16px;"
        "    border-radius: 4px;"
        "    border: 1px solid rgba(0, 210, 255, 0.55);"
        "    background: rgba(15, 23, 42, 0.85);"
        "}"
        "QCheckBox::indicator:hover {"
        "    border: 1px solid #00E5FF;"
        "    background: rgba(0, 210, 255, 0.18);"
        "}"
        "QCheckBox::indicator:checked {"
        "    background: #00D2FF;"
        "    border: 1px solid #00F0FF;"
        "}"
    ));
    leftFooterLayout->addWidget(dont_ask_cb, 0, Qt::AlignLeft | Qt::AlignVCenter);
    leftFooterLayout->addStretch(1);
    left_col->addWidget(leftFooterCard);

    main_hlayout->addLayout(left_col, 1);

    // =========================================================================
    // RIGHT COLUMN: Styled Vertical Action Panel with Choice Buttons
    // =========================================================================
    auto* rightCard = new QFrame(&fixDialog);
    rightCard->setFixedWidth(250);
    rightCard->setStyleSheet(QStringLiteral(
        "QFrame {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 rgba(19, 28, 46, 0.92), stop:1 rgba(11, 15, 25, 0.96));"
        "    border: 1px solid rgba(0, 210, 255, 0.32);"
        "    border-radius: 10px;"
        "}"
    ));
    auto* right_col = new QVBoxLayout(rightCard);
    right_col->setContentsMargins(14, 14, 14, 14);
    right_col->setSpacing(10);

    auto* actionHeader = new QLabel(StormLang(
        QStringLiteral("⚡ ВЫБОР ДЕЙСТВИЯ"),
        QStringLiteral("⚡ SELECT ACTION"),
        QStringLiteral("⚡ AKTION WÄHLEN"),
        QStringLiteral("⚡ CHOISIR L'ACTION"),
        QStringLiteral("⚡ 选择操作"),
        QStringLiteral("⚡ アクション選択"),
        QStringLiteral("⚡ اختر الإجراء"),
        QStringLiteral("⚡ SELECCIONAR ACCIÓN")), rightCard);
    actionHeader->setAlignment(Qt::AlignCenter);
    actionHeader->setStyleSheet(QStringLiteral(
        "color: #00E5FF; font-size: 12px; font-weight: 800; letter-spacing: 0.8px;"
        "background: rgba(0, 210, 255, 0.10); border: 1px solid rgba(0, 210, 255, 0.25);"
        "border-radius: 6px; padding: 6px 8px;"
    ));
    right_col->addWidget(actionHeader);

    auto* promptLabel = new QLabel(force_show
        ? StormLang(
            QStringLiteral("Применить параметры авто-исправления к профилю игры?"),
            QStringLiteral("Apply auto-fix parameters to this game's profile?"),
            QStringLiteral("Auto-Fix-Parameter auf dieses Spielprofil anwenden?"),
            QStringLiteral("Appliquer les paramètres d'auto-correction au profil du jeu ?"),
            QStringLiteral("将自动修复参数应用到此游戏配置？"),
            QStringLiteral("自動修正パラメータをこのゲームプロファイルに適用しますか？"),
            QStringLiteral("هل تريد تطبيق معلمات الإصلاح التلقائي على ملف اللعبة؟"),
            QStringLiteral("¿Aplicar parámetros de auto-corrección al perfil del juego?"))
        : StormLang(
            QStringLiteral("Применить рекомендуемые исправления перед запуском игры?"),
            QStringLiteral("Apply recommended fixes before launching the game?"),
            QStringLiteral("Empfohlene Korrekturen vor dem Spielstart anwenden?"),
            QStringLiteral("Appliquer les corrections recommandées avant de lancer le jeu ?"),
            QStringLiteral("在启动游戏前应用推荐的修复参数？"),
            QStringLiteral("ゲーム起動前に推奨される修正を適用しますか？"),
            QStringLiteral("هل تريد تطبيق الإصلاحات الموصى بها قبل تشغيل اللعبة؟"),
            QStringLiteral("¿Aplicar las correcciones recomendadas antes de iniciar el juego?")), rightCard);
    promptLabel->setWordWrap(true);
    promptLabel->setAlignment(Qt::AlignCenter);
    promptLabel->setStyleSheet(QStringLiteral("font-size: 11.5px; color: #CBD5E1; background: transparent; border: none; padding: 2px 2px;"));
    right_col->addWidget(promptLabel);

    right_col->addSpacing(4);

    auto* applyBtn = new QPushButton(force_show
        ? StormLang(QStringLiteral("🛡️ Применить для игры"), QStringLiteral("🛡️ Apply to Game"), QStringLiteral("🛡️ Auf Spiel anwenden"), QStringLiteral("🛡️ Appliquer au jeu"), QStringLiteral("🛡️ 应用到游戏"), QStringLiteral("🛡️ ゲームに適用"), QStringLiteral("🛡️ تطبيق على اللعبة"), QStringLiteral("🛡️ Aplicar al juego"))
        : StormLang(QStringLiteral("🛡️ Применить и запустить"), QStringLiteral("🛡️ Apply & Launch"), QStringLiteral("🛡️ Anwenden & Starten"), QStringLiteral("🛡️ Appliquer et lancer"), QStringLiteral("🛡️ 应用并启动"), QStringLiteral("🛡️ 適用して起動"), QStringLiteral("🛡️ تطبيق وتشغيل"), QStringLiteral("🛡️ Aplicar e iniciar")), rightCard);
    applyBtn->setObjectName(QStringLiteral("PrimaryDialogButton"));
    applyBtn->setCursor(Qt::PointingHandCursor);
    applyBtn->setMinimumHeight(44);
    applyBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00E5FF, stop:1 #0284C7);"
        "    color: #040914;"
        "    font-weight: 800;"
        "    font-size: 12.5px;"
        "    padding: 8px 12px;"
        "    border-radius: 7px;"
        "    border: 1px solid #67E8F9;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #67E8F9, stop:1 #00E5FF);"
        "    color: #000000;"
        "}"
        "QPushButton:pressed {"
        "    background: #0369A1;"
        "    color: #FFFFFF;"
        "}"
    ));

    auto* skipBtn = new QPushButton(force_show
        ? StormLang(QStringLiteral("Закрыть"), QStringLiteral("Close"), QStringLiteral("Schließen"), QStringLiteral("Fermer"), QStringLiteral("关闭"), QStringLiteral("閉じる"), QStringLiteral("إغلاق"), QStringLiteral("Cerrar"))
        : StormLang(QStringLiteral("Запустить без изменений"), QStringLiteral("Launch Without Changes"), QStringLiteral("Ohne Änderungen starten"), QStringLiteral("Lancer sans modification"), QStringLiteral("不作更改直接启动"), QStringLiteral("変更せずに起動"), QStringLiteral("تشغيل بدون تغييرات"), QStringLiteral("Iniciar sin cambios")), rightCard);
    skipBtn->setCursor(Qt::PointingHandCursor);
    skipBtn->setMinimumHeight(40);
    skipBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "    background: rgba(30, 41, 59, 0.85);"
        "    color: #E2E8F0;"
        "    font-weight: 600;"
        "    font-size: 12px;"
        "    padding: 8px 12px;"
        "    border-radius: 7px;"
        "    border: 1px solid rgba(148, 163, 184, 0.35);"
        "}"
        "QPushButton:hover {"
        "    background: rgba(51, 65, 85, 0.95);"
        "    border: 1px solid #38BDF8;"
        "    color: #FFFFFF;"
        "}"
        "QPushButton:pressed {"
        "    background: rgba(15, 23, 42, 0.95);"
        "}"
    ));

    QPushButton* cancelBtn = nullptr;
    if (!force_show) {
        cancelBtn = new QPushButton(StormLang(
            QStringLiteral("Отменить"),
            QStringLiteral("Cancel"),
            QStringLiteral("Abbrechen"),
            QStringLiteral("Annuler"),
            QStringLiteral("取消"),
            QStringLiteral("キャンセル"),
            QStringLiteral("إلغاء"),
            QStringLiteral("Cancelar")), rightCard);
        cancelBtn->setCursor(Qt::PointingHandCursor);
        cancelBtn->setMinimumHeight(40);
        cancelBtn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "    background: rgba(46, 16, 20, 0.80);"
            "    color: #FCA5A5;"
            "    font-weight: 600;"
            "    font-size: 12px;"
            "    padding: 8px 12px;"
            "    border-radius: 7px;"
            "    border: 1px solid rgba(239, 68, 68, 0.40);"
            "}"
            "QPushButton:hover {"
            "    background: rgba(239, 68, 68, 0.88);"
            "    border: 1px solid #FF5252;"
            "    color: #FFFFFF;"
            "}"
            "QPushButton:pressed {"
            "    background: #991B1B;"
            "}"
        ));
    }

    right_col->addWidget(applyBtn);
    right_col->addWidget(skipBtn);
    if (cancelBtn) {
        right_col->addWidget(cancelBtn);
    }

    right_col->addStretch(1);

    auto* infoBadge = new QLabel(StormLang(
        QStringLiteral("💡 Выбранные исправления применяются только к текущей игре и не затрагивают глобальные настройки."),
        QStringLiteral("💡 Selected fixes apply only to this game and do not alter your global settings."),
        QStringLiteral("💡 Ausgewählte Korrekturen gelten nur für dieses Spiel und ändern keine globalen Einstellungen."),
        QStringLiteral("💡 Les corrections sélectionnées s'appliquent uniquement à ce jeu sans modifier vos paramètres globaux."),
        QStringLiteral("💡 所选修复仅应用于当前游戏，不会修改您的全局设置。"),
        QStringLiteral("💡 選択した修正はこのゲームのみに適用され、全体設定には影響しません。"),
        QStringLiteral("💡 تُطبّق الإصلاحات المحددة على هذه اللعبة فقط ولا تغير الإعدادات العامة."),
        QStringLiteral("💡 Las correcciones seleccionadas solo se aplican a este juego y no alteran la configuración global.")), rightCard);
    infoBadge->setWordWrap(true);
    infoBadge->setAlignment(Qt::AlignCenter);
    infoBadge->setStyleSheet(QStringLiteral(
        "color: #94A3B8; font-size: 10.5px; background: rgba(15, 23, 42, 0.65);"
        "border: 1px solid rgba(148, 163, 184, 0.16); border-radius: 6px; padding: 7px;"
    ));
    right_col->addWidget(infoBadge);

    main_hlayout->addWidget(rightCard);

    GameFixDialogResult action = GameFixDialogResult::Cancel;

    connect(applyBtn, &QPushButton::clicked, &fixDialog, [&fixDialog, &action] {
        action = GameFixDialogResult::ApplyAndLaunch;
        fixDialog.accept();
    });
    connect(skipBtn, &QPushButton::clicked, &fixDialog, [&fixDialog, &action] {
        action = GameFixDialogResult::LaunchWithoutChanges;
        fixDialog.accept();
    });
    if (cancelBtn) {
        connect(cancelBtn, &QPushButton::clicked, &fixDialog, [&fixDialog, &action] {
            action = GameFixDialogResult::Cancel;
            fixDialog.reject();
        });
    }

    fixDialog.exec();

    if (action == GameFixDialogResult::Cancel) {
        return GameFixDialogResult::Cancel;
    }

    const bool new_dont_ask = dont_ask_cb->isChecked();
    Core::GameFixDatabase::SetDontAskAgain(title_id, target_ini, new_dont_ask);
    Core::GameFixDatabase::SetDontAskAgain(title_id, (custom_path / (legacy_config + ".ini")).string(), new_dont_ask);

    if (action == GameFixDialogResult::ApplyAndLaunch) {
        std::vector<std::string> excluded_keys;
        for (const auto& [key, cb] : param_checkboxes) {
            if (cb && !cb->isChecked()) {
                excluded_keys.push_back(key);
            }
        }
        Core::GameFixDatabase::SetExcludedKeys(excluded_keys);
        Core::GameFixDatabase::SetFixesEnabled(true);
        if (force_show) {
            Core::GameFixDatabase::ApplyProfileDirectly(title_id, game_path.toStdString());
            QMessageBox::information(this, tr("🛡️ Авто-исправление"),
                tr("Параметры авто-исправления успешно применены для текущей сессии игры: %1").arg(clean_game_name));
        }
        return GameFixDialogResult::ApplyAndLaunch;
    }
    Core::GameFixDatabase::SetFixesEnabled(false);
    return GameFixDialogResult::LaunchWithoutChanges;
}

void MainWindow::OnResetGameFixSuppression() {
    int count = Core::GameFixDatabase::ResetAllDontAskAgain();
    if (count > 0) {
        QMessageBox::information(this, tr("Авто-исправления STORM SWITCH"),
            tr("Сброшено скрытых диалогов авто-исправлений: %1.\nПри следующем запуске оптимизированных игр диалог будет предложен снова.").arg(count));
    } else {
        QMessageBox::information(this, tr("Авто-исправления STORM SWITCH"),
            tr("Нет скрытых диалогов авто-исправлений. Все диалоги уже активны."));
    }
}

void MainWindow::BootGame(const QString& filename, Service::AM::FrontendAppletParameters params,
                          StartGameType type) {
    if (emulation_running || (QtCommon::emu_thread && QtCommon::emu_thread->isRunning()) || QtCommon::system->IsPoweredOn()) {
        LOG_WARNING(Frontend, "Emulation is already running or powering on. Ignoring BootGame request.");
        return;
    }

    LOG_INFO(Frontend, "STORM SWITCH starting...");

    if (game_list) {
        game_list->CancelPopulate();
        game_list->setDisabled(true);
    }

    if (params.program_id == 0 ||
        params.program_id > static_cast<u64>(Service::AM::AppletProgramId::MaxProgramId)) {
        StoreRecentFile(filename); // Put the filename on top of the list
    }

    // Save UI settings without blocking boot with heavy synchronous file writing
    UpdateUISettings();
    game_list->SaveInterfaceLayout();

    m_session_backup.Capture();

    u64 title_id{0};
    if (params.program_id != 0) {
        title_id = params.program_id;
    } else {
        static const QRegularExpression tid_regex(QStringLiteral(R"(([0-9a-fA-F]{16}))"));
        const auto match = tid_regex.match(filename);
        if (match.hasMatch()) {
            bool ok = false;
            const u64 parsed = match.captured(1).toULongLong(&ok, 16);
            if (ok && parsed != 0) {
                title_id = parsed;
            }
        }
    }

    last_filename_booted = filename;

    const auto utf8_str = filename.toUtf8();
    QtCommon::Content::configureFilesystemProvider(filename.toStdString());

    if (title_id == 0) {
        const auto v_file = Core::GetGameFileFromPath(QtCommon::vfs, utf8_str.constData());
        const auto loader =
            Loader::GetLoader(*QtCommon::system, v_file, params.program_id, params.program_index);

        if (loader != nullptr) {
            loader->ReadProgramId(title_id);
            if (title_id == 0) {
                std::vector<u64> pids;
                if (loader->ReadProgramIds(pids) == Loader::ResultStatus::Success && !pids.empty()) {
                    title_id = pids[0];
                }
            }
        }
    }

    if (params.program_id == 0 && title_id != 0) {
        params.program_id = title_id;
    }

    if (title_id != 0 && type == StartGameType::Normal) {
        const auto file_path_hash = Common::CityHash64(utf8_str.constData(), static_cast<std::size_t>(utf8_str.size()));
        const auto specific_config = fmt::format("{:016X}_{:016X}", title_id, file_path_hash);
        const auto legacy_config = fmt::format("{:016X}", title_id);

        std::filesystem::path custom_path = Common::FS::GetEdenPath(Common::FS::EdenPath::ConfigDir) / "custom";
        std::string target_ini = (custom_path / (specific_config + ".ini")).string();

        // Load per game settings FIRST if custom configuration file exists so user per-game
        // overrides have UsingGlobal() == false before Auto-Fix runs!
        std::string config_to_load = specific_config;
        bool has_custom_ini = std::filesystem::exists(custom_path / (specific_config + ".ini"));
        if (!has_custom_ini && std::filesystem::exists(custom_path / (legacy_config + ".ini"))) {
            config_to_load = legacy_config;
            has_custom_ini = true;
        }

        if (has_custom_ini) {
            QtConfig per_game_config(config_to_load, Config::ConfigType::PerGameConfig);
        }
    }

    if (type == StartGameType::Normal) {
        if (params.launch_type == Service::AM::LaunchType::ApplicationInitiated) {
            Core::GameFixDatabase::SetFixesEnabled(true);
        } else {
            const auto fix_result = ShowGameFixDialog(title_id, filename, false /* force_show */);
            if (fix_result == GameFixDialogResult::Cancel) {
                LOG_INFO(Frontend, "Запуск игры отменен пользователем в диалоге авто-исправлений");
                RestoreSessionSettings();
                m_session_backup.is_active = false;
                game_list->setEnabled(true);
                return;
            }
        }
    }

    if (title_id != 0 && type == StartGameType::Normal) {

        // Always guarantee global player control bindings are preserved unless a custom profile was explicitly set
        bool has_custom_profile = false;
        for (const auto& player : Settings::values.players.GetValue()) {
            if (!player.profile_name.empty()) {
                has_custom_profile = true;
                break;
            }
        }
        if (!has_custom_profile) {
            Settings::values.players.SetGlobal(true);
        }

        QtCommon::system->HIDCore().ReloadInputDevices();
        QtCommon::system->ApplySettings();

        if (Core::GameFixDatabase::AreFixesEnabled()) {
            Core::GameFixDatabase::ApplyProfileDirectly(title_id, filename.toStdString());
            QtCommon::system->ApplySettings();
            statusBar()->showMessage(tr("🛡️ Авто-исправление: Применено"), 8000);
        } else {
            Core::GameFixDatabase::SanitizeSettingsForHostHardware();
            QtCommon::system->ApplySettings();
            if (Core::GameFixDatabase::GetProfileByTitleOrPath(title_id, filename.toStdString()) != nullptr) {
                statusBar()->showMessage(tr("⚠️ Авто-исправление: Не применено"), 8000);
            }
        }
        UpdateStatusButtons();
    }

    Settings::LogSettings();

    if (UISettings::values.select_user_on_boot && !user_flag_cmd_line) {
        const Core::Frontend::ProfileSelectParameters parameters{
            .mode = Service::AM::Frontend::UiMode::UserSelector,
            .invalid_uid_list = {},
            .display_options = {},
            .purpose = Service::AM::Frontend::UserSelectionPurpose::General,
        };
        if (SelectAndSetCurrentUser(parameters) == false) {
            RestoreSessionSettings();
            m_session_backup.is_active = false;
            game_list->setEnabled(true);
            return;
        }
    }

    // If the user specifies -u (successfully) on the cmd line, don't prompt for a user on first
    // game startup only. If the user stops emulation and starts a new one, go back to the expected
    // behavior of asking.
    user_flag_cmd_line = false;
    m_is_cmd_line_launch = false;

    if (!LoadROM(filename, params)) {
        RestoreSessionSettings();
        m_session_backup.is_active = false;
        game_list->setEnabled(true);
        return;
    }

    QtCommon::system->SetShuttingDown(false);
    game_list->setDisabled(true);

    std::string title_name;
    std::string title_version;
    std::string title_developer;
    QPixmap game_icon_pix;
    const auto res = QtCommon::system->GetGameName(title_name);

    const FileSys::PatchManager pm(title_id, QtCommon::system->GetFileSystemController(),
                                   QtCommon::system->GetContentProvider());

    if (QtCommon::system != nullptr && QtCommon::system->HasAppLoader()) {
        FileSys::NACP app_nacp{};
        if (QtCommon::system->GetAppLoader().ReadControlData(app_nacp) == Loader::ResultStatus::Success) {
            title_version = app_nacp.GetVersionString();
            if (title_name.empty()) {
                title_name = app_nacp.GetApplicationName();
            }
            title_developer = app_nacp.GetDeveloperName();
        }
    }

    if (title_id != 0) {
        const auto cached_icon_path = Common::FS::PathToUTF8String(
            Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) / "game_list" /
            fmt::format("{:016X}.jpeg", title_id));
        if (Common::FS::Exists(cached_icon_path)) {
            game_icon_pix.load(QString::fromStdString(cached_icon_path));
        }
    }

    if (game_icon_pix.isNull() && QtCommon::system != nullptr && QtCommon::system->HasAppLoader()) {
        std::vector<u8> bytes;
        if (QtCommon::system->GetAppLoader().ReadIcon(bytes) == Loader::ResultStatus::Success && !bytes.empty()) {
            game_icon_pix.loadFromData(bytes.data(), static_cast<u32>(bytes.size()));
        }
    }

    if (title_name.empty() || title_version.empty() || game_icon_pix.isNull()) {
        const auto metadata = pm.GetControlMetadata();
        if (metadata.first != nullptr) {
            if (title_version.empty()) {
                title_version = metadata.first->GetVersionString();
            }
            if (title_name.empty()) {
                title_name = metadata.first->GetApplicationName();
            }
            if (title_developer.empty()) {
                title_developer = metadata.first->GetDeveloperName();
            }
        }
        if (game_icon_pix.isNull() && metadata.second != nullptr) {
            const auto bytes = metadata.second->ReadAllBytes();
            game_icon_pix.loadFromData(bytes.data(), static_cast<u32>(bytes.size()));
        }
    }
    if (res != Loader::ResultStatus::Success || title_name.empty()) {
        title_name = Common::FS::PathToUTF8String(
            std::filesystem::path{Common::U16StringFromBuffer(filename.utf16(), filename.size())}
                .filename());
    }
    const auto full_file_info_name = QFileInfo(filename).fileName();
    QString resolved_display_version = QString::fromStdString(title_version);

    u32 raw_internal_version = pm.GetGameVersion().value_or(0);
    static const QRegularExpression fn_pair_ver_regex{QStringLiteral(R"(\(([0-9]+\.[0-9]+(?:\.[0-9]+)*)\s*-\s*([0-9]+))")};
    const auto fm = fn_pair_ver_regex.match(full_file_info_name);
    if (fm.hasMatch() && !fm.captured(1).isEmpty()) {
        resolved_display_version = fm.captured(1);
        if (raw_internal_version == 0 && !fm.captured(2).isEmpty()) {
            raw_internal_version = fm.captured(2).toUInt();
        }
    } else {
        static const QRegularExpression fn_ver_regex{QStringLiteral(R"((?:[\(\[\s]v?|\b)([0-9]+\.[0-9]+(?:\.[0-9]+)*)(?!\s*(?:GB|MB|KB|TB|ГБ|МБ|КБ|Б|B)\b))")};
        const auto m = fn_ver_regex.match(full_file_info_name);
        if (m.hasMatch() && m.hasCaptured(1)) {
            if (resolved_display_version.isEmpty() || resolved_display_version == QStringLiteral("1.0.0") || resolved_display_version == QStringLiteral("0")) {
                resolved_display_version = m.captured(1);
            }
        }
    }

    while (resolved_display_version.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        resolved_display_version.remove(0, 1);
    }
    resolved_display_version = resolved_display_version.trimmed();
    if (resolved_display_version.isEmpty() || resolved_display_version == QStringLiteral("0") || resolved_display_version == QStringLiteral("PACKED")) {
        resolved_display_version = QStringLiteral("1.0.0");
    }

    if (raw_internal_version == 0) {
        static const QRegularExpression ver_regex(QStringLiteral(R"([\[\(_]v(\d+)[\]\)]|[-_\s](\d{5,8})[-_\s\)])"), QRegularExpression::CaseInsensitiveOption);
        const auto match = ver_regex.match(full_file_info_name);
        if (match.hasMatch()) {
            for (int i = 1; i <= match.lastCapturedIndex(); ++i) {
                const auto cap = match.captured(i);
                if (!cap.isEmpty()) {
                    raw_internal_version = cap.toUInt();
                    break;
                }
            }
        }
    }

    const std::string display_version_str = resolved_display_version.toStdString();
    if (raw_internal_version == 0 && !display_version_str.empty() && display_version_str != "1.0.0" && display_version_str != "1.0") {
        int major = 1, minor = 0, patch = 0;
        if (std::sscanf(display_version_str.c_str(), "%d.%d.%d", &major, &minor, &patch) >= 2) {
            if (major >= 1) {
                raw_internal_version = static_cast<u32>((major - 1) * 655360 + minor * 65536 + (patch * 65536) / 10);
            }
        }
    }
    title_version = display_version_str;
    const std::string internal_version_str = std::to_string(raw_internal_version);
    std::string raw_gpu_vendor = "GPU";
    try {
        if (QtCommon::system != nullptr) {
            raw_gpu_vendor = QtCommon::system->GPU().Renderer().GetDeviceVendor();
        }
    } catch (...) {
        raw_gpu_vendor = "VULKAN GPU";
    }
    const auto gpu_vendor = QString::fromStdString(raw_gpu_vendor).toUpper().toStdString();

    const auto full_file_name = full_file_info_name.toStdString();
    LOG_INFO(Frontend, "Booting game: {:016X} | {} | {} | {} | {}", title_id, full_file_name, display_version_str, internal_version_str, gpu_vendor);
    UpdateWindowTitle(full_file_name, display_version_str, internal_version_str, gpu_vendor);
    m_current_addons_game_path = filename.toStdString();
    UpdateAddonsStatusButton(title_id, QString::fromStdString(title_name));

    const QString file_ext = QFileInfo(filename).suffix().toUpper();
    loading_screen->SetGameInfo(
        QString::fromStdString(title_name),
        QString::fromStdString(title_version),
        QString::fromStdString(title_developer),
        title_id,
        game_icon_pix,
        file_ext
    );
    if (QtCommon::system != nullptr && QtCommon::system->HasAppLoader()) {
        loading_screen->Prepare(QtCommon::system->GetAppLoader());
    }
    if (ui->action_Single_Window_Mode->isChecked()) {
        loading_screen->setGeometry(ui->centralwidget->rect());
    }
    loading_screen->show();
    loading_screen->raise();

    // Create emulation thread and connect progress signals BEFORE starting execution
    QtCommon::emu_thread = std::make_unique<EmuThread>();
    connect(QtCommon::emu_thread.get(), &EmuThread::LoadProgress, loading_screen,
            &LoadingScreen::OnLoadProgress, Qt::QueuedConnection);
    emit EmulationStarting();
    QtCommon::emu_thread->start();

    // Register an ExecuteProgram callback such that Core can execute a sub-program
    QtCommon::system->RegisterExecuteProgramCallback(
        [this](std::size_t program_index_) { render_window->ExecuteProgram(program_index_); });

    QtCommon::system->RegisterExitCallback([this] {
        QtCommon::emu_thread->ForceStop();
        render_window->Exit();
    });

    connect(render_window, &GRenderWindow::Closed, this, &MainWindow::OnStopGame);
    connect(render_window, &GRenderWindow::MouseActivity, this, &MainWindow::OnMouseActivity);

    // Update the GUI
    UpdateStatusButtons();
    if (ui->action_Single_Window_Mode->isChecked()) {
        game_list->hide();
        game_list_placeholder->hide();
        render_window->show();
        render_window->setFocus();
    }
    status_bar_update_timer.start(250);
    renderer_status_button->setDisabled(true);
    refresh_button->setDisabled(true);
    SetFPSSuffix();

    if (UISettings::values.hide_mouse || Settings::values.mouse_panning) {
        render_window->installEventFilter(render_window);
        render_window->setAttribute(Qt::WA_Hover, true);
    }

    if (UISettings::values.hide_mouse) {
        mouse_hide_timer.start();
    }

    emulation_running = true;
    if (ui->action_Fullscreen->isChecked()) {
        ShowFullscreen();
    }
    if (ui->action_Single_Window_Mode->isChecked() && loading_screen->isVisible()) {
        loading_screen->setGeometry(ui->centralwidget->rect());
        loading_screen->raise();
    }
    OnStartGame();
}

void MainWindow::BootGameFromList(const QString& filename, StartGameType with_config) {
    BootGame(filename, ApplicationAppletParameters(), with_config);
}

bool MainWindow::OnShutdownBegin() {
    if (!emulation_running) {
        return false;
    }

    if (ui->action_Fullscreen->isChecked()) {
        HideFullscreen();
    }

    AllowOSSleep();

    // Immediately halt performance stats updates to prevent accessing destroyed GPU cores
    status_bar_update_timer.stop();
    shader_building_label->setVisible(false);
    res_scale_label->setVisible(false);
    emu_speed_label->setVisible(false);
    game_fps_label->setVisible(false);
    emu_frametime_label->setVisible(false);

    // Disable unlimited frame rate and turbo/slow modes
    Settings::values.use_speed_limit.SetValue(true);
    Settings::values.current_speed_mode = Settings::SpeedMode::Standard;

    if (QtCommon::system->IsShuttingDown()) {
        return false;
    }

    if (perf_overlay) {
        perf_overlay->hide();
        perf_overlay->deleteLater();
        perf_overlay = nullptr;
    }

    QtCommon::system->SetShuttingDown(true);
    discord_rpc->Pause();

    RequestGameExit();
    QtCommon::emu_thread->disconnect();
    QtCommon::emu_thread->SetRunning(true);

    connect(QtCommon::emu_thread.get(), &QThread::finished, this, &MainWindow::OnEmulationStopped);
    emit EmulationStopping();

    int shutdown_time = 1000;

    if (QtCommon::system->DebuggerEnabled()) {
        shutdown_time = 0;
    } else if (QtCommon::system->GetExitLocked()) {
        shutdown_time = 5000;
    }

    shutdown_timer.stop();
    shutdown_timer.disconnect();
    shutdown_timer.setSingleShot(true);
    connect(&shutdown_timer, &QTimer::timeout, this, &MainWindow::OnEmulationStopTimeExpired);
    shutdown_timer.start(shutdown_time);

    // Disable everything to prevent anything from being triggered here
    ui->action_Pause->setEnabled(false);
    ui->action_Restart->setEnabled(false);
    ui->action_Stop->setEnabled(false);

    if (ui->action_Single_Window_Mode->isChecked()) {
        loading_screen->setGeometry(ui->centralwidget->rect());
        loading_screen->ShowShutdownState();
    }

    return true;
}

void MainWindow::OnShutdownBeginDialog() {
    if (ui->action_Single_Window_Mode->isChecked()) {
        loading_screen->ShowShutdownState();
    } else {
        shutdown_dialog =
            new OverlayDialog(this, *QtCommon::system, QString{}, tr("Closing software..."), QString{},
                              QString{}, Qt::AlignHCenter | Qt::AlignVCenter);
        shutdown_dialog->open();
    }
}

void MainWindow::OnEmulationStopTimeExpired() {
    if (QtCommon::emu_thread) {
        QtCommon::emu_thread->ForceStop();
    }
}

void MainWindow::OnEmulationStopped() {
    static bool s_is_stopping = false;
    if (s_is_stopping) {
        return;
    }
    s_is_stopping = true;
    SCOPE_EXIT {
        s_is_stopping = false;
    };

    status_bar_update_timer.stop();
    shutdown_timer.stop();
    shutdown_timer.disconnect();
    if (QtCommon::emu_thread) {
        QtCommon::emu_thread->disconnect();
        if (QtCommon::emu_thread->isRunning()) {
            QtCommon::emu_thread->ForceStop();
            // Wait synchronously for EmuThread to exit cleanly without re-entrant event pumping
            if (!QtCommon::emu_thread->wait(5000)) {
                LOG_WARNING(Frontend, "EmuThread did not exit within 5s; waiting an additional 5s");
                if (!QtCommon::emu_thread->wait(5000)) {
                    LOG_CRITICAL(Frontend, "EmuThread hung during shutdown, forcefully terminating before context cleanup");
                    QtCommon::emu_thread->terminate();
                    QtCommon::emu_thread->wait(2000);
                }
            }
        }
        Core::FlushPendingSaveStates();
        if (QtCommon::system) {
            const u64 stopped_prog_id = QtCommon::system->GetApplicationProcessProgramID();
            if (stopped_prog_id != 0) {
                StormSaveSyncDialog::CreateAutoBackupForTitle(stopped_prog_id);
            }
        }
        QtCommon::emu_thread.reset();
    }

    if (shutdown_dialog) {
        shutdown_dialog->deleteLater();
        shutdown_dialog = nullptr;
    }

    emulation_running = false;

    if (floating_translate_button) {
        floating_translate_button->SetVisibleState(false);
    }

    discord_rpc->Update();
    Common::FeralGamemode::Stop();

    // The emulation is stopped, so closing the window or not does not matter anymore
    disconnect(render_window, &GRenderWindow::Closed, this, &MainWindow::OnStopGame);

    // Update the GUI
    UpdateMenuState();

    render_window->hide();
    loading_screen->hide();
    loading_screen->Clear();
    if (game_list->IsEmpty()) {
        game_list_placeholder->show();
    } else {
        game_list->show();
    }
    game_list->SetFilterFocus();
    tas_label->clear();
    input_subsystem->GetTas()->Stop();
    OnTasStateChanged();
    render_window->FinalizeCamera();

    QtCommon::system->GetFrontendAppletHolder().SetCurrentAppletId(Service::AM::AppletId::None);

    // Enable all controllers
    QtCommon::system->HIDCore().SetSupportedStyleTag({Core::HID::NpadStyleSet::All});

    render_window->removeEventFilter(render_window);
    render_window->setAttribute(Qt::WA_Hover, false);

    UpdateWindowTitle();

    // Disable status bar updates
    status_bar_update_timer.stop();
    shader_building_label->setVisible(false);
    res_scale_label->setVisible(false);
    emu_speed_label->setVisible(false);
    game_fps_label->setVisible(false);
    emu_frametime_label->setVisible(false);
    renderer_status_button->setEnabled(!UISettings::values.has_broken_vulkan);
    refresh_button->setEnabled(true);

    if (!firmware_label->text().isEmpty()) {
        firmware_label->setVisible(true);
    }

    current_game_path.clear();

    // When closing the game, destroy the GLWindow to clear the context after the game is closed
    render_window->ReleaseRenderTarget();

    // Enable game list
    game_list->setEnabled(true);

    RestoreSessionSettings();
    Settings::RestoreGlobalState(false);
    if (config) {
        config->SaveAllValues();
    }
    QtCommon::system->HIDCore().ReloadInputDevices();
    UpdateStatusButtons();
    UpdateGPUAccuracyButton();
    UpdateAirplaneModeButton();
    UpdateAstcRecompressText();
    UpdateAnisotropyText();
    UpdateResScaleText();
    UpdateVSyncText();
    UpdateSpeedLimitText();
    UpdateNvdecText();
    UpdateDockedButton();
    m_is_stopping_emulation = false;
    // Update game list to show new play time
    game_list->PopulateAsync(UISettings::values.game_dirs);
}

void MainWindow::ShutdownGame() {
    if (!emulation_running) {
        return;
    }

    // TODO(crueter): make this common as well (frontend_common?)
    play_time_manager->Stop();
    OnShutdownBegin();
    OnEmulationStopTimeExpired();
    OnEmulationStopped();
}

void MainWindow::StoreRecentFile(const QString& filename) {
    UISettings::values.recent_files.prepend(filename);
    UISettings::values.recent_files.removeDuplicates();
    while (UISettings::values.recent_files.size() > max_recent_files_item) {
        UISettings::values.recent_files.removeLast();
    }

    UpdateRecentFiles();
}

void MainWindow::UpdateRecentFiles() {
    const int num_recent_files =
        (std::min)(static_cast<int>(UISettings::values.recent_files.size()), max_recent_files_item);

    for (int i = 0; i < num_recent_files; i++) {
        const QString text = QStringLiteral("&%1. %2").arg(i + 1).arg(
            QFileInfo(UISettings::values.recent_files[i]).fileName());
        actions_recent_files[i]->setText(text);
        actions_recent_files[i]->setData(UISettings::values.recent_files[i]);
        actions_recent_files[i]->setToolTip(UISettings::values.recent_files[i]);
        actions_recent_files[i]->setVisible(true);
    }

    for (int j = num_recent_files; j < max_recent_files_item; ++j) {
        actions_recent_files[j]->setVisible(false);
    }

    // Enable the recent files menu if the list isn't empty
    ui->menu_recent_files->setEnabled(num_recent_files != 0);
}

void MainWindow::OnGameListLoadFile(QString game_path, u64 program_id) {
    if (emulation_running || (QtCommon::emu_thread && QtCommon::emu_thread->isRunning()) || QtCommon::system->IsPoweredOn()) {
        return;
    }
    auto params = ApplicationAppletParameters();
    params.program_id = program_id;

    BootGame(game_path, params);
}

// TODO(crueter): Common profile selector
void MainWindow::OnGameListOpenFolder(u64 program_id, GameListOpenTarget target,
                                      const std::string& game_path) {
    std::filesystem::path path;
    QString open_target;

    const auto [user_save_size, device_save_size] = [&game_path, &program_id] {
        const FileSys::PatchManager pm{program_id, QtCommon::system->GetFileSystemController(),
                                       QtCommon::system->GetContentProvider()};
        const auto control = pm.GetControlMetadata().first;
        if (control != nullptr) {
            return std::make_pair(control->GetDefaultNormalSaveSize(),
                                  control->GetDeviceSaveDataSize());
        } else {
            const auto file = Core::GetGameFileFromPath(QtCommon::vfs, game_path);
            const auto loader = Loader::GetLoader(*QtCommon::system, file);

            FileSys::NACP nacp{};
            loader->ReadControlData(nacp);
            return std::make_pair(nacp.GetDefaultNormalSaveSize(), nacp.GetDeviceSaveDataSize());
        }
    }();

    const bool has_user_save{user_save_size > 0};
    const bool has_device_save{device_save_size > 0};

    ASSERT_MSG(has_user_save != has_device_save, "Game uses both user and device savedata?");

    switch (target) {
    case GameListOpenTarget::SaveData: {
        open_target = tr("Save Data");
        const auto save_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::SaveDir);
        auto vfs_save_dir = QtCommon::vfs->OpenDirectory(Common::FS::PathToUTF8String(save_dir),
                                                         FileSys::OpenMode::Read);

        if (has_user_save) {
            // User save data
            const auto user_id = GetProfileID();
            assert(user_id);

            const auto user_save_data_path = FileSys::SaveDataFactory::GetFullPath(
                {}, vfs_save_dir, FileSys::SaveDataSpaceId::User, FileSys::SaveDataType::Account,
                program_id, user_id->AsU128(), 0);

            path = Common::FS::ConcatPathSafe(save_dir, user_save_data_path);
        } else {
            // Device save data
            const auto device_save_data_path = FileSys::SaveDataFactory::GetFullPath(
                {}, vfs_save_dir, FileSys::SaveDataSpaceId::User, FileSys::SaveDataType::Account,
                program_id, {}, 0);

            path = Common::FS::ConcatPathSafe(save_dir, device_save_data_path);
        }

        if (!Common::FS::CreateDirs(path)) {
            LOG_ERROR(Frontend, "Unable to create the directories for save data");
        }

        break;
    }
    case GameListOpenTarget::ModData: {
        open_target = tr("Mod Data");
        path = Common::FS::GetEdenPath(Common::FS::EdenPath::LoadDir) /
               fmt::format("{:016X}", program_id);
        break;
    }
    default:
        LOG_WARNING(Frontend, "Unimplemented GameListOpenTarget");
        return;
    }

    const QString qpath = QString::fromStdString(Common::FS::PathToUTF8String(path));
    const QDir dir(qpath);
    if (!dir.exists()) {
        QMessageBox::warning(this, tr("Error Opening %1 Folder").arg(open_target),
                             tr("Folder does not exist!"));
        return;
    }
    LOG_INFO(Frontend, "Opening {} path for program_id={:016x}", open_target.toStdString(),
             program_id);
    QDesktopServices::openUrl(QUrl::fromLocalFile(qpath));
}

static bool RomFSRawCopy(size_t total_size, size_t& read_size, QProgressDialog& dialog,
                         const FileSys::VirtualDir& src, const FileSys::VirtualDir& dest,
                         bool full) {
    if (src == nullptr || dest == nullptr || !src->IsReadable() || !dest->IsWritable())
        return false;
    if (dialog.wasCanceled())
        return false;

    std::vector<u8> buffer(CopyBufferSize);
    auto last_timestamp = std::chrono::steady_clock::now();

    const auto QtRawCopy = [&](const FileSys::VirtualFile& src_file,
                               const FileSys::VirtualFile& dest_file) {
        if (src_file == nullptr || dest_file == nullptr) {
            return false;
        }
        if (!dest_file->Resize(src_file->GetSize())) {
            return false;
        }

        for (std::size_t i = 0; i < src_file->GetSize(); i += buffer.size()) {
            if (dialog.wasCanceled()) {
                dest_file->Resize(0);
                return false;
            }

            using namespace std::literals::chrono_literals;
            const auto new_timestamp = std::chrono::steady_clock::now();

            if ((new_timestamp - last_timestamp) > 33ms) {
                last_timestamp = new_timestamp;
                dialog.setValue(
                    static_cast<int>((std::min)(read_size, total_size) * 100 / total_size));
                QCoreApplication::processEvents();
            }

            const auto read = src_file->Read(buffer.data(), buffer.size(), i);
            dest_file->Write(buffer.data(), read, i);

            read_size += read;
        }

        return true;
    };

    if (full) {
        for (const auto& file : src->GetFiles()) {
            const auto out = VfsDirectoryCreateFileWrapper(dest, file->GetName());
            if (!QtRawCopy(file, out))
                return false;
        }
    }

    for (const auto& dir : src->GetSubdirectories()) {
        const auto out = dest->CreateSubdirectory(dir->GetName());
        if (!RomFSRawCopy(total_size, read_size, dialog, dir, out, full))
            return false;
    }

    return true;
}

// TODO(crueter): All this can be transfered to qt_common
// Aldoe I need to decide re: message boxes for QML
// translations_common? strings_common? qt_strings? who knows
void MainWindow::OnGameListRemoveInstalledEntry(u64 program_id,
                                                QtCommon::Game::InstalledEntryType type) {
    const std::string& lang = UISettings::values.language.GetValue();
    const bool is_ru = lang.rfind("ru", 0) == 0;
    const QString entry_question = [type, is_ru] {
        switch (type) {
        case QtCommon::Game::InstalledEntryType::Game:
            return is_ru ? QStringLiteral("Удалить установленный контент игры?")
                         : tr("Remove Installed Game Contents?");
        case QtCommon::Game::InstalledEntryType::Update:
            return is_ru ? QStringLiteral("Удалить установленное обновление игры?")
                         : tr("Remove Installed Game Update?");
        case QtCommon::Game::InstalledEntryType::AddOnContent:
            return is_ru ? QStringLiteral("Удалить установленные DLC игры?")
                         : tr("Remove Installed Game DLC?");
        default:
            return is_ru ? QStringLiteral("Удалить установленный элемент?")
                         : tr("Remove Installed Game Entry?");
        }
    }();

    if (!question(this, is_ru ? QStringLiteral("Удаление") : tr("Removing"), entry_question,
                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No)) {
        return;
    }

    // TODO(crueter): move this to QtCommon (populate async?)
    switch (type) {
    case QtCommon::Game::InstalledEntryType::Game:
        QtCommon::Game::RemoveBaseContent(program_id, type);
        [[fallthrough]];
    case QtCommon::Game::InstalledEntryType::Update:
        QtCommon::Game::RemoveUpdateContent(program_id, type);
        if (type != QtCommon::Game::InstalledEntryType::Game) {
            break;
        }
        [[fallthrough]];
    case QtCommon::Game::InstalledEntryType::AddOnContent:
        QtCommon::Game::RemoveAddOnContent(program_id, type);
        break;
    }
    Common::FS::RemoveDirRecursively(Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) /
                                     "game_list");
    game_list->PopulateAsync(UISettings::values.game_dirs);
}

void MainWindow::OnGameListRemoveFile(u64 program_id, QtCommon::Game::GameListRemoveTarget target,
                                      const std::string& game_path) {
    const std::string& lang = UISettings::values.language.GetValue();
    const bool is_ru = lang.rfind("ru", 0) == 0;
    const QString question = [target, is_ru] {
        switch (target) {
        case QtCommon::Game::GameListRemoveTarget::GlShaderCache:
            return is_ru ? QStringLiteral("Удалить переносимый кэш шейдеров OpenGL?")
                         : tr("Delete OpenGL Transferable Shader Cache?");
        case QtCommon::Game::GameListRemoveTarget::VkShaderCache:
            return is_ru ? QStringLiteral("Удалить переносимый кэш шейдеров Vulkan?")
                         : tr("Delete Vulkan Transferable Shader Cache?");
        case QtCommon::Game::GameListRemoveTarget::AllShaderCache:
            return is_ru ? QStringLiteral("Удалить все кэши шейдеров?")
                         : tr("Delete All Transferable Shader Caches?");
        case QtCommon::Game::GameListRemoveTarget::CustomConfiguration:
            return is_ru ? QStringLiteral("Удалить пользовательскую конфигурацию игры?")
                         : tr("Delete Custom Game Configuration?");
        case QtCommon::Game::GameListRemoveTarget::CacheStorage:
            return is_ru ? QStringLiteral("Удалить кэш хранилища?")
                         : tr("Delete Cache Storage?");
        default:
            return QString{};
        }
    }();

    if (!MainWindow::question(this, is_ru ? QStringLiteral("Удаление файла") : tr("File Removing"),
                              question, QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No)) {
        return;
    }

    switch (target) {
    case QtCommon::Game::GameListRemoveTarget::VkShaderCache:
        QtCommon::Game::RemoveVulkanDriverPipelineCache(program_id);
        [[fallthrough]];
    case QtCommon::Game::GameListRemoveTarget::GlShaderCache:
        QtCommon::Game::RemoveTransferableShaderCache(program_id, target);
        break;
    case QtCommon::Game::GameListRemoveTarget::AllShaderCache:
        QtCommon::Game::RemoveAllTransferableShaderCaches(program_id);
        break;
    case QtCommon::Game::GameListRemoveTarget::CustomConfiguration:
        QtCommon::Game::RemoveCustomConfiguration(program_id, game_path);
        break;
    case QtCommon::Game::GameListRemoveTarget::CacheStorage:
        QtCommon::Game::RemoveCacheStorage(program_id);
        break;
    }
}

void MainWindow::OnGameListSetPlayTime(u64 program_id) {
    const u64 current_play_time = play_time_manager->GetPlayTime(program_id);

    SetPlayTimeDialog dialog(this, current_play_time);

    if (dialog.exec() == QDialog::Accepted) {
        const u64 total_seconds = dialog.GetTotalSeconds();
        play_time_manager->SetPlayTime(program_id, total_seconds);
        game_list->PopulateAsync(UISettings::values.game_dirs);
    }
}

void MainWindow::OnGameListRemovePlayTimeData(u64 program_id) {
    if (QMessageBox::question(this, tr("Remove Play Time Data"), tr("Reset play time?"),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    play_time_manager->ResetProgramPlayTime(program_id);
    game_list->PopulateAsync(UISettings::values.game_dirs);
}

void MainWindow::OnGameListDumpRomFS(u64 program_id, const std::string& game_path,
                                     DumpRomFSTarget target) {
    const auto failed = [this] {
        QMessageBox::warning(this, tr("RomFS Extraction Failed!"),
                             tr("There was an error copying the RomFS files or the user "
                                "cancelled the operation."));
    };

    const auto loader = Loader::GetLoader(
        *QtCommon::system, QtCommon::vfs->OpenFile(game_path, FileSys::OpenMode::Read));
    if (loader == nullptr) {
        failed();
        return;
    }

    FileSys::VirtualFile packed_update_raw{};
    loader->ReadUpdateRaw(packed_update_raw);

    const auto& installed = QtCommon::system->GetContentProvider();

    u64 title_id{};
    u8 raw_type{};
    if (!SelectRomFSDumpTarget(installed, program_id, &title_id, &raw_type)) {
        failed();
        return;
    }

    const auto type = static_cast<FileSys::ContentRecordType>(raw_type);
    const auto base_nca = installed.GetEntry(title_id, type);
    if (!base_nca) {
        failed();
        return;
    }

    const FileSys::NCA update_nca{packed_update_raw, nullptr};
    if (type != FileSys::ContentRecordType::Program ||
        update_nca.GetStatus() != Loader::ResultStatus::ErrorMissingBKTRBaseRomFS ||
        update_nca.GetTitleId() != FileSys::GetUpdateTitleID(title_id)) {
        packed_update_raw = {};
    }

    const auto base_romfs = base_nca->GetRomFS();
    const auto dump_dir =
        target == DumpRomFSTarget::Normal
            ? Common::FS::GetEdenPath(Common::FS::EdenPath::DumpDir)
            : Common::FS::GetEdenPath(Common::FS::EdenPath::SDMCDir) / "atmosphere" / "contents";
    const auto romfs_dir = fmt::format("{:016X}/romfs", title_id);

    const auto path = Common::FS::PathToUTF8String(dump_dir / romfs_dir);

    const FileSys::PatchManager pm{title_id, QtCommon::system->GetFileSystemController(),
                                   installed};
    auto romfs = pm.PatchRomFS(base_nca.get(), base_romfs, type, packed_update_raw, false);

    const auto out = VfsFilesystemCreateDirectoryWrapper(path, FileSys::OpenMode::ReadWrite);

    if (out == nullptr) {
        failed();
        QtCommon::vfs->DeleteDirectory(path);
        return;
    }

    bool ok = false;
    const QStringList selections{tr("Full"), tr("Skeleton")};
    const auto res = QInputDialog::getItem(
        this, tr("Select RomFS Dump Mode"),
        tr("Please select the how you would like the RomFS dumped.<br>Full will copy all of the "
           "files into the new directory while <br>skeleton will only create the directory "
           "structure."),
        selections, 0, false, &ok);
    if (!ok) {
        failed();
        QtCommon::vfs->DeleteDirectory(path);
        return;
    }

    const auto extracted = FileSys::ExtractRomFS(romfs);
    if (extracted == nullptr) {
        failed();
        return;
    }

    const auto full = res == selections.constFirst();

    // The expected required space is the size of the RomFS + 1 GiB
    const auto minimum_free_space = romfs->GetSize() + 0x40000000;

    if (full && Common::FS::GetFreeSpaceSize(path) < minimum_free_space) {
        QMessageBox::warning(this, tr("RomFS Extraction Failed!"),
                             tr("There is not enough free space at %1 to extract the RomFS. Please "
                                "free up space or select a different dump directory at "
                                "Emulation > Configure > System > Filesystem > Dump Root")
                                 .arg(QString::fromStdString(path)));
        return;
    }

    QProgressDialog progress(tr("Extracting RomFS..."), tr("Cancel"), 0, 100, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(100);
    progress.setAutoClose(false);
    progress.setAutoReset(false);

    size_t read_size = 0;

    if (RomFSRawCopy(romfs->GetSize(), read_size, progress, extracted, out, full)) {
        progress.close();
        QMessageBox::information(this, tr("RomFS Extraction Succeeded!"),
                                 tr("The operation completed successfully."));
        QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(path)));
    } else {
        progress.close();
        failed();
        QtCommon::vfs->DeleteDirectory(path);
    }
}

// END
void MainWindow::OnGameListVerifyIntegrity(const std::string& game_path) {
    QtCommon::Content::VerifyGameContents(game_path);
}

void MainWindow::OnGameListCopyTID(u64 program_id) {
    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setText(QString::fromStdString(fmt::format("{:016X}", program_id)));
}

void MainWindow::OnGameListCreateShortcut(u64 program_id, const std::string& game_path,
                                          const QtCommon::Game::ShortcutTarget target) {
    // Create shortcu
    std::string arguments = fmt::format("-g \"{:s}\"", game_path);

    QtCommon::Game::CreateShortcut(game_path, program_id, "", target, arguments, true);
}

void MainWindow::OnGameListOpenDirectory(const QString& directory) {
    // TODO(crueter): QtCommon
    std::filesystem::path fs_path;
    if (directory == QStringLiteral("SDMC")) {
        fs_path =
            Common::FS::GetEdenPath(Common::FS::EdenPath::SDMCDir) / "Nintendo/Contents/registered";
    } else if (directory == QStringLiteral("UserNAND")) {
        fs_path =
            Common::FS::GetEdenPath(Common::FS::EdenPath::NANDDir) / "user/Contents/registered";
    } else if (directory == QStringLiteral("SysNAND")) {
        fs_path =
            Common::FS::GetEdenPath(Common::FS::EdenPath::NANDDir) / "system/Contents/registered";
    } else {
        fs_path = directory.toStdString();
    }

    const auto qt_path = QString::fromStdString(Common::FS::PathToUTF8String(fs_path));

    if (!Common::FS::IsDir(fs_path)) {
        QMessageBox::critical(this, tr("Error Opening %1").arg(qt_path),
                              tr("Folder does not exist!"));
        return;
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(qt_path));
}

void MainWindow::OnGameListAddDirectory() {
    const QString dir_path = QFileDialog::getExistingDirectory(this, tr("Select Directory"));
    if (dir_path.isEmpty()) {
        return;
    }

    const QString clean_path = QDir::cleanPath(dir_path);
    const std::string path_str = clean_path.toStdString();

    const auto it = std::find_if(UISettings::values.game_dirs.begin(),
                                 UISettings::values.game_dirs.end(),
                                 [&](const UISettings::GameDir& d) {
                                     return QDir::cleanPath(QString::fromStdString(d.path)).compare(clean_path, Qt::CaseInsensitive) == 0;
                                 });
    if (it == UISettings::values.game_dirs.end()) {
        UISettings::GameDir game_dir{path_str, true, true};
        UISettings::values.game_dirs.append(game_dir);
    } else {
        it->expanded = true;
        it->deep_scan = true;
    }
    game_list->PopulateAsync(UISettings::values.game_dirs);
    OnSaveConfig();
}

void MainWindow::OnGameListShowList(bool show) {
    if (emulation_running && ui->action_Single_Window_Mode->isChecked())
        return;
    game_list->setVisible(show);
    game_list_placeholder->setVisible(!show);
};

void MainWindow::OnGameListOpenPerGameProperties(const std::string& file) {
    u64 title_id{};
    const auto v_file = Core::GetGameFileFromPath(QtCommon::vfs, file);
    const auto loader = Loader::GetLoader(*QtCommon::system, v_file);

    if (loader == nullptr || loader->ReadProgramId(title_id) != Loader::ResultStatus::Success) {
        QMessageBox::information(this, tr("Properties"),
                                 tr("The game properties could not be loaded."));
        return;
    }

    OpenPerGameConfiguration(title_id, file);
}

void MainWindow::OnLinkToRyujinx(const u64& program_id) {
    namespace fs = std::filesystem;

    fs::path ryu_dir;

    // find an existing Ryujinx linked path in config.ini; if it exists, use it as a "hint"
    // If it's not defined in config.ini, use default
    const fs::path existing_path =
        UISettings::values.ryujinx_link_paths
            .value(program_id, QDir(Common::FS::GetLegacyPath(Common::FS::RyujinxDir)))
            .filesystemAbsolutePath();

    // this function also prompts the user to manually specify a portable location
    ryu_dir = QtCommon::FS::GetRyujinxSavePath(existing_path, program_id);

    if (ryu_dir.empty())
        return;

    const std::string user_id = GetProfileIDString();
    if (user_id.empty())
        return;

    const std::string hex_program = fmt::format("{:016X}", program_id);

    const fs::path eden_dir = FrontendCommon::DataManager::GetDataDir(
                                  FrontendCommon::DataManager::DataDir::Saves, user_id) /
                              hex_program;

    // CheckUnlink basically just checks to see if one or both are linked, and prompts the user to
    // unlink if this is the case.
    // If it returns false, neither dir is linked so it's fine to continue
    if (!QtCommon::FS::CheckUnlink(eden_dir, ryu_dir)) {
        RyujinxDialog dialog(eden_dir, ryu_dir, this);
        if (dialog.exec() == QDialog::Accepted) {
            UISettings::values.ryujinx_link_paths.insert(
                program_id,
                QString::fromStdString(Common::FS::GetRyuPathFromSavePath(ryu_dir).string()));
        }
    } else {
        UISettings::values.ryujinx_link_paths.remove(program_id);
    }

    config->SaveAllValues();
}

void MainWindow::OnMenuLoadFile() {
    if (is_load_file_select_active) {
        return;
    }

    is_load_file_select_active = true;
    const QString extensions =
        QStringLiteral("*.")
            .append(QtCommon::supported_file_extensions.join(QStringLiteral(" *.")))
            .append(QStringLiteral(" main"));
    const QString file_filter = tr("Switch Executable (%1);;All Files (*.*)",
                                   "%1 is an identifier for the Switch executable file extensions.")
                                    .arg(extensions);
    const QString filename = QFileDialog::getOpenFileName(
        this, tr("Load File"), QString::fromStdString(UISettings::values.roms_path), file_filter);
    is_load_file_select_active = false;

    if (filename.isEmpty()) {
        return;
    }

    UISettings::values.roms_path = QFileInfo(filename).path().toStdString();
    BootGame(filename, ApplicationAppletParameters());
}

void MainWindow::OnMenuLoadFolder() {
    const QString dir_path =
        QFileDialog::getExistingDirectory(this, tr("Open Extracted ROM Directory"));

    if (dir_path.isNull()) {
        return;
    }

    const QDir dir{dir_path};
    const QStringList matching_main = dir.entryList({QStringLiteral("main")}, QDir::Files);
    if (matching_main.size() == 1) {
        BootGame(dir.path() + QDir::separator() + matching_main[0], ApplicationAppletParameters());
    } else {
        QMessageBox::warning(this, tr("Invalid Directory Selected"),
                             tr("The directory you have selected does not contain a 'main' file."));
    }
}

void MainWindow::IncrementInstallProgress() {
    install_progress->setValue(install_progress->value() + 1);
}

void MainWindow::OnMenuInstallToNAND() {
    const QString file_filter =
        tr("Installable Switch File (*.nca *.nsp *.nsz *.xci *.xcz);;Nintendo Content Archive "
           "(*.nca);;Nintendo Submission Package (*.nsp *.nsz);;NX Cartridge "
           "Image (*.xci *.xcz)");

    QStringList filenames = QFileDialog::getOpenFileNames(
        this, tr("Install Files"), QString::fromStdString(UISettings::values.roms_path),
        file_filter);

    if (filenames.isEmpty()) {
        return;
    }

    InstallDialog installDialog(this, filenames);
    if (installDialog.exec() == QDialog::Rejected) {
        return;
    }

    const QStringList files = installDialog.GetFiles();

    if (files.isEmpty()) {
        return;
    }

    // Save folder location of the first selected file
    UISettings::values.roms_path = QFileInfo(filenames[0]).path().toStdString();

    int remaining = filenames.size();

    // This would only overflow above 2^51 bytes (2.252 PB)
    int total_size = 0;
    for (const QString& file : files) {
        total_size += static_cast<int>(QFile(file).size() / CopyBufferSize);
    }
    if (total_size < 0) {
        LOG_CRITICAL(Frontend, "Attempting to install too many files, aborting.");
        return;
    }

    QStringList new_files{};         // Newly installed files that do not yet exist in the NAND
    QStringList overwritten_files{}; // Files that overwrote those existing in the NAND
    QStringList failed_files{};      // Files that failed to install due to errors
    bool detected_base_install{};    // Whether a base game was attempted to be installed

    ui->action_Install_File_NAND->setEnabled(false);

    install_progress = new QProgressDialog(QString{}, tr("Cancel"), 0, total_size, this);
    install_progress->setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    install_progress->setAttribute(Qt::WA_DeleteOnClose, true);
    install_progress->setFixedWidth(installDialog.GetMinimumWidth() + 40);
    install_progress->show();

    for (const QString& file : files) {
        install_progress->setWindowTitle(tr("%n file(s) remaining", "", remaining));
        install_progress->setLabelText(
            tr("Installing file \"%1\"...").arg(QFileInfo(file).fileName()));

        QFuture<ContentManager::InstallResult> future;
        ContentManager::InstallResult result;

        if (file.endsWith(QStringLiteral("nsp"), Qt::CaseInsensitive) ||
            file.endsWith(QStringLiteral("nsz"), Qt::CaseInsensitive) ||
            file.endsWith(QStringLiteral("xci"), Qt::CaseInsensitive) ||
            file.endsWith(QStringLiteral("xcz"), Qt::CaseInsensitive)) {
            const auto progress_callback = [this](size_t size, size_t progress) {
                emit UpdateInstallProgress();
                if (install_progress->wasCanceled()) {
                    return true;
                }
                return false;
            };
            future = QtConcurrent::run([&file, progress_callback] {
                return ContentManager::InstallNSP(*QtCommon::system, *QtCommon::vfs,
                                                  file.toStdString(), progress_callback);
            });

            while (!future.isFinished()) {
                QCoreApplication::processEvents();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            result = future.result();

        } else {
            result = InstallNCA(file);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        switch (result) {
        case ContentManager::InstallResult::Success:
            new_files.append(QFileInfo(file).fileName());
            break;
        case ContentManager::InstallResult::Overwrite:
            overwritten_files.append(QFileInfo(file).fileName());
            break;
        case ContentManager::InstallResult::Failure:
            failed_files.append(QFileInfo(file).fileName());
            break;
        case ContentManager::InstallResult::BaseInstallAttempted:
            failed_files.append(QFileInfo(file).fileName());
            detected_base_install = true;
            break;
        }

        --remaining;
    }

    install_progress->close();

    if (detected_base_install) {
        QMessageBox::warning(
            this, tr("Install Results"),
            tr("To avoid possible conflicts, we discourage users from installing base games to the "
               "NAND.\nPlease, only use this feature to install updates and DLC."));
    }

    const QString install_results =
        (new_files.isEmpty() ? QString{}
                             : tr("%n file(s) were newly installed\n", "", new_files.size())) +
        (overwritten_files.isEmpty()
             ? QString{}
             : tr("%n file(s) were overwritten\n", "", overwritten_files.size())) +
        (failed_files.isEmpty() ? QString{}
                                : tr("%n file(s) failed to install\n", "", failed_files.size()));

    QMessageBox::information(this, tr("Install Results"), install_results);
    Common::FS::RemoveDirRecursively(Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) /
                                     "game_list");
    game_list->PopulateAsync(UISettings::values.game_dirs);
    ui->action_Install_File_NAND->setEnabled(true);
}

ContentManager::InstallResult MainWindow::InstallNCA(const QString& filename) {
    const QStringList tt_options{tr("System Application"),
                                 tr("System Archive"),
                                 tr("System Application Update"),
                                 tr("Firmware Package (Type A)"),
                                 tr("Firmware Package (Type B)"),
                                 tr("Game"),
                                 tr("Game Update"),
                                 tr("Game DLC"),
                                 tr("Delta Title")};
    bool ok;
    const auto item = QInputDialog::getItem(
        this, tr("Select NCA Install Type..."),
        tr("Please select the type of title you would like to install this NCA as:\n(In "
           "most instances, the default 'Game' is fine.)"),
        tt_options, 5, false, &ok);

    auto index = tt_options.indexOf(item);
    if (!ok || index == -1) {
        QMessageBox::warning(this, tr("Failed to Install"),
                             tr("The title type you selected for the NCA is invalid."));
        return ContentManager::InstallResult::Failure;
    }

    // If index is equal to or past Game, add the jump in TitleType.
    if (index >= 5) {
        index += static_cast<size_t>(FileSys::TitleType::Application) -
                 static_cast<size_t>(FileSys::TitleType::FirmwarePackageB);
    }

    const bool is_application = index >= static_cast<s32>(FileSys::TitleType::Application);
    const auto& fs_controller = QtCommon::system->GetFileSystemController();
    auto* registered_cache = is_application ? fs_controller.GetUserNANDContents()
                                            : fs_controller.GetSystemNANDContents();

    const auto progress_callback = [this](size_t size, size_t progress) {
        emit UpdateInstallProgress();
        if (install_progress->wasCanceled()) {
            return true;
        }
        return false;
    };
    return ContentManager::InstallNCA(*QtCommon::vfs, filename.toStdString(), *registered_cache,
                                      static_cast<FileSys::TitleType>(index), progress_callback);
}

void MainWindow::OnMenuRecentFile() {
    QAction* action = qobject_cast<QAction*>(sender());
    assert(action);

    const QString filename = action->data().toString();
    if (QFileInfo::exists(filename)) {
        BootGame(filename, ApplicationAppletParameters());
    } else {
        // Display an error message and remove the file from the list.
        QMessageBox::information(this, tr("File not found"),
                                 tr("File \"%1\" not found").arg(filename));

        UISettings::values.recent_files.removeOne(filename);
        UpdateRecentFiles();
    }
}

void MainWindow::OnStartGame() {
    PreventOSSleep();

    QtCommon::emu_thread->SetRunning(true);

    UpdateMenuState();
    OnTasStateChanged();

    const u64 started_prog_id = QtCommon::system->GetApplicationProcessProgramID();
    play_time_manager->SetProgramId(started_prog_id);
    play_time_manager->Start();
    if (started_prog_id != 0) {
        StormSaveSyncDialog::CreateAutoBackupForTitle(started_prog_id);
    }

    discord_rpc->Update();
    Common::FeralGamemode::Start();

    const bool enable_floating = UISettings::values.enable_floating_translate_button.GetValue();
    if (enable_floating) {
        QWidget* target_parent = render_window ? static_cast<QWidget*>(render_window) : static_cast<QWidget*>(this);
        if (!floating_translate_button) {
            floating_translate_button = new FloatingTranslateButton(target_parent);
            connect(floating_translate_button, &FloatingTranslateButton::TranslateRequested, this, &MainWindow::OnTranslateScreen);
            connect(floating_translate_button, &FloatingTranslateButton::OpenSettingsRequested, this, &MainWindow::OnOpenTranslatorSettings);
            connect(floating_translate_button, &FloatingTranslateButton::ToggleHUDRequested, this, [this]() {
                if (m_game_translator) {
                    m_game_translator->ToggleHUD();
                }
            });
        } else if (render_window && floating_translate_button->parent() != render_window) {
            floating_translate_button->setParent(render_window);
        }
        if (render_window) {
            floating_translate_button->move(std::max(10, render_window->width() - 76), std::max(10, render_window->height() - 110));
        }
        floating_translate_button->SetVisibleState(true);
    } else if (floating_translate_button) {
        floating_translate_button->SetVisibleState(false);
    }
}

void MainWindow::OnRestartGame() {
    if (!QtCommon::system->IsPoweredOn()) {
        return;
    }

    if (ConfirmShutdownGame()) {
        // Make a copy since ShutdownGame edits game_path
        const auto current_game = QString(current_game_path);
        ShutdownGame();
        BootGame(current_game, ApplicationAppletParameters());
    }
}

void MainWindow::OnPauseGame() {
    QtCommon::emu_thread->SetRunning(false);
    play_time_manager->Stop();
    UpdateMenuState();
    AllowOSSleep();
    Common::FeralGamemode::Stop();
}

void MainWindow::OnPauseContinueGame() {
    if (emulation_running) {
        if (QtCommon::emu_thread->IsRunning()) {
            OnPauseGame();
        } else {
            OnStartGame();
        }
    }
}

void MainWindow::OnStopGame() {
    if (m_is_stopping_emulation || !emulation_running) {
        return;
    }
    disconnect(render_window, &GRenderWindow::Closed, this, &MainWindow::OnStopGame);
    if (ConfirmShutdownGame()) {
        m_is_stopping_emulation = true;
        play_time_manager->Stop();
        if (OnShutdownBegin()) {
            OnShutdownBeginDialog();
        } else {
            OnEmulationStopped();
        }
    } else {
        connect(render_window, &GRenderWindow::Closed, this, &MainWindow::OnStopGame);
    }
}

bool MainWindow::ConfirmShutdownGame() {
    if (UISettings::values.confirm_before_stopping.GetValue() == ConfirmStop::Ask_Always) {
        if (QtCommon::system->GetExitLocked()) {
            if (!ConfirmForceLockedExit()) {
                return false;
            }
        } else {
            if (!ConfirmChangeGame()) {
                return false;
            }
        }
    } else {
        if (UISettings::values.confirm_before_stopping.GetValue() ==
                ConfirmStop::Ask_Based_On_Game &&
            QtCommon::system->GetExitLocked()) {
            if (!ConfirmForceLockedExit()) {
                return false;
            }
        }
    }
    return true;
}

void MainWindow::OnLoadComplete() {
    loading_screen->OnLoadComplete();

    if (!perf_overlay) {
        perf_overlay = new PerformanceOverlay(this);
        perf_overlay->setVisible(ui->action_Show_Performance_Overlay->isChecked());

        connect(perf_overlay, &PerformanceOverlay::closed, perf_overlay,
                [this]() { ui->action_Show_Performance_Overlay->setChecked(false); });
    }
}

void MainWindow::OnExecuteProgram(std::size_t program_index) {
    ShutdownGame();

    auto params = ApplicationAppletParameters();
    if (program_index >= 0x0100000000000000ULL) {
        params.program_id = program_index;
        params.program_index = 0;
    } else {
        params.program_index = static_cast<s32>(program_index);
        params.program_id = 0;
    }
    params.launch_type = Service::AM::LaunchType::ApplicationInitiated;
    BootGame(last_filename_booted, params);
}

void MainWindow::OnExit() {
    m_is_quitting_from_tray = true;
    if (m_tray_icon) {
        m_tray_icon->hide();
    }
    ShutdownGame();
    close();
}

void MainWindow::OnSaveConfig() {
    QtCommon::system->ApplySettings();
    config->SaveAllValues();
}

void MainWindow::ErrorDisplayDisplayError(QString error_code, QString error_text) {
    error_applet = new OverlayDialog(render_window, *QtCommon::system, error_code, error_text,
                                     QString{}, tr("OK"), Qt::AlignLeft | Qt::AlignVCenter);
    SCOPE_EXIT {
        error_applet->deleteLater();
        error_applet = nullptr;
    };
    error_applet->exec();

    emit ErrorDisplayFinished();
}

void MainWindow::ErrorDisplayRequestExit() {
    if (error_applet) {
        error_applet->reject();
    }
}

void MainWindow::OnMenuReportCompatibility() {
    QtCommon::Frontend::Critical(
        tr("Function Disabled"),
        tr("Compatibility list reporting is currently disabled. Check back later!"));

    // #if defined(ARCHITECTURE_x86_64) && !defined(__APPLE__)
    //     const auto& caps = g_cpu_caps;
    //     const bool has_fma = caps.fma;
    //     const auto processor_count = std::thread::hardware_concurrency();
    //     const bool has_4threads = processor_count == 0 || processor_count >= 4;
    //     const bool has_8gb_ram = Common::GetMemInfo().TotalPhysicalMemory >= 8_GiB;
    //     const bool has_broken_vulkan = UISettings::values.has_broken_vulkan;

    //     if (!has_fma || !has_4threads || !has_8gb_ram || has_broken_vulkan) {
    //         QMessageBox::critical(this, tr("Hardware requirements not met"),
    //                               tr("Your system does not meet the recommended hardware
    //                               requirements. "
    //                                  "Compatibility reporting has been disabled."));
    //         return;
    //     }

    //     if (!Settings::values.eden_token.GetValue().empty() &&
    //         !Settings::values.eden_username.GetValue().empty()) {
    //     } else {
    //         QMessageBox::critical(
    //             this, tr("Missing yuzu Account"),
    //             tr("In order to submit a game compatibility test case, you must set up your web
    //             token "
    //                "and "
    //                "username.<br><br/>To link your eden account, go to Emulation &gt;
    //                Configuration "
    //                "&gt; "
    //                "Web."));
    //     }
    // #else
    //     QMessageBox::critical(this, tr("Hardware requirements not met"),
    //                           tr("Your system does not meet the recommended hardware
    //                           requirements. "
    //                              "Compatibility reporting has been disabled."));
    // #endif
}

void MainWindow::OpenURL(const QUrl& url) {
    const bool open = QDesktopServices::openUrl(url);
    if (!open) {
        QMessageBox::warning(this, tr("Error opening URL"),
                             tr("Unable to open the URL \"%1\".").arg(url.toString()));
    }
}

void MainWindow::OnOpenModsPage() {
    OpenURL(QUrl(QStringLiteral("https://github.com/ReiKatari/STORM_SWITCH")));
}

void MainWindow::OnOpenQuickstartGuide() {
    OpenURL(QUrl(QStringLiteral("https://github.com/ReiKatari/STORM_SWITCH#readme")));
}

void MainWindow::OnOpenFAQ() {
    OpenURL(QUrl(QStringLiteral("https://github.com/ReiKatari/STORM_SWITCH#readme")));
}

void MainWindow::ToggleFullscreen() {
    if (!emulation_running) {
        return;
    }
    if (ui->action_Fullscreen->isChecked()) {
        ShowFullscreen();
    } else {
        HideFullscreen();
    }
}

// We're going to return the screen that the given window has the most pixels on
static QScreen* GuessCurrentScreen(QWidget* window) {
    const QList<QScreen*> screens = QGuiApplication::screens();
    return *std::max_element(
        screens.cbegin(), screens.cend(), [window](const QScreen* left, const QScreen* right) {
            const QSize left_size = left->geometry().intersected(window->geometry()).size();
            const QSize right_size = right->geometry().intersected(window->geometry()).size();
            return (left_size.height() * left_size.width()) <
                   (right_size.height() * right_size.width());
        });
}

bool MainWindow::UsingExclusiveFullscreen() {
    return Settings::values.fullscreen_mode.GetValue() == Settings::FullscreenMode::Exclusive ||
           QGuiApplication::platformName() == QStringLiteral("wayland") ||
           QGuiApplication::platformName() == QStringLiteral("wayland-egl");
}

void MainWindow::ShowFullscreen() {
    const auto show_fullscreen = [this](QWidget* window) {
        if (UsingExclusiveFullscreen()) {
            window->showFullScreen();
            return;
        }
        window->hide();
        window->setWindowFlags(window->windowFlags() | Qt::FramelessWindowHint);
        const auto screen_geometry = GuessCurrentScreen(window)->geometry();
        window->setGeometry(screen_geometry.x(), screen_geometry.y(), screen_geometry.width(),
                            screen_geometry.height() + 1);
        window->raise();
        window->showNormal();
    };

    if (ui->action_Single_Window_Mode->isChecked()) {
        UISettings::values.geometry = saveGeometry();

        ui->menubar->hide();
        statusBar()->hide();

        show_fullscreen(this);
    } else {
        UISettings::values.renderwindow_geometry = render_window->saveGeometry();
        show_fullscreen(render_window);
    }
}

void MainWindow::HideFullscreen() {
    if (ui->action_Single_Window_Mode->isChecked()) {
        if (UsingExclusiveFullscreen()) {
            showNormal();
            restoreGeometry(UISettings::values.geometry);
        } else {
            hide();
            setWindowFlags(windowFlags() & ~Qt::FramelessWindowHint);
            restoreGeometry(UISettings::values.geometry);
            raise();
            show();
        }

        statusBar()->setVisible(ui->action_Show_Status_Bar->isChecked());
        ui->menubar->show();
    } else {
        if (UsingExclusiveFullscreen()) {
            render_window->showNormal();
            render_window->restoreGeometry(UISettings::values.renderwindow_geometry);
        } else {
            render_window->hide();
            render_window->setWindowFlags(windowFlags() & ~Qt::FramelessWindowHint);
            render_window->restoreGeometry(UISettings::values.renderwindow_geometry);
            render_window->raise();
            render_window->show();
        }
    }
}

void MainWindow::ToggleWindowMode() {
    if (ui->action_Single_Window_Mode->isChecked()) {
        // Render in the main window...
        render_window->BackupGeometry();
        ui->horizontalLayout->addWidget(render_window);
        render_window->setFocusPolicy(Qt::StrongFocus);
        if (emulation_running) {
            render_window->setVisible(true);
            render_window->setFocus();
            game_list->hide();
        }

    } else {
        // Render in a separate window...
        ui->horizontalLayout->removeWidget(render_window);
        render_window->setParent(nullptr);
        render_window->setFocusPolicy(Qt::StrongFocus);
        if (emulation_running) {
            render_window->setVisible(true);
            render_window->RestoreGeometry();
            render_window->setFocus();
            game_list->show();
        }
    }
}

void MainWindow::ResetWindowSize(u32 width, u32 height) {
    const auto aspect_ratio = Layout::EmulationAspectRatio(Settings::values.aspect_ratio.GetValue(),
                                                           float(height) / width);
    if (!ui->action_Single_Window_Mode->isChecked()) {
        render_window->resize(height / aspect_ratio, height);
    } else {
        const bool show_status_bar = ui->action_Show_Status_Bar->isChecked();
        const auto status_bar_height = show_status_bar ? statusBar()->height() : 0;
        resize(height / aspect_ratio, height + menuBar()->height() + status_bar_height);
    }
}

void MainWindow::ResetWindowSize720() {
    ResetWindowSize(Layout::ScreenUndocked::Width, Layout::ScreenUndocked::Height);
}

void MainWindow::ResetWindowSize900() {
    ResetWindowSize(1600U, 900U);
}

void MainWindow::ResetWindowSize1080() {
    ResetWindowSize(Layout::ScreenDocked::Width, Layout::ScreenDocked::Height);
}

void MainWindow::SetGameListMode(Settings::GameListMode mode) {
    ui->action_Grid_View->setChecked(mode == Settings::GameListMode::GridView);
    ui->action_Tree_View->setChecked(mode == Settings::GameListMode::TreeView);
    ui->action_Carousel_View->setChecked(mode == Settings::GameListMode::CarouselView);

    UISettings::values.game_list_mode = mode;
    ui->action_Show_Game_Name->setEnabled(mode != Settings::GameListMode::TreeView);

    CheckIconSize();
    game_list->ResetViewMode();
}

void MainWindow::SetGridView() {
    SetGameListMode(Settings::GameListMode::GridView);
}

void MainWindow::SetTreeView() {
    SetGameListMode(Settings::GameListMode::TreeView);
}

void MainWindow::SetCarouselView() {
    SetGameListMode(Settings::GameListMode::CarouselView);
}

void MainWindow::CheckIconSize() {
    // When in grid/carousel view mode, with text off
    // there is no point in having icons turned off
    auto actions = game_size_actions->actions();
    if (UISettings::values.game_list_mode.GetValue() != Settings::GameListMode::TreeView &&
        !UISettings::values.show_game_name.GetValue()) {
        u32 newSize = UISettings::values.game_icon_size.GetValue();
        if (newSize == 0) {
            newSize = 64;
            UISettings::values.game_icon_size.SetValue(newSize);
        }

        // Then disable the "none" action and update that menu.
        for (size_t i = 0; i < default_game_icon_sizes.size(); i++) {
            const auto current_size = newSize;
            const auto size = default_game_icon_sizes[i].first;
            if (current_size == size)
                actions.at(i)->setChecked(true);
        }

        // Update this if you add anything before None.
        actions.at(0)->setEnabled(false);
    } else {
        actions.at(0)->setEnabled(true);
    }
}

void MainWindow::ToggleShowGameName() {
    auto& setting = UISettings::values.show_game_name;
    const bool newValue = !setting.GetValue();
    ui->action_Show_Game_Name->setChecked(newValue);
    setting.SetValue(newValue);

    CheckIconSize();

    game_list->UpdateIconSizes();
    game_list->RefreshGameDirectory();
}

void MainWindow::OnConfigure() {
    const auto old_theme = UISettings::values.theme;
    const bool old_discord_presence = UISettings::values.enable_discord_presence.GetValue();
    const auto old_language_index = Settings::values.language_index.GetValue();
    const bool old_gamemode = UISettings::values.enable_gamemode.GetValue();
#ifdef __unix__
    const bool old_force_x11 = UISettings::values.gui_force_x11.GetValue();
#endif

    Settings::SetConfiguringGlobal(true);
    if (!emulation_running) {
        Settings::RestoreGlobalState(false);
    }
    ConfigureDialog configure_dialog(this, hotkey_registry, input_subsystem.get(),
                                     vk_device_records, *QtCommon::system,
                                     !multiplayer_state->IsHostingPublicRoom());
    connect(&configure_dialog, &ConfigureDialog::LanguageChanged, this,
            &MainWindow::OnLanguageChanged);
    connect(&configure_dialog, &ConfigureDialog::ThemeChanged, this,
            [this](const QString&) { UpdateUITheme(); });
    connect(&configure_dialog, &ConfigureDialog::ExternalContentDirsChanged, this,
            &MainWindow::OnGameListRefresh);
    connect(&configure_dialog, &ConfigureDialog::ConfigurationApplied, this,
            [this]() {
                UpdateStatusButtons();
                if (m_session_backup.is_active) {
                    m_session_backup.Capture();
                }
                if (config) {
                    config->SaveAllValues();
                }
            });

    const auto result = configure_dialog.exec();
    if (result != QDialog::Accepted && !UISettings::values.configuration_applied &&
        !UISettings::values.reset_to_defaults) {
        if (UISettings::values.theme != old_theme) {
            UISettings::values.theme = old_theme;
            UpdateUITheme();
        }
        return;
    } else if (result == QDialog::Accepted) {
        // Only apply new changes if user hit Okay
        // This is here to avoid applying changes if the user hit Apply, made some changes, then hit
        // Cancel
        configure_dialog.ApplyConfiguration();
        if (m_session_backup.is_active) {
            m_session_backup.Capture();
        }
        config->SaveAllValues();
        UpdateStatusButtons();
    } else if (UISettings::values.reset_to_defaults) {
        LOG_INFO(Frontend, "Resetting all settings to defaults");
        if (!Common::FS::RemoveFile(config->GetConfigFilePath())) {
            LOG_WARNING(Frontend, "Failed to remove configuration file");
        }
        if (!Common::FS::RemoveDirContentsRecursively(
                Common::FS::GetEdenPath(Common::FS::EdenPath::ConfigDir) / "custom")) {
            LOG_WARNING(Frontend, "Failed to remove custom configuration files");
        }
        if (!Common::FS::RemoveDirRecursively(
                Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) / "game_list")) {
            LOG_WARNING(Frontend, "Failed to remove game metadata cache files");
        }

        // Explicitly save the game directories, since reinitializing config does not explicitly do
        // so.
        QVector<UISettings::GameDir> old_game_dirs = std::move(UISettings::values.game_dirs);
        QVector<u64> old_favorited_ids = std::move(UISettings::values.favorited_ids);

        Settings::values.disabled_addons.clear();

        config = std::make_unique<QtConfig>();
        UISettings::values.reset_to_defaults = false;

        UISettings::values.game_dirs = std::move(old_game_dirs);
        UISettings::values.favorited_ids = std::move(old_favorited_ids);

        InitializeRecentFileMenuActions();

        SetDefaultUIGeometry();
        RestoreUIState();
    }
    InitializeHotkeys();

    if (UISettings::values.theme != old_theme) {
        UpdateUITheme();
    }
    if (UISettings::values.enable_discord_presence.GetValue() != old_discord_presence) {
        SetDiscordEnabled(UISettings::values.enable_discord_presence.GetValue());
    }
    if (UISettings::values.enable_gamemode.GetValue() != old_gamemode) {
    SetGamemodeEnabled(UISettings::values.enable_gamemode.GetValue());
    }
#ifdef __unix__
    if (UISettings::values.gui_force_x11.GetValue() != old_force_x11) {
        GraphicsBackend::SetForceX11(UISettings::values.gui_force_x11.GetValue());
    }
#endif

    if (!multiplayer_state->IsHostingPublicRoom()) {
        multiplayer_state->UpdateCredentials();
    }

    emit UpdateThemedIcons();

    const auto reload = UISettings::values.is_game_list_reload_pending.exchange(false);
    if (reload || Settings::values.language_index.GetValue() != old_language_index) {
        game_list->PopulateAsync(UISettings::values.game_dirs);
    }

    UISettings::values.configuration_applied = false;

    config->SaveAllValues();
    UpdateStatusButtons();

    if ((UISettings::values.hide_mouse || Settings::values.mouse_panning) && emulation_running) {
        render_window->installEventFilter(render_window);
        render_window->setAttribute(Qt::WA_Hover, true);
    } else {
        render_window->removeEventFilter(render_window);
        render_window->setAttribute(Qt::WA_Hover, false);
    }

    if (UISettings::values.hide_mouse) {
        mouse_hide_timer.start();
    }

    // Restart camera config
    if (emulation_running) {
        render_window->FinalizeCamera();
    }

    if (!UISettings::values.has_broken_vulkan) {
        renderer_status_button->setEnabled(!emulation_running);
    }

    if (floating_translate_button) {
        floating_translate_button->SetVisibleState(UISettings::values.enable_floating_translate_button.GetValue() && emulation_running);
    }

    UpdateStatusButtons();
    controller_dialog->refreshConfiguration();
    QtCommon::system->ApplySettings();
}

void MainWindow::OnConfigureTas() {
    ConfigureTasDialog dialog(this);
    const auto result = dialog.exec();

    if (result != QDialog::Accepted && !UISettings::values.configuration_applied) {
        Settings::RestoreGlobalState(QtCommon::system->IsPoweredOn());
        return;
    } else if (result == QDialog::Accepted) {
        dialog.ApplyConfiguration();
        OnSaveConfig();
    }
}

void MainWindow::OnTasStartStop() {
    if (!emulation_running) {
        return;
    }

    // Disable system buttons to prevent TAS from executing a hotkey
    auto* controller =
        QtCommon::system->HIDCore().GetEmulatedController(Core::HID::NpadIdType::Player1);
    controller->ResetSystemButtons();

    input_subsystem->GetTas()->StartStop();
    OnTasStateChanged();
}

void MainWindow::OnTasRecord() {
    if (!emulation_running) {
        return;
    }
    if (is_tas_recording_dialog_active) {
        return;
    }

    // Disable system buttons to prevent TAS from recording a hotkey
    auto* controller =
        QtCommon::system->HIDCore().GetEmulatedController(Core::HID::NpadIdType::Player1);
    controller->ResetSystemButtons();

    const bool is_recording = input_subsystem->GetTas()->Record();
    if (!is_recording) {
        if (Settings::values.tas_show_recording_dialog.GetValue()) {
            is_tas_recording_dialog_active = true;

            bool answer = question(this, tr("TAS Recording"), tr("Overwrite file of player 1?"),
                                   QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

            input_subsystem->GetTas()->SaveRecording(answer);
            is_tas_recording_dialog_active = false;
        } else {
            input_subsystem->GetTas()->SaveRecording(true);
        }
    }
    OnTasStateChanged();
}

void MainWindow::OnTasReset() {
    input_subsystem->GetTas()->Reset();
}

void MainWindow::OnToggleDockedMode() {
    const bool is_docked = Settings::IsDockedMode();
    auto* player_1 =
        QtCommon::system->HIDCore().GetEmulatedController(Core::HID::NpadIdType::Player1);
    auto* handheld =
        QtCommon::system->HIDCore().GetEmulatedController(Core::HID::NpadIdType::Handheld);

    if (!is_docked && handheld->IsConnected()) {
        QMessageBox::warning(this, tr("Invalid config detected"),
                             tr("Handheld controller can't be used on docked mode. Pro "
                                "controller will be selected."));
        handheld->Disconnect();
        player_1->SetNpadStyleIndex(Core::HID::NpadStyleIndex::Fullkey);
        player_1->Connect();
        controller_dialog->refreshConfiguration();
    }

    const auto new_mode = is_docked ? Settings::ConsoleMode::Handheld
                                    : Settings::ConsoleMode::Docked;
    ApplyStatusBarSwitchableSetting(Settings::values.use_docked_mode, new_mode,
                                    m_session_backup, m_session_backup.use_docked_mode,
                                    current_game_path);
    ApplyDynamicSettingChange();
    UpdateDockedButton();
    UpdateResScaleText();
    OnDockedModeChanged(is_docked, !is_docked, *QtCommon::system);
}

void MainWindow::OnToggleGpuAccuracy() {
    auto new_acc = Settings::values.gpu_accuracy.GetValue();
    switch (new_acc) {
    case Settings::GpuAccuracy::Low:
        new_acc = Settings::GpuAccuracy::High;
        break;
    case Settings::GpuAccuracy::High:
        new_acc = Settings::GpuAccuracy::Low;
        break;
    }

    ApplyStatusBarSwitchableSetting(Settings::values.gpu_accuracy, new_acc,
                                    m_session_backup, m_session_backup.gpu_accuracy,
                                    current_game_path);
    ApplyDynamicSettingChange();
    UpdateGPUAccuracyButton();
}

void MainWindow::OnHealGraphicsArtifacts() {
    ApplyStatusBarSwitchableSetting(Settings::values.gpu_accuracy, Settings::GpuAccuracy::High,
                                    m_session_backup, m_session_backup.gpu_accuracy,
                                    current_game_path);
    ApplyStatusBarSwitchableSetting(Settings::values.dma_accuracy, Settings::DmaAccuracy::Safe,
                                    m_session_backup, m_session_backup.dma_accuracy,
                                    current_game_path);
    ApplyStatusBarSwitchableSetting(Settings::values.gpu_fence_behavior,
                                    Settings::GpuFenceBehavior::Accurate,
                                    m_session_backup, m_session_backup.gpu_fence_behavior,
                                    current_game_path);
    Settings::values.use_reactive_flushing.SetValue(true);
    Settings::values.sync_memory_operations.SetValue(true);
    ApplyDynamicSettingChange();
    UpdateStatusButtons();
    statusBar()->showMessage(StormLang(
        QStringLiteral("Автолечение графики применено: GPU High + Safe DMA + Accurate Barriers + Reactive Flushing"),
        QStringLiteral("Graphics Auto-Heal applied: GPU High + Safe DMA + Accurate Barriers + Reactive Flushing"),
        QStringLiteral("Grafik-Auto-Heilung angewendet: GPU High + Safe DMA + Accurate Barriers + Reactive Flushing"),
        QStringLiteral("Auto-réparation graphique appliquée : GPU High + Safe DMA + Accurate Barriers + Reactive Flushing"),
        QStringLiteral("图形自动修复已应用：GPU 高精度 + 安全 DMA + 精确屏障 + 响应式刷新"),
        QStringLiteral("グラフィックス自動修復を適用しました: GPU High + Safe DMA + Accurate Barriers + Reactive Flushing")
    ), 5000);
}

void MainWindow::OnMute() {
    Settings::values.audio_muted.SetValue(!Settings::values.audio_muted.GetValue());
    ApplyDynamicSettingChange();
    UpdateVolumeUI();
    UpdateMuteButton();
}

void MainWindow::OnDecreaseVolume() {
    Settings::values.audio_muted.SetValue(false);
    const auto current_volume = static_cast<s32>(Settings::values.volume.GetValue());
    int step = 5;
    if (current_volume <= 30) {
        step = 2;
    }
    if (current_volume <= 6) {
        step = 1;
    }
    Settings::values.volume.SetValue((std::max)(current_volume - step, 0));
    ApplyDynamicSettingChange();
    UpdateVolumeUI();
    UpdateMuteButton();
}

void MainWindow::OnIncreaseVolume() {
    Settings::values.audio_muted.SetValue(false);
    const auto current_volume = static_cast<s32>(Settings::values.volume.GetValue());
    int step = 5;
    if (current_volume < 30) {
        step = 2;
    }
    if (current_volume < 6) {
        step = 1;
    }
    Settings::values.volume.SetValue(current_volume + step);
    ApplyDynamicSettingChange();
    UpdateVolumeUI();
    UpdateMuteButton();
}

void MainWindow::OnToggleAdaptingFilter() {
    auto filter = Settings::values.scaling_filter.GetValue();
    filter = Settings::ScalingFilter(u32(filter) + 1);
    if (u32(filter) > u32(Settings::EnumMetadata<Settings::ScalingFilter>::GetLast()))
        filter = Settings::EnumMetadata<Settings::ScalingFilter>::GetFirst();
    ApplyStatusBarSwitchableSetting(Settings::values.scaling_filter, filter,
                                    m_session_backup, m_session_backup.scaling_filter,
                                    current_game_path);
    filter_status_button->setChecked(true);
    ApplyDynamicSettingChange();
    UpdateFilterText();
}

void MainWindow::OnToggleGraphicsAPI() {
    auto api = Settings::values.renderer_backend.GetValue();
    switch (api) {
#ifdef HAS_OPENGL
    case Settings::RendererBackend::Vulkan:
        api = Settings::RendererBackend::OpenGL_GLSL;
        break;
    case Settings::RendererBackend::OpenGL_GLSL:
        api = Settings::RendererBackend::OpenGL_GLSL;
        break;
    case Settings::RendererBackend::OpenGL_SPIRV:
        api = Settings::RendererBackend::OpenGL_GLASM;
        break;
    case Settings::RendererBackend::OpenGL_GLASM:
        api = Settings::RendererBackend::Null;
        break;
#else
    case Settings::RendererBackend::Vulkan:
        api = Settings::RendererBackend::Null;
        break;
#endif
    case Settings::RendererBackend::Null:
        api = Settings::RendererBackend::Vulkan;
        break;
    default:
        break;
    }
    ApplyStatusBarSwitchableSettingNoBackup(Settings::values.renderer_backend, api,
                                            current_game_path);
    renderer_status_button->setChecked(api == Settings::RendererBackend::Vulkan);
    ApplyDynamicSettingChange();
    UpdateAPIText();
}

void MainWindow::OnConfigurePerGame() {
    const u64 title_id = QtCommon::system->GetApplicationProcessProgramID();
    OpenPerGameConfiguration(title_id, current_game_path.toStdString());
}

void MainWindow::OnModManagerDialog() {
    u64 title_id = 0;
    QString game_path;
    QString game_name;

    if (QtCommon::system->IsPoweredOn()) {
        title_id = QtCommon::system->GetApplicationProcessProgramID();
        game_path = current_game_path;
        game_name = m_current_addons_game_name;
    } else if (m_current_addons_title_id != 0) {
        title_id = m_current_addons_title_id;
        game_path = QString::fromStdString(m_current_addons_game_path);
        game_name = m_current_addons_game_name;
    }

    if (title_id == 0) {
        QMessageBox::information(this, tr("Менеджер модов"),
                                 tr("Выберите игру из списка или запустите эмуляцию для управления модами."));
        return;
    }

    ModManagerDialog dialog(this, *QtCommon::system, title_id, game_path, game_name);
    dialog.exec();
}

void MainWindow::OnNandManagerDialog() {
    NandManagerDialog dialog(this, *QtCommon::system);
    dialog.exec();
}

void MainWindow::EjectAmiibo() {
    if (input_subsystem) {
        auto* virtual_amiibo = input_subsystem->GetVirtualAmiibo();
        if (virtual_amiibo) {
            virtual_amiibo->CloseAmiibo();
        }
    }
}

bool MainWindow::IsAmiiboActive() const {
    if (input_subsystem) {
        auto* virtual_amiibo = input_subsystem->GetVirtualAmiibo();
        if (virtual_amiibo) {
            return virtual_amiibo->GetCurrentState() == InputCommon::VirtualAmiibo::State::TagNearby;
        }
    }
    return false;
}

void MainWindow::OnCheatsDialog() {
    u64 title_id = 0;
    QString game_path;

    if (QtCommon::system->IsPoweredOn()) {
        title_id = QtCommon::system->GetApplicationProcessProgramID();
        game_path = current_game_path;
    } else if (m_current_addons_title_id != 0) {
        title_id = m_current_addons_title_id;
        game_path = QString::fromStdString(m_current_addons_game_path);
    }

    if (title_id == 0) {
        QMessageBox::information(this, tr("Чит-коды"),
                                 tr("Выберите игру из списка или запустите эмуляцию для настройки чит-кодов."));
        return;
    }

    CheatsDialog dialog(this, *QtCommon::system, title_id, game_path);
    dialog.exec();
}

void MainWindow::StartSilentCheatsSync() {
    // Non-blocking silent background cheats sync for games in library
    const QString cheats_root = QString::fromStdString(Common::FS::PathToUTF8String(
        Common::FS::GetEdenPath(Common::FS::EdenPath::EdenDir) / "cheats"));
    QDir().mkpath(cheats_root);

    const auto& content_provider = QtCommon::system->GetContentProvider();
    const auto entries = content_provider.ListEntriesFilter(FileSys::TitleType::Application,
                                                           FileSys::ContentRecordType::Program);

    for (const auto& entry : entries) {
        const u64 tid = entry.title_id;
        if (tid == 0) continue;

        const QString tid_hex = QString(QStringLiteral("%1")).arg(tid, 16, 16, QLatin1Char('0')).toUpper();
        const QString game_cheats_dir = QDir(cheats_root).filePath(tid_hex);

        bool has_cheats = false;
        QDir dir(game_cheats_dir);
        if (dir.exists()) {
            const auto files = dir.entryInfoList(QStringList() << QStringLiteral("*.txt"), QDir::Files);
            for (const auto& file : files) {
                if (file.size() > 0) {
                    has_cheats = true;
                    break;
                }
            }
        }

        if (!has_cheats) {
            auto* nam = new QNetworkAccessManager(this);
            const QString url_str = QString(QStringLiteral("https://raw.githubusercontent.com/HamletDuFromage/switch-cheats-db/master/cheats/%1.json"))
                                        .arg(tid_hex);
            QNetworkRequest request{QUrl(url_str)};
            request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("STORM_SWITCH_Emulator/13.2.0"));
            request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

            auto* reply = nam->get(request);
            connect(reply, &QNetworkReply::finished, this, [reply, nam, game_cheats_dir, tid_hex]() {
                reply->deleteLater();
                nam->deleteLater();

                if (reply->error() == QNetworkReply::NoError &&
                    reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200) {
                    const QByteArray data = reply->readAll();
                    QJsonDocument doc = QJsonDocument::fromJson(data);
                    if (doc.isObject()) {
                        const QJsonObject root = doc.object();
                        QString combined_cheats;
                        for (auto it = root.begin(); it != root.end(); ++it) {
                            const QString bid_key = it.key();
                            const QJsonObject bid_cheats = it.value().toObject();
                            QString bid_file_content;
                            for (auto c_it = bid_cheats.begin(); c_it != bid_cheats.end(); ++c_it) {
                                const QString cheat_body = c_it.value().toString();
                                if (!cheat_body.trimmed().isEmpty()) {
                                    bid_file_content += cheat_body.trimmed() + QStringLiteral("\n\n");
                                    combined_cheats += cheat_body.trimmed() + QStringLiteral("\n\n");
                                }
                            }
                            if (!bid_file_content.isEmpty()) {
                                QDir().mkpath(game_cheats_dir);
                                QFile bid_file(QDir(game_cheats_dir).filePath(QString(QStringLiteral("%1.txt")).arg(bid_key.toUpper())));
                                if (bid_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                                    QTextStream stream(&bid_file);
                                    stream << bid_file_content;
                                    bid_file.close();
                                }
                            }
                        }
                        if (!combined_cheats.isEmpty()) {
                            QDir().mkpath(game_cheats_dir);
                            QFile all_file(QDir(game_cheats_dir).filePath(QStringLiteral("cheats.txt")));
                            if (all_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                                QTextStream stream(&all_file);
                                stream << combined_cheats;
                                all_file.close();
                            }
                        }
                    }
                }
            });
        }
    }
}

void MainWindow::OpenPerGameConfiguration(u64 title_id, const std::string& file_name) {
    const auto v_file = Core::GetGameFileFromPath(QtCommon::vfs, file_name);

    Settings::SetConfiguringGlobal(false);
    ConfigurePerGame dialog(this, title_id, file_name, vk_device_records, *QtCommon::system);
    dialog.LoadFromFile(v_file);

    const auto result = dialog.exec();

    if (result != QDialog::Accepted && !UISettings::values.configuration_applied) {
        Settings::RestoreGlobalState(QtCommon::system->IsPoweredOn());
        return;
    } else if (result == QDialog::Accepted) {
        dialog.ApplyConfiguration();
    }

    const auto reload = UISettings::values.is_game_list_reload_pending.exchange(false);
    if (reload) {
        OnGameListRefresh();
    }

    // Do not cause the global config to write local settings into the config file
    const bool is_powered_on = QtCommon::system->IsPoweredOn();
    Settings::RestoreGlobalState(is_powered_on);
    QtCommon::system->HIDCore().ReloadInputDevices();
    if (is_powered_on) {
        Settings::UpdateGPUAccuracy();
        Settings::UpdateRescalingInfo();
        QtCommon::system->ApplySettings();
    }

    UISettings::values.configuration_applied = false;

    if (!is_powered_on) {
        config->SaveAllValues();
    }
    UpdateStatusButtons();
}

void MainWindow::OnLoadAmiibo() {
    if (QtCommon::emu_thread == nullptr || !QtCommon::emu_thread->IsRunning() ||
        is_amiibo_file_select_active)
        return;

    auto* virtual_amiibo = input_subsystem->GetVirtualAmiibo();

    // If an amiibo is currently attached, ask if user wants to disconnect/remove it or load another
    if (virtual_amiibo->GetCurrentState() == InputCommon::VirtualAmiibo::State::TagNearby) {
        const auto reply = QMessageBox::question(
            this, tr("Amiibo"),
            tr("An Amiibo is currently active.\nDo you want to disconnect/remove it?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (reply == QMessageBox::Yes) {
            virtual_amiibo->CloseAmiibo();
            QMessageBox::information(this, tr("Amiibo"), tr("The current amiibo has been disconnected and removed."));
            return;
        }
    }

    if (virtual_amiibo->GetCurrentState() != InputCommon::VirtualAmiibo::State::WaitingForAmiibo) {
        QMessageBox::warning(this, tr("Error"), tr("The current game is not looking for amiibos"));
        return;
    }

    is_amiibo_file_select_active = true;
    const QString extensions{QStringLiteral("*.bin")};
    const QString file_filter = tr("Amiibo File (%1);; All Files (*.*)").arg(extensions);
    const QString filename = QFileDialog::getOpenFileName(this, tr("Load Amiibo"), {}, file_filter);
    is_amiibo_file_select_active = false;

    if (filename.isEmpty()) {
        return;
    }

    LoadAmiibo(filename);
}

void MainWindow::OnAmiiboOnlineDatabase() {
    QString current_game_hint = m_current_addons_game_name;
    if (current_game_hint.isEmpty()) {
        if (!current_game_path.isEmpty()) {
            current_game_hint = QFileInfo(current_game_path).completeBaseName();
        } else if (!last_filename_booted.isEmpty()) {
            current_game_hint = QFileInfo(last_filename_booted).completeBaseName();
        }
    }

    AmiiboBrowserDialog dialog(this, *QtCommon::system, current_game_hint);
    connect(&dialog, &AmiiboBrowserDialog::AmiiboSelectedForLoading, this, [this](const QString& path) {
        if (QtCommon::emu_thread && QtCommon::emu_thread->IsRunning()) {
            LoadAmiibo(path);
        }
    });
    connect(&dialog, &AmiiboBrowserDialog::AmiiboRemoveRequested, this, [this]() {
        auto* virtual_amiibo = input_subsystem->GetVirtualAmiibo();
        if (virtual_amiibo) {
            virtual_amiibo->CloseAmiibo();
        }
    });
    dialog.exec();
}

void MainWindow::OnTranslateScreen() {
    if (!m_game_translator) {
        m_game_translator = new GameTranslator(*QtCommon::system, this);
    }
    if (m_game_translator->IsOverlayVisible()) {
        m_game_translator->DismissOverlay();
        return;
    }

    if (render_window && QtCommon::system && QtCommon::system->IsPoweredOn()) {
        QImage lastFrame = render_window->GetLastFrame();
        if (!lastFrame.isNull()) {
            m_game_translator->TranslateFrame(lastFrame);
            return;
        }
        QPointer<GameTranslator> safeTranslator = m_game_translator;
        render_window->CaptureFrame([safeTranslator](const QImage& frame) {
            if (safeTranslator && !frame.isNull()) {
                safeTranslator->TranslateFrame(frame);
            }
        });
    } else {
        m_game_translator->show();
        m_game_translator->raise();
        m_game_translator->activateWindow();
    }
}

void MainWindow::OnOpenTranslatorSettings() {
    if (!m_game_translator) {
        m_game_translator = new GameTranslator(*QtCommon::system, this);
    }
    m_game_translator->show();
    m_game_translator->raise();
    m_game_translator->activateWindow();
    if (floating_translate_button) {
        connect(m_game_translator, &QDialog::finished, this, [this](int) {
            if (floating_translate_button) {
                const bool enable_floating = UISettings::values.enable_floating_translate_button.GetValue();
                floating_translate_button->SetVisibleState(enable_floating && emulation_running);
            }
        }, Qt::UniqueConnection);
    }
    m_game_translator->show();
}

// TODO(crueter): does this need to be ported to QML?
bool MainWindow::question(QWidget* parent, const QString& title, const QString& text,
                          QMessageBox::StandardButtons buttons,
                          QMessageBox::StandardButton defaultButton) {
    QMessageBox* box_dialog = new QMessageBox(parent);
    box_dialog->setWindowTitle(title);
    box_dialog->setText(text);
    box_dialog->setStandardButtons(buttons);
    box_dialog->setDefaultButton(defaultButton);

    ControllerNavigation* controller_navigation =
        new ControllerNavigation(QtCommon::system->HIDCore(), box_dialog);
    connect(controller_navigation, &ControllerNavigation::TriggerKeyboardEvent,
            [box_dialog](Qt::Key key) {
                QKeyEvent* event = new QKeyEvent(QEvent::KeyPress, key, Qt::NoModifier);
                QCoreApplication::postEvent(box_dialog, event);
            });
    int res = box_dialog->exec();

    controller_navigation->UnloadController();
    return res == QMessageBox::Yes;
}

void MainWindow::LoadAmiibo(const QString& filename) {
    auto* virtual_amiibo = input_subsystem->GetVirtualAmiibo();
    const QString title = tr("Error loading Amiibo data");
    
    // Automatically close existing amiibo before injecting new one
    if (virtual_amiibo->GetCurrentState() == InputCommon::VirtualAmiibo::State::TagNearby) {
        virtual_amiibo->CloseAmiibo();
    }

    switch (virtual_amiibo->LoadAmiibo(filename.toStdString())) {
    case InputCommon::VirtualAmiibo::Info::NotAnAmiibo:
        QMessageBox::warning(this, title, tr("The selected file is not a valid amiibo"));
        break;
    case InputCommon::VirtualAmiibo::Info::UnableToLoad:
        QMessageBox::warning(this, title, tr("The selected file is already on use"));
        break;
    case InputCommon::VirtualAmiibo::Info::WrongDeviceState:
        QMessageBox::warning(this, title, tr("The current game is not looking for amiibos"));
        break;
    case InputCommon::VirtualAmiibo::Info::Unknown:
        QMessageBox::warning(this, title, tr("An unknown error occurred"));
        break;
    default:
        break;
    }
}

void MainWindow::OnOpenRootDataFolder() {
    QtCommon::Game::OpenRootDataFolder();
}

void MainWindow::OnOpenNANDFolder() {
    QtCommon::Game::OpenNANDFolder();
}

void MainWindow::OnOpenSDMCFolder() {
    QtCommon::Game::OpenSDMCFolder();
}

void MainWindow::OnOpenModFolder() {
    QtCommon::Game::OpenModFolder();
}

void MainWindow::OnOpenLogFolder() {
    QtCommon::Game::OpenLogFolder();
}

void MainWindow::OnVerifyInstalledContents() {
    QtCommon::Content::VerifyInstalledContents();
}

void MainWindow::OnInstallFirmware() {
    QtCommon::Content::InstallFirmware();
    OnCheckFirmwareDecryption();
}

void MainWindow::OnInstallFirmwareFromZIP() {
    QtCommon::Content::InstallFirmwareZip();
    OnCheckFirmwareDecryption();
}

void MainWindow::OnInstallFirmwareOnline() {
    if (QtCommon::emu_thread != nullptr && QtCommon::emu_thread->IsRunning())
        return;

    OnlineToolsDialog dialog(this, *QtCommon::system, OnlineToolType::Firmware);
    if (dialog.exec() == QDialog::Accepted) {
        OnCheckFirmwareDecryption();
        game_list->PopulateAsync(UISettings::values.game_dirs);
    }
}

// TODO(crueter): QtCommon this: game list populate can be a signal?
void MainWindow::OnInstallDecryptionKeys() {
    // Don't do this while emulation is running.
    if (QtCommon::emu_thread != nullptr && QtCommon::emu_thread->IsRunning())
        return;

    QtCommon::Content::InstallKeys();

    game_list->PopulateAsync(UISettings::values.game_dirs);
    OnCheckFirmwareDecryption();
}

void MainWindow::OnInstallKeysFromZIP() {
    if (QtCommon::emu_thread != nullptr && QtCommon::emu_thread->IsRunning())
        return;

    QtCommon::Content::InstallKeysZip();
    game_list->PopulateAsync(UISettings::values.game_dirs);
    OnCheckFirmwareDecryption();
}

void MainWindow::OnInstallKeysOnline() {
    if (QtCommon::emu_thread != nullptr && QtCommon::emu_thread->IsRunning())
        return;

    OnlineToolsDialog dialog(this, *QtCommon::system, OnlineToolType::Keys);
    if (dialog.exec() == QDialog::Accepted) {
        OnCheckFirmwareDecryption();
        game_list->PopulateAsync(UISettings::values.game_dirs);
    }
}

void MainWindow::OnCheckUpdates(bool manual_check) {
    auto* nam = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl(QStringLiteral("https://api.github.com/repos/ReiKatari/STORM_SWITCH/releases/latest")));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("STORM_SWITCH-Updater"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    auto* reply = nam->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, nam, manual_check]() {
        reply->deleteLater();
        nam->deleteLater();

        auto handle_version_check = [this, manual_check](const QString& tag_name, const QString& release_name,
                                                          const QString& release_body, const QString& html_url) {
            QString clean_tag = tag_name.trimmed();
            if (clean_tag.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
                clean_tag = clean_tag.mid(1);
            }

            QString current_ver = QString::fromStdString(Common::g_build_version).trimmed();
            if (current_ver.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
                current_ver = current_ver.mid(1);
            }

            if (clean_tag.compare(current_ver, Qt::CaseInsensitive) == 0) {
                if (manual_check) {
                    QMessageBox::information(this, tr("STORM SWITCH — Обновления"),
                                             tr("У вас установлена последняя актуальная версия STORM SWITCH (%1).\nОбновлений не найдено.").arg(current_ver));
                }
                return;
            }

            auto parse_version = [](const QString& ver_str) -> std::vector<int> {
                std::vector<int> parts;
                const auto list = ver_str.split(QLatin1Char('.'), Qt::SkipEmptyParts);
                for (const auto& s : list) {
                    parts.push_back(s.toInt());
                }
                return parts;
            };

            const auto remote_parts = parse_version(clean_tag);
            const auto local_parts = parse_version(current_ver);

            bool is_newer = false;
            for (size_t i = 0; i < std::max(remote_parts.size(), local_parts.size()); ++i) {
                const int r = i < remote_parts.size() ? remote_parts[i] : 0;
                const int l = i < local_parts.size() ? local_parts[i] : 0;
                if (r > l) {
                    is_newer = true;
                    break;
                } else if (r < l) {
                    break;
                }
            }

            if (is_newer) {
                QDialog dlg(this);
                dlg.setWindowTitle(tr("Доступно обновление STORM SWITCH"));
                dlg.resize(650, 460);
                dlg.setStyleSheet(QStringLiteral(
                    "QDialog { background-color: #0b0f19; color: #ffffff; font-family: 'Segoe UI', sans-serif; }"
                    "QLabel { color: #ffffff; }"
                    "QTextEdit { background-color: #121826; color: #e2e8f0; border: 1px solid #1e283d; border-radius: 8px; padding: 10px; font-size: 9pt; }"
                    "QPushButton { background-color: #161e30; color: #ffffff; border: 1px solid #23314a; border-radius: 6px; padding: 8px 18px; font-weight: bold; font-size: 9pt; }"
                    "QPushButton:hover { background-color: #00e5ff; color: #000000; border-color: #00e5ff; }"
                    "QPushButton#DownloadBtn { background-color: #00e5ff; color: #000000; border: 1px solid #00e5ff; }"
                    "QPushButton#DownloadBtn:hover { background-color: #33ebff; }"
                ));

                auto* layout = new QVBoxLayout(&dlg);
                layout->setContentsMargins(20, 20, 20, 20);
                layout->setSpacing(14);

                auto* title = new QLabel(tr("<h2 style='margin:0; color:#00e5ff;'>⚡ Доступна новая версия: %1</h2>")
                                             .arg(release_name.isEmpty() ? tag_name : release_name), &dlg);
                auto* sub_info = new QLabel(tr("<span style='color:#94a3b8;'>Текущая версия: <b>%1</b> &nbsp;|&nbsp; Новая версия: <b>%2</b></span>")
                                                .arg(current_ver, clean_tag), &dlg);

                auto* notes_box = new QTextEdit(&dlg);
                notes_box->setReadOnly(true);
                notes_box->setPlainText(release_body.isEmpty() ? tr("Информация об изменениях не указана.") : release_body);

                auto* btn_layout = new QHBoxLayout();
                auto* download_btn = new QPushButton(tr("🚀 Скачать обновление"), &dlg);
                download_btn->setObjectName(QStringLiteral("DownloadBtn"));
                auto* close_btn = new QPushButton(tr("Позже"), &dlg);

                connect(download_btn, &QPushButton::clicked, [&dlg, html_url]() {
                    const QString target_url = html_url.isEmpty()
                        ? QStringLiteral("https://github.com/ReiKatari/STORM_SWITCH/releases")
                        : html_url;
                    QDesktopServices::openUrl(QUrl(target_url));
                    dlg.accept();
                });
                connect(close_btn, &QPushButton::clicked, &dlg, &QDialog::reject);

                btn_layout->addStretch();
                btn_layout->addWidget(close_btn);
                btn_layout->addWidget(download_btn);

                layout->addWidget(title);
                layout->addWidget(sub_info);
                layout->addWidget(notes_box);
                layout->addLayout(btn_layout);

                dlg.exec();
            } else if (manual_check) {
                QMessageBox::information(this, tr("STORM SWITCH — Обновления"),
                                         tr("У вас установлена последняя актуальная версия STORM SWITCH (%1).\nОбновлений не найдено.").arg(current_ver));
            }
        };

        if (reply->error() != QNetworkReply::NoError) {
            // Fallback: GitHub API may return HTTP 403 rate limit for unauthenticated IP.
            // Query the web releases/latest redirect which is never rate limited.
            auto* fallback_nam = new QNetworkAccessManager(this);
            QNetworkRequest fallback_req(QUrl(QStringLiteral("https://github.com/ReiKatari/STORM_SWITCH/releases/latest")));
            fallback_req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"));
            fallback_req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);

            auto* fallback_reply = fallback_nam->get(fallback_req);
            connect(fallback_reply, &QNetworkReply::finished, this, [this, fallback_reply, fallback_nam, manual_check, handle_version_check]() {
                fallback_reply->deleteLater();
                fallback_nam->deleteLater();

                QString target_url;
                const auto redirect_var = fallback_reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
                if (redirect_var.isValid() && !redirect_var.toUrl().isEmpty()) {
                    target_url = redirect_var.toUrl().toString();
                } else {
                    target_url = fallback_reply->url().toString();
                }

                static const QRegularExpression tag_regex(QStringLiteral(R"(/releases/tag/v?([0-9]+\.[0-9]+(?:\.[0-9]+)*))"));
                const auto match = tag_regex.match(target_url);
                if (match.hasMatch()) {
                    const QString latest_ver = match.captured(1);
                    handle_version_check(latest_ver, tr("Релиз %1").arg(latest_ver),
                                         tr("Перейдите по ссылке для загрузки обновлений."),
                                         QStringLiteral("https://github.com/ReiKatari/STORM_SWITCH/releases/latest"));
                    return;
                }

                if (manual_check) {
                    QMessageBox::information(this, tr("STORM SWITCH — Обновления"),
                                             tr("У вас установлена последняя актуальная версия STORM SWITCH (%1).").arg(QString::fromStdString(Common::g_build_version)));
                }
            });
            return;
        }

        const auto doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            if (manual_check) {
                QMessageBox::warning(this, tr("Проверка обновлений"), tr("Получен некорректный ответ от сервера обновлений."));
            }
            return;
        }

        const auto obj = doc.object();
        const QString tag_name = obj[QStringLiteral("tag_name")].toString().trimmed();
        const QString release_name = obj[QStringLiteral("name")].toString().trimmed();
        const QString release_body = obj[QStringLiteral("body")].toString();
        const QString html_url = obj[QStringLiteral("html_url")].toString().trimmed();

        handle_version_check(tag_name, release_name, release_body, html_url);
    });
}

void MainWindow::OnAbout() {
    AboutDialog aboutDialog(this);
    aboutDialog.exec();
}

void MainWindow::OnEdenDependencies() {
    DepsDialog depsDialog(this);
    depsDialog.exec();
}

void MainWindow::OnDataDialog() {
    DataDialog dataDialog(this);
    dataDialog.exec();

    // refresh stuff in case it was cleared
    OnGameListRefresh();
}

void MainWindow::OnToggleFilterBar() {
    game_list->SetFilterVisible(ui->action_Show_Filter_Bar->isChecked());
    if (ui->action_Show_Filter_Bar->isChecked())
        game_list->SetFilterFocus();
    else
        game_list->ClearFilter();
}

void MainWindow::OnToggleStatusBar() {
    statusBar()->setVisible(ui->action_Show_Status_Bar->isChecked());
}

void MainWindow::OnTogglePerfOverlay() {
    if (!perf_overlay) return;
    if (ui->action_Show_Performance_Overlay->isChecked()) {
        perf_overlay->SetExpanded(true);
        perf_overlay->show();
    } else {
        perf_overlay->hide();
    }
}

void MainWindow::OnQuickSave() {
    OnSaveStateSlot(1);
}

void MainWindow::OnQuickLoad() {
    OnLoadStateSlot(1);
}

void MainWindow::OnSaveStateSlot(int slot) {
    if (!emulation_running || !QtCommon::system || !QtCommon::system->IsPoweredOn()) {
        QMessageBox::information(
            this,
            StormLang(QStringLiteral("Быстрое сохранение SESS v2"),
                      QStringLiteral("Quick Save SESS v2"),
                      QStringLiteral("Schnellspeichern SESS v2"),
                      QStringLiteral("Sauvegarde rapide SESS v2"),
                      QStringLiteral("快速保存 SESS v2"),
                      QStringLiteral("クイックセーブ SESS v2")),
            StormLang(QStringLiteral("Запустите игру, чтобы создать мгновенное быстрое сохранение (Слот %1).\nВо время игры вы также можете нажать клавишу F5 (или настроить удобную клавишу/кнопку геймпада в меню «Эмуляция -> Параметры -> Общие -> Горячие клавиши»).").arg(slot),
                      QStringLiteral("Launch a game first to create an instant Quick Save (Slot %1).\nDuring gameplay you can also press F5 (or customize the hotkey in Emulation -> Configure -> General -> Hotkeys).").arg(slot),
                      QStringLiteral("Starten Sie zuerst ein Spiel, um eine Schnell-Speicherung (Slot %1) zu erstellen.").arg(slot),
                      QStringLiteral("Lancez d'abord un jeu pour créer une sauvegarde rapide (Emplacement %1).").arg(slot),
                      QStringLiteral("请先启动游戏以创建快速保存 (槽位 %1)。").arg(slot),
                      QStringLiteral("クイックセーブ (スロット %1) を作成するには、まずゲームを起動してください。").arg(slot)));
        return;
    }
    const u64 title_id = QtCommon::system->GetApplicationProcessProgramID();
    if (title_id == 0) {
        return;
    }
    {
        const auto path = Core::GetSaveStatePath(title_id, static_cast<u32>(slot));
        const auto res = Core::CreateSaveState(*QtCommon::system, path);
        if (res != Core::SaveStateResult::Success) {
            statusBar()->showMessage(StormLang(
                QStringLiteral("Ошибка быстрого сохранения в Слот %1").arg(slot),
                QStringLiteral("Failed to save state to Slot %1").arg(slot),
                QStringLiteral("Fehler beim Speichern in Slot %1").arg(slot),
                QStringLiteral("Échec de la sauvegarde dans l'emplacement %1").arg(slot),
                QStringLiteral("保存到槽位 %1 失败").arg(slot),
                QStringLiteral("スロット %1 へのセーブに失敗しました").arg(slot)), 4000);
            return;
        }
        if (perf_overlay) {
            perf_overlay->RefreshSaveSlots();
        }
    }
    if (render_window) {
        auto preview_path = Core::GetSaveStatePath(title_id, static_cast<u32>(slot));
        preview_path.replace_extension(".png");
        const QString qpreview = QString::fromStdString(Common::FS::PathToUTF8String(preview_path));
        if (QScreen* scr = render_window->screen()) {
            const QPixmap grabbed = scr->grabWindow(render_window->winId());
            if (!grabbed.isNull()) {
                grabbed.save(qpreview, "PNG");
            }
        }
        render_window->CaptureScreenshot(qpreview);
    }
    const QString msg = StormLang(
        QStringLiteral("⚡ Быстрое сохранение: Слот %1 сохранён (SESS v2)").arg(slot),
        QStringLiteral("⚡ Quick Save: Slot %1 saved (SESS v2)").arg(slot),
        QStringLiteral("⚡ Schnellspeichern: Slot %1 gespeichert (SESS v2)").arg(slot),
        QStringLiteral("⚡ Sauvegarde rapide : Emplacement %1 enregistré (SESS v2)").arg(slot),
        QStringLiteral("⚡ 快速保存：槽位 %1 已保存 (SESS v2)").arg(slot),
        QStringLiteral("⚡ クイックセーブ：スロット %1 に保存しました (SESS v2)").arg(slot));
    statusBar()->showMessage(msg, 4000);
    if (in_game_notification) {
        const QString notif_title = StormLang(
            QStringLiteral("\u26a1 \u0411\u042b\u0421\u0422\u0420\u041e\u0415 \u0421\u041e\u0425\u0420\u0410\u041d\u0415\u041d\u0418\u0415 - \u0421\u041b\u041e\u0422 %1").arg(slot),
            QStringLiteral("\u26a1 QUICK SAVE COMPLETE - SLOT %1").arg(slot),
            QStringLiteral("\u26a1 SCHNELLSPEICHERN - SLOT %1").arg(slot),
            QStringLiteral("\u26a1 SAUVEGARDE RAPIDE - EMPLACEMENT %1").arg(slot),
            QStringLiteral("\u26a1 \u5feb\u901f\u4fdd\u5b58\u6210\u529f - \u69fd\u4f4d %1").arg(slot),
            QStringLiteral("\u26a1 \u30af\u30a4\u30c3\u30af\u30bb\u30fc\u30d6\u5b8c\u4e86 - \u30b9\u30ed\u30c3\u30c8 %1").arg(slot));
        const QString notif_sub = StormLang(
            QStringLiteral("\u041c\u0433\u043d\u043e\u0432\u0435\u043d\u043d\u044b\u0439 \u0441\u043d\u0438\u043c\u043e\u043a \u043f\u0430\u043c\u044f\u0442\u0438 SESS v2 \u0443\u0441\u043f\u0435\u0448\u043d\u043e \u0441\u043e\u0445\u0440\u0430\u043d\u0451\u043d"),
            QStringLiteral("Instant memory snapshot SESS v2 saved successfully"),
            QStringLiteral("Sofortiger Speicher-Snapshot SESS v2 erfolgreich gespeichert"),
            QStringLiteral("Instantan\u00e9 m\u00e9moire SESS v2 enregistr\u00e9 avec succ\u00e8s"),
            QStringLiteral("SESS v2 \u5185\u5b58\u5feb\u7167\u5df2\u6210\u529f\u4fdd\u5b58"),
            QStringLiteral("SESS v2 \u30e1\u30e2\u30ea\u30b9\u30ca\u30c3\u30d7\u30b7\u30e7\u30c3\u30c8\u3092\u4fdd\u5b58\u3057\u307e\u3057\u305f"));
        in_game_notification->ShowNotification(notif_title, notif_sub, QColor(0, 229, 255), QStringLiteral("\U0001f4be"), 2800);
    }
    RefreshSaveStateMenus();
}

void MainWindow::OnLoadStateSlot(int slot) {
    if (!emulation_running || !QtCommon::system || !QtCommon::system->IsPoweredOn()) {
        QMessageBox::information(
            this,
            StormLang(QStringLiteral("Быстрая загрузка SESS v2"),
                      QStringLiteral("Quick Load SESS v2"),
                      QStringLiteral("Schnellladen SESS v2"),
                      QStringLiteral("Chargement rapide SESS v2"),
                      QStringLiteral("快速读取 SESS v2"),
                      QStringLiteral("クイックロード SESS v2")),
            StormLang(QStringLiteral("Запустите игру, чтобы загрузить быстрое сохранение из Слота %1.\nВо время игры вы можете нажать клавишу F7 (или настроить удобную клавишу/кнопку геймпада в меню «Эмуляция -> Параметры -> Общие -> Горячие клавиши»).").arg(slot),
                      QStringLiteral("Launch a game first to load a Quick Save from Slot %1.\nDuring gameplay you can press F7 (or customize the hotkey in Emulation -> Configure -> General -> Hotkeys).").arg(slot),
                      QStringLiteral("Starten Sie zuerst ein Spiel, um aus Slot %1 zu laden.").arg(slot),
                      QStringLiteral("Lancez d'abord un jeu pour charger depuis l'emplacement %1.").arg(slot),
                      QStringLiteral("请先启动游戏以从槽位 %1 读取快速保存。").arg(slot),
                      QStringLiteral("スロット %1 からクイックロードするには、まずゲームを起動してください。").arg(slot)));
        return;
    }
    const u64 title_id = QtCommon::system->GetApplicationProcessProgramID();
    if (title_id == 0) {
        return;
    }
    const auto path = Core::GetSaveStatePath(title_id, static_cast<u32>(slot));
    if (!std::filesystem::exists(path)) {
        const QString empty_msg = StormLang(
            QStringLiteral("⚠️ Слот %1 пуст - сначала сделайте быстрое сохранение (F5)").arg(slot),
            QStringLiteral("⚠️ Slot %1 is empty - create a Quick Save (F5) first").arg(slot),
            QStringLiteral("⚠️ Slot %1 ist leer - bitte zuerst speichern (F5)").arg(slot),
            QStringLiteral("⚠️ L'emplacement %1 est vide - sauvegardez d'abord (F5)").arg(slot),
            QStringLiteral("⚠️ 槽位 %1 为空 - 请先快速保存 (F5)").arg(slot),
            QStringLiteral("⚠️ スロット %1 は空です - 先にクイックセーブ (F5) してください").arg(slot));
        statusBar()->showMessage(empty_msg, 4000);
        if (in_game_notification) {
            in_game_notification->ShowNotification(QStringLiteral("SESS v2"), empty_msg, QColor(255, 193, 7), QStringLiteral("⚠️"), 3000);
        }
        return;
    }
    const auto res = Core::LoadSaveState(*QtCommon::system, path);
    if (res != Core::SaveStateResult::Success) {
        const QString err_msg = StormLang(
            QStringLiteral("\u041e\u0448\u0438\u0431\u043a\u0430 \u0431\u044b\u0441\u0442\u0440\u043e\u0439 \u0437\u0430\u0433\u0440\u0443\u0437\u043a\u0438 \u0438\u0437 \u0421\u043b\u043e\u0442\u0430 %1").arg(slot),
            QStringLiteral("Failed to load state from Slot %1").arg(slot),
            QStringLiteral("Fehler beim Laden aus Slot %1").arg(slot),
            QStringLiteral("\u00c9chec du chargement depuis l'emplacement %1").arg(slot),
            QStringLiteral("\u4ece\u69fd\u4f4d %1 \u8bfb\u53d6\u5931\u8d25").arg(slot),
            QStringLiteral("\u30b9\u30ed\u30c3\u30c8 %1 \u304b\u3089\u306e\u30ed\u30fc\u30c9\u306b\u5931\u6557\u3057\u307e\u3057\u305f").arg(slot));
        statusBar()->showMessage(err_msg, 4000);
        if (in_game_notification) {
            in_game_notification->ShowNotification(QStringLiteral("SESS v2"), err_msg, QColor(239, 68, 68), QStringLiteral("\u274c"), 3000);
        }
        return;
    }
    if (perf_overlay) {
        perf_overlay->RefreshSaveSlots();
    }
    const QString msg = StormLang(
        QStringLiteral("\u26a1 \u0411\u044b\u0441\u0442\u0440\u0430\u044f \u0437\u0430\u0433\u0440\u0443\u0437\u043a\u0430: \u0421\u043b\u043e\u0442 %1 \u0437\u0430\u0433\u0440\u0443\u0436\u0435\u043d (SESS v2)").arg(slot),
        QStringLiteral("\u26a1 Quick Load: Slot %1 loaded (SESS v2)").arg(slot),
        QStringLiteral("\u26a1 Schnellladen: Slot %1 geladen (SESS v2)").arg(slot),
        QStringLiteral("\u26a1 Chargement rapide : Emplacement %1 charg\u00e9 (SESS v2)").arg(slot),
        QStringLiteral("\u26a1 \u5feb\u901f\u8bfb\u53d6\uff1a\u69fd\u4f4d %1 \u5df2\u52a0\u8f7d (SESS v2)").arg(slot),
        QStringLiteral("\u26a1 \u30af\u30a4\u30c3\u30af\u30ed\u30fc\u30c9\uff1a\u30b9\u30ed\u30c3\u30c8 %1 \u3092\u30ed\u30fc\u30c9\u3057\u307e\u3057\u305f (SESS v2)").arg(slot));
    statusBar()->showMessage(msg, 4000);
    if (render_window) {
        auto preview_path = Core::GetSaveStatePath(title_id, static_cast<u32>(slot));
        preview_path.replace_extension(".png");
        if (!std::filesystem::exists(preview_path)) {
            const QString qpreview = QString::fromStdString(Common::FS::PathToUTF8String(preview_path));
            QTimer::singleShot(700, this, [this, qpreview]() {
                if (render_window && emulation_running) {
                    if (QScreen* scr = render_window->screen() ? render_window->screen()
                                                               : QGuiApplication::primaryScreen()) {
                        const QPixmap grabbed = scr->grabWindow(render_window->winId());
                        if (!grabbed.isNull() && grabbed.width() > 32 && grabbed.height() > 32) {
                            grabbed.scaled(640, 360, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                                .save(qpreview, "PNG");
                        }
                    }
                    render_window->CaptureScreenshot(qpreview);
                }
            });
        }
    }
    if (in_game_notification) {
        const QString notif_title = StormLang(
            QStringLiteral("\u26a1 \u0411\u042b\u0421\u0422\u0420\u0410\u042f \u0417\u0410\u0413\u0420\u0423\u0417\u041a\u0410 - \u0421\u041b\u041e\u0422 %1").arg(slot),
            QStringLiteral("\u26a1 QUICK LOAD COMPLETE - SLOT %1").arg(slot),
            QStringLiteral("\u26a1 SCHNELLLADEN - SLOT %1").arg(slot),
            QStringLiteral("\u26a1 CHARGEMENT RAPIDE - EMPLACEMENT %1").arg(slot),
            QStringLiteral("\u26a1 \u5feb\u901f\u8bfb\u53d6\u6210\u529f - \u69fd\u4f4d %1").arg(slot),
            QStringLiteral("\u26a1 \u30af\u30a4\u30c3\u30af\u30ed\u30fc\u30c9\u5b8c\u4e86 - \u30b9\u30ed\u30c3\u30c8 %1").arg(slot));
        const QString notif_sub = StormLang(
            QStringLiteral("\u0421\u043e\u0441\u0442\u043e\u044f\u043d\u0438\u0435 \u0438\u0433\u0440\u044b SESS v2 \u0443\u0441\u043f\u0435\u0448\u043d\u043e \u0432\u043e\u0441\u0441\u0442\u0430\u043d\u043e\u0432\u043b\u0435\u043d\u043e"),
            QStringLiteral("Game state SESS v2 restored successfully"),
            QStringLiteral("Spielstatus SESS v2 erfolgreich wiederhergestellt"),
            QStringLiteral("\u00c9tat du jeu SESS v2 restaur\u00e9 avec succ\u00e8s"),
            QStringLiteral("SESS v2 \u6e38\u620f\u72b6\u6001\u5df2\u6210\u529f\u6062\u590d"),
            QStringLiteral("SESS v2 \u30b2\u30fc\u30e0\u72b6\u614b\u3092\u5fa9\u5143\u3057\u307e\u3057\u305f"));
        in_game_notification->ShowNotification(notif_title, notif_sub, QColor(52, 211, 153), QStringLiteral("\u26a1"), 2800);
    }
}

void MainWindow::OnSaveStateManager() {
    constexpr int kMaxSlots = 10;
    const auto savestates_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::SaveDir) / "savestates";
    std::error_code dir_ec;
    std::filesystem::create_directories(savestates_dir, dir_ec);

    // Collect all Title IDs that are currently running, selected, have .ses files, or are installed in ContentProvider
    std::vector<u64> title_ids;
    auto add_tid = [&title_ids](u64 tid) {
        if (tid != 0 && std::find(title_ids.begin(), title_ids.end(), tid) == title_ids.end()) {
            title_ids.push_back(tid);
        }
    };

    u64 active_tid = 0;
    if (emulation_running && QtCommon::system && QtCommon::system->IsPoweredOn()) {
        active_tid = QtCommon::system->GetApplicationProcessProgramID();
        add_tid(active_tid);
    }
    if (m_current_addons_title_id != 0) {
        add_tid(m_current_addons_title_id);
    }

    if (std::filesystem::exists(savestates_dir, dir_ec)) {
        static const QRegularExpression ses_re(QStringLiteral(R"(^([0-9A-Fa-f]{16})_slot(\d+)\.ses$)"));
        for (const auto& entry : std::filesystem::directory_iterator(savestates_dir, dir_ec)) {
            if (!entry.is_regular_file()) continue;
            const QString fname = QString::fromStdString(Common::FS::PathToUTF8String(entry.path().filename()));
            const auto m = ses_re.match(fname);
            if (m.hasMatch()) {
                bool ok = false;
                const u64 parsed_tid = m.captured(1).toULongLong(&ok, 16);
                if (ok && parsed_tid != 0) {
                    add_tid(parsed_tid);
                }
            }
        }
    }

    if (QtCommon::system) {
        const auto& cp = QtCommon::system->GetContentProvider();
        for (const auto& e : cp.ListEntriesFilter(FileSys::TitleType::Application, FileSys::ContentRecordType::Program)) {
            if (e.title_id != 0) {
                add_tid(e.title_id);
            }
        }
    }

    // If a game is currently running and any of its occupied slots lacks a .png preview, capture it now
    if (active_tid != 0 && render_window) {
        for (int s = 1; s <= kMaxSlots; ++s) {
            const auto ses_p = Core::GetSaveStatePath(active_tid, static_cast<u32>(s));
            auto png_p = ses_p;
            png_p.replace_extension(".png");
            std::error_code ec;
            if (std::filesystem::exists(ses_p, ec) && !std::filesystem::exists(png_p, ec)) {
                const QString qpreview = QString::fromStdString(Common::FS::PathToUTF8String(png_p));
                if (QScreen* scr = render_window->screen()) {
                    const QPixmap grabbed = scr->grabWindow(render_window->winId());
                    if (!grabbed.isNull()) {
                        grabbed.save(qpreview, "PNG");
                    }
                }
                render_window->CaptureScreenshot(qpreview);
            }
        }
    }

    auto resolve_game_title = [](u64 tid) -> QString {
        if (tid == 0) {
            return StormLang(QStringLiteral("Игра не выбрана"), QStringLiteral("No Game Selected"),
                             QStringLiteral("Kein Spiel ausgewählt"), QStringLiteral("Aucun jeu sélectionné"),
                             QStringLiteral("未选择游戏"), QStringLiteral("ゲーム未選択"));
        }
        const QString hex_id = QStringLiteral("%1").arg(tid, 16, 16, QLatin1Char('0')).toUpper();
        if (QtCommon::system) {
            const FileSys::PatchManager pm(tid, QtCommon::system->GetFileSystemController(),
                                           QtCommon::system->GetContentProvider());
            const auto control = pm.GetControlMetadata();
            if (control.first != nullptr) {
                const std::string name = control.first->GetApplicationName();
                if (!name.empty()) {
                    return QStringLiteral("%1 [%2]").arg(QString::fromStdString(name), hex_id);
                }
            }
        }
        if (const auto* prof = Core::GameFixDatabase::GetProfile(tid)) {
            if (!prof->game_name.empty()) {
                return QStringLiteral("%1 [%2]").arg(QString::fromStdString(prof->game_name), hex_id);
            }
        }
        if (tid == 0x010020D01AD24000ULL) {
            return QStringLiteral("ANIMAL WELL [010020D01AD24000]");
        }
        return QStringLiteral("Title ID: %1 [%1]").arg(hex_id);
    };

    auto make_placeholder_preview = [&resolve_game_title](u64 tid, int slot_num, bool exists,
                                                          const Core::SaveStateHeader& hdr,
                                                          double size_mb, const QString& dt_str,
                                                          const QString& note_str) -> QPixmap {
        QPixmap px(640, 360);
        px.fill(Qt::transparent);
        QPainter p(&px);
        p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

        QLinearGradient bg(0, 0, 640, 360);
        if (exists) {
            bg.setColorAt(0.0, QColor(14, 27, 46));
            bg.setColorAt(0.5, QColor(18, 38, 64));
            bg.setColorAt(1.0, QColor(10, 18, 32));
        } else {
            bg.setColorAt(0.0, QColor(15, 19, 28));
            bg.setColorAt(1.0, QColor(10, 13, 20));
        }
        p.setBrush(bg);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(0, 0, 640, 360, 14, 14);

        p.setPen(QPen(exists ? QColor(0, 229, 255, 18) : QColor(148, 163, 184, 10), 1));
        for (int x = 32; x < 640; x += 32) p.drawLine(x, 0, x, 360);
        for (int y = 32; y < 360; y += 32) p.drawLine(0, y, 640, y);

        if (exists) {
            QRadialGradient glow(320, 165, 180);
            glow.setColorAt(0.0, QColor(0, 229, 255, 55));
            glow.setColorAt(0.6, QColor(56, 189, 248, 18));
            glow.setColorAt(1.0, Qt::transparent);
            p.setBrush(glow);
            p.drawEllipse(140, 10, 360, 310);
        }

        p.setBrush(exists ? QColor(0, 229, 255, 35) : QColor(51, 65, 85, 80));
        p.setPen(QPen(exists ? QColor(0, 229, 255, 140) : QColor(100, 116, 139, 100), 1.5));
        p.drawRoundedRect(24, 22, 168, 34, 8, 8);

        QFont f_badge(QStringLiteral("Segoe UI"), 10, QFont::Bold);
        p.setFont(f_badge);
        p.setPen(exists ? QColor(0, 229, 255) : QColor(148, 163, 184));
        p.drawText(QRect(24, 22, 168, 34), Qt::AlignCenter,
                   QStringLiteral("SLOT #%1 - SESS v2").arg(slot_num));

        QFont f_icon(QStringLiteral("Segoe UI Emoji"), 36, QFont::Bold);
        p.setFont(f_icon);
        p.setPen(exists ? QColor(56, 189, 248) : QColor(71, 85, 105));
        p.drawText(QRect(0, 78, 640, 60), Qt::AlignCenter,
                   exists ? QStringLiteral("⚡") : QStringLiteral("📭"));

        QFont f_title(QStringLiteral("Segoe UI"), 14, QFont::Bold);
        p.setFont(f_title);
        p.setPen(exists ? QColor(241, 245, 249) : QColor(100, 116, 139));
        p.drawText(QRect(24, 146, 592, 32), Qt::AlignCenter,
                   exists ? resolve_game_title(tid)
                          : StormLang(QStringLiteral("Слот %1 пуст").arg(slot_num),
                                      QStringLiteral("Slot %1 is Empty").arg(slot_num),
                                      QStringLiteral("Slot %1 ist leer").arg(slot_num),
                                      QStringLiteral("Emplacement %1 vide").arg(slot_num),
                                      QStringLiteral("槽位 %1 为空").arg(slot_num),
                                      QStringLiteral("スロット %1 は空です").arg(slot_num)));

        QFont f_sub(QStringLiteral("Segoe UI"), 10, QFont::DemiBold);
        p.setFont(f_sub);
        p.setPen(exists ? QColor(52, 211, 153) : QColor(100, 116, 139));
        if (exists) {
            p.drawText(QRect(30, 184, 580, 26), Qt::AlignCenter,
                       QStringLiteral("%1 MB  -  %2 4KB Pages  -  %3 Threads")
                           .arg(QString::number(size_mb, 'f', 2))
                           .arg(hdr.num_stored_pages)
                           .arg(hdr.num_threads));
            p.setPen(QColor(148, 163, 184));
            p.drawText(QRect(30, 214, 580, 24), Qt::AlignCenter,
                       QStringLiteral("🕒 %1").arg(dt_str));
            if (!note_str.isEmpty()) {
                p.setPen(QColor(251, 191, 36));
                p.drawText(QRect(40, 244, 560, 44), Qt::AlignCenter | Qt::TextWordWrap,
                           QStringLiteral("📝 %1").arg(note_str));
            }
            p.setPen(QColor(125, 211, 252, 190));
            QFont f_hint(QStringLiteral("Segoe UI"), 9);
            p.setFont(f_hint);
            p.drawText(QRect(30, 316, 580, 24), Qt::AlignCenter,
                       StormLang(QStringLiteral("Скриншот-превью создаётся автоматически при сохранении или загрузке слота в игре"),
                                 QStringLiteral("Live gameplay preview is captured automatically on save/load"),
                                 QStringLiteral("Live-Vorschau wird beim Speichern/Laden automatisch aufgenommen"),
                                 QStringLiteral("L'aperçu en direct est capturé automatiquement lors de la sauvegarde/du chargement"),
                                 QStringLiteral("保存或加载时自动捕获实时游戏预览图"),
                                 QStringLiteral("セーブ・ロード時にゲーム画面のプレビューが自動保存されます")));
        } else {
            p.drawText(QRect(40, 196, 560, 28), Qt::AlignCenter,
                       StormLang(QStringLiteral("Нажмите «Сохранить в слот» во время игры или скопируйте сюда другое сохранение"),
                                 QStringLiteral("Click 'Save to Slot' during gameplay or copy another slot here"),
                                 QStringLiteral("Klicken Sie im Spiel auf 'In Slot speichern' oder kopieren Sie einen Slot hierher"),
                                 QStringLiteral("Cliquez sur « Sauvegarder » en jeu ou copiez un autre emplacement ici"),
                                 QStringLiteral("在游戏过程中点击“保存到槽位”或将其他槽位复制到此处"),
                                 QStringLiteral("ゲーム中に「スロットに保存」を押すか、他のスロットからコピーしてください")));
        }
        p.end();
        return px;
    };

    QDialog dlg(this);
    dlg.setWindowTitle(StormLang(
        QStringLiteral("🗂️ Менеджер быстрых сохранений (STORM SESS v2)"),
        QStringLiteral("🗂️ Save State Manager (STORM SESS v2)"),
        QStringLiteral("🗂️ Schnellspeicher-Manager (STORM SESS v2)"),
        QStringLiteral("🗂️ Gestionnaire de sauvegardes rapides (STORM SESS v2)"),
        QStringLiteral("🗂️ 快速保存管理器 (STORM SESS v2)"),
        QStringLiteral("🗂️ クイックセーブマネージャー (STORM SESS v2)")));
    dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    dlg.setMinimumSize(1140, 720);
    dlg.resize(1220, 770);

    dlg.setStyleSheet(QStringLiteral(
        "QDialog {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #0B0F19, stop:0.5 #111827, stop:1 #090D16);"
        "  color: #F1F5F9;"
        "  font-family: 'Segoe UI', sans-serif;"
        "}"
        "QLabel { color: #E2E8F0; background: transparent; }"
        "QComboBox {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1E293B, stop:1 #0F172A);"
        "  color: #F8FAFC; border: 1.5px solid rgba(56, 189, 248, 0.45); border-radius: 8px;"
        "  padding: 5px 12px; font-size: 9.5pt; font-weight: 600; min-height: 24px;"
        "}"
        "QComboBox:hover { border-color: #00E5FF; }"
        "QComboBox QAbstractItemView {"
        "  background-color: #0F172A; color: #F8FAFC; selection-background-color: #0284C7;"
        "  border: 1.5px solid #38BDF8; border-radius: 6px; padding: 4px;"
        "}"
        "QListWidget {"
        "  background-color: rgba(15, 23, 42, 0.78);"
        "  border: 1px solid rgba(56, 189, 248, 0.28);"
        "  border-radius: 12px; padding: 6px; outline: none;"
        "}"
        "QListWidget::item {"
        "  background-color: rgba(30, 41, 59, 0.55);"
        "  border: 1px solid rgba(148, 163, 184, 0.16);"
        "  border-radius: 10px; margin: 4px 2px; padding: 6px; color: #F1F5F9;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: rgba(30, 58, 95, 0.65);"
        "  border: 1px solid rgba(56, 189, 248, 0.55);"
        "}"
        "QListWidget::item:selected {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(2, 132, 199, 0.42), stop:1 rgba(15, 23, 42, 0.85));"
        "  border: 1.5px solid #00E5FF; color: #FFFFFF;"
        "}"
        "QLineEdit, QPlainTextEdit {"
        "  background-color: rgba(15, 23, 42, 0.92); color: #F8FAFC;"
        "  border: 1.5px solid rgba(56, 189, 248, 0.38); border-radius: 8px;"
        "  padding: 5px 10px; font-size: 9.5pt;"
        "}"
        "QLineEdit:focus, QPlainTextEdit:focus { border: 1.5px solid #00E5FF; background-color: #131D31; }"
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1E293B, stop:1 #0F172A);"
        "  color: #F1F5F9; border: 1px solid rgba(148, 163, 184, 0.32);"
        "  border-bottom: 2px solid rgba(0, 0, 0, 0.65);"
        "  border-radius: 8px; padding: 8px 14px; font-size: 9.5pt; font-weight: 600;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #334155, stop:1 #1E293B);"
        "  border-color: #38BDF8; color: #FFFFFF;"
        "}"
        "QPushButton:pressed { padding-top: 9px; padding-bottom: 7px; }"
        "QPushButton:disabled { color: #475569; background: rgba(15, 23, 42, 0.4); border-color: rgba(71, 85, 105, 0.2); }"
    ));

    auto* root_layout = new QVBoxLayout(&dlg);
    root_layout->setContentsMargins(18, 14, 18, 14);
    root_layout->setSpacing(10);

    // Top Header Banner
    auto* header_frame = new QFrame(&dlg);
    header_frame->setStyleSheet(QStringLiteral(
        "QFrame {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(0, 229, 255, 0.14), stop:0.5 rgba(30, 41, 59, 0.65), stop:1 rgba(16, 185, 129, 0.12));"
        "  border: 1px solid rgba(0, 229, 255, 0.35); border-radius: 12px;"
        "}"));
    auto* header_layout = new QHBoxLayout(header_frame);
    header_layout->setContentsMargins(16, 10, 16, 10);
    header_layout->setSpacing(16);

    // Left column of header: Title + 4-line (3 line breaks) description
    auto* title_col = new QVBoxLayout();
    title_col->setSpacing(4);
    auto* lbl_main_title = new QLabel(StormLang(
        QStringLiteral("⚡ МЕНЕДЖЕР БЫСТРЫХ СОХРАНЕНИЙ SESS v2"),
        QStringLiteral("⚡ STORM SESS v2 SAVE STATE MANAGER"),
        QStringLiteral("⚡ STORM SESS v2 SCHNELLSPEICHER-MANAGER"),
        QStringLiteral("⚡ GESTIONNAIRE DE SAUVEGARDES SESS v2"),
        QStringLiteral("⚡ STORM SESS v2 快速保存管理器"),
        QStringLiteral("⚡ STORM SESS v2 クイックセーブマネージャー")), header_frame);
    lbl_main_title->setStyleSheet(QStringLiteral("font-size: 12.5pt; font-weight: 800; color: #00E5FF; border: none; background: transparent;"));
    title_col->addWidget(lbl_main_title);

    auto* lbl_subtitle = new QLabel(StormLang(
        QStringLiteral("Мгновенные снимки состояния памяти и GPU:\n• F5 - быстрое сохранение в Слот 1\n• F7 - быстрая загрузка из Слота 1\n• Управление 10 слотами, заметками и превью"),
        QStringLiteral("Instant RAM & GPU state snapshots:\n• F5 - Quick Save to Slot 1\n• F7 - Quick Load from Slot 1\n• Manage 10 slots, custom notes & live previews"),
        QStringLiteral("Sofortige RAM- & GPU-Schnappschüsse:\n• F5 - Schnellspeichern in Slot 1\n• F7 - Schnellladen aus Slot 1\n• Verwaltung von 10 Slots, Notizen & Vorschau"),
        QStringLiteral("Instantanés d'état RAM & GPU :\n• F5 - Sauvegarde rapide dans l'emplacement 1\n• F7 - Chargement rapide depuis l'emplacement 1\n• Gestion de 10 emplacements, notes et aperçus"),
        QStringLiteral("即时内存与 GPU 状态快照：\n• F5 - 快速保存到槽位 1\n• F7 - 从槽位 1 快速读取\n• 管理 10 个槽位、自定义备注与实时预览图"),
        QStringLiteral("高速メモリ＆GPU状態スナップショット:\n• F5 - スロット 1 にクイックセーブ\n• F7 - スロット 1 からクイックロード\n• 10スロット・メモ・ライブプレビュー管理")), header_frame);
    lbl_subtitle->setStyleSheet(QStringLiteral("font-size: 9pt; color: #CBD5E1; border: none; background: transparent; line-height: 1.25;"));
    title_col->addWidget(lbl_subtitle);
    header_layout->addLayout(title_col, 0);

    // Right column of header: Fast Game Search + Full-Width Game Selector with Full Title & ID + Folder Button
    auto* selector_col = new QVBoxLayout();
    selector_col->setSpacing(6);

    auto* search_and_folder_row = new QHBoxLayout();
    search_and_folder_row->setSpacing(8);

    auto* game_search_edit = new QLineEdit(header_frame);
    game_search_edit->setClearButtonEnabled(true);
    game_search_edit->setMinimumWidth(420);
    game_search_edit->setPlaceholderText(StormLang(
        QStringLiteral("🔍 Поиск игры по названию или Title ID (например: Animal Well, 010020D01AD24000)..."),
        QStringLiteral("🔍 Search game by title or Title ID (e.g. Animal Well, 010020D01AD24000)..."),
        QStringLiteral("🔍 Spiel nach Name oder Title-ID suchen..."),
        QStringLiteral("🔍 Rechercher un jeu par titre ou Title ID..."),
        QStringLiteral("🔍 按游戏名称或 Title ID 搜索..."),
        QStringLiteral("🔍 ゲーム名または Title ID で検索...")));
    search_and_folder_row->addWidget(game_search_edit, 1);

    auto* btn_open_folder = new QPushButton(StormLang(
        QStringLiteral("📂 Папка"),
        QStringLiteral("📂 Folder"),
        QStringLiteral("📂 Ordner"),
        QStringLiteral("📂 Dossier"),
        QStringLiteral("📂 文件夹"),
        QStringLiteral("📂 フォルダ")), header_frame);
    btn_open_folder->setToolTip(StormLang(
        QStringLiteral("Открыть директорию хранения файлов быстрых сохранений (.ses) в Проводнике"),
        QStringLiteral("Open the save states directory (.ses) in File Explorer"),
        QStringLiteral("Speicherstatus-Ordner (.ses) im Explorer öffnen"),
        QStringLiteral("Ouvrir le dossier des sauvegardes rapides (.ses)"),
        QStringLiteral("在资源管理器中打开快速保存目录 (.ses)"),
        QStringLiteral("エクスプローラーでクイックセーブフォルダ (.ses) を開きます")));
    connect(btn_open_folder, &QPushButton::clicked, &dlg, [savestates_dir]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(
            QString::fromStdString(Common::FS::PathToUTF8String(savestates_dir))));
    });
    search_and_folder_row->addWidget(btn_open_folder, 0);
    selector_col->addLayout(search_and_folder_row);

    auto* game_combo = new QComboBox(header_frame);
    game_combo->setMinimumWidth(560);
    game_combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    game_combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    if (game_combo->view()) {
        game_combo->view()->setTextElideMode(Qt::ElideNone);
    }
    game_combo->setToolTip(StormLang(
        QStringLiteral("Выберите игру для просмотра и управления её слотами быстрых сохранений (отображаются полные названия и Title ID)"),
        QStringLiteral("Select a game to inspect and manage its save state slots (shows full titles and Title IDs)"),
        QStringLiteral("Wählen Sie ein Spiel aus, um dessen Speicher-Slots zu verwalten"),
        QStringLiteral("Sélectionnez un jeu pour gérer ses emplacements de sauvegarde"),
        QStringLiteral("选择游戏以查看和管理其快速保存槽位（显示完整名称与 ID）"),
        QStringLiteral("クイックセーブスロットを管理するゲームを選択します（フルネームとIDを表示）")));

    struct GameComboItem {
        u64 tid;
        QString label;
    };
    std::vector<GameComboItem> all_game_items;
    for (u64 tid : title_ids) {
        all_game_items.push_back({tid, resolve_game_title(tid)});
    }
    if (all_game_items.empty()) {
        all_game_items.push_back({0, resolve_game_title(0)});
    }

    auto populate_game_combo = [&](const QString& filter_text) {
        const u64 prev_tid = static_cast<u64>(game_combo->currentData().toULongLong());
        game_combo->blockSignals(true);
        game_combo->clear();
        const QString f = filter_text.trimmed();
        int max_px = 540;
        const QFontMetrics fm(game_combo->font());
        int restore_idx = 0;

        for (const auto& gi : all_game_items) {
            if (!f.isEmpty() && !gi.label.contains(f, Qt::CaseInsensitive)) {
                continue;
            }
            const int cur_idx = game_combo->count();
            game_combo->addItem(gi.label, QVariant::fromValue<qulonglong>(gi.tid));
            max_px = std::max(max_px, fm.horizontalAdvance(gi.label) + 64);
            if (gi.tid == prev_tid && prev_tid != 0) {
                restore_idx = cur_idx;
            }
        }
        if (game_combo->count() == 0) {
            game_combo->addItem(resolve_game_title(0), QVariant::fromValue<qulonglong>(0));
        } else {
            game_combo->setCurrentIndex(restore_idx);
        }
        if (game_combo->view()) {
            game_combo->view()->setMinimumWidth(max_px);
        }
        game_combo->blockSignals(false);
    };

    populate_game_combo(QString());
    selector_col->addWidget(game_combo);
    header_layout->addLayout(selector_col, 1);

    root_layout->addWidget(header_frame);

    // Split View: Left = Wide Slot List (Slots 1..10), Right = Large 16:9 Preview & Action Center
    auto* body_layout = new QHBoxLayout();
    body_layout->setSpacing(16);

    // Left Column
    auto* left_col = new QVBoxLayout();
    left_col->setSpacing(8);

    auto* lbl_slots_header = new QLabel(&dlg);
    lbl_slots_header->setStyleSheet(QStringLiteral("font-size: 10pt; font-weight: 700; color: #38BDF8;"));
    left_col->addWidget(lbl_slots_header);

    auto* slot_list = new QListWidget(&dlg);
    slot_list->setIconSize(QSize(136, 76));
    slot_list->setSpacing(4);
    slot_list->setWordWrap(true);
    slot_list->setMinimumWidth(540);
    slot_list->setToolTip(StormLang(
        QStringLiteral("Двойной щелчок по занятому слоту мгновенно загружает его в запущенную игру"),
        QStringLiteral("Double-click an occupied slot to immediately load it into the running game"),
        QStringLiteral("Doppelklicken Sie auf einen belegten Slot, um ihn sofort im laufenden Spiel zu laden"),
        QStringLiteral("Double-cliquez sur un emplacement occupé pour le charger immédiatement en jeu"),
        QStringLiteral("双击已占用的槽位可立即在运行中的游戏里加载该状态"),
        QStringLiteral("使用中のスロットをダブルクリックすると、実行中のゲームに即座にロードします")));
    left_col->addWidget(slot_list, 1);

    body_layout->addLayout(left_col, 5);

    // Right Column: Preview Card + Metadata + 2-Row Wrapped Note + Management Buttons
    auto* right_frame = new QFrame(&dlg);
    right_frame->setMinimumWidth(560);
    right_frame->setStyleSheet(QStringLiteral(
        "QFrame#RightInspector {"
        "  background-color: rgba(15, 23, 42, 0.82);"
        "  border: 1px solid rgba(56, 189, 248, 0.30);"
        "  border-radius: 12px;"
        "}"));
    right_frame->setObjectName(QStringLiteral("RightInspector"));
    auto* right_layout = new QVBoxLayout(right_frame);
    right_layout->setContentsMargins(16, 12, 16, 12);
    right_layout->setSpacing(8);

    auto* lbl_preview_title = new QLabel(&dlg);
    lbl_preview_title->setWordWrap(true);
    lbl_preview_title->setStyleSheet(QStringLiteral("font-size: 11pt; font-weight: 800; color: #F8FAFC;"));
    right_layout->addWidget(lbl_preview_title);

    auto* preview_box = new QLabel(&dlg);
    preview_box->setFixedSize(508, 286);
    preview_box->setAlignment(Qt::AlignCenter);
    preview_box->setStyleSheet(QStringLiteral(
        "QLabel {"
        "  background-color: #070A10;"
        "  border: 2px solid rgba(0, 229, 255, 0.45);"
        "  border-radius: 12px;"
        "}"));
    right_layout->addWidget(preview_box, 0, Qt::AlignHCenter);

    // Metadata Info Grid
    auto* meta_frame = new QFrame(&dlg);
    meta_frame->setStyleSheet(QStringLiteral(
        "QFrame {"
        "  background-color: rgba(30, 41, 59, 0.55);"
        "  border: 1px solid rgba(148, 163, 184, 0.20);"
        "  border-radius: 8px;"
        "}"));
    auto* meta_layout = new QGridLayout(meta_frame);
    meta_layout->setContentsMargins(12, 6, 12, 6);
    meta_layout->setHorizontalSpacing(16);
    meta_layout->setVerticalSpacing(4);

    auto* lbl_meta_status = new QLabel(&dlg);
    auto* lbl_meta_size = new QLabel(&dlg);
    auto* lbl_meta_date = new QLabel(&dlg);
    auto* lbl_meta_pages = new QLabel(&dlg);
    lbl_meta_status->setStyleSheet(QStringLiteral("font-size: 9.5pt; font-weight: 700; border: none;"));
    lbl_meta_size->setStyleSheet(QStringLiteral("font-size: 9.5pt; color: #E2E8F0; border: none;"));
    lbl_meta_date->setStyleSheet(QStringLiteral("font-size: 9.5pt; color: #CBD5E1; border: none;"));
    lbl_meta_pages->setStyleSheet(QStringLiteral("font-size: 9.5pt; color: #94A3B8; border: none;"));
    meta_layout->addWidget(lbl_meta_status, 0, 0);
    meta_layout->addWidget(lbl_meta_size, 0, 1);
    meta_layout->addWidget(lbl_meta_date, 1, 0);
    meta_layout->addWidget(lbl_meta_pages, 1, 1);
    right_layout->addWidget(meta_frame);

    // Expanded 2-Row Word-Wrapped Custom Note Area
    auto* note_row = new QHBoxLayout();
    note_row->setSpacing(8);
    auto* edit_note = new QPlainTextEdit(&dlg);
    edit_note->setFixedHeight(58);
    edit_note->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    edit_note->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    edit_note->setPlaceholderText(StormLang(
        QStringLiteral("📝 Заметка к слоту в 2 ряда с переносом строк (например: «Перед боссом - 2-й этаж, все ключи собраны»)..."),
        QStringLiteral("📝 2-row wrapped slot note / label (e.g. 'Before Boss - 2nd floor, all keys collected')..."),
        QStringLiteral("📝 2-zeilige Slot-Notiz mit Zeilenumbruch (z. B. 'Vor dem Boss - alle Schlüssel')..."),
        QStringLiteral("📝 Note d'emplacement sur 2 lignes avec retour à la ligne..."),
        QStringLiteral("📝 双行自动换行槽位备注（例如：“Boss 战前 - 已收集所有钥匙”）..."),
        QStringLiteral("📝 2行折り返し対応スロットメモ（例：「ボス戦前 - 全アイテム回収済み」）...")));
    note_row->addWidget(edit_note, 1);

    auto* btn_save_note = new QPushButton(StormLang(
        QStringLiteral("💾 Заметка"),
        QStringLiteral("💾 Save Note"),
        QStringLiteral("💾 Notiz"),
        QStringLiteral("💾 Note"),
        QStringLiteral("💾 保存备注"),
        QStringLiteral("💾 メモ保存")), &dlg);
    btn_save_note->setFixedHeight(58);
    btn_save_note->setToolTip(StormLang(
        QStringLiteral("Сохранить текстовую заметку для выбранного слота"),
        QStringLiteral("Save custom text note for the selected slot"),
        QStringLiteral("Benutzerdefinierte Notiz für den ausgewählten Slot speichern"),
        QStringLiteral("Enregistrer la note personnalisée pour l'emplacement sélectionné"),
        QStringLiteral("为所选槽位保存自定义文本备注"),
        QStringLiteral("選択したスロットのカスタムメモを保存します")));
    note_row->addWidget(btn_save_note);
    right_layout->addLayout(note_row);

    // Primary Action Row (Load & Save)
    auto* primary_row = new QHBoxLayout();
    primary_row->setSpacing(10);

    auto* btn_load_slot = new QPushButton(StormLang(
        QStringLiteral("⚡ Загрузить выбранный слот"),
        QStringLiteral("⚡ Load Selected Slot"),
        QStringLiteral("⚡ Ausgewählten Slot laden"),
        QStringLiteral("⚡ Charger l'emplacement"),
        QStringLiteral("⚡ 加载所选槽位"),
        QStringLiteral("⚡ 選択スロットをロード")), &dlg);
    btn_load_slot->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0284C7, stop:1 #0369A1);"
        "  color: #FFFFFF; border: 1px solid #38BDF8; font-size: 10pt; font-weight: 700; padding: 9px 16px;"
        "}"
        "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0EA5E9, stop:1 #0284C7); }"
        "QPushButton:disabled { background: rgba(15, 23, 42, 0.5); color: #475569; border-color: rgba(71, 85, 105, 0.25); }"));
    btn_load_slot->setToolTip(StormLang(
        QStringLiteral("Мгновенно загрузить состояние из выбранного слота в запущенную игру"),
        QStringLiteral("Immediately load the selected save state slot into the running game"),
        QStringLiteral("Ausgewählten Slot sofort in das laufende Spiel laden"),
        QStringLiteral("Charger immédiatement l'emplacement sélectionné dans le jeu en cours"),
        QStringLiteral("立即将所选快速保存槽位加载到当前运行的游戏中"),
        QStringLiteral("選択したスロットのステートを実行中のゲームに即座にロードします")));
    primary_row->addWidget(btn_load_slot, 1);

    auto* btn_save_slot = new QPushButton(StormLang(
        QStringLiteral("💾 Сохранить игру в этот слот"),
        QStringLiteral("💾 Save Current Game to Slot"),
        QStringLiteral("💾 Spiel in diesen Slot speichern"),
        QStringLiteral("💾 Sauvegarder le jeu ici"),
        QStringLiteral("💾 将当前游戏保存到此槽位"),
        QStringLiteral("💾 現在のゲームをこのスロットに保存")), &dlg);
    btn_save_slot->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #059669, stop:1 #047857);"
        "  color: #FFFFFF; border: 1px solid #34D399; font-size: 10pt; font-weight: 700; padding: 9px 16px;"
        "}"
        "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #10B981, stop:1 #059669); }"
        "QPushButton:disabled { background: rgba(15, 23, 42, 0.5); color: #475569; border-color: rgba(71, 85, 105, 0.25); }"));
    btn_save_slot->setToolTip(StormLang(
        QStringLiteral("Создать мгновенное сохранение запущенной игры и сохранить скриншот-превью в выбранный слот"),
        QStringLiteral("Save the currently running game state and live preview screenshot into the selected slot"),
        QStringLiteral("Aktuelles Spiel samt Vorschaubild in den ausgewählten Slot speichern"),
        QStringLiteral("Sauvegarder le jeu en cours avec capture d'aperçu dans l'emplacement sélectionné"),
        QStringLiteral("将当前运行的游戏状态与实时截图保存到所选槽位"),
        QStringLiteral("実行中のゲーム状態とプレビュー画像をこのスロットに保存します")));
    primary_row->addWidget(btn_save_slot, 1);

    right_layout->addLayout(primary_row);

    // Management Grid (Copy to slot, Swap with slot, Export, Import, Delete slot, Delete all)
    auto* mgmt_grid = new QGridLayout();
    mgmt_grid->setHorizontalSpacing(10);
    mgmt_grid->setVerticalSpacing(8);

    auto* btn_copy_slot = new QPushButton(StormLang(
        QStringLiteral("📋 Копировать в слот..."),
        QStringLiteral("📋 Copy to Another Slot..."),
        QStringLiteral("📋 In anderen Slot kopieren..."),
        QStringLiteral("📋 Copier vers un emplacement..."),
        QStringLiteral("📋 复制到其他槽位..."),
        QStringLiteral("📋 別のスロットにコピー...")), &dlg);
    btn_copy_slot->setToolTip(StormLang(
        QStringLiteral("Скопировать это сохранение вместе с превью и заметкой в другой слот (1-10)"),
        QStringLiteral("Copy this save state, preview image, and note to another slot (1-10)"),
        QStringLiteral("Diesen Speicherstand samt Vorschau und Notiz in einen anderen Slot (1-10) kopieren"),
        QStringLiteral("Copier cette sauvegarde, son aperçu et sa note vers un autre emplacement (1-10)"),
        QStringLiteral("将此保存状态、预览图和备注复制到另一个槽位 (1-10)"),
        QStringLiteral("このセーブステート・プレビュー・メモを別のスロット (1-10) にコピーします")));

    auto* btn_swap_slot = new QPushButton(StormLang(
        QStringLiteral("🔀 Обменять / Переместить..."),
        QStringLiteral("🔀 Swap / Move to Slot..."),
        QStringLiteral("🔀 Mit Slot tauschen / verschieben..."),
        QStringLiteral("🔀 Échanger / Déplacer..."),
        QStringLiteral("🔀 交换 / 移动到槽位..."),
        QStringLiteral("🔀 スロット入れ替え / 移動...")), &dlg);
    btn_swap_slot->setToolTip(StormLang(
        QStringLiteral("Поменять местами или переместить выбранный слот в другой слот (1-10)"),
        QStringLiteral("Swap or move the selected slot with another slot (1-10)"),
        QStringLiteral("Ausgewählten Slot mit einem anderen Slot (1-10) tauschen oder verschieben"),
        QStringLiteral("Échanger ou déplacer l'emplacement sélectionné avec un autre (1-10)"),
        QStringLiteral("将所选槽位与另一个槽位 (1-10) 交换或移动"),
        QStringLiteral("選択したスロットを別のスロット (1-10) と入れ替えまたは移動します")));

    auto* btn_export_slot = new QPushButton(StormLang(
        QStringLiteral("📤 Экспорт файла .ses..."),
        QStringLiteral("📤 Export .ses File..."),
        QStringLiteral("📤 .ses-Datei exportieren..."),
        QStringLiteral("📤 Exporter le fichier .ses..."),
        QStringLiteral("📤 导出 .ses 文件..."),
        QStringLiteral("📤 .ses ファイルをエクスポート...")), &dlg);
    btn_export_slot->setToolTip(StormLang(
        QStringLiteral("Экспортировать файл сохранения (.ses) в любую внешнюю папку для резервной копии"),
        QStringLiteral("Export the save state (.ses) file to an external folder for backup or sharing"),
        QStringLiteral("Speicherstand (.ses) als Backup in einen beliebigen Ordner exportieren"),
        QStringLiteral("Exporter le fichier de sauvegarde (.ses) vers un dossier externe"),
        QStringLiteral("将快速保存文件 (.ses) 导出到外部文件夹进行备份"),
        QStringLiteral("バックアップ用にセーブステート (.ses) を外部フォルダへエクスポートします")));

    auto* btn_import_slot = new QPushButton(StormLang(
        QStringLiteral("📥 Импорт файла .ses в слот..."),
        QStringLiteral("📥 Import .ses into Slot..."),
        QStringLiteral("📥 .ses in Slot importieren..."),
        QStringLiteral("📥 Importer un .ses ici..."),
        QStringLiteral("📥 导入 .ses 到此槽位..."),
        QStringLiteral("📥 このスロットに .ses をインポート...")), &dlg);
    btn_import_slot->setToolTip(StormLang(
        QStringLiteral("Импортировать внешний файл .ses в выбранный слот"),
        QStringLiteral("Import an external .ses save state file into the selected slot"),
        QStringLiteral("Externe .ses-Datei in den ausgewählten Slot importieren"),
        QStringLiteral("Importer un fichier .ses externe dans l'emplacement sélectionné"),
        QStringLiteral("将外部 .ses 快速保存文件导入到当前选中的槽位"),
        QStringLiteral("外部の .ses ファイルをこのスロットにインポートします")));

    auto* btn_delete_slot = new QPushButton(StormLang(
        QStringLiteral("🗑️ Удалить из слота"),
        QStringLiteral("🗑️ Delete Slot"),
        QStringLiteral("🗑️ Slot löschen"),
        QStringLiteral("🗑️ Supprimer l'emplacement"),
        QStringLiteral("🗑️ 删除此槽位"),
        QStringLiteral("🗑️ このスロットを削除")), &dlg);
    btn_delete_slot->setStyleSheet(QStringLiteral(
        "QPushButton { color: #FCA5A5; border-color: rgba(239, 68, 68, 0.45); }"
        "QPushButton:hover { background-color: rgba(185, 28, 28, 0.45); color: #FFFFFF; border-color: #EF4444; }"
        "QPushButton:disabled { color: #475569; border-color: rgba(71, 85, 105, 0.2); }"));
    btn_delete_slot->setToolTip(StormLang(
        QStringLiteral("Удалить файл сохранения, превью и заметку из выбранного слота"),
        QStringLiteral("Delete the save state file, preview screenshot, and note from the selected slot"),
        QStringLiteral("Speicherstand, Vorschaubild und Notiz aus dem ausgewählten Slot löschen"),
        QStringLiteral("Supprimer la sauvegarde, l'aperçu et la note de l'emplacement sélectionné"),
        QStringLiteral("删除所选槽位中的快速保存文件、预览图和备注"),
        QStringLiteral("選択したスロットのセーブデータ・プレビュー・メモを削除します")));

    auto* btn_clear_all = new QPushButton(StormLang(
        QStringLiteral("🧹 Очистить все слоты игры"),
        QStringLiteral("🧹 Clear All Game Slots"),
        QStringLiteral("🧹 Alle Slots des Spiels leeren"),
        QStringLiteral("🧹 Vider tous les emplacements"),
        QStringLiteral("🧹 清空该游戏所有槽位"),
        QStringLiteral("🧹 このゲームの全スロットを消去")), &dlg);
    btn_clear_all->setStyleSheet(QStringLiteral(
        "QPushButton { color: #FCD34D; border-color: rgba(245, 158, 11, 0.45); }"
        "QPushButton:hover { background-color: rgba(180, 83, 9, 0.40); color: #FFFFFF; border-color: #F59E0B; }"
        "QPushButton:disabled { color: #475569; border-color: rgba(71, 85, 105, 0.2); }"));
    btn_clear_all->setToolTip(StormLang(
        QStringLiteral("Удалить все быстрые сохранения (слоты 1-10) для выбранной игры"),
        QStringLiteral("Delete all save state slots (1-10) for the currently selected game"),
        QStringLiteral("Alle Speicher-Slots (1-10) für das ausgewählte Spiel löschen"),
        QStringLiteral("Supprimer tous les emplacements (1-10) pour le jeu sélectionné"),
        QStringLiteral("删除当前所选游戏的所有快速保存槽位 (1-10)"),
        QStringLiteral("選択されているゲームの全スロット (1-10) を一括削除します")));

    mgmt_grid->addWidget(btn_copy_slot, 0, 0);
    mgmt_grid->addWidget(btn_swap_slot, 0, 1);
    mgmt_grid->addWidget(btn_export_slot, 1, 0);
    mgmt_grid->addWidget(btn_import_slot, 1, 1);
    mgmt_grid->addWidget(btn_delete_slot, 2, 0);
    mgmt_grid->addWidget(btn_clear_all, 2, 1);
    right_layout->addLayout(mgmt_grid);

    // Bottom status bar + Close button
    auto* footer_row = new QHBoxLayout();
    auto* lbl_status_feedback = new QLabel(&dlg);
    lbl_status_feedback->setStyleSheet(QStringLiteral("font-size: 9.5pt; font-weight: 700; color: #34D399;"));
    footer_row->addWidget(lbl_status_feedback, 1);

    auto* btn_close = new QPushButton(StormLang(
        QStringLiteral("Закрыть"),
        QStringLiteral("Close"),
        QStringLiteral("Schließen"),
        QStringLiteral("Fermer"),
        QStringLiteral("关闭"),
        QStringLiteral("閉じる")), &dlg);
    connect(btn_close, &QPushButton::clicked, &dlg, &QDialog::accept);
    footer_row->addWidget(btn_close);
    right_layout->addLayout(footer_row);

    body_layout->addWidget(right_frame, 5);
    root_layout->addLayout(body_layout, 1);

    auto get_selected_tid = [game_combo]() -> u64 {
        return static_cast<u64>(game_combo->currentData().toULongLong());
    };

    auto get_selected_slot = [slot_list]() -> int {
        const int row = slot_list->currentRow();
        return (row >= 0 && row < kMaxSlots) ? (row + 1) : 1;
    };

    auto read_slot_note = [](const std::filesystem::path& ses_path) -> QString {
        auto note_path = ses_path;
        note_path.replace_extension(".txt");
        QFile f(QString::fromStdString(Common::FS::PathToUTF8String(note_path)));
        if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString::fromUtf8(f.readAll()).trimmed();
        }
        return {};
    };

    auto write_slot_note = [](const std::filesystem::path& ses_path, const QString& text) {
        auto note_path = ses_path;
        note_path.replace_extension(".txt");
        QFile f(QString::fromStdString(Common::FS::PathToUTF8String(note_path)));
        if (text.trimmed().isEmpty()) {
            if (f.exists()) f.remove();
            return;
        }
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            f.write(text.trimmed().toUtf8());
        }
    };

    auto format_note_two_rows = [](const QString& raw_note) -> QString {
        QString clean = raw_note.trimmed();
        if (clean.isEmpty()) return {};
        QStringList lines = clean.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        if (lines.size() == 1 && lines[0].length() > 54) {
            int split_pos = lines[0].lastIndexOf(QLatin1Char(' '), 54);
            if (split_pos <= 15) split_pos = 54;
            const QString first_line = lines[0].left(split_pos).trimmed();
            const QString second_line = lines[0].mid(split_pos).trimmed();
            return first_line + QStringLiteral("\n   ") + second_line;
        } else if (lines.size() >= 2) {
            return lines[0].trimmed() + QStringLiteral("\n   ") + lines[1].trimmed();
        }
        return clean;
    };

    auto refresh_all = [&, kMaxSlots]() {
        const u64 tid = get_selected_tid();
        const int prev_row = std::clamp(slot_list->currentRow(), 0, kMaxSlots - 1);
        slot_list->blockSignals(true);
        slot_list->clear();

        int occupied_count = 0;
        double total_mb = 0.0;

        for (int s = 1; s <= kMaxSlots; ++s) {
            const auto ses_path = (tid != 0) ? Core::GetSaveStatePath(tid, static_cast<u32>(s))
                                             : std::filesystem::path{};
            Core::SaveStateHeader hdr{};
            std::error_code ec;
            const bool exists = (tid != 0) && std::filesystem::exists(ses_path, ec) &&
                                Core::GetSaveStateInfo(ses_path, hdr);

            double mb = 0.0;
            QString dt_str;
            QString note_str;
            if (exists) {
                ++occupied_count;
                const auto fsize = std::filesystem::file_size(ses_path, ec);
                mb = static_cast<double>(fsize) / (1024.0 * 1024.0);
                total_mb += mb;
                QFileInfo fi(QString::fromStdString(Common::FS::PathToUTF8String(ses_path)));
                dt_str = (hdr.timestamp > 0)
                             ? QDateTime::fromSecsSinceEpoch(static_cast<qint64>(hdr.timestamp))
                                   .toString(QStringLiteral("dd.MM.yyyy HH:mm:ss"))
                             : fi.lastModified().toString(QStringLiteral("dd.MM.yyyy HH:mm:ss"));
                note_str = read_slot_note(ses_path);
            }

            QPixmap thumb_px;
            if (exists) {
                auto png_path = ses_path;
                png_path.replace_extension(".png");
                if (std::filesystem::exists(png_path, ec)) {
                    thumb_px.load(QString::fromStdString(Common::FS::PathToUTF8String(png_path)));
                }
            }
            if (thumb_px.isNull()) {
                thumb_px = make_placeholder_preview(tid, s, exists, hdr, mb, dt_str, note_str);
            }

            const QString slot_badge = (s == 1)
                ? StormLang(QStringLiteral("Слот 1  [Быстрое сохранение F5 / F7]"),
                            QStringLiteral("Slot 1  [Quick Save F5 / F7]"),
                            QStringLiteral("Slot 1  [Schnellspeichern F5 / F7]"),
                            QStringLiteral("Emplacement 1  [Sauvegarde rapide F5 / F7]"),
                            QStringLiteral("槽位 1  [快速保存 F5 / F7]"),
                            QStringLiteral("スロット 1  [クイックセーブ F5 / F7]"))
                : StormLang(QStringLiteral("Слот %1").arg(s),
                            QStringLiteral("Slot %1").arg(s),
                            QStringLiteral("Slot %1").arg(s),
                            QStringLiteral("Emplacement %1").arg(s),
                            QStringLiteral("槽位 %1").arg(s),
                            QStringLiteral("スロット %1").arg(s));

            QString item_text = slot_badge + QStringLiteral("\n");
            if (exists) {
                item_text += tr("✅ %1 МБ  -  🕒 %2  -  %3 стр. 4КБ")
                                 .arg(QString::number(mb, 'f', 2), dt_str)
                                 .arg(hdr.num_stored_pages);
                if (!note_str.isEmpty()) {
                    item_text += QStringLiteral("\n📝 ") + format_note_two_rows(note_str);
                }
            } else {
                item_text += StormLang(
                    QStringLiteral("⚪ [Пусто - доступно для сохранения или копирования]"),
                    QStringLiteral("⚪ [Empty - available for saving or copying]"),
                    QStringLiteral("⚪ [Leer - verfügbar zum Speichern oder Kopieren]"),
                    QStringLiteral("⚪ [Vide - disponible pour sauvegarder ou copier]"),
                    QStringLiteral("⚪ [空 - 可用于保存或复制]"),
                    QStringLiteral("⚪ [空 - 保存またはコピー可能]"));
            }

            auto* item = new QListWidgetItem(
                QIcon(thumb_px.scaled(136, 76, Qt::KeepAspectRatio, Qt::SmoothTransformation)),
                item_text);
            item->setSizeHint(QSize(500, !note_str.isEmpty() ? 104 : 88));
            slot_list->addItem(item);
        }

        lbl_slots_header->setText(StormLang(
            QStringLiteral("Слоты сохранений (занято: %1 из %2 - общий объём: %3 МБ):")
                .arg(occupied_count).arg(kMaxSlots).arg(QString::number(total_mb, 'f', 2)),
            QStringLiteral("Save State Slots (used: %1 of %2 - total size: %3 MB):")
                .arg(occupied_count).arg(kMaxSlots).arg(QString::number(total_mb, 'f', 2)),
            QStringLiteral("Speicher-Slots (belegt: %1 von %2 - Gesamtgröße: %3 MB):")
                .arg(occupied_count).arg(kMaxSlots).arg(QString::number(total_mb, 'f', 2)),
            QStringLiteral("Emplacements (utilisés : %1 sur %2 - taille totale : %3 Mo) :")
                .arg(occupied_count).arg(kMaxSlots).arg(QString::number(total_mb, 'f', 2)),
            QStringLiteral("保存槽位（已用：%1 / %2 - 总大小：%3 MB）：")
                .arg(occupied_count).arg(kMaxSlots).arg(QString::number(total_mb, 'f', 2)),
            QStringLiteral("セーブスロット（使用中: %1 / %2 - 合計サイズ: %3 MB）:")
                .arg(occupied_count).arg(kMaxSlots).arg(QString::number(total_mb, 'f', 2))));

        btn_clear_all->setEnabled(occupied_count > 0);
        slot_list->setCurrentRow(prev_row);
        slot_list->blockSignals(false);
    };

    auto update_inspector = [&]() {
        const u64 tid = get_selected_tid();
        const int s = get_selected_slot();
        const auto ses_path = (tid != 0) ? Core::GetSaveStatePath(tid, static_cast<u32>(s))
                                         : std::filesystem::path{};
        Core::SaveStateHeader hdr{};
        std::error_code ec;
        const bool exists = (tid != 0) && std::filesystem::exists(ses_path, ec) &&
                            Core::GetSaveStateInfo(ses_path, hdr);

        double mb = 0.0;
        QString dt_str = QStringLiteral("-");
        QString note_str;
        if (exists) {
            const auto fsize = std::filesystem::file_size(ses_path, ec);
            mb = static_cast<double>(fsize) / (1024.0 * 1024.0);
            QFileInfo fi(QString::fromStdString(Common::FS::PathToUTF8String(ses_path)));
            dt_str = (hdr.timestamp > 0)
                         ? QDateTime::fromSecsSinceEpoch(static_cast<qint64>(hdr.timestamp))
                               .toString(QStringLiteral("dd.MM.yyyy HH:mm:ss"))
                         : fi.lastModified().toString(QStringLiteral("dd.MM.yyyy HH:mm:ss"));
            note_str = read_slot_note(ses_path);
        }

        lbl_preview_title->setText(StormLang(
            QStringLiteral("🎯 Слот #%1 - %2").arg(s).arg(resolve_game_title(tid)),
            QStringLiteral("🎯 Slot #%1 - %2").arg(s).arg(resolve_game_title(tid)),
            QStringLiteral("🎯 Slot #%1 - %2").arg(s).arg(resolve_game_title(tid)),
            QStringLiteral("🎯 Emplacement #%1 - %2").arg(s).arg(resolve_game_title(tid)),
            QStringLiteral("🎯 槽位 #%1 - %2").arg(s).arg(resolve_game_title(tid)),
            QStringLiteral("🎯 スロット #%1 - %2").arg(s).arg(resolve_game_title(tid))));

        QPixmap large_px;
        if (exists) {
            auto png_path = ses_path;
            png_path.replace_extension(".png");
            if (std::filesystem::exists(png_path, ec)) {
                large_px.load(QString::fromStdString(Common::FS::PathToUTF8String(png_path)));
            }
        }
        if (large_px.isNull()) {
            large_px = make_placeholder_preview(tid, s, exists, hdr, mb, dt_str, note_str);
        }
        preview_box->setPixmap(large_px.scaled(504, 282, Qt::KeepAspectRatio, Qt::SmoothTransformation));

        if (exists) {
            lbl_meta_status->setText(StormLang(
                QStringLiteral("🟢 Статус: Готов (SESS v%1)").arg(hdr.version),
                QStringLiteral("🟢 Status: Ready (SESS v%1)").arg(hdr.version),
                QStringLiteral("🟢 Status: Bereit (SESS v%1)").arg(hdr.version),
                QStringLiteral("🟢 Statut : Prêt (SESS v%1)").arg(hdr.version),
                QStringLiteral("🟢 状态：就绪 (SESS v%1)").arg(hdr.version),
                QStringLiteral("🟢 状態: 準備完了 (SESS v%1)").arg(hdr.version)));
            lbl_meta_status->setStyleSheet(QStringLiteral("font-size: 9.5pt; font-weight: 700; color: #34D399; border: none;"));
            lbl_meta_size->setText(StormLang(
                QStringLiteral("📦 Размер: %1 МБ").arg(QString::number(mb, 'f', 2)),
                QStringLiteral("📦 Size: %1 MB").arg(QString::number(mb, 'f', 2)),
                QStringLiteral("📦 Größe: %1 MB").arg(QString::number(mb, 'f', 2)),
                QStringLiteral("📦 Taille : %1 Mo").arg(QString::number(mb, 'f', 2)),
                QStringLiteral("📦 大小：%1 MB").arg(QString::number(mb, 'f', 2)),
                QStringLiteral("📦 サイズ: %1 MB").arg(QString::number(mb, 'f', 2))));
            lbl_meta_date->setText(StormLang(
                QStringLiteral("🕒 Создан: %1").arg(dt_str),
                QStringLiteral("🕒 Created: %1").arg(dt_str),
                QStringLiteral("🕒 Erstellt: %1").arg(dt_str),
                QStringLiteral("🕒 Créé : %1").arg(dt_str),
                QStringLiteral("🕒 创建时间：%1").arg(dt_str),
                QStringLiteral("🕒 作成日時: %1").arg(dt_str)));
            lbl_meta_pages->setText(StormLang(
                QStringLiteral("🧩 Страниц 4 КБ: %1 - Потоков: %2").arg(hdr.num_stored_pages).arg(hdr.num_threads),
                QStringLiteral("🧩 4KB Pages: %1 - Threads: %2").arg(hdr.num_stored_pages).arg(hdr.num_threads),
                QStringLiteral("🧩 4KB-Seiten: %1 - Threads: %2").arg(hdr.num_stored_pages).arg(hdr.num_threads),
                QStringLiteral("🧩 Pages 4 Ko : %1 - Threads : %2").arg(hdr.num_stored_pages).arg(hdr.num_threads),
                QStringLiteral("🧩 4KB 页面：%1 - 线程：%2").arg(hdr.num_stored_pages).arg(hdr.num_threads),
                QStringLiteral("🧩 4KBページ: %1 - スレッド: %2").arg(hdr.num_stored_pages).arg(hdr.num_threads)));
        } else {
            lbl_meta_status->setText(StormLang(
                QStringLiteral("⚪ Статус: Слот пуст"),
                QStringLiteral("⚪ Status: Slot is Empty"),
                QStringLiteral("⚪ Status: Slot ist leer"),
                QStringLiteral("⚪ Statut : Emplacement vide"),
                QStringLiteral("⚪ 状态：槽位为空"),
                QStringLiteral("⚪ 状態: 空きスロット")));
            lbl_meta_status->setStyleSheet(QStringLiteral("font-size: 9.5pt; font-weight: 700; color: #94A3B8; border: none;"));
            lbl_meta_size->setText(QStringLiteral("📦 -"));
            lbl_meta_date->setText(QStringLiteral("🕒 -"));
            lbl_meta_pages->setText(QStringLiteral("🧩 -"));
        }

        edit_note->setPlainText(note_str);
        edit_note->setEnabled(tid != 0);
        btn_save_note->setEnabled(tid != 0);

        const bool game_matches_running = emulation_running && QtCommon::system &&
                                          QtCommon::system->IsPoweredOn() &&
                                          (active_tid != 0 && active_tid == tid);
        btn_load_slot->setEnabled(exists && game_matches_running);
        btn_save_slot->setEnabled(game_matches_running);
        btn_copy_slot->setEnabled(exists);
        btn_swap_slot->setEnabled(exists);
        btn_export_slot->setEnabled(exists);
        btn_import_slot->setEnabled(tid != 0);
        btn_delete_slot->setEnabled(exists);
    };

    connect(game_search_edit, &QLineEdit::textChanged, &dlg, [&](const QString& txt) {
        populate_game_combo(txt);
        refresh_all();
        update_inspector();
    });

    connect(game_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), &dlg, [&]() {
        refresh_all();
        update_inspector();
    });

    connect(slot_list, &QListWidget::currentRowChanged, &dlg, [&](int) {
        update_inspector();
    });

    connect(slot_list, &QListWidget::itemDoubleClicked, &dlg, [&](QListWidgetItem*) {
        if (btn_load_slot->isEnabled()) {
            btn_load_slot->click();
        }
    });

    connect(btn_save_note, &QPushButton::clicked, &dlg, [&]() {
        const u64 tid = get_selected_tid();
        const int s = get_selected_slot();
        if (tid == 0) return;
        const auto ses_path = Core::GetSaveStatePath(tid, static_cast<u32>(s));
        write_slot_note(ses_path, edit_note->toPlainText());
        refresh_all();
        update_inspector();
        lbl_status_feedback->setText(StormLang(
            QStringLiteral("✅ Заметка для Слота %1 сохранена").arg(s),
            QStringLiteral("✅ Note saved for Slot %1").arg(s),
            QStringLiteral("✅ Notiz für Slot %1 gespeichert").arg(s),
            QStringLiteral("✅ Note enregistrée pour l'emplacement %1").arg(s),
            QStringLiteral("✅ 槽位 %1 的备注已保存").arg(s),
            QStringLiteral("✅ スロット %1 のメモを保存しました").arg(s)));
    });

    connect(btn_load_slot, &QPushButton::clicked, &dlg, [&]() {
        const int s = get_selected_slot();
        dlg.accept();
        OnLoadStateSlot(s);
    });

    connect(btn_save_slot, &QPushButton::clicked, &dlg, [&]() {
        const int s = get_selected_slot();
        const u64 tid = get_selected_tid();
        OnSaveStateSlot(s);
        if (!edit_note->toPlainText().trimmed().isEmpty() && tid != 0) {
            write_slot_note(Core::GetSaveStatePath(tid, static_cast<u32>(s)), edit_note->toPlainText());
        }
        Core::FlushPendingSaveStates();
        refresh_all();
        update_inspector();
        QTimer::singleShot(250, &dlg, [&]() {
            refresh_all();
            update_inspector();
            lbl_status_feedback->setText(StormLang(
                QStringLiteral("⚡ Состояние игры и превью сохранены в Слот %1!").arg(get_selected_slot()),
                QStringLiteral("⚡ Game state & preview saved to Slot %1!").arg(get_selected_slot()),
                QStringLiteral("⚡ Spielstatus & Vorschau in Slot %1 gespeichert!").arg(get_selected_slot()),
                QStringLiteral("⚡ État du jeu et aperçu enregistrés dans l'emplacement %1 !").arg(get_selected_slot()),
                QStringLiteral("⚡ 游戏状态与预览图已保存到槽位 %1！").arg(get_selected_slot()),
                QStringLiteral("⚡ スロット %1 にゲーム状態とプレビューを保存しました！").arg(get_selected_slot())));
        });
    });

    auto ask_target_slot = [&](int src_slot, const QString& title_text) -> int {
        QStringList items;
        const u64 tid = get_selected_tid();
        for (int i = 1; i <= kMaxSlots; ++i) {
            if (i == src_slot) continue;
            const auto p = Core::GetSaveStatePath(tid, static_cast<u32>(i));
            std::error_code ec;
            const bool occ = std::filesystem::exists(p, ec);
            items << (occ
                ? StormLang(QStringLiteral("Слот %1 (Занят - перезаписать/обменять)").arg(i),
                            QStringLiteral("Slot %1 (Occupied)").arg(i),
                            QStringLiteral("Slot %1 (Belegt)").arg(i),
                            QStringLiteral("Emplacement %1 (Occupé)").arg(i),
                            QStringLiteral("槽位 %1（已占用）").arg(i),
                            QStringLiteral("スロット %1（使用中）").arg(i))
                : StormLang(QStringLiteral("Слот %1 (Пусто)").arg(i),
                            QStringLiteral("Slot %1 (Empty)").arg(i),
                            QStringLiteral("Slot %1 (Leer)").arg(i),
                            QStringLiteral("Emplacement %1 (Vide)").arg(i),
                            QStringLiteral("槽位 %1（空）").arg(i),
                            QStringLiteral("スロット %1（空）").arg(i)));
        }
        bool ok = false;
        const QString chosen = QInputDialog::getItem(
            &dlg, title_text,
            StormLang(QStringLiteral("Выберите целевой слот (из Слота %1):").arg(src_slot),
                      QStringLiteral("Select target slot (from Slot %1):").arg(src_slot),
                      QStringLiteral("Ziel-Slot auswählen (von Slot %1):").arg(src_slot),
                      QStringLiteral("Sélectionnez l'emplacement cible (depuis %1) :").arg(src_slot),
                      QStringLiteral("选择目标槽位（来自槽位 %1）：").arg(src_slot),
                      QStringLiteral("対象スロットを選択（スロット %1 から）:").arg(src_slot)),
            items, 0, false, &ok);
        if (!ok || chosen.isEmpty()) return 0;
        static const QRegularExpression num_re(QStringLiteral(R"((\d+))"));
        const auto m = num_re.match(chosen);
        return m.hasMatch() ? m.captured(1).toInt() : 0;
    };

    connect(btn_copy_slot, &QPushButton::clicked, &dlg, [&]() {
        const u64 tid = get_selected_tid();
        const int src = get_selected_slot();
        if (tid == 0) return;
        const int dst = ask_target_slot(src, StormLang(
            QStringLiteral("Копирование слота"), QStringLiteral("Copy Slot"),
            QStringLiteral("Slot kopieren"), QStringLiteral("Copier l'emplacement"),
            QStringLiteral("复制槽位"), QStringLiteral("スロットのコピー")));
        if (dst <= 0 || dst == src) return;

        Core::FlushPendingSaveStates();
        const auto src_ses = Core::GetSaveStatePath(tid, static_cast<u32>(src));
        const auto dst_ses = Core::GetSaveStatePath(tid, static_cast<u32>(dst));
        std::error_code ec;
        std::filesystem::copy_file(src_ses, dst_ses, std::filesystem::copy_options::overwrite_existing, ec);

        for (const char* ext : {".png", ".txt"}) {
            auto sp = src_ses; sp.replace_extension(ext);
            auto dp = dst_ses; dp.replace_extension(ext);
            if (std::filesystem::exists(sp, ec)) {
                std::filesystem::copy_file(sp, dp, std::filesystem::copy_options::overwrite_existing, ec);
            } else if (std::filesystem::exists(dp, ec)) {
                std::filesystem::remove(dp, ec);
            }
        }
        slot_list->setCurrentRow(dst - 1);
        refresh_all();
        update_inspector();
        RefreshSaveStateMenus();
        lbl_status_feedback->setText(StormLang(
            QStringLiteral("📋 Слот %1 успешно скопирован в Слот %2!").arg(src).arg(dst),
            QStringLiteral("📋 Slot %1 copied to Slot %2!").arg(src).arg(dst),
            QStringLiteral("📋 Slot %1 in Slot %2 kopiert!").arg(src).arg(dst),
            QStringLiteral("📋 Emplacement %1 copié vers %2 !").arg(src).arg(dst),
            QStringLiteral("📋 槽位 %1 已复制到槽位 %2！").arg(src).arg(dst),
            QStringLiteral("📋 スロット %1 をスロット %2 にコピーしました！").arg(src).arg(dst)));
    });

    connect(btn_swap_slot, &QPushButton::clicked, &dlg, [&]() {
        const u64 tid = get_selected_tid();
        const int src = get_selected_slot();
        if (tid == 0) return;
        const int dst = ask_target_slot(src, StormLang(
            QStringLiteral("Обмен / Перемещение слота"), QStringLiteral("Swap / Move Slot"),
            QStringLiteral("Slot tauschen / verschieben"), QStringLiteral("Échanger / Déplacer"),
            QStringLiteral("交换 / 移动槽位"), QStringLiteral("スロットの入れ替え・移動")));
        if (dst <= 0 || dst == src) return;

        Core::FlushPendingSaveStates();
        const auto src_ses = Core::GetSaveStatePath(tid, static_cast<u32>(src));
        const auto dst_ses = Core::GetSaveStatePath(tid, static_cast<u32>(dst));
        std::error_code ec;

        for (const char* ext : {".ses", ".png", ".txt"}) {
            auto sp = src_ses; sp.replace_extension(ext);
            auto dp = dst_ses; dp.replace_extension(ext);
            auto tp = src_ses; tp.replace_extension(std::string(ext) + ".tmp_swap");
            std::filesystem::remove(tp, ec);
            const bool has_s = std::filesystem::exists(sp, ec);
            const bool has_d = std::filesystem::exists(dp, ec);
            if (has_s && has_d) {
                std::filesystem::rename(sp, tp, ec);
                std::filesystem::rename(dp, sp, ec);
                std::filesystem::rename(tp, dp, ec);
            } else if (has_s) {
                std::filesystem::rename(sp, dp, ec);
            } else if (has_d) {
                std::filesystem::rename(dp, sp, ec);
            }
        }
        slot_list->setCurrentRow(dst - 1);
        refresh_all();
        update_inspector();
        RefreshSaveStateMenus();
        lbl_status_feedback->setText(StormLang(
            QStringLiteral("🔀 Слот %1 и Слот %2 успешно обменялись местами!").arg(src).arg(dst),
            QStringLiteral("🔀 Slot %1 and Slot %2 swapped!").arg(src).arg(dst),
            QStringLiteral("🔀 Slot %1 und Slot %2 getauscht!").arg(src).arg(dst),
            QStringLiteral("🔀 Emplacements %1 et %2 échangés !").arg(src).arg(dst),
            QStringLiteral("🔀 槽位 %1 与槽位 %2 已交换！").arg(src).arg(dst),
            QStringLiteral("🔀 スロット %1 とスロット %2 を入れ替えました！").arg(src).arg(dst)));
    });

    connect(btn_export_slot, &QPushButton::clicked, &dlg, [&]() {
        const u64 tid = get_selected_tid();
        const int s = get_selected_slot();
        if (tid == 0) return;
        const auto src_ses = Core::GetSaveStatePath(tid, static_cast<u32>(s));
        const QString default_name = QStringLiteral("%1_slot%2.ses")
                                         .arg(tid, 16, 16, QLatin1Char('0')).toUpper().arg(s);
        const QString target = QFileDialog::getSaveFileName(
            &dlg,
            StormLang(QStringLiteral("Экспорт быстрого сохранения SESS v2"),
                      QStringLiteral("Export SESS v2 Save State"),
                      QStringLiteral("SESS v2 Speicherstand exportieren"),
                      QStringLiteral("Exporter la sauvegarde SESS v2"),
                      QStringLiteral("导出 SESS v2 快速保存"),
                      QStringLiteral("SESS v2 クイックセーブをエクスポート")),
            default_name,
            QStringLiteral("STORM Save State (*.ses)"));
        if (target.isEmpty()) return;
        Core::FlushPendingSaveStates();
        std::error_code ec;
        std::filesystem::copy_file(src_ses, Common::FS::ToU8String(target.toStdString()),
                                   std::filesystem::copy_options::overwrite_existing, ec);
        if (!ec) {
            lbl_status_feedback->setText(StormLang(
                QStringLiteral("📤 Слот %1 экспортирован!").arg(s),
                QStringLiteral("📤 Slot %1 exported!").arg(s),
                QStringLiteral("📤 Slot %1 exportiert!").arg(s),
                QStringLiteral("📤 Emplacement %1 exporté !").arg(s),
                QStringLiteral("📤 槽位 %1 已导出！").arg(s),
                QStringLiteral("📤 スロット %1 をエクスポートしました！").arg(s)));
        }
    });

    connect(btn_import_slot, &QPushButton::clicked, &dlg, [&]() {
        const u64 tid = get_selected_tid();
        const int s = get_selected_slot();
        if (tid == 0) return;
        const QString source = QFileDialog::getOpenFileName(
            &dlg,
            StormLang(QStringLiteral("Импорт быстрого сохранения SESS v2 в Слот %1").arg(s),
                      QStringLiteral("Import SESS v2 Save State into Slot %1").arg(s),
                      QStringLiteral("SESS v2 in Slot %1 importieren").arg(s),
                      QStringLiteral("Importer une sauvegarde SESS v2 dans l'emplacement %1").arg(s),
                      QStringLiteral("将 SESS v2 导入到槽位 %1").arg(s),
                      QStringLiteral("スロット %1 に SESS v2 をインポート").arg(s)),
            QString(),
            QStringLiteral("STORM Save State (*.ses)"));
        if (source.isEmpty()) return;
        const std::filesystem::path src_p(Common::FS::ToU8String(source.toStdString()));
        Core::SaveStateHeader hdr{};
        if (!Core::GetSaveStateInfo(src_p, hdr)) {
            QMessageBox::warning(&dlg, QStringLiteral("SESS v2"),
                                 StormLang(QStringLiteral("Выбранный файл не является корректным сохранением SESS v2."),
                                           QStringLiteral("The selected file is not a valid SESS v2 save state."),
                                           QStringLiteral("Die ausgewählte Datei ist kein gültiger SESS v2-Speicherstand."),
                                           QStringLiteral("Le fichier sélectionné n'est pas une sauvegarde SESS v2 valide."),
                                           QStringLiteral("所选文件不是有效的 SESS v2 快速保存文件。"),
                                           QStringLiteral("選択されたファイルは有効な SESS v2 セーブデータではありません。")));
            return;
        }
        const auto dst_ses = Core::GetSaveStatePath(tid, static_cast<u32>(s));
        std::error_code ec;
        std::filesystem::copy_file(src_p, dst_ses, std::filesystem::copy_options::overwrite_existing, ec);
        refresh_all();
        update_inspector();
        RefreshSaveStateMenus();
        lbl_status_feedback->setText(StormLang(
            QStringLiteral("📥 Файл успешно импортирован в Слот %1!").arg(s),
            QStringLiteral("📥 File imported into Slot %1!").arg(s),
            QStringLiteral("📥 Datei in Slot %1 importiert!").arg(s),
            QStringLiteral("📥 Fichier importé dans l'emplacement %1 !").arg(s),
            QStringLiteral("📥 文件已导入到槽位 %1！").arg(s),
            QStringLiteral("📥 スロット %1 にインポートしました！").arg(s)));
    });

    connect(btn_delete_slot, &QPushButton::clicked, &dlg, [&]() {
        const u64 tid = get_selected_tid();
        const int s = get_selected_slot();
        if (tid == 0) return;
        const auto ans = QMessageBox::question(
            &dlg,
            StormLang(QStringLiteral("Удаление сохранения"), QStringLiteral("Delete Save State"),
                      QStringLiteral("Speicherstand löschen"), QStringLiteral("Supprimer la sauvegarde"),
                      QStringLiteral("删除快速保存"), QStringLiteral("セーブステートの削除")),
            StormLang(QStringLiteral("Вы действительно хотите удалить быстрое сохранение из Слота %1?").arg(s),
                      QStringLiteral("Are you sure you want to delete the save state in Slot %1?").arg(s),
                      QStringLiteral("Möchten Sie den Speicherstand in Slot %1 wirklich löschen?").arg(s),
                      QStringLiteral("Voulez-vous vraiment supprimer la sauvegarde de l'emplacement %1 ?").arg(s),
                      QStringLiteral("确定要删除槽位 %1 中的快速保存吗？").arg(s),
                      QStringLiteral("スロット %1 のクイックセーブを削除してもよろしいですか？").arg(s)));
        if (ans != QMessageBox::Yes) return;

        Core::FlushPendingSaveStates();
        const auto ses_path = Core::GetSaveStatePath(tid, static_cast<u32>(s));
        std::error_code ec;
        for (const char* ext : {".ses", ".png", ".txt"}) {
            auto p = ses_path; p.replace_extension(ext);
            std::filesystem::remove(p, ec);
        }
        refresh_all();
        update_inspector();
        RefreshSaveStateMenus();
        lbl_status_feedback->setText(StormLang(
            QStringLiteral("🗑️ Слот %1 очищен").arg(s),
            QStringLiteral("🗑️ Slot %1 deleted").arg(s),
            QStringLiteral("🗑️ Slot %1 gelöscht").arg(s),
            QStringLiteral("🗑️ Emplacement %1 supprimé").arg(s),
            QStringLiteral("🗑️ 槽位 %1 已删除").arg(s),
            QStringLiteral("🗑️ スロット %1 を削除しました").arg(s)));
    });

    connect(btn_clear_all, &QPushButton::clicked, &dlg, [&]() {
        const u64 tid = get_selected_tid();
        if (tid == 0) return;
        const auto ans = QMessageBox::warning(
            &dlg,
            StormLang(QStringLiteral("Очистка всех слотов игры"), QStringLiteral("Clear All Game Slots"),
                      QStringLiteral("Alle Slots leeren"), QStringLiteral("Vider tous les emplacements"),
                      QStringLiteral("清空所有槽位"), QStringLiteral("全スロットの消去")),
            StormLang(QStringLiteral("Удалить ВСЕ быстрые сохранения (слоты 1-10) для игры «%1»?").arg(resolve_game_title(tid)),
                      QStringLiteral("Delete ALL save state slots (1-10) for '%1'?").arg(resolve_game_title(tid)),
                      QStringLiteral("ALLE Speicher-Slots (1-10) für '%1' löschen?").arg(resolve_game_title(tid)),
                      QStringLiteral("Supprimer TOUS les emplacements (1-10) pour « %1 » ?").arg(resolve_game_title(tid)),
                      QStringLiteral("确定要删除“%1”的所有快速保存槽位 (1-10) 吗？").arg(resolve_game_title(tid)),
                      QStringLiteral("「%1」の全セーブスロット (1-10) を削除しますか？").arg(resolve_game_title(tid))),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ans != QMessageBox::Yes) return;

        Core::FlushPendingSaveStates();
        std::error_code ec;
        for (int s = 1; s <= kMaxSlots; ++s) {
            const auto ses_path = Core::GetSaveStatePath(tid, static_cast<u32>(s));
            for (const char* ext : {".ses", ".png", ".txt"}) {
                auto p = ses_path; p.replace_extension(ext);
                std::filesystem::remove(p, ec);
            }
        }
        refresh_all();
        update_inspector();
        RefreshSaveStateMenus();
        lbl_status_feedback->setText(StormLang(
            QStringLiteral("🧹 Все слоты выбранной игры очищены"),
            QStringLiteral("🧹 All slots for the selected game cleared"),
            QStringLiteral("🧹 Alle Slots für das Spiel wurden geleert"),
            QStringLiteral("🧹 Tous les emplacements du jeu ont été vidés"),
            QStringLiteral("🧹 所选游戏的所有槽位已清空"),
            QStringLiteral("🧹 選択したゲームの全スロットを消去しました")));
    });

    refresh_all();
    update_inspector();
    if (active_tid != 0 && render_window) {
        QTimer::singleShot(280, &dlg, [&]() {
            refresh_all();
            update_inspector();
        });
    }
    dlg.exec();
    RefreshSaveStateMenus();
}

void MainWindow::RefreshSaveStateMenus() {
    u64 title_id = 0;
    if (emulation_running && QtCommon::system && QtCommon::system->IsPoweredOn()) {
        title_id = QtCommon::system->GetApplicationProcessProgramID();
    }
    if (title_id == 0) {
        title_id = m_current_addons_title_id;
    }
    for (int i = 0; i < 5; ++i) {
        const int slot_num = i + 1;
        QString suffix = StormLang(
            QStringLiteral(" (Пусто)"),
            QStringLiteral(" (Empty)"),
            QStringLiteral(" (Leer)"),
            QStringLiteral(" (Vide)"),
            QStringLiteral(" (空)"),
            QStringLiteral(" (空)"));
        bool has_file = false;
        if (title_id != 0) {
            const auto path = Core::GetSaveStatePath(title_id, static_cast<u32>(slot_num));
            std::error_code ec;
            if (std::filesystem::exists(path, ec)) {
                has_file = true;
                const auto fsize = std::filesystem::file_size(path, ec);
                const double mb = static_cast<double>(fsize) / (1024.0 * 1024.0);
                QFileInfo fi(QString::fromStdString(Common::FS::PathToUTF8String(path)));
                const QString dt = fi.lastModified().toString(QStringLiteral("dd.MM.yyyy HH:mm:ss"));
                suffix = QStringLiteral("  [%1 MB - %2]").arg(QString::number(mb, 'f', 2), dt);
            }
        }
        const QString slot_prefix = StormLang(
            QStringLiteral("Слот %1").arg(slot_num),
            QStringLiteral("Slot %1").arg(slot_num),
            QStringLiteral("Slot %1").arg(slot_num),
            QStringLiteral("Emplacement %1").arg(slot_num),
            QStringLiteral("槽位 %1").arg(slot_num),
            QStringLiteral("スロット %1").arg(slot_num));
        if (save_slot_actions[i]) {
            save_slot_actions[i]->setText(slot_prefix + suffix);
        }
        if (load_slot_actions[i]) {
            load_slot_actions[i]->setText(slot_prefix + suffix);
            load_slot_actions[i]->setEnabled(has_file || title_id == 0);
        }
    }
}

void MainWindow::OnGameListRefresh() {
    // Resets metadata cache and reloads
    QtCommon::Game::ResetMetadata(false);
    game_list->RefreshGameDirectory();
    game_list->RefreshExternalContent();
    SetFirmwareVersion();
}

void MainWindow::LaunchFirmwareApplet(u64 raw_program_id,
                                      std::optional<Service::NFP::CabinetMode> cabinet_mode) {
    auto const program_id = Service::AM::AppletProgramId(raw_program_id);
    auto result = FirmwareManager::VerifyFirmware(*QtCommon::system.get());
    using namespace QtCommon::StringLookup;
    switch (result) {
    case FirmwareManager::ErrorFirmwareMissing:
        QMessageBox::warning(this, tr("No firmware available"),
                             Lookup(FwCheckErrorFirmwareMissing));
        return;
    case FirmwareManager::ErrorFirmwareCorrupted:
        QMessageBox::warning(this, tr("Firmware Corrupted"), Lookup(FwCheckErrorFirmwareCorrupted));
        return;
    default:
        break;
    }
    auto bis_system = QtCommon::system->GetFileSystemController().GetSystemNANDContents();
    if (auto applet_nca =
            bis_system->GetEntry(u64(program_id), FileSys::ContentRecordType::Program);
        applet_nca) {
        if (auto const applet_id =
                [program_id] {
                    using namespace Service::AM;
                    switch (program_id) {
                    case AppletProgramId::OverlayDisplay:
                        return AppletId::OverlayDisplay;
                    case AppletProgramId::QLaunch:
                        return AppletId::QLaunch;
                    case AppletProgramId::Starter:
                        return AppletId::Starter;
                    case AppletProgramId::Auth:
                        return AppletId::Auth;
                    case AppletProgramId::Cabinet:
                        return AppletId::Cabinet;
                    case AppletProgramId::Controller:
                        return AppletId::Controller;
                    case AppletProgramId::DataErase:
                        return AppletId::DataErase;
                    case AppletProgramId::Error:
                        return AppletId::Error;
                    case AppletProgramId::NetConnect:
                        return AppletId::NetConnect;
                    case AppletProgramId::ProfileSelect:
                        return AppletId::ProfileSelect;
                    case AppletProgramId::SoftwareKeyboard:
                        return AppletId::SoftwareKeyboard;
                    case AppletProgramId::MiiEdit:
                        return AppletId::MiiEdit;
                    case AppletProgramId::Web:
                        return AppletId::Web;
                    case AppletProgramId::Shop:
                        return AppletId::Shop;
                    case AppletProgramId::PhotoViewer:
                        return AppletId::PhotoViewer;
                    case AppletProgramId::Settings:
                        return AppletId::Settings;
                    case AppletProgramId::OfflineWeb:
                        return AppletId::OfflineWeb;
                    case AppletProgramId::LoginShare:
                        return AppletId::LoginShare;
                    case AppletProgramId::WebAuth:
                        return AppletId::WebAuth;
                    case AppletProgramId::MyPage:
                        return AppletId::MyPage;
                    default:
                        return AppletId::None;
                    }
                }();
            applet_id != Service::AM::AppletId::None) {
            QtCommon::system->GetFrontendAppletHolder().SetCurrentAppletId(applet_id);
            if (cabinet_mode)
                QtCommon::system->GetFrontendAppletHolder().SetCabinetMode(*cabinet_mode);
            // ?
            auto const filename = QString::fromStdString((applet_nca->GetFullPath()));
            UISettings::values.roms_path = QFileInfo(filename).path().toStdString();
            BootGame(filename, LibraryAppletParameters(u64(program_id), applet_id));
        } else {
            QMessageBox::warning(this, tr("Unknown applet"),
                                 tr("Applet doesn't map to a known value."));
        }
    } else {
        QMessageBox::warning(this, tr("Record not found"),
                             tr("Applet not found. Please reinstall firmware."));
    }
}

void MainWindow::OnCreateHomeMenuDesktopShortcut() {
    QtCommon::Game::CreateHomeMenuShortcut(QtCommon::Game::ShortcutTarget::Desktop);
}

void MainWindow::OnCreateHomeMenuApplicationMenuShortcut() {
    QtCommon::Game::CreateHomeMenuShortcut(QtCommon::Game::ShortcutTarget::Applications);
}

void MainWindow::OnCaptureScreenshot() {
    if (QtCommon::emu_thread == nullptr || !QtCommon::emu_thread->IsRunning())
        return;

    const u64 title_id = QtCommon::system->GetApplicationProcessProgramID();
    const auto screenshot_path =
        QString::fromStdString(Common::FS::GetEdenPathString(Common::FS::EdenPath::ScreenshotsDir));
    const auto date =
        QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_hh-mm-ss-zzz"));
    QString filename = QStringLiteral("%1/%2_%3.png")
                           .arg(screenshot_path)
                           .arg(title_id, 16, 16, QLatin1Char{'0'})
                           .arg(date);

    if (!Common::FS::CreateDir(screenshot_path.toStdString()))
        return;

#ifdef _WIN32
    if (UISettings::values.enable_screenshot_save_as) {
        OnPauseGame();
        filename = QFileDialog::getSaveFileName(this, tr("Capture Screenshot"), filename,
                                                tr("PNG Image (*.png)"));
        OnStartGame();
        if (filename.isEmpty()) {
            return;
        }
    }
#endif
    render_window->CaptureScreenshot(filename);
}

#ifdef ENABLE_UPDATE_CHECKER
void MainWindow::OnEmulatorUpdateAvailable() {
    std::optional<Common::Net::Release> version = update_future.result();
    if (!version)
        return;

    auto parse_ver = [](std::string s) {
        while (!s.empty() && (s.front() == 'v' || s.front() == 'V' || s.front() == ' ')) {
            s.erase(s.begin());
        }
        std::vector<int> parts;
        std::string cur;
        for (unsigned char c : s) {
            if (c >= '0' && c <= '9') {
                cur += static_cast<char>(c);
            } else if (c == '.') {
                if (!cur.empty()) {
                    try { parts.push_back(std::stoi(cur)); } catch (...) { parts.push_back(0); }
                    cur.clear();
                }
            }
        }
        if (!cur.empty()) {
            try { parts.push_back(std::stoi(cur)); } catch (...) { parts.push_back(0); }
        }
        while (parts.size() < 3) parts.push_back(0);
        return parts;
    };

    const auto remote_p = parse_ver(version->tag);
    const auto local_p = parse_ver(std::string{Common::g_build_version});
    bool is_newer = false;
    for (size_t i = 0; i < std::max(remote_p.size(), local_p.size()); ++i) {
        const int r = i < remote_p.size() ? remote_p[i] : 0;
        const int l = i < local_p.size() ? local_p[i] : 0;
        if (r > l) { is_newer = true; break; }
        if (r < l) { is_newer = false; break; }
    }
    if (!is_newer)
        return;

    UpdateDialog dialog(version.value(), this);
    dialog.exec();
}
#endif

void MainWindow::UpdateWindowTitle(std::string_view filename, std::string_view display_version,
                                   std::string_view internal_version, std::string_view gpu_vendor) {
    static const std::string window_title =
        fmt::format("{} {}", std::string{Common::g_build_name}, std::string{Common::g_build_version});

    if (filename.empty()) {
        setWindowTitle(QString::fromStdString(window_title));
    } else {
        const std::string upper_gpu = QString::fromStdString(std::string{gpu_vendor}).toUpper().toStdString();
        const auto run_title = [filename, display_version, internal_version, upper_gpu]() {
            std::string disp_ver = std::string{display_version};
            if (disp_ver.empty()) {
                disp_ver = "1.0.0";
            }
            std::string intern_ver = std::string{internal_version};
            if (intern_ver.empty()) {
                intern_ver = "0";
            }
            return fmt::format("{} {} | {} | {} | {} | {}",
                               std::string{Common::g_build_name},
                               std::string{Common::g_build_version},
                               filename,
                               disp_ver,
                               intern_ver,
                               upper_gpu.empty() ? "GPU" : upper_gpu);
        }();
        setWindowTitle(QString::fromStdString(run_title));
    }
}

std::string MainWindow::CreateTASFramesString(
    std::array<size_t, InputCommon::TasInput::PLAYER_NUMBER> frames) const {
    std::string string = "";
    size_t maxPlayerIndex = 0;
    for (size_t i = 0; i < frames.size(); i++) {
        if (frames[i] != 0) {
            if (maxPlayerIndex != 0)
                string += ", ";
            while (maxPlayerIndex++ != i)
                string += "0, ";
            string += std::to_string(frames[i]);
        }
    }
    return string;
}

QString MainWindow::GetTasStateDescription() const {
    auto [tas_status, current_tas_frame, total_tas_frames] = input_subsystem->GetTas()->GetStatus();
    std::string tas_frames_string = CreateTASFramesString(total_tas_frames);
    switch (tas_status) {
    case InputCommon::TasInput::TasState::Running:
        return tr("TAS state: Running %1/%2")
            .arg(current_tas_frame)
            .arg(QString::fromStdString(tas_frames_string));
    case InputCommon::TasInput::TasState::Recording:
        return tr("TAS state: Recording %1").arg(total_tas_frames[0]);
    case InputCommon::TasInput::TasState::Stopped:
        return tr("TAS state: Idle %1/%2")
            .arg(current_tas_frame)
            .arg(QString::fromStdString(tas_frames_string));
    default:
        return tr("TAS State: Invalid");
    }
}

void MainWindow::OnTasStateChanged() {
    bool is_running = false;
    bool is_recording = false;
    if (emulation_running) {
        const InputCommon::TasInput::TasState tas_status =
            std::get<0>(input_subsystem->GetTas()->GetStatus());
        is_running = tas_status == InputCommon::TasInput::TasState::Running;
        is_recording = tas_status == InputCommon::TasInput::TasState::Recording;
    }

    ui->action_TAS_Start->setText(is_running ? tr("&Stop Running") : tr("&Start"));
    ui->action_TAS_Record->setText(is_recording ? tr("Stop R&ecording") : tr("R&ecord"));

    ui->action_TAS_Start->setEnabled(emulation_running);
    ui->action_TAS_Record->setEnabled(emulation_running);
    ui->action_TAS_Reset->setEnabled(emulation_running);
}

void MainWindow::UpdateStatusBar() {
    if (QtCommon::emu_thread == nullptr || !QtCommon::system || !QtCommon::system->IsPoweredOn() ||
        QtCommon::system->IsShuttingDown() || m_is_stopping_emulation || !emulation_running) {
        status_bar_update_timer.stop();
        return;
    }

    if (Settings::values.tas_enable)
        tas_label->setText(GetTasStateDescription());
    else
        tas_label->clear();

    auto results = QtCommon::system->GetAndResetPerfStats();
    auto& shader_notify = QtCommon::system->GPU().ShaderNotify();
    const int shaders_building = shader_notify.ShadersBuilding();

    emit statsUpdated(results, shader_notify);

    if (shaders_building > 0) {
        shader_building_label->setText(StormLang(
            QStringLiteral("Компиляция: %1 шейдер(ов)").arg(shaders_building),
            QStringLiteral("Compiling: %1 shader(s)").arg(shaders_building),
            QStringLiteral("Kompilierung: %1 Shader").arg(shaders_building),
            QStringLiteral("Compilation : %1 shader(s)").arg(shaders_building),
            QStringLiteral("编译中：%1 个着色器").arg(shaders_building),
            QStringLiteral("コンパイル中: %1 シェーダー").arg(shaders_building)
        ));
        shader_building_label->setStyleSheet(QStringLiteral(
            "QLabel { background-color: rgba(255, 64, 129, 0.12); color: #ff4081; border: 1px solid rgba(255, 64, 129, 0.35); "
            "border-radius: 4px; padding: 2px 6px; font-size: 7.2pt; font-weight: 700; }"));
        shader_building_label->setVisible(true);
    } else {
        shader_building_label->setVisible(false);
    }

    res_scale_label->setVisible(false);

    const QString spd_str = QString::number(results.emulation_speed * 100.0, 'f', 0);
    if (Settings::values.use_speed_limit.GetValue()) {
        const QString limit_str = QString::number(Settings::SpeedLimit());
        emu_speed_label->setText(StormLang(
            QStringLiteral("Скорость: %1% / %2%").arg(spd_str, limit_str),
            QStringLiteral("Speed: %1% / %2%").arg(spd_str, limit_str),
            QStringLiteral("Geschwindigkeit: %1% / %2%").arg(spd_str, limit_str),
            QStringLiteral("Vitesse : %1% / %2%").arg(spd_str, limit_str),
            QStringLiteral("速度：%1% / %2%").arg(spd_str, limit_str),
            QStringLiteral("速度: %1% / %2%").arg(spd_str, limit_str)
        ));
    } else {
        emu_speed_label->setText(StormLang(
            QStringLiteral("Скорость: %1%").arg(spd_str),
            QStringLiteral("Speed: %1%").arg(spd_str),
            QStringLiteral("Geschwindigkeit: %1%").arg(spd_str),
            QStringLiteral("Vitesse : %1%").arg(spd_str),
            QStringLiteral("速度：%1%").arg(spd_str),
            QStringLiteral("速度: %1%").arg(spd_str)
        ));
    }
    emu_speed_label->setStyleSheet(QStringLiteral(
        "QLabel { background-color: rgba(0, 230, 118, 0.10); color: #00e676; border: 1px solid rgba(0, 230, 118, 0.30); "
        "border-radius: 4px; padding: 2px 6px; font-size: 7.2pt; font-weight: 700; }"));

    double display_fps = results.average_game_fps;
    if (std::isnan(display_fps) || display_fps <= 0.0) {
        display_fps = results.system_fps;
    } else if (results.system_fps > 0.0 && results.system_fps > display_fps && (results.system_fps - display_fps) > 5.0) {
        display_fps = results.system_fps;
    }
    if (std::isnan(display_fps) || display_fps < 0.0) {
        display_fps = 0.0;
    }

    QString fpsText;
    if (Settings::values.frame_gen.GetValue() && results.average_game_fps > 0.0) {
        const u32 mult = std::clamp<u32>(Settings::values.frame_gen_multiplier.GetValue(), 2, 4);
        const double base_fps = results.average_game_fps;
        const double gen_fps = base_fps * mult;
        fpsText = QStringLiteral("🎮 %1 FPS [LSFG %2X -> %3 FPS]").arg(std::round(base_fps), 0, 'f', 0).arg(mult).arg(std::round(gen_fps), 0, 'f', 0);
    } else {
        fpsText = QStringLiteral("🎮 %1 FPS").arg(std::round(display_fps), 0, 'f', 0);
    }
    if (!m_fpsSuffix.isEmpty())
        fpsText = fpsText % QStringLiteral(" (%1)").arg(m_fpsSuffix);

    game_fps_label->setText(fpsText);
    game_fps_label->setStyleSheet(QStringLiteral(
        "QLabel { background-color: rgba(0, 229, 255, 0.10); color: #00e5ff; border: 1px solid rgba(0, 229, 255, 0.30); "
        "border-radius: 4px; padding: 2px 6px; font-size: 7.2pt; font-weight: 700; }"));

    const double ft_ms = (std::isfinite(results.frametime) && results.frametime >= 0.0)
                             ? (results.frametime * 1000.0)
                             : 0.0;
    const QString ms_str = QString::number(ft_ms, 'f', 2);
    emu_frametime_label->setText(StormLang(
        QStringLiteral("⏱️ %1 мс").arg(ms_str),
        QStringLiteral("⏱️ %1 ms").arg(ms_str),
        QStringLiteral("⏱️ %1 ms").arg(ms_str),
        QStringLiteral("⏱️ %1 ms").arg(ms_str),
        QStringLiteral("⏱️ %1 毫秒").arg(ms_str),
        QStringLiteral("⏱️ %1 ms").arg(ms_str)
    ));
    emu_frametime_label->setStyleSheet(QStringLiteral(
        "QLabel { background-color: rgba(255, 202, 40, 0.10); color: #ffca28; border: 1px solid rgba(255, 202, 40, 0.30); "
        "border-radius: 4px; padding: 2px 6px; font-size: 7.2pt; font-weight: 700; }"));

    emu_speed_label->setVisible(!Settings::values.use_multi_core.GetValue());
    game_fps_label->setVisible(true);
    emu_frametime_label->setVisible(true);

    static u32 s_fps_log_counter = 0;
    if (++s_fps_log_counter >= 5) {
        s_fps_log_counter = 0;
        LOG_INFO(Frontend, "Performance: FPS: {:.1f} | Frametime: {:.2f} ms | Speed: {:.0f}%", display_fps, ft_ms, results.emulation_speed * 100.0);
    }

    if (qEnvironmentVariableIsSet("STORM_AUTOTEST_SAVESTATE")) {
        const bool is_cold_load_test = (qEnvironmentVariable("STORM_AUTOTEST_SAVESTATE") == QStringLiteral("2"));
        static u32 s_autotest_ticks = 0;
        static u32 s_zero_fps_ticks = 0;
        if (display_fps >= 45.0) {
            s_zero_fps_ticks = 0;
            ++s_autotest_ticks;
            if (s_autotest_ticks == 3) {
                LOG_INFO(Frontend, "AutoTest: [Step 1/3] Saving State A (SHARED MEMORY) to Slot 2 at {:.1f} FPS...", display_fps);
                OnSaveStateSlot(2);
            } else if (s_autotest_ticks == 22) {
                LOG_INFO(Frontend, "AutoTest: [Step 2/3] In Initial Menu - Loading State A from Slot 2 at {:.1f} FPS...", display_fps);
                OnLoadStateSlot(2);
            } else if (s_autotest_ticks == 28) {
                LOG_INFO(Frontend, "AutoTest: ALL SAVESTATE TESTS PASSED! Stable {:.1f} FPS after multiple Quick Loads.", display_fps);
                Core::FlushPendingSaveStates();
                std::exit(0);
            }
        } else if (s_autotest_ticks >= 8 && display_fps < 5.0) {
            if (++s_zero_fps_ticks >= 6) {
                LOG_ERROR(Frontend, "AutoTest: FAILED! FPS dropped to {:.1f} after Quick Save/Load!", display_fps);
                std::exit(2);
            }
        }
    }
}

void MainWindow::ApplyDynamicSettingChange() {
    Settings::UpdateGPUAccuracy();
    Settings::UpdateRescalingInfo();
    const bool is_running = QtCommon::system && QtCommon::system->IsPoweredOn();
    if (is_running) {
        QtCommon::system->ApplySettings();
        if (render_window) {
            render_window->OnFramebufferSizeChanged();
        }
    }
    std::string per_game_config_name;
    const bool has_per_game_file =
        GetActivePerGameConfigName(current_game_path, last_filename_booted, per_game_config_name);
    if (has_per_game_file) {
        QtConfig per_game_config(per_game_config_name, Config::ConfigType::PerGameConfig, false);
        per_game_config.SaveAllValues();
    } else if (config) {
        config->SaveAllValues();
    }
    UpdateStatusButtons();
    ConfigurationShared::ReloadAllActiveWidgets();
}

void MainWindow::OnDynamicSettingChangedFromUI() {
    Settings::UpdateGPUAccuracy();
    Settings::UpdateRescalingInfo();
    if (QtCommon::system && QtCommon::system->IsPoweredOn()) {
        QtCommon::system->ApplySettings();
    }
    UpdateStatusButtons();
}

QString MainWindow::CleanDisplayString(const QString& str) {
    QString res = str;
    int idx = res.indexOf(QLatin1Char('('));
    if (idx > 0) {
        res = res.left(idx).trimmed();
    }
    return res;
}

void MainWindow::UpdateGPUAccuracyButton() {
    if (!gpu_accuracy_button) return;
    const auto gpu_accuracy = Settings::values.gpu_accuracy.GetValue();
    QString text = (gpu_accuracy == Settings::GpuAccuracy::Low)
        ? StormLang(QStringLiteral("Быстрый"), QStringLiteral("Fast"), QStringLiteral("Schnell"), QStringLiteral("Rapide"), QStringLiteral("快速"), QStringLiteral("高速"))
        : StormLang(QStringLiteral("Высокая"), QStringLiteral("High"), QStringLiteral("Hoch"), QStringLiteral("Élevée"), QStringLiteral("高"), QStringLiteral("高"));
    gpu_accuracy_button->setText(StormLang(
        QStringLiteral("ТОЧНОСТЬ ГПУ:\n%1"),
        QStringLiteral("GPU ACCURACY:\n%1"),
        QStringLiteral("GPU-GENAUIGKEIT:\n%1"),
        QStringLiteral("PRÉCISION GPU:\n%1"),
        QStringLiteral("GPU 精度:\n%1"),
        QStringLiteral("GPU 精度:\n%1")
    ).arg(text.toUpper()));
    gpu_accuracy_button->setChecked(gpu_accuracy != Settings::GpuAccuracy::Low);
}

void MainWindow::UpdateDockedButton() {
    if (!dock_status_button) return;
    const auto console_mode = Settings::values.use_docked_mode.GetValue();
    dock_status_button->setChecked(Settings::IsDockedMode());
    const QString mode_text = (console_mode == Settings::ConsoleMode::Docked)
        ? StormLang(QStringLiteral("В ДОКЕ"), QStringLiteral("DOCKED"), QStringLiteral("DOCKED"), QStringLiteral("DOCKÉ"), QStringLiteral("底座"), QStringLiteral("ドック"))
        : StormLang(QStringLiteral("ПОРТАТИВ"), QStringLiteral("HANDHELD"), QStringLiteral("HANDHELD"), QStringLiteral("PORTABLE"), QStringLiteral("掌机"), QStringLiteral("携帯"));
    dock_status_button->setText(StormLang(
        QStringLiteral("РЕЖИМ:\n%1"),
        QStringLiteral("MODE:\n%1"),
        QStringLiteral("MODUS:\n%1"),
        QStringLiteral("MODE:\n%1"),
        QStringLiteral("模式:\n%1"),
        QStringLiteral("モード:\n%1")
    ).arg(mode_text));
}

void MainWindow::UpdateAPIText() {
    if (!renderer_status_button) return;
    const auto api = Settings::values.renderer_backend.GetValue();
    QString api_text;
    switch (api) {
    case Settings::RendererBackend::Vulkan:
        api_text = QStringLiteral("VULKAN");
        break;
    case Settings::RendererBackend::OpenGL_GLSL:
        api_text = QStringLiteral("OPENGL GLSL");
        break;
    case Settings::RendererBackend::OpenGL_SPIRV:
        api_text = QStringLiteral("OPENGL SPIRV");
        break;
    case Settings::RendererBackend::OpenGL_GLASM:
        api_text = QStringLiteral("OPENGL GLASM");
        break;
    default:
        api_text = QStringLiteral("VULKAN");
        break;
    }
    renderer_status_button->setText(StormLang(
        QStringLiteral("РЕНДЕР:\n%1"),
        QStringLiteral("RENDER:\n%1"),
        QStringLiteral("RENDER:\n%1"),
        QStringLiteral("RENDU:\n%1"),
        QStringLiteral("渲染:\n%1"),
        QStringLiteral("レンダー:\n%1")
    ).arg(api_text));
}

void MainWindow::UpdateShaderText() {
    if (!shader_status_button) return;
    const int idx = Settings::values.post_processing_shader.GetValue();
    const QString short_name = StormShaders::GetLocalizedShortName(idx);
    shader_status_button->setChecked(idx > 0);
    shader_status_button->setText(StormLang(
        QStringLiteral("ШЕЙДЕР:\n%1"),
        QStringLiteral("SHADER:\n%1"),
        QStringLiteral("SHADER:\n%1"),
        QStringLiteral("SHADER:\n%1"),
        QStringLiteral("着色器:\n%1"),
        QStringLiteral("シェーダー:\n%1"),
        QStringLiteral("المظلل:\n%1"),
        QStringLiteral("SHADER:\n%1")
    ).arg(short_name));

    const auto& cat = StormShaders::GetRetroArchShaderCatalog();
    const int safe_idx = std::clamp(idx, 0, static_cast<int>(cat.size()) - 1);
    const QString full_name = StormShaders::GetLocalizedName(cat[static_cast<std::size_t>(safe_idx)]);
    shader_status_button->setToolTip(StormLang(
        QStringLiteral("Шейдер постобработки (RetroArch): %1\nНажмите для быстрого поиска и динамического переключения на лету"),
        QStringLiteral("Post-Processing Shader (RetroArch): %1\nClick for fast search and real-time switching"),
        QStringLiteral("Post-Processing-Shader (RetroArch): %1\nKlicken für Schnellsuche und Echtzeit-Wechsel"),
        QStringLiteral("Shader de post-traitement (RetroArch) : %1\nCliquer pour rechercher et changer en temps réel"),
        QStringLiteral("后处理着色器 (RetroArch)：%1\n点击快速搜索并实时动态切换"),
        QStringLiteral("ポストプロセスシェーダー (RetroArch): %1\nクリックして高速検索とリアルタイム切り替え"),
        QStringLiteral("مظلل المعالجة اللاحقة (RetroArch): %1\nانقر للبحث السريع والتبديل الفوري"),
        QStringLiteral("Sombreador de postprocesado (RetroArch): %1\nHaz clic para buscar y cambiar en tiempo real")
    ).arg(full_name));
}

void MainWindow::UpdateFilterText() {
    if (!filter_status_button) return;
    const auto filter = Settings::values.scaling_filter.GetValue();
    QString filter_text;
    switch (filter) {
    case Settings::ScalingFilter::NearestNeighbor:
        filter_text = StormLang(QStringLiteral("Ближайший"), QStringLiteral("Nearest"), QStringLiteral("Nächster"), QStringLiteral("Plus proche"), QStringLiteral("最近邻"), QStringLiteral("最近傍"));
        break;
    case Settings::ScalingFilter::Bilinear:
        filter_text = StormLang(QStringLiteral("Билинейный"), QStringLiteral("Bilinear"), QStringLiteral("Bilinear"), QStringLiteral("Bilinéaire"), QStringLiteral("双线性"), QStringLiteral("バイリニア"));
        break;
    case Settings::ScalingFilter::Bicubic:
        filter_text = StormLang(QStringLiteral("Бикубический"), QStringLiteral("Bicubic"), QStringLiteral("Bikubisch"), QStringLiteral("Bicubique"), QStringLiteral("双三次"), QStringLiteral("バイキュービック"));
        break;
    case Settings::ScalingFilter::Gaussian:
        filter_text = StormLang(QStringLiteral("Гаусс"), QStringLiteral("Gaussian"), QStringLiteral("Gauß"), QStringLiteral("Gauss"), QStringLiteral("高斯"), QStringLiteral("ガウス"));
        break;
    case Settings::ScalingFilter::Lanczos:
        filter_text = StormLang(QStringLiteral("Ланцош"), QStringLiteral("Lanczos"), QStringLiteral("Lanczos"), QStringLiteral("Lanczos"), QStringLiteral("兰索斯"), QStringLiteral("ランツォシュ"));
        break;
    case Settings::ScalingFilter::ScaleForce:
        filter_text = QStringLiteral("ScaleForce");
        break;
    case Settings::ScalingFilter::Fsr:
        filter_text = QStringLiteral("FSR");
        break;
    case Settings::ScalingFilter::Area:
        filter_text = QStringLiteral("Area");
        break;
    case Settings::ScalingFilter::Mmpx:
        filter_text = QStringLiteral("MMPX");
        break;
    case Settings::ScalingFilter::ZeroTangent:
        filter_text = QStringLiteral("Zero-Tangent");
        break;
    case Settings::ScalingFilter::BSpline:
        filter_text = QStringLiteral("B-Spline");
        break;
    case Settings::ScalingFilter::Mitchell:
        filter_text = QStringLiteral("Mitchell");
        break;
    case Settings::ScalingFilter::Spline1:
        filter_text = QStringLiteral("Spline-1");
        break;
    case Settings::ScalingFilter::Sgsr:
        filter_text = QStringLiteral("SGSR");
        break;
    case Settings::ScalingFilter::SgsrEdge:
        filter_text = QStringLiteral("SGSR EdgeDir");
        break;
    default:
        filter_text = QStringLiteral("FSR");
        break;
    }
    filter_status_button->setText(StormLang(
        QStringLiteral("ФИЛЬТР:\n%1"),
        QStringLiteral("FILTER:\n%1"),
        QStringLiteral("FILTER:\n%1"),
        QStringLiteral("FILTRE:\n%1"),
        QStringLiteral("滤镜:\n%1"),
        QStringLiteral("フィルター:\n%1")
    ).arg(filter_text.toUpper()));
}

void MainWindow::UpdateAAText() {
    if (!aa_status_button) return;
    const auto aa_mode = Settings::values.anti_aliasing.GetValue();
    QString val_text;
    switch (aa_mode) {
    case Settings::AntiAliasing::None:
        val_text = StormLang(QStringLiteral("ВЫКЛ"), QStringLiteral("OFF"), QStringLiteral("AUS"), QStringLiteral("DÉSACTIVÉ"), QStringLiteral("关闭"), QStringLiteral("オフ"));
        break;
    case Settings::AntiAliasing::Fxaa:
        val_text = QStringLiteral("FXAA");
        break;
    case Settings::AntiAliasing::Smaa:
        val_text = QStringLiteral("SMAA");
        break;
    default:
        val_text = StormLang(QStringLiteral("ВЫКЛ"), QStringLiteral("OFF"), QStringLiteral("AUS"), QStringLiteral("DÉSACTIVÉ"), QStringLiteral("关闭"), QStringLiteral("オフ"));
        break;
    }
    aa_status_button->setText(StormLang(
        QStringLiteral("СГЛАЖИВАНИЕ:\n%1"),
        QStringLiteral("ANTI-ALIASING:\n%1"),
        QStringLiteral("KANTENGLÄTTUNG:\n%1"),
        QStringLiteral("ANTICRÉNELAGE:\n%1"),
        QStringLiteral("抗锯齿:\n%1"),
        QStringLiteral("アンチエイリアス:\n%1")
    ).arg(val_text));
}

void MainWindow::UpdateVolumeUI() {
    if (!volume_button || !volume_slider) return;
    const auto volume_value = static_cast<int>(Settings::values.volume.GetValue());
    volume_slider->setValue(volume_value);
    if (volume_val_label) {
        volume_val_label->setText(StormLang(
            QStringLiteral("Громкость: %1%"),
            QStringLiteral("Volume: %1%"),
            QStringLiteral("Lautstärke: %1%"),
            QStringLiteral("Volume : %1%"),
            QStringLiteral("音量: %1%"),
            QStringLiteral("音量: %1%")
        ).arg(volume_value));
    }
    if (Settings::values.audio_muted.GetValue()) {
        volume_button->setChecked(false);
        volume_button->setText(StormLang(
            QStringLiteral("ГРОМКОСТЬ:\nВЫКЛ"),
            QStringLiteral("VOLUME:\nMUTE"),
            QStringLiteral("LAUTSTÄRKE:\nSTUMM"),
            QStringLiteral("VOLUME:\nMUET"),
            QStringLiteral("音量:\n静音"),
            QStringLiteral("音量:\n消音")
        ));
    } else {
        volume_button->setChecked(true);
        volume_button->setText(StormLang(
            QStringLiteral("ГРОМКОСТЬ:\n%1%"),
            QStringLiteral("VOLUME:\n%1%"),
            QStringLiteral("LAUTSTÄRKE:\n%1%"),
            QStringLiteral("VOLUME:\n%1%"),
            QStringLiteral("音量:\n%1%"),
            QStringLiteral("音量:\n%1%")
        ).arg(volume_value));
    }
}

void MainWindow::UpdateAspectText() {
    if (!aspect_ratio_button) return;
    QString val_text;
    switch (Settings::values.aspect_ratio.GetValue()) {
    case Settings::AspectRatio::R16_9:
        val_text = QStringLiteral("16:9");
        break;
    case Settings::AspectRatio::R4_3:
        val_text = QStringLiteral("4:3");
        break;
    case Settings::AspectRatio::R21_9:
        val_text = QStringLiteral("21:9");
        break;
    case Settings::AspectRatio::R16_10:
        val_text = QStringLiteral("16:10");
        break;
    case Settings::AspectRatio::Stretch:
        val_text = StormLang(QStringLiteral("РАСТЯНУТЬ"), QStringLiteral("STRETCH"), QStringLiteral("STRECKEN"), QStringLiteral("ÉTIRER"), QStringLiteral("拉伸"), QStringLiteral("引き伸ばし"));
        break;
    case Settings::AspectRatio::R32_9:
        val_text = QStringLiteral("32:9");
        break;
    case Settings::AspectRatio::R1_1:
        val_text = QStringLiteral("1:1");
        break;
    case Settings::AspectRatio::R3_2:
        val_text = QStringLiteral("3:2");
        break;
    case Settings::AspectRatio::R5_4:
        val_text = QStringLiteral("5:4");
        break;
    case Settings::AspectRatio::R18_9:
        val_text = QStringLiteral("18:9");
        break;
    case Settings::AspectRatio::R19_5_9:
        val_text = QStringLiteral("19.5:9");
        break;
    case Settings::AspectRatio::R20_9:
        val_text = QStringLiteral("20:9");
        break;
    case Settings::AspectRatio::R21_10:
        val_text = QStringLiteral("21:10");
        break;
    case Settings::AspectRatio::R24_10:
        val_text = QStringLiteral("24:10");
        break;
    case Settings::AspectRatio::R32_10:
        val_text = QStringLiteral("32:10");
        break;
    case Settings::AspectRatio::R48_9:
        val_text = QStringLiteral("48:9");
        break;
    case Settings::AspectRatio::Auto_Window:
        val_text = StormLang(QStringLiteral("АВТО ОКНО"), QStringLiteral("AUTO WIN"), QStringLiteral("AUTO FENST."), QStringLiteral("AUTO FEN."), QStringLiteral("自动窗口"), QStringLiteral("自動ウィンドウ"), QStringLiteral("تلقائي"), QStringLiteral("AUTO VENT."));
        break;
    default:
        val_text = QStringLiteral("16:9");
        break;
    }
    aspect_ratio_button->setText(StormLang(
        QStringLiteral("СООТНОШЕНИЕ:\n%1"),
        QStringLiteral("ASPECT RATIO:\n%1"),
        QStringLiteral("SEITENVERH.:\n%1"),
        QStringLiteral("PROPORTION:\n%1"),
        QStringLiteral("宽高比:\n%1"),
        QStringLiteral("アスペクト比:\n%1")
    ).arg(val_text));
    aspect_ratio_button->setToolTip(StormLang(
        QStringLiteral("Соотношение сторон экрана"),
        QStringLiteral("Screen aspect ratio"),
        QStringLiteral("Bildschirm-Seitenverhältnis"),
        QStringLiteral("Format d'image de l'écran"),
        QStringLiteral("屏幕宽高比"),
        QStringLiteral("画面のアスペクト比")
    ));
}

void MainWindow::UpdateDmaText() {
    if (!dma_accuracy_button) return;
    QString val_text;
    switch (Settings::values.dma_accuracy.GetValue()) {
    case Settings::DmaAccuracy::Default:
        val_text = StormLang(QStringLiteral("ПО УМОЛЧ."), QStringLiteral("DEFAULT"), QStringLiteral("STANDARD"), QStringLiteral("DÉFAUT"), QStringLiteral("默认"), QStringLiteral("デフォルト"));
        break;
    case Settings::DmaAccuracy::Normal:
        val_text = StormLang(QStringLiteral("НОРМАЛЬНО"), QStringLiteral("NORMAL"), QStringLiteral("NORMAL"), QStringLiteral("NORMAL"), QStringLiteral("正常"), QStringLiteral("通常"));
        break;
    case Settings::DmaAccuracy::Unsafe:
        val_text = StormLang(QStringLiteral("НЕБЕЗОПАСНО"), QStringLiteral("UNSAFE"), QStringLiteral("UNSICHER"), QStringLiteral("NON SÉCURISÉ"), QStringLiteral("不安全"), QStringLiteral("非安全"));
        break;
    case Settings::DmaAccuracy::Safe:
        val_text = StormLang(QStringLiteral("БЕЗОПАСНО"), QStringLiteral("SAFE"), QStringLiteral("SICHER"), QStringLiteral("SÉCURISÉ"), QStringLiteral("安全"), QStringLiteral("安全"));
        break;
    default:
        val_text = StormLang(QStringLiteral("ПО УМОЛЧ."), QStringLiteral("DEFAULT"), QStringLiteral("STANDARD"), QStringLiteral("DÉFAUT"), QStringLiteral("默认"), QStringLiteral("デフォルト"));
        break;
    }
    dma_accuracy_button->setText(StormLang(
        QStringLiteral("DMA:\n%1"),
        QStringLiteral("DMA:\n%1"),
        QStringLiteral("DMA:\n%1"),
        QStringLiteral("DMA:\n%1"),
        QStringLiteral("DMA:\n%1"),
        QStringLiteral("DMA:\n%1")
    ).arg(val_text));
    dma_accuracy_button->setToolTip(StormLang(
        QStringLiteral("Точность Direct Memory Access (DMA)"),
        QStringLiteral("Direct Memory Access (DMA) accuracy"),
        QStringLiteral("Direct Memory Access (DMA)-Genauigkeit"),
        QStringLiteral("Précision Direct Memory Access (DMA)"),
        QStringLiteral("直接内存访问 (DMA) 精度"),
        QStringLiteral("Direct Memory Access (DMA) 精度")
    ));
}

void MainWindow::UpdateGpuFenceText() {
    if (!gpu_fence_button) return;
    QString val_text;
    switch (Settings::values.gpu_fence_behavior.GetValue()) {
    case Settings::GpuFenceBehavior::Default:
        val_text = StormLang(QStringLiteral("ПО УМОЛЧ."), QStringLiteral("DEFAULT"), QStringLiteral("STANDARD"), QStringLiteral("DÉFAUT"), QStringLiteral("默认"), QStringLiteral("デフォルト"));
        break;
    case Settings::GpuFenceBehavior::Immediate:
        val_text = StormLang(QStringLiteral("НЕМЕДЛЕННО"), QStringLiteral("IMMEDIATE"), QStringLiteral("SOFORT"), QStringLiteral("IMMÉDIAT"), QStringLiteral("即时"), QStringLiteral("即時"));
        break;
    case Settings::GpuFenceBehavior::Balanced:
        val_text = StormLang(QStringLiteral("БАЛАНС"), QStringLiteral("BALANCED"), QStringLiteral("AUSGEGLICHEN"), QStringLiteral("ÉQUILIBRÉ"), QStringLiteral("均衡"), QStringLiteral("バランス"));
        break;
    case Settings::GpuFenceBehavior::Accurate:
        val_text = StormLang(QStringLiteral("ТОЧНО"), QStringLiteral("ACCURATE"), QStringLiteral("GENAU"), QStringLiteral("PRÉCIS"), QStringLiteral("准确"), QStringLiteral("正確"));
        break;
    case Settings::GpuFenceBehavior::Strict:
        val_text = StormLang(QStringLiteral("СТРОГО"), QStringLiteral("STRICT"), QStringLiteral("STRIKT"), QStringLiteral("STRICT"), QStringLiteral("严格"), QStringLiteral("厳格"));
        break;
    default:
        val_text = StormLang(QStringLiteral("ПО УМОЛЧ."), QStringLiteral("DEFAULT"), QStringLiteral("STANDARD"), QStringLiteral("DÉFAUT"), QStringLiteral("默认"), QStringLiteral("デフォルト"));
        break;
    }
    gpu_fence_button->setText(StormLang(
        QStringLiteral("БАРЬЕРЫ ГПУ:\n%1"),
        QStringLiteral("GPU BARRIERS:\n%1"),
        QStringLiteral("GPU-BARRIEREN:\n%1"),
        QStringLiteral("BARRIÈRES GPU:\n%1"),
        QStringLiteral("GPU 栅栏:\n%1"),
        QStringLiteral("GPU バリア:\n%1")
    ).arg(val_text));
    gpu_fence_button->setToolTip(StormLang(
        QStringLiteral("Поведение барьеров GPU — синхронизация команд рендеринга"),
        QStringLiteral("GPU fence behavior — render command synchronization"),
        QStringLiteral("GPU-Fence-Verhalten — Renderbefehl-Synchronisation"),
        QStringLiteral("Comportement des barrières GPU — synchronisation des commandes de rendu"),
        QStringLiteral("GPU 栅栏行为 — 渲染命令同步"),
        QStringLiteral("GPU バリア動作 — レンダリングコマンド同期")
    ));
}

void MainWindow::UpdateVramText() {
    if (!vram_mode_button) return;
    QString val_text;
    switch (Settings::values.vram_usage_mode.GetValue()) {
    case Settings::VramUsageMode::Conservative:
        val_text = StormLang(QStringLiteral("ЭКОНОМНЫЙ"), QStringLiteral("CONSERVATIVE"), QStringLiteral("SPARSAM"), QStringLiteral("ÉCONOME"), QStringLiteral("保守"), QStringLiteral("省メモリ"));
        break;
    case Settings::VramUsageMode::Normal:
        val_text = StormLang(QStringLiteral("НОРМАЛЬНЫЙ"), QStringLiteral("NORMAL"), QStringLiteral("NORMAL"), QStringLiteral("NORMAL"), QStringLiteral("正常"), QStringLiteral("通常"));
        break;
    case Settings::VramUsageMode::Aggressive:
        val_text = StormLang(QStringLiteral("АГРЕССИВНЫЙ"), QStringLiteral("AGGRESSIVE"), QStringLiteral("AGGRESSIV"), QStringLiteral("AGRESSIF"), QStringLiteral("激进"), QStringLiteral("アグレッシブ"));
        break;
    default:
        val_text = StormLang(QStringLiteral("НОРМАЛЬНЫЙ"), QStringLiteral("NORMAL"), QStringLiteral("NORMAL"), QStringLiteral("NORMAL"), QStringLiteral("正常"), QStringLiteral("通常"));
        break;
    }
    vram_mode_button->setText(StormLang(
        QStringLiteral("VRAM:\n%1"),
        QStringLiteral("VRAM:\n%1"),
        QStringLiteral("VRAM:\n%1"),
        QStringLiteral("VRAM:\n%1"),
        QStringLiteral("显存模式:\n%1"),
        QStringLiteral("VRAM:\n%1")
    ).arg(val_text));
    vram_mode_button->setToolTip(StormLang(
        QStringLiteral("Режим использования видеопамяти (VRAM)\n• Экономный: жесткое ограничение VRAM\n• Нормальный: полное выделение физической VRAM с резервом для ОС\n• Агрессивный: 100% мощности и физической памяти видеокарты без ограничений"),
        QStringLiteral("Video Memory (VRAM) usage mode\n• Conservative: strict VRAM limit\n• Normal: full physical VRAM allocation with OS reserve\n• Aggressive: 100% GPU memory allocation without limits"),
        QStringLiteral("Videospeicher (VRAM)-Nutzungsmodus\n• Sparsam: striktes VRAM-Limit\n• Normal: volle physische VRAM-Zuweisung mit OS-Reserve\n• Aggressiv: 100% GPU-Speicherzuweisung ohne Einschränkungen"),
        QStringLiteral("Mode d'utilisation de la mémoire vidéo (VRAM)\n• Économe : limite stricte de VRAM\n• Normal : allocation complète avec réserve pour l'OS\n• Agressif : 100% de la puissance et de la mémoire sans limites"),
        QStringLiteral("显存 (VRAM) 使用模式\n• 保守: 严格限制显存使用\n• 正常: 完整分配物理显存并保留系统冗余\n• 激进: 无限制榨干 100% 显存性能"),
        QStringLiteral("ビデオメモリ (VRAM) 使用モード\n• 省メモリ: 厳格なVRAM制限\n• 通常: OS用の予約を確保しつつ全物理VRAMを割り当て\n• アグレッシブ: 制限なしで100%のGPUメモリパワーを使用")
    ));
}

void MainWindow::UpdateAnisotropyText() {
    if (!anisotropy_button) return;
    QString val_text;
    switch (Settings::values.max_anisotropy.GetValue()) {
    case Settings::AnisotropyMode::Automatic:
        val_text = StormLang(QStringLiteral("АВТО"), QStringLiteral("AUTO"), QStringLiteral("AUTO"), QStringLiteral("AUTO"), QStringLiteral("自动"), QStringLiteral("自動"));
        break;
    case Settings::AnisotropyMode::Default:
        val_text = StormLang(QStringLiteral("ПО УМОЛЧ."), QStringLiteral("DEFAULT"), QStringLiteral("STANDARD"), QStringLiteral("DÉFAUT"), QStringLiteral("默认"), QStringLiteral("デフォルト"));
        break;
    case Settings::AnisotropyMode::X2:
        val_text = QStringLiteral("2x");
        break;
    case Settings::AnisotropyMode::X4:
        val_text = QStringLiteral("4x");
        break;
    case Settings::AnisotropyMode::X8:
        val_text = QStringLiteral("8x");
        break;
    case Settings::AnisotropyMode::X16:
        val_text = QStringLiteral("16x");
        break;
    case Settings::AnisotropyMode::X32:
        val_text = QStringLiteral("32x");
        break;
    case Settings::AnisotropyMode::X64:
        val_text = QStringLiteral("64x");
        break;
    case Settings::AnisotropyMode::None:
        val_text = StormLang(QStringLiteral("ВЫКЛ"), QStringLiteral("OFF"), QStringLiteral("AUS"), QStringLiteral("DÉSACTIVÉ"), QStringLiteral("关闭"), QStringLiteral("オフ"));
        break;
    default:
        val_text = StormLang(QStringLiteral("АВТО"), QStringLiteral("AUTO"), QStringLiteral("AUTO"), QStringLiteral("AUTO"), QStringLiteral("自动"), QStringLiteral("自動"));
        break;
    }
    anisotropy_button->setText(StormLang(
        QStringLiteral("АНИЗОТРОПИЯ:\n%1"),
        QStringLiteral("ANISOTROPY:\n%1"),
        QStringLiteral("ANISOTROPIE:\n%1"),
        QStringLiteral("ANISOTROPIE:\n%1"),
        QStringLiteral("各向异性:\n%1"),
        QStringLiteral("異方性:\n%1")
    ).arg(val_text));
}

void MainWindow::UpdateAstcDecodeText() {
    if (!astc_decode_button) return;
    QString val_text = StormLang(QStringLiteral("ЦП АСИНХ."), QStringLiteral("CPU ASYNC"), QStringLiteral("CPU ASYNC"), QStringLiteral("CPU ASYNC"), QStringLiteral("CPU 异步"), QStringLiteral("CPU 非同期"));
    switch (Settings::values.accelerate_astc.GetValue()) {
    case Settings::AstcDecodeMode::CpuAsynchronous:
        val_text = StormLang(QStringLiteral("ЦП АСИНХ."), QStringLiteral("CPU ASYNC"), QStringLiteral("CPU ASYNC"), QStringLiteral("CPU ASYNC"), QStringLiteral("CPU 异步"), QStringLiteral("CPU 非同期"));
        break;
    case Settings::AstcDecodeMode::Cpu:
        val_text = StormLang(QStringLiteral("ЦП"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"));
        break;
    case Settings::AstcDecodeMode::Gpu:
        val_text = StormLang(QStringLiteral("ГПУ"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"));
        break;
    case Settings::AstcDecodeMode::Hybrid:
        val_text = StormLang(QStringLiteral("ГИБРИД"), QStringLiteral("HYBRID"), QStringLiteral("HYBRID"), QStringLiteral("HYBRIDE"), QStringLiteral("混合"), QStringLiteral("ハイブリッド"));
        break;
    }
    astc_decode_button->setText(StormLang(
        QStringLiteral("ДЕКОД. ASTC:\n%1"),
        QStringLiteral("ASTC DECODE:\n%1"),
        QStringLiteral("ASTC-DEKOD.:\n%1"),
        QStringLiteral("DÉCOD. ASTC:\n%1"),
        QStringLiteral("ASTC 解码:\n%1"),
        QStringLiteral("ASTC デコード:\n%1")
    ).arg(val_text));
}

void MainWindow::UpdateAstcRecompressText() {
    if (!astc_recompress_button) return;
    QString val_text = StormLang(QStringLiteral("БЕЗ СЖАТИЯ"), QStringLiteral("UNCOMPRESSED"), QStringLiteral("UNKOMPRIMIERT"), QStringLiteral("NON COMPRESSÉ"), QStringLiteral("未压缩"), QStringLiteral("非圧縮"));
    switch (Settings::values.astc_recompression.GetValue()) {
    case Settings::AstcRecompression::Uncompressed:
        val_text = StormLang(QStringLiteral("БЕЗ СЖАТИЯ"), QStringLiteral("UNCOMPRESSED"), QStringLiteral("UNKOMPRIMIERT"), QStringLiteral("NON COMPRESSÉ"), QStringLiteral("未压缩"), QStringLiteral("非圧縮"));
        break;
    case Settings::AstcRecompression::Bc1:
        val_text = QStringLiteral("BC1");
        break;
    case Settings::AstcRecompression::Bc3:
        val_text = QStringLiteral("BC3");
        break;
    case Settings::AstcRecompression::Bc5:
        val_text = QStringLiteral("BC5");
        break;
    }
    astc_recompress_button->setText(StormLang(
        QStringLiteral("ПЕРЕСЖ. ASTC:\n%1"),
        QStringLiteral("ASTC RECOMP:\n%1"),
        QStringLiteral("ASTC-NEUKOMPR.:\n%1"),
        QStringLiteral("RECOMP. ASTC:\n%1"),
        QStringLiteral("ASTC 重新压缩:\n%1"),
        QStringLiteral("ASTC 再圧縮:\n%1")
    ).arg(val_text));
}

void MainWindow::UpdateResScaleText() {
    if (!res_scale_button) return;
    const bool is_docked = Settings::values.use_docked_mode.GetValue() == Settings::ConsoleMode::Docked;
    QString val_text = QStringLiteral("1X");
    switch (Settings::values.resolution_setup.GetValue()) {
    case Settings::ResolutionSetup::Res1_4X: val_text = is_docked ? QStringLiteral("0.25X (270p)") : QStringLiteral("0.25X (180p)"); break;
    case Settings::ResolutionSetup::Res1_2X: val_text = is_docked ? QStringLiteral("0.5X (540p)") : QStringLiteral("0.5X (360p)"); break;
    case Settings::ResolutionSetup::Res3_4X: val_text = is_docked ? QStringLiteral("0.75X (810p)") : QStringLiteral("0.75X (540p)"); break;
    case Settings::ResolutionSetup::Res1X: val_text = is_docked ? QStringLiteral("1X (1080p)") : QStringLiteral("1X (720p)"); break;
    case Settings::ResolutionSetup::Res5_4X: val_text = is_docked ? QStringLiteral("1.25X (1350p)") : QStringLiteral("1.25X (900p)"); break;
    case Settings::ResolutionSetup::Res3_2X: val_text = is_docked ? QStringLiteral("1.5X (1620p)") : QStringLiteral("1.5X (1080p)"); break;
    case Settings::ResolutionSetup::Res2X: val_text = is_docked ? QStringLiteral("2X (4K)") : QStringLiteral("2X (2K)"); break;
    case Settings::ResolutionSetup::Res3X: val_text = is_docked ? QStringLiteral("3X (6K)") : QStringLiteral("3X (4K)"); break;
    case Settings::ResolutionSetup::Res4X: val_text = is_docked ? QStringLiteral("4X (8K)") : QStringLiteral("4X (2880p)"); break;
    case Settings::ResolutionSetup::Res5X: val_text = is_docked ? QStringLiteral("5X (5400p)") : QStringLiteral("5X (3600p)"); break;
    case Settings::ResolutionSetup::Res6X: val_text = is_docked ? QStringLiteral("6X (6480p)") : QStringLiteral("6X (4320p)"); break;
    case Settings::ResolutionSetup::Res7X: val_text = is_docked ? QStringLiteral("7X (7560p)") : QStringLiteral("7X (5040p)"); break;
    case Settings::ResolutionSetup::Res8X: val_text = is_docked ? QStringLiteral("8X (8640p)") : QStringLiteral("8X (5760p)"); break;
    default: break;
    }
    res_scale_button->setText(StormLang(
        QStringLiteral("МАСШТАБ:\n%1"),
        QStringLiteral("SCALE:\n%1"),
        QStringLiteral("SKALIERUNG:\n%1"),
        QStringLiteral("ÉCHELLE:\n%1"),
        QStringLiteral("分辨率缩放:\n%1"),
        QStringLiteral("解像度:\n%1")
    ).arg(val_text));
}

void MainWindow::UpdateAddonsStatusButton(u64 title_id, const QString& game_name) {
    if (!addons_status_button) return;
    if (title_id != 0) {
        m_current_addons_title_id = title_id;
    }
    if (!game_name.isEmpty()) {
        m_current_addons_game_name = game_name;
    }
    if (m_current_addons_title_id == 0) {
        addons_status_button->setText(StormLang(
            QStringLiteral("ДОПОЛНЕНИЯ:\nНЕТ"),
            QStringLiteral("ADD-ONS:\nNONE"),
            QStringLiteral("ADD-ONS:\nKEINE"),
            QStringLiteral("EXTENSIONS:\nAUCUN"),
            QStringLiteral("附加组件:\n无"),
            QStringLiteral("アドオン:\nなし")
        ));
        addons_status_button->setStyleSheet(QString{});
        addons_status_button->setToolTip(StormLang(
            QStringLiteral("Выберите или запустите игру для просмотра дополнений"),
            QStringLiteral("Select or launch a game to view add-ons"),
            QStringLiteral("Spiel auswählen oder starten, um Add-ons anzuzeigen"),
            QStringLiteral("Sélectionnez ou lancez un jeu pour voir les extensions"),
            QStringLiteral("选择或启动游戏以查看附加组件"),
            QStringLiteral("ゲームを選択または起動してアドオンを表示")
        ));
        UpdateGameFixStatusButton();
        return;
    }

    const u64 cur_tid = m_current_addons_title_id;
    const QString cur_name = m_current_addons_game_name;
    const std::string cur_path = m_current_addons_game_path;

    auto apply_button_style = [this, cur_tid, cur_name](int file_dlc_count, int tinfoil_dlc_count) {
        if (m_current_addons_title_id != cur_tid) return;
        const QString dlc_display_text = QStringLiteral("%1 / %2").arg(file_dlc_count).arg(tinfoil_dlc_count);
        addons_status_button->setText(StormLang(
            QStringLiteral("ДОПОЛНЕНИЯ:\n%1"),
            QStringLiteral("ADD-ONS:\n%1"),
            QStringLiteral("ADD-ONS:\n%1"),
            QStringLiteral("EXTENSIONS:\n%1"),
            QStringLiteral("附加组件:\n%1"),
            QStringLiteral("アドオン:\n%1")
        ).arg(dlc_display_text));

        if (file_dlc_count >= tinfoil_dlc_count && (file_dlc_count > 0 || tinfoil_dlc_count > 0)) {
            addons_status_button->setStyleSheet(QStringLiteral(
                "QPushButton#TogglableStatusBarButton, QPushButton {"
                "  background-color: #0c2618;"
                "  color: #00e676;"
                "  border: 1px solid #00e676;"
                "  border-radius: 4px;"
                "  padding: 2px 5px;"
                "  font-size: 6.8pt;"
                "  font-weight: 800;"
                "  min-height: 26px;"
                "  min-width: 36px;"
                "}"
                "QPushButton#TogglableStatusBarButton:hover, QPushButton:hover {"
                "  background-color: #00e676;"
                "  color: #000000;"
                "}"
            ));
        } else if (file_dlc_count < tinfoil_dlc_count) {
            addons_status_button->setStyleSheet(QStringLiteral(
                "QPushButton#TogglableStatusBarButton, QPushButton {"
                "  background-color: #2e1014;"
                "  color: #ff5252;"
                "  border: 1px solid #ff5252;"
                "  border-radius: 4px;"
                "  padding: 2px 5px;"
                "  font-size: 6.8pt;"
                "  font-weight: 800;"
                "  min-height: 26px;"
                "  min-width: 36px;"
                "}"
                "QPushButton#TogglableStatusBarButton:hover, QPushButton:hover {"
                "  background-color: #ff5252;"
                "  color: #000000;"
                "}"
            ));
        } else {
            addons_status_button->setStyleSheet(QString{});
        }

        addons_status_button->setToolTip(StormLang(
            QStringLiteral("Дополнения (DLC):\n• В файле игры: %1\n• В базе Tinfoil: %2\nИгра: %3 (ID: 0x%4)\n\nНажмите для просмотра подробного списка всех дополнений и модов"),
            QStringLiteral("Add-ons (DLC):\n• In game file: %1\n• In Tinfoil database: %2\nGame: %3 (ID: 0x%4)\n\nClick to view full list of add-ons and mods"),
            QStringLiteral("Add-ons (DLC):\n• In Spieledatei: %1\n• In Tinfoil-Datenbank: %2\nSpiel: %3 (ID: 0x%4)\n\nKlicken für vollständige Liste aller Add-ons und Mods"),
            QStringLiteral("Extensions (DLC) :\n• Dans le fichier du jeu : %1\n• Dans la base Tinfoil : %2\nJeu : %3 (ID : 0x%4)\n\nCliquer pour voir la liste complète des extensions et mods"),
            QStringLiteral("附加组件 (DLC):\n• 游戏文件中: %1\n• Tinfoil 数据库中: %2\n游戏: %3 (ID: 0x%4)\n\n点击查看所有附加组件和模组详细列表"),
            QStringLiteral("アドオン (DLC):\n• ゲームファイル内: %1\n• Tinfoil データベース内: %2\nゲーム: %3 (ID: 0x%4)\n\nクリックしてすべてのアドオンとModの詳細リストを表示")
        ).arg(file_dlc_count)
         .arg(tinfoil_dlc_count)
         .arg(cur_name.isEmpty() ? StormLang(QStringLiteral("игре"), QStringLiteral("game"), QStringLiteral("Spiel"), QStringLiteral("jeu"), QStringLiteral("游戏"), QStringLiteral("ゲーム")) : cur_name,
              QStringLiteral("%1").arg(cur_tid, 16, 16, QLatin1Char('0')).toUpper()));
    };

    {
        std::lock_guard<std::mutex> lock(m_addons_cache_mutex);
        auto it = m_addons_dlc_cache.find(cur_tid);
        if (it != m_addons_dlc_cache.end()) {
            apply_button_style(it->second.first, it->second.second);
            return;
        }
    }

    // Show quick status immediately without blocking
    addons_status_button->setText(StormLang(
        QStringLiteral("ДОПОЛНЕНИЯ:\n..."),
        QStringLiteral("ADD-ONS:\n..."),
        QStringLiteral("ADD-ONS:\n..."),
        QStringLiteral("EXTENSIONS:\n..."),
        QStringLiteral("附加组件:\n..."),
        QStringLiteral("アドオン:\n...")
    ));

    // Compute in background thread
    std::thread([this, cur_tid, cur_name, cur_path, apply_button_style]() {
        std::set<u64> seen_dlc_ids;

        if (QtCommon::system) {
            const auto aoc_data = QtCommon::system->GetContentProvider().ListEntriesFilter(
                FileSys::TitleType::AOC, FileSys::ContentRecordType::Data);
            for (const auto& entry : aoc_data) {
                if ((entry.title_id & 0xFFFFFFFFFFFFF000) == (cur_tid & 0xFFFFFFFFFFFFF000) ||
                    (entry.title_id >= cur_tid + 1 && entry.title_id < cur_tid + 0x2000)) {
                    seen_dlc_ids.insert(entry.title_id);
                }
            }

            const FileSys::PatchManager patch_manager(cur_tid, QtCommon::system->GetFileSystemController(), QtCommon::system->GetContentProvider());
            const auto patches = patch_manager.GetPatches();
            for (const auto& patch : patches) {
                if (patch.type == FileSys::PatchType::DLC) {
                    const QStringList dlc_indices = QString::fromStdString(patch.version).split(QLatin1Char(','), Qt::SkipEmptyParts);
                    for (const auto& idx_str : dlc_indices) {
                        const u32 idx_val = idx_str.trimmed().toUInt();
                        const u64 generated_tid = (cur_tid & 0xFFFFFFFFFFFFF000) | (idx_val > 0 ? (0x1000 | (idx_val & 0x7FF)) : 0x1001);
                        seen_dlc_ids.insert(generated_tid);
                    }
                    if (dlc_indices.isEmpty()) {
                        seen_dlc_ids.insert((cur_tid & 0xFFFFFFFFFFFFF000) | 0x1001);
                    }
                }
            }
        }

        if (!cur_path.empty()) {
            static const QRegularExpression fn_dlc_tag{QStringLiteral(R"((\+|\b)([0-9]+)D(?:LC)?(?:\b|\)))"), QRegularExpression::CaseInsensitiveOption};
            const auto dm = fn_dlc_tag.match(QString::fromStdString(cur_path));
            if (dm.hasMatch() && dm.hasCaptured(2)) {
                const int tag_count = dm.captured(2).toInt();
                for (int i = 1; i <= tag_count; ++i) {
                    seen_dlc_ids.insert((cur_tid & 0xFFFFFFFFFFFFF000) | (0x1000 + i));
                }
            }
            if (seen_dlc_ids.empty() && QtCommon::vfs) {
                const auto game_vfs = Core::GetGameFileFromPath(QtCommon::vfs, cur_path);
                if (game_vfs) {
                    const auto nsp = std::make_shared<FileSys::NSP>(game_vfs);
                    if (nsp && nsp->GetStatus() == Loader::ResultStatus::Success) {
                        for (const auto& [nca_tid, nca_map] : nsp->GetNCAs()) {
                            if (((nca_tid & 0xFFFFFFFFFFFFF000) == (cur_tid & 0xFFFFFFFFFFFFF000) ||
                                 (nca_tid >= cur_tid + 1 && nca_tid < cur_tid + 0x2000)) &&
                                 nca_tid != cur_tid && (nca_tid & 0x800) == 0) {
                                seen_dlc_ids.insert(nca_tid);
                            }
                        }
                    }
                }
            }
        }

        const int file_dlc_count = static_cast<int>(seen_dlc_ids.size());
        TitleDB::TitleDatabase::Instance().WaitLoaded(std::chrono::milliseconds(3000));
        const int tinfoil_dlc_count = TitleDB::TitleDatabase::Instance().GetDlcCount(cur_tid);

        {
            std::lock_guard<std::mutex> lock(m_addons_cache_mutex);
            m_addons_dlc_cache[cur_tid] = {file_dlc_count, tinfoil_dlc_count};
        }

        QMetaObject::invokeMethod(this, [apply_button_style, file_dlc_count, tinfoil_dlc_count]() {
            apply_button_style(file_dlc_count, tinfoil_dlc_count);
        }, Qt::QueuedConnection);
    }).detach();
    UpdateGameFixStatusButton();
}

void MainWindow::UpdateGameFixStatusButton() {
    if (!gamefix_status_button) return;
    const auto* profile = Core::GameFixDatabase::GetProfileByTitleOrPath(
        m_current_addons_title_id, m_current_addons_game_path);
    const bool fixes_active = Core::GameFixDatabase::AreFixesEnabled() && profile != nullptr;
    if (fixes_active) {
        gamefix_status_button->setText(StormLang(
            QStringLiteral("АВТО-ФИКС:\nАКТИВЕН"),
            QStringLiteral("AUTO-FIX:\nACTIVE"),
            QStringLiteral("AUTO-FIX:\nAKTIV"),
            QStringLiteral("AUTO-FIX:\nACTIF"),
            QStringLiteral("自动修复:\n已激活"),
            QStringLiteral("自動修正:\n有効")
        ));
        gamefix_status_button->setStyleSheet(QStringLiteral(
            "QPushButton#TogglableStatusBarButton, QPushButton {"
            "  background-color: #0b2433;"
            "  color: #00e5ff;"
            "  border: 1px solid #00e5ff;"
            "  border-radius: 4px;"
            "  padding: 2px 5px;"
            "  font-size: 6.8pt;"
            "  font-weight: 800;"
            "  min-height: 26px;"
            "  min-width: 36px;"
            "}"
            "QPushButton#TogglableStatusBarButton:hover, QPushButton:hover {"
            "  background-color: #00e5ff;"
            "  color: #000000;"
            "}"
        ));
        gamefix_status_button->setToolTip(StormLang(
            QStringLiteral("🛡️ Активен профиль авто-исправления: %1\nНажмите для меню быстрых настроек (LSFG, DRS 100%, Reactive Flushing)"),
            QStringLiteral("🛡️ Active Auto-Fix profile: %1\nClick for Quick Settings menu (LSFG, DRS 100%, Reactive Flushing)"),
            QStringLiteral("🛡️ Aktives Auto-Fix-Profil: %1\nKlicken für Schnelleinstellungen"),
            QStringLiteral("🛡️ Profil Auto-Fix actif : %1\nCliquer pour les paramètres rapides"),
            QStringLiteral("🛡️ 已激活自动修复配置: %1\n点击打开快捷设置菜单"),
            QStringLiteral("🛡️ 自動修正プロファイル有効: %1\nクリックしてクイック設定メニューを表示")
        ).arg(QString::fromStdString(profile->game_name)));
    } else if (profile != nullptr) {
        gamefix_status_button->setText(StormLang(
            QStringLiteral("АВТО-ФИКС:\nДОСТУПЕН"),
            QStringLiteral("AUTO-FIX:\nREADY"),
            QStringLiteral("AUTO-FIX:\nBEREIT"),
            QStringLiteral("AUTO-FIX:\nPRÊT"),
            QStringLiteral("自动修复:\n可用"),
            QStringLiteral("自動修正:\n利用可能")
        ));
        gamefix_status_button->setStyleSheet(QString{});
        gamefix_status_button->setToolTip(StormLang(
            QStringLiteral("🛡️ Доступен профиль авто-исправления: %1\nНажмите для применения или вызова быстрых настроек"),
            QStringLiteral("🛡️ Auto-Fix profile available: %1\nClick to apply or open Quick Settings"),
            QStringLiteral("🛡️ Auto-Fix-Profil verfügbar: %1\nKlicken zum Anwenden oder für Schnelleinstellungen"),
            QStringLiteral("🛡️ Profil Auto-Fix disponible : %1\nCliquer pour appliquer ou ouvrir les paramètres rapides"),
            QStringLiteral("🛡️ 自动修复配置可用: %1\n点击应用或打开快捷设置"),
            QStringLiteral("🛡️ 自動修正プロファイル利用可能: %1\nクリックして適用またはクイック設定を開く")
        ).arg(QString::fromStdString(profile->game_name)));
    } else {
        const bool fg_on = Settings::values.frame_gen.GetValue();
        gamefix_status_button->setText(fg_on
            ? StormLang(QStringLiteral("БЫСТРЫЕ:\nLSFG ВКЛ"), QStringLiteral("QUICK:\nLSFG ON"), QStringLiteral("SCHNELL:\nLSFG EIN"), QStringLiteral("RAPIDE:\nLSFG ON"), QStringLiteral("快捷设置:\nLSFG 开启"), QStringLiteral("クイック:\nLSFG オン"))
            : StormLang(QStringLiteral("БЫСТРЫЕ:\nОПЦИИ"), QStringLiteral("QUICK:\nOPTIONS"), QStringLiteral("SCHNELL:\nOPTIONEN"), QStringLiteral("RAPIDE:\nOPTIONS"), QStringLiteral("快捷设置:\n选项"), QStringLiteral("クイック:\n設定")));
        gamefix_status_button->setStyleSheet(QString{});
        gamefix_status_button->setToolTip(StormLang(
            QStringLiteral("⚡ Меню быстрых настроек (LSFG FrameGen, DRS 100%, Reactive Flushing, GPU Readback, HDR Bloom)"),
            QStringLiteral("⚡ Quick Settings menu (LSFG FrameGen, DRS 100%, Reactive Flushing, GPU Readback, HDR Bloom)"),
            QStringLiteral("⚡ Schnelleinstellungsmenü (LSFG FrameGen, DRS 100%, Reactive Flushing, GPU Readback, HDR Bloom)"),
            QStringLiteral("⚡ Menu des paramètres rapides (LSFG FrameGen, DRS 100%, Reactive Flushing, GPU Readback, HDR Bloom)"),
            QStringLiteral("⚡ 快捷设置菜单 (LSFG 帧生成, DRS 100%, 响应式刷新, GPU 回读, HDR 泛光)"),
            QStringLiteral("⚡ クイック設定メニュー (LSFG フレーム生成, DRS 100%, リアクティブフラッシング, GPU リードバック, HDR ブルーム)")
        ));
    }
}

void MainWindow::UpdateAirplaneModeButton() {
    if (!airplane_mode_button) return;
    const bool airplane = Settings::values.airplane_mode.GetValue();
    airplane_mode_button->setText(airplane
        ? StormLang(QStringLiteral("САМОЛЁТ:\nВКЛ"), QStringLiteral("AIRPLANE:\nON"), QStringLiteral("FLUGMODUS:\nEIN"), QStringLiteral("AVION:\nACTIVÉ"), QStringLiteral("飞行模式:\n开启"), QStringLiteral("機内モード:\nオン"))
        : StormLang(QStringLiteral("САМОЛЁТ:\nВЫКЛ"), QStringLiteral("AIRPLANE:\nOFF"), QStringLiteral("FLUGMODUS:\nAUS"), QStringLiteral("AVION:\nDÉSACTIVÉ"), QStringLiteral("飞行模式:\n关闭"), QStringLiteral("機内モード:\nオフ")));
    airplane_mode_button->setToolTip(StormLang(
        QStringLiteral("Режим полёта (отключение сетевых функций Switch)"),
        QStringLiteral("Airplane mode (disable Switch network functions)"),
        QStringLiteral("Flugmodus (Switch-Netzwerkfunktionen deaktivieren)"),
        QStringLiteral("Mode avion (désactive les fonctions réseau de la Switch)"),
        QStringLiteral("飞行模式 (禁用 Switch 网络功能)"),
        QStringLiteral("機内モード (Switch ネットワーク機能を無効化)")
    ));
}

void MainWindow::UpdateVSyncText() {
    if (!vsync_mode_button) return;
    QString val_text = QStringLiteral("FIFO");
    const auto vsync = Settings::values.vsync_mode.GetValue();
    switch (vsync) {
    case Settings::VSyncMode::Immediate:
        val_text = StormLang(QStringLiteral("ВЫКЛ"), QStringLiteral("OFF"), QStringLiteral("AUS"), QStringLiteral("DÉSACTIVÉ"), QStringLiteral("关闭"), QStringLiteral("オフ"));
        break;
    case Settings::VSyncMode::Mailbox:
        val_text = QStringLiteral("MAILBOX");
        break;
    case Settings::VSyncMode::Fifo:
        val_text = QStringLiteral("FIFO");
        break;
    case Settings::VSyncMode::FifoRelaxed:
        val_text = QStringLiteral("RELAXED");
        break;
    default:
        break;
    }
    vsync_mode_button->setText(StormLang(
        QStringLiteral("VSYNC:\n%1"),
        QStringLiteral("VSYNC:\n%1"),
        QStringLiteral("VSYNC:\n%1"),
        QStringLiteral("VSYNC:\n%1"),
        QStringLiteral("垂直同步:\n%1"),
        QStringLiteral("垂直同期:\n%1")
    ).arg(val_text));
}

void MainWindow::UpdateSpeedLimitText() {
    if (!speed_limit_button) return;
    if (!Settings::values.use_speed_limit.GetValue()) {
        speed_limit_button->setText(StormLang(
            QStringLiteral("СКОРОСТЬ:\nБЕЗ ЛИМИТА"),
            QStringLiteral("SPEED:\nUNLIMITED"),
            QStringLiteral("GESCHW.:\nUNBEGRENZT"),
            QStringLiteral("VITESSE:\nILLIMITÉE"),
            QStringLiteral("速度:\n无限制"),
            QStringLiteral("速度:\n無制限")
        ));
    } else {
        const auto limit = Settings::values.speed_limit.GetValue();
        speed_limit_button->setText(StormLang(
            QStringLiteral("СКОРОСТЬ:\n%1%"),
            QStringLiteral("SPEED:\n%1%"),
            QStringLiteral("GESCHW.:\n%1%"),
            QStringLiteral("VITESSE:\n%1%"),
            QStringLiteral("速度:\n%1%"),
            QStringLiteral("速度:\n%1%")
        ).arg(limit));
    }
}

void MainWindow::UpdateNvdecText() {
    if (!nvdec_status_button) return;
    QString val_text = StormLang(QStringLiteral("ГПУ"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"));
    const auto nvdec = Settings::values.nvdec_emulation.GetValue();
    switch (nvdec) {
    case Settings::NvdecEmulation::Off:
        val_text = StormLang(QStringLiteral("ВЫКЛ"), QStringLiteral("OFF"), QStringLiteral("AUS"), QStringLiteral("DÉSACTIVÉ"), QStringLiteral("关闭"), QStringLiteral("オフ"));
        break;
    case Settings::NvdecEmulation::Cpu:
        val_text = StormLang(QStringLiteral("ЦП"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"));
        break;
    case Settings::NvdecEmulation::Gpu:
        val_text = StormLang(QStringLiteral("ГПУ"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"));
        break;
    case Settings::NvdecEmulation::Hybrid:
        val_text = StormLang(QStringLiteral("ГИБРИД"), QStringLiteral("HYBRID"), QStringLiteral("HYBRID"), QStringLiteral("HYBRIDE"), QStringLiteral("混合"), QStringLiteral("ハイブリッド"));
        break;
    default:
        break;
    }
    nvdec_status_button->setText(StormLang(
        QStringLiteral("NVDEC:\n%1"),
        QStringLiteral("NVDEC:\n%1"),
        QStringLiteral("NVDEC:\n%1"),
        QStringLiteral("NVDEC:\n%1"),
        QStringLiteral("NVDEC:\n%1"),
        QStringLiteral("NVDEC:\n%1")
    ).arg(val_text));
}

void MainWindow::UpdateCpuAccuracyText() {
    if (!cpu_accuracy_button) return;
    QString val_text = StormLang(QStringLiteral("АВТО"), QStringLiteral("AUTO"), QStringLiteral("AUTO"), QStringLiteral("AUTO"), QStringLiteral("自动"), QStringLiteral("自動"));
    const auto cpu_acc = Settings::values.cpu_accuracy.GetValue();
    switch (cpu_acc) {
    case Settings::CpuAccuracy::Auto:
        val_text = StormLang(QStringLiteral("АВТО"), QStringLiteral("AUTO"), QStringLiteral("AUTO"), QStringLiteral("AUTO"), QStringLiteral("自动"), QStringLiteral("自動"));
        break;
    case Settings::CpuAccuracy::Accurate:
        val_text = StormLang(QStringLiteral("ТОЧНО"), QStringLiteral("ACCURATE"), QStringLiteral("GENAU"), QStringLiteral("PRÉCIS"), QStringLiteral("准确"), QStringLiteral("正確"));
        break;
    case Settings::CpuAccuracy::Unsafe:
        val_text = StormLang(QStringLiteral("НЕБЕЗОПАСНО"), QStringLiteral("UNSAFE"), QStringLiteral("UNSICHER"), QStringLiteral("NON SÉCURISÉ"), QStringLiteral("不安全"), QStringLiteral("非安全"));
        break;
    default:
        break;
    }
    cpu_accuracy_button->setText(StormLang(
        QStringLiteral("ТОЧНОСТЬ ЦП:\n%1"),
        QStringLiteral("CPU ACCURACY:\n%1"),
        QStringLiteral("CPU-GENAUIGKEIT:\n%1"),
        QStringLiteral("PRÉCISION CPU:\n%1"),
        QStringLiteral("CPU 精度:\n%1"),
        QStringLiteral("CPU 精度:\n%1")
    ).arg(val_text));
}

void MainWindow::UpdateDiskCacheText() {
    if (!disk_cache_button) return;
    const bool enabled = Settings::values.use_disk_shader_cache.GetValue();
    disk_cache_button->setText(enabled
        ? StormLang(QStringLiteral("КЭШ ШЕЙДЕРОВ:\nВКЛ"), QStringLiteral("DISK CACHE:\nON"), QStringLiteral("FESTPLATTEN-CACHE:\nEIN"), QStringLiteral("CACHE DISQUE:\nACTIVÉ"), QStringLiteral("着色器缓存:\n开启"), QStringLiteral("ディスクキャッシュ:\nオン"))
        : StormLang(QStringLiteral("КЭШ ШЕЙДЕРОВ:\nВЫКЛ"), QStringLiteral("DISK CACHE:\nOFF"), QStringLiteral("FESTPLATTEN-CACHE:\nAUS"), QStringLiteral("CACHE DISQUE:\nDÉSACTIVÉ"), QStringLiteral("着色器缓存:\n关闭"), QStringLiteral("ディスクキャッシュ:\nオフ")));
}

void MainWindow::UpdateRefreshButton() {
    if (!refresh_button) return;
    refresh_button->setText(StormLang(
        QStringLiteral("СПИСОК:\nОБНОВИТЬ"),
        QStringLiteral("LIST:\nREFRESH"),
        QStringLiteral("LISTE:\nAKTUAL."),
        QStringLiteral("LISTE:\nACTUAL."),
        QStringLiteral("列表:\n刷新"),
        QStringLiteral("リスト:\n更新")
    ));
    refresh_button->setToolTip(StormLang(
        QStringLiteral("Обновить список игр"),
        QStringLiteral("Refresh game list"),
        QStringLiteral("Spielliste aktualisieren"),
        QStringLiteral("Actualiser la liste des jeux"),
        QStringLiteral("刷新游戏列表"),
        QStringLiteral("ゲームリストを更新")
    ));
}

void MainWindow::UpdateFullscreenButton() {
    if (!fullscreen_button) return;
    const bool is_full = ui->action_Fullscreen->isChecked();
    fullscreen_button->setText(is_full
        ? StormLang(QStringLiteral("ЭКРАН:\nПОЛНЫЙ"), QStringLiteral("SCREEN:\nFULL"), QStringLiteral("BILDSCHIRM:\nVOLL"), QStringLiteral("ÉCRAN:\nPLEIN"), QStringLiteral("屏幕:\n全屏"), QStringLiteral("画面:\n全画面"))
        : StormLang(QStringLiteral("ЭКРАН:\nОКНО"), QStringLiteral("SCREEN:\nWINDOW"), QStringLiteral("BILDSCHIRM:\nFENSTER"), QStringLiteral("ÉCRAN:\nFENÊTRE"), QStringLiteral("屏幕:\n窗口"), QStringLiteral("画面:\nウィンドウ")));
}

void MainWindow::UpdateMuteButton() {
    if (!mute_button) return;
    const bool muted = Settings::values.audio_muted.GetValue();
    mute_button->setText(muted
        ? StormLang(QStringLiteral("ЗВУК:\nМУТ"), QStringLiteral("MUTE:\nON"), QStringLiteral("STUMM:\nEIN"), QStringLiteral("MUET:\nACTIVÉ"), QStringLiteral("静音:\n开启"), QStringLiteral("ミュート:\nオン"))
        : StormLang(QStringLiteral("ЗВУК:\nВКЛ"), QStringLiteral("SOUND:\nON"), QStringLiteral("TON:\nEIN"), QStringLiteral("SON:\nACTIVÉ"), QStringLiteral("声音:\n开启"), QStringLiteral("サウンド:\nオン")));
}

void MainWindow::SaveFooterSettings() {
    QStringList hidden;
    for (int i = 0; i < static_cast<int>(m_status_groups.size()); ++i) {
        if (m_status_groups[i] && !m_status_groups[i]->isVisible()) {
            hidden.append(QStringLiteral("grp_%1").arg(i));
        }
    }
    const std::vector<std::pair<QString, QWidget*>> btns = {
        {QStringLiteral("refresh"), refresh_button},
        {QStringLiteral("addons"), addons_status_button},
        {QStringLiteral("fullscreen"), fullscreen_button},
        {QStringLiteral("renderer"), renderer_status_button},
        {QStringLiteral("gpu_acc"), gpu_accuracy_button},
        {QStringLiteral("cpu_acc"), cpu_accuracy_button},
        {QStringLiteral("vsync"), vsync_mode_button},
        {QStringLiteral("dma"), dma_accuracy_button},
        {QStringLiteral("gpu_fence"), gpu_fence_button},
        {QStringLiteral("nvdec"), nvdec_status_button},
        {QStringLiteral("aa"), aa_status_button},
        {QStringLiteral("filter"), filter_status_button},
        {QStringLiteral("shader"), shader_status_button},
        {QStringLiteral("aspect"), aspect_ratio_button},
        {QStringLiteral("res_scale"), res_scale_button},
        {QStringLiteral("vram"), vram_mode_button},
        {QStringLiteral("anisotropy"), anisotropy_button},
        {QStringLiteral("disk_cache"), disk_cache_button},
        {QStringLiteral("astc_decode"), astc_decode_button},
        {QStringLiteral("astc_recompress"), astc_recompress_button},
        {QStringLiteral("dock"), dock_status_button},
        {QStringLiteral("airplane"), airplane_mode_button},
        {QStringLiteral("speed_limit"), speed_limit_button},
        {QStringLiteral("volume"), volume_button},
        {QStringLiteral("mute"), mute_button},
        {QStringLiteral("firmware"), firmware_label},
    };
    for (const auto& [id, w] : btns) {
        if (w && !w->isVisible()) {
            hidden.append(id);
        }
    }
    UISettings::values.hidden_footer_items.SetValue(hidden.join(QLatin1Char(';')).toStdString());
}

void MainWindow::LoadFooterSettings() {
    const std::string saved = UISettings::values.hidden_footer_items.GetValue();
    if (saved.empty()) return;
    const QStringList hidden = QString::fromStdString(saved).split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (int i = 0; i < static_cast<int>(m_status_groups.size()); ++i) {
        if (m_status_groups[i] && hidden.contains(QStringLiteral("grp_%1").arg(i))) {
            m_status_groups[i]->setVisible(false);
        }
    }
    const std::vector<std::pair<QString, QWidget*>> btns = {
        {QStringLiteral("refresh"), refresh_button},
        {QStringLiteral("addons"), addons_status_button},
        {QStringLiteral("fullscreen"), fullscreen_button},
        {QStringLiteral("renderer"), renderer_status_button},
        {QStringLiteral("gpu_acc"), gpu_accuracy_button},
        {QStringLiteral("cpu_acc"), cpu_accuracy_button},
        {QStringLiteral("vsync"), vsync_mode_button},
        {QStringLiteral("dma"), dma_accuracy_button},
        {QStringLiteral("gpu_fence"), gpu_fence_button},
        {QStringLiteral("nvdec"), nvdec_status_button},
        {QStringLiteral("aa"), aa_status_button},
        {QStringLiteral("filter"), filter_status_button},
        {QStringLiteral("shader"), shader_status_button},
        {QStringLiteral("aspect"), aspect_ratio_button},
        {QStringLiteral("res_scale"), res_scale_button},
        {QStringLiteral("vram"), vram_mode_button},
        {QStringLiteral("anisotropy"), anisotropy_button},
        {QStringLiteral("disk_cache"), disk_cache_button},
        {QStringLiteral("astc_decode"), astc_decode_button},
        {QStringLiteral("astc_recompress"), astc_recompress_button},
        {QStringLiteral("dock"), dock_status_button},
        {QStringLiteral("airplane"), airplane_mode_button},
        {QStringLiteral("speed_limit"), speed_limit_button},
        {QStringLiteral("volume"), volume_button},
        {QStringLiteral("mute"), mute_button},
        {QStringLiteral("firmware"), firmware_label},
    };
    for (const auto& [id, w] : btns) {
        if (w && hidden.contains(id)) {
            w->setVisible(false);
        }
    }
}

void MainWindow::ShowFooterCustomizeMenu() {
    QMenu context_menu(this);
    context_menu.setTitle(StormLang(
        QStringLiteral("Настройка панели подвала"),
        QStringLiteral("Footer panel customization"),
        QStringLiteral("Anpassung der Fußzeile"),
        QStringLiteral("Personnalisation du pied de page"),
        QStringLiteral("底部栏设置"),
        QStringLiteral("フッターパネルの設定")
    ));

    auto* header_act = context_menu.addAction(StormLang(
        QStringLiteral("--- РАЗДЕЛЫ ПОДВАЛА ---"),
        QStringLiteral("--- FOOTER SECTIONS ---"),
        QStringLiteral("--- FUSSZEILEN-ABSCHNITTE ---"),
        QStringLiteral("--- SECTIONS DU PIED DE PAGE ---"),
        QStringLiteral("--- 底部栏分区 ---"),
        QStringLiteral("--- フッターセクション ---")
    ));
    header_act->setEnabled(false);

    struct GroupInfo {
        QString name;
        QWidget* widget;
    };

    const std::vector<GroupInfo> groups = {
        {StormLang(QStringLiteral("Раздел: УПРАВЛЕНИЕ"), QStringLiteral("Section: CONTROLS"), QStringLiteral("Abschnitt: STEUERUNG"), QStringLiteral("Section : COMMANDES"), QStringLiteral("分区: 控制"), QStringLiteral("セクション: 操作")), m_status_groups.size() > 0 ? m_status_groups[0] : nullptr},
        {StormLang(QStringLiteral("Раздел: ДОПОЛНЕНИЯ"), QStringLiteral("Section: ADD-ONS"), QStringLiteral("Abschnitt: ADD-ONS"), QStringLiteral("Section : ADD-ONS"), QStringLiteral("分区: 附加内容"), QStringLiteral("セクション: アドオン")), m_status_groups.size() > 1 ? m_status_groups[1] : nullptr},
        {StormLang(QStringLiteral("Раздел: РЕНДЕР"), QStringLiteral("Section: RENDER"), QStringLiteral("Abschnitt: RENDER"), QStringLiteral("Section : RENDU"), QStringLiteral("分区: 渲染"), QStringLiteral("セクション: レンダラー")), m_status_groups.size() > 2 ? m_status_groups[2] : nullptr},
        {StormLang(QStringLiteral("Раздел: ГРАФИКА"), QStringLiteral("Section: GRAPHICS"), QStringLiteral("Abschnitt: GRAFIK"), QStringLiteral("Section : GRAPHISMES"), QStringLiteral("分区: 图形"), QStringLiteral("セクション: グラフィックス")), m_status_groups.size() > 3 ? m_status_groups[3] : nullptr},
        {StormLang(QStringLiteral("Раздел: ASTC"), QStringLiteral("Section: ASTC"), QStringLiteral("Abschnitt: ASTC"), QStringLiteral("Section : ASTC"), QStringLiteral("分区: ASTC"), QStringLiteral("セクション: ASTC")), m_status_groups.size() > 4 ? m_status_groups[4] : nullptr},
        {StormLang(QStringLiteral("Раздел: РЕЖИМ"), QStringLiteral("Section: MODE"), QStringLiteral("Abschnitt: MODUS"), QStringLiteral("Section : MODE"), QStringLiteral("分区: 模式"), QStringLiteral("セクション: モード")), m_status_groups.size() > 5 ? m_status_groups[5] : nullptr},
        {StormLang(QStringLiteral("Раздел: СИСТЕМА"), QStringLiteral("Section: SYSTEM"), QStringLiteral("Abschnitt: SYSTEM"), QStringLiteral("Section : SYSTÈME"), QStringLiteral("分区: 系统"), QStringLiteral("セクション: システム")), m_status_groups.size() > 6 ? m_status_groups[6] : nullptr},
        {StormLang(QStringLiteral("Раздел: СЕТЬ"), QStringLiteral("Section: NETWORK"), QStringLiteral("Abschnitt: NETZWERK"), QStringLiteral("Section : RÉSEAU"), QStringLiteral("分区: 网络"), QStringLiteral("セクション: ネットワーク")), m_status_groups.size() > 7 ? m_status_groups[7] : nullptr},
    };

    for (const auto& g : groups) {
        if (!g.widget) continue;
        auto* act = context_menu.addAction(g.name, [this, g] {
            g.widget->setVisible(!g.widget->isVisible());
            SaveFooterSettings();
        });
        act->setCheckable(true);
        act->setChecked(g.widget->isVisible());
    }

    context_menu.addSeparator();
    auto* btn_header = context_menu.addAction(StormLang(
        QStringLiteral("--- ОТДЕЛЬНЫЕ КНОПКИ ---"),
        QStringLiteral("--- INDIVIDUAL BUTTONS ---"),
        QStringLiteral("--- EINZELNE SCHALTFLÄCHEN ---"),
        QStringLiteral("--- BOUTONS INDIVIDUELS ---"),
        QStringLiteral("--- 单独按钮 ---"),
        QStringLiteral("--- 個別ボタン ---")
    ));
    btn_header->setEnabled(false);

    struct ButtonInfo {
        QString name;
        QWidget* widget;
    };

    const std::vector<ButtonInfo> buttons = {
        {StormLang(QStringLiteral("Обновить список"), QStringLiteral("Refresh list"), QStringLiteral("Liste aktualisieren"), QStringLiteral("Actualiser la liste"), QStringLiteral("刷新列表"), QStringLiteral("リストを更新")), refresh_button},
        {StormLang(QStringLiteral("Дополнения"), QStringLiteral("Add-ons"), QStringLiteral("Add-ons"), QStringLiteral("Add-ons"), QStringLiteral("附加内容"), QStringLiteral("アドオン")), addons_status_button},
        {StormLang(QStringLiteral("Полный экран"), QStringLiteral("Fullscreen"), QStringLiteral("Vollbild"), QStringLiteral("Plein écran"), QStringLiteral("全屏"), QStringLiteral("全画面")), fullscreen_button},
        {StormLang(QStringLiteral("Рендер API"), QStringLiteral("Render API"), QStringLiteral("Render-API"), QStringLiteral("API de rendu"), QStringLiteral("渲染 API"), QStringLiteral("レンダー API")), renderer_status_button},
        {StormLang(QStringLiteral("Точность ГПУ"), QStringLiteral("GPU accuracy"), QStringLiteral("GPU-Genauigkeit"), QStringLiteral("Précision GPU"), QStringLiteral("GPU 精度"), QStringLiteral("GPU 精度")), gpu_accuracy_button},
        {StormLang(QStringLiteral("Точность ЦП"), QStringLiteral("CPU accuracy"), QStringLiteral("CPU-Genauigkeit"), QStringLiteral("Précision CPU"), QStringLiteral("CPU 精度"), QStringLiteral("CPU 精度")), cpu_accuracy_button},
        {StormLang(QStringLiteral("VSync"), QStringLiteral("VSync"), QStringLiteral("VSync"), QStringLiteral("VSync"), QStringLiteral("垂直同步"), QStringLiteral("垂直同期")), vsync_mode_button},
        {StormLang(QStringLiteral("DMA"), QStringLiteral("DMA"), QStringLiteral("DMA"), QStringLiteral("DMA"), QStringLiteral("DMA"), QStringLiteral("DMA")), dma_accuracy_button},
        {StormLang(QStringLiteral("Барьеры ГПУ"), QStringLiteral("GPU fences"), QStringLiteral("GPU-Fences"), QStringLiteral("Barrières GPU"), QStringLiteral("GPU 栅栏"), QStringLiteral("GPU フェンス")), gpu_fence_button},
        {StormLang(QStringLiteral("NVDEC"), QStringLiteral("NVDEC"), QStringLiteral("NVDEC"), QStringLiteral("NVDEC"), QStringLiteral("NVDEC"), QStringLiteral("NVDEC")), nvdec_status_button},
        {StormLang(QStringLiteral("Сглаживание"), QStringLiteral("Anti-aliasing"), QStringLiteral("Kantenglättung"), QStringLiteral("Anticrénelage"), QStringLiteral("抗锯齿"), QStringLiteral("アンチエイリアス")), aa_status_button},
        {StormLang(QStringLiteral("Фильтр масштабирования"), QStringLiteral("Scaling filter"), QStringLiteral("Skalierungsfilter"), QStringLiteral("Filtre de mise à l'échelle"), QStringLiteral("缩放过滤器"), QStringLiteral("スケーリングフィルター")), filter_status_button},
        {StormLang(QStringLiteral("Соотношение сторон"), QStringLiteral("Aspect ratio"), QStringLiteral("Seitenverhältnis"), QStringLiteral("Format d'image"), QStringLiteral("宽高比"), QStringLiteral("アスペクト比")), aspect_ratio_button},
        {StormLang(QStringLiteral("Масштаб разрешения"), QStringLiteral("Resolution scale"), QStringLiteral("Auflösungsskalierung"), QStringLiteral("Échelle de résolution"), QStringLiteral("分辨率缩放"), QStringLiteral("解像度スケール")), res_scale_button},
        {StormLang(QStringLiteral("VRAM"), QStringLiteral("VRAM"), QStringLiteral("VRAM"), QStringLiteral("VRAM"), QStringLiteral("显存"), QStringLiteral("ビデオメモリ")), vram_mode_button},
        {StormLang(QStringLiteral("Анизотропия"), QStringLiteral("Anisotropy"), QStringLiteral("Anisotropie"), QStringLiteral("Anisotropie"), QStringLiteral("各向异性"), QStringLiteral("異方性")), anisotropy_button},
        {StormLang(QStringLiteral("Кэш шейдеров"), QStringLiteral("Shader cache"), QStringLiteral("Shader-Cache"), QStringLiteral("Cache de shaders"), QStringLiteral("着色器缓存"), QStringLiteral("シェーダーキャッシュ")), disk_cache_button},
        {StormLang(QStringLiteral("Декод. ASTC"), QStringLiteral("ASTC decode"), QStringLiteral("ASTC-Dekodierung"), QStringLiteral("Décodage ASTC"), QStringLiteral("ASTC 解码"), QStringLiteral("ASTC デコード")), astc_decode_button},
        {StormLang(QStringLiteral("Пересж. ASTC"), QStringLiteral("ASTC recompress"), QStringLiteral("ASTC-Rekompression"), QStringLiteral("Recompression ASTC"), QStringLiteral("ASTC 重压缩"), QStringLiteral("ASTC 再圧縮")), astc_recompress_button},
        {StormLang(QStringLiteral("Режим ТВ / Портал"), QStringLiteral("Docked / Handheld"), QStringLiteral("Dock- / Handheld-Modus"), QStringLiteral("Mode TV / Portable"), QStringLiteral("底座 / 掌机模式"), QStringLiteral("ドック / 携帯モード")), dock_status_button},
        {StormLang(QStringLiteral("Режим полёта"), QStringLiteral("Airplane mode"), QStringLiteral("Flugmodus"), QStringLiteral("Mode avion"), QStringLiteral("飞行模式"), QStringLiteral("機内モード")), airplane_mode_button},
        {StormLang(QStringLiteral("Лимит скорости"), QStringLiteral("Speed limit"), QStringLiteral("Geschwindigkeitsbegrenzung"), QStringLiteral("Limite de vitesse"), QStringLiteral("速度限制"), QStringLiteral("速度制限")), speed_limit_button},
        {StormLang(QStringLiteral("Громкость"), QStringLiteral("Volume"), QStringLiteral("Lautstärke"), QStringLiteral("Volume"), QStringLiteral("音量"), QStringLiteral("音量")), volume_button},
        {StormLang(QStringLiteral("Отключение звука"), QStringLiteral("Mute sound"), QStringLiteral("Stummschaltung"), QStringLiteral("Couper le son"), QStringLiteral("静音"), QStringLiteral("消音")), mute_button},
        {StormLang(QStringLiteral("Прошивка"), QStringLiteral("Firmware"), QStringLiteral("Firmware"), QStringLiteral("Firmware"), QStringLiteral("固件"), QStringLiteral("ファームウェア")), firmware_label},
    };

    for (const auto& b : buttons) {
        if (!b.widget) continue;
        auto* act = context_menu.addAction(b.name, [this, b] {
            b.widget->setVisible(!b.widget->isVisible());
            SaveFooterSettings();
        });
        act->setCheckable(true);
        act->setChecked(b.widget->isVisible());
    }

    context_menu.addSeparator();
    context_menu.addAction(StormLang(
        QStringLiteral("Показать все разделы и элементы"),
        QStringLiteral("Show all sections and elements"),
        QStringLiteral("Alle Abschnitte und Elemente anzeigen"),
        QStringLiteral("Afficher toutes les sections et éléments"),
        QStringLiteral("显示所有分区和元素"),
        QStringLiteral("すべてのセクションと要素を表示")
    ), [this, groups, buttons] {
        for (const auto& g : groups) {
            if (g.widget) g.widget->setVisible(true);
        }
        for (const auto& b : buttons) {
            if (b.widget) b.widget->setVisible(true);
        }
        SaveFooterSettings();
    });

    context_menu.exec(QCursor::pos());
}

void MainWindow::ShowMenuAtWidget(QMenu& menu, QWidget* widget) {
    if (!widget) {
        menu.exec(QCursor::pos());
        return;
    }
    menu.ensurePolished();
    const QSize menu_size = menu.sizeHint();
    const QPoint global_pos = widget->mapToGlobal(QPoint(0, 0));
    int x = global_pos.x();
    int y = global_pos.y() - menu_size.height();

    const auto* screen = widget->screen();
    if (screen) {
        const QRect screen_geo = screen->availableGeometry();
        if (y < screen_geo.top()) {
            y = global_pos.y() + widget->height();
        }
        if (x + menu_size.width() > screen_geo.right()) {
            x = screen_geo.right() - menu_size.width();
        }
        if (x < screen_geo.left()) {
            x = screen_geo.left();
        }
    }
    menu.exec(QPoint(x, y));
}

void MainWindow::ShowGroupMenu(int group_index, QWidget* group_widget) {
    if (group_index == 1) {
        if (m_current_addons_title_id == 0 && game_list) {
            const auto [tid, path] = game_list->GetSelectedGameInfo();
            if (tid != 0) {
                m_current_addons_title_id = tid;
                m_current_addons_game_path = path.toStdString();
                m_current_addons_game_name = QFileInfo(path).completeBaseName();
                UpdateAddonsStatusButton(m_current_addons_title_id, m_current_addons_game_name);
            }
        }
        ShowDLCDialog(m_current_addons_title_id, m_current_addons_game_name);
        return;
    }
    QMenu context_menu(this);
    if (group_index == 4) {
        auto* header_act = context_menu.addAction(StormLang(
            QStringLiteral("🎨 Управление текстурами ASTC"),
            QStringLiteral("🎨 ASTC Texture Management"),
            QStringLiteral("🎨 ASTC-Texturverwaltung"),
            QStringLiteral("🎨 Gestion des textures ASTC"),
            QStringLiteral("🎨 ASTC 纹理管理"),
            QStringLiteral("🎨 ASTC テクスチャ管理")
        ));
        header_act->setEnabled(false);
        context_menu.addSeparator();

        auto* decode_menu = context_menu.addMenu(StormLang(
            QStringLiteral("⚙️ Метод декодирования ASTC"),
            QStringLiteral("⚙️ ASTC Decode Method"),
            QStringLiteral("⚙️ ASTC-Dekodiermethode"),
            QStringLiteral("⚙️ Méthode de décodage ASTC"),
            QStringLiteral("⚙️ ASTC 解码方式"),
            QStringLiteral("⚙️ ASTC デコード方法")
        ));
        const auto cur_dec = Settings::values.accelerate_astc.GetValue();
        const std::vector<std::pair<Settings::AstcDecodeMode, QString>> dec_options = {
            {Settings::AstcDecodeMode::CpuAsynchronous, StormLang(QStringLiteral("ЦП асинхронно (рекомендуется)"), QStringLiteral("CPU Asynchronous (Recommended)"), QStringLiteral("CPU asynchron (empfohlen)"), QStringLiteral("CPU asynchrone (recommandé)"), QStringLiteral("CPU 异步 (推荐)"), QStringLiteral("CPU 非同期 (推奨)"))},
            {Settings::AstcDecodeMode::Cpu, StormLang(QStringLiteral("ЦП (синхронно)"), QStringLiteral("CPU (Synchronous)"), QStringLiteral("CPU (synchron)"), QStringLiteral("CPU (synchrone)"), QStringLiteral("CPU (同步)"), QStringLiteral("CPU (同期)"))},
            {Settings::AstcDecodeMode::Gpu, StormLang(QStringLiteral("ГПУ (аппаратное декодирование)"), QStringLiteral("GPU (Hardware Decoding)"), QStringLiteral("GPU (Hardware-Dekodierung)"), QStringLiteral("GPU (décodage matériel)"), QStringLiteral("GPU (硬件解码)"), QStringLiteral("GPU (ハードウェアデコード)"))},
            {Settings::AstcDecodeMode::Hybrid, StormLang(QStringLiteral("Гибридный (ГПУ и ЦП)"), QStringLiteral("Hybrid (GPU and CPU)"), QStringLiteral("Hybrid (GPU und CPU)"), QStringLiteral("Hybride (GPU et CPU)"), QStringLiteral("混合 (GPU 和 CPU)"), QStringLiteral("ハイブリッド (GPU と CPU)"))},
        };
        for (const auto& opt : dec_options) {
            auto* act = decode_menu->addAction(opt.second, [this, opt] {
                ApplyStatusBarSwitchableSetting(Settings::values.accelerate_astc, opt.first,
                                                m_session_backup, m_session_backup.accelerate_astc,
                                                current_game_path);
                UpdateAstcDecodeText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(opt.first == cur_dec);
        }

        auto* recomp_menu = context_menu.addMenu(StormLang(
            QStringLiteral("📦 Пересжатие ASTC"),
            QStringLiteral("📦 ASTC Recompression"),
            QStringLiteral("📦 ASTC-Rekompression"),
            QStringLiteral("📦 Recompression ASTC"),
            QStringLiteral("📦 ASTC 重新压缩"),
            QStringLiteral("📦 ASTC 再圧縮")
        ));
        const auto cur_rec = Settings::values.astc_recompression.GetValue();
        const std::vector<std::pair<Settings::AstcRecompression, QString>> rec_options = {
            {Settings::AstcRecompression::Uncompressed, StormLang(QStringLiteral("Без сжатия (лучшее качество)"), QStringLiteral("Uncompressed (Best Quality)"), QStringLiteral("Unkomprimiert (beste Qualität)"), QStringLiteral("Non compressé (meilleure qualité)"), QStringLiteral("未压缩 (最高质量)"), QStringLiteral("非圧縮 (最高品質)"))},
            {Settings::AstcRecompression::Bc1, StormLang(QStringLiteral("BC1 (низкое качество)"), QStringLiteral("BC1 (Low Quality)"), QStringLiteral("BC1 (niedrige Qualität)"), QStringLiteral("BC1 (basse qualité)"), QStringLiteral("BC1 (低质量)"), QStringLiteral("BC1 (低品質)"))},
            {Settings::AstcRecompression::Bc3, StormLang(QStringLiteral("BC3 (среднее качество)"), QStringLiteral("BC3 (Medium Quality)"), QStringLiteral("BC3 (mittlere Qualität)"), QStringLiteral("BC3 (qualité moyenne)"), QStringLiteral("BC3 (中质量)"), QStringLiteral("BC3 (中品質)"))},
            {Settings::AstcRecompression::Bc5, StormLang(QStringLiteral("BC5 (высокое качество)"), QStringLiteral("BC5 (High Quality)"), QStringLiteral("BC5 (hohe Qualität)"), QStringLiteral("BC5 (haute qualité)"), QStringLiteral("BC5 (高质量)"), QStringLiteral("BC5 (高品質)"))},
        };
        for (const auto& opt : rec_options) {
            auto* act = recomp_menu->addAction(opt.second, [this, opt] {
                ApplyStatusBarSwitchableSetting(Settings::values.astc_recompression, opt.first,
                                                m_session_backup, m_session_backup.astc_recompression,
                                                current_game_path);
                UpdateAstcRecompressText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(opt.first == cur_rec);
        }
    } else if (group_index == 2) {
        auto* header_act = context_menu.addAction(StormLang(
            QStringLiteral("⚡ Параметры рендера"),
            QStringLiteral("⚡ Render Settings"),
            QStringLiteral("⚡ Rendereinstellungen"),
            QStringLiteral("⚡ Paramètres de rendu"),
            QStringLiteral("⚡ 渲染设置"),
            QStringLiteral("⚡ レンダラー設定")
        ));
        header_act->setEnabled(false);
        context_menu.addSeparator();

        auto* api_menu = context_menu.addMenu(StormLang(
            QStringLiteral("🎮 Графический API"),
            QStringLiteral("🎮 Graphics API"),
            QStringLiteral("🎮 Grafik-API"),
            QStringLiteral("🎮 API graphique"),
            QStringLiteral("🎮 图形 API"),
            QStringLiteral("🎮 グラフィックス API")
        ));
        const auto cur_api = Settings::values.renderer_backend.GetValue();
        const std::vector<std::pair<Settings::RendererBackend, QString>> api_options = {
            {Settings::RendererBackend::Vulkan, QStringLiteral("Vulkan")},
            {Settings::RendererBackend::OpenGL_GLSL, QStringLiteral("OpenGL GLSL")},
            {Settings::RendererBackend::OpenGL_SPIRV, QStringLiteral("OpenGL SPIRV")},
            {Settings::RendererBackend::OpenGL_GLASM, QStringLiteral("OpenGL GLASM")},
        };
        for (const auto& pair : api_options) {
            auto* act = api_menu->addAction(pair.second, [this, pair] {
                ApplyStatusBarSwitchableSettingNoBackup(Settings::values.renderer_backend, pair.first,
                                                        current_game_path);
                UpdateAPIText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(pair.first == cur_api);
        }

        auto* gpu_acc_menu = context_menu.addMenu(StormLang(
            QStringLiteral("🎯 Точность GPU"),
            QStringLiteral("🎯 GPU Accuracy"),
            QStringLiteral("🎯 GPU-Genauigkeit"),
            QStringLiteral("🎯 Précision GPU"),
            QStringLiteral("🎯 GPU 精度"),
            QStringLiteral("🎯 GPU 精度")
        ));
        const auto cur_gpu_acc = Settings::values.gpu_accuracy.GetValue();
        const std::vector<std::pair<Settings::GpuAccuracy, QString>> gpu_acc_options = {
            {Settings::GpuAccuracy::Low, StormLang(QStringLiteral("Быстрый (низкая точность)"), QStringLiteral("Fast (low accuracy)"), QStringLiteral("Schnell (niedrige Genauigkeit)"), QStringLiteral("Rapide (faible précision)"), QStringLiteral("快速 (低精度)"), QStringLiteral("高速 (低精度)"))},
            {Settings::GpuAccuracy::High, StormLang(QStringLiteral("Высокая точность"), QStringLiteral("High accuracy"), QStringLiteral("Hohe Genauigkeit"), QStringLiteral("Haute précision"), QStringLiteral("高精度"), QStringLiteral("高精度"))},
        };
        for (const auto& pair : gpu_acc_options) {
            auto* act = gpu_acc_menu->addAction(pair.second, [this, pair] {
                ApplyStatusBarSwitchableSetting(Settings::values.gpu_accuracy, pair.first,
                                                m_session_backup, m_session_backup.gpu_accuracy,
                                                current_game_path);
                UpdateGPUAccuracyButton();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(pair.first == cur_gpu_acc);
        }

        auto* cpu_acc_menu = context_menu.addMenu(StormLang(
            QStringLiteral("🧠 Точность CPU"),
            QStringLiteral("🧠 CPU Accuracy"),
            QStringLiteral("🧠 CPU-Genauigkeit"),
            QStringLiteral("🧠 Précision CPU"),
            QStringLiteral("🧠 CPU 精度"),
            QStringLiteral("🧠 CPU 精度")
        ));
        const auto cur_cpu = Settings::values.cpu_accuracy.GetValue();
        const std::vector<std::pair<Settings::CpuAccuracy, QString>> cpu_options = {
            {Settings::CpuAccuracy::Auto, StormLang(QStringLiteral("Авто"), QStringLiteral("Auto"), QStringLiteral("Automatisch"), QStringLiteral("Auto"), QStringLiteral("自动"), QStringLiteral("自動"))},
            {Settings::CpuAccuracy::Accurate, StormLang(QStringLiteral("Точно"), QStringLiteral("Accurate"), QStringLiteral("Genau"), QStringLiteral("Précis"), QStringLiteral("准确"), QStringLiteral("正確"))},
            {Settings::CpuAccuracy::Unsafe, StormLang(QStringLiteral("Небезопасно"), QStringLiteral("Unsafe"), QStringLiteral("Unsicher"), QStringLiteral("Non sécurisé"), QStringLiteral("不安全"), QStringLiteral("非安全"))},
        };
        for (const auto& opt : cpu_options) {
            auto* act = cpu_acc_menu->addAction(opt.second, [this, opt] {
                ApplyStatusBarSwitchableSetting(Settings::values.cpu_accuracy, opt.first,
                                                m_session_backup, m_session_backup.cpu_accuracy,
                                                current_game_path);
                UpdateCpuAccuracyText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(opt.first == cur_cpu);
        }

        auto* vsync_menu = context_menu.addMenu(StormLang(
            QStringLiteral("⏱️ Синхронизация кадров"),
            QStringLiteral("⏱️ Frame Synchronization"),
            QStringLiteral("⏱️ Bildsynchronisation"),
            QStringLiteral("⏱️ Synchronisation des images"),
            QStringLiteral("⏱️ 帧同步"),
            QStringLiteral("⏱️ フレーム同期")
        ));
        const auto cur_vsync = Settings::values.vsync_mode.GetValue();
        const std::vector<std::pair<Settings::VSyncMode, QString>> vsync_options = {
            {Settings::VSyncMode::Fifo, QStringLiteral("FIFO")},
            {Settings::VSyncMode::FifoRelaxed, QStringLiteral("FIFO Relaxed")},
            {Settings::VSyncMode::Mailbox, QStringLiteral("Mailbox")},
            {Settings::VSyncMode::Immediate, StormLang(QStringLiteral("Выключено (Immediate)"), QStringLiteral("Immediate (Off)"), QStringLiteral("Aus (Immediate)"), QStringLiteral("Désactivé (Immediate)"), QStringLiteral("关闭 (Immediate)"), QStringLiteral("オフ (Immediate)"))},
        };
        for (const auto& opt : vsync_options) {
            auto* act = vsync_menu->addAction(opt.second, [this, opt] {
                ApplyStatusBarSwitchableSettingNoBackup(Settings::values.vsync_mode, opt.first,
                                                        current_game_path);
                UpdateVSyncText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(opt.first == cur_vsync);
        }

        auto* nvdec_menu = context_menu.addMenu(StormLang(
            QStringLiteral("🎬 Декодирование видео"),
            QStringLiteral("🎬 Video Decoding"),
            QStringLiteral("🎬 Videodekodierung"),
            QStringLiteral("🎬 Décodage vidéo"),
            QStringLiteral("🎬 视频解码"),
            QStringLiteral("🎬 ビデオデコード")
        ));
        const auto cur_nvdec = Settings::values.nvdec_emulation.GetValue();
        const std::vector<std::pair<Settings::NvdecEmulation, QString>> nvdec_options = {
            {Settings::NvdecEmulation::Gpu, StormLang(QStringLiteral("ГПУ"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"), QStringLiteral("GPU"))},
            {Settings::NvdecEmulation::Hybrid, StormLang(QStringLiteral("Гибридный"), QStringLiteral("Hybrid"), QStringLiteral("Hybrid"), QStringLiteral("Hybride"), QStringLiteral("混合"), QStringLiteral("ハイブリッド"))},
            {Settings::NvdecEmulation::Cpu, StormLang(QStringLiteral("ЦП"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"))},
            {Settings::NvdecEmulation::Off, StormLang(QStringLiteral("Отключено"), QStringLiteral("Disabled"), QStringLiteral("Deaktiviert"), QStringLiteral("Désactivé"), QStringLiteral("已禁用"), QStringLiteral("無効"))},
        };
        for (const auto& opt : nvdec_options) {
            auto* act = nvdec_menu->addAction(opt.second, [this, opt] {
                ApplyStatusBarSwitchableSetting(Settings::values.nvdec_emulation, opt.first,
                                                m_session_backup, m_session_backup.nvdec_emulation,
                                                current_game_path);
                UpdateNvdecText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(opt.first == cur_nvdec);
        }

        auto* dma_menu = context_menu.addMenu(StormLang(
            QStringLiteral("⚡ Точность DMA"),
            QStringLiteral("⚡ DMA Accuracy"),
            QStringLiteral("⚡ DMA-Genauigkeit"),
            QStringLiteral("⚡ Précision DMA"),
            QStringLiteral("⚡ DMA 精度"),
            QStringLiteral("⚡ DMA 精度")
        ));
        const auto cur_dma = Settings::values.dma_accuracy.GetValue();
        const std::vector<std::pair<Settings::DmaAccuracy, QString>> dma_options = {
            {Settings::DmaAccuracy::Default, StormLang(QStringLiteral("По умолчанию"), QStringLiteral("Default"), QStringLiteral("Standard"), QStringLiteral("Par défaut"), QStringLiteral("默认"), QStringLiteral("デフォルト"))},
            {Settings::DmaAccuracy::Normal, StormLang(QStringLiteral("Нормальная"), QStringLiteral("Normal"), QStringLiteral("Normal"), QStringLiteral("Normal"), QStringLiteral("正常"), QStringLiteral("通常"))},
            {Settings::DmaAccuracy::Unsafe, StormLang(QStringLiteral("Небезопасная"), QStringLiteral("Unsafe"), QStringLiteral("Unsicher"), QStringLiteral("Non sécurisée"), QStringLiteral("不安全"), QStringLiteral("非安全"))},
            {Settings::DmaAccuracy::Safe, StormLang(QStringLiteral("Безопасная"), QStringLiteral("Safe"), QStringLiteral("Sicher"), QStringLiteral("Sécurisée"), QStringLiteral("安全"), QStringLiteral("安全"))},
        };
        for (const auto& item : dma_options) {
            auto* act = dma_menu->addAction(item.second, [this, item] {
                ApplyStatusBarSwitchableSetting(Settings::values.dma_accuracy, item.first,
                                                m_session_backup, m_session_backup.dma_accuracy,
                                                current_game_path);
                UpdateDmaText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(item.first == cur_dma);
        }

        auto* fence_menu = context_menu.addMenu(StormLang(
            QStringLiteral("🛡️ Поведение барьеров ГПУ"),
            QStringLiteral("🛡️ GPU Fence Behavior"),
            QStringLiteral("🛡️ GPU-Fence-Verhalten"),
            QStringLiteral("🛡️ Comportement des barrières GPU"),
            QStringLiteral("🛡️ GPU 栅栏行为"),
            QStringLiteral("🛡️ GPU フェンス動作")
        ));
        const auto cur_fence = Settings::values.gpu_fence_behavior.GetValue();
        const std::vector<std::pair<Settings::GpuFenceBehavior, QString>> fence_options = {
            {Settings::GpuFenceBehavior::Default, StormLang(QStringLiteral("По умолчанию"), QStringLiteral("Default"), QStringLiteral("Standard"), QStringLiteral("Par défaut"), QStringLiteral("默认"), QStringLiteral("デフォルト"))},
            {Settings::GpuFenceBehavior::Immediate, StormLang(QStringLiteral("Мгновенно"), QStringLiteral("Immediate"), QStringLiteral("Sofort"), QStringLiteral("Immédiat"), QStringLiteral("立即"), QStringLiteral("即時"))},
            {Settings::GpuFenceBehavior::Balanced, StormLang(QStringLiteral("Сбалансированно"), QStringLiteral("Balanced"), QStringLiteral("Ausgeglichen"), QStringLiteral("Équilibré"), QStringLiteral("平衡"), QStringLiteral("バランス"))},
            {Settings::GpuFenceBehavior::Accurate, StormLang(QStringLiteral("Точно"), QStringLiteral("Accurate"), QStringLiteral("Genau"), QStringLiteral("Précis"), QStringLiteral("准确"), QStringLiteral("正確"))},
            {Settings::GpuFenceBehavior::Strict, StormLang(QStringLiteral("Строго"), QStringLiteral("Strict"), QStringLiteral("Strikt"), QStringLiteral("Strict"), QStringLiteral("严格"), QStringLiteral("厳格"))},
        };
        for (const auto& item : fence_options) {
            auto* act = fence_menu->addAction(item.second, [this, item] {
                ApplyStatusBarSwitchableSetting(Settings::values.gpu_fence_behavior, item.first,
                                                m_session_backup, m_session_backup.gpu_fence_behavior,
                                                current_game_path);
                UpdateGpuFenceText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(item.first == cur_fence);
        }

        auto* pacing_menu = context_menu.addMenu(StormLang(
            QStringLiteral("⏱️ Тайминг кадров"),
            QStringLiteral("⏱️ Frame Pacing"),
            QStringLiteral("⏱️ Frame-Pacing"),
            QStringLiteral("⏱️ Synchronisation des images"),
            QStringLiteral("⏱️ 帧步调"),
            QStringLiteral("⏱️ フレームペーシング")
        ));
        const auto cur_pacing = Settings::values.frame_pacing_mode.GetValue();
        const std::vector<std::pair<Settings::FramePacingMode, QString>> pacing_options = {
            {Settings::FramePacingMode::Target_Auto, StormLang(QStringLiteral("Автоматически"), QStringLiteral("Auto"), QStringLiteral("Automatisch"), QStringLiteral("Automatique"), QStringLiteral("自动"), QStringLiteral("自動"))},
            {Settings::FramePacingMode::Target_30, QStringLiteral("30 FPS")},
            {Settings::FramePacingMode::Target_60, QStringLiteral("60 FPS")},
            {Settings::FramePacingMode::Target_90, QStringLiteral("90 FPS")},
            {Settings::FramePacingMode::Target_120, QStringLiteral("120 FPS")},
        };
        for (const auto& item : pacing_options) {
            auto* act = pacing_menu->addAction(item.second, [this, item] {
                ApplyStatusBarSwitchableSetting(Settings::values.frame_pacing_mode, item.first,
                                                m_session_backup, m_session_backup.frame_pacing_mode,
                                                current_game_path);
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(item.first == cur_pacing);
        }
    } else if (group_index == 3) {
        auto* header_act = context_menu.addAction(StormLang(
            QStringLiteral("🖼️ Графика и масштабирование"),
            QStringLiteral("🖼️ Graphics and Scaling"),
            QStringLiteral("🖼️ Grafik und Skalierung"),
            QStringLiteral("🖼️ Graphismes et mise à l'échelle"),
            QStringLiteral("🖼️ 图形与缩放"),
            QStringLiteral("🖼️ グラフィックスとスケーリング")
        ));
        header_act->setEnabled(false);
        context_menu.addSeparator();

        auto* res_menu = context_menu.addMenu(StormLang(
            QStringLiteral("📐 Разрешение"),
            QStringLiteral("📐 Resolution"),
            QStringLiteral("📐 Auflösung"),
            QStringLiteral("📐 Résolution"),
            QStringLiteral("📐 分辨率"),
            QStringLiteral("📐 解像度")
        ));
        const auto cur_res = Settings::values.resolution_setup.GetValue();
        const bool is_docked = Settings::values.use_docked_mode.GetValue() == Settings::ConsoleMode::Docked;
        const std::vector<std::pair<Settings::ResolutionSetup, QString>> res_options = {
            {Settings::ResolutionSetup::Res1_4X, is_docked ? QStringLiteral("0.25X (270p)") : QStringLiteral("0.25X (180p)")},
            {Settings::ResolutionSetup::Res1_2X, is_docked ? QStringLiteral("0.5X (540p)") : QStringLiteral("0.5X (360p)")},
            {Settings::ResolutionSetup::Res3_4X, is_docked ? QStringLiteral("0.75X (810p)") : QStringLiteral("0.75X (540p)")},
            {Settings::ResolutionSetup::Res1X, is_docked ? QStringLiteral("1X (1080p)") : QStringLiteral("1X (720p)")},
            {Settings::ResolutionSetup::Res5_4X, is_docked ? QStringLiteral("1.25X (1350p)") : QStringLiteral("1.25X (900p)")},
            {Settings::ResolutionSetup::Res3_2X, is_docked ? QStringLiteral("1.5X (1620p)") : QStringLiteral("1.5X (1080p)")},
            {Settings::ResolutionSetup::Res2X, is_docked ? QStringLiteral("2X (2160p / 4K)") : QStringLiteral("2X (1440p / 2K)")},
            {Settings::ResolutionSetup::Res3X, is_docked ? QStringLiteral("3X (3240p / 6K)") : QStringLiteral("3X (2160p / 4K)")},
            {Settings::ResolutionSetup::Res4X, is_docked ? QStringLiteral("4X (4320p / 8K)") : QStringLiteral("4X (2880p)")},
            {Settings::ResolutionSetup::Res5X, is_docked ? QStringLiteral("5X (5400p)") : QStringLiteral("5X (3600p)")},
            {Settings::ResolutionSetup::Res6X, is_docked ? QStringLiteral("6X (6480p)") : QStringLiteral("6X (4320p)")},
            {Settings::ResolutionSetup::Res7X, is_docked ? QStringLiteral("7X (7560p)") : QStringLiteral("7X (5040p)")},
            {Settings::ResolutionSetup::Res8X, is_docked ? QStringLiteral("8X (8640p)") : QStringLiteral("8X (5760p)")},
        };
        for (const auto& opt : res_options) {
            auto* act = res_menu->addAction(opt.second, [this, opt] {
                ApplyStatusBarSwitchableSetting(Settings::values.resolution_setup, opt.first,
                                                m_session_backup, m_session_backup.resolution_setup,
                                                current_game_path);
                UpdateResScaleText();
                ApplyDynamicSettingChange();
                if (QtCommon::system && QtCommon::system->IsPoweredOn()) {
                    statusBar()->showMessage(StormLang(
                        QStringLiteral("Масштаб разрешения изменен. Для предотвращения графических артефактов рекомендуется перезапуск игры."),
                        QStringLiteral("Resolution scale changed. Restarting the game is recommended to avoid visual artifacts."),
                        QStringLiteral("Auflösungsskalierung geändert. Ein Neustart des Spiels wird empfohlen."),
                        QStringLiteral("Échelle de résolution modifiée. Redémarrer le jeu est recommandé."),
                        QStringLiteral("分辨率缩放已更改。建议重新启动游戏以避免视觉伪影。"),
                        QStringLiteral("解像度スケールを変更しました。視覚的なアーティファクトを避けるためゲームの再起動をお勧めします。")
                    ), 6000);
                }
            });
            act->setCheckable(true);
            act->setChecked(opt.first == cur_res);
        }

        auto* aspect_menu = context_menu.addMenu(StormLang(
            QStringLiteral("📐 Соотношение сторон"),
            QStringLiteral("📐 Aspect Ratio"),
            QStringLiteral("📐 Seitenverhältnis"),
            QStringLiteral("📐 Format d'image"),
            QStringLiteral("📐 宽高比"),
            QStringLiteral("📐 アスペクト比")
        ));
        const auto cur_aspect = Settings::values.aspect_ratio.GetValue();
        const std::vector<std::pair<Settings::AspectRatio, QString>> aspect_options = {
            {Settings::AspectRatio::R16_9, StormLang(QStringLiteral("16:9 (Стандарт HD)"), QStringLiteral("16:9 (Standard HD)"), QStringLiteral("16:9 (Standard-HD)"), QStringLiteral("16:9 (Standard HD)"), QStringLiteral("16:9 (标准 HD)"), QStringLiteral("16:9 (標準 HD)"), QStringLiteral("16:9 (قياسي HD)"), QStringLiteral("16:9 (Estándar HD)"))},
            {Settings::AspectRatio::R4_3, StormLang(QStringLiteral("4:3 (Классический)"), QStringLiteral("4:3 (Classic)"), QStringLiteral("4:3 (Klassisch)"), QStringLiteral("4:3 (Classique)"), QStringLiteral("4:3 (经典)"), QStringLiteral("4:3 (クラシック)"), QStringLiteral("4:3 (كلاسيكي)"), QStringLiteral("4:3 (Clásico)"))},
            {Settings::AspectRatio::R21_9, StormLang(QStringLiteral("21:9 (Ультраширокий)"), QStringLiteral("21:9 (Ultrawide)"), QStringLiteral("21:9 (Ultrawide)"), QStringLiteral("21:9 (Ultra-large)"), QStringLiteral("21:9 (超宽屏)"), QStringLiteral("21:9 (ウルトラワイド)"), QStringLiteral("21:9 (عريض جداً)"), QStringLiteral("21:9 (Ultraancho)"))},
            {Settings::AspectRatio::R16_10, StormLang(QStringLiteral("16:10 (Ноутбуки / Steam Deck)"), QStringLiteral("16:10 (Laptops / Steam Deck)"), QStringLiteral("16:10 (Laptops / Steam Deck)"), QStringLiteral("16:10 (PC portables / Steam Deck)"), QStringLiteral("16:10 (笔记本 / Steam Deck)"), QStringLiteral("16:10 (ノートPC / Steam Deck)"), QStringLiteral("16:10 (لابتوب / Steam Deck)"), QStringLiteral("16:10 (Portátiles / Steam Deck)"))},
            {Settings::AspectRatio::Stretch, StormLang(QStringLiteral("Растянуть по окну"), QStringLiteral("Stretch to Window"), QStringLiteral("An Fenster anpassen"), QStringLiteral("Étirer à la fenêtre"), QStringLiteral("拉伸至窗口"), QStringLiteral("ウィンドウに合わせる"), QStringLiteral("تمديد للنافذة"), QStringLiteral("Estirar a la ventana"))},
            {Settings::AspectRatio::R32_9, StormLang(QStringLiteral("32:9 (Супер-ультраширокий)"), QStringLiteral("32:9 (Super Ultrawide)"), QStringLiteral("32:9 (Super-Ultrawide)"), QStringLiteral("32:9 (Super ultra-large)"), QStringLiteral("32:9 (超级超宽屏)"), QStringLiteral("32:9 (スーパーウルトラワイド)"), QStringLiteral("32:9 (فائق العرض)"), QStringLiteral("32:9 (Súper ultraancho)"))},
            {Settings::AspectRatio::R1_1, StormLang(QStringLiteral("1:1 (Квадратный)"), QStringLiteral("1:1 (Square)"), QStringLiteral("1:1 (Quadratisch)"), QStringLiteral("1:1 (Carré)"), QStringLiteral("1:1 (正方形)"), QStringLiteral("1:1 (正方形)"), QStringLiteral("1:1 (مربع)"), QStringLiteral("1:1 (Cuadrado)"))},
            {Settings::AspectRatio::R3_2, StormLang(QStringLiteral("3:2 (Surface / Планшеты)"), QStringLiteral("3:2 (Surface / Tablets)"), QStringLiteral("3:2 (Surface / Tablets)"), QStringLiteral("3:2 (Surface / Tablettes)"), QStringLiteral("3:2 (Surface / 平板)"), QStringLiteral("3:2 (Surface / タブレット)"), QStringLiteral("3:2 (Surface / الأجهزة اللوحية)"), QStringLiteral("3:2 (Surface / Tabletas)"))},
            {Settings::AspectRatio::R5_4, StormLang(QStringLiteral("5:4 (Мониторы SXGA)"), QStringLiteral("5:4 (SXGA Monitors)"), QStringLiteral("5:4 (SXGA-Monitore)"), QStringLiteral("5:4 (Moniteurs SXGA)"), QStringLiteral("5:4 (SXGA 显示器)"), QStringLiteral("5:4 (SXGA モニター)"), QStringLiteral("5:4 (شاشات SXGA)"), QStringLiteral("5:4 (Monitores SXGA)"))},
            {Settings::AspectRatio::R18_9, StormLang(QStringLiteral("18:9 / 2:1 (Смартфоны Univisium)"), QStringLiteral("18:9 / 2:1 (Univisium Mobile)"), QStringLiteral("18:9 / 2:1 (Univisium Mobil)"), QStringLiteral("18:9 / 2:1 (Mobile Univisium)"), QStringLiteral("18:9 / 2:1 (Univisium 手机)"), QStringLiteral("18:9 / 2:1 (Univisium モバイル)"), QStringLiteral("18:9 / 2:1 (هواتف Univisium)"), QStringLiteral("18:9 / 2:1 (Móviles Univisium)"))},
            {Settings::AspectRatio::R19_5_9, StormLang(QStringLiteral("19.5:9 (Современные смартфоны)"), QStringLiteral("19.5:9 (Modern Smartphones)"), QStringLiteral("19.5:9 (Moderne Smartphones)"), QStringLiteral("19.5:9 (Smartphones modernes)"), QStringLiteral("19.5:9 (现代智能手机)"), QStringLiteral("19.5:9 (最新スマートフォン)"), QStringLiteral("19.5:9 (الهواتف الذكية الحديثة)"), QStringLiteral("19.5:9 (Smartphones modernos)"))},
            {Settings::AspectRatio::R20_9, StormLang(QStringLiteral("20:9 (Игровые смартфоны)"), QStringLiteral("20:9 (Gaming Smartphones)"), QStringLiteral("20:9 (Gaming-Smartphones)"), QStringLiteral("20:9 (Smartphones gaming)"), QStringLiteral("20:9 (游戏手机)"), QStringLiteral("20:9 (ゲーミングスマホ)"), QStringLiteral("20:9 (هواتف الألعاب)"), QStringLiteral("20:9 (Smartphones gaming)"))},
            {Settings::AspectRatio::R21_10, StormLang(QStringLiteral("21:10 (Широкоформатный)"), QStringLiteral("21:10 (Widescreen)"), QStringLiteral("21:10 (Breitbild)"), QStringLiteral("21:10 (Écran large)"), QStringLiteral("21:10 (宽屏)"), QStringLiteral("21:10 (ワイドスクリーン)"), QStringLiteral("21:10 (شاشة عريضة)"), QStringLiteral("21:10 (Pantalla ancha)"))},
            {Settings::AspectRatio::R24_10, StormLang(QStringLiteral("24:10 / 2.4:1 (Киноформат)"), QStringLiteral("24:10 / 2.4:1 (Cinema)"), QStringLiteral("24:10 / 2.4:1 (Kinoformat)"), QStringLiteral("24:10 / 2.4:1 (Cinéma)"), QStringLiteral("24:10 / 2.4:1 (电影宽银幕)"), QStringLiteral("24:10 / 2.4:1 (シネマ)"), QStringLiteral("24:10 / 2.4:1 (سينمائي)"), QStringLiteral("24:10 / 2.4:1 (Cine)"))},
            {Settings::AspectRatio::R32_10, StormLang(QStringLiteral("32:10 (Двойной 16:10)"), QStringLiteral("32:10 (Dual 16:10)"), QStringLiteral("32:10 (Dual 16:10)"), QStringLiteral("32:10 (Double 16:10)"), QStringLiteral("32:10 (双 16:10)"), QStringLiteral("32:10 (デュアル 16:10)"), QStringLiteral("32:10 (مزدوج 16:10)"), QStringLiteral("32:10 (Doble 16:10)"))},
            {Settings::AspectRatio::R48_9, StormLang(QStringLiteral("48:9 (Три монитора 16:9)"), QStringLiteral("48:9 (Triple 16:9 Surround)"), QStringLiteral("48:9 (Dreifach-16:9 Surround)"), QStringLiteral("48:9 (Triple écran 16:9)"), QStringLiteral("48:9 (三联屏 16:9)"), QStringLiteral("48:9 (トリプル 16:9)"), QStringLiteral("48:9 (ثلاث شاشات 16:9)"), QStringLiteral("48:9 (Triple monitor 16:9)"))},
            {Settings::AspectRatio::Auto_Window, StormLang(QStringLiteral("Авто по пропорциям окна (3D без растяжения)"), QStringLiteral("Auto Window Aspect (3D Native)"), QStringLiteral("Automatisch nach Fenstergröße (3D Nativ)"), QStringLiteral("Auto selon la fenêtre (3D natif)"), QStringLiteral("按窗口比例自动适配 (3D 原生无拉伸)"), QStringLiteral("ウィンドウ比率に自動適応 (3D ネイティブ)"), QStringLiteral("تلقائي حسب أبعاد النافذة (3D أصلي)"), QStringLiteral("Auto según ventana (3D nativo)"))},
        };
        for (const auto& opt : aspect_options) {
            auto* act = aspect_menu->addAction(opt.second, [this, opt] {
                ApplyStatusBarSwitchableSetting(Settings::values.aspect_ratio, opt.first,
                                                m_session_backup, m_session_backup.aspect_ratio,
                                                current_game_path);
                UpdateAspectText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(opt.first == cur_aspect);
        }

        auto* vram_menu = context_menu.addMenu(StormLang(
            QStringLiteral("💾 Видеопамять"),
            QStringLiteral("💾 Video Memory"),
            QStringLiteral("💾 Videospeicher"),
            QStringLiteral("💾 Mémoire vidéo"),
            QStringLiteral("💾 显存"),
            QStringLiteral("💾 ビデオメモリ")
        ));
        const auto cur_vram = Settings::values.vram_usage_mode.GetValue();
        const std::vector<std::pair<Settings::VramUsageMode, QString>> vram_options = {
            {Settings::VramUsageMode::Conservative, StormLang(QStringLiteral("Консервативный"), QStringLiteral("Conservative"), QStringLiteral("Konservativ"), QStringLiteral("Conservateur"), QStringLiteral("保守"), QStringLiteral("控えめ"))},
            {Settings::VramUsageMode::Normal, StormLang(QStringLiteral("Нормальный"), QStringLiteral("Normal"), QStringLiteral("Normal"), QStringLiteral("Normal"), QStringLiteral("正常"), QStringLiteral("通常"))},
            {Settings::VramUsageMode::Aggressive, StormLang(QStringLiteral("Агрессивный"), QStringLiteral("Aggressive"), QStringLiteral("Aggressiv"), QStringLiteral("Agressif"), QStringLiteral("激进"), QStringLiteral("積極的"))},
        };
        for (const auto& opt : vram_options) {
            auto* act = vram_menu->addAction(opt.second, [this, opt] {
                ApplyStatusBarSwitchableSetting(Settings::values.vram_usage_mode, opt.first,
                                                m_session_backup, m_session_backup.vram_usage_mode,
                                                current_game_path);
                UpdateVramText();
                ApplyDynamicSettingChange();
                if (QtCommon::system && QtCommon::system->IsPoweredOn()) {
                    statusBar()->showMessage(StormLang(
                        QStringLiteral("Режим использования VRAM изменен. Полный бюджет видеопамяти применится при следующем запуске игры."),
                        QStringLiteral("VRAM usage mode changed. Full GPU memory budget will apply upon next game launch."),
                        QStringLiteral("VRAM-Nutzungsmodus geändert. Das vollständige GPU-Speicherbudget gilt beim nächsten Spielstart."),
                        QStringLiteral("Mode d'utilisation de la VRAM modifié. Le budget mémoire s'appliquera au prochain lancement."),
                        QStringLiteral("显存使用模式已更改。完整的显存预算将在下次启动游戏时生效。"),
                        QStringLiteral("VRAM使用モードを変更しました。完全なGPUメモリ割り当ては次回のゲーム起動時に適用されます。")
                    ), 6000);
                }
            });
            act->setCheckable(true);
            act->setChecked(opt.first == cur_vram);
        }

        auto* aniso_menu = context_menu.addMenu(StormLang(
            QStringLiteral("🔍 Анизотропная фильтрация"),
            QStringLiteral("🔍 Anisotropic Filtering"),
            QStringLiteral("🔍 Anisotrope Filterung"),
            QStringLiteral("🔍 Filtrage anisotrope"),
            QStringLiteral("🔍 各向异性过滤"),
            QStringLiteral("🔍 異方性フィルタリング")
        ));
        const auto cur_aniso = Settings::values.max_anisotropy.GetValue();
        const std::vector<std::pair<Settings::AnisotropyMode, QString>> aniso_options = {
            {Settings::AnisotropyMode::Automatic, StormLang(QStringLiteral("Автоматически"), QStringLiteral("Automatic"), QStringLiteral("Automatisch"), QStringLiteral("Automatique"), QStringLiteral("自动"), QStringLiteral("自動"))},
            {Settings::AnisotropyMode::Default, StormLang(QStringLiteral("По умолчанию"), QStringLiteral("Default"), QStringLiteral("Standard"), QStringLiteral("Par défaut"), QStringLiteral("默认"), QStringLiteral("デフォルト"))},
            {Settings::AnisotropyMode::X2, QStringLiteral("2x")},
            {Settings::AnisotropyMode::X4, QStringLiteral("4x")},
            {Settings::AnisotropyMode::X8, QStringLiteral("8x")},
            {Settings::AnisotropyMode::X16, QStringLiteral("16x")},
            {Settings::AnisotropyMode::X32, QStringLiteral("32x")},
            {Settings::AnisotropyMode::X64, QStringLiteral("64x")},
            {Settings::AnisotropyMode::None, StormLang(QStringLiteral("Отключено"), QStringLiteral("Disabled"), QStringLiteral("Deaktiviert"), QStringLiteral("Désactivé"), QStringLiteral("已禁用"), QStringLiteral("無効"))},
        };
        for (const auto& opt : aniso_options) {
            auto* act = aniso_menu->addAction(opt.second, [this, opt] {
                ApplyStatusBarSwitchableSetting(Settings::values.max_anisotropy, opt.first,
                                                m_session_backup, m_session_backup.max_anisotropy,
                                                current_game_path);
                UpdateAnisotropyText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(opt.first == cur_aniso);
        }

        auto* aa_menu = context_menu.addMenu(StormLang(
            QStringLiteral("✨ Сглаживание"),
            QStringLiteral("✨ Anti-Aliasing"),
            QStringLiteral("✨ Kantenglättung"),
            QStringLiteral("✨ Anticrénelage"),
            QStringLiteral("✨ 抗锯齿"),
            QStringLiteral("✨ アンチエイリアス")
        ));
        const auto cur_aa = Settings::values.anti_aliasing.GetValue();
        const std::vector<std::pair<Settings::AntiAliasing, QString>> aa_options = {
            {Settings::AntiAliasing::None, StormLang(QStringLiteral("Отключено"), QStringLiteral("Disabled"), QStringLiteral("Deaktiviert"), QStringLiteral("Désactivé"), QStringLiteral("已禁用"), QStringLiteral("無効"))},
            {Settings::AntiAliasing::Fxaa, QStringLiteral("FXAA")},
            {Settings::AntiAliasing::Smaa, QStringLiteral("SMAA")},
        };
        for (const auto& pair : aa_options) {
            auto* act = aa_menu->addAction(pair.second, [this, pair] {
                ApplyStatusBarSwitchableSetting(Settings::values.anti_aliasing, pair.first,
                                                m_session_backup, m_session_backup.anti_aliasing,
                                                current_game_path);
                UpdateAAText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(pair.first == cur_aa);
        }

        auto* filter_menu = context_menu.addMenu(StormLang(
            QStringLiteral("🔬 Фильтрация масштабирования"),
            QStringLiteral("🔬 Scaling Filter"),
            QStringLiteral("🔬 Skalierungsfilter"),
            QStringLiteral("🔬 Filtre de mise à l'échelle"),
            QStringLiteral("🔬 缩放过滤器"),
            QStringLiteral("🔬 スケーリングフィルター")
        ));
        const auto cur_filter = Settings::values.scaling_filter.GetValue();
        const std::vector<std::pair<Settings::ScalingFilter, QString>> filter_options = {
            {Settings::ScalingFilter::NearestNeighbor, StormLang(QStringLiteral("Ближайший сосед"), QStringLiteral("Nearest Neighbor"), QStringLiteral("Nächster Nachbar"), QStringLiteral("Plus proche voisin"), QStringLiteral("最近邻"), QStringLiteral("最近傍"))},
            {Settings::ScalingFilter::Bilinear, StormLang(QStringLiteral("Билинейный"), QStringLiteral("Bilinear"), QStringLiteral("Bilinear"), QStringLiteral("Bilinéaire"), QStringLiteral("双线性"), QStringLiteral("バイリニア"))},
            {Settings::ScalingFilter::Bicubic, StormLang(QStringLiteral("Бикубический"), QStringLiteral("Bicubic"), QStringLiteral("Bikubisch"), QStringLiteral("Bicubique"), QStringLiteral("双三次"), QStringLiteral("バイキュービック"))},
            {Settings::ScalingFilter::Gaussian, StormLang(QStringLiteral("Гаусс"), QStringLiteral("Gaussian"), QStringLiteral("Gauß"), QStringLiteral("Gauss"), QStringLiteral("高斯"), QStringLiteral("ガウス"))},
            {Settings::ScalingFilter::Lanczos, StormLang(QStringLiteral("Ланцош"), QStringLiteral("Lanczos"), QStringLiteral("Lanczos"), QStringLiteral("Lanczos"), QStringLiteral("兰索斯"), QStringLiteral("ランツォシュ"))},
            {Settings::ScalingFilter::ScaleForce, QStringLiteral("ScaleForce")},
            {Settings::ScalingFilter::Fsr, QStringLiteral("FSR")},
            {Settings::ScalingFilter::Area, QStringLiteral("Area")},
            {Settings::ScalingFilter::Mmpx, QStringLiteral("MMPX")},
            {Settings::ScalingFilter::ZeroTangent, QStringLiteral("Zero-Tangent")},
            {Settings::ScalingFilter::BSpline, QStringLiteral("B-Spline")},
            {Settings::ScalingFilter::Mitchell, QStringLiteral("Mitchell")},
            {Settings::ScalingFilter::Spline1, QStringLiteral("Spline-1")},
            {Settings::ScalingFilter::Sgsr, QStringLiteral("SGSR")},
            {Settings::ScalingFilter::SgsrEdge, QStringLiteral("SGSR EdgeDir")},
        };
        for (const auto& pair : filter_options) {
            auto* act = filter_menu->addAction(pair.second, [this, pair] {
                ApplyStatusBarSwitchableSetting(Settings::values.scaling_filter, pair.first,
                                                m_session_backup, m_session_backup.scaling_filter,
                                                current_game_path);
                UpdateFilterText();
                ApplyDynamicSettingChange();
            });
            act->setCheckable(true);
            act->setChecked(pair.first == cur_filter);
        }

        context_menu.addAction(StormLang(
            QStringLiteral("🎨 Шейдеры постобработки (RetroArch)..."),
            QStringLiteral("🎨 Post-Processing Shaders (RetroArch)..."),
            QStringLiteral("🎨 Post-Processing-Shader (RetroArch)..."),
            QStringLiteral("🎨 Shaders de post-traitement (RetroArch)..."),
            QStringLiteral("🎨 后处理着色器 (RetroArch)..."),
            QStringLiteral("🎨 ポストプロセスシェーダー (RetroArch)...")
        ), [this] {
            if (shader_status_button) {
                shader_status_button->click();
            }
        });

        auto* disk_act = context_menu.addAction(StormLang(
            QStringLiteral("💽 Кэш шейдеров на диске"),
            QStringLiteral("💽 Disk Shader Cache"),
            QStringLiteral("💽 Festplatten-Shader-Cache"),
            QStringLiteral("💽 Cache de shaders sur disque"),
            QStringLiteral("💽 磁盘着色器缓存"),
            QStringLiteral("💽 ディスクシェーダーキャッシュ")
        ), [this] {
            ApplyStatusBarSwitchableSettingNoBackup(
                Settings::values.use_disk_shader_cache,
                !Settings::values.use_disk_shader_cache.GetValue(), current_game_path);
            UpdateDiskCacheText();
            ApplyDynamicSettingChange();
        });
        disk_act->setCheckable(true);
        disk_act->setChecked(Settings::values.use_disk_shader_cache.GetValue());
    } else if (group_index == 5) {
        auto* header_act = context_menu.addAction(StormLang(
            QStringLiteral("🕹️ Режимы работы консоли"),
            QStringLiteral("🕹️ Console Operation Modes"),
            QStringLiteral("🕹️ Konsolen-Betriebsmodi"),
            QStringLiteral("🕹️ Modes de fonctionnement de la console"),
            QStringLiteral("🕹️ 主机运行模式"),
            QStringLiteral("🕹️ 本体の動作モード")
        ));
        header_act->setEnabled(false);
        context_menu.addSeparator();

        auto* dock_menu = context_menu.addMenu(StormLang(
            QStringLiteral("📺 Режим док-станции"),
            QStringLiteral("📺 Docked Mode"),
            QStringLiteral("📺 Dock-Modus"),
            QStringLiteral("📺 Mode station d'accueil"),
            QStringLiteral("📺 底座模式"),
            QStringLiteral("📺 ドックモード")
        ));
        const auto cur_dock = Settings::values.use_docked_mode.GetValue();
        const std::vector<std::pair<Settings::ConsoleMode, QString>> dock_options = {
            {Settings::ConsoleMode::Docked, StormLang(QStringLiteral("В док-станции"), QStringLiteral("Docked"), QStringLiteral("Im Dock"), QStringLiteral("En station d'accueil"), QStringLiteral("底座模式"), QStringLiteral("ドックモード"))},
            {Settings::ConsoleMode::Handheld, StormLang(QStringLiteral("Портативный"), QStringLiteral("Handheld"), QStringLiteral("Handheld"), QStringLiteral("Portable"), QStringLiteral("掌机模式"), QStringLiteral("携帯モード"))},
        };
        for (const auto& pair : dock_options) {
            auto* act = dock_menu->addAction(pair.second, [this, pair] {
                if (pair.first != Settings::values.use_docked_mode.GetValue()) {
                    OnToggleDockedMode();
                }
            });
            act->setCheckable(true);
            act->setChecked(pair.first == cur_dock);
        }

        auto* speed_menu = context_menu.addMenu(StormLang(
            QStringLiteral("⚡ Ограничение скорости"),
            QStringLiteral("⚡ Speed Limit"),
            QStringLiteral("⚡ Geschwindigkeitsbegrenzung"),
            QStringLiteral("⚡ Limite de vitesse"),
            QStringLiteral("⚡ 速度限制"),
            QStringLiteral("⚡ 速度制限")
        ));
        speed_menu->addAction(QStringLiteral("100%"), [this] {
            ApplyStatusBarSwitchableSettingNoBackup(Settings::values.use_speed_limit, true,
                                                    current_game_path);
            ApplyStatusBarSwitchableSettingNoBackup(Settings::values.speed_limit, static_cast<u16>(100),
                                                    current_game_path);
            UpdateSpeedLimitText();
            ApplyDynamicSettingChange();
        });
        speed_menu->addAction(QStringLiteral("150%"), [this] {
            ApplyStatusBarSwitchableSettingNoBackup(Settings::values.use_speed_limit, true,
                                                    current_game_path);
            ApplyStatusBarSwitchableSettingNoBackup(Settings::values.speed_limit, static_cast<u16>(150),
                                                    current_game_path);
            UpdateSpeedLimitText();
            ApplyDynamicSettingChange();
        });
        speed_menu->addAction(QStringLiteral("200%"), [this] {
            ApplyStatusBarSwitchableSettingNoBackup(Settings::values.use_speed_limit, true,
                                                    current_game_path);
            ApplyStatusBarSwitchableSettingNoBackup(Settings::values.speed_limit, static_cast<u16>(200),
                                                    current_game_path);
            UpdateSpeedLimitText();
            ApplyDynamicSettingChange();
        });
        speed_menu->addAction(QStringLiteral("300%"), [this] {
            ApplyStatusBarSwitchableSettingNoBackup(Settings::values.use_speed_limit, true,
                                                    current_game_path);
            ApplyStatusBarSwitchableSettingNoBackup(Settings::values.speed_limit, static_cast<u16>(300),
                                                    current_game_path);
            UpdateSpeedLimitText();
            ApplyDynamicSettingChange();
        });
        speed_menu->addAction(StormLang(
            QStringLiteral("Без лимита скорости"),
            QStringLiteral("No Speed Limit"),
            QStringLiteral("Keine Geschwindigkeitsbegrenzung"),
            QStringLiteral("Pas de limite de vitesse"),
            QStringLiteral("无速度限制"),
            QStringLiteral("速度制限なし")
        ), [this] {
            ApplyStatusBarSwitchableSettingNoBackup(Settings::values.use_speed_limit, false,
                                                    current_game_path);
            UpdateSpeedLimitText();
            ApplyDynamicSettingChange();
        });

        auto* airplane_act = context_menu.addAction(StormLang(
            QStringLiteral("✈️ Режим полёта"),
            QStringLiteral("✈️ Airplane Mode"),
            QStringLiteral("✈️ Flugmodus"),
            QStringLiteral("✈️ Mode avion"),
            QStringLiteral("✈️ 飞行模式"),
            QStringLiteral("✈️ 機内モード")
        ), [this] {
            ApplyStatusBarSwitchableSetting(Settings::values.airplane_mode,
                                            !Settings::values.airplane_mode.GetValue(),
                                            m_session_backup, m_session_backup.airplane_mode,
                                            current_game_path);
            UpdateAirplaneModeButton();
            ApplyDynamicSettingChange();
        });
        airplane_act->setCheckable(true);
        airplane_act->setChecked(Settings::values.airplane_mode.GetValue());

        auto* mute_act = context_menu.addAction(StormLang(
            QStringLiteral("🔇 Отключить звук"),
            QStringLiteral("🔇 Mute Audio"),
            QStringLiteral("🔇 Ton stummschalten"),
            QStringLiteral("🔇 Couper le son"),
            QStringLiteral("🔇 静音"),
            QStringLiteral("🔇 消音")
        ), [this] {
            OnMute();
        });
        mute_act->setCheckable(true);
        mute_act->setChecked(Settings::values.audio_muted.GetValue());
    } else if (group_index == 0) {
        context_menu.addAction(StormLang(
            QStringLiteral("🔄 Обновить список игр"),
            QStringLiteral("🔄 Refresh Game List"),
            QStringLiteral("🔄 Spielliste aktualisieren"),
            QStringLiteral("🔄 Actualiser la liste des jeux"),
            QStringLiteral("🔄 刷新游戏列表"),
            QStringLiteral("🔄 ゲームリストを更新")
        ), this, &MainWindow::OnGameListRefresh);
        context_menu.addAction(StormLang(
            QStringLiteral("🖥️ Полноэкранный режим"),
            QStringLiteral("🖥️ Fullscreen"),
            QStringLiteral("🖥️ Vollbild"),
            QStringLiteral("🖥️ Plein écran"),
            QStringLiteral("🖥️ 全屏模式"),
            QStringLiteral("🖥️ フルスクリーン")
        ), this, [this] {
            ui->action_Fullscreen->setChecked(!ui->action_Fullscreen->isChecked());
            ToggleFullscreen();
            UpdateFullscreenButton();
        });
    } else if (group_index == 6) {
        auto* header_act = context_menu.addAction(StormLang(
            QStringLiteral("🛠️ Системные компоненты"),
            QStringLiteral("🛠️ System Components"),
            QStringLiteral("🛠️ Systemkomponenten"),
            QStringLiteral("🛠️ Composants système"),
            QStringLiteral("🛠️ 系统组件"),
            QStringLiteral("🛠️ システムコンポーネント")
        ));
        header_act->setEnabled(false);
        context_menu.addSeparator();
        context_menu.addAction(StormLang(
            QStringLiteral("🌐 Онлайн-установка прошивки из сети..."),
            QStringLiteral("🌐 Online Install Firmware from Network..."),
            QStringLiteral("🌐 Online-Firmware-Installation aus dem Netzwerk..."),
            QStringLiteral("🌐 Installation en ligne du firmware depuis le réseau..."),
            QStringLiteral("🌐 从网络在线安装固件..."),
            QStringLiteral("🌐 ネットワークからファームウェアをオンラインインストール...")
        ), this, &MainWindow::OnInstallFirmwareOnline);
        context_menu.addAction(StormLang(
            QStringLiteral("📦 Установить прошивку из ZIP..."),
            QStringLiteral("📦 Install Firmware from ZIP..."),
            QStringLiteral("📦 Firmware aus ZIP installieren..."),
            QStringLiteral("📦 Installer le firmware depuis un ZIP..."),
            QStringLiteral("📦 从 ZIP 安装固件..."),
            QStringLiteral("📦 ZIP からファームウェアをインストール...")
        ), this, &MainWindow::OnInstallFirmwareFromZIP);
        context_menu.addAction(StormLang(
            QStringLiteral("📁 Установить прошивку из папки..."),
            QStringLiteral("📁 Install Firmware from Folder..."),
            QStringLiteral("📁 Firmware aus Ordner installieren..."),
            QStringLiteral("📁 Installer le firmware depuis un dossier..."),
            QStringLiteral("📁 从文件夹安装固件..."),
            QStringLiteral("📁 フォルダからファームウェアをインストール...")
        ), this, &MainWindow::OnInstallFirmware);
        context_menu.addSeparator();
        context_menu.addAction(StormLang(
            QStringLiteral("🔑 Онлайн-установка ключей из сети..."),
            QStringLiteral("🔑 Online Install Keys from Network..."),
            QStringLiteral("🔑 Online-Schlüssel-Installation aus dem Netzwerk..."),
            QStringLiteral("🔑 Installation en ligne des clés depuis le réseau..."),
            QStringLiteral("🔑 从网络在线安装密钥..."),
            QStringLiteral("🔑 ネットワークからキーをオンラインインストール...")
        ), this, &MainWindow::OnInstallKeysOnline);
        context_menu.addAction(StormLang(
            QStringLiteral("📦 Установить ключи из ZIP..."),
            QStringLiteral("📦 Install Keys from ZIP..."),
            QStringLiteral("📦 Schlüssel aus ZIP installieren..."),
            QStringLiteral("📦 Installer les clés depuis un ZIP..."),
            QStringLiteral("📦 从 ZIP 安装密钥..."),
            QStringLiteral("📦 ZIP からキーをインストール...")
        ), this, &MainWindow::OnInstallKeysFromZIP);
        context_menu.addAction(StormLang(
            QStringLiteral("🔑 Установить ключи из файла (prod.keys)..."),
            QStringLiteral("🔑 Install Decryption Keys (prod.keys)..."),
            QStringLiteral("🔑 Entschlüsselungsschlüssel installieren (prod.keys)..."),
            QStringLiteral("🔑 Installer les clés de déchiffrement (prod.keys)..."),
            QStringLiteral("🔑 从文件安装解密密钥 (prod.keys)..."),
            QStringLiteral("🔑 ファイルから復号キーをインストール (prod.keys)...")
        ), this, &MainWindow::OnInstallDecryptionKeys);
        context_menu.addSeparator();
        context_menu.addAction(StormLang(
            QStringLiteral("📁 Открыть папку NAND..."),
            QStringLiteral("📁 Open NAND Folder..."),
            QStringLiteral("📁 NAND-Ordner öffnen..."),
            QStringLiteral("📁 Ouvrir le dossier NAND..."),
            QStringLiteral("📁 打开 NAND 文件夹..."),
            QStringLiteral("📁 NAND フォルダを開く...")
        ), this, &MainWindow::OnOpenNANDFolder);
    } else if (group_index == 7) {
        auto* header_act = context_menu.addAction(StormLang(
            QStringLiteral("🌐 Сетевые функции"),
            QStringLiteral("🌐 Network Features"),
            QStringLiteral("🌐 Netzwerkfunktionen"),
            QStringLiteral("🌐 Fonctions réseau"),
            QStringLiteral("🌐 网络功能"),
            QStringLiteral("🌐 ネットワーク機能")
        ));
        header_act->setEnabled(false);
        context_menu.addSeparator();
        context_menu.addAction(StormLang(
            QStringLiteral("🌐 Мультиплеер (обзор комнат)..."),
            QStringLiteral("🌐 Multiplayer (Browse Rooms)..."),
            QStringLiteral("🌐 Mehrspieler (Räume durchsuchen)..."),
            QStringLiteral("🌐 Multijoueur (parcourir les salons)..."),
            QStringLiteral("🌐 多人游戏 (浏览房间)..."),
            QStringLiteral("🌐 マルチプレイヤー (ルーム閲覧)...")
        ), multiplayer_state, &MultiplayerState::OnOpenNetworkRoom);
        context_menu.addAction(StormLang(
            QStringLiteral("🔗 Прямое подключение к комнате..."),
            QStringLiteral("🔗 Direct Connect to Room..."),
            QStringLiteral("🔗 Direktverbindung zum Raum..."),
            QStringLiteral("🔗 Connexion directe au salon..."),
            QStringLiteral("🔗 直接连接到房间..."),
            QStringLiteral("🔗 ルームに直接接続...")
        ), multiplayer_state, &MultiplayerState::OnDirectConnectToRoom);
        context_menu.addAction(StormLang(
            QStringLiteral("⚙️ Настройки сети..."),
            QStringLiteral("⚙️ Network Settings..."),
            QStringLiteral("⚙️ Netzwerkeinstellungen..."),
            QStringLiteral("⚙️ Paramètres réseau..."),
            QStringLiteral("⚙️ 网络设置..."),
            QStringLiteral("⚙️ ネットワーク設定...")
        ), this, [this] {
            OnConfigure();
        });
    }
    context_menu.addSeparator();
    context_menu.addAction(StormLang(
        QStringLiteral("⚙️ Настроить подвал..."),
        QStringLiteral("⚙️ Customize Footer..."),
        QStringLiteral("⚙️ Fußzeile anpassen..."),
        QStringLiteral("⚙️ Personnaliser le pied de page..."),
        QStringLiteral("⚙️ 自定义底部栏..."),
        QStringLiteral("⚙️ フッターのカスタマイズ...")
    ), this, &MainWindow::ShowFooterCustomizeMenu);

    ShowMenuAtWidget(context_menu, group_widget);
}

void MainWindow::UpdateStatusButtons() {
    if (renderer_status_button) {
        renderer_status_button->setChecked(Settings::values.renderer_backend.GetValue() ==
                                           Settings::RendererBackend::Vulkan);
    }
    UpdateAPIText();
    UpdateGPUAccuracyButton();
    UpdateDockedButton();
    UpdateFilterText();
    UpdateAAText();
    UpdateAspectText();
    UpdateDmaText();
    UpdateGpuFenceText();
    UpdateVramText();
    UpdateAnisotropyText();
    UpdateAstcDecodeText();
    UpdateAstcRecompressText();
    UpdateResScaleText();
    UpdateAirplaneModeButton();
    UpdateVSyncText();
    UpdateSpeedLimitText();
    UpdateNvdecText();
    UpdateCpuAccuracyText();
    UpdateDiskCacheText();
    UpdateFullscreenButton();
    UpdateRefreshButton();
    UpdateMuteButton();
    UpdateVolumeUI();
    UpdateGameFixStatusButton();
}

// TODO(crueter): Use this for game list stuff
void MainWindow::UpdateUISettings() {
    if (!ui->action_Fullscreen->isChecked()) {
        UISettings::values.geometry = saveGeometry();
        UISettings::values.renderwindow_geometry = render_window->saveGeometry();
    }
    UISettings::values.state = saveState();
    UISettings::values.single_window_mode = ui->action_Single_Window_Mode->isChecked();
    UISettings::values.fullscreen = ui->action_Fullscreen->isChecked();
    UISettings::values.show_filter_bar = ui->action_Show_Filter_Bar->isChecked();
    UISettings::values.show_status_bar = ui->action_Show_Status_Bar->isChecked();
    UISettings::values.show_perf_overlay = ui->action_Show_Performance_Overlay->isChecked();

    UISettings::values.first_start = false;

    Settings::values.enable_overlay = ui->action_Enable_Overlay_Applet->isChecked();
}

void MainWindow::UpdateInputDrivers() {
    const bool high_rate = emulation_running || (controller_dialog && controller_dialog->isVisible());
    const int desired_interval = high_rate ? 4 : 32;
    if (update_input_timer.interval() != desired_interval) {
        update_input_timer.setInterval(desired_interval);
    }
    if (!input_subsystem)
        return;
    input_subsystem->PumpEvents();
}

void MainWindow::InitializeSystemTray() {
    if (m_tray_icon || !QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }

    m_tray_icon = new QSystemTrayIcon(windowIcon(), this);
    m_tray_icon->setToolTip(QStringLiteral("STORM SWITCH"));

    m_tray_menu = new QMenu(this);
    m_tray_menu->setObjectName(QStringLiteral("StormSystemTrayMenu"));
    // STORM Ecosystem Tray Menu Standard:
    // Pure text (NO icons, NO emojis), Segoe UI 9pt, bold first item, #141026 background, #8B5CF6 border, #2E2652 hover, #FFFFFF text
    QFont menu_font(QStringLiteral("Segoe UI"), 9);
    m_tray_menu->setFont(menu_font);
    m_tray_menu->setStyleSheet(QStringLiteral(
        "QMenu#StormSystemTrayMenu {"
        "  background-color: #141026;"
        "  color: #FFFFFF;"
        "  border: 1px solid #8B5CF6;"
        "  padding: 4px 0px;"
        "  font-family: 'Segoe UI';"
        "  font-size: 9pt;"
        "}"
        "QMenu#StormSystemTrayMenu::item {"
        "  background-color: transparent;"
        "  color: #FFFFFF;"
        "  padding: 6px 20px 6px 14px;"
        "  margin: 0px;"
        "}"
        "QMenu#StormSystemTrayMenu::item:selected {"
        "  background-color: #2E2652;"
        "  color: #FFFFFF;"
        "}"
        "QMenu#StormSystemTrayMenu::separator {"
        "  height: 1px;"
        "  background: #2E2652;"
        "  margin: 4px 8px;"
        "}"
        "QMenu#StormSystemTrayMenu::icon {"
        "  width: 0px;"
        "  padding: 0px;"
        "  margin: 0px;"
        "}"
    ));

    m_tray_restore_action = m_tray_menu->addAction(QStringLiteral("Развернуть STORM SWITCH"));
    QFont bold_font = menu_font;
    bold_font.setBold(true);
    m_tray_restore_action->setFont(bold_font);
    m_tray_restore_action->setIconVisibleInMenu(false);

    m_tray_about_action = m_tray_menu->addAction(QStringLiteral("О программе"));
    m_tray_about_action->setFont(menu_font);
    m_tray_about_action->setIconVisibleInMenu(false);

    m_tray_menu->addSeparator();

    m_tray_exit_action = m_tray_menu->addAction(QStringLiteral("Выход"));
    m_tray_exit_action->setFont(menu_font);
    m_tray_exit_action->setIconVisibleInMenu(false);

    connect(m_tray_restore_action, &QAction::triggered, this, [this]() {
        showNormal();
        raise();
        activateWindow();
    });

    connect(m_tray_about_action, &QAction::triggered, this, [this]() {
        showNormal();
        raise();
        activateWindow();
        OnAbout();
    });

    connect(m_tray_exit_action, &QAction::triggered, this, [this]() {
        m_is_quitting_from_tray = true;
        if (m_tray_icon) {
            m_tray_icon->hide();
        }
        close();
    });

    connect(m_tray_icon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            if (isHidden() || isMinimized()) {
                showNormal();
                raise();
                activateWindow();
            } else {
                raise();
                activateWindow();
            }
        }
    });

    m_tray_icon->setContextMenu(m_tray_menu);
    UpdateSystemTrayStrings();
    m_tray_icon->show();

#ifdef _WIN32
    if (!m_restore_event) {
        m_restore_event = CreateEventW(nullptr, FALSE, FALSE, L"Global\\STORM_SWITCH_RestoreEvent");
        if (m_restore_event) {
            m_restore_timer = new QTimer(this);
            connect(m_restore_timer, &QTimer::timeout, this, [this]() {
                if (m_restore_event && WaitForSingleObject(static_cast<HANDLE>(m_restore_event), 0) == WAIT_OBJECT_0) {
                    if (isHidden() || isMinimized()) {
                        showNormal();
                    }
                    setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
                    raise();
                    activateWindow();
                }
            });
            m_restore_timer->start(150);
        }
    }
#endif
}

void MainWindow::UpdateSystemTrayStrings() {
    if (!m_tray_menu) {
        return;
    }
    if (m_tray_restore_action) {
        m_tray_restore_action->setText(StormLang(
            QStringLiteral("Развернуть STORM SWITCH"),
            QStringLiteral("Restore STORM SWITCH"),
            QStringLiteral("STORM SWITCH wiederherstellen"),
            QStringLiteral("Restaurer STORM SWITCH"),
            QStringLiteral("展开 STORM SWITCH"),
            QStringLiteral("STORM SWITCH を表示"),
            QStringLiteral("استعادة STORM SWITCH"),
            QStringLiteral("Restaurar STORM SWITCH")
        ));
    }
    if (m_tray_about_action) {
        m_tray_about_action->setText(StormLang(
            QStringLiteral("О программе"),
            QStringLiteral("About"),
            QStringLiteral("Über das Programm"),
            QStringLiteral("À propos"),
            QStringLiteral("关于程序"),
            QStringLiteral("バージョン情報"),
            QStringLiteral("حول البرنامج"),
            QStringLiteral("Acerca de")
        ));
    }
    if (m_tray_exit_action) {
        m_tray_exit_action->setText(StormLang(
            QStringLiteral("Выход"),
            QStringLiteral("Exit"),
            QStringLiteral("Beenden"),
            QStringLiteral("Quitter"),
            QStringLiteral("退出"),
            QStringLiteral("終了"),
            QStringLiteral("خروج"),
            QStringLiteral("Salir")
        ));
    }
}

void MainWindow::HideMouseCursor() {
    if (QtCommon::emu_thread == nullptr && UISettings::values.hide_mouse) {
        mouse_hide_timer.stop();
        ShowMouseCursor();
        return;
    }
    render_window->setCursor(QCursor(Qt::BlankCursor));
}

void MainWindow::ShowMouseCursor() {
    render_window->unsetCursor();
    if (QtCommon::emu_thread != nullptr && UISettings::values.hide_mouse)
        mouse_hide_timer.start();
}

void MainWindow::OnMouseActivity() {
    if (!Settings::values.mouse_panning) {
        ShowMouseCursor();
    }
}

void MainWindow::OnCheckFirmwareDecryption() {
    if (!ContentManager::AreKeysPresent()) {
        QMessageBox msg(this);
        msg.setWindowTitle(StormLang(
            QStringLiteral("Отсутствуют ключи дешифрования"),
            QStringLiteral("Decryption Keys Missing"),
            QStringLiteral("Entschlüsselungsschlüssel fehlen"),
            QStringLiteral("Clés de déchiffrement manquantes"),
            QStringLiteral("缺少解密密钥"),
            QStringLiteral("復号キーがありません")
        ));
        msg.setText(StormLang(
            QStringLiteral("Ключи шифрования (prod.keys) не найдены.\nЖелаете загрузить и установить актуальные ключи онлайн?"),
            QStringLiteral("Decryption keys (prod.keys) not found.\nWould you like to download and install the latest keys online?"),
            QStringLiteral("Entschlüsselungsschlüssel (prod.keys) nicht gefunden.\nMöchten Sie die neuesten Schlüssel online herunterladen und installieren?"),
            QStringLiteral("Clés de déchiffrement (prod.keys) introuvables.\nSouhaitez-vous télécharger et installer les clés actuelles en ligne ?"),
            QStringLiteral("未找到加密密钥 (prod.keys)。\n是否要在线下载并安装最新的密钥？"),
            QStringLiteral("暗号化キー (prod.keys) が見つかりません。\nオンラインで最新のキーをダウンロードしてインストールしますか？")
        ));
        msg.setIcon(QMessageBox::Question);
        auto* online_btn = msg.addButton(StormLang(
            QStringLiteral("🌐 Установить онлайн"),
            QStringLiteral("🌐 Install Online"),
            QStringLiteral("🌐 Online installieren"),
            QStringLiteral("🌐 Installer en ligne"),
            QStringLiteral("🌐 在线安装"),
            QStringLiteral("🌐 オンラインでインストール")
        ), QMessageBox::ActionRole);
        auto* file_btn = msg.addButton(StormLang(
            QStringLiteral("📁 Выбрать файл"),
            QStringLiteral("📁 Select File"),
            QStringLiteral("📁 Datei auswählen"),
            QStringLiteral("📁 Choisir un fichier"),
            QStringLiteral("📁 选择文件"),
            QStringLiteral("📁 ファイルを選択")
        ), QMessageBox::ActionRole);
        msg.addButton(StormLang(
            QStringLiteral("Позже"),
            QStringLiteral("Later"),
            QStringLiteral("Später"),
            QStringLiteral("Plus tard"),
            QStringLiteral("稍后"),
            QStringLiteral("後で")
        ), QMessageBox::RejectRole);
        msg.exec();

        if (msg.clickedButton() == online_btn) {
            OnInstallKeysOnline();
        } else if (msg.clickedButton() == file_btn) {
            OnInstallDecryptionKeys();
        }
    }

    SetFirmwareVersion();
    UpdateMenuState();
}

void MainWindow::ShowFirmwareContextMenu() {
    QMenu context_menu(this);
    auto* header = context_menu.addAction(StormLang(
        QStringLiteral("🛠️ Прошивка и ключи Nintendo Switch"),
        QStringLiteral("🛠️ Nintendo Switch Firmware and Keys"),
        QStringLiteral("🛠️ Nintendo Switch Firmware und Schlüssel"),
        QStringLiteral("🛠️ Firmware et clés Nintendo Switch"),
        QStringLiteral("🛠️ Nintendo Switch 固件与密钥"),
        QStringLiteral("🛠️ Nintendo Switch ファームウェアとキー")
    ));
    header->setEnabled(false);
    context_menu.addSeparator();
    context_menu.addAction(StormLang(
        QStringLiteral("🌐 Онлайн-установка прошивки из сети..."),
        QStringLiteral("🌐 Online Install Firmware from Network..."),
        QStringLiteral("🌐 Online-Firmware-Installation aus dem Netzwerk..."),
        QStringLiteral("🌐 Installation en ligne du firmware depuis le réseau..."),
        QStringLiteral("🌐 从网络在线安装固件..."),
        QStringLiteral("🌐 ネットワークからファームウェアをオンラインインストール...")
    ), this, &MainWindow::OnInstallFirmwareOnline);
    context_menu.addAction(StormLang(
        QStringLiteral("📦 Установить прошивку из ZIP..."),
        QStringLiteral("📦 Install Firmware from ZIP..."),
        QStringLiteral("📦 Firmware aus ZIP installieren..."),
        QStringLiteral("📦 Installer le firmware depuis un ZIP..."),
        QStringLiteral("📦 从 ZIP 安装固件..."),
        QStringLiteral("📦 ZIP からファームウェアをインストール...")
    ), this, &MainWindow::OnInstallFirmwareFromZIP);
    context_menu.addAction(StormLang(
        QStringLiteral("📁 Установить прошивку из папки..."),
        QStringLiteral("📁 Install Firmware from Folder..."),
        QStringLiteral("📁 Firmware aus Ordner installieren..."),
        QStringLiteral("📁 Installer le firmware depuis un dossier..."),
        QStringLiteral("📁 从文件夹安装固件..."),
        QStringLiteral("📁 フォルダからファームウェアをインストール...")
    ), this, &MainWindow::OnInstallFirmware);
    context_menu.addSeparator();
    context_menu.addAction(StormLang(
        QStringLiteral("🔑 Онлайн-установка ключей из сети..."),
        QStringLiteral("🔑 Online Install Keys from Network..."),
        QStringLiteral("🔑 Online-Schlüssel-Installation aus dem Netzwerk..."),
        QStringLiteral("🔑 Installation en ligne des clés depuis le réseau..."),
        QStringLiteral("🔑 从网络在线安装密钥..."),
        QStringLiteral("🔑 ネットワークからキーをオンラインインストール...")
    ), this, &MainWindow::OnInstallKeysOnline);
    context_menu.addAction(StormLang(
        QStringLiteral("📦 Установить ключи из ZIP..."),
        QStringLiteral("📦 Install Keys from ZIP..."),
        QStringLiteral("📦 Schlüssel aus ZIP installieren..."),
        QStringLiteral("📦 Installer les clés depuis un ZIP..."),
        QStringLiteral("📦 从 ZIP 安装密钥..."),
        QStringLiteral("📦 ZIP からキーをインストール...")
    ), this, &MainWindow::OnInstallKeysFromZIP);
    context_menu.addAction(StormLang(
        QStringLiteral("🔑 Установить ключи из файла (prod.keys)..."),
        QStringLiteral("🔑 Install Decryption Keys (prod.keys)..."),
        QStringLiteral("🔑 Entschlüsselungsschlüssel installieren (prod.keys)..."),
        QStringLiteral("🔑 Installer les clés de déchiffrement (prod.keys)..."),
        QStringLiteral("🔑 从文件安装解密密钥 (prod.keys)..."),
        QStringLiteral("🔑 ファイルから復号キーをインストール (prod.keys)...")
    ), this, &MainWindow::OnInstallDecryptionKeys);
    context_menu.addSeparator();
    context_menu.addAction(StormLang(
        QStringLiteral("📁 Открыть папку NAND..."),
        QStringLiteral("📁 Open NAND Folder..."),
        QStringLiteral("📁 NAND-Ordner öffnen..."),
        QStringLiteral("📁 Ouvrir le dossier NAND..."),
        QStringLiteral("📁 打开 NAND 文件夹..."),
        QStringLiteral("📁 NAND フォルダを開く...")
    ), this, &MainWindow::OnOpenNANDFolder);

    context_menu.exec(QCursor::pos());
}

#ifdef __unix__
void MainWindow::OnCheckGraphicsBackend() {
    const QString platformName = QGuiApplication::platformName();
    const QByteArray qtPlatform = qgetenv("QT_QPA_PLATFORM");

    if (platformName == QStringLiteral("xcb") || qtPlatform == "xcb")
        return;

    const bool isWayland =
        platformName.startsWith(QStringLiteral("wayland"), Qt::CaseInsensitive) ||
        qtPlatform.startsWith("wayland");
    if (!isWayland)
        return;

    const bool currently_hidden = UISettings::values.gui_hide_backend_warning.GetValue();
    if (currently_hidden)
        return;

    QMessageBox msgbox(this);
    msgbox.setWindowTitle(tr("Wayland Detected!"));
    msgbox.setText(
        tr("Wayland is known to have significant performance issues and mysterious bugs.\n"
           "It's recommended to use X11 instead.\n\n"
           "Would you like to force it for future launches?"));
    msgbox.setIcon(QMessageBox::Warning);

    QPushButton* okButton = msgbox.addButton(tr("Use X11"), QMessageBox::AcceptRole);
    msgbox.addButton(tr("Continue with Wayland"), QMessageBox::RejectRole);
    msgbox.setDefaultButton(okButton);

    QCheckBox* cb = new QCheckBox(tr("Don't show again"), &msgbox);
    cb->setChecked(currently_hidden);
    msgbox.setCheckBox(cb);

    msgbox.exec();

    const bool hide = cb->isChecked();
    if (hide != currently_hidden) {
        UISettings::values.gui_hide_backend_warning.SetValue(hide);
    }

    if (msgbox.clickedButton() == okButton) {
        UISettings::values.gui_force_x11.SetValue(true);
        GraphicsBackend::SetForceX11(true);
        QMessageBox::information(this, tr("Restart Required"),
                                 tr("Restart STORM SWITCH to apply the X11 backend."));
    }
}
#endif

bool MainWindow::CheckFirmwarePresence() {
    return FirmwareManager::CheckFirmwarePresence(*QtCommon::system.get());
}

void MainWindow::SetFirmwareVersion() {
    const auto pair = FirmwareManager::GetFirmwareVersion(*QtCommon::system.get());
    const auto firmware_data = pair.first;
    const auto result = pair.second;

    if (result.IsError() || !CheckFirmwarePresence()) {
        LOG_INFO(Frontend, "Installed firmware: No firmware available");
        ui->menu_Applets->setEnabled(false);
        ui->menu_Create_Shortcuts->setEnabled(false);
        firmware_label->setAlignment(Qt::AlignCenter);
        firmware_label->setText(StormLang(
            QStringLiteral("ПРОШИВКА:\nНЕТ"),
            QStringLiteral("FIRMWARE:\nNONE"),
            QStringLiteral("FIRMWARE:\nKEINE"),
            QStringLiteral("FIRMWARE:\nAUCUN"),
            QStringLiteral("固件:\n无"),
            QStringLiteral("ファームウェア:\nなし")
        ));
        firmware_label->setToolTip(StormLang(
            QStringLiteral("Прошивка не установлена (Инструменты -> Установить прошивку)"),
            QStringLiteral("Firmware not installed (Tools -> Install firmware)"),
            QStringLiteral("Firmware nicht installiert (Werkzeuge -> Firmware installieren)"),
            QStringLiteral("Firmware non installé (Outils -> Installer le firmware)"),
            QStringLiteral("固件未安装 (工具 -> 安装固件)"),
            QStringLiteral("ファームウェアがインストールされていません (ツール -> ファームウェアのインストール)")
        ));
        firmware_label->setVisible(true);
        return;
    }

    firmware_label->setVisible(true);
    ui->menu_Applets->setEnabled(true);
    ui->menu_Create_Shortcuts->setEnabled(true);

    const std::string display_version(firmware_data.display_version.data());
    const std::string display_title(firmware_data.display_title.data());

    static std::string s_last_logged_fw;
    if (s_last_logged_fw != display_version) {
        s_last_logged_fw = display_version;
        LOG_INFO(Frontend, "Installed firmware: {}", display_version);
    }

    firmware_label->setAlignment(Qt::AlignCenter);
    firmware_label->setText(StormLang(
        QStringLiteral("ПРОШИВКА:\n%1"),
        QStringLiteral("FIRMWARE:\n%1"),
        QStringLiteral("FIRMWARE:\n%1"),
        QStringLiteral("FIRMWARE:\n%1"),
        QStringLiteral("固件:\n%1"),
        QStringLiteral("ファームウェア:\n%1")
    ).arg(QString::fromStdString(display_version)));
    firmware_label->setToolTip(QString::fromStdString(display_title));
}

void MainWindow::SetFPSSuffix() {
    switch (Settings::values.current_speed_mode.GetValue()) {
    case Settings::SpeedMode::Slow:
        m_fpsSuffix = tr("Slow");
        break;
    case Settings::SpeedMode::Turbo:
        m_fpsSuffix = tr("Turbo");
        break;
    case Settings::SpeedMode::Standard:
        const bool limited = Settings::values.use_speed_limit.GetValue();
        m_fpsSuffix = limited ? QString{} : tr("Unlocked");
        break;
    }
}

bool MainWindow::SelectRomFSDumpTarget(const FileSys::ContentProvider& installed, u64 program_id,
                                       u64* selected_title_id, u8* selected_content_record_type) {
    using ContentInfo = std::tuple<u64, FileSys::TitleType, FileSys::ContentRecordType>;
    boost::container::flat_set<ContentInfo> available_title_ids;

    const auto RetrieveEntries = [&](FileSys::TitleType title_type,
                                     FileSys::ContentRecordType record_type) {
        const auto entries = installed.ListEntriesFilter(title_type, record_type);
        for (const auto& entry : entries) {
            if (FileSys::GetBaseTitleID(entry.title_id) == program_id &&
                installed.GetEntry(entry)->GetStatus() == Loader::ResultStatus::Success) {
                available_title_ids.insert({entry.title_id, title_type, record_type});
            }
        }
    };

    RetrieveEntries(FileSys::TitleType::Application, FileSys::ContentRecordType::Program);
    RetrieveEntries(FileSys::TitleType::Application, FileSys::ContentRecordType::HtmlDocument);
    RetrieveEntries(FileSys::TitleType::Application, FileSys::ContentRecordType::LegalInformation);
    RetrieveEntries(FileSys::TitleType::AOC, FileSys::ContentRecordType::Data);

    if (available_title_ids.empty()) {
        return false;
    }

    size_t title_index = 0;

    if (available_title_ids.size() > 1) {
        QStringList list;
        for (auto& [title_id, title_type, record_type] : available_title_ids) {
            const auto hex_title_id = QString::fromStdString(fmt::format("{:X}", title_id));
            if (record_type == FileSys::ContentRecordType::Program) {
                list.push_back(QStringLiteral("Program [%1]").arg(hex_title_id));
            } else if (record_type == FileSys::ContentRecordType::HtmlDocument) {
                list.push_back(QStringLiteral("HTML document [%1]").arg(hex_title_id));
            } else if (record_type == FileSys::ContentRecordType::LegalInformation) {
                list.push_back(QStringLiteral("Legal information [%1]").arg(hex_title_id));
            } else {
                list.push_back(
                    QStringLiteral("DLC %1 [%2]").arg(title_id & 0x7FF).arg(hex_title_id));
            }
        }

        bool ok;
        const auto res = QInputDialog::getItem(
            this, tr("Select RomFS Dump Target"),
            tr("Please select which RomFS you would like to dump."), list, 0, false, &ok);
        if (!ok) {
            return false;
        }

        title_index = list.indexOf(res);
    }

    const auto& [title_id, title_type, record_type] = *available_title_ids.nth(title_index);
    *selected_title_id = title_id;
    *selected_content_record_type = static_cast<u8>(record_type);
    return true;
}

bool MainWindow::ConfirmClose() {
    if (QtCommon::emu_thread == nullptr ||
        UISettings::values.confirm_before_stopping.GetValue() == ConfirmStop::Ask_Never)
        return true;

    if (!QtCommon::system->GetExitLocked() &&
        UISettings::values.confirm_before_stopping.GetValue() == ConfirmStop::Ask_Based_On_Game)
        return true;

    const auto text = StormLang(
        QStringLiteral("Вы действительно хотите закрыть STORM SWITCH?"),
        QStringLiteral("Are you sure you want to close STORM SWITCH?"),
        QStringLiteral("Möchten Sie STORM SWITCH wirklich schließen?"),
        QStringLiteral("Voulez-vous vraiment fermer STORM SWITCH ?"),
        QStringLiteral("您确定要关闭 STORM SWITCH 吗？"),
        QStringLiteral("STORM SWITCH を本当に終了しますか？")
    );
    return question(this, QStringLiteral("STORM SWITCH"), text);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (!ConfirmClose()) {
        m_is_quitting_from_tray = false;
        event->ignore();
        return;
    }

    if (m_tray_icon) {
        m_tray_icon->hide();
    }
#ifdef _WIN32
    if (m_restore_timer) {
        m_restore_timer->stop();
    }
    if (m_restore_event) {
        CloseHandle(static_cast<HANDLE>(m_restore_event));
        m_restore_event = nullptr;
    }
#endif
    update_input_timer.stop();
    mouse_hide_timer.stop();

    UpdateUISettings();
    game_list->SaveInterfaceLayout();
    SaveFooterSettings();
    UISettings::SaveWindowState();
    hotkey_registry.SaveHotkeys();
    if (config) {
        config->SaveAllValues();
    }

    // Unload controllers early
    controller_dialog->UnloadController();
    game_list->UnloadController();

    // Shutdown session if the emu thread is active...
    if (QtCommon::emu_thread != nullptr)
        ShutdownGame();

    Core::FlushPendingSaveStates();
    RestoreSessionSettings();
    if (config) {
        config->SaveAllValues();
    }

    render_window->close();
    multiplayer_state->Close();
    QtCommon::system->HIDCore().UnloadInputDevices();
    Network::Shutdown();

    QWidget::closeEvent(event);
    QCoreApplication::quit();
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    if (loading_screen && loading_screen->isVisible()) {
        loading_screen->setGeometry(ui->centralwidget->rect());
    }
    if (in_game_notification && in_game_notification->isVisible()) {
        in_game_notification->RepositionToParent();
    }
    emit sizeChanged(event->size());
}

void MainWindow::moveEvent(QMoveEvent* event) {
    if (in_game_notification && in_game_notification->isVisible()) {
        in_game_notification->RepositionToParent();
    }
    auto window_frame_height = frameGeometry().height() - geometry().height();
    emit positionChanged(event->pos() - QPoint{0, window_frame_height});
}

static bool IsSingleFileDropEvent(const QMimeData* mime) {
    return mime->hasUrls() && mime->urls().length() == 1;
}

void MainWindow::AcceptDropEvent(QDropEvent* event) {
    if (IsSingleFileDropEvent(event->mimeData())) {
        event->setDropAction(Qt::DropAction::LinkAction);
        event->accept();
    }
}

bool MainWindow::DropAction(QDropEvent* event) {
    if (!IsSingleFileDropEvent(event->mimeData())) {
        return false;
    }

    const QMimeData* mime_data = event->mimeData();
    const QString& filename = mime_data->urls().at(0).toLocalFile();

    if (emulation_running && QFileInfo(filename).suffix() == QStringLiteral("bin")) {
        // Amiibo
        LoadAmiibo(filename);
    } else {
        // Game
        if (ConfirmChangeGame()) {
            BootGame(filename, ApplicationAppletParameters());
        }
    }
    return true;
}

void MainWindow::dropEvent(QDropEvent* event) {
    DropAction(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    AcceptDropEvent(event);
}

void MainWindow::dragMoveEvent(QDragMoveEvent* event) {
    AcceptDropEvent(event);
}

void MainWindow::ShowDLCDialog(u64 title_id, const QString& game_name) {
    if (title_id == 0) {
        QMessageBox::information(this, QStringLiteral("STORM SWITCH"), StormLang(
            QStringLiteral("Нет выделенной или запущенной игры."),
            QStringLiteral("No selected or running game."),
            QStringLiteral("Kein ausgewähltes oder laufendes Spiel."),
            QStringLiteral("Aucun jeu sélectionné ou en cours d'exécution."),
            QStringLiteral("没有选中或正在运行的游戏。"),
            QStringLiteral("選択中または実行中のゲームがありません。")
        ));
        return;
    }

    const FileSys::PatchManager patch_manager(title_id, QtCommon::system->GetFileSystemController(), QtCommon::system->GetContentProvider());
    const auto patches = patch_manager.GetPatches();
    const auto& provider = QtCommon::system->GetContentProvider();
    TitleDB::TitleDatabase::Instance().WaitLoaded(std::chrono::milliseconds(3000));

    const int parent_width = this->width();
    const int parent_height = this->height();
    const int target_width = std::clamp(static_cast<int>(parent_width * 0.92), 760, 1800);
    const int target_height = std::clamp(static_cast<int>(parent_height * 0.88), 520, 1080);

    QDialog dlg(this);
    dlg.setWindowTitle(StormLang(
        QStringLiteral("STORM SWITCH — Менеджер дополнений"),
        QStringLiteral("STORM SWITCH — Add-ons Manager"),
        QStringLiteral("STORM SWITCH — Add-ons-Manager"),
        QStringLiteral("STORM SWITCH — Gestionnaire d'extensions"),
        QStringLiteral("STORM SWITCH — 附加组件管理器"),
        QStringLiteral("STORM SWITCH — アドオンマネージャー")
    ));
    dlg.resize(target_width, target_height);
    dlg.setMinimumSize(680, 460);
    dlg.setStyleSheet(QStringLiteral(
        "QDialog { background-color: #0b0f19; color: #ffffff; font-family: 'Segoe UI', sans-serif; }"
        "QLabel { color: #ffffff; }"
        "QLineEdit { background-color: #141b2a; color: #ffffff; border: 1px solid #23314a; border-radius: 6px; padding: 7px 12px; font-size: 9pt; }"
        "QLineEdit:focus { border: 1px solid #00e5ff; }"
        "QTableWidget { background-color: #0f1422; color: #ffffff; gridline-color: #1b2438; border: 1px solid #1c273c; border-radius: 8px; selection-background-color: rgba(0, 229, 255, 0.25); selection-color: #ffffff; font-size: 9pt; }"
        "QHeaderView::section { background-color: #161e30; color: #00e5ff; font-weight: bold; border: 1px solid #1c273c; padding: 7px; font-size: 9pt; }"
        "QPushButton { background-color: #161e30; color: #ffffff; border: 1px solid #23314a; border-radius: 6px; padding: 7px 16px; font-weight: bold; font-size: 8.5pt; }"
        "QPushButton:hover { background-color: #00e5ff; color: #000000; border-color: #00e5ff; }"
        "QPushButton#CopyBtn { background-color: #1b273d; color: #00e5ff; border: 1px solid #00e5ff; }"
        "QPushButton#CopyBtn:hover { background-color: #00e5ff; color: #000000; }"
        "QMenu { background-color: #121826; color: #ffffff; border: 1px solid #1e283d; padding: 4px; }"
        "QMenu::item:selected { background-color: #00e5ff; color: #000000; font-weight: bold; border-radius: 4px; }"
    ));

    auto* main_layout = new QVBoxLayout(&dlg);
    main_layout->setContentsMargins(18, 18, 18, 18);
    main_layout->setSpacing(12);

    QString full_display_name;
    if (!m_current_addons_game_path.empty()) {
        const QFileInfo fi(QString::fromStdString(m_current_addons_game_path));
        full_display_name = fi.completeBaseName();
    }
    if (full_display_name.isEmpty()) {
        full_display_name = game_name.isEmpty() ? QString::fromStdString(fmt::format("{:016X}", title_id)) : game_name;
    }

    const QString tid_str = QString::fromStdString(fmt::format("{:016X}", title_id));

    QString base_clean_name;
    const auto base_tdb = TitleDB::TitleDatabase::Instance().Lookup(title_id);
    if (base_tdb.has_value() && !base_tdb->name.empty()) {
        base_clean_name = QString::fromStdString(base_tdb->name).trimmed();
    }
    if (base_clean_name.isEmpty() && !game_name.isEmpty()) {
        base_clean_name = game_name.trimmed();
    }

    const auto tdb_dlcs = TitleDB::TitleDatabase::Instance().GetDlcs(title_id);

    auto clean_item_name = [&](QString raw_name) -> QString {
        raw_name = raw_name.trimmed();
        if (raw_name.isEmpty()) return QString{};

        if (!base_clean_name.isEmpty()) {
            if (raw_name.startsWith(base_clean_name, Qt::CaseInsensitive)) {
                raw_name = raw_name.mid(base_clean_name.length()).trimmed();
            } else {
                const QStringList parts = base_clean_name.split(QRegularExpression(QStringLiteral("[:—–\\-]")), Qt::SkipEmptyParts);
                for (const auto& part : parts) {
                    const QString trimmed_part = part.trimmed();
                    if (trimmed_part.length() >= 4) {
                        const int idx = raw_name.indexOf(trimmed_part, 0, Qt::CaseInsensitive);
                        if (idx >= 0) {
                            raw_name.remove(idx, trimmed_part.length());
                            raw_name = raw_name.trimmed();
                        }
                    }
                }
            }
        }

        if (raw_name.startsWith(QLatin1Char('T')) && raw_name.length() > 1 && (raw_name[1] == QLatin1Char(':') || raw_name[1] == QLatin1Char(' '))) {
            raw_name = raw_name.mid(1).trimmed();
        }

        while (!raw_name.isEmpty() && (raw_name.startsWith(QLatin1Char(':')) ||
                                       raw_name.startsWith(QLatin1Char('-')) ||
                                       raw_name.startsWith(QStringLiteral("—")) ||
                                       raw_name.startsWith(QStringLiteral("–")) ||
                                       raw_name.startsWith(QLatin1Char('|')) ||
                                       raw_name.startsWith(QLatin1Char('~')) ||
                                       raw_name.startsWith(QLatin1Char(' ')))) {
            raw_name = raw_name.mid(1).trimmed();
        }
        return raw_name;
    };

    auto resolve_dlc_name = [&](u64 dlc_tid, int dlc_order, const QString& nacp_fallback = QString{}) -> QString {
        const int dlc_num = static_cast<int>(dlc_tid & 0x7FF);
        const int num_to_show = dlc_order > 0 ? dlc_order : (dlc_num > 0 ? dlc_num : 1);

        const auto tdb = TitleDB::TitleDatabase::Instance().Lookup(dlc_tid);
        if (tdb.has_value() && !tdb->name.empty()) {
            QString name_str = QString::fromStdString(tdb->name).trimmed();
            QString cleaned = clean_item_name(name_str);
            if (!cleaned.isEmpty()) return cleaned;
            return name_str;
        }

        const std::string d_hex = fmt::format("{:016X}", dlc_tid);
        for (const auto& d : tdb_dlcs) {
            if (d.id == d_hex && !d.name.empty()) {
                QString name_str = QString::fromStdString(d.name).trimmed();
                QString cleaned = clean_item_name(name_str);
                if (!cleaned.isEmpty()) return cleaned;
                return name_str;
            }
        }

        if (dlc_order > 0 && dlc_order <= static_cast<int>(tdb_dlcs.size())) {
            const auto& d = tdb_dlcs[dlc_order - 1];
            if (!d.name.empty()) {
                QString name_str = QString::fromStdString(d.name).trimmed();
                QString cleaned = clean_item_name(name_str);
                if (!cleaned.isEmpty()) return cleaned;
                return name_str;
            }
        }

        if (!nacp_fallback.trimmed().isEmpty()) {
            const QString cleaned = clean_item_name(nacp_fallback);
            if (!cleaned.isEmpty()) return cleaned;
            return nacp_fallback;
        }

        return tr("Дополнение #%1").arg(num_to_show);
    };

    auto resolve_dlc_desc = [&](u64 dlc_tid, int dlc_order) -> QString {
        const auto tdb = TitleDB::TitleDatabase::Instance().Lookup(dlc_tid);
        if (tdb.has_value() && !tdb->description.empty()) {
            const QString desc = QString::fromStdString(tdb->description).trimmed();
            if (!base_tdb.has_value() || desc != QString::fromStdString(base_tdb->description).trimmed()) {
                return desc;
            }
        }
        if (dlc_order > 0 && dlc_order <= static_cast<int>(tdb_dlcs.size())) {
            const auto& d = tdb_dlcs[dlc_order - 1];
            if (!d.description.empty()) {
                const QString desc = QString::fromStdString(d.description).trimmed();
                if (!base_tdb.has_value() || desc != QString::fromStdString(base_tdb->description).trimmed()) {
                    return desc;
                }
            }
        }
        return tr("Официальный загружаемый контент (DLC). Включает дополнительные игровые материалы, бонусы или сценарии.");
    };

    struct RowItem {
        QString type;
        QString tid;
        QString name;
        QString desc;
        QString ver;
        QString internal_ver;
        QString status;
        int type_priority{0}; // 0 = Update, 1 = DLC, 2 = Mod
        u64 raw_tid{0};
    };
    std::vector<RowItem> rows;

    int total_dlcs = 0;
    int total_updates = 0;
    int total_mods = 0;

    QString update_ver_str;
    QString update_internal_ver_str = QStringLiteral("0");

    // 1. Try PatchManager control metadata
    if (const auto nacp = patch_manager.GetControlMetadata().first; nacp != nullptr) {
        const auto ver = nacp->GetVersionString();
        if (!ver.empty() && ver != "0") {
            update_ver_str = QString::fromStdString(ver);
        }
    }

    // 2. Try provider update version
    const u32 prov_update_num = provider.GetEntryVersion(FileSys::GetUpdateTitleID(title_id)).value_or(0);
    if (prov_update_num > 0) {
        update_internal_ver_str = QString::number(prov_update_num);
        if (update_ver_str.isEmpty() || update_ver_str == QStringLiteral("1.0.0") || update_ver_str == QStringLiteral("0")) {
            update_ver_str = QString::number(prov_update_num);
        }
    }

    // 3. Try reading NACP directly from the running file
    if ((update_ver_str.isEmpty() || update_ver_str == QStringLiteral("1.0.0") || update_ver_str == QStringLiteral("0")) && !m_current_addons_game_path.empty()) {
        const auto v_file = Core::GetGameFileFromPath(QtCommon::vfs, m_current_addons_game_path);
        if (v_file) {
            const auto file_loader = Loader::GetLoader(*QtCommon::system, v_file);
            if (file_loader) {
                FileSys::NACP file_nacp;
                if (file_loader->ReadControlData(file_nacp) == Loader::ResultStatus::Success) {
                    const auto ver = file_nacp.GetVersionString();
                    if (!ver.empty() && ver != "0") {
                        update_ver_str = QString::fromStdString(ver);
                    }
                }
            }
        }
    }

    // 4. Try matching version pair (e.g. "(1.0.7 - 393216 - ...") from filename
    if (!m_current_addons_game_path.empty()) {
        static const QRegularExpression fn_pair_ver_regex{QStringLiteral(R"(\(([0-9]+\.[0-9]+(?:\.[0-9]+)*)\s*-\s*([0-9]+))")};
        const auto fm = fn_pair_ver_regex.match(QString::fromStdString(m_current_addons_game_path));
        if (fm.hasMatch() && !fm.captured(1).isEmpty()) {
            update_ver_str = fm.captured(1);
            update_internal_ver_str = fm.captured(2);
        } else {
            static const QRegularExpression fn_ver_regex{QStringLiteral(R"((?:[\(\[\s]v?|\b)([0-9]+\.[0-9]+(?:\.[0-9]+)*)(?!\s*(?:GB|MB|KB|TB|ГБ|МБ|КБ|Б|B)\b))")};
            const auto m = fn_ver_regex.match(QString::fromStdString(m_current_addons_game_path));
            if (m.hasMatch() && m.hasCaptured(1) && (update_ver_str.isEmpty() || update_ver_str == QStringLiteral("1.0.0"))) {
                update_ver_str = m.captured(1);
            }
            static const QRegularExpression fn_vnum_regex{QStringLiteral(R"(\[v([0-9]+)\])")};
            const auto vm = fn_vnum_regex.match(QString::fromStdString(m_current_addons_game_path));
            if (vm.hasMatch() && update_internal_ver_str == QStringLiteral("0")) {
                update_internal_ver_str = vm.captured(1);
            }
        }
    }

    while (update_ver_str.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        update_ver_str.remove(0, 1);
    }
    update_ver_str = update_ver_str.trimmed();

    if (update_ver_str.isEmpty() || update_ver_str == QStringLiteral("0")) {
        update_ver_str = QStringLiteral("1.0.0");
    }

    // 1. Collect individual DLCs from ContentProvider
    const auto aoc_data = provider.ListEntriesFilter(
        FileSys::TitleType::AOC, FileSys::ContentRecordType::Data);
    std::set<u64> seen_dlc_ids;
    std::set<u64> seen_update_ids;

    for (const auto& entry : aoc_data) {
        if (FileSys::GetBaseTitleID(entry.title_id) == FileSys::GetBaseTitleID(title_id) ||
            (entry.title_id >= title_id + 1 && entry.title_id < title_id + 0x2000)) {
            seen_dlc_ids.insert(entry.title_id);
            total_dlcs++;

            QString nacp_title;
            const auto dlc_ctrl = provider.GetEntry(entry.title_id, FileSys::ContentRecordType::Control);
            if (dlc_ctrl) {
                const auto [nacp, icon] = patch_manager.ParseControlNCA(*dlc_ctrl);
                if (nacp) {
                    nacp_title = QString::fromStdString(nacp->GetApplicationName());
                }
            }

            const QString dlc_name = resolve_dlc_name(entry.title_id, total_dlcs, nacp_title);
            const QString dlc_desc = resolve_dlc_desc(entry.title_id, total_dlcs);
            const u32 dlc_ver = provider.GetEntryVersion(entry.title_id).value_or(0);

            rows.push_back({
                tr("DLC"),
                QString::fromStdString(fmt::format("{:016X}", entry.title_id)),
                dlc_name,
                dlc_desc,
                QStringLiteral("1.0.0"),
                QString::number(dlc_ver),
                tr("✓ В файле / Активно"),
                1,
                entry.title_id,
            });
        }
    }

    // 2. Fallback for DLCs listed in PatchManager if not separately indexed in provider
    for (const auto& p : patches) {
        if (p.type == FileSys::PatchType::DLC) {
            const QStringList dlc_indices = QString::fromStdString(p.version).split(QLatin1Char(','), Qt::SkipEmptyParts);
            for (const auto& idx_str : dlc_indices) {
                const u32 idx_val = idx_str.trimmed().toUInt();
                const u64 generated_tid = (title_id & 0xFFFFFFFFFFFFF000) | (idx_val > 0 ? (0x1000 | (idx_val & 0x7FF)) : 0x1001);
                if (seen_dlc_ids.find(generated_tid) == seen_dlc_ids.end()) {
                    seen_dlc_ids.insert(generated_tid);
                    total_dlcs++;

                    const QString dlc_name = resolve_dlc_name(generated_tid, total_dlcs);
                    const QString dlc_desc = resolve_dlc_desc(generated_tid, total_dlcs);

                    rows.push_back({
                        tr("DLC"),
                        QString::fromStdString(fmt::format("{:016X}", generated_tid)),
                        dlc_name,
                        dlc_desc,
                        QStringLiteral("1.0.0"),
                        QStringLiteral("0"),
                        tr("✓ В файле / Активно"),
                        1,
                        generated_tid,
                    });
                }
            }
        } else if (p.type == FileSys::PatchType::Update) {
            seen_update_ids.insert(p.title_id);
            total_updates++;
            u32 update_num = provider.GetEntryVersion(FileSys::GetUpdateTitleID(title_id)).value_or(0);
            if (update_num == 0 && update_internal_ver_str != QStringLiteral("0")) {
                update_num = update_internal_ver_str.toUInt();
            }
            const QString upd_name = tr("Пакет обновления игры");
            const QString upd_desc = tr("Накопительный пакет обновлений. Включает оптимизацию производительности, исправления ошибок и актуальные игровые данные.");

            rows.push_back({
                tr("Update"),
                QString::fromStdString(fmt::format("{:016X}", p.title_id)),
                upd_name,
                upd_desc,
                update_ver_str,
                update_num > 0 ? QString::number(update_num) : update_internal_ver_str,
                p.enabled ? tr("✓ В файле / Активно") : tr("Отключено"),
                0,
                p.title_id,
            });
        } else if (p.type == FileSys::PatchType::Mod) {
            const std::string lower_name = Common::ToLower(p.name);
            if (lower_name == "sdmc" || lower_name == "romfs" ||
                lower_name.find("sdmc") != std::string::npos ||
                lower_name.find("romfs") != std::string::npos) {
                continue;
            }
            total_mods++;
            rows.push_back({
                tr("Mod"),
                QString::fromStdString(fmt::format("{:016X}", p.title_id)),
                QString::fromStdString(p.name),
                tr("Пользовательская модификация игры (LayeredFS)"),
                QStringLiteral("-"),
                QStringLiteral("-"),
                p.enabled ? tr("✓ Установлен") : tr("Отключен"),
                2,
                p.title_id,
            });
        }
    }

    // 3. Fallback scan for DLCs and Updates directly inside the running file / container if not registered in provider
    if (!m_current_addons_game_path.empty()) {
        const auto game_vfs = Core::GetGameFileFromPath(QtCommon::vfs, m_current_addons_game_path);
        if (game_vfs) {
            const auto nsp = std::make_shared<FileSys::NSP>(game_vfs);
            if (nsp && nsp->GetStatus() == Loader::ResultStatus::Success) {
                for (const auto& [nca_tid, nca_map] : nsp->GetNCAs()) {
                    // Embedded Update NCA check (ends in 0x800)
                    if ((nca_tid & 0x800) != 0 && (nca_tid & 0xFFFFFFFFFFFFF000) == (title_id & 0xFFFFFFFFFFFFF000)) {
                        if (seen_update_ids.find(nca_tid) == seen_update_ids.end()) {
                            seen_update_ids.insert(nca_tid);
                            total_updates++;

                            const QString upd_name = tr("Пакет обновления игры");
                            const QString upd_desc = tr("Накопительный пакет обновлений, вшитый в файл игры. Включает исправления и игровые ресурсы.");

                            rows.push_back({
                                tr("Update"),
                                QString::fromStdString(fmt::format("{:016X}", nca_tid)),
                                upd_name,
                                upd_desc,
                                update_ver_str,
                                update_internal_ver_str,
                                tr("✓ В файле / Активно"),
                                0,
                                nca_tid,
                            });
                        }
                    }
                    // Embedded DLC NCA check
                    else if (((nca_tid & 0xFFFFFFFFFFFFF000) == (title_id & 0xFFFFFFFFFFFFF000) ||
                         (nca_tid >= title_id + 1 && nca_tid < title_id + 0x2000)) &&
                        nca_tid != title_id) {
                        if (seen_dlc_ids.find(nca_tid) == seen_dlc_ids.end()) {
                            seen_dlc_ids.insert(nca_tid);
                            total_dlcs++;

                            const QString dlc_name = resolve_dlc_name(nca_tid, total_dlcs);
                            const QString dlc_desc = resolve_dlc_desc(nca_tid, total_dlcs);

                            rows.push_back({
                                tr("DLC"),
                                QString::fromStdString(fmt::format("{:016X}", nca_tid)),
                                dlc_name,
                                dlc_desc,
                                QStringLiteral("1.0.0"),
                                QStringLiteral("0"),
                                tr("✓ В файле / Активно"),
                                1,
                                nca_tid,
                            });
                        }
                    }
                }
            }
        }

        // Check filename tag +1U or (1G+1U)
        static const QRegularExpression fn_u_tag{QStringLiteral(R"((?:\+|\b)([0-9]+)U(?:\b|\)))"), QRegularExpression::CaseInsensitiveOption};
        const auto um = fn_u_tag.match(QString::fromStdString(m_current_addons_game_path));
        if (um.hasMatch() && seen_update_ids.empty()) {
            const u64 upd_tid = FileSys::GetUpdateTitleID(title_id);
            seen_update_ids.insert(upd_tid);
            total_updates++;

            const QString upd_name = tr("Пакет обновления игры");
            const QString upd_desc = tr("Накопительный пакет обновлений, вшитый в файл игры. Включает исправления и игровые ресурсы.");

            rows.push_back({
                tr("Update"),
                QString::fromStdString(fmt::format("{:016X}", upd_tid)),
                upd_name,
                upd_desc,
                update_ver_str,
                update_internal_ver_str,
                tr("✓ В файле / Активно"),
                0,
                upd_tid,
            });
        }

        // Check filename tags like +1D, +2D, (1G+1U+1D), (1G+3D)
        static const QRegularExpression fn_dlc_tag{QStringLiteral(R"((?:\+|\b)([0-9]+)D(?:LC)?(?:\b|\)))"), QRegularExpression::CaseInsensitiveOption};
        const auto dm = fn_dlc_tag.match(QString::fromStdString(m_current_addons_game_path));
        if (dm.hasMatch() && dm.hasCaptured(1)) {
            const int tag_count = dm.captured(1).toInt();
            for (int i = 1; i <= tag_count; ++i) {
                const u64 generated_tid = (title_id & 0xFFFFFFFFFFFFF000) | (0x1000 + i);
                if (seen_dlc_ids.find(generated_tid) == seen_dlc_ids.end()) {
                    seen_dlc_ids.insert(generated_tid);
                    total_dlcs++;

                    const QString dlc_name = resolve_dlc_name(generated_tid, i);
                    const QString dlc_desc = resolve_dlc_desc(generated_tid, i);

                    rows.push_back({
                        tr("DLC"),
                        QString::fromStdString(fmt::format("{:016X}", generated_tid)),
                        dlc_name,
                        dlc_desc,
                        QStringLiteral("1.0.0"),
                        QStringLiteral("0"),
                        tr("✓ В файле / Активно"),
                        1,
                        generated_tid,
                    });
                }
            }
        }
    }

    // 4. Add missing DLCs from TitleDB / Tinfoil that are not present in the game file
    for (size_t i = 0; i < tdb_dlcs.size(); ++i) {
        const auto& dlc_entry = tdb_dlcs[i];
        u64 dlc_tid = 0;
        try {
            dlc_tid = std::stoull(dlc_entry.id, nullptr, 16);
        } catch (...) {
            dlc_tid = (title_id & 0xFFFFFFFFFFFFF000) | (0x1000 + i + 1);
        }

        if (seen_dlc_ids.find(dlc_tid) == seen_dlc_ids.end()) {
            QString name_str = QString::fromStdString(dlc_entry.name).trimmed();
            QString cleaned = clean_item_name(name_str);
            if (cleaned.isEmpty()) cleaned = name_str;
            if (cleaned.isEmpty()) cleaned = tr("Дополнение #%1").arg(i + 1);

            QString desc_str = QString::fromStdString(dlc_entry.description).trimmed();
            if (desc_str.isEmpty() || (base_tdb.has_value() && desc_str == QString::fromStdString(base_tdb->description).trimmed())) {
                desc_str = tr("Официальный загружаемый контент (DLC). Отсутствует в файлах игры.");
            }

            rows.push_back({
                tr("DLC"),
                QString::fromStdString(fmt::format("{:016X}", dlc_tid)),
                cleaned,
                desc_str,
                QStringLiteral("—"),
                QStringLiteral("—"),
                tr("Отсутствует"),
                1,
                dlc_tid,
            });
        }
    }

    auto format_dlc_count_ru = [](int count) -> QString {
        const int mod10 = count % 10;
        const int mod100 = count % 100;
        if (mod100 >= 11 && mod100 <= 19) {
            return tr("%1 дополнений").arg(count);
        }
        if (mod10 == 1) {
            return tr("%1 дополнение").arg(count);
        }
        if (mod10 >= 2 && mod10 <= 4) {
            return tr("%1 дополнения").arg(count);
        }
        return tr("%1 дополнений").arg(count);
    };

    const int tinfoil_dlc_count = TitleDB::TitleDatabase::Instance().GetDlcCount(title_id);
    const QString tinfoil_badge_text = tinfoil_dlc_count > 0 ? format_dlc_count_ru(tinfoil_dlc_count) : (base_tdb.has_value() ? tr("0 дополнений") : tr("Не найдено"));

    auto* header_card = new QWidget(&dlg);
    header_card->setStyleSheet(QStringLiteral("background-color: #121826; border: 1px solid #1e283d; border-radius: 8px; padding: 6px;"));
    auto* header_layout = new QVBoxLayout(header_card);
    header_layout->setContentsMargins(14, 10, 14, 10);
    header_layout->setSpacing(6);

    auto* title_label = new QLabel(tr("<h2 style='margin:0; color:#00e5ff;'>🎮 %1</h2>").arg(full_display_name), header_card);
    auto* badges_label = new QLabel(tr(
        "<span style='background:#1b2438; color:#94a3b8; padding:4px 9px; border-radius:5px; font-weight:bold;'>ID: %1</span> &nbsp; "
        "<span style='background:#143324; color:#00e676; padding:4px 9px; border-radius:5px; font-weight:bold;'>📦 Дополнений в файле: %2</span> &nbsp; "
        "<span style='background:#142738; color:#00e5ff; padding:4px 9px; border-radius:5px; font-weight:bold;'>🗃️ Всего в базе Tinfoil: %3</span> &nbsp; "
        "<span style='background:#2d2915; color:#ffca28; padding:4px 9px; border-radius:5px; font-weight:bold;'>🆙 Версия игры: %4</span> &nbsp; "
        "<span style='background:#2e183a; color:#e040fb; padding:4px 9px; border-radius:5px; font-weight:bold;'>⚡ Модов: %5</span>")
        .arg(tid_str, QString::number(total_dlcs), tinfoil_badge_text, update_ver_str, QString::number(total_mods)), header_card);

    header_layout->addWidget(title_label);
    header_layout->addWidget(badges_label);
    main_layout->addWidget(header_card);

    auto* search_box = new QLineEdit(&dlg);
    search_box->setPlaceholderText(StormLang(
        QStringLiteral("🔍 Поиск по названию, описанию или Title ID..."),
        QStringLiteral("🔍 Search by title, description, or Title ID..."),
        QStringLiteral("🔍 Suche nach Titel, Beschreibung oder Title-ID..."),
        QStringLiteral("🔍 Rechercher par titre, description ou Title ID..."),
        QStringLiteral("🔍 按名称、描述或 Title ID 搜索..."),
        QStringLiteral("🔍 タイトル、説明、または Title ID で検索...")
    ));
    search_box->setClearButtonEnabled(true);
    main_layout->addWidget(search_box);

    auto* table = new QTableWidget(&dlg);
    table->setColumnCount(8);
    table->setHorizontalHeaderLabels({
        QStringLiteral("№"),
        StormLang(QStringLiteral("Тип"), QStringLiteral("Type"), QStringLiteral("Typ"), QStringLiteral("Type"), QStringLiteral("类型"), QStringLiteral("タイプ")),
        QStringLiteral("Title ID"),
        StormLang(QStringLiteral("Полное название"), QStringLiteral("Full Name"), QStringLiteral("Vollständiger Name"), QStringLiteral("Nom complet"), QStringLiteral("完整名称"), QStringLiteral("完全な名前")),
        StormLang(QStringLiteral("Описание"), QStringLiteral("Description"), QStringLiteral("Beschreibung"), QStringLiteral("Description"), QStringLiteral("描述"), QStringLiteral("説明")),
        StormLang(QStringLiteral("Версия"), QStringLiteral("Version"), QStringLiteral("Version"), QStringLiteral("Version"), QStringLiteral("版本"), QStringLiteral("バージョン")),
        StormLang(QStringLiteral("Внутренняя версия"), QStringLiteral("Internal Version"), QStringLiteral("Interne Version"), QStringLiteral("Version interne"), QStringLiteral("内部版本"), QStringLiteral("内部バージョン")),
        StormLang(QStringLiteral("Статус"), QStringLiteral("Status"), QStringLiteral("Status"), QStringLiteral("Statut"), QStringLiteral("状态"), QStringLiteral("ステータス"))
    });
    table->setWordWrap(true);
    table->setTextElideMode(Qt::ElideNone);
    table->setSelectionBehavior(QAbstractItemView::SelectItems);
    table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setContextMenuPolicy(Qt::CustomContextMenu);

    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);
    table->setColumnWidth(3, 400);
    table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Interactive);
    table->setColumnWidth(5, 95);
    table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Interactive);
    table->setColumnWidth(6, 140);
    table->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    table->verticalHeader()->setVisible(false);
    table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    main_layout->addWidget(table);

    int row_idx = 0;
    QStringList copy_lines;
    copy_lines << QStringLiteral("============================================================");
    copy_lines << tr("STORM SWITCH — Список дополнений и обновлений");
    copy_lines << tr("Игра: %1 (ID: %2)").arg(full_display_name, tid_str);
    copy_lines << tr("Версия игры: %1 | Дополнений в файле: %2 | В базе Tinfoil: %3 | Модов: %4")
        .arg(update_ver_str, QString::number(total_dlcs), QString::number(tinfoil_dlc_count), QString::number(total_mods));
    copy_lines << QStringLiteral("------------------------------------------------------------");

    // Sort rows: Updates (0) -> DLCs (1, sorted by raw_tid) -> Mods (2, sorted by name)
    std::stable_sort(rows.begin(), rows.end(), [](const RowItem& a, const RowItem& b) {
        if (a.type_priority != b.type_priority) {
            return a.type_priority < b.type_priority;
        }
        if (a.type_priority == 1) {
            return a.raw_tid < b.raw_tid;
        }
        if (a.type_priority == 2) {
            return a.name < b.name;
        }
        return a.raw_tid < b.raw_tid;
    });

    for (const auto& r : rows) {
        table->insertRow(row_idx);

        auto* item0 = new QTableWidgetItem();
        item0->setData(Qt::EditRole, row_idx + 1);
        item0->setTextAlignment(Qt::AlignCenter);
        table->setItem(row_idx, 0, item0);

        auto* item1 = new QTableWidgetItem(r.type);
        item1->setTextAlignment(Qt::AlignCenter);
        if (r.type == tr("Update")) {
            item1->setForeground(QColor("#ffca28"));
        } else if (r.type == tr("DLC")) {
            item1->setForeground(QColor("#00e5ff"));
        } else if (r.type == tr("Mod")) {
            item1->setForeground(QColor("#e040fb"));
        }
        table->setItem(row_idx, 1, item1);

        auto* item2 = new QTableWidgetItem(r.tid);
        item2->setTextAlignment(Qt::AlignCenter);
        table->setItem(row_idx, 2, item2);

        auto* item3 = new QTableWidgetItem(r.name);
        table->setItem(row_idx, 3, item3);

        auto* item4 = new QTableWidgetItem(r.desc);
        table->setItem(row_idx, 4, item4);

        auto* item5 = new QTableWidgetItem(r.ver);
        item5->setTextAlignment(Qt::AlignCenter);
        table->setItem(row_idx, 5, item5);

        auto* item6 = new QTableWidgetItem(r.internal_ver);
        item6->setTextAlignment(Qt::AlignCenter);
        table->setItem(row_idx, 6, item6);

        auto* item7 = new QTableWidgetItem(r.status);
        item7->setTextAlignment(Qt::AlignCenter);
        if (r.status == tr("Отсутствует")) {
            item7->setText(tr("❌ Отсутствует"));
            item7->setForeground(QColor("#ef4444"));
            item3->setForeground(QColor("#94a3b8"));
            item0->setForeground(QColor("#64748b"));
            item1->setForeground(QColor("#64748b"));
            item2->setForeground(QColor("#64748b"));
            item4->setForeground(QColor("#64748b"));
            item5->setForeground(QColor("#64748b"));
            item6->setForeground(QColor("#64748b"));
        } else if (r.status.contains(QStringLiteral("✓"))) {
            item7->setForeground(QColor("#00e676"));
            QFont f = item7->font();
            f.setBold(true);
            item7->setFont(f);
        }
        table->setItem(row_idx, 7, item7);

        copy_lines << tr("%1. [%2] %3 — %4 | %5 | Версия: %6 (Внутр: %7) | %8")
            .arg(QString::number(row_idx + 1), r.type, r.tid, r.name, r.desc, r.ver, r.internal_ver, r.status);
        row_idx++;
    }

    if (row_idx == 0) {
        table->insertRow(0);
        for (int c = 0; c < 8; ++c) {
            table->setItem(0, c, new QTableWidgetItem(c == 3 ? tr("Дополнения или обновления не обнаружены.") : QStringLiteral("-")));
        }
        copy_lines << tr("Дополнения не найдены.");
    } else {
        copy_lines << QStringLiteral("============================================================");
        copy_lines << tr("Всего элементов: %1").arg(row_idx);
    }

    table->setSortingEnabled(true);

    connect(search_box, &QLineEdit::textChanged, [table](const QString& text) {
        for (int r = 0; r < table->rowCount(); ++r) {
            bool match = false;
            for (int c = 0; c < table->columnCount(); ++c) {
                auto* item = table->item(r, c);
                if (item && item->text().contains(text, Qt::CaseInsensitive)) {
                    match = true;
                    break;
                }
            }
            table->setRowHidden(r, !match);
        }
    });

    // Custom Context Menu for individual field copy
    connect(table, &QTableWidget::customContextMenuRequested, [table](const QPoint& pos) {
        QTableWidgetItem* item = table->itemAt(pos);
        if (!item) return;
        QMenu menu(table);
        menu.addAction(QObject::tr("📋 Копировать ячейку"), [item] {
            QGuiApplication::clipboard()->setText(item->text());
        });
        const int row = item->row();
        auto* tid_item = table->item(row, 2);
        if (tid_item) {
            menu.addAction(QObject::tr("📋 Копировать Title ID"), [tid_item] {
                QGuiApplication::clipboard()->setText(tid_item->text());
            });
        }
        auto* name_item = table->item(row, 3);
        if (name_item) {
            menu.addAction(QObject::tr("📋 Копировать название"), [name_item] {
                QGuiApplication::clipboard()->setText(name_item->text());
            });
        }
        auto* desc_item = table->item(row, 4);
        if (desc_item) {
            menu.addAction(QObject::tr("📋 Копировать описание"), [desc_item] {
                QGuiApplication::clipboard()->setText(desc_item->text());
            });
        }
        menu.addSeparator();
        menu.addAction(QObject::tr("📋 Копировать строку целиком"), [table, row] {
            QStringList cell_texts;
            for (int col = 0; col < table->columnCount(); ++col) {
                auto* cell = table->item(row, col);
                if (cell) cell_texts << cell->text();
            }
            QGuiApplication::clipboard()->setText(cell_texts.join(QStringLiteral(" | ")));
        });
        menu.exec(table->viewport()->mapToGlobal(pos));
    });

    // Ctrl+C Shortcut for cell selection
    auto* copy_shortcut = new QShortcut(QKeySequence::Copy, table);
    connect(copy_shortcut, &QShortcut::activated, [table] {
        const auto selected_items = table->selectedItems();
        if (selected_items.isEmpty()) return;
        if (selected_items.size() == 1) {
            QGuiApplication::clipboard()->setText(selected_items.first()->text());
            return;
        }
        QMap<int, QMap<int, QString>> row_col_map;
        for (auto* item : selected_items) {
            row_col_map[item->row()][item->column()] = item->text();
        }
        QStringList rows_str;
        for (auto row_it = row_col_map.begin(); row_it != row_col_map.end(); ++row_it) {
            QStringList cols_str;
            for (auto col_it = row_it.value().begin(); col_it != row_it.value().end(); ++col_it) {
                cols_str << col_it.value();
            }
            rows_str << cols_str.join(QLatin1Char('\t'));
        }
        QGuiApplication::clipboard()->setText(rows_str.join(QLatin1Char('\n')));
    });

    main_layout->addWidget(table);

    auto* btn_layout = new QHBoxLayout();
    auto* copy_btn = new QPushButton(StormLang(
        QStringLiteral("📋 Копировать список"),
        QStringLiteral("📋 Copy List"),
        QStringLiteral("📋 Liste kopieren"),
        QStringLiteral("📋 Copier la liste"),
        QStringLiteral("📋 复制列表"),
        QStringLiteral("📋 リストをコピー")
    ), &dlg);
    copy_btn->setObjectName(QStringLiteral("CopyBtn"));
    connect(copy_btn, &QPushButton::clicked, [copy_lines, copy_btn] {
        QGuiApplication::clipboard()->setText(copy_lines.join(QLatin1Char('\n')));
        copy_btn->setText(StormLang(
            QStringLiteral("✅ Скопировано в буфер обмена!"),
            QStringLiteral("✅ Copied to clipboard!"),
            QStringLiteral("✅ In Zwischenablage kopiert!"),
            QStringLiteral("✅ Copié dans le presse-papiers !"),
            QStringLiteral("✅ 已复制到剪贴板！"),
            QStringLiteral("✅ クリップボードにコピーしました！")
        ));
        QTimer::singleShot(2000, [copy_btn] {
            if (copy_btn) copy_btn->setText(StormLang(
                QStringLiteral("📋 Копировать список"),
                QStringLiteral("📋 Copy List"),
                QStringLiteral("📋 Liste kopieren"),
                QStringLiteral("📋 Copier la liste"),
                QStringLiteral("📋 复制列表"),
                QStringLiteral("📋 リストをコピー")
            ));
        });
    });

    auto* manage_btn = new QPushButton(StormLang(
        QStringLiteral("⚙️ Управление дополнениями..."),
        QStringLiteral("⚙️ Manage Add-ons..."),
        QStringLiteral("⚙️ Add-ons verwalten..."),
        QStringLiteral("⚙️ Gérer les extensions..."),
        QStringLiteral("⚙️ 管理附加组件..."),
        QStringLiteral("⚙️ アドオンを管理...")
    ), &dlg);
    connect(manage_btn, &QPushButton::clicked, [this, title_id, &dlg] {
        dlg.accept();
        OpenPerGameConfiguration(title_id, m_current_addons_game_path);
    });

    auto* close_btn = new QPushButton(StormLang(
        QStringLiteral("Закрыть"),
        QStringLiteral("Close"),
        QStringLiteral("Schließen"),
        QStringLiteral("Fermer"),
        QStringLiteral("关闭"),
        QStringLiteral("閉じる")
    ), &dlg);
    connect(close_btn, &QPushButton::clicked, &dlg, &QDialog::accept);

    btn_layout->addWidget(copy_btn);
    btn_layout->addWidget(manage_btn);
    btn_layout->addStretch();
    btn_layout->addWidget(close_btn);

    main_layout->addLayout(btn_layout);

    dlg.exec();
}

bool MainWindow::ConfirmChangeGame() {
    if (QtCommon::emu_thread == nullptr)
        return true;

    // Use custom question to link controller navigation
    return question(
        this, QStringLiteral("STORM SWITCH"),
        StormLang(
            QStringLiteral("Вы действительно хотите остановить эмуляцию?\nВсе несохраненные данные будут потеряны."),
            QStringLiteral("Are you sure you want to stop emulation?\nAll unsaved data will be lost."),
            QStringLiteral("Möchten Sie die Emulation wirklich stoppen?\nAlle nicht gespeicherten Daten gehen verloren."),
            QStringLiteral("Voulez-vous vraiment arrêter l'émulation ?\nToutes les données non enregistrées seront perdues."),
            QStringLiteral("您确定要停止模拟吗？\n所有未保存的数据都将丢失。"),
            QStringLiteral("エミュレーションを本当に停止しますか？\n保存されていないデータはすべて失われます。")
        ),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
}

bool MainWindow::ConfirmForceLockedExit() {
    if (QtCommon::emu_thread == nullptr)
        return true;

    const auto text = StormLang(
        QStringLiteral("Запущенное приложение запросило запрет на выход из STORM SWITCH.\n\n"
                       "Вы действительно хотите принудительно завершить работу и выйти?"),
        QStringLiteral("The running application has requested not to exit STORM SWITCH.\n\n"
                       "Are you sure you want to force exit?"),
        QStringLiteral("Die laufende Anwendung hat angefordert, STORM SWITCH nicht zu beenden.\n\n"
                       "Möchten Sie das Beenden wirklich erzwingen?"),
        QStringLiteral("L'application en cours a demandé à ne pas quitter STORM SWITCH.\n\n"
                       "Voulez-vous vraiment forcer la fermeture ?"),
        QStringLiteral("正在运行的应用程序已请求禁止退出 STORM SWITCH。\n\n"
                       "您确定要强制退出吗？"),
        QStringLiteral("実行中のアプリケーションが STORM SWITCH の終了禁止を要求しました。\n\n"
                       "本当に強制終了しますか？")
    );

    return question(this, QStringLiteral("STORM SWITCH"), text);
}

void MainWindow::RequestGameExit() {
    if (!QtCommon::system->IsPoweredOn())
        return;

    QtCommon::system->SetExitRequested(true);
    QtCommon::system->GetAppletManager().RequestExit();
}

void MainWindow::filterBarSetChecked(bool state) {
    ui->action_Show_Filter_Bar->setChecked(state);
    emit(OnToggleFilterBar());
}

static void AdjustLinkColor() {
    QPalette new_pal(qApp->palette());

    if (UISettings::IsDarkTheme())
        new_pal.setColor(QPalette::Link, QColor(0, 190, 255, 255));
    else
        new_pal.setColor(QPalette::Link, QColor(0, 140, 200, 255));

    if (qApp->palette().color(QPalette::Link) != new_pal.color(QPalette::Link))
        qApp->setPalette(new_pal);
}

void MainWindow::UpdateUITheme() {
    const QString default_theme = QString::fromUtf8(UISettings::themes[size_t(UISettings::default_theme)].second);
    QString current_theme = QString::fromStdString(UISettings::values.theme);

    if (current_theme.isEmpty())
        current_theme = default_theme;

#ifdef _WIN32
    QIcon::setThemeName(current_theme);
    AdjustLinkColor();
#else
    QIcon::setThemeName(current_theme);
    QIcon::setThemeSearchPaths(QStringList(QStringLiteral(":/icons")));
    AdjustLinkColor();
#endif

    if (current_theme != default_theme) {
        QString theme_uri{QStringLiteral(":%1/style.qss").arg(current_theme)};
        QFile f(theme_uri);
        if (!f.open(QFile::ReadOnly | QFile::Text)) {
            LOG_ERROR(Frontend, "Unable to open style \"{}\", fallback to the default theme",
                      UISettings::values.theme);
            current_theme = default_theme;
        }
    }

    QString theme_uri{QStringLiteral(":%1/style.qss").arg(current_theme)};
    QFile f(theme_uri);
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream ts(&f);
        const QString stylesheet = ts.readAll();
        qApp->setStyleSheet(stylesheet);
        setStyleSheet(stylesheet);
    } else {
        LOG_ERROR(Frontend, "Unable to set style \"{}\", stylesheet file not found",
                  UISettings::values.theme);
        qApp->setStyleSheet({});
        setStyleSheet({});
    }

#ifdef _WIN32
    ApplyGlobalDarkTitlebar(UISettings::IsDarkTheme());
#endif
}

#include "storm_switch/common/storm_i18n_dict.h"

QString StormLookupMultiLang(const QString& en_key, const std::string& lang) {
    return StormI18n::LookupMultiLang(en_key, lang);
}

namespace {
class StormFallbackTranslator final : public QTranslator {
public:
    explicit StormFallbackTranslator(QTranslator* base_translator, QObject* parent = nullptr)
        : QTranslator(parent), base(base_translator) {}

    QString translate(const char* context, const char* sourceText,
                      const char* disambiguation = nullptr, int n = -1) const override {
        if (!sourceText || sourceText[0] == '\0') {
            return {};
        }
        std::string lang = UISettings::values.language.GetValue();
        if (lang.empty()) {
            lang = QLocale::system().name().toStdString();
        }
        const bool is_ru = lang.rfind("ru", 0) == 0;
        const bool is_en = lang.rfind("en", 0) == 0;
        const std::string_view src_sv(sourceText);

        if (is_ru) {
            return StormI18n::LookupEnToRu(src_sv);
        }

        if (const char* en_src = StormI18n::LookupRuToEn(src_sv)) {
            if (is_en) {
                return QString::fromUtf8(en_src);
            }
            const QString ml = StormI18n::LookupMultiLangSv(en_src, lang);
            if (!ml.isEmpty()) {
                return ml;
            }
            if (base && !base->isEmpty()) {
                static constexpr const char* kFallbackContexts[] = {
                    nullptr, "MainWindow", "ConfigurationShared", "ConfigureGeneral",
                    "ConfigureSystem", "ConfigureGraphics", "ConfigureGraphicsAdvanced",
                    "ConfigureAudio", "ConfigureCpu", "ConfigureUi", "ConfigureInputPlayer",
                    "QObject"
                };
                for (const char* ctx : kFallbackContexts) {
                    const char* use_ctx = ctx ? ctx : context;
                    if (!use_ctx) continue;
                    QString translated = base->translate(use_ctx, en_src, disambiguation, n);
                    if (!translated.isEmpty()) {
                        return translated;
                    }
                }
            }
            return QString::fromUtf8(en_src);
        }

        if (!is_en) {
            if (base && !base->isEmpty() && context) {
                QString direct = base->translate(context, sourceText, disambiguation, n);
                if (!direct.isEmpty()) {
                    return direct;
                }
            }
            const QString ml = StormI18n::LookupMultiLangSv(src_sv, lang);
            if (!ml.isEmpty()) {
                return ml;
            }
        }
        return {};
    }

private:
    QTranslator* base{};
};
} // namespace

void MainWindow::LoadTranslation() {
    static StormFallbackTranslator* fallback_translator = nullptr;
    if (!fallback_translator) {
        fallback_translator = new StormFallbackTranslator(&translator, qApp);
    }
    qApp->removeTranslator(fallback_translator);
    qApp->removeTranslator(&translator);

    bool loaded = false;
    const std::string lang_val = UISettings::values.language.GetValue();

    if (lang_val.empty()) {
        // If the selected language is empty, use system locale
        loaded = translator.load(QLocale(), {}, {}, QStringLiteral(":/languages/"));
    } else if (lang_val != "en") {
        const QString lang_qstr = QString::fromStdString(lang_val);
        loaded = translator.load(lang_qstr, QStringLiteral(":/languages/"));
        if (!loaded) {
            if (lang_val == "ru") {
                loaded = translator.load(QStringLiteral("ru_RU"), QStringLiteral(":/languages/"));
            } else if (lang_val == "ja") {
                loaded = translator.load(QStringLiteral("ja_JP"), QStringLiteral(":/languages/"));
            } else if (lang_val == "ko") {
                loaded = translator.load(QStringLiteral("ko_KR"), QStringLiteral(":/languages/"));
            } else if (lang_val == "zh") {
                loaded = translator.load(QStringLiteral("zh_CN"), QStringLiteral(":/languages/"));
            } else if (lang_val == "pt") {
                loaded = translator.load(QStringLiteral("pt_BR"), QStringLiteral(":/languages/"));
            } else if (lang_val == "tr") {
                loaded = translator.load(QStringLiteral("tr_TR"), QStringLiteral(":/languages/"));
            }
        }
        if (!loaded) {
            loaded = translator.load(QLocale(lang_qstr), {}, {}, QStringLiteral(":/languages/"));
        }
    }

    if (loaded) {
        qApp->installTranslator(&translator);
    }
    // Install fallback translator first/topmost so Russian source strings are translated in non-RU locales
    qApp->installTranslator(fallback_translator);
}

void MainWindow::OnLanguageChanged(const QString& locale) {
    qApp->removeTranslator(&translator);

    UISettings::values.language = locale.toStdString();
    LoadTranslation();

    if (game_size_actions) {
        QList<QAction*> actions = game_size_actions->actions();
        for (size_t i = 0; i < default_game_icon_sizes.size() && i < static_cast<size_t>(actions.size()); i++) {
            if (actions.at(static_cast<int>(i))) {
                actions.at(static_cast<int>(i))->setText(GetTranslatedGameIconSize(i));
            }
        }
    }

    ui->retranslateUi(this);

    // Explicitly retranslate menus and actions across all 6 core languages with StormLang
    if (ui->menuOpen_Eden_Folders) {
        ui->menuOpen_Eden_Folders->setTitle(StormLang(
            QStringLiteral("Открыть папки STORM SWITCH"),
            QStringLiteral("Open STORM SWITCH Folders"),
            QStringLiteral("STORM SWITCH-Ordner öffnen"),
            QStringLiteral("Ouvrir les dossiers STORM SWITCH"),
            QStringLiteral("打开 STORM SWITCH 文件夹"),
            QStringLiteral("STORM SWITCH フォルダーを開く")
        ));
    }
    if (ui->action_Root_Data_Folder) {
        ui->action_Root_Data_Folder->setText(StormLang(
            QStringLiteral("Корневая папка"),
            QStringLiteral("Root Folder"),
            QStringLiteral("Stammordner"),
            QStringLiteral("Dossier racine"),
            QStringLiteral("根文件夹"),
            QStringLiteral("ルートフォルダー")
        ));
    }
    if (ui->action_NAND_Folder) {
        ui->action_NAND_Folder->setText(StormLang(
            QStringLiteral("Каталог NAND"),
            QStringLiteral("NAND Directory"),
            QStringLiteral("NAND-Verzeichnis"),
            QStringLiteral("Répertoire NAND"),
            QStringLiteral("NAND 目录"),
            QStringLiteral("NAND ディレクトリ")
        ));
    }
    if (ui->action_SDMC_Folder) {
        ui->action_SDMC_Folder->setText(StormLang(
            QStringLiteral("Каталог SDMC"),
            QStringLiteral("SDMC Directory"),
            QStringLiteral("SDMC-Verzeichnis"),
            QStringLiteral("Répertoire SDMC"),
            QStringLiteral("SDMC 目录"),
            QStringLiteral("SDMC ディレクトリ")
        ));
    }
    if (ui->action_Mod_Folder) {
        ui->action_Mod_Folder->setText(StormLang(
            QStringLiteral("Папка модов"),
            QStringLiteral("Load (Mods) Directory"),
            QStringLiteral("Mod-Verzeichnis laden"),
            QStringLiteral("Répertoire des mods"),
            QStringLiteral("模组 (Mods) 目录"),
            QStringLiteral("Mod ディレクトリ")
        ));
    }
    if (ui->action_Log_Folder) {
        ui->action_Log_Folder->setText(StormLang(
            QStringLiteral("Папка журнала"),
            QStringLiteral("Log Directory"),
            QStringLiteral("Protokollverzeichnis"),
            QStringLiteral("Répertoire des journaux"),
            QStringLiteral("日志目录"),
            QStringLiteral("ログディレクトリ")
        ));
    }
    if (ui->action_Install_File_NAND) {
        ui->action_Install_File_NAND->setText(StormLang(
            QStringLiteral("Установить файлы в NAND..."),
            QStringLiteral("Install Files to NAND..."),
            QStringLiteral("Dateien in NAND installieren..."),
            QStringLiteral("Installer les fichiers dans la NAND..."),
            QStringLiteral("安装文件到 NAND..."),
            QStringLiteral("NAND にファイルをインストール...")
        ));
    }
    if (quick_save_action) {
        quick_save_action->setText(StormLang(
            QStringLiteral("Быстрое сохранение (Слот 1)"),
            QStringLiteral("Quick Save (Slot 1)"),
            QStringLiteral("Schnellspeichern (Slot 1)"),
            QStringLiteral("Sauvegarde rapide (Emplacement 1)"),
            QStringLiteral("快速保存 (槽位 1)"),
            QStringLiteral("クイックセーブ (スロット 1)")
        ));
    }
    if (quick_load_action) {
        quick_load_action->setText(StormLang(
            QStringLiteral("Быстрая загрузка (Слот 1)"),
            QStringLiteral("Quick Load (Slot 1)"),
            QStringLiteral("Schnellladen (Slot 1)"),
            QStringLiteral("Chargement rapide (Emplacement 1)"),
            QStringLiteral("快速读取 (槽位 1)"),
            QStringLiteral("クイックロード (スロット 1)")
        ));
    }
    if (save_state_slots_menu) {
        save_state_slots_menu->setTitle(StormLang(
            QStringLiteral("Сохранить состояние в слот"),
            QStringLiteral("Save State to Slot"),
            QStringLiteral("Status in Slot speichern"),
            QStringLiteral("Sauvegarder l'état dans l'emplacement"),
            QStringLiteral("保存状态到槽位"),
            QStringLiteral("スロットにステートを保存")
        ));
    }
    if (load_state_slots_menu) {
        load_state_slots_menu->setTitle(StormLang(
            QStringLiteral("Загрузить состояние из слота"),
            QStringLiteral("Load State from Slot"),
            QStringLiteral("Status aus Slot laden"),
            QStringLiteral("Charger l'état depuis l'emplacement"),
            QStringLiteral("从槽位读取状态"),
            QStringLiteral("スロットからステートをロード")
        ));
    }
    if (save_state_manager_action) {
        save_state_manager_action->setText(StormLang(
            QStringLiteral("Менеджер быстрых сохранений..."),
            QStringLiteral("Save State Manager..."),
            QStringLiteral("Schnellspeicher-Manager..."),
            QStringLiteral("Gestionnaire de sauvegardes rapides..."),
            QStringLiteral("快速保存管理器..."),
            QStringLiteral("クイックセーブマネージャー...")
        ));
    }
    RefreshSaveStateMenus();
    if (ui->action_Amiibo_Online_Database) {
        ui->action_Amiibo_Online_Database->setText(StormLang(
            QStringLiteral("Онлайн-база Amiibo..."),
            QStringLiteral("Online Amiibo Database..."),
            QStringLiteral("Online-Amiibo-Datenbank..."),
            QStringLiteral("Base de données Amiibo en ligne..."),
            QStringLiteral("在线 Amiibo 数据库..."),
            QStringLiteral("オンライン Amiibo データベース...")
        ));
    }
    if (ui->action_Nand_Manager) {
        ui->action_Nand_Manager->setText(StormLang(
            QStringLiteral("Менеджер профилей NAND..."),
            QStringLiteral("NAND Profiles Manager..."),
            QStringLiteral("NAND-Profilmanager..."),
            QStringLiteral("Gestionnaire de profils NAND..."),
            QStringLiteral("NAND 配置文件管理器..."),
            QStringLiteral("NANDプロファイルマネージャー...")
        ));
    }
    if (ui->action_Show_Performance_Overlay) {
        ui->action_Show_Performance_Overlay->setText(StormLang(
            QStringLiteral("Отображать оверлей производительности"),
            QStringLiteral("Show Performance Overlay"),
            QStringLiteral("Leistungs-Overlay anzeigen"),
            QStringLiteral("Afficher l'overlay de performances"),
            QStringLiteral("显示性能悬浮窗"),
            QStringLiteral("パフォーマンスオーバーレイを表示")
        ));
    }
    if (ui->action_Show_Game_Name) {
        ui->action_Show_Game_Name->setText(StormLang(
            QStringLiteral("Отображать название игры"),
            QStringLiteral("Show Game Title"),
            QStringLiteral("Spieltitel anzeigen"),
            QStringLiteral("Afficher le titre du jeu"),
            QStringLiteral("显示游戏名称"),
            QStringLiteral("ゲームタイトルを表示")
        ));
    }
    if (ui->menu_Game_List_Mode) {
        ui->menu_Game_List_Mode->setTitle(StormLang(
            QStringLiteral("Режим списка игр"),
            QStringLiteral("Game List Mode"),
            QStringLiteral("Spielelisten-Modus"),
            QStringLiteral("Mode liste de jeux"),
            QStringLiteral("游戏列表模式"),
            QStringLiteral("ゲームリスト表示モード")
        ));
    }
    if (ui->action_Tree_View) {
        ui->action_Tree_View->setText(StormLang(
            QStringLiteral("Дерево"),
            QStringLiteral("Tree"),
            QStringLiteral("Baumansicht"),
            QStringLiteral("Arborescence"),
            QStringLiteral("树状视图"),
            QStringLiteral("ツリー表示")
        ));
    }
    if (ui->action_Grid_View) {
        ui->action_Grid_View->setText(StormLang(
            QStringLiteral("Сетка"),
            QStringLiteral("Grid"),
            QStringLiteral("Rasteransicht"),
            QStringLiteral("Grille"),
            QStringLiteral("网格视图"),
            QStringLiteral("グリッド表示")
        ));
    }
    if (ui->action_Carousel_View) {
        ui->action_Carousel_View->setText(StormLang(
            QStringLiteral("Карусель"),
            QStringLiteral("Carousel"),
            QStringLiteral("Karussell"),
            QStringLiteral("Carrousel"),
            QStringLiteral("轮播视图"),
            QStringLiteral("カルーセル表示")
        ));
    }
    if (ui->menuGame_Icon_Size) {
        ui->menuGame_Icon_Size->setTitle(StormLang(
            QStringLiteral("Размер иконок игр"),
            QStringLiteral("Game Icon Size"),
            QStringLiteral("Symbolgröße der Spiele"),
            QStringLiteral("Taille des icônes de jeux"),
            QStringLiteral("游戏图标大小"),
            QStringLiteral("ゲームアイコンサイズ")
        ));
    }
    if (ui->menuInstall_Keys) {
        ui->menuInstall_Keys->setTitle(StormLang(
            QStringLiteral("Установить ключи дешифрования"),
            QStringLiteral("Install Decryption Keys"),
            QStringLiteral("Entschlüsselungsschlüssel installieren"),
            QStringLiteral("Installer les clés de déchiffrement"),
            QStringLiteral("安装解密密钥"),
            QStringLiteral("復号キーをインストール")
        ));
    }
    if (ui->action_Install_Keys_Online) {
        ui->action_Install_Keys_Online->setText(StormLang(
            QStringLiteral("Онлайн-установка ключей из сети..."),
            QStringLiteral("Install Keys Online from Network..."),
            QStringLiteral("Schlüssel online aus dem Netzwerk installieren..."),
            QStringLiteral("Installer les clés en ligne depuis le réseau..."),
            QStringLiteral("从网络在线安装密钥..."),
            QStringLiteral("ネットワークからキーをオンラインインストール...")
        ));
    }
    if (ui->action_Install_Keys_From_ZIP) {
        ui->action_Install_Keys_From_ZIP->setText(StormLang(
            QStringLiteral("Из архива (ZIP)..."),
            QStringLiteral("From Archive (ZIP)..."),
            QStringLiteral("Aus Archiv (ZIP)..."),
            QStringLiteral("Depuis une archive (ZIP)..."),
            QStringLiteral("从压缩包 (ZIP)..."),
            QStringLiteral("アーカイブから (ZIP)...")
        ));
    }
    if (ui->action_Install_Keys) {
        ui->action_Install_Keys->setText(StormLang(
            QStringLiteral("Из файла (prod.keys)..."),
            QStringLiteral("From File (prod.keys)..."),
            QStringLiteral("Aus Datei (prod.keys)..."),
            QStringLiteral("Depuis un fichier (prod.keys)..."),
            QStringLiteral("从文件 (prod.keys)..."),
            QStringLiteral("ファイルから (prod.keys)...")
        ));
    }
    if (ui->menuInstall_Firmware) {
        ui->menuInstall_Firmware->setTitle(StormLang(
            QStringLiteral("Установить прошивку"),
            QStringLiteral("Install Firmware"),
            QStringLiteral("Firmware installieren"),
            QStringLiteral("Installer le firmware"),
            QStringLiteral("安装固件"),
            QStringLiteral("ファームウェアをインストール")
        ));
    }
    if (ui->action_Firmware_Online) {
        ui->action_Firmware_Online->setText(StormLang(
            QStringLiteral("Онлайн-установка прошивки из сети..."),
            QStringLiteral("Install Firmware Online from Network..."),
            QStringLiteral("Firmware online aus dem Netzwerk installieren..."),
            QStringLiteral("Installer le firmware en ligne depuis le réseau..."),
            QStringLiteral("从网络在线安装固件..."),
            QStringLiteral("ネットワークからファームウェアをオンラインインストール...")
        ));
    }
    if (ui->action_Firmware_From_Folder) {
        ui->action_Firmware_From_Folder->setText(StormLang(
            QStringLiteral("Из папки..."),
            QStringLiteral("From Folder..."),
            QStringLiteral("Aus Ordner..."),
            QStringLiteral("Depuis un dossier..."),
            QStringLiteral("从文件夹..."),
            QStringLiteral("フォルダーから...")
        ));
    }
    if (ui->action_Firmware_From_ZIP) {
        ui->action_Firmware_From_ZIP->setText(StormLang(
            QStringLiteral("Из архива (ZIP)..."),
            QStringLiteral("From Archive (ZIP)..."),
            QStringLiteral("Aus Archiv (ZIP)..."),
            QStringLiteral("Depuis une archive (ZIP)..."),
            QStringLiteral("从压缩包 (ZIP)..."),
            QStringLiteral("アーカイブから (ZIP)...")
        ));
    }
    if (ui->action_Verify_installed_contents) {
        ui->action_Verify_installed_contents->setText(StormLang(
            QStringLiteral("Проверить установленный контент"),
            QStringLiteral("Verify Installed Contents"),
            QStringLiteral("Installierte Inhalte überprüfen"),
            QStringLiteral("Vérifier le contenu installé"),
            QStringLiteral("验证已安装内容"),
            QStringLiteral("インストール済みコンテンツを検証")
        ));
    }
    if (ui->action_Data_Manager) {
        ui->action_Data_Manager->setText(StormLang(
            QStringLiteral("Менеджер данных..."),
            QStringLiteral("Data Manager..."),
            QStringLiteral("Datenmanager..."),
            QStringLiteral("Gestionnaire de données..."),
            QStringLiteral("数据管理器..."),
            QStringLiteral("データマネージャー...")
        ));
    }
    if (ui->action_Translate_Screen) {
        ui->action_Translate_Screen->setText(StormLang(
            QStringLiteral("Авто-переводчик (OCR и озвучка)..."),
            QStringLiteral("Auto Translator (OCR and Voice)..."),
            QStringLiteral("Automatischer Übersetzer (OCR und Sprachausgabe)..."),
            QStringLiteral("Traducteur automatique (OCR et Voix)..."),
            QStringLiteral("自动翻译器 (OCR 和 语音)..."),
            QStringLiteral("自動翻訳（OCRと音声読み上げ）...")
        ));
    }
    if (ui->menuTAS) {
        ui->menuTAS->setTitle(StormLang(
            QStringLiteral("TAS"),
            QStringLiteral("TAS"),
            QStringLiteral("TAS"),
            QStringLiteral("TAS"),
            QStringLiteral("TAS"),
            QStringLiteral("TAS")
        ));
    }
    if (ui->action_TAS_Start) {
        ui->action_TAS_Start->setText(StormLang(
            QStringLiteral("Запуск / Пауза TAS"),
            QStringLiteral("Start / Pause TAS"),
            QStringLiteral("TAS starten / pausieren"),
            QStringLiteral("Démarrer / Mettre en pause TAS"),
            QStringLiteral("启动 / 暂停 TAS"),
            QStringLiteral("TAS を開始 / 一時停止")
        ));
    }
    if (ui->action_TAS_Record) {
        ui->action_TAS_Record->setText(StormLang(
            QStringLiteral("Запись скрипта TAS"),
            QStringLiteral("Record TAS"),
            QStringLiteral("TAS aufzeichnen"),
            QStringLiteral("Enregistrer TAS"),
            QStringLiteral("录制 TAS"),
            QStringLiteral("TAS を録画")
        ));
    }
    if (ui->action_TAS_Reset) {
        ui->action_TAS_Reset->setText(StormLang(
            QStringLiteral("Сброс TAS"),
            QStringLiteral("Reset TAS"),
            QStringLiteral("TAS zurücksetzen"),
            QStringLiteral("Réinitialiser TAS"),
            QStringLiteral("重置 TAS"),
            QStringLiteral("TAS をリセット")
        ));
    }
    if (ui->action_Configure_Tas) {
        ui->action_Configure_Tas->setText(StormLang(
            QStringLiteral("Настройка TAS..."),
            QStringLiteral("Configure TAS..."),
            QStringLiteral("TAS konfigurieren..."),
            QStringLiteral("Configurer TAS..."),
            QStringLiteral("配置 TAS..."),
            QStringLiteral("TAS を設定...")
        ));
    }
    if (ui->action_Mod_Manager) {
        ui->action_Mod_Manager->setText(StormLang(
            QStringLiteral("Менеджер модов..."),
            QStringLiteral("Mod Manager..."),
            QStringLiteral("Mod-Manager..."),
            QStringLiteral("Gestionnaire de mods..."),
            QStringLiteral("模组管理器..."),
            QStringLiteral("Mod マネージャー...")
        ));
    }
    if (ui->action_Cheats) {
        ui->action_Cheats->setText(StormLang(
            QStringLiteral("Чит-коды..."),
            QStringLiteral("Cheats..."),
            QStringLiteral("Cheats..."),
            QStringLiteral("Codes de triche..."),
            QStringLiteral("金手指 / 作弊码..."),
            QStringLiteral("チートコード...")
        ));
    }
    if (ui->action_Check_Updates) {
        ui->action_Check_Updates->setText(StormLang(
            QStringLiteral("Проверка обновлений..."),
            QStringLiteral("Check for Updates..."),
            QStringLiteral("Nach Updates suchen..."),
            QStringLiteral("Rechercher des mises à jour..."),
            QStringLiteral("检查更新..."),
            QStringLiteral("アップデートを確認...")
        ));
    }
    if (ui->action_About) {
        ui->action_About->setText(StormLang(
            QStringLiteral("О STORM SWITCH"),
            QStringLiteral("About STORM SWITCH"),
            QStringLiteral("Über STORM SWITCH"),
            QStringLiteral("À propos de STORM SWITCH"),
            QStringLiteral("关于 STORM SWITCH"),
            QStringLiteral("STORM SWITCH について")
        ));
    }
    if (ui->action_Eden_Dependencies) {
        ui->action_Eden_Dependencies->setText(StormLang(
            QStringLiteral("Зависимости STORM SWITCH"),
            QStringLiteral("STORM SWITCH Dependencies"),
            QStringLiteral("STORM SWITCH-Abhängigkeiten"),
            QStringLiteral("Dépendances de STORM SWITCH"),
            QStringLiteral("STORM SWITCH 依赖项"),
            QStringLiteral("STORM SWITCH 依存関係")
        ));
    }

    if (storm_games_world_action) {
        storm_games_world_action->setText(StormLang(
            QStringLiteral("Каталог и менеджер игр STORM GAMES WORLD..."),
            QStringLiteral("STORM GAMES WORLD Game Catalog and Manager..."),
            QStringLiteral("STORM GAMES WORLD Spielekatalog und Manager..."),
            QStringLiteral("Catalogue et gestionnaire de jeux STORM GAMES WORLD..."),
            QStringLiteral("STORM GAMES WORLD 游戏目录与管理器..."),
            QStringLiteral("STORM GAMES WORLD ゲームカタログとマネージャー...")
        ));
    }
    if (storm_save_sync_action) {
        storm_save_sync_action->setText(StormLang(
            QStringLiteral("Синхронизация сохранений (STORM SAVE SYNC)..."),
            QStringLiteral("Save Synchronization (STORM SAVE SYNC)..."),
            QStringLiteral("Spielstand-Synchronisierung (STORM SAVE SYNC)..."),
            QStringLiteral("Synchronisation des sauvegardes (STORM SAVE SYNC)..."),
            QStringLiteral("存档同步 (STORM SAVE SYNC)..."),
            QStringLiteral("セーブデータ同期 (STORM SAVE SYNC)...")
        ));
    }
    if (log_viewer_action) {
        log_viewer_action->setText(StormLang(
            QStringLiteral("Журнал работы (Логи)..."),
            QStringLiteral("Log Viewer (Logs)..."),
            QStringLiteral("Ereignisprotokoll (Logs)..."),
            QStringLiteral("Journal des opérations (Logs)..."),
            QStringLiteral("运行日志 (Logs)..."),
            QStringLiteral("動作ログ (Logs)...")
        ));
    }
    if (reset_gamefix_action) {
        reset_gamefix_action->setText(StormLang(
            QStringLiteral("Сбросить скрытые диалоги авто-исправлений..."),
            QStringLiteral("Reset Hidden Auto-Fix Dialogs..."),
            QStringLiteral("Ausgeblendete Auto-Fix-Dialoge zurücksetzen..."),
            QStringLiteral("Réinitialiser les dialogues d'auto-correction masqués..."),
            QStringLiteral("重置隐藏的自动修复对话框..."),
            QStringLiteral("非表示の自動修正ダイアログをリセット...")
        ));
    }

    // Retranslate footer status bar group headers
    if (m_status_group_headers.size() >= 8) {
        m_status_group_headers[0]->setText(StormLang(QStringLiteral("УПРАВЛЕНИЕ"), QStringLiteral("CONTROLS"), QStringLiteral("STEUERUNG"), QStringLiteral("COMMANDES"), QStringLiteral("控制"), QStringLiteral("操作")));
        m_status_group_headers[1]->setText(StormLang(QStringLiteral("ДОПОЛНЕНИЯ"), QStringLiteral("ADD-ONS"), QStringLiteral("ADD-ONS"), QStringLiteral("EXTENSIONS"), QStringLiteral("附加组件"), QStringLiteral("アドオン")));
        m_status_group_headers[2]->setText(StormLang(QStringLiteral("РЕНДЕР"), QStringLiteral("RENDER"), QStringLiteral("RENDER"), QStringLiteral("RENDU"), QStringLiteral("渲染"), QStringLiteral("レンダー")));
        m_status_group_headers[3]->setText(StormLang(QStringLiteral("ГРАФИКА"), QStringLiteral("GRAPHICS"), QStringLiteral("GRAFIK"), QStringLiteral("GRAPHISMES"), QStringLiteral("图形"), QStringLiteral("グラフィックス")));
        m_status_group_headers[4]->setText(QStringLiteral("ASTC"));
        m_status_group_headers[5]->setText(StormLang(QStringLiteral("РЕЖИМ"), QStringLiteral("MODE"), QStringLiteral("MODUS"), QStringLiteral("MODE"), QStringLiteral("模式"), QStringLiteral("モード")));
        m_status_group_headers[6]->setText(StormLang(QStringLiteral("СИСТЕМА"), QStringLiteral("SYSTEM"), QStringLiteral("SYSTEM"), QStringLiteral("SYSTÈME"), QStringLiteral("系统"), QStringLiteral("システム")));
        m_status_group_headers[7]->setText(StormLang(QStringLiteral("СЕТЬ"), QStringLiteral("NETWORK"), QStringLiteral("NETZWERK"), QStringLiteral("RÉSEAU"), QStringLiteral("网络"), QStringLiteral("ネットワーク")));

        for (size_t i = 0; i < m_status_group_headers.size() && i < 8; ++i) {
            const QString title = m_status_group_headers[i]->text();
            m_status_group_headers[i]->setToolTip(StormLang(
                QStringLiteral("Нажмите для быстрого меню раздела «%1»").arg(title),
                QStringLiteral("Click for quick menu of \"%1\" section").arg(title),
                QStringLiteral("Klicken für Schnellmenü des Abschnitts \"%1\"").arg(title),
                QStringLiteral("Cliquer pour le menu rapide de la section \"%1\"").arg(title),
                QStringLiteral("点击打开“%1”分区的快捷菜单").arg(title),
                QStringLiteral("クリックして「%1」セクションのクイックメニューを開く").arg(title)
            ));
        }
    }

    if (footer_customize_button) {
        footer_customize_button->setToolTip(StormLang(
            QStringLiteral("Настройка отображения разделов и кнопок подвала"),
            QStringLiteral("Customize footer sections and buttons display"),
            QStringLiteral("Fußzeilenabschnitte und Schaltflächen anpassen"),
            QStringLiteral("Personnaliser l'affichage des sections et boutons du pied de page"),
            QStringLiteral("自定义底部栏分区与按钮显示"),
            QStringLiteral("フッターのセクションとボタンの表示をカスタマイズ")
        ));
    }

    UpdateStatusButtons();
    SetFirmwareVersion();

    if (multiplayer_state) {
        multiplayer_state->retranslateUi();
    }
    UpdateSystemTrayStrings();
    UpdateWindowTitle();
}

void MainWindow::SetDiscordEnabled([[maybe_unused]] bool state) {
#ifdef USE_DISCORD_PRESENCE
    if (state)
        discord_rpc = std::make_unique<DiscordRPC::DiscordImpl>(*QtCommon::system);
    else
        discord_rpc = std::make_unique<DiscordRPC::NullImpl>();
#else
    discord_rpc = std::make_unique<DiscordRPC::NullImpl>();
#endif
    discord_rpc->Update();
}

void MainWindow::SetGamemodeEnabled(bool state) {
    if (emulation_running) {
        if (state)
            Common::FeralGamemode::Start();
        else
            Common::FeralGamemode::Stop();
    }
}

void MainWindow::changeEvent(QEvent* event) {
#ifdef __unix__
    // PaletteChange event appears to only reach so far into the GUI, explicitly asking to
    // UpdateUITheme is a decent work around
    if (event->type() == QEvent::PaletteChange) {
        const QPalette test_palette(qApp->palette());
        const QString current_theme = QString::fromStdString(UISettings::values.theme);
        // Keeping eye on QPalette::Window to avoid looping. QPalette::Text might be useful too
        static QColor last_window_color;
        const QColor window_color = test_palette.color(QPalette::Active, QPalette::Window);
        if (last_window_color != window_color && (current_theme == QStringLiteral("default") ||
                                                  current_theme == QStringLiteral("colorful"))) {
            UpdateUITheme();
        }
        last_window_color = window_color;
    }
#endif // __unix__
    QWidget::changeEvent(event);
}

Service::AM::FrontendAppletParameters MainWindow::ApplicationAppletParameters() {
    return Service::AM::FrontendAppletParameters{
        .applet_id = Service::AM::AppletId::Application,
        .applet_type = Service::AM::AppletType::Application,
    };
}

Service::AM::FrontendAppletParameters MainWindow::LibraryAppletParameters(
    u64 program_id, Service::AM::AppletId applet_id) {
    return Service::AM::FrontendAppletParameters{
        .program_id = program_id,
        .applet_id = applet_id,
        .applet_type = Service::AM::AppletType::LibraryApplet,
    };
}

void VolumeButton::wheelEvent(QWheelEvent* event) {

    int num_degrees = event->angleDelta().y() / 8;
    int num_steps = (num_degrees / 15) * scroll_multiplier;
    // Stated in QT docs: Most mouse types work in steps of 15 degrees, in which case the delta
    // value is a multiple of 120; i.e., 120 units * 1/8 = 15 degrees.

    if (num_steps > 0) {
        Settings::values.volume.SetValue(
            (std::min)(200, Settings::values.volume.GetValue() + num_steps));
    } else {
        Settings::values.volume.SetValue(
            (std::max)(0, Settings::values.volume.GetValue() + num_steps));
    }

    scroll_multiplier = (std::min)(MaxMultiplier, scroll_multiplier * 2);
    scroll_timer.start(100); // reset the multiplier if no scroll event occurs within 100 ms

    emit VolumeChanged();
    event->accept();
}

void VolumeButton::ResetMultiplier() {
    scroll_multiplier = 1;
}

#ifdef main
#undef main
#endif

#if !defined(QT_STATICPLUGIN) || defined(__APPLE__)
#define VMA_IMPLEMENTATION
#include "video_core/vulkan_common/vma.h"
#endif

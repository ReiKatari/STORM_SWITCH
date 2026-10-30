// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include "startup_checks.h"

#if YUZU_ROOM
#include <cstring>
#include "dedicated_room/yuzu_room.h"
#endif
#ifdef __unix__
#include "qt_common/gui_settings.h"
#endif

#ifndef _WIN32
#include <sys/resource.h>
#endif

#if defined(__APPLE__)
#include <climits>
#include <cstdlib>
#include <cstring>
#endif

#include "core/save_state.h"
#include "main_window.h"
#include "qt_common/titledb.h"

#ifdef _WIN32
#include <windows.h>
#include <processthreadsapi.h>
#include <dbghelp.h>
#include <QScreen>
#include "common/logging.h"

#include <atomic>

static LONG WINAPI StormCrashHandler(EXCEPTION_POINTERS* exception_info) {
    if (!exception_info || !exception_info->ExceptionRecord) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    const DWORD code = exception_info->ExceptionRecord->ExceptionCode;
    if (code != EXCEPTION_ACCESS_VIOLATION and
        code != EXCEPTION_ILLEGAL_INSTRUCTION and
        code != EXCEPTION_ARRAY_BOUNDS_EXCEEDED and
        code != EXCEPTION_DATATYPE_MISALIGNMENT and
        code != EXCEPTION_STACK_OVERFLOW and
        code != 0xE06D7363) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    static std::atomic<bool> s_dumped{false};
    if (s_dumped.exchange(true)) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    Common::Log::Stop();

    HMODULE dbghelp = LoadLibraryA("dbghelp.dll");
    if (dbghelp) {
        using MiniDumpWriteDumpFn = BOOL(WINAPI*)(
            HANDLE, DWORD, HANDLE, MINIDUMP_TYPE,
            PMINIDUMP_EXCEPTION_INFORMATION,
            PMINIDUMP_USER_STREAM_INFORMATION,
            PMINIDUMP_CALLBACK_INFORMATION);
        auto pfn = reinterpret_cast<MiniDumpWriteDumpFn>(
            GetProcAddress(dbghelp, "MiniDumpWriteDump"));
        if (pfn) {
            SYSTEMTIME st;
            GetLocalTime(&st);
            char dump_path[MAX_PATH];
            snprintf(dump_path, sizeof(dump_path),
                     "user\\crash_dumps\\crash_%04d%02d%02d_%02d%02d%02d.dmp",
                     st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
            CreateDirectoryA("user", nullptr);
            CreateDirectoryA("user\\crash_dumps", nullptr);
            HANDLE file = CreateFileA(dump_path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file != INVALID_HANDLE_VALUE) {
                MINIDUMP_EXCEPTION_INFORMATION mei{};
                mei.ThreadId = GetCurrentThreadId();
                mei.ExceptionPointers = exception_info;
                mei.ClientPointers = FALSE;
                pfn(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpNormal, &mei, nullptr, nullptr);
                CloseHandle(file);
            }
        }
    }

    CreateDirectoryA("user", nullptr);
    CreateDirectoryA("user\\crash_dumps", nullptr);
    FILE* f = fopen("user\\crash_dumps\\crash_report.txt", "w");
    if (f) {
        if (exception_info && exception_info->ExceptionRecord) {
            fprintf(f, "Exception Code: 0x%08X\n", exception_info->ExceptionRecord->ExceptionCode);
            fprintf(f, "Exception Address: %p\n", exception_info->ExceptionRecord->ExceptionAddress);
            HMODULE mod = nullptr;
            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                   (LPCSTR)exception_info->ExceptionRecord->ExceptionAddress, &mod)) {
                char mod_name[MAX_PATH];
                GetModuleFileNameA(mod, mod_name, sizeof(mod_name));
                fprintf(f, "Faulting Module: %s\n", mod_name);
                fprintf(f, "Module Base: %p, Offset: 0x%llx\n", mod, (uintptr_t)exception_info->ExceptionRecord->ExceptionAddress - (uintptr_t)mod);
            }
            if (exception_info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION and
                exception_info->ExceptionRecord->NumberParameters >= 2) {
                fprintf(f, "Access Type: %s\n", exception_info->ExceptionRecord->ExceptionInformation[0] == 0 ? "Read" : (exception_info->ExceptionRecord->ExceptionInformation[0] == 1 ? "Write" : "Execute"));
                fprintf(f, "Faulting Address: 0x%llx\n", (unsigned long long)exception_info->ExceptionRecord->ExceptionInformation[1]);
            }
            if (exception_info->ExceptionRecord->ExceptionCode == 0xE06D7363 and
                exception_info->ExceptionRecord->NumberParameters >= 2) {
                __try {
                    auto* exc = reinterpret_cast<std::exception*>(exception_info->ExceptionRecord->ExceptionInformation[1]);
                    if (exc) {
                        fprintf(f, "C++ Exception: %s\n", exc->what());
                    }
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    fprintf(f, "C++ Exception object could not be read\n");
                }
            }
        }
        fclose(f);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

static void OverrideWindowsFont() {
    // Qt5 chooses these fonts on Windows and they have fairly ugly alphanumeric/cyrillic characters
    // Asking to use "MS Shell Dlg 2" gives better other chars while leaving the Chinese Characters.
    const QString startup_font = QApplication::font().family();
    const QStringList ugly_fonts = {QStringLiteral("SimSun"), QStringLiteral("PMingLiU")};
    if (ugly_fonts.contains(startup_font)) {
        QApplication::setFont(QFont(QStringLiteral("MS Shell Dlg 2"), 9, QFont::Normal));
    }
}
#endif

static Qt::HighDpiScaleFactorRoundingPolicy GetHighDpiRoundingPolicy() {
#ifdef _WIN32
    // Query primary screen dimensions directly via Win32 without constructing a temporary
    // QApplication (which corrupts Qt6 global statics when destroyed before the main QApplication).
    const int real_width = GetSystemMetrics(SM_CXSCREEN);
    const int real_height = GetSystemMetrics(SM_CYSCREEN);
    if (real_width <= 0 || real_height <= 0) {
        return Qt::HighDpiScaleFactorRoundingPolicy::PassThrough;
    }
    constexpr double minimum_width = 1350.0;
    constexpr double minimum_height = 900.0;
    const double width_ratio = (std::max)(1.0, static_cast<double>(real_width) / minimum_width);
    const double height_ratio = (std::max)(1.0, static_cast<double>(real_height) / minimum_height);
    const double max_ratio = std::trunc((std::min)(width_ratio, height_ratio));
    return max_ratio >= 2.0 ? Qt::HighDpiScaleFactorRoundingPolicy::Round
                            : Qt::HighDpiScaleFactorRoundingPolicy::Floor;
#else
    return Qt::HighDpiScaleFactorRoundingPolicy::PassThrough;
#endif
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Issue #22: Disable Windows 11 EcoQoS (Power Throttling) for optimal APU/GPU boost on handhelds (ROG Ally)
#ifndef PROCESS_POWER_THROTTLING_CURRENT_VERSION
#define PROCESS_POWER_THROTTLING_CURRENT_VERSION 1
#endif
#ifndef PROCESS_POWER_THROTTLING_EXECUTION_SPEED
#define PROCESS_POWER_THROTTLING_EXECUTION_SPEED 0x1
#endif
    PROCESS_POWER_THROTTLING_STATE power_throttling{};
    power_throttling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    power_throttling.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
    power_throttling.StateMask = 0;
    SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &power_throttling,
                          sizeof(power_throttling));

    char exe_path[MAX_PATH];
    if (GetModuleFileNameA(nullptr, exe_path, sizeof(exe_path)) > 0) {
        char* last_slash = strrchr(exe_path, '\\');
        if (last_slash != nullptr) {
            *last_slash = '\0';
            SetCurrentDirectoryA(exe_path);
        }
    }
    SetUnhandledExceptionFilter(StormCrashHandler);

    // Law 22: UIPI bypass for Drag-and-Drop when running elevated as Administrator
    typedef BOOL(WINAPI* PFN_ChangeWindowMessageFilter)(UINT, DWORD);
    if (HMODULE user32 = GetModuleHandleW(L"user32.dll")) {
        auto pfnFilter = reinterpret_cast<PFN_ChangeWindowMessageFilter>(
            GetProcAddress(user32, "ChangeWindowMessageFilter"));
        if (pfnFilter) {
            pfnFilter(0x0233 /* WM_DROPFILES */, 1 /* MSGFLT_ADD */);
            pfnFilter(0x004A /* WM_COPYDATA */, 1 /* MSGFLT_ADD */);
            pfnFilter(0x0049 /* WM_COPYGLOBALDATA */, 1 /* MSGFLT_ADD */);
        }
    }
#endif
    std::set_terminate([]() {
        std::string msg = "Unknown terminate reason";
        if (auto e = std::current_exception()) {
            try {
                std::rethrow_exception(e);
            } catch (const std::exception bitand ex) {
                msg = ex.what();
            } catch (...) {
                msg = "Non-std exception";
            }
        }
        CreateDirectoryA("user", nullptr);
        CreateDirectoryA("user\\crash_dumps", nullptr);
        FILE* f = fopen("user\\crash_dumps\\crash_report.txt", "w");
        if (f) {
            fprintf(f, "std::terminate called on thread %lu: %s\n", GetCurrentThreadId(), msg.c_str());
            fflush(f);
            fclose(f);
        }
        abort();
    });
#if YUZU_ROOM
    bool launch_room = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--room") == 0) {
            launch_room = true;
        }
    }

    if (launch_room) {
        LaunchRoom(argc, argv, true);
        return 0;
    }
#endif

    bool has_broken_vulkan = false;
    bool is_child = false;
    if (CheckEnvVars(&is_child)) {
        return 0;
    }

#ifdef _WIN32
    HANDLE hSingleInstanceMutex = CreateMutexW(nullptr, TRUE, L"Global\\STORM_SWITCH_SingleInstanceMutex");
    if (hSingleInstanceMutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        AllowSetForegroundWindow(ASFW_ANY);
        HANDLE hRestoreEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, L"Global\\STORM_SWITCH_RestoreEvent");
        if (hRestoreEvent) {
            SetEvent(hRestoreEvent);
            CloseHandle(hRestoreEvent);
        }
        HWND existingWnd = nullptr;
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            wchar_t className[256];
            if (GetClassNameW(hwnd, className, 256) > 0) {
                if (wcsstr(className, L"Qt") == nullptr && wcsstr(className, L"QWindow") == nullptr) {
                    return TRUE;
                }
            }
            wchar_t title[256];
            if (GetWindowTextW(hwnd, title, 256) > 0) {
                if (wcsstr(title, L"STORM SWITCH") != nullptr) {
                    *reinterpret_cast<HWND*>(lParam) = hwnd;
                    return FALSE;
                }
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&existingWnd));

        if (existingWnd) {
            if (IsIconic(existingWnd)) {
                ShowWindow(existingWnd, SW_RESTORE);
            } else {
                ShowWindow(existingWnd, SW_SHOW);
            }
            SetForegroundWindow(existingWnd);
        }
        if (hSingleInstanceMutex) {
            CloseHandle(hSingleInstanceMutex);
        }
        return 0;
    }
#endif

    if (StartupChecks(argv[0], &has_broken_vulkan,
                      Settings::values.perform_vulkan_check.GetValue())) {
        return 0;
    }

#ifdef YUZU_CRASH_DUMPS
    Breakpad::InstallCrashHandler();
#endif

    // Init settings params
    QCoreApplication::setOrganizationName(QStringLiteral("STORM SWITCH"));
    QCoreApplication::setApplicationName(QStringLiteral("STORM SWITCH"));

    // Increases the maximum open file limit.
    // TODO: This should be common to all frontends.
#ifdef _WIN32
    // MSVCRT limits this to 2048 for some inexplicable (and likely arcane) reason,
    // so we have to account for that as well.
#ifdef __MSVCRT__
    _setmaxstdio(2048);
#else
    _setmaxstdio(8192);
#endif // __MSVCRT__
#elif defined(__unix__) || defined(__APPLE__)
    // Set the max open file limit to 8192, or the hard limit.
    // Most sane systems should not hit the hard limit here.
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
        rl.rlim_cur = std::min<rlim_t>(8192, rl.rlim_max);
        setrlimit(RLIMIT_NOFILE, &rl);
    }
#endif // _WIN32

#if defined(__APPLE__)
    // Convert the relative path to an absolute path before the chdir
    char resolved[PATH_MAX];
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "-input-profile") == 0) {
            ++i;
        } else if (argv[i][0] != '-' && argv[i][0] != '/' && realpath(argv[i], resolved)) {
            argv[i] = strdup(resolved);
        }
    }

    // If you start a bundle (binary) on OSX without the Terminal, the working directory is "/".
    // But since we require the working directory to be the executable path for the location of
    // the user folder in the Qt Frontend, we need to cd into that working directory
    const auto bin_path = Common::FS::GetBundleDirectory() / "..";
    chdir(Common::FS::PathToUTF8String(bin_path).c_str());
#endif

#ifdef __unix__
    // Set the DISPLAY variable in order to open web browsers
    // TODO (lat9nq): Find a better solution for AppImages to start external applications
    if (QString::fromLocal8Bit(qgetenv("DISPLAY")).isEmpty()) {
        qputenv("DISPLAY", ":0");
    }

    if (GraphicsBackend::GetForceX11() && qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "xcb");

    // Fix the Wayland appId. This needs to match the name of the .desktop file without the .desktop
    // suffix.
    QGuiApplication::setDesktopFileName(QStringLiteral("dev.eden_emu.eden"));
#endif

    auto rounding_policy = GetHighDpiRoundingPolicy();
    QApplication::setHighDpiScaleFactorRoundingPolicy(rounding_policy);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Disables the "?" button on all dialogs. Disabled by default on Qt6.
    QCoreApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);
#endif

    // Enables the core to make the qt created contexts current on std::threads
    QCoreApplication::setAttribute(Qt::AA_DontCheckOpenGLContextThreadAffinity);

#ifdef _WIN32
    QApplication::setStyle(QStringLiteral("windowsvista"));
#endif

    QApplication app(argc, argv);
    app.setApplicationDisplayName({});

#ifdef _WIN32
    OverrideWindowsFont();
#endif

    // Workaround for QTBUG-85409, for Suzhou numerals the number 1 is actually \u3021
    // so we can see if we get \u3008 instead
    // TL;DR all other number formats are consecutive in unicode code points
    // This bug is fixed in Qt6, specifically 6.0.0-alpha1
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const QLocale locale = QLocale::system();
    if (QStringLiteral("\u3008") == locale.toString(1)) {
        QLocale::setDefault(QLocale::system().name());
    }
#endif

    // Qt changes the locale and causes issues in float conversion using std::to_string() when
    // generating shaders
    setlocale(LC_ALL, "C");

    MainWindow main_window{has_broken_vulkan};
    // After settings have been loaded by GMainWindow, apply the filter
    main_window.show();

#ifdef _WIN32
    // Law 22: UIPI bypass for Drag-and-Drop window message filter
    typedef BOOL(WINAPI* PFN_ChangeWindowMessageFilterEx)(HWND, UINT, DWORD, PCHANGEFILTERSTRUCT);
    if (HMODULE user32 = GetModuleHandleW(L"user32.dll")) {
        auto pfnFilterEx = reinterpret_cast<PFN_ChangeWindowMessageFilterEx>(
            GetProcAddress(user32, "ChangeWindowMessageFilterEx"));
        if (pfnFilterEx) {
            HWND hwnd = reinterpret_cast<HWND>(main_window.winId());
            pfnFilterEx(hwnd, 0x0233 /* WM_DROPFILES */, 1 /* MSGFLT_ALLOW */, nullptr);
            pfnFilterEx(hwnd, 0x004A /* WM_COPYDATA */, 1 /* MSGFLT_ALLOW */, nullptr);
            pfnFilterEx(hwnd, 0x0049 /* WM_COPYGLOBALDATA */, 1 /* MSGFLT_ALLOW */, nullptr);
        }
    }
#endif

    app.setQuitOnLastWindowClosed(true);
    app.connect(&app, &QGuiApplication::applicationStateChanged, &main_window, &MainWindow::OnAppFocusStateChanged);
    const int exit_code = app.exec();
    Core::FlushPendingSaveStates();
    Common::Log::Stop();
#ifdef _WIN32
    if (hSingleInstanceMutex) {
        ReleaseMutex(hSingleInstanceMutex);
        CloseHandle(hSingleInstanceMutex);
        hSingleInstanceMutex = nullptr;
    }
    ::TerminateProcess(::GetCurrentProcess(), static_cast<UINT>(exit_code));
#endif
    return exit_code;
}

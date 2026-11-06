// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Don't move this! put it below and MSVC will get angry!
#include "video_core/vulkan_common/vulkan_wrapper.h"

#ifdef _WIN32
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <processthreadsapi.h>
#include <windows.h>
#include "common/fs/path_util.h"
#else
#include <cstring>
#include <errno.h>
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <fmt/core.h>
#include "video_core/vulkan_common/vulkan_instance.h"
#include "video_core/vulkan_common/vulkan_library.h"
#include "storm_switch/startup_checks.h"

#ifdef _WIN32
namespace {

std::filesystem::path GetVulkanStampPath(const char* arg0) {
    std::error_code ec;
    std::filesystem::path exe_dir;
    wchar_t exe_buf[MAX_PATH];
    if (GetModuleFileNameW(nullptr, exe_buf, MAX_PATH) > 0) {
        exe_dir = std::filesystem::path(exe_buf).parent_path();
    } else if (arg0 && *arg0) {
        exe_dir = std::filesystem::u8path(arg0).parent_path();
    }

    // 1. Portable mode: check if user folder exists next to executable
    if (!exe_dir.empty()) {
        const auto portable_user = exe_dir / L"user";
        if (std::filesystem::is_directory(portable_user, ec)) {
            return portable_user / L"cache" / L"vulkan_check_ok.stamp";
        }
    }

    // 2. Check Common::FS::GetEdenPath if initialized
    try {
        const auto& eden_cache = Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir);
        if (!eden_cache.empty()) {
            return eden_cache / "vulkan_check_ok.stamp";
        }
    } catch (...) {
    }

    // 3. Fallback to %APPDATA%/storm_switch/cache/vulkan_check_ok.stamp
    const wchar_t* appdata = _wgetenv(L"APPDATA");
    if (appdata && *appdata) {
        return std::filesystem::path(appdata) / L"storm_switch" / L"cache" / L"vulkan_check_ok.stamp";
    }

    if (!exe_dir.empty()) {
        return exe_dir / L"cache" / L"vulkan_check_ok.stamp";
    }

    return {};
}

std::string GetVulkanDllSignature() {
    wchar_t sys_dir[MAX_PATH];
    std::wstring vk_dll_path;
    if (GetSystemDirectoryW(sys_dir, MAX_PATH) > 0) {
        vk_dll_path = std::wstring(sys_dir) + L"\\vulkan-1.dll";
    } else {
        vk_dll_path = L"C:\\Windows\\System32\\vulkan-1.dll";
    }

    WIN32_FILE_ATTRIBUTE_DATA attr;
    if (!GetFileAttributesExW(vk_dll_path.c_str(), GetFileExInfoStandard, &attr)) {
        return {};
    }

    ULARGE_INTEGER file_size;
    file_size.HighPart = attr.nFileSizeHigh;
    file_size.LowPart = attr.nFileSizeLow;

    ULARGE_INTEGER mtime;
    mtime.HighPart = attr.ftLastWriteTime.dwHighDateTime;
    mtime.LowPart = attr.ftLastWriteTime.dwLowDateTime;

    return fmt::format("{}:{}", file_size.QuadPart, mtime.QuadPart);
}

bool ReadVulkanStamp(const std::filesystem::path& stamp_path, std::string& out_stamp) {
    std::error_code ec;
    if (stamp_path.empty() || !std::filesystem::exists(stamp_path, ec)) {
        return false;
    }
    std::ifstream file(stamp_path);
    if (!file.is_open()) {
        return false;
    }
    std::string line;
    if (std::getline(file, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) {
            line.pop_back();
        }
        out_stamp = line;
        return !out_stamp.empty();
    }
    return false;
}

void WriteVulkanStamp(const std::filesystem::path& stamp_path, const std::string& sig) {
    if (stamp_path.empty() || sig.empty()) {
        return;
    }
    std::error_code ec;
    std::filesystem::create_directories(stamp_path.parent_path(), ec);
    if (ec) {
        return;
    }
    std::ofstream file(stamp_path, std::ios::trunc);
    if (file.is_open()) {
        file << sig << '\n';
    }
}

} // anonymous namespace
#endif

void CheckVulkan() {
    // Just start the Vulkan loader, this will crash if something is wrong
    try {
        Vulkan::vk::InstanceDispatch dld;
        const auto library = Vulkan::OpenLibrary();
        const Vulkan::vk::Instance instance =
            Vulkan::CreateInstance(*library, dld, VK_API_VERSION_1_1);

    } catch (const Vulkan::vk::Exception& exception) {
        fmt::print(stderr, "Failed to initialize Vulkan: {}\n", exception.what());
    }
}

bool CheckEnvVars(bool* is_child) {
#ifdef _WIN32
    // Check environment variable to see if we are the child
    char variable_contents[8];
    const DWORD startup_check_var =
        GetEnvironmentVariableA(STARTUP_CHECK_ENV_VAR, variable_contents, 8);
    if (startup_check_var > 0 && std::strncmp(variable_contents, ENV_VAR_ENABLED_TEXT, 8) == 0) {
        SetEnvironmentVariableA(STARTUP_CHECK_ENV_VAR, nullptr);
        CheckVulkan();
        return true;
    }

    // Don't perform startup checks if we are a child process
    char is_child_s[8];
    const DWORD is_child_len = GetEnvironmentVariableA(IS_CHILD_ENV_VAR, is_child_s, 8);
    if (is_child_len > 0 && std::strncmp(is_child_s, ENV_VAR_ENABLED_TEXT, 8) == 0) {
        *is_child = true;
        return false;
    }
#else
    const char* startup_check_var = getenv(STARTUP_CHECK_ENV_VAR);
    if (startup_check_var != nullptr &&
        std::strncmp(startup_check_var, ENV_VAR_ENABLED_TEXT, 8) == 0) {
        unsetenv(STARTUP_CHECK_ENV_VAR);
        CheckVulkan();
        return true;
    }
#endif
    return false;
}

bool StartupChecks(const char* arg0, bool* has_broken_vulkan, bool perform_vulkan_check) {
#ifdef _WIN32
    if (perform_vulkan_check) {
        const std::filesystem::path stamp_path = GetVulkanStampPath(arg0);
        const std::string current_sig = GetVulkanDllSignature();

        if (!stamp_path.empty() && !current_sig.empty()) {
            std::string cached_sig;
            if (ReadVulkanStamp(stamp_path, cached_sig) && cached_sig == current_sig) {
                // Instant cached path: Vulkan loader verified on this system previously
                if (has_broken_vulkan) {
                    *has_broken_vulkan = false;
                }
                return false;
            }
        }

        SetEnvironmentVariableA(STARTUP_CHECK_ENV_VAR, ENV_VAR_ENABLED_TEXT);
        SetEnvironmentVariableA(IS_CHILD_ENV_VAR, ENV_VAR_ENABLED_TEXT);

        // Spawn child process that performs Vulkan check
        PROCESS_INFORMATION process_info;
        std::memset(&process_info, '\0', sizeof(process_info));

        if (SpawnChild(arg0, &process_info, 0)) {
            // Wait until the process exits and get exit code from it
            WaitForSingleObject(process_info.hProcess, INFINITE);
            DWORD exit_code = STILL_ACTIVE;
            const int err = GetExitCodeProcess(process_info.hProcess, &exit_code);
            if (err == 0) {
                fmt::print(stderr, "GetExitCodeProcess failed with error {}\n", GetLastError());
            }

            // Vulkan is broken if the child crashed (return value is not zero)
            if (has_broken_vulkan) {
                *has_broken_vulkan = (exit_code != 0);
            }

            if (exit_code == 0 && !stamp_path.empty() && !current_sig.empty()) {
                WriteVulkanStamp(stamp_path, current_sig);
            } else if (has_broken_vulkan && *has_broken_vulkan && !stamp_path.empty()) {
                std::error_code ec;
                std::filesystem::remove(stamp_path, ec);
            }

            if (CloseHandle(process_info.hProcess) == 0) {
                fmt::print(stderr, "CloseHandle failed with error {}\n", GetLastError());
            }
            if (CloseHandle(process_info.hThread) == 0) {
                fmt::print(stderr, "CloseHandle failed with error {}\n", GetLastError());
            }
        } else {
            if (has_broken_vulkan) {
                *has_broken_vulkan = false;
            }
        }

        SetEnvironmentVariableA(STARTUP_CHECK_ENV_VAR, nullptr);
        SetEnvironmentVariableA(IS_CHILD_ENV_VAR, nullptr);
    }

#else
    const int env_var_set = setenv(STARTUP_CHECK_ENV_VAR, ENV_VAR_ENABLED_TEXT, 1);
    if (env_var_set == -1) {
        const int err = errno;
        fmt::print(stderr, "setenv failed to set {} with error {}\n", STARTUP_CHECK_ENV_VAR, err);
        return false;
    }

    if (perform_vulkan_check) {
        const pid_t pid = SpawnChild(arg0);
        if (pid == -1) {
            return false;
        }

        // Get exit code from child process
        int status;
        const int r_val = waitpid(pid, &status, 0);
        if (r_val == -1) {
            const int err = errno;
            fmt::print(stderr, "wait failed with error {}\n", err);
            return false;
        }
        // Vulkan is broken if the child crashed (return value is not zero)
        *has_broken_vulkan = (status != 0);
    }

    const int env_var_cleared = unsetenv(STARTUP_CHECK_ENV_VAR);
    if (env_var_cleared == -1) {
        const int err = errno;
        fmt::print(stderr, "unsetenv failed to clear {} with error {}\n", STARTUP_CHECK_ENV_VAR,
                   err);
    }
#endif
    return false;
}

#ifdef _WIN32
bool SpawnChild(const char* arg0, PROCESS_INFORMATION* pi, int flags) {
    STARTUPINFOA startup_info;

    std::memset(&startup_info, '\0', sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);

    std::string cmd = fmt::format("\"{}\"", arg0);

    const bool process_created = CreateProcessA(nullptr,       // lpApplicationName
                                                cmd.data(),    // lpCommandLine
                                                nullptr,       // lpProcessAttributes
                                                nullptr,       // lpThreadAttributes
                                                false,         // bInheritHandles
                                                flags,         // dwCreationFlags
                                                nullptr,       // lpEnvironment
                                                nullptr,       // lpCurrentDirectory
                                                &startup_info, // lpStartupInfo
                                                pi             // lpProcessInformation
    );
    if (!process_created) {
        fmt::print(stderr, "CreateProcessA failed with error {}\n", GetLastError());
        return false;
    }

    return true;
}
#else
pid_t SpawnChild(const char* arg0) {
    const pid_t pid = fork();

    if (pid == -1) {
        // error
        const int err = errno;
        fmt::print(stderr, "fork failed with error {}\n", err);
        return pid;
    } else if (pid == 0) {
        // child
        execlp(arg0, arg0, nullptr);
        const int err = errno;
        fmt::print(stderr, "execl failed with error {}\n", err);
        _exit(0);
    }

    return pid;
}
#endif

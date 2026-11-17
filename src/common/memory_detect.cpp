// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#ifdef _WIN32
// clang-format off
#include <windows.h>
#include <sysinfoapi.h>
// clang-format on
#else
#include <sys/types.h>
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/sysctl.h>
#elif defined(__linux__)
#include <sys/sysinfo.h>
#else
#include <unistd.h>
#endif
#endif

#include <atomic>
#include <chrono>
#include <cstdio>

#include "common/memory_detect.h"

namespace Common {

// Detects the RAM and Swapfile sizes
static MemoryInfo Detect() {
    MemoryInfo mem_info{};

#ifdef _WIN32
    MEMORYSTATUSEX memorystatus;
    memorystatus.dwLength = sizeof(memorystatus);
    GlobalMemoryStatusEx(&memorystatus);
    mem_info.TotalPhysicalMemory = memorystatus.ullTotalPhys;
    mem_info.TotalSwapMemory = memorystatus.ullTotalPageFile - mem_info.TotalPhysicalMemory;
#elif defined(__APPLE__)
    u64 ramsize;
    struct xsw_usage vmusage;
    std::size_t sizeof_ramsize = sizeof(ramsize);
    std::size_t sizeof_vmusage = sizeof(vmusage);
    // hw and vm are defined in sysctl.h
    // https://github.com/apple/darwin-xnu/blob/master/bsd/sys/sysctl.h#L471
    // sysctlbyname(const char *, void *, size_t *, void *, size_t);
    sysctlbyname("hw.memsize", &ramsize, &sizeof_ramsize, nullptr, 0);
    sysctlbyname("vm.swapusage", &vmusage, &sizeof_vmusage, nullptr, 0);
    mem_info.TotalPhysicalMemory = ramsize;
    mem_info.TotalSwapMemory = vmusage.xsu_total;
#elif defined(__FreeBSD__)
    u_long physmem, swap_total;
    std::size_t sizeof_u_long = sizeof(u_long);
    // sysctlbyname(const char *, void *, size_t *, const void *, size_t);
    sysctlbyname("hw.physmem", &physmem, &sizeof_u_long, nullptr, 0);
    sysctlbyname("vm.swap_total", &swap_total, &sizeof_u_long, nullptr, 0);
    mem_info.TotalPhysicalMemory = physmem;
    mem_info.TotalSwapMemory = swap_total;
#elif defined(__linux__)
    struct sysinfo meminfo;
    sysinfo(&meminfo);
    mem_info.TotalPhysicalMemory = meminfo.totalram;
    mem_info.TotalSwapMemory = meminfo.totalswap;
#else
    mem_info.TotalPhysicalMemory = sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGE_SIZE);
    mem_info.TotalSwapMemory = 0;
#endif

    return mem_info;
}

const MemoryInfo& GetMemInfo() {
    static MemoryInfo mem_info = Detect();
    return mem_info;
}

u64 GetAvailablePhysicalMemory() {
#ifdef _WIN32
    MEMORYSTATUSEX memorystatus;
    memorystatus.dwLength = sizeof(memorystatus);
    if (!GlobalMemoryStatusEx(&memorystatus)) {
        return 0;
    }
    return memorystatus.ullAvailPhys;
#elif defined(__linux__)
    // MemAvailable counts the page cache the kernel can drop, which sysinfo's freeram doesn't.
    std::FILE* const file = std::fopen("/proc/meminfo", "r");
    if (file == nullptr) {
        return 0;
    }
    u64 available_kib = 0;
    char line[128];
    while (std::fgets(line, sizeof(line), file) != nullptr) {
        unsigned long long value = 0;
        if (std::sscanf(line, "MemAvailable: %llu kB", &value) == 1) {
            available_kib = value;
            break;
        }
    }
    std::fclose(file);
    return available_kib * 1024;
#else
    return 0;
#endif
}

bool IsHostLowOnMemory() {
    using Clock = std::chrono::steady_clock;
    static std::atomic<Clock::rep> checked_at{};
    static std::atomic<bool> low{};
    const Clock::rep now = Clock::now().time_since_epoch().count();
    const Clock::rep last = checked_at.load(std::memory_order_relaxed);
    if (last == 0 || now - last >= Clock::duration{std::chrono::seconds{1}}.count()) {
        checked_at.store(now, std::memory_order_relaxed);
        low.store(IsLowOnMemory(GetAvailablePhysicalMemory(), GetMemInfo().TotalPhysicalMemory),
                  std::memory_order_relaxed);
    }
    return low.load(std::memory_order_relaxed);
}

} // namespace Common

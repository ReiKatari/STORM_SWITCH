# SPDX-FileCopyrightText: Copyright 2025 crueter
# SPDX-License-Identifier: LGPL-3.0-or-later

## UseLTO ##

# Enable Interprocedural Optimization (IPO) / Link-Time Optimization (LTO).

option(ENABLE_LTO "Enable Link-Time Optimization (LTO)" OFF)
option(YUZU_USE_THIN_LTO "Use Clang ThinLTO for faster and scalable LTO" ON)

if (ENABLE_LTO)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND YUZU_USE_THIN_LTO)
        message(STATUS "LTO: Using Clang ThinLTO (-flto=thin)")
        add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:-flto=thin>)
        add_link_options("-flto=thin" "-Wl,--thinlto-cache-dir=${CMAKE_BINARY_DIR}/thinlto-cache")
        set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
    else()
        include(CheckIPOSupported)
        check_ipo_supported(RESULT COMPILER_SUPPORTS_LTO)
        if(NOT COMPILER_SUPPORTS_LTO)
            message(FATAL_ERROR
            "Your compiler does not support interprocedural optimization"
            " (IPO). Disable ENABLE_LTO and try again.")
        endif()
        set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ${COMPILER_SUPPORTS_LTO})
    endif()
endif()
# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

# SPDX-FileCopyrightText: 2019 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

# generate git/build information
include(GetSCMRev)

function(get_timestamp _var)
    string(TIMESTAMP timestamp "%Y-%m-%d" UTC)
    set(${_var} "${timestamp}" PARENT_SCOPE)
endfunction()

get_timestamp(BUILD_DATE)

set(REPO_NAME "STORM SWITCH")

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/GIT-RELEASE"
    "${CMAKE_SOURCE_DIR}/GIT-TAG"
)

# Determine version priority:
# 1. Always read fresh from GIT-RELEASE file on disk if available
# 2. GIT_RELEASE / BUILD_VERSION / GIT_TAG CMake variables
# 3. Fallback default 14.0.3
if (EXISTS "${CMAKE_SOURCE_DIR}/GIT-RELEASE")
    file(STRINGS "${CMAKE_SOURCE_DIR}/GIT-RELEASE" _storm_git_rel LIMIT_COUNT 1)
    string(STRIP "${_storm_git_rel}" BUILD_VERSION)
elseif (DEFINED SRC_DIR AND EXISTS "${SRC_DIR}/GIT-RELEASE")
    file(STRINGS "${SRC_DIR}/GIT-RELEASE" _storm_git_rel LIMIT_COUNT 1)
    string(STRIP "${_storm_git_rel}" BUILD_VERSION)
elseif (DEFINED GIT_RELEASE AND NOT GIT_RELEASE STREQUAL "")
    string(STRIP "${GIT_RELEASE}" BUILD_VERSION)
elseif (DEFINED BUILD_VERSION AND NOT BUILD_VERSION STREQUAL "")
    string(STRIP "${BUILD_VERSION}" BUILD_VERSION)
elseif (DEFINED GIT_TAG AND NOT GIT_TAG STREQUAL "")
    string(STRIP "${GIT_TAG}" BUILD_VERSION)
else()
    set(BUILD_VERSION "14.0.3")
endif()

set(GIT_TAG "${BUILD_VERSION}")
set(GIT_RELEASE "${BUILD_VERSION}")
set(GIT_REFSPEC "main")
set(IS_DEV_BUILD false)
set(IS_NIGHTLY_BUILD false)

set(BUILD_TAG "${BUILD_VERSION}")
set(BUILD_ID "${BUILD_VERSION}")
set(BUILD_FULLNAME "${REPO_NAME} ${BUILD_VERSION}")
set(GIT_DESC "${BUILD_VERSION}")

# Generate cpp with Git revision from template

set(BUILD_AUTO_UPDATE_STABLE_REPO "ReiKatari/STORM_SWITCH")
set(BUILD_AUTO_UPDATE_STABLE_API "api.github.com")
set(BUILD_AUTO_UPDATE_STABLE_API_PATH "/repos/")

set(BUILD_AUTO_UPDATE_WEBSITE "https://github.com")
set(BUILD_AUTO_UPDATE_API "api.github.com")
set(BUILD_AUTO_UPDATE_API_PATH "/repos/ReiKatari/STORM_SWITCH/releases/latest")
set(BUILD_AUTO_UPDATE_REPO "ReiKatari/STORM_SWITCH")

set(TITLE_BAR_FORMAT_IDLE "STORM SWITCH ${BUILD_VERSION}")
set(TITLE_BAR_FORMAT_RUNNING "STORM SWITCH ${BUILD_VERSION} | {3} | {1} | {2}")
set(CXX_COMPILER "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

configure_file(scm_rev.cpp.in scm_rev.cpp @ONLY)

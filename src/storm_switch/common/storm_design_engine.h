// SPDX-FileCopyrightText: 2026 STORM SOFT
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <string>
#include <utility>
#include <QString>

namespace StormTheme {
struct Palette;
}

namespace StormDesign {

enum class Preset {
    HyperionGlass = 0,
    CyberDeck = 1,
    AuroraQuantum = 2,
    OrbitalSpatial = 3,
    ChronoNexus = 4,
};

inline constexpr std::array<std::pair<const char*, const char*>, 5> kDesigns{{
    {"hyperion_glass", "STORM HYPERION GLASS 3D"},
    {"cyber_deck", "STORM CYBER-DECK TACTILE"},
    {"aurora_quantum", "STORM AURORA QUANTUM"},
    {"orbital_spatial", "STORM ORBITAL SPATIAL UI"},
    {"chrono_nexus", "STORM CHRONO-NEXUS COMMANDER"},
}};

Preset GetActivePreset(const std::string& explicit_id = {});
std::string GetActiveDesignId();

QString GetDesignSelectorLabel();
QString GetLocalizedDesignName(const std::string& design_id);
QString GetLocalizedDesignTooltip();

/// Complete 3D volumetric & futuristic dialog/widget stylesheet parameterized by active Theme Palette + active Design
QString BuildDialogStylesheet(const StormTheme::Palette& pal, const std::string& explicit_design = {});

/// Complete 3D volumetric stylesheet for ConfigureDialog & ConfigurePerGame
QString BuildConfigDialogStylesheet(const StormTheme::Palette& pal, bool is_per_game = false,
                                    const std::string& explicit_design = {});

/// Complete 3D volumetric architectural overlay for MainWindow (toolbar, menus, cards, game list, status bar, inputs, buttons)
QString BuildMainWindowDesignOverlay(const StormTheme::Palette& pal, const std::string& explicit_design = {});

/// Installs the lightweight 60/120 FPS interactive 3D hover/press/tab-transition animator on qApp (idempotent)
void InstallGlobalAnimator();

} // namespace StormDesign

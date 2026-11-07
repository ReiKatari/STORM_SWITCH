// SPDX-FileCopyrightText: 2026 STORM SOFT
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <string>
#include <utility>
#include <QLocale>
#include <QString>

#include "common/common_types.h"
#include "qt_common/config/uisettings.h"

namespace StormTheme {
struct Palette;
}

namespace StormDesign {

enum class Preset {
    TitaniumCyberDeck = 0,
    HyperionDockedConsole = 1,
    ValkyrieTacticalRig = 2,
    SolarisSwitch2Pro = 3,
    AetherFrostedStudio = 4,

    // Backward compatibility aliases
    HyperionGlass = HyperionDockedConsole,
    CyberDeck = TitaniumCyberDeck,
    AuroraQuantum = AetherFrostedStudio,
    OrbitalSpatial = ValkyrieTacticalRig,
    ChronoNexus = SolarisSwitch2Pro,
};

inline constexpr std::array<std::pair<const char*, const char*>, 5> kDesigns{{
    {"titanium_cyber_deck", "TITANIUM CYBER DECK"},
    {"hyperion_docked_console", "HYPERION DOCKED CONSOLE"},
    {"valkyrie_tactical_rig", "VALKYRIE TACTICAL RIG"},
    {"solaris_switch2_pro", "SOLARIS SWITCH 2 PRO"},
    {"aether_frosted_studio", "AETHER FROSTED STUDIO"},
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

namespace StormFormat {

inline bool IsRussian() {
    const std::string& lang = UISettings::values.language.GetValue();
    if (lang.rfind("ru", 0) == 0) {
        return true;
    }
    if (lang.empty() && QLocale().language() == QLocale::Russian) {
        return true;
    }
    return false;
}

inline QString FloatNumber(double val, int precision = 2) {
    QString s = QString::number(val, 'f', precision);
    if (IsRussian()) {
        s.replace(QLatin1Char('.'), QLatin1Char(','));
    }
    return s;
}

inline QString ReadableByteSize(qulonglong bytes) {
    if (bytes == 0) {
        return IsRussian() ? QStringLiteral("0 Б") : QStringLiteral("0 B");
    }
    const double b = static_cast<double>(bytes);
    const bool ru = IsRussian();
    if (bytes >= 1024ULL * 1024ULL * 1024ULL) {
        const double gb = b / (1024.0 * 1024.0 * 1024.0);
        return FloatNumber(gb, 2) + (ru ? QStringLiteral(" ГБ") : QStringLiteral(" GB"));
    }
    if (bytes >= 1024ULL * 1024ULL) {
        const double mb = b / (1024.0 * 1024.0);
        return FloatNumber(mb, 2) + (ru ? QStringLiteral(" МБ") : QStringLiteral(" MB"));
    }
    if (bytes >= 1024ULL) {
        const double kb = b / 1024.0;
        return FloatNumber(kb, 1) + (ru ? QStringLiteral(" КБ") : QStringLiteral(" KB"));
    }
    return QString::number(bytes) + (ru ? QStringLiteral(" Б") : QStringLiteral(" B"));
}

inline QString Fps(double fps) {
    return FloatNumber(fps, 0) + (IsRussian() ? QStringLiteral(" кадр/с") : QStringLiteral(" fps"));
}

inline QString Frametime(double ms) {
    return FloatNumber(ms, 2) + (IsRussian() ? QStringLiteral(" мс") : QStringLiteral(" ms"));
}

inline QString Temperature(double deg_c) {
    return FloatNumber(deg_c, 1) + QStringLiteral(" °C");
}

inline QString Multiplier(double mult) {
    return FloatNumber(mult, 2) + (IsRussian() ? QStringLiteral("×") : QStringLiteral("x"));
}

inline QString CleanVersion(QString ver) {
    while (ver.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        ver.remove(0, 1);
    }
    ver = ver.trimmed();
    if (ver.isEmpty() || ver == QStringLiteral("0")) {
        ver = QStringLiteral("1.0.0");
    }
    return ver;
}

inline QString FormatTitleId(u64 title_id) {
    return QString::asprintf("%016llX", static_cast<unsigned long long>(title_id));
}

inline QString FormatAddons(int count) {
    if (count <= 0) {
        return IsRussian() ? QStringLiteral("Нет дополнений") : QStringLiteral("No Add-ons");
    }
    return IsRussian() ? QStringLiteral("Дополнений: %1").arg(count)
                       : QStringLiteral("Add-ons: %1").arg(count);
}

} // namespace StormFormat

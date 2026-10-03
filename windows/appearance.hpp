// SPDX-License-Identifier: GPL-3.0-only
// LLM-assisted implementation; see LICENSE and DISCLAIMER.md.
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include "tone_core.hpp"

namespace morse {
enum class ThemeId { System, Light, Dark, Midnight, Amber, Matrix };
struct Settings {
    ThemeId theme = ThemeId::System;
    unsigned frequency = toneDefaultHz;
};
struct ThemePalette {
    std::uint32_t background, text, muted, contact, output, border, warning, surface;
    std::uint32_t contactActive, outputActive, control, hover, selection, selectionText;
    bool dark;
};
constexpr std::uint32_t themeRgb(unsigned red, unsigned green, unsigned blue) {
    return red | (green << 8) | (blue << 16);
}
inline const char* themeIdName(ThemeId id) {
    switch (id) {
    case ThemeId::Light: return "light";
    case ThemeId::Dark: return "dark";
    case ThemeId::Midnight: return "midnight";
    case ThemeId::Amber: return "amber";
    case ThemeId::Matrix: return "matrix";
    default: return "system";
    }
}
inline std::string_view trimSetting(std::string_view value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}
inline bool settingEquals(std::string_view left, std::string_view right) {
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0; i < left.size(); ++i) {
        const auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? char(c + ('a' - 'A')) : c; };
        if (lower(left[i]) != lower(right[i])) return false;
    }
    return true;
}
inline bool parseThemeId(std::string_view value, ThemeId& result) {
    value = trimSetting(value);
    constexpr std::array<ThemeId, 6> ids{ThemeId::System, ThemeId::Light, ThemeId::Dark,
                                        ThemeId::Midnight, ThemeId::Amber, ThemeId::Matrix};
    for (const auto id : ids) if (settingEquals(value, themeIdName(id))) { result = id; return true; }
    return false;
}
inline bool parseFrequency(std::string_view value, unsigned& result) {
    value = trimSetting(value);
    if (value.empty() || value.size() > 4) return false;
    unsigned hz = 0;
    for (const char c : value) {
        if (c < '0' || c > '9') return false;
        hz = hz * 10 + unsigned(c - '0');
    }
    if (hz < toneMinHz || hz > toneMaxHz) return false;
    result = hz; return true;
}
inline Settings parseSettings(std::string_view source) {
    Settings result;
    if (source.size() > 16384) return result;
    if (source.substr(0, 3) == "\xef\xbb\xbf") source.remove_prefix(3);
    bool ownSection = false;
    while (!source.empty()) {
        const auto end = source.find('\n');
        const auto line = trimSetting(source.substr(0, end));
        if (end == std::string_view::npos) source = {};
        else source.remove_prefix(end + 1);
        if (line.empty() || line.front() == ';' || line.front() == '#') continue;
        if (line.front() == '[') {
            ownSection = line.back() == ']' && settingEquals(trimSetting(line.substr(1, line.size() - 2)), "MorseBridge");
            continue;
        }
        if (!ownSection) continue;
        const auto equals = line.find('=');
        if (equals == std::string_view::npos) continue;
        const auto key = trimSetting(line.substr(0, equals));
        const auto value = trimSetting(line.substr(equals + 1));
        if (settingEquals(key, "Theme")) {
            ThemeId theme;
            result.theme = parseThemeId(value, theme) ? theme : ThemeId::System;
        } else if (settingEquals(key, "Frequency")) {
            unsigned frequency;
            result.frequency = parseFrequency(value, frequency) ? frequency : toneDefaultHz;
        }
    }
    return result;
}
inline std::string serializeSettings(const Settings& settings) {
    const auto hz = settings.frequency >= toneMinHz && settings.frequency <= toneMaxHz
        ? settings.frequency : toneDefaultHz;
    return std::string("[MorseBridge]\nTheme=") + themeIdName(settings.theme) +
           "\nFrequency=" + std::to_string(hz) + "\n";
}
inline ThemePalette paletteFor(ThemeId id, bool systemDark, bool highContrast) {
    using C = std::uint32_t;
    constexpr auto rgb = themeRgb;
    // The Windows adapter replaces this portable contrast palette with actual
    // GetSysColor values, so selecting a theme cannot override a contrast theme.
    if (highContrast) return {C(0), rgb(255,255,255), rgb(255,255,255), rgb(255,255,255),
        rgb(255,255,255), rgb(255,255,255), rgb(255,255,255), C(0), C(0), C(0), C(0),
        rgb(0,0,128), rgb(0,0,128), rgb(255,255,255), true};
    if (id == ThemeId::System) id = systemDark ? ThemeId::Dark : ThemeId::Light;
    switch (id) {
    case ThemeId::Dark:
        return {rgb(18,24,32),rgb(234,240,247),rgb(166,183,201),rgb(94,215,168),rgb(117,177,255),
                rgb(68,83,100),rgb(255,187,103),rgb(28,36,46),rgb(24,63,51),rgb(29,52,81),
                rgb(36,46,59),rgb(50,65,82),rgb(48,93,151),rgb(255,255,255),true};
    case ThemeId::Midnight:
        return {rgb(9,17,35),rgb(223,235,255),rgb(158,180,211),rgb(104,224,202),rgb(135,171,255),
                rgb(48,69,105),rgb(255,188,120),rgb(16,29,52),rgb(18,65,65),rgb(29,46,92),
                rgb(23,39,67),rgb(35,56,89),rgb(61,85,157),rgb(255,255,255),true};
    case ThemeId::Amber:
        return {rgb(27,21,12),rgb(255,224,164),rgb(216,185,137),rgb(255,197,85),rgb(246,161,97),
                rgb(102,76,40),rgb(255,135,109),rgb(43,32,17),rgb(77,55,18),rgb(78,40,22),
                rgb(51,38,20),rgb(74,55,28),rgb(125,83,25),rgb(255,237,205),true};
    case ThemeId::Matrix:
        return {rgb(5,15,8),rgb(184,255,195),rgb(129,201,150),rgb(117,255,155),rgb(104,216,205),
                rgb(40,90,53),rgb(255,198,101),rgb(10,27,15),rgb(20,62,32),rgb(17,54,47),
                rgb(15,37,21),rgb(24,55,32),rgb(37,105,52),rgb(228,255,231),true};
    default:
        return {rgb(244,247,249),rgb(23,42,59),rgb(90,108,124),rgb(0,125,103),rgb(38,101,186),
                rgb(168,184,197),rgb(163,86,8),rgb(255,255,255),rgb(226,247,240),rgb(231,240,255),
                rgb(255,255,255),rgb(228,236,244),rgb(38,101,186),rgb(255,255,255),false};
    }
}
} // namespace morse

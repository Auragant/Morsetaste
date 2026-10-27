// SPDX-License-Identifier: GPL-3.0-only
// LLM-assisted implementation; see LICENSE and DISCLAIMER.md.
#pragma once
#include <cstddef>
#include <string_view>
#include <vector>

namespace morse {
inline constexpr unsigned spaceKey = 0x20;
inline constexpr unsigned leftCtrlKey = 0xA2;
inline constexpr std::size_t maxCustomOutputKeys = 32;
struct OutputKey { unsigned vk; const char* name; };

// Stable Windows virtual-key identifiers. Keeping this table platform neutral
// lets persistence and validation be checked without generating keyboard input.
inline const std::vector<OutputKey>& supportedOutputKeys() {
    static const std::vector<OutputKey> keys{
        {spaceKey,"SPACE"}, {leftCtrlKey,"Left CTRL"}, {0xA3,"Right CTRL"},
        {0xA0,"Left SHIFT"}, {0xA1,"Right SHIFT"}, {0xA4,"Left ALT"}, {0xA5,"Right ALT"},
        {0x0D,"ENTER"}, {0x09,"TAB"}, {0x1B,"ESC"}, {0x08,"BACKSPACE"},
        {0x2E,"DELETE"}, {0x2D,"INSERT"}, {0x24,"HOME"}, {0x23,"END"},
        {0x21,"PAGE UP"}, {0x22,"PAGE DOWN"}, {0x25,"LEFT"}, {0x27,"RIGHT"},
        {0x26,"UP"}, {0x28,"DOWN"},
        {0x41,"A"}, {0x42,"B"}, {0x43,"C"}, {0x44,"D"}, {0x45,"E"}, {0x46,"F"},
        {0x47,"G"}, {0x48,"H"}, {0x49,"I"}, {0x4A,"J"}, {0x4B,"K"}, {0x4C,"L"},
        {0x4D,"M"}, {0x4E,"N"}, {0x4F,"O"}, {0x50,"P"}, {0x51,"Q"}, {0x52,"R"},
        {0x53,"S"}, {0x54,"T"}, {0x55,"U"}, {0x56,"V"}, {0x57,"W"}, {0x58,"X"},
        {0x59,"Y"}, {0x5A,"Z"},
        {0x30,"0"}, {0x31,"1"}, {0x32,"2"}, {0x33,"3"}, {0x34,"4"},
        {0x35,"5"}, {0x36,"6"}, {0x37,"7"}, {0x38,"8"}, {0x39,"9"},
        {0x70,"F1"}, {0x71,"F2"}, {0x72,"F3"}, {0x73,"F4"}, {0x74,"F5"}, {0x75,"F6"},
        {0x76,"F7"}, {0x77,"F8"}, {0x78,"F9"}, {0x79,"F10"}, {0x7A,"F11"}, {0x7B,"F12"},
        {0x7C,"F13"}, {0x7D,"F14"}, {0x7E,"F15"}, {0x7F,"F16"}, {0x80,"F17"}, {0x81,"F18"},
        {0x82,"F19"}, {0x83,"F20"}, {0x84,"F21"}, {0x85,"F22"}, {0x86,"F23"}, {0x87,"F24"},
        {0x60,"NUMPAD 0"}, {0x61,"NUMPAD 1"}, {0x62,"NUMPAD 2"}, {0x63,"NUMPAD 3"}, {0x64,"NUMPAD 4"},
        {0x65,"NUMPAD 5"}, {0x66,"NUMPAD 6"}, {0x67,"NUMPAD 7"}, {0x68,"NUMPAD 8"}, {0x69,"NUMPAD 9"},
        {0x6B,"NUMPAD ADD"}, {0x6D,"NUMPAD SUBTRACT"}, {0x6A,"NUMPAD MULTIPLY"},
        {0x6F,"NUMPAD DIVIDE"}, {0x6E,"NUMPAD DECIMAL"}
    };
    return keys;
}
inline const OutputKey* findOutputKey(unsigned vk) {
    for (const auto& key : supportedOutputKeys()) if (key.vk == vk) return &key;
    return nullptr;
}
inline const OutputKey* parseOutputKey(std::string_view name) {
    const auto first = name.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return nullptr;
    name = name.substr(first, name.find_last_not_of(" \t\r\n") - first + 1);
    for (const auto& key : supportedOutputKeys()) {
        const std::string_view candidate(key.name);
        if (candidate.size() != name.size()) continue;
        bool equal = true;
        for (std::size_t i = 0; i < name.size(); ++i) {
            const auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? char(c + ('a' - 'A')) : c; };
            if (lower(candidate[i]) != lower(name[i])) { equal = false; break; }
        }
        if (equal) return &key;
    }
    return nullptr;
}
} // namespace morse

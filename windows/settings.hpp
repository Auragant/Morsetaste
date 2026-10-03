// SPDX-License-Identifier: GPL-3.0-only
// LLM-assisted implementation; see LICENSE and DISCLAIMER.md.
#pragma once
#include "appearance.hpp"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <system_error>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#endif

namespace morse {
inline Settings loadSettings(const std::filesystem::path& path) noexcept {
    try {
        if (path.empty()) return {};
        std::ifstream stream(path, std::ios::binary | std::ios::ate);
        if (!stream) return {};
        const auto length = stream.tellg();
        if (length < 0 || length > 16384) return {};
        std::string contents(static_cast<std::size_t>(length), '\0');
        stream.seekg(0);
        if (length != 0 && !stream.read(contents.data(), length)) return {};
        return parseSettings(contents);
    } catch (...) { return {}; }
}
inline bool saveSettings(const std::filesystem::path& path, const Settings& value) noexcept {
    std::filesystem::path temporary;
    try {
        if (path.empty()) return false;
        std::error_code error;
        if (!path.parent_path().empty()) {
            std::filesystem::create_directories(path.parent_path(), error);
            if (error) return false;
        }
        static std::atomic<unsigned> sequence{0};
        const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
        temporary = path;
        temporary += ".tmp-" + std::to_string(tick) + "-" + std::to_string(++sequence);
        const auto contents = serializeSettings(value);
        {
            std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
            if (!stream) return false;
            stream.write(contents.data(), static_cast<std::streamsize>(contents.size()));
            stream.close();
            if (!stream) { std::filesystem::remove(temporary, error); return false; }
        }
#ifdef _WIN32
        const bool replaced = MoveFileExW(temporary.c_str(), path.c_str(),
                                          MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
#else
        std::filesystem::rename(temporary, path, error);
        const bool replaced = !error;
#endif
        if (!replaced) std::filesystem::remove(temporary, error);
        return replaced;
    } catch (...) {
        if (!temporary.empty()) { std::error_code error; std::filesystem::remove(temporary, error); }
        return false;
    }
}
#ifdef _WIN32
inline std::wstring settingsPath() noexcept {
    PWSTR base = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &base))) return {};
    std::wstring result;
    try { result = (std::filesystem::path(base) / L"MorseBridge" / L"settings.ini").wstring(); }
    catch (...) {}
    CoTaskMemFree(base);
    return result;
}
inline Settings loadSettings(const std::wstring& path) noexcept {
    try { return loadSettings(std::filesystem::path(path)); } catch (...) { return {}; }
}
inline bool saveSettings(const std::wstring& path, const Settings& value) noexcept {
    try { return saveSettings(std::filesystem::path(path), value); } catch (...) { return false; }
}
#endif
} // namespace morse

// SPDX-License-Identifier: GPL-3.0-only
// LLM-assisted implementation; see LICENSE, DISCLAIMER.md and THIRD_PARTY_NOTICES.md.
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <setupapi.h>
#include <devguid.h>
#include <wtsapi32.h>
#include <shellapi.h>
#include <gdiplus.h>
#include <dwmapi.h>
#include <oleacc.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cwctype>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "bridge_core.hpp"
#include "audio_output.hpp"
#include "version.hpp"
#include "resource_ids.h"
#include "appearance.hpp"
#include "settings.hpp"
#include "ui_theme.hpp"
#include "update_client.hpp"

namespace {
constexpr wchar_t CLASS_NAME[] = L"JunkerMorseBridgeWindow";
constexpr int ID_PORT = 101, ID_REFRESH = 102, ID_PAUSE = 103, ID_TOP = 104, ID_CLOSE = 105;
constexpr int ID_AUDIO = 106, ID_FREQUENCY = 107, ID_SPIN = 108;
constexpr int ID_SETTINGS = 201, ID_ABOUT = 202, ID_UPDATE = 203;
constexpr int ID_THEME = 301, ID_GITHUB = 302, ID_RELEASE = 303, ID_UPDATE_STATUS = 304;
constexpr UINT WM_UPDATE_READY = WM_APP + 1;
constexpr int HOTKEY_PAUSE = 1;

// Each window owns its DPI-sized icons. Shared resource icons are reserved for
// the class fallback, since LR_SHARED can return an earlier, differently sized
// load of the same resource.
class WindowIcons {
    HICON large_ = nullptr, small_ = nullptr;
    static HICON load(int width, int height) {
        HICON icon = reinterpret_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr),
            MAKEINTRESOURCEW(IDI_MORSEBRIDGE), IMAGE_ICON, width, height, 0));
        if (!icon) icon = reinterpret_cast<HICON>(CopyImage(
            LoadIconW(nullptr, IDI_APPLICATION), IMAGE_ICON, width, height, 0));
        return icon;
    }
public:
    WindowIcons() = default;
    WindowIcons(const WindowIcons&) = delete;
    WindowIcons& operator=(const WindowIcons&) = delete;
    ~WindowIcons() { clear(); }
    void apply(HWND window, UINT dpi) {
        if (!dpi) dpi = 96;
        HICON large = load(GetSystemMetricsForDpi(SM_CXICON, dpi),
                           GetSystemMetricsForDpi(SM_CYICON, dpi));
        HICON smallIcon = load(GetSystemMetricsForDpi(SM_CXSMICON, dpi),
                           GetSystemMetricsForDpi(SM_CYSMICON, dpi));
        SendMessageW(window, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(large));
        SendMessageW(window, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(smallIcon));
        clear(); large_ = large; small_ = smallIcon;
    }
    void clear() {
        if (large_) DestroyIcon(large_);
        if (small_) DestroyIcon(small_);
        large_ = nullptr; small_ = nullptr;
    }
};

struct PortInfo { std::wstring name, label; bool ch340 = false; };
std::wstring property(HDEVINFO list, SP_DEVINFO_DATA& dev, DWORD id) {
    wchar_t buffer[2048]{};
    if (!SetupDiGetDeviceRegistryPropertyW(list, &dev, id, nullptr,
            reinterpret_cast<BYTE*>(buffer), sizeof(buffer), nullptr)) return {};
    return buffer;
}
std::vector<PortInfo> enumeratePorts() {
    std::vector<PortInfo> ports;
    HDEVINFO list = SetupDiGetClassDevsW(&GUID_DEVCLASS_PORTS, nullptr, nullptr, DIGCF_PRESENT);
    if (list == INVALID_HANDLE_VALUE) return ports;
    SP_DEVINFO_DATA dev{}; dev.cbSize = sizeof(dev);
    for (DWORD i = 0; SetupDiEnumDeviceInfo(list, i, &dev); ++i) {
        HKEY key = SetupDiOpenDevRegKey(list, &dev, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
        wchar_t port[256]{}; DWORD bytes = sizeof(port), type = 0;
        const bool valid = key != INVALID_HANDLE_VALUE &&
            RegQueryValueExW(key, L"PortName", nullptr, &type, reinterpret_cast<BYTE*>(port), &bytes) == ERROR_SUCCESS;
        if (key != INVALID_HANDLE_VALUE) RegCloseKey(key);
        if (!valid || type != REG_SZ || wcsncmp(port, L"COM", 3) != 0) continue;
        std::wstring id = property(list, dev, SPDRP_HARDWAREID);
        std::transform(id.begin(), id.end(), id.begin(), [](wchar_t c) { return wchar_t(towupper(c)); });
        const bool ch = id.find(L"VID_1A86&PID_7523") != std::wstring::npos ||
                        id.find(L"VID_1A86&PID_5523") != std::wstring::npos;
        auto label = property(list, dev, SPDRP_FRIENDLYNAME);
        if (label.empty()) label = port;
        ports.push_back({port, label, ch});
    }
    SetupDiDestroyDeviceInfoList(list);
    std::sort(ports.begin(), ports.end(), [](const PortInfo& a, const PortInfo& b) {
        return wcstol(a.name.c_str() + 3, nullptr, 10) < wcstol(b.name.c_str() + 3, nullptr, 10);
    });
    return ports;
}

// Overlapped reads allow the worker to service pause, shutdown and the watchdog
// while waiting for USB. All ownership and cancellation stay on this thread.
class SerialPort {
public:
    HANDLE handle = INVALID_HANDLE_VALUE;
    OVERLAPPED ov{};
    bool pending = false;
    char buffer[256]{};
    SerialPort() { ov.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr); }
    ~SerialPort() { close(); if (ov.hEvent) CloseHandle(ov.hEvent); }
    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;
    bool open(const std::wstring& name, DWORD& error) {
        close();
        handle = CreateFileW((L"\\\\.\\" + name).c_str(), GENERIC_READ | GENERIC_WRITE, 0,
                             nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
        if (handle == INVALID_HANDLE_VALUE) { error = GetLastError(); return false; }
        DCB dcb{}; dcb.DCBlength = sizeof(dcb);
        if (!GetCommState(handle, &dcb)) { error = GetLastError(); close(); return false; }
        dcb.BaudRate = CBR_115200; dcb.ByteSize = 8; dcb.Parity = NOPARITY; dcb.StopBits = ONESTOPBIT;
        dcb.fBinary = TRUE; dcb.fParity = FALSE; dcb.fOutxCtsFlow = FALSE; dcb.fOutxDsrFlow = FALSE;
        dcb.fDtrControl = DTR_CONTROL_ENABLE; dcb.fDsrSensitivity = FALSE;
        dcb.fTXContinueOnXoff = TRUE; dcb.fOutX = FALSE; dcb.fInX = FALSE;
        dcb.fErrorChar = FALSE; dcb.fNull = FALSE; dcb.fRtsControl = RTS_CONTROL_DISABLE;
        dcb.fAbortOnError = FALSE;
        COMMTIMEOUTS timeout{};
        timeout.ReadIntervalTimeout = MAXDWORD;
        timeout.ReadTotalTimeoutMultiplier = MAXDWORD;
        timeout.ReadTotalTimeoutConstant = 20;
        if (!ov.hEvent || !SetCommState(handle, &dcb) || !SetCommTimeouts(handle, &timeout)) {
            error = GetLastError(); close(); return false;
        }
        PurgeComm(handle, PURGE_RXCLEAR);
        return true;
    }
    // -1: device error, 0: no complete read, 1: bytes available.
    int read(std::string& bytes) {
        DWORD count = 0;
        if (pending) {
            if (!GetOverlappedResult(handle, &ov, &count, FALSE)) {
                if (GetLastError() == ERROR_IO_INCOMPLETE) return 0;
                pending = false; return -1;
            }
            pending = false;
        } else {
            ResetEvent(ov.hEvent);
            if (!ReadFile(handle, buffer, sizeof(buffer), &count, &ov)) {
                if (GetLastError() != ERROR_IO_PENDING) return -1;
                pending = true; return 0;
            }
        }
        bytes.assign(buffer, count);
        return count ? 1 : 0;
    }
    void close() {
        if (handle != INVALID_HANDLE_VALUE) {
            if (pending) {
                CancelIoEx(handle, &ov);
                DWORD ignored = 0; GetOverlappedResult(handle, &ov, &ignored, TRUE);
            }
            CloseHandle(handle);
        }
        handle = INVALID_HANDLE_VALUE; pending = false;
        if (ov.hEvent) ResetEvent(ov.hEvent);
    }
    bool isOpen() const { return handle != INVALID_HANDLE_VALUE; }
};

struct Edge { uint64_t at; bool contact, output; };
struct Snapshot {
    std::wstring status = L"Suche nach Nano ...";
    std::wstring detail = L"Automatische Suche nach CH340-Anschlüssen";
    std::wstring port;
    std::vector<PortInfo> ports;
    bool connected = false, physical = false, output = false, enabled = true;
    bool armed = false, fault = false, ownWindow = true, demo = false;
    uint64_t presses = 0, lastDuration = 0, lastPacket = 0;
    std::deque<Edge> edges;
    morse::PulseHistory pulses;
};

struct App {
    WindowIcons icons;
    HWND window = nullptr, combo = nullptr, refresh = nullptr, pause = nullptr, top = nullptr, close = nullptr;
    HWND audioCheck = nullptr, frequency = nullptr, spin = nullptr;
    HWND dialog = nullptr;
    morse::Settings settings;
    morse::ThemeId displayTheme = morse::ThemeId::System;
    std::wstring settingsFile;
    bool persistenceError = false;
    morse::ThemePalette palette = morse::paletteFor(morse::ThemeId::System, false, false);
    morse::UiTheme theme;
    morse::UpdateChecker updates{[this](morse::Version current,const std::atomic<bool>& cancel,morse::UpdateChecker::Deadline deadline) {
        if (renderTest || uiTest) {
            for (int i=0; i<16 && !cancel; ++i) Sleep(5);
            return morse::UpdateResponse{200,"{\"tag_name\":\"v1.2.3\",\"draft\":false,\"prerelease\":false}",{},0};
        }
        return morse::update_detail::fetchLatest(current,cancel,deadline);
    }};
    HBRUSH menuBrush = nullptr;
    morse::AudioOutput audio;
    HFONT font = nullptr, titleFont = nullptr, labelFont = nullptr, stateFont = nullptr;
    UINT dpi = 96;
    std::atomic<bool> stop{false}, enabled{true}, suspended{false}, locked{false};
    std::atomic<unsigned> revision{0};
    HANDLE wake = nullptr;
    std::thread worker;
    std::mutex mutex;
    std::wstring selected;
    Snapshot snapshot;
    std::vector<PortInfo> shownPorts;
    bool demo = false, renderTest = false, uiTest = false, hotkey = false;
    int scale(int x) const { return MulDiv(x, int(dpi), 96); }
    void signal() { if (wake) SetEvent(wake); }
    Snapshot readSnapshot() { std::lock_guard<std::mutex> lock(mutex); return snapshot; }
    void publish(const Snapshot& value) { std::lock_guard<std::mutex> lock(mutex); snapshot = value; }
    void shutdown() {
        stop = true; signal();
        if (worker.joinable()) worker.join();
        audio.shutdown();
        updates.shutdown();
    }
    ~App() {
        shutdown();
        if (wake) CloseHandle(wake);
        for (HFONT f : {font, titleFont, labelFont, stateFont}) if (f) DeleteObject(f);
        if (menuBrush) DeleteObject(menuBrush);
    }
} app;

#define BG (app.palette.background)
#define INK (app.palette.text)
#define MUTED (app.palette.muted)
#define GREEN (app.palette.contact)
#define BLUE (app.palette.output)
#define BORDER (app.palette.border)
#define ORANGE (app.palette.warning)
#define WHITE (app.palette.surface)

void applyAppearance(morse::ThemeId id);
void persistSettings();

bool sendSpace(bool down) {
    INPUT input{}; input.type = INPUT_KEYBOARD;
    input.ki.wScan = 0x39; // PC scan code: Space, independent of keyboard layout.
    input.ki.dwFlags = KEYEVENTF_SCANCODE | (down ? 0 : KEYEVENTF_KEYUP);
    input.ki.dwExtraInfo = 0x4D4F5253;
    return SendInput(1, &input, sizeof(input)) == 1;
}

void workerMain() {
    SerialPort serial;
    morse::FrameParser parser;
    morse::Bridge bridge([](bool down) { return app.demo || sendSpace(down); });
    Snapshot state; state.demo = app.demo;
    std::vector<PortInfo> candidates;
    size_t candidate = 0;
    unsigned revision = app.revision.load();
    uint64_t nextScan = 0, openedAt = 0, pressAt = 0;
    uint64_t lastPublish = 0, lastDemoFrame = 0;
    bool accepted = false, lastPhysical = false, lastOutput = false;
    HWND previousForeground = nullptr;
    auto disconnect = [&] {
        app.audio.setContact(false);
        state.pulses.interrupt();
        bridge.disconnect(); serial.close(); parser.reset(); accepted = false;
        state.port.clear();
    };
    auto buildCandidates = [&] {
        state.ports = enumeratePorts();
        std::wstring selected;
        { std::lock_guard<std::mutex> lock(app.mutex); selected = app.selected; }
        candidates.clear(); candidate = 0;
        for (const auto& port : state.ports) {
            if ((!selected.empty() && port.name == selected) || (selected.empty() && port.ch340))
                candidates.push_back(port);
        }
        if (candidates.empty()) {
            state.status = L"Kein Nano gefunden";
            state.detail = selected.empty() ? L"Nano per USB anschließen · CH340-Treiber und Datenkabel prüfen."
                                          : selected + L" ist nicht verfügbar. Warte auf Wiederverbindung ...";
        }
    };
    while (!app.stop) {
        const uint64_t now = GetTickCount64();
        app.audio.setSuspended(app.suspended || app.locked);
        const HWND foreground = GetForegroundWindow();
        DWORD foregroundPid = 0; GetWindowThreadProcessId(foreground, &foregroundPid);
        const bool ownWindow = foregroundPid == GetCurrentProcessId();
        if (foreground != previousForeground && bridge.outputDown) bridge.block(true);
        previousForeground = foreground;
        bridge.block(!app.demo && (ownWindow || !foreground || app.suspended || app.locked));
        bridge.enable(app.enabled);
        state.ownWindow = ownWindow;
        if (revision != app.revision.load()) {
            revision = app.revision.load(); disconnect(); candidates.clear(); candidate = 0; nextScan = 0;
        }
        if (app.suspended || app.locked) {
            if (serial.isOpen() || bridge.connected) disconnect();
            state.status = L"Verbindung angehalten";
            state.detail = L"Nach Entsperren / Aufwecken wird der Nano erneut gesucht.";
            candidates.clear(); nextScan = now + 500;
        } else if (app.demo) {
            state.status = L"Demo · kein Nano erforderlich";
            state.detail = L"Simuliertes SOS · in diesem Modus werden keine Tastendrücke gesendet.";
            state.port = L"DEMO";
            const uint64_t phase = now % 6000;
            const bool down = (phase < 120) || (phase >= 240 && phase < 360) ||
                (phase >= 480 && phase < 600) || (phase >= 960 && phase < 1320) ||
                (phase >= 1440 && phase < 1800) || (phase >= 1920 && phase < 2280) ||
                (phase >= 2640 && phase < 2760) || (phase >= 2880 && phase < 3000) ||
                (phase >= 3120 && phase < 3240);
            if (down != bridge.physicalDown || now - lastDemoFrame >= 100) {
                state.pulses.receive(down, now);
                bridge.receive(down, now); lastDemoFrame = now;
                app.audio.setContact(bridge.physicalDown);
            }
        } else {
            if (accepted && now - bridge.lastSeen >= morse::Bridge::timeoutMs) {
                disconnect(); state.status = L"Verbindung verloren";
                state.detail = L"Keine gültige Meldung seit 1 s. Space freigegeben; suche erneut ...";
                nextScan = now + 1000;
            }
            if (!serial.isOpen() && now >= nextScan) {
                if (candidate >= candidates.size()) buildCandidates();
                if (!candidates.empty()) {
                    const auto port = candidates[candidate++];
                    DWORD error = 0;
                    if (serial.open(port.name, error)) {
                        state.port = port.name; state.status = port.name + L" · prüfe Firmware ...";
                        state.detail = L"Warte auf JunkerSpace-Kennung (Nano kann beim Öffnen neu starten).";
                        openedAt = now; accepted = false; parser.reset();
                    } else {
                        state.status = port.name + L" konnte nicht geöffnet werden";
                        state.detail = L"Arduino-Seriellmonitor / andere COM-Programme schließen. Fehler " + std::to_wstring(error);
                        nextScan = now + (candidate < candidates.size() ? 100 : 2500);
                    }
                } else nextScan = now + 2500;
            }
            if (serial.isOpen()) {
                for (int batch = 0; batch < 16; ++batch) {
                    std::string bytes;
                    const int result = serial.read(bytes);
                    if (result < 0) {
                        disconnect(); state.status = L"USB-Verbindung unterbrochen";
                        state.detail = L"Space freigegeben. Automatische Wiederverbindung läuft ...";
                        nextScan = now + 1000; break;
                    }
                    if (result == 0) break;
                    parser.feed(bytes.data(), bytes.size(), [&](bool down) {
                        state.pulses.receive(down, GetTickCount64());
                        bridge.receive(down, GetTickCount64());
                        app.audio.setContact(bridge.physicalDown);
                        accepted = true;
                        state.status = L"Nano verbunden · " + state.port;
                        state.detail = L"JunkerSpace v1 · 115200 Baud · D2 ↔ GND";
                        // Record EVERY transition, even D and U in the same USB read.
                        const uint64_t at = GetTickCount64();
                        if (bridge.physicalDown != lastPhysical || bridge.outputDown != lastOutput) {
                            if (bridge.physicalDown && !lastPhysical) { ++state.presses; pressAt = at; }
                            if (!bridge.physicalDown && lastPhysical) state.lastDuration = at - pressAt;
                            state.edges.push_back({at, bridge.physicalDown, bridge.outputDown});
                            lastPhysical = bridge.physicalDown; lastOutput = bridge.outputDown;
                        }
                    });
                }
                if (serial.isOpen() && !accepted && now - openedAt >= 4500) {
                    const std::wstring port = state.port; disconnect();
                    state.status = port + L" · keine JunkerSpace-Firmware";
                    state.detail = L"Sketch JunkerSpace auf den Nano laden. Suche wird wiederholt.";
                    nextScan = now + (candidate < candidates.size() ? 100 : 2500);
                }
            }
        }
        bridge.tick(GetTickCount64());
        app.audio.setContact(bridge.connected && bridge.physicalDown);
        if (!bridge.connected) state.pulses.interrupt();
        state.pulses.prune(GetTickCount64());
        if (bridge.physicalDown != lastPhysical || bridge.outputDown != lastOutput) {
            if (bridge.physicalDown && !lastPhysical) { ++state.presses; pressAt = now; }
            if (!bridge.physicalDown && lastPhysical) state.lastDuration = now - pressAt;
            state.edges.push_back({now, bridge.physicalDown, bridge.outputDown});
            lastPhysical = bridge.physicalDown; lastOutput = bridge.outputDown;
        }
        while (state.edges.size() > 1 && state.edges[1].at + 10000 < now) state.edges.pop_front();
        while (state.edges.size() > 2048) state.edges.pop_front();
        state.connected = bridge.connected; state.physical = bridge.physicalDown;
        state.output = bridge.outputDown; state.enabled = bridge.enabled; state.armed = bridge.armed;
        state.fault = bridge.fault; state.lastPacket = bridge.lastSeen;
        if (now - lastPublish >= 16) { app.publish(state); lastPublish = now; }
        HANDLE events[2] = {app.wake, serial.ov.hEvent};
        WaitForMultipleObjects(serial.pending ? 2 : 1, events, FALSE, serial.isOpen() && !serial.pending ? 1 : 10);
    }
    disconnect();
    // In ordinary operation this succeeds on the first call; retry a rejected
    // key-up briefly. No unconditional key-up is ever sent for someone else's key.
    for (int i = 0; bridge.outputDown && i < 10; ++i) { Sleep(10); bridge.release(); }
}

void createFonts() {
    for (HFONT f : {app.font, app.titleFont, app.labelFont, app.stateFont}) if (f) DeleteObject(f);
    auto make = [](int height, int weight) {
        return CreateFontW(-app.scale(height), 0, 0, 0, weight, FALSE, FALSE, FALSE,
                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                          DEFAULT_PITCH, L"Segoe UI");
    };
    app.font = make(14, FW_NORMAL); app.titleFont = make(29, FW_SEMIBOLD);
    app.labelFont = make(12, FW_SEMIBOLD); app.stateFont = make(25, FW_SEMIBOLD);
    for (HWND control : {app.combo, app.refresh, app.pause, app.top, app.close, app.audioCheck, app.frequency})
        if (control) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(app.font), TRUE);
}
void layout() {
    RECT rc{}; GetClientRect(app.window, &rc);
    const int w = MulDiv(rc.right, 96, int(app.dpi)), h = MulDiv(rc.bottom, 96, int(app.dpi));
    auto move = [](HWND control, int x, int y, int width, int height) {
        MoveWindow(control, app.scale(x), app.scale(y), app.scale(width), app.scale(height), TRUE);
    };
    move(app.combo, 42, 183, w - 230, 220);
    move(app.refresh, w - 174, 182, 132, 28);
    move(app.pause, 26, h - 130, 180, 36);
    move(app.top, 226, h - 125, 260, 28);
    move(app.close, w - 142, h - 130, 116, 36);
    move(app.audioCheck, w - 307, 26, 140, 28);
    move(app.frequency, w - 160, 26, 66, 28);
    move(app.spin, w - 94, 26, 18, 28);
}

void fillRect(HDC dc, RECT rect, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color); FillRect(dc, &rect, brush); DeleteObject(brush);
}
void drawText(HDC dc, const std::wstring& text, RECT r, HFONT font, COLORREF color,
              UINT format = DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS) {
    HGDIOBJ old = SelectObject(dc, font); SetTextColor(dc, color); SetBkMode(dc, TRANSPARENT);
    DrawTextW(dc, text.c_str(), int(text.size()), &r, format | DT_NOPREFIX); SelectObject(dc, old);
}
void drawUi(HDC dc, int width, int height) {
    const auto s = app.readSnapshot();
    const int w = MulDiv(width, 96, int(app.dpi)), h = MulDiv(height, 96, int(app.dpi));
    auto rect = [](int x, int y, int rw, int rh) -> RECT {
        return {app.scale(x), app.scale(y), app.scale(x + rw), app.scale(y + rh)};
    };
    auto text = [&](const std::wstring& str, int x, int y, int rw, int rh, HFONT font, COLORREF color) {
        drawText(dc, str, rect(x,y,rw,rh), font, color);
    };
    auto card = [&](int x, int y, int rw, int rh, COLORREF bg) {
        HBRUSH brush = CreateSolidBrush(bg); HPEN pen = CreatePen(PS_SOLID, 1, BORDER);
        HGDIOBJ ob = SelectObject(dc, brush), op = SelectObject(dc, pen);
        RoundRect(dc, app.scale(x), app.scale(y), app.scale(x+rw), app.scale(y+rh), app.scale(14), app.scale(14));
        SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(brush); DeleteObject(pen);
    };
    fillRect(dc, {0,0,width,height}, BG);
    text(L"MorseBridge", 26, 19, w-355, 38, app.titleFont, INK);
    text(L"Hz", w-68, 26, 34, 28, app.font, MUTED);
    text(s.demo ? L"DEMO · Vorschau ohne Tastaturausgabe" : L"Junker M.T.  →  Arduino Nano  →  Leertaste", 28, 59, w-56, 24, app.font, MUTED);
    card(26, 96, w-52, 126, WHITE);
    text(L"VERBINDUNG", 42, 106, 150, 20, app.labelFont, MUTED);
    text(s.status, 42, 127, w-84, 29, app.stateFont, s.connected ? GREEN : INK);
    text(s.detail, 42, 158, w-84, 22, app.font, MUTED);
    const int cw = (w-68)/2, right = 42 + cw;
    card(26, 238, cw, 110, s.physical ? app.palette.contactActive : WHITE);
    card(right, 238, cw, 110, s.output ? app.palette.outputActive : WHITE);
    text(L"MORSETASTE · KONTAKT", 42, 251, cw-32, 20, app.labelFont, MUTED);
    text(!s.connected ? L"—" : s.physical ? L"Gedrückt" : L"Losgelassen", 42, 276, cw-32, 33, app.stateFont, s.physical ? GREEN : INK);
    text(std::to_wstring(s.presses) + L" Betätigungen · zuletzt " + std::to_wstring(s.lastDuration) + L" ms", 42, 316, cw-32, 20, app.font, MUTED);
    text(s.demo ? L"SPACE · SIMULATION" : L"SPACE · WINDOWS-AUSGABE", right+16, 251, cw-32, 20, app.labelFont, MUTED);
    text(s.output ? L"Gehalten" : L"Freigegeben", right+16, 276, cw-32, 33, app.stateFont, s.output ? BLUE : INK);
    std::wstring hint = L"Bereit für das aktive Zielfenster";
    if (s.fault) hint = L"Eingabe blockiert · Pause / Fortsetzen";
    else if (!s.enabled) hint = L"Ausgabe pausiert";
    else if (!s.connected) hint = L"Warte auf Nano";
    else if (s.ownWindow && !s.demo) hint = L"Zum Senden das Zielfenster aktivieren";
    else if (!s.armed) hint = L"Morsetaste einmal loslassen";
    text(hint, right+16, 316, cw-32, 20, app.font, s.fault ? ORANGE : MUTED);

    const int graphBottom = 504;
    card(26, 364, w-52, graphBottom-364, WHITE);
    text(L"LIVE-VERLAUF", 42, 375, 150, 22, app.labelFont, MUTED);
    text(L"Letzte 8 Sekunden", w-208, 375, 166, 22, app.font, MUTED);
    const int x0 = 118, x1 = w-44, y0 = 413, y1 = graphBottom-22;
    const uint64_t now = GetTickCount64(), start = now > 8000 ? now-8000 : 0;
    for (int i = 0; i <= 8; ++i) {
        int x = x0 + (x1-x0)*i/8;
        fillRect(dc, rect(x, y0-8, 1, y1-y0+15), BORDER);
    }
    text(L"Kontakt", 42, y0-10, 70, 20, app.font, MUTED);
    text(L"Space", 42, y1-10, 70, 20, app.font, MUTED);
    auto drawLane = [&](bool output, int y, COLORREF color) {
        fillRect(dc, rect(x0,y+7,x1-x0,1), BORDER);
        bool down = false;
        uint64_t at = start;
        auto segment = [&](uint64_t end) {
            if (down && end > at) {
                const int left = x0 + int((std::min(at,now)-start)*uint64_t(x1-x0)/8000);
                const int edge = x0 + int((std::min(end,now)-start)*uint64_t(x1-x0)/8000);
                fillRect(dc, rect(left,y-7,std::max(1,edge-left),14), color);
            }
        };
        for (const auto& edge : s.edges) {
            if (edge.at <= start) { down = output ? edge.output : edge.contact; continue; }
            if (edge.at > now) break;
            segment(edge.at); at = edge.at; down = output ? edge.output : edge.contact;
        }
        segment(now);
    };
    drawLane(false, y0, GREEN); drawLane(true, y1, BLUE);
    const int histTop = 520, histBottom = h-148;
    card(26, histTop, w-52, histBottom-histTop, WHITE);
    text(L"IMPULSLÄNGEN", 42, histTop+11, 180, 22, app.labelFont, MUTED);
    text(L"Letzte 5 Minuten · bis 1.000 ms", w-332, histTop+11, 290, 22, app.font, MUTED);
    const auto& samples = s.pulses.samples;
    uint64_t maximum = 0;
    for (const auto& sample : samples) maximum = std::max(maximum, sample.duration);
    // 5-ms bins for normal Morse speeds; broaden only for longer holds.
    const uint64_t binMs = std::max(uint64_t(5), (maximum/60/5+1)*5);
    const uint64_t range = std::max((uint64_t(200)+binMs-1)/binMs, maximum/binMs+1)*binMs;
    std::vector<unsigned> bins(size_t(range/binMs), 0);
    for (const auto& sample : samples) ++bins[size_t(sample.duration/binMs)];
    const unsigned peak = *std::max_element(bins.begin(), bins.end());
    const int hx0 = 76, hx1 = w-52, hy0 = histTop+61, hy1 = histBottom-51;
    for (int i=0; i<=4; ++i) {
        const int y = hy1-(hy1-hy0)*i/4;
        fillRect(dc, rect(hx0,y,hx1-hx0,1), BORDER);
    }
    text(std::to_wstring(peak), 42, hy0-10, 30, 20, app.labelFont, MUTED);
    text(L"0", 42, hy1-10, 30, 20, app.labelFont, MUTED);
    for (size_t i=0; i<bins.size(); ++i) {
        if (!bins[i]) continue;
        const int left = hx0+int(i*size_t(hx1-hx0)/bins.size());
        const int next = hx0+int((i+1)*size_t(hx1-hx0)/bins.size());
        const int barHeight = std::max(1, int(uint64_t(bins[i])*uint64_t(hy1-hy0)/peak));
        fillRect(dc, rect(left,hy1-barHeight,std::max(1,next-left-1),barHeight), GREEN);
    }
    for (int i=0; i<=4; ++i) {
        const int x = hx0+(hx1-hx0)*i/4;
        drawText(dc, std::to_wstring(range*uint64_t(i)/4), rect(x-24,hy1+4,48,20),
                 app.font, MUTED, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    }
    text(std::to_wstring(samples.size()) + L" Impulse · Klassen " + std::to_wstring(binMs) +
         L" ms · Dauer in ms (PC-Empfang)", 42, histBottom-26, w-84, 20, app.font, MUTED);
    if (samples.empty())
        text(L"Warte auf vollständige Kontaktimpulse ...", hx0+12, hy0+12, hx1-hx0-24, 24, app.font, MUTED);
    std::wstring footer = s.demo ? L"Demo aktiv: Der Verlauf ist simuliert; echte Tastaturausgabe ist ausgeschaltet."
                                 : L"Space geht an das aktive Programm – auch bei minimierter MorseBridge.";
    if (app.audio.error)
        footer = L"PC-Mithörton deaktiviert · Windows-Audiogerät prüfen (Fehler " +
                 std::to_wstring(app.audio.error.load()) + L").";
    if (app.persistenceError && !app.audio.error)
        footer = L"Einstellungen konnten nicht gespeichert werden · Änderung gilt für diese Sitzung.";
    text(footer, 28, h-82, w-56, 22, app.font, MUTED);
    text(app.persistenceError && app.audio.error ? L"Einstellungen konnten nicht gespeichert werden · Änderung gilt für diese Sitzung."
         : app.hotkey ? L"Strg + Alt + F12: Pause / Fortsetzen · Schließen beendet das Hilfsprogramm."
                    : L"Pause per Schaltfläche · Schließen beendet das Hilfsprogramm.", 28, h-56, w-56, 22, app.font, MUTED);
    text(L"v" MB_VERSION_WSTRING L" · 5 ms Entprellung · 1 s Verbindungsüberwachung", 28, h-29, w-56, 18, app.labelFont, MUTED);
}

void syncControls(const Snapshot& s) {
    Button_SetCheck(app.audioCheck, app.audio.enabled ? BST_CHECKED : BST_UNCHECKED);
    SetWindowTextW(app.pause, s.enabled ? L"Ausgabe pausieren" : L"Ausgabe fortsetzen");
    // Do not disturb an open dropdown or the user's selection.
    if (SendMessageW(app.combo, CB_GETDROPPEDSTATE, 0, 0)) return;
    bool same = s.ports.size() == app.shownPorts.size();
    if (same) for (size_t i=0; i<s.ports.size(); ++i)
        if (s.ports[i].name != app.shownPorts[i].name || s.ports[i].label != app.shownPorts[i].label) same = false;
    if (same && SendMessageW(app.combo, CB_GETCOUNT, 0, 0)) return;
    app.shownPorts = s.ports;
    SendMessageW(app.combo, CB_RESETCONTENT, 0, 0);
    SendMessageW(app.combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Automatisch (CH340)"));
    std::wstring selected;
    { std::lock_guard<std::mutex> lock(app.mutex); selected = app.selected; }
    int selection = 0;
    for (size_t i=0; i<s.ports.size(); ++i) {
        SendMessageW(app.combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s.ports[i].label.c_str()));
        if (s.ports[i].name == selected) selection = int(i)+1;
    }
    // Keep a disconnected manual port visible; do not silently switch to auto.
    if (!selected.empty() && selection == 0) {
        app.shownPorts.push_back({selected, selected + L" (nicht verbunden)", false});
        SendMessageW(app.combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(app.shownPorts.back().label.c_str()));
        selection = int(app.shownPorts.size());
    }
    SendMessageW(app.combo, CB_SETCURSEL, selection, 0);
}

void togglePause() { app.enabled = !app.enabled.load(); app.signal(); }
void readFrequency(bool normalize) {
    wchar_t value[16]{}; GetWindowTextW(app.frequency, value, 16);
    wchar_t* end = nullptr;
    const unsigned long hz = wcstoul(value, &end, 10);
    if (end != value && *end == L'\0' && hz >= morse::toneMinHz && hz <= morse::toneMaxHz)
        app.audio.frequency = unsigned(hz);
    if (normalize) {
        const auto label = std::to_wstring(app.audio.frequency.load());
        SetWindowTextW(app.frequency, label.c_str());
        app.settings.frequency = app.audio.frequency.load();
        persistSettings();
    }
}
#include "ui_shell.hpp"
LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        app.window = hwnd; app.dpi = GetDpiForWindow(hwnd);
        app.icons.apply(hwnd, app.dpi);
        auto control = [&](const wchar_t* cls, const wchar_t* text, DWORD style, int id) {
            return CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | style,
                0,0,0,0, hwnd, reinterpret_cast<HMENU>(INT_PTR(id)), GetModuleHandleW(nullptr), nullptr);
        };
        app.combo = control(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL, ID_PORT);
        app.refresh = control(WC_BUTTONW, L"Neu suchen", BS_PUSHBUTTON, ID_REFRESH);
        app.pause = control(WC_BUTTONW, L"Ausgabe pausieren", BS_PUSHBUTTON, ID_PAUSE);
        app.top = control(WC_BUTTONW, L"Immer im Vordergrund", BS_AUTOCHECKBOX, ID_TOP);
        app.close = control(WC_BUTTONW, L"Schließen", BS_PUSHBUTTON, ID_CLOSE);
        app.audioCheck = control(WC_BUTTONW, L"PC-Mithörton", BS_AUTOCHECKBOX, ID_AUDIO);
        app.frequency = control(WC_EDITW, L"650", WS_BORDER | ES_NUMBER | ES_RIGHT, ID_FREQUENCY);
        SendMessageW(app.frequency, EM_SETLIMITTEXT, 4, 0);
        app.spin = control(UPDOWN_CLASSW, L"", UDS_SETBUDDYINT | UDS_ARROWKEYS | UDS_NOTHOUSANDS, ID_SPIN);
        SendMessageW(app.spin, UDM_SETBUDDY, reinterpret_cast<WPARAM>(app.frequency), 0);
        SendMessageW(app.spin, UDM_SETRANGE32, morse::toneMinHz, morse::toneMaxHz);
        SendMessageW(app.spin, UDM_SETPOS32, 0, app.settings.frequency);
        SetWindowTextW(app.frequency,std::to_wstring(app.settings.frequency).c_str());
        app.audio.frequency=app.settings.frequency;
        for (HWND child : {app.combo,app.refresh,app.pause,app.top,app.close,app.audioCheck,app.frequency,app.spin})
            app.theme.attach(child);
        createFonts(); layout();
        applyAppearance(app.settings.theme);
        app.hotkey = !app.renderTest && RegisterHotKey(hwnd, HOTKEY_PAUSE, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, VK_F12);
        if (!app.renderTest) WTSRegisterSessionNotification(hwnd, NOTIFY_FOR_THIS_SESSION);
        SetTimer(hwnd, 1, 33, nullptr); return 0;
    }
    case WM_SIZE: layout(); return 0;
    case WM_DPICHANGED: {
        app.dpi = HIWORD(wParam); createFonts();
        app.icons.apply(hwnd, app.dpi);
        auto* r = reinterpret_cast<RECT*>(lParam);
        SetWindowPos(hwnd, nullptr, r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);
        layout(); app.theme.refresh(); DrawMenuBar(hwnd); return 0;
    }
    case WM_GETMINMAXINFO: {
        auto* limits = reinterpret_cast<MINMAXINFO*>(lParam);
        RECT r{0,0,app.scale(740),app.scale(888)};
        AdjustWindowRectExForDpi(&r, WS_OVERLAPPEDWINDOW, TRUE, 0, app.dpi);
        limits->ptMinTrackSize = {r.right-r.left, r.bottom-r.top}; return 0;
    }
    case WM_TIMER:
        syncControls(app.readSnapshot()); InvalidateRect(hwnd, nullptr, FALSE); return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_PORT:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                const int index = int(SendMessageW(app.combo, CB_GETCURSEL, 0, 0));
                { std::lock_guard<std::mutex> lock(app.mutex);
                  app.selected = index > 0 && size_t(index) <= app.shownPorts.size() ? app.shownPorts[size_t(index-1)].name : L""; }
                ++app.revision; app.signal();
            } break;
        case ID_REFRESH: ++app.revision; app.signal(); break;
        case ID_PAUSE: togglePause(); break;
        case ID_AUDIO: app.audio.setEnabled(Button_GetCheck(app.audioCheck) == BST_CHECKED); break;
        case ID_FREQUENCY:
            if (HIWORD(wParam) == EN_CHANGE) readFrequency(false);
            else if (HIWORD(wParam) == EN_KILLFOCUS) readFrequency(true);
            break;
        case ID_TOP:
            SetWindowPos(hwnd, Button_GetCheck(app.top) == BST_CHECKED ? HWND_TOPMOST : HWND_NOTOPMOST,
                         0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE); break;
        case ID_SETTINGS: readFrequency(true); openShell(DialogKind::Settings); break;
        case ID_ABOUT: readFrequency(true); openShell(DialogKind::About); break;
        case ID_UPDATE: readFrequency(true); openShell(DialogKind::Update); break;
        case ID_CLOSE: PostMessageW(hwnd, WM_CLOSE, 0, 0); break;
        } return 0;
    case WM_UPDATE_READY: refreshUpdateDialog(); return 0;
    case WM_NOTIFY: {
        LRESULT result=0;
        if (app.theme.handleNotify(lParam,result)) return result;
        break;
    }
    case WM_DRAWITEM: {
        const auto& item=*reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (drawMenuItem(item) || app.theme.handleDrawItem(item)) return TRUE;
        break;
    }
    case WM_MEASUREITEM: {
        auto& item=*reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
        if (measureMenuItem(item) || app.theme.handleMeasureItem(item)) return TRUE;
        break;
    }
    case WM_MENUCHAR: return menuMnemonic(wParam,reinterpret_cast<HMENU>(lParam));
    case WM_NCPAINT: case WM_NCACTIVATE: {
        const LRESULT result=DefWindowProcW(hwnd,message,wParam,lParam); paintMenuBar(hwnd); return result;
    }
    case WM_SETTINGCHANGE: case WM_THEMECHANGED: case WM_SYSCOLORCHANGE:
        applyAppearance(app.displayTheme); return 0;
    case WM_HOTKEY: if (wParam == HOTKEY_PAUSE) togglePause(); return 0;
    case WM_POWERBROADCAST:
        if (wParam == PBT_APMSUSPEND) { app.suspended = true; app.audio.setSuspended(true); app.signal(); }
        if (wParam == PBT_APMRESUMEAUTOMATIC || wParam == PBT_APMRESUMESUSPEND) { app.suspended = false; ++app.revision; app.signal(); }
        return TRUE;
    case WM_WTSSESSION_CHANGE:
        if (wParam == WTS_SESSION_LOCK || wParam == WTS_SESSION_LOGOFF) { app.locked = true; app.audio.setSuspended(true); app.signal(); }
        if (wParam == WTS_SESSION_UNLOCK || wParam == WTS_SESSION_LOGON) { app.locked = false; ++app.revision; app.signal(); }
        return 0;
    case WM_ERASEBKGND: return 1;
    case WM_CTLCOLORBTN: case WM_CTLCOLOREDIT: case WM_CTLCOLORSTATIC: case WM_CTLCOLORLISTBOX:
        return reinterpret_cast<LRESULT>(app.theme.colorControl(reinterpret_cast<HDC>(wParam),reinterpret_cast<HWND>(lParam),message));
    case WM_PRINTCLIENT: {
        RECT r{}; GetClientRect(hwnd, &r); drawUi(reinterpret_cast<HDC>(wParam), r.right,r.bottom); return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT paint{}; HDC target = BeginPaint(hwnd, &paint);
        RECT r{}; GetClientRect(hwnd, &r);
        HDC back = CreateCompatibleDC(target); HBITMAP bmp = CreateCompatibleBitmap(target, std::max(1L,r.right),std::max(1L,r.bottom));
        HGDIOBJ old = SelectObject(back, bmp); drawUi(back,r.right,r.bottom);
        BitBlt(target,0,0,r.right,r.bottom,back,0,0,SRCCOPY);
        SelectObject(back,old); DeleteObject(bmp); DeleteDC(back); EndPaint(hwnd,&paint); return 0;
    }
    case WM_QUERYENDSESSION: app.suspended = true; app.audio.setSuspended(true); app.signal(); return TRUE;
    case WM_ENDSESSION:
        if (wParam) app.shutdown();
        else { app.suspended = false; ++app.revision; app.signal(); }
        return 0;
    case WM_CLOSE:
        readFrequency(true); app.stop=true;
        if (app.dialog) closeShell(false);
        app.shutdown(); DestroyWindow(hwnd); return 0;
    case WM_DESTROY:
        KillTimer(hwnd,1);
        if (app.hotkey) UnregisterHotKey(hwnd,HOTKEY_PAUSE);
        if (!app.renderTest) WTSUnRegisterSessionNotification(hwnd);
        PostQuitMessage(0); return 0;
    case WM_NCDESTROY: {
        const LRESULT result = DefWindowProcW(hwnd, message, wParam, lParam);
        app.icons.clear(); return result;
    }
    }
    return DefWindowProcW(hwnd,message,wParam,lParam);
}

bool saveWindowPng(const std::wstring& path) {
    RECT rc{}; GetClientRect(app.window,&rc);
    HDC screen = GetDC(app.window), dc = CreateCompatibleDC(screen);
    HBITMAP bitmap = CreateCompatibleBitmap(screen,rc.right,rc.bottom);
    HGDIOBJ old = SelectObject(dc,bitmap);
    drawUi(dc,rc.right,rc.bottom);
    // Paint child HWNDs in CLIENT coordinates. WM_PRINT on the top-level window
    // also includes non-client offsets; PrintWindow may fail for hidden windows.
    for (HWND control : {app.combo,app.refresh,app.pause,app.top,app.close,app.audioCheck,app.frequency,app.spin}) {
        RECT child{}; GetWindowRect(control,&child);
        MapWindowPoints(HWND_DESKTOP,app.window,reinterpret_cast<POINT*>(&child),2);
        const int saved = SaveDC(dc);
        SetViewportOrgEx(dc,child.left,child.top,nullptr);
        SendMessageW(control,WM_PRINT,reinterpret_cast<WPARAM>(dc),PRF_CLIENT|PRF_CHILDREN|PRF_ERASEBKGND);
        RestoreDC(dc,saved);
    }
    SelectObject(dc,old);
    const CLSID png = {0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};
    bool ok = false;
    { Gdiplus::Bitmap image(bitmap,nullptr); ok = image.Save(path.c_str(),&png,nullptr) == Gdiplus::Ok; }
    DeleteObject(bitmap); DeleteDC(dc); ReleaseDC(app.window,screen); return ok;
}
bool saveCompleteWindowPng(HWND window,const std::wstring& path) {
    RECT bounds{}; GetWindowRect(window,&bounds);
    const int width=bounds.right-bounds.left, height=bounds.bottom-bounds.top;
    HDC screen=GetDC(window), dc=CreateCompatibleDC(screen);
    HBITMAP bitmap=CreateCompatibleBitmap(screen,width,height);
    HGDIOBJ old=SelectObject(dc,bitmap);
    fillRect(dc,{0,0,width,height},app.palette.background);
    SendMessageW(window,WM_PRINT,reinterpret_cast<WPARAM>(dc),PRF_NONCLIENT | PRF_CLIENT | PRF_CHILDREN | PRF_ERASEBKGND);
    if (window==app.window) {
        MENUBARINFO bar{}; bar.cbSize=sizeof(bar);
        if (GetMenuBarInfo(window,OBJID_MENU,0,&bar)) {
            RECT rect=bar.rcBar; OffsetRect(&rect,-bounds.left,-bounds.top); fillRect(dc,rect,app.palette.background);
            for (int i=0; i<3; ++i) {
                if (GetMenuItemRect(window,bar.hMenu,UINT(i),&rect)) {
                    OffsetRect(&rect,-bounds.left,-bounds.top); paintMenuEntry(dc,rect,menuEntries[size_t(i)],false,false,false);
                }
            }
        }
    }
    SelectObject(dc,old);
    const CLSID png={0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};
    bool ok=false;
    { Gdiplus::Bitmap value(bitmap,nullptr); ok=value.Save(path.c_str(),&png,nullptr)==Gdiplus::Ok; }
    DeleteObject(bitmap); DeleteDC(dc); ReleaseDC(window,screen); return ok;
}

int renderTests(const std::wstring& directory) {
    Gdiplus::GdiplusStartupInput input; ULONG_PTR token = 0;
    if (Gdiplus::GdiplusStartup(&token,&input,nullptr) != Gdiplus::Ok) return 2;
    CreateDirectoryW(directory.c_str(),nullptr);
    Snapshot state; state.status = L"Kein Nano gefunden";
    state.detail = L"Nano per USB anschließen · CH340-Treiber und Datenkabel prüfen.";
    app.publish(state); syncControls(state);
    wchar_t selected[128]{}; GetWindowTextW(app.combo,selected,128);
    if (wcscmp(selected,L"Automatisch (CH340)") != 0) {
        Gdiplus::GdiplusShutdown(token); DestroyWindow(app.window); return 4;
    }
    bool ok = saveWindowPng(directory+L"\\gui-suche.png");
    ok = !app.audio.enabled && app.audio.frequency == 650 && ok;
    for (const unsigned hz : {400u, 650u, 1000u}) {
        SetWindowTextW(app.frequency, std::to_wstring(hz).c_str());
        ok = app.audio.frequency == hz && ok;
    }
    for (const wchar_t* invalid : {L"399", L"1001", L"", L"abc"}) {
        SetWindowTextW(app.frequency, invalid);
        SendMessageW(app.window, WM_COMMAND, MAKEWPARAM(ID_FREQUENCY, EN_KILLFOCUS), 0);
        GetWindowTextW(app.frequency, selected, 128);
        ok = app.audio.frequency == 1000 && wcscmp(selected, L"1000") == 0 && ok;
    }
    SetWindowTextW(app.frequency, L"650");
    Button_SetCheck(app.audioCheck, BST_CHECKED);
    SendMessageW(app.window, WM_COMMAND, ID_AUDIO, 0);
    ok = app.audio.enabled && ok;
    state.demo = true; state.connected = true; state.physical = true; state.output = true; state.armed = true;
    state.status = L"Demo · Nano verbunden · COM5"; state.detail = L"Vorschau · keine echte Tastaturausgabe";
    state.port = L"COM5"; state.presses = 12; state.lastDuration = 124;
    const uint64_t now = GetTickCount64();
    for (int i=0; i<12; ++i) {
        const uint64_t at = now-7800+uint64_t(i)*650;
        state.edges.push_back({at,true,true});
        state.edges.push_back({at+uint64_t(i%3 == 0 ? 360 : 120),false,false});
    }
    state.edges.push_back({now-220,true,true});
    state.pulses.receive(false, now-60000);
    for (int i=0; i<120; ++i) {
        const uint64_t at = now-59000+uint64_t(i)*400;
        state.pulses.receive(true,at);
        state.pulses.receive(false,at+uint64_t(i%3 == 0 ? 145+i%4*5 : 42+i%4*5));
    }
    app.publish(state); syncControls(state); ok = saveWindowPng(directory+L"\\gui-demo.png") && ok;
    state.enabled = false; state.output = false; state.edges.push_back({now,false,false});
    app.publish(state); syncControls(state); ok = saveWindowPng(directory+L"\\gui-pause.png") && ok;
    Button_SetCheck(app.audioCheck, BST_UNCHECKED);
    SendMessageW(app.window, WM_COMMAND, ID_AUDIO, 0);
    ok = !app.audio.enabled && ok;
    ok = Button_GetCheck(app.top)==BST_UNCHECKED && app.selected.empty() && ok;
    // Deterministic UI tests never read/write the user's profile or access HTTP.
    // Exercise palette changes, scaled geometry and real owner/child dialogs.
    for (const UINT dpi : {96u,144u,192u}) {
        RECT bounds{0,0,MulDiv(800,int(dpi),96),MulDiv(908,int(dpi),96)};
        AdjustWindowRectExForDpi(&bounds,WS_OVERLAPPEDWINDOW,TRUE,0,dpi);
        SendMessageW(app.window,WM_DPICHANGED,MAKEWPARAM(dpi,dpi),reinterpret_cast<LPARAM>(&bounds));
        for (const auto id : {morse::ThemeId::System,morse::ThemeId::Light,morse::ThemeId::Dark,
                              morse::ThemeId::Midnight,morse::ThemeId::Amber,morse::ThemeId::Matrix}) {
            const unsigned revision=app.revision.load(); const unsigned frequency=app.audio.frequency.load();
            applyAppearance(id);
            const std::wstring name(morse::themeIdName(id),morse::themeIdName(id)+strlen(morse::themeIdName(id)));
            ok=saveWindowPng(directory+L"\\theme-"+name+L"-"+std::to_wstring(dpi)+L".png") && ok;
            ok=app.revision==revision && app.audio.frequency==frequency && !app.audio.enabled && ok;
        }
    }
    RECT bounds{0,0,800,908}; AdjustWindowRectExForDpi(&bounds,WS_OVERLAPPEDWINDOW,TRUE,0,96);
    SendMessageW(app.window,WM_DPICHANGED,MAKEWPARAM(96,96),reinterpret_cast<LPARAM>(&bounds));
    applyAppearance(morse::ThemeId::Matrix);
    ok=saveCompleteWindowPng(app.window,directory+L"\\menu-matrix.png") && ok;
    for (const auto id : {morse::ThemeId::Light,morse::ThemeId::Dark,morse::ThemeId::Midnight,morse::ThemeId::Amber,morse::ThemeId::Matrix}) {
        applyAppearance(id);
        const std::wstring name(morse::themeIdName(id),morse::themeIdName(id)+strlen(morse::themeIdName(id)));
        openShell(DialogKind::Settings);
        ok=saveCompleteWindowPng(app.dialog,directory+L"\\settings-"+name+L".png") && ok;
        SendMessageW(shell.choice,CB_SETCURSEL,WPARAM(morse::ThemeId::Light),0);
        SendMessageW(app.dialog,WM_COMMAND,MAKEWPARAM(ID_THEME,CBN_SELCHANGE),reinterpret_cast<LPARAM>(shell.choice));
        ok=app.displayTheme==morse::ThemeId::Light && ok;
        SendMessageW(app.dialog,WM_COMMAND,IDCANCEL,0);
        ok=!app.dialog && app.displayTheme==id && IsWindowEnabled(app.window) && ok;
        openShell(DialogKind::About);
        ok=saveCompleteWindowPng(app.dialog,directory+L"\\about-"+name+L".png") && ok;
        closeShell(false);
    }
    openShell(DialogKind::Settings);
    applyAppearance(morse::ThemeId::Midnight);
    SendMessageW(app.dialog,WM_COMMAND,IDOK,0);
    ok=app.settings.theme==morse::ThemeId::Midnight && !app.dialog && ok;
    openShell(DialogKind::Update);
    ok=saveCompleteWindowPng(app.dialog,directory+L"\\update-checking.png") && ok;
    const uint64_t updateDeadline=GetTickCount64()+1000;
    while (app.updates.running() && GetTickCount64()<updateDeadline) Sleep(5);
    MSG updateMessage{};
    while (PeekMessageW(&updateMessage,app.window,WM_UPDATE_READY,WM_UPDATE_READY,PM_REMOVE))
        DispatchMessageW(&updateMessage);
    ok=app.updates.result().state==morse::UpdateState::Available && IsWindowEnabled(shell.release) && ok;
    ok=saveCompleteWindowPng(app.dialog,directory+L"\\update-available.png") && ok;
    closeShell(false);
    openShell(DialogKind::Update); closeShell(false);
    ok=(GetMenuState(GetMenu(app.window),ID_UPDATE,MF_BYCOMMAND) & MF_GRAYED)==0 && ok;
    app.settings.theme=morse::ThemeId::System; applyAppearance(app.settings.theme);
    Gdiplus::GdiplusShutdown(token);
    // Smoke-test the real background thread and the actual window commands.
    // Demo mode guarantees that no serial port or system keyboard is touched.
    app.demo = true;
    app.wake = CreateEventW(nullptr,FALSE,FALSE,nullptr);
    if (!app.wake) { DestroyWindow(app.window); return 5; }
    app.worker = std::thread(workerMain);
    app.audio.start();
    auto awaitState = [](bool enabled) {
        const uint64_t deadline = GetTickCount64()+1000;
        while (GetTickCount64()<deadline) {
            const auto value = app.readSnapshot();
            if (value.connected && value.demo && value.enabled == enabled) return true;
            Sleep(10);
        }
        return false;
    };
    ok = awaitState(true) && ok;
    SendMessageW(app.window,WM_COMMAND,ID_PAUSE,0);
    ok = awaitState(false) && ok;
    const uint64_t contactDeadline = GetTickCount64()+6500;
    bool contactWhilePaused = false;
    while (GetTickCount64()<contactDeadline) {
        const auto value = app.readSnapshot();
        if (!value.enabled && value.physical && app.audio.contact && !value.output) {
            contactWhilePaused = true; break;
        }
        Sleep(10);
    }
    ok = contactWhilePaused && ok;
    SendMessageW(app.window,WM_COMMAND,ID_PAUSE,0);
    ok = awaitState(true) && ok;
    SendMessageW(app.window,WM_COMMAND,ID_CLOSE,0);
    MSG message{};
    while (PeekMessageW(&message,nullptr,0,0,PM_REMOVE)) {
        if (message.message != WM_QUIT) { TranslateMessage(&message); DispatchMessageW(&message); }
    }
    ok = !IsWindow(app.window) && !app.worker.joinable() && ok;
    ok = !app.audio.contact && !app.audio.active && ok;
    app.shutdown();
    return ok ? 0 : 3;
}
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    int argc = 0; LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::wstring renderDirectory;
    for (int i=1; i<argc; ++i) {
        if (wcscmp(argv[i],L"--demo") == 0) app.demo = true;
        else if (wcscmp(argv[i],L"--render-test") == 0 && i+1<argc) { app.renderTest = true; renderDirectory = argv[++i]; }
        else if (wcscmp(argv[i],L"--ui-test") == 0 && i+1<argc) {
            app.uiTest=true; app.demo=true; app.settingsFile=argv[++i];
        }
        else { LocalFree(argv); return 64; }
    }
    LocalFree(argv);
    if (!app.renderTest) {
        if (!app.uiTest) app.settingsFile=morse::settingsPath();
        app.settings=morse::loadSettings(app.settingsFile);
    }
    app.audio.frequency=app.settings.frequency;
    HANDLE instanceLock = nullptr;
    if (!app.renderTest && !app.demo) {
        instanceLock = CreateMutexW(nullptr, TRUE, L"Local\\JunkerMorseBridge-v1");
        if (instanceLock && GetLastError() == ERROR_ALREADY_EXISTS) {
            HWND existing = FindWindowW(CLASS_NAME, nullptr);
            if (existing) { ShowWindow(existing,SW_RESTORE); SetForegroundWindow(existing); }
            CloseHandle(instanceLock); return 0;
        }
    }
    INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_STANDARD_CLASSES | ICC_UPDOWN_CLASS | ICC_LINK_CLASS}; InitCommonControlsEx(&controls);
    WNDCLASSEXW cls{}; cls.cbSize = sizeof(cls); cls.lpfnWndProc = windowProc; cls.hInstance = instance;
    cls.hCursor = LoadCursorW(nullptr,IDC_ARROW);
    cls.hIcon = LoadIconW(instance,MAKEINTRESOURCEW(IDI_MORSEBRIDGE));
    if (!cls.hIcon) cls.hIcon = LoadIconW(nullptr,IDI_APPLICATION);
    cls.lpszClassName = CLASS_NAME;
    if (!RegisterClassExW(&cls)) { if (instanceLock) CloseHandle(instanceLock); return 1; }
    WNDCLASSEXW dialogClass=cls;
    dialogClass.lpfnWndProc=shellProc; dialogClass.lpszClassName=SHELL_CLASS_NAME;
    if (!RegisterClassExW(&dialogClass)) { if (instanceLock) CloseHandle(instanceLock); return 1; }
    app.dpi = GetDpiForSystem();
    RECT size{0,0,app.scale(800),app.scale(908)};
    AdjustWindowRectExForDpi(&size,WS_OVERLAPPEDWINDOW,TRUE,0,app.dpi);
    HWND window = CreateWindowExW(0,CLASS_NAME,app.demo ? L"MorseBridge – Demo (ohne Tastaturausgabe)" : L"MorseBridge – Junker M.T.",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT,CW_USEDEFAULT,size.right-size.left,size.bottom-size.top,
        nullptr,makeMenu(),instance,nullptr);
    if (!window) { if (instanceLock) CloseHandle(instanceLock); return 1; }
    if (app.renderTest) return renderTests(renderDirectory);
    app.wake = CreateEventW(nullptr,FALSE,FALSE,nullptr);
    if (!app.wake) { DestroyWindow(window); if (instanceLock) CloseHandle(instanceLock); return 1; }
    app.worker = std::thread(workerMain);
    app.audio.start();
    ShowWindow(window,show); UpdateWindow(window);
    MSG message{};
    while (GetMessageW(&message,nullptr,0,0) > 0) {
        if (message.message==WM_KEYDOWN && message.wParam==VK_RETURN && GetFocus()==app.frequency) {
            readFrequency(true); continue;
        }
        if (!(app.dialog && IsDialogMessageW(app.dialog,&message)) && !IsDialogMessageW(window,&message)) {
            TranslateMessage(&message); DispatchMessageW(&message);
        }
    }
    app.shutdown();
    if (instanceLock) CloseHandle(instanceLock);
    return int(message.wParam);
}

// SPDX-License-Identifier: GPL-3.0-only
// LLM-assisted implementation; see LICENSE and DISCLAIMER.md.
#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <functional>
#include <utility>

namespace morse {
// Remember the scan code accepted by Windows until its matching key-up succeeds.
// Neither a layout change nor a new selected key may change that release event.
class KeyboardOutput {
public:
    using Emit = std::function<bool(const INPUT&)>;
    using Map = std::function<unsigned(unsigned)>;
    explicit KeyboardOutput(Emit emit = {}, Map map = {})
        : emit_(emit ? std::move(emit) : Emit([](const INPUT& input) {
              INPUT event = input;
              return SendInput(1, &event, sizeof(event)) == 1;
          })),
          map_(map ? std::move(map) : Map([](unsigned vk) {
              const HWND foreground = GetForegroundWindow();
              const DWORD thread = foreground ? GetWindowThreadProcessId(foreground, nullptr) : 0;
              return static_cast<unsigned>(MapVirtualKeyExW(vk, MAPVK_VK_TO_VSC_EX,
                                                            GetKeyboardLayout(thread)));
          })) {}

    bool send(unsigned vk, bool down) {
        if (down && owned_) return false;
        if (!down && !owned_) return true;

        WORD scan = scan_;
        DWORD flags = flags_;
        if (down) {
            if (vk == 0 || vk > 0xff) return false;
            const unsigned mapped = map_(vk);
            const unsigned prefix = mapped >> 8;
            // SendInput represents E0 keys with EXTENDEDKEY. E1 sequences need
            // more than this single scan-code event and are intentionally rejected.
            if ((mapped & 0xff) == 0 || (prefix != 0 && prefix != 0xe0)) return false;
            scan = static_cast<WORD>(mapped & 0xff);
            flags = KEYEVENTF_SCANCODE | (prefix == 0xe0 ? KEYEVENTF_EXTENDEDKEY : 0);
        }

        INPUT input{};
        input.type = INPUT_KEYBOARD;
        input.ki.wScan = scan;
        input.ki.dwFlags = flags | (down ? 0 : KEYEVENTF_KEYUP);
        input.ki.dwExtraInfo = 0x4D4F5253;
        if (!emit_(input)) return false;
        owned_ = down;
        if (down) { scan_ = scan; flags_ = flags; }
        return true;
    }

private:
    Emit emit_;
    Map map_;
    bool owned_ = false;
    WORD scan_ = 0;
    DWORD flags_ = 0;
};
} // namespace morse
#endif

// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cstdint>
#include <string>
#include <vector>
constexpr uint8_t LOW=0, HIGH=1, INPUT_PULLUP=2, OUTPUT=3, LED_BUILTIN=13;
#define F(x) x
inline uint32_t fakeNow=0;
inline bool fakeContact=false;
inline bool fakeBuzzerSwitchClosed=false;
inline uint8_t fakeLed=LOW, fakeKeyMode=0;
inline uint8_t fakeBuzzer=LOW, fakeBuzzerMode=0, fakeBuzzerSwitchMode=0;
inline uint32_t millis() { return fakeNow; }
inline int digitalRead(uint8_t pin) {
    if (pin==2) return fakeContact ? LOW : HIGH;
    if (pin==4) return fakeBuzzerSwitchClosed ? LOW : HIGH;
    return HIGH;
}
inline void digitalWrite(uint8_t pin,uint8_t value) {
    if (pin==LED_BUILTIN) fakeLed=value;
    if (pin==8) fakeBuzzer=value;
}
inline void pinMode(uint8_t pin,uint8_t value) {
    if (pin==2) fakeKeyMode=value;
    if (pin==4) fakeBuzzerSwitchMode=value;
    if (pin==8) fakeBuzzerMode=value;
}
struct MockSerial {
    uint32_t baud=0;
    std::vector<std::string> lines;
    void begin(uint32_t value) { baud=value; }
    void println(const char* value) { lines.emplace_back(value); }
};
inline MockSerial Serial;

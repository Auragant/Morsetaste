// SPDX-License-Identifier: GPL-3.0-only
// LLM-assisted implementation; see LICENSE and DISCLAIMER.md.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace morse {
constexpr unsigned toneMinHz = 400, toneMaxHz = 1000, toneDefaultHz = 650;
inline unsigned clampToneHz(unsigned value) {
    return std::clamp(value, toneMinHz, toneMaxHz);
}

// Continuous-phase mono PCM sine with a 3-ms envelope to soften contact edges.
class ToneGenerator {
public:
    static constexpr unsigned sampleRate = 48000;
    static constexpr int amplitude = 6000;
    void render(int16_t* samples, size_t count, bool down, unsigned hz) {
        constexpr double tau = 6.2831853071795864769;
        constexpr double ramp = 1.0 / (sampleRate * 0.003);
        const double step = tau * clampToneHz(hz) / sampleRate;
        for (size_t i = 0; i < count; ++i) {
            gain_ = down ? std::min(1.0, gain_ + ramp) : std::max(0.0, gain_ - ramp);
            samples[i] = static_cast<int16_t>(std::lround(std::sin(phase_) * amplitude * gain_));
            phase_ += step;
            if (phase_ >= tau) phase_ -= tau;
        }
    }
private:
    double phase_ = 0, gain_ = 0;
};
} // namespace morse

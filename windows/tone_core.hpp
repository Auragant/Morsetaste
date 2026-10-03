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

// Continuous-phase sine at the device rate, with a 3-ms cosine envelope.
class ToneGenerator {
public:
    static constexpr unsigned sampleRate = 48000;
    static constexpr int amplitude = 6000;
    explicit ToneGenerator(unsigned rate = sampleRate) : rate_(rate ? rate : sampleRate) {}
    float nextSample(bool down, unsigned hz) {
        constexpr double tau = 6.2831853071795864769;
        const double ramp = 1.0 / (rate_ * 0.003);
        envelope_ = down ? std::min(1.0, envelope_ + ramp) : std::max(0.0, envelope_ - ramp);
        const double gain = 0.5 - 0.5 * std::cos(envelope_ * 3.14159265358979323846);
        const float sample = gain == 0 ? 0.0f : static_cast<float>(std::sin(phase_) * (amplitude / 32768.0) * gain);
        phase_ += tau * clampToneHz(hz) / rate_;
        if (phase_ >= tau) phase_ -= tau;
        return sample;
    }
    void render(int16_t* samples, size_t count, bool down, unsigned hz) {
        for (size_t i = 0; i < count; ++i)
            samples[i] = static_cast<int16_t>(std::lround(nextSample(down, hz) * 32768.0f));
    }
private:
    unsigned rate_;
    double phase_ = 0, envelope_ = 0;
};
} // namespace morse

// SPDX-License-Identifier: GPL-3.0-only
#include "../windows/tone_core.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {
int checks = 0;
void require(bool condition, const char* message) {
    ++checks;
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
}
int main() {
    using morse::ToneGenerator;
    require(morse::toneDefaultHz == 650, "default pitch 650Hz");
    require(morse::clampToneHz(0) == 400 && morse::clampToneHz(2000) == 1000,
            "pitch limited to 400-1000Hz");
    for (unsigned hz : {400u, 650u, 1000u}) {
        ToneGenerator tone;
        std::vector<int16_t> samples(ToneGenerator::sampleRate);
        tone.render(samples.data(), samples.size(), true, hz);
        int crossings = 0;
        double energy = 0, error = 0;
        for (size_t i=480; i<samples.size(); ++i) {
            if (samples[i-1] <= 0 && samples[i] > 0) ++crossings;
            energy += double(samples[i]) * samples[i];
            const double expected = std::sin(6.2831853071795864769 * hz * double(i) /
                                             ToneGenerator::sampleRate) * ToneGenerator::amplitude;
            error = std::max(error, std::abs(samples[i] - expected));
        }
        require(std::abs(crossings - int(hz * 0.99)) <= 1, "measured frequency matches selected Hz");
        require(error < 0.51, "PCM follows a sine, not a square wave");
        const double rms = std::sqrt(energy / double(samples.size()-480));
        require(std::abs(rms - ToneGenerator::amplitude / std::sqrt(2.0)) < 2,
                "sine amplitude is bounded below clipping");
        std::array<int16_t, 480> tail{};
        tone.render(tail.data(), tail.size(), false, hz);
        require(std::all_of(tail.begin()+144, tail.end(), [](int16_t sample) { return sample == 0; }),
                "key-up fades to exact silence within 3ms");
    }
    ToneGenerator whole, chunks;
    std::array<int16_t, 960> a{}, b{};
    whole.render(a.data(), a.size(), true, 650);
    for (size_t i=0; i<b.size(); i+=240) chunks.render(b.data()+i, 240, true, 650);
    require(a == b, "phase and envelope continuous across device buffers");
    ToneGenerator silent;
    silent.render(a.data(), a.size(), false, 650);
    require(std::all_of(a.begin(), a.end(), [](int16_t sample) { return sample == 0; }),
            "unpressed contact generates silence");
    std::cout << "PASS: " << checks << " sine / envelope checks\n";
}

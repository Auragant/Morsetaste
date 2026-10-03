// SPDX-License-Identifier: GPL-3.0-only
// LLM-assisted implementation; see LICENSE and DISCLAIMER.md.
#pragma once
#include <cstring>
#include "tone_core.hpp"

namespace morse {
struct AudioFormat {
    unsigned rate = 0, channels = 0, bits = 0, validBits = 0;
    uint32_t channelMask = 0;
    bool floating = false;
    bool valid() const {
        if (rate < 8000 || rate > 768000 || channels == 0 || channels > 32) return false;
        if (channelMask) {
            unsigned speakers = 0;
            for (uint32_t mask = channelMask; mask; mask >>= 1) speakers += mask & 1;
            if (speakers != channels) return false;
        }
        if (floating) return (bits == 32 || bits == 64) && validBits == bits;
        return (bits == 16 || bits == 24 || bits == 32) && validBits > 0 && validBits <= bits;
    }
    size_t frameBytes() const { return size_t(channels) * (bits / 8); }
};

class ToneBuffer {
public:
    explicit ToneBuffer(AudioFormat format) : format_(format), tone_(format.rate) {}
    void render(uint8_t* destination, size_t frames, bool down, unsigned hz) {
        for (size_t frame = 0; frame < frames; ++frame) {
            const float sample = tone_.nextSample(down, hz);
            unsigned speaker = 0;
            for (unsigned channel = 0; channel < format_.channels; ++channel) {
                bool audible = format_.channels == 1 || channel < 2;
                if (format_.channelMask && format_.channels > 1) {
                    while (speaker < 32 && !(format_.channelMask & (uint32_t(1) << speaker))) ++speaker;
                    audible = speaker < 2; // front left/right; keep LFE silent
                    if (speaker < 32) ++speaker;
                }
                encode(destination, audible ? sample : 0.0f);
                destination += format_.bits / 8;
            }
        }
    }
private:
    AudioFormat format_;
    ToneGenerator tone_;
    void encode(uint8_t* destination, float sample) const {
        if (format_.floating) {
            if (format_.bits == 32) std::memcpy(destination, &sample, sizeof(sample));
            else { const double value = sample; std::memcpy(destination, &value, sizeof(value)); }
            return;
        }
        const int64_t scale = int64_t(1) << (format_.validBits - 1);
        const int64_t value = std::clamp(int64_t(std::llround(double(sample) * double(scale))), -scale, scale-1);
        const uint64_t packed = uint64_t(value) << (format_.bits - format_.validBits);
        for (unsigned byte = 0; byte < format_.bits/8; ++byte)
            destination[byte] = uint8_t(packed >> (byte*8));
    }
};
} // namespace morse

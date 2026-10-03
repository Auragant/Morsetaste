// SPDX-License-Identifier: GPL-3.0-only
#include "../windows/audio_core.hpp"
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {
int checks = 0;
void require(bool condition, const char* name) {
    ++checks;
    if (!condition) { std::cerr << "FAIL: " << name << '\n'; std::exit(1); }
}
double decode(const uint8_t* source, const morse::AudioFormat& format) {
    if (format.floating) {
        if (format.bits == 32) { float value = 0; std::memcpy(&value,source,4); return value; }
        double value = 0; std::memcpy(&value,source,8); return value;
    }
    uint64_t packed = 0;
    for (unsigned byte=0; byte<format.bits/8; ++byte) packed |= uint64_t(source[byte]) << (byte*8);
    const int64_t signedValue = packed & (uint64_t(1) << (format.bits-1))
        ? int64_t(packed) - (int64_t(1) << format.bits) : int64_t(packed);
    return double(signedValue) / double(int64_t(1) << (format.bits-1));
}
}
int main() {
    using morse::AudioFormat;
    using morse::ToneBuffer;
    require(!AudioFormat{}.valid(), "uninitialized format rejected");
    require(!AudioFormat{48000,0,32,32,0,true}.valid(), "zero channels rejected");
    require(!AudioFormat{48000,2,32,32,4,true}.valid(), "inconsistent speaker mask rejected");
    require(!AudioFormat{48000,2,8,8,0,false}.valid(), "unsupported encoding rejected");
    require(!AudioFormat{48000,2,32,40,0,false}.valid(), "invalid PCM precision rejected");
    for (unsigned rate : {44100u,48000u,96000u,192000u}) {
        for (const auto& encoding : std::vector<AudioFormat>{
                 {rate,2,32,32,3,true}, {rate,2,64,64,3,true},
                 {rate,2,16,16,3,false}, {rate,2,24,24,3,false},
                 {rate,2,32,24,3,false}, {rate,2,32,32,3,false}}) {
            require(encoding.valid(), "supported engine format accepted");
            const size_t bytes = size_t(rate)*encoding.frameBytes();
            std::vector<uint8_t> buffer(bytes+32, 0xa5);
            ToneBuffer tone(encoding);
            size_t at = 0;
            while (at<rate) {
                const size_t frames = std::min(size_t(rate)-at, size_t(127+(at%503)));
                tone.render(buffer.data()+16+at*encoding.frameBytes(),frames,true,650);
                at += frames;
            }
            double maxError = 0;
            bool stereo = true, aligned = true;
            int crossings = 0;
            double previous = 0;
            for (size_t frame=rate/100; frame<rate; ++frame) {
                const uint8_t* sample = buffer.data()+16+frame*encoding.frameBytes();
                const double actual = decode(sample,encoding);
                const double right = decode(sample+encoding.bits/8,encoding);
                const double expected = std::sin(6.2831853071795864769*650*double(frame)/rate)*6000/32768;
                maxError = std::max(maxError,std::abs(actual-expected));
                stereo = stereo && actual == right;
                if (previous<=0 && actual>0) ++crossings;
                previous = actual;
                if (!encoding.floating && encoding.bits == 32 && encoding.validBits == 24)
                    aligned = aligned && sample[0] == 0;
            }
            const double tolerance = encoding.floating ? 1e-7 :
                1.0 / double(uint64_t(1) << encoding.validBits)+1e-7;
            require(maxError<tolerance, "native float/PCM output follows exact 650Hz sine");
            require(stereo && aligned, "identical stereo and correctly aligned integer samples");
            require(crossings >= 643 && crossings <= 645, "pitch preserved at device sample rate");
            require(std::all_of(buffer.begin(),buffer.begin()+16,[](uint8_t b) { return b == 0xa5; }) &&
                    std::all_of(buffer.end()-16,buffer.end(),[](uint8_t b) { return b == 0xa5; }),
                    "irregular frame sizes never overwrite buffer guards");
            const size_t tailFrames = rate/100;
            std::vector<uint8_t> tail(tailFrames*encoding.frameBytes(),0xa5);
            tone.render(tail.data(),tailFrames,false,650);
            const size_t quiet = size_t(std::ceil(rate*0.003))*encoding.frameBytes();
            require(std::all_of(tail.begin()+quiet,tail.end(),[](uint8_t b) { return b == 0; }),
                    "release is silent in every native format");
        }
    }
    const AudioFormat surround{48000,6,32,32,0x3f,true};
    ToneBuffer surroundTone(surround);
    std::vector<uint8_t> surroundData(480*surround.frameBytes());
    surroundTone.render(surroundData.data(),480,true,650);
    bool quietOthers = true;
    for (size_t frame=0; frame<480; ++frame)
        for (unsigned channel=2; channel<6; ++channel)
            quietOthers = quietOthers && decode(surroundData.data()+frame*24+channel*4,surround) == 0;
    require(quietOthers, "surround LFE and rear channels stay silent");
    std::cout << "PASS: " << checks << " native audio format / buffer checks\n";
}

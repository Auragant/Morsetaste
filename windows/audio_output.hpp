// SPDX-License-Identifier: GPL-3.0-only
// LLM-assisted implementation; see LICENSE and DISCLAIMER.md.
#pragma once
#include <windows.h>
#include <mmsystem.h>
#include <array>
#include <atomic>
#include <thread>
#include "tone_core.hpp"

namespace morse {
// All waveOut ownership stays on this thread, separate from serial I/O and UI.
// Three 5-ms buffers keep the queued PCM small. Driver/device latency is additional.
class AudioOutput {
public:
    std::atomic<bool> enabled{false}, contact{false}, suspended{false}, active{false};
    std::atomic<unsigned> frequency{toneDefaultHz}, error{0};
    std::atomic<uint64_t> submitted{0};
    void start() {
        if (worker_.joinable()) return;
        stopping_ = false;
        worker_ = std::thread([this] { run(); });
    }
    void shutdown() {
        stopping_ = true;
        if (worker_.joinable()) worker_.join();
    }
    ~AudioOutput() { shutdown(); }
    AudioOutput() = default;
    AudioOutput(const AudioOutput&) = delete;
    AudioOutput& operator=(const AudioOutput&) = delete;
private:
    std::atomic<bool> stopping_{false};
    std::thread worker_;
    bool wanted() const { return enabled && !suspended && !stopping_; }
    void run() {
        HANDLE done = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!done) { error = MMSYSERR_NOMEM; return; }
        while (!stopping_) {
            if (!wanted()) { error = 0; Sleep(5); continue; }
            WAVEFORMATEX format{};
            format.wFormatTag = WAVE_FORMAT_PCM; format.nChannels = 1;
            format.nSamplesPerSec = ToneGenerator::sampleRate; format.wBitsPerSample = 16;
            format.nBlockAlign = 2; format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
            HWAVEOUT device = nullptr;
            MMRESULT result = waveOutOpen(&device, WAVE_MAPPER, &format,
                                         reinterpret_cast<DWORD_PTR>(done), 0, CALLBACK_EVENT);
            if (result == MMSYSERR_NOERROR) {
                error = 0;
                struct Buffer {
                    std::array<int16_t, ToneGenerator::sampleRate / 200> samples{};
                    WAVEHDR header{};
                    bool prepared = false, queued = false;
                };
                std::array<Buffer, 3> buffers{};
                ToneGenerator tone;
                for (auto& buffer : buffers) {
                    buffer.header.lpData = reinterpret_cast<LPSTR>(buffer.samples.data());
                    buffer.header.dwBufferLength = DWORD(buffer.samples.size() * sizeof(int16_t));
                    result = waveOutPrepareHeader(device, &buffer.header, sizeof(WAVEHDR));
                    if (result != MMSYSERR_NOERROR) break;
                    buffer.prepared = true;
                }
                active = result == MMSYSERR_NOERROR;
                while (wanted() && result == MMSYSERR_NOERROR) {
                    for (auto& buffer : buffers) {
                        if (buffer.queued && !(buffer.header.dwFlags & WHDR_DONE)) continue;
                        tone.render(buffer.samples.data(), buffer.samples.size(), contact, frequency);
                        result = waveOutWrite(device, &buffer.header, sizeof(WAVEHDR));
                        if (result != MMSYSERR_NOERROR) break;
                        buffer.queued = true; ++submitted;
                    }
                    if (result == MMSYSERR_NOERROR) WaitForSingleObject(done, 5);
                }
                active = false;
                // Reset returns every outstanding buffer before its memory is released.
                waveOutReset(device);
                for (auto& buffer : buffers)
                    if (buffer.prepared) waveOutUnprepareHeader(device, &buffer.header, sizeof(WAVEHDR));
                waveOutClose(device);
            }
            error = result;
            // Reopen the default device after an error; off/on retries immediately.
            if (result != MMSYSERR_NOERROR)
                for (int i = 0; i < 200 && wanted(); ++i) Sleep(10);
        }
        active = false;
        CloseHandle(done);
    }
};
} // namespace morse

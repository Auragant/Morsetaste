// SPDX-License-Identifier: GPL-3.0-only
#define NOMINMAX
#include "../windows/audio_output.hpp"
#include <iostream>

int main() {
    morse::AudioOutput audio;
    audio.start();
    Sleep(30);
    if (audio.active || audio.submitted != 0) return 1;
    audio.enabled = true;
    const uint64_t deadline = GetTickCount64() + 3000;
    while (!audio.active && GetTickCount64() < deadline && !audio.error) Sleep(5);
    if (!audio.active || audio.error) {
        std::cerr << "FAIL: default audio device unavailable, error " << audio.error << '\n';
        return 2;
    }
    for (unsigned hz : {650u, 400u, 1000u}) {
        audio.frequency = hz; audio.contact = true;
        Sleep(100); audio.contact = false; Sleep(30);
    }
    const uint64_t submitted = audio.submitted;
    if (audio.error || submitted < 20) return 3;
    audio.enabled = false;
    const uint64_t offDeadline = GetTickCount64() + 1000;
    while (audio.active && GetTickCount64() < offDeadline) Sleep(5);
    if (audio.active) return 4;
    const uint64_t offSubmitted = audio.submitted;
    Sleep(30);
    if (audio.submitted != offSubmitted) return 5;
    audio.enabled = true;
    const uint64_t restartDeadline = GetTickCount64() + 3000;
    while (!audio.active && GetTickCount64() < restartDeadline && !audio.error) Sleep(5);
    if (!audio.active || audio.error) return 6;
    audio.suspended = true;
    const uint64_t stopDeadline = GetTickCount64() + 1000;
    while (audio.active && GetTickCount64() < stopDeadline) Sleep(5);
    if (audio.active) return 7;
    Sleep(30);
    const uint64_t suspendedSubmitted = audio.submitted;
    Sleep(30);
    if (audio.submitted != suspendedSubmitted) return 8;
    audio.enabled = false; audio.suspended = false;
    audio.shutdown();
    std::cout << "PASS: default audio device, sine buffers, off/on, suspend and shutdown ("
              << audio.submitted << " buffers submitted)\n";
}

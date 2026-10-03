// SPDX-License-Identifier: GPL-3.0-only
// LLM-assisted implementation; see LICENSE and DISCLAIMER.md.
#pragma once
#include <cstdint>
#include <deque>
#include <functional>
#include <string>

namespace morse {

// Session-only rolling sample window. Heartbeats and interrupted holds are not samples.
class PulseHistory {
public:
    static constexpr uint64_t windowMs = 300000;
    static constexpr uint64_t maxDurationMs = 1000;
    struct Sample { uint64_t at, duration; };
    std::deque<Sample> samples;
    void receive(bool down, uint64_t now) {
        if (!known_) { known_ = true; down_ = down; return; }
        if (down == down_) return;
        down_ = down;
        if (down) { start_ = now; measuring_ = true; }
        else if (measuring_) {
            const uint64_t duration = now - start_;
            if (duration <= maxDurationMs) samples.push_back({now, duration});
            measuring_ = false;
            // Also bound memory in case of unusually dense input.
            if (samples.size() > 16384) samples.pop_front();
        }
        prune(now);
    }
    void interrupt() { known_ = false; measuring_ = false; }
    void prune(uint64_t now) {
        while (!samples.empty() && now - samples.front().at >= windowMs) samples.pop_front();
    }
private:
    bool known_ = false, down_ = false, measuring_ = false;
    uint64_t start_ = 0;
};

// Strict, bounded framing: fragmented reads, CRLF/LF and bootloader garbage
// are supported; an overlong line is discarded in its entirety.
class FrameParser {
public:
    template<class Receive> void feed(const char* bytes, size_t size, Receive receive) {
        for (size_t i = 0; i < size; ++i) {
            const char c = bytes[i];
            if (c == '\n') {
                if (!overflow_) {
                    if (!line_.empty() && line_.back() == '\r') line_.pop_back();
                    if (line_ == "JUNKER/1 D") receive(true);
                    else if (line_ == "JUNKER/1 U") receive(false);
                }
                reset();
            } else if (!overflow_) {
                if (line_.size() >= 64) { overflow_ = true; line_.clear(); }
                else line_ += c;
            }
        }
    }
    void reset() { line_.clear(); overflow_ = false; }
private:
    std::string line_;
    bool overflow_ = false;
};

// Used by both the real serial worker and the automated tests. The output
// callback returns true only if the OS accepted the key event.
class Bridge {
public:
    static constexpr uint64_t timeoutMs = 1000;
    explicit Bridge(std::function<bool(bool)> send) : send_(std::move(send)) {}

    void receive(bool down, uint64_t now) {
        if (!connected) { connected = true; armed = false; }
        lastSeen = now;
        physicalDown = down;
        if (!down) {
            release();
            armed = enabled && !blocked && !fault && !outputDown;
        } else if (enabled && !blocked && !fault && armed && !outputDown) {
            if (send_(true)) outputDown = true;
            else { fault = true; armed = false; }
        }
    }
    void enable(bool value) {
        if (enabled == value) return;
        enabled = value;
        armed = false;
        release();
        if (value && !outputDown) fault = false;
    }
    void block(bool value) {
        if (blocked == value) return;
        blocked = value;
        armed = false;
        if (value) release();
    }
    void disconnect() {
        connected = false;
        physicalDown = false;
        armed = false;
        release();
    }
    void tick(uint64_t now) {
        if (connected && now - lastSeen >= timeoutMs) disconnect();
        // Retry failed key-up; never claim it was released when SendInput failed.
        if (outputDown && (fault || !enabled || blocked || !connected)) release();
    }
    void release() {
        if (!outputDown) return;
        if (send_(false)) outputDown = false;
        else { fault = true; armed = false; }
    }

    bool connected = false;
    bool physicalDown = false;
    bool outputDown = false;
    bool enabled = true;
    bool blocked = false;
    bool armed = false;
    bool fault = false;
    uint64_t lastSeen = 0;
private:
    std::function<bool(bool)> send_;
};
} // namespace morse

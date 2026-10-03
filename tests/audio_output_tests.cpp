// SPDX-License-Identifier: GPL-3.0-only
// Exercises the production thread with a fake transport: no audio device or sound.
#define NOMINMAX
#include "../windows/audio_output.hpp"
#include <iostream>
#include <mutex>
#include <vector>

namespace {
int checks = 0;
void require(bool condition, const char* name) {
    ++checks;
    if (!condition) { std::cerr << "FAIL: " << name << '\n'; std::exit(1); }
}
template<class Predicate> bool await(Predicate predicate) {
    const uint64_t until = GetTickCount64()+1500;
    while (GetTickCount64()<until) { if (predicate()) return true; Sleep(1); }
    return predicate();
}
struct State {
    std::atomic<HANDLE> ready{nullptr};
    std::atomic<unsigned> opens{0}, closes{0}, starts{0}, stops{0}, commits{0}, padding{0};
    std::atomic<unsigned> fail{0};
    std::atomic<bool> malformedPadding{false};
    std::mutex mutex;
    std::vector<BYTE> last;
    unsigned lastFrames = 0;
    bool primed = false, requestValid = true;
    void consume(unsigned frames) {
        padding = padding >= frames ? padding-frames : 0;
        if (ready) SetEvent(ready);
    }
};
class FakeDevice final : public morse::AudioDevice {
public:
    explicit FakeDevice(std::shared_ptr<State> state) : state_(std::move(state)), data_(960*8) {}
    HRESULT open(HANDLE ready, morse::AudioFormat& format, UINT32& frames) override {
        ++state_->opens; state_->ready = ready;
        if (state_->fail == 1) return AUDCLNT_E_DEVICE_INVALIDATED;
        format = {48000,2,32,32,3,true}; frames = 960; state_->padding = 0;
        return S_OK;
    }
    HRESULT padding(UINT32& frames) override {
        if (state_->fail == 2) return AUDCLNT_E_DEVICE_INVALIDATED;
        frames = state_->malformedPadding ? 961 : state_->padding.load(); return S_OK;
    }
    HRESULT acquire(UINT32 frames, BYTE*& data) override {
        if (state_->fail == 3) return E_FAIL;
        state_->requestValid = state_->requestValid && frames>0 && frames<=960-state_->padding;
        acquired_ = frames; data = data_.data(); return S_OK;
    }
    HRESULT commit(UINT32 frames, bool silent) override {
        if (state_->fail == 4) return E_FAIL;
        if (silent) { state_->primed = frames == 960; }
        else {
            std::lock_guard<std::mutex> lock(state_->mutex);
            state_->last.assign(data_.begin(), data_.begin()+frames*8); state_->lastFrames = frames;
        }
        state_->requestValid = state_->requestValid && frames == acquired_;
        state_->padding += frames; ++state_->commits; return S_OK;
    }
    HRESULT start() override {
        if (state_->fail == 5) return E_FAIL;
        if (!state_->primed) return E_UNEXPECTED;
        started_ = true; ++state_->starts; return S_OK;
    }
    void stop() override { if (started_) { ++state_->stops; started_ = false; } }
    ~FakeDevice() override { stop(); ++state_->closes; }
private:
    std::shared_ptr<State> state_;
    std::vector<BYTE> data_;
    unsigned acquired_ = 0;
    bool started_ = false;
};
morse::AudioOutput::Factory factory(std::shared_ptr<State> state) {
    return [state] { return std::make_unique<FakeDevice>(state); };
}
}
int main() {
    auto state = std::make_shared<State>();
    morse::AudioOutput audio(factory(state)); audio.start();
    Sleep(20); require(state->opens == 0, "disabled audio never opens a device");
    audio.setEnabled(true);
    require(await([&] { return audio.active.load(); }), "event transport starts");
    require(state->primed && state->starts == 1, "stream primed before Start");
    audio.setContact(true); state->consume(240);
    require(await([&] { return audio.submitted >= 1; }), "engine event requests a refill");
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        require(state->lastFrames == 240 && state->requestValid, "exact available frame count, no overwrite");
        bool sine = false, equal = true;
        for (size_t i=0; i<state->last.size(); i+=8) {
            float left = 0, right = 0;
            std::memcpy(&left, state->last.data()+i, 4); std::memcpy(&right, state->last.data()+i+4, 4);
            sine = sine || std::abs(left)>0.1f; equal = equal && left == right;
        }
        require(sine && equal, "contact renders identical stereo sine samples");
    }
    const auto written = audio.submitted.load();
    SetEvent(state->ready); Sleep(20);
    require(audio.submitted == written, "full buffer is never overwritten");
    audio.setContact(false); state->consume(480);
    require(await([&] { return audio.submitted > written; }), "release gets rendered");
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        bool silent = true;
        for (size_t i=144*8; i<state->last.size(); ++i) silent = silent && state->last[i] == 0;
        require(silent, "released contact fades to zero samples");
    }
    audio.setEnabled(false);
    require(await([&] { return state->closes == 1; }) && !audio.active, "disable closes stream exactly once");
    audio.setEnabled(true);
    require(await([&] { return audio.active.load(); }), "explicit reenable reopens stream");
    audio.setSuspended(true);
    require(await([&] { return state->closes == 2; }) && !audio.active, "suspend releases stream");
    audio.setSuspended(false);
    require(await([&] { return audio.active.load(); }), "resume reopens enabled stream");
    audio.shutdown();
    require(state->closes == 3 && state->starts == state->stops && !audio.active,
            "shutdown wakes event wait and balances every Start/Stop");
    for (unsigned failure : {1u,2u,3u,4u,5u}) {
        auto failed = std::make_shared<State>(); failed->fail = failure;
        morse::AudioOutput test(factory(failed)); test.setEnabled(true); test.start();
        require(await([&] { return test.error != 0; }) && !test.enabled && !test.active,
                "device failure disables output with an error");
        Sleep(20); require(failed->opens == 1 && failed->closes == 1, "failed device never enters reopen loop");
        failed->fail = 0; test.setEnabled(true);
        require(await([&] { return test.active.load(); }) && test.error == 0, "user retry clears error and starts");
        test.shutdown();
    }
    auto invalid = std::make_shared<State>(); invalid->malformedPadding = true;
    morse::AudioOutput guarded(factory(invalid)); guarded.setEnabled(true); guarded.start();
    require(await([&] { return guarded.error != 0; }) && !guarded.enabled, "invalid padding fails before buffer arithmetic");
    guarded.shutdown();
    std::cout << "PASS: " << checks << " audio thread / fake transport checks (no device, no sound)\n";
}
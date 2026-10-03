// SPDX-License-Identifier: GPL-3.0-only
// LLM-assisted implementation; see LICENSE and DISCLAIMER.md.
#pragma once
#include <atomic>
#include <functional>
#include <thread>
#include "wasapi_device.hpp"
#include <avrt.h>

namespace morse {
// WASAPI and COM stay on one thread, woken by engine/control events.
class AudioOutput {
public:
    using Factory = std::function<std::unique_ptr<AudioDevice>()>;
    std::atomic<bool> enabled{false}, contact{false}, suspended{false}, active{false};
    std::atomic<unsigned> frequency{toneDefaultHz}, error{0};
    std::atomic<uint64_t> submitted{0};
    explicit AudioOutput(Factory factory = [] { return std::make_unique<WasapiDevice>(); })
        : factory_(std::move(factory)), control_(CreateEventW(nullptr, FALSE, FALSE, nullptr)) {}
    void setEnabled(bool value) { error = 0; enabled = value; wake(); }
    void setContact(bool value) { if (contact.exchange(value) != value) wake(); }
    void setSuspended(bool value) { if (suspended.exchange(value) != value) wake(); }
    void start() {
        if (worker_.joinable()) return;
        stopping_ = false;
        worker_ = std::thread([this] { run(); });
    }
    void shutdown() {
        stopping_ = true; wake();
        if (worker_.joinable()) worker_.join();
    }
    ~AudioOutput() { shutdown(); if (control_) CloseHandle(control_); }
    AudioOutput(const AudioOutput&) = delete;
    AudioOutput& operator=(const AudioOutput&) = delete;
private:
    Factory factory_;
    HANDLE control_ = nullptr;
    std::atomic<bool> stopping_{false};
    std::thread worker_;
    void wake() { if (control_) SetEvent(control_); }
    bool wanted() const { return enabled && !suspended && !stopping_; }
    HRESULT stream(HANDLE ready) {
        auto device = factory_();
        if (!device) return E_OUTOFMEMORY;
        AudioFormat format;
        UINT32 capacity = 0;
        HRESULT result = device->open(ready, format, capacity);
        if (FAILED(result)) return result;
        if (!format.valid() || !capacity || capacity > format.rate*2) return AUDCLNT_E_UNSUPPORTED_FORMAT;
        ToneBuffer tone(format);
        BYTE* data = nullptr;
        result = device->acquire(capacity, data);
        if (FAILED(result)) return result;
        // Prime before Start, using the WASAPI silent-buffer flag.
        result = device->commit(capacity, true);
        if (FAILED(result)) return result;
        result = device->start();
        if (FAILED(result)) return result;
        active = true;
        HANDLE events[] = {control_, ready};
        while (wanted()) {
            const DWORD wait = WaitForMultipleObjects(2, events, FALSE, 2000);
            if (!wanted()) break;
            if (wait == WAIT_TIMEOUT) { result = HRESULT_FROM_WIN32(ERROR_TIMEOUT); break; }
            if (wait == WAIT_FAILED) { result = HRESULT_FROM_WIN32(GetLastError()); break; }
            UINT32 padding = 0;
            result = device->padding(padding);
            if (FAILED(result)) break;
            if (padding > capacity) { result = E_UNEXPECTED; break; }
            const UINT32 available = capacity-padding;
            if (!available) continue;
            result = device->acquire(available, data);
            if (FAILED(result)) break;
            if (!data) { device->commit(available, true); result = E_POINTER; break; }
            tone.render(data, available, contact, frequency);
            result = device->commit(available, false);
            if (FAILED(result)) break;
            ++submitted;
        }
        device->stop();
        active = false;
        return result;
    }
    void run() {
        if (!control_) { error = unsigned(E_OUTOFMEMORY); enabled = false; return; }
        const HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(initialized)) { error = unsigned(initialized); enabled = false; return; }
        HANDLE ready = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!ready) { error = unsigned(E_OUTOFMEMORY); enabled = false; CoUninitialize(); return; }
        DWORD taskIndex = 0;
        HANDLE scheduling = AvSetMmThreadCharacteristicsW(L"Audio", &taskIndex);
        while (!stopping_) {
            if (!wanted()) { WaitForSingleObject(control_, INFINITE); continue; }
            ResetEvent(ready);
            const HRESULT result = stream(ready);
            if (FAILED(result)) {
                // Latch failures; only explicit enabling retries, with no restart loop.
                error = unsigned(result); enabled = false;
            }
        }
        if (scheduling) AvRevertMmThreadCharacteristics(scheduling);
        CloseHandle(ready);
        CoUninitialize();
    }
};
} // namespace morse
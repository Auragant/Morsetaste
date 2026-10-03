// SPDX-License-Identifier: GPL-3.0-only
// LLM-assisted implementation; see LICENSE and DISCLAIMER.md.
#pragma once
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include <memory>
#include "audio_core.hpp"

namespace morse {
template<class T> class ComOwner {
public:
    T* get() const { return value_; }
    T* operator->() const { return value_; }
    T** receive() { reset(); return &value_; }
    void reset() { if (value_) { value_->Release(); value_ = nullptr; } }
    ~ComOwner() { reset(); }
    ComOwner() = default;
    ComOwner(const ComOwner&) = delete;
    ComOwner& operator=(const ComOwner&) = delete;
private:
    T* value_ = nullptr;
};

// Test boundary for the real rendering loop; test devices never access hardware.
class AudioDevice {
public:
    virtual HRESULT open(HANDLE ready, AudioFormat& format, UINT32& frames) = 0;
    virtual HRESULT padding(UINT32& frames) = 0;
    virtual HRESULT acquire(UINT32 frames, BYTE*& data) = 0;
    virtual HRESULT commit(UINT32 frames, bool silent) = 0;
    virtual HRESULT start() = 0;
    virtual void stop() = 0;
    virtual ~AudioDevice() = default;
};

class WasapiDevice final : public AudioDevice {
public:
    HRESULT open(HANDLE ready, AudioFormat& format, UINT32& frames) override {
        HRESULT result = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                          __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(enumerator_.receive()));
        if (FAILED(result)) return result;
        result = enumerator_->GetDefaultAudioEndpoint(eRender, eConsole, endpoint_.receive());
        if (FAILED(result)) return result;
        result = endpoint_->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                     reinterpret_cast<void**>(client_.receive()));
        if (FAILED(result)) return result;
        WAVEFORMATEX* mix = nullptr;
        result = client_->GetMixFormat(&mix);
        if (FAILED(result)) return result;
        if (!mix) return E_POINTER;
        struct MixDeleter { void operator()(WAVEFORMATEX* p) const { CoTaskMemFree(p); } };
        const std::unique_ptr<WAVEFORMATEX, MixDeleter> ownedMix(mix);
        format.rate = mix->nSamplesPerSec; format.channels = mix->nChannels;
        format.bits = mix->wBitsPerSample; format.validBits = mix->wBitsPerSample;
        WORD tag = mix->wFormatTag;
        if (tag == WAVE_FORMAT_EXTENSIBLE) {
            if (mix->cbSize < sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX)) return AUDCLNT_E_UNSUPPORTED_FORMAT;
            const auto* ext = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(mix);
            format.validBits = ext->Samples.wValidBitsPerSample;
            format.channelMask = ext->dwChannelMask;
            constexpr GUID floatFormat = {STATIC_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT};
            constexpr GUID pcmFormat = {STATIC_KSDATAFORMAT_SUBTYPE_PCM};
            if (IsEqualGUID(ext->SubFormat, floatFormat)) tag = WAVE_FORMAT_IEEE_FLOAT;
            else if (IsEqualGUID(ext->SubFormat, pcmFormat)) tag = WAVE_FORMAT_PCM;
            else return AUDCLNT_E_UNSUPPORTED_FORMAT;
        }
        format.floating = tag == WAVE_FORMAT_IEEE_FLOAT;
        if ((tag != WAVE_FORMAT_PCM && !format.floating) || !format.valid() ||
            format.frameBytes() != mix->nBlockAlign) return AUDCLNT_E_UNSUPPORTED_FORMAT;
        // Shared mode with engine-selected timing and buffer; no exclusive output.
        result = client_->Initialize(AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST, 0, 0, mix, nullptr);
        if (FAILED(result)) return result;
        result = client_->SetEventHandle(ready);
        if (FAILED(result)) return result;
        result = client_->GetBufferSize(&frames);
        if (FAILED(result)) return result;
        return client_->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(render_.receive()));
    }
    HRESULT padding(UINT32& frames) override { return client_->GetCurrentPadding(&frames); }
    HRESULT acquire(UINT32 frames, BYTE*& data) override { return render_->GetBuffer(frames, &data); }
    HRESULT commit(UINT32 frames, bool silent) override {
        return render_->ReleaseBuffer(frames, silent ? AUDCLNT_BUFFERFLAGS_SILENT : 0);
    }
    HRESULT start() override {
        const HRESULT result = client_->Start();
        started_ = SUCCEEDED(result);
        return result;
    }
    void stop() override {
        if (started_) { client_->Stop(); started_ = false; }
    }
    ~WasapiDevice() override { stop(); }
private:
    bool started_ = false;
    ComOwner<IMMDeviceEnumerator> enumerator_;
    ComOwner<IMMDevice> endpoint_;
    ComOwner<IAudioClient> client_;
    ComOwner<IAudioRenderClient> render_;
};
} // namespace morse

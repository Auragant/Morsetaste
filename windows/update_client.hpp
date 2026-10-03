// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>
#include <array>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include "update_core.hpp"

namespace morse {
struct UpdateResponse {
    unsigned status = 0;
    std::string body;
    std::wstring error;
    uint32_t retryAfterSeconds = 0;
};

namespace update_detail {
using UpdateClock = std::chrono::steady_clock;
struct HttpEvent { DWORD status = 0, bytes = 0, error = 0; };
struct HttpContext : std::enable_shared_from_this<HttpContext> {
    HANDLE wake = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    HANDLE closed = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    std::array<char, 8192> buffer{};
    std::array<HttpEvent, 16> events{};
    std::mutex mutex;
    size_t begin = 0, count = 0;
    std::atomic<bool> overflow{false};
    // A request owns this reference until its last callback, even on close errors.
    std::shared_ptr<HttpContext> requestLifetime;
    ~HttpContext() { if (wake) CloseHandle(wake); if (closed) CloseHandle(closed); }
    bool pop(HttpEvent& output) {
        std::lock_guard<std::mutex> lock(mutex);
        if (!count) return false;
        output = events[begin]; begin = (begin + 1) % events.size(); --count; return true;
    }
    static void CALLBACK callback(HINTERNET, DWORD_PTR raw, DWORD status, void* information, DWORD length) noexcept {
        if (!raw) return;
        auto* context = reinterpret_cast<HttpContext*>(raw);
        try {
            const auto lifetime = context->shared_from_this();
            if (status == WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING) {
                context->requestLifetime.reset();
                SetEvent(context->closed);
                return;
            }
            if (status != WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE &&
                    status != WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE &&
                    status != WINHTTP_CALLBACK_STATUS_READ_COMPLETE &&
                    status != WINHTTP_CALLBACK_STATUS_REQUEST_ERROR) return;
            HttpEvent event{status, length, 0};
            if (status == WINHTTP_CALLBACK_STATUS_REQUEST_ERROR) {
                event.error = information && length == sizeof(WINHTTP_ASYNC_RESULT)
                    ? static_cast<WINHTTP_ASYNC_RESULT*>(information)->dwError : ERROR_WINHTTP_INTERNAL_ERROR;
            }
            {
                std::lock_guard<std::mutex> lock(context->mutex);
                if (context->count == context->events.size()) context->overflow = true;
                else {
                    context->events[(context->begin + context->count) % context->events.size()] = event;
                    ++context->count;
                }
            }
            SetEvent(context->wake);
        } catch (...) {
            // No C++ exception may cross the Windows callback boundary.
            context->overflow = true;
            SetEvent(context->wake);
        }
    }
};
struct HttpHandles {
    std::shared_ptr<HttpContext> context;
    HINTERNET session = nullptr, connection = nullptr, request = nullptr;
    bool registered = false, abandoned = false;
    explicit HttpHandles(std::shared_ptr<HttpContext> value) : context(std::move(value)) {}
    bool close() noexcept {
        if (abandoned) return false;
        if (request) {
            const HINTERNET closing = request;
            request = nullptr;
            if (!WinHttpCloseHandle(closing)) {
                // Keep the pinned context and parent handles alive if Windows could
                // not close the request. Freeing callback data here would be unsafe.
                abandoned = true; return false;
            }
            if (registered) WaitForSingleObject(context->closed, INFINITE);
        }
        if (connection) { WinHttpCloseHandle(connection); connection = nullptr; }
        if (session) { WinHttpCloseHandle(session); session = nullptr; }
        return true;
    }
    ~HttpHandles() { close(); }
};
inline std::wstring networkError(DWORD error) {
    if (error == ERROR_WINHTTP_TIMEOUT) return L"Die Updateprüfung hat zu lange gedauert. Bitte später erneut versuchen.";
    if (error == ERROR_WINHTTP_SECURE_FAILURE) return L"Die sichere Verbindung zu GitHub konnte nicht geprüft werden.";
    if (error == ERROR_WINHTTP_NAME_NOT_RESOLVED || error == ERROR_WINHTTP_CANNOT_CONNECT ||
            error == ERROR_WINHTTP_CONNECTION_ERROR) return L"GitHub ist nicht erreichbar. Bitte die Internetverbindung prüfen.";
    return L"Die Updateprüfung konnte nicht abgeschlossen werden.";
}
inline uint32_t unsignedHeader(std::wstring_view text) {
    uint32_t result = 0;
    if (text.empty()) return 0;
    for (wchar_t c : text) {
        if (c < L'0' || c > L'9' || result > (std::numeric_limits<uint32_t>::max() - uint32_t(c - L'0')) / 10) return 0;
        result = result * 10 + uint32_t(c - L'0');
    }
    return result;
}
inline uint32_t retryDelay(HINTERNET request) {
    auto header = [&](const wchar_t* name) {
        wchar_t text[64]{};
        DWORD bytes = sizeof(text);
        if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM, name, text, &bytes, WINHTTP_NO_HEADER_INDEX))
            return std::wstring();
        return std::wstring(text);
    };
    const uint32_t retry = unsignedHeader(header(L"Retry-After"));
    if (retry) return retry;
    if (header(L"X-RateLimit-Remaining") == L"0") {
        const uint32_t reset = unsignedHeader(header(L"X-RateLimit-Reset"));
        const auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (reset > now) return uint32_t(reset - now);
    }
    return 60;
}
inline UpdateResponse fetchLatest(Version current, const std::atomic<bool>& cancel, UpdateClock::time_point deadline) {
    UpdateResponse response;
    auto context = std::make_shared<HttpContext>();
    if (!context->wake || !context->closed) { response.error = networkError(ERROR_NOT_ENOUGH_MEMORY); return response; }
    HttpHandles handles(context);
    const std::string agentText = "MorseBridge/" + versionString(current);
    const std::wstring agent(agentText.begin(), agentText.end());
    handles.session = WinHttpOpen(agent.c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
    if (!handles.session || !WinHttpSetTimeouts(handles.session, 5000, 5000, 5000, 5000)) {
        response.error = networkError(GetLastError()); return response;
    }
    handles.connection = WinHttpConnect(handles.session, L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!handles.connection) { response.error = networkError(GetLastError()); return response; }
    handles.request = WinHttpOpenRequest(handles.connection, L"GET", L"/repos/Auragant/Morsetaste/releases/latest",
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!handles.request) { response.error = networkError(GetLastError()); return response; }
    DWORD disabled = WINHTTP_DISABLE_REDIRECTS | WINHTTP_DISABLE_COOKIES;
    DWORD_PTR rawContext = reinterpret_cast<DWORD_PTR>(context.get());
    if (!WinHttpSetOption(handles.request, WINHTTP_OPTION_DISABLE_FEATURE, &disabled, sizeof(disabled)) ||
            !WinHttpSetOption(handles.request, WINHTTP_OPTION_CONTEXT_VALUE, &rawContext, sizeof(rawContext))) {
        response.error = networkError(GetLastError()); return response;
    }
    context->requestLifetime = context;
    if (WinHttpSetStatusCallback(handles.request, HttpContext::callback,
            WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS | WINHTTP_CALLBACK_FLAG_HANDLES, 0) == WINHTTP_INVALID_STATUS_CALLBACK) {
        context->requestLifetime.reset();
        response.error = networkError(GetLastError()); return response;
    }
    handles.registered = true;
    if (cancel || UpdateClock::now() >= deadline) {
        response.error = networkError(ERROR_WINHTTP_OPERATION_CANCELLED); return response;
    }
    constexpr wchar_t headers[] = L"Accept: application/vnd.github+json\r\nX-GitHub-Api-Version: 2026-03-10\r\n";
    if (!WinHttpSendRequest(handles.request, headers, DWORD(-1), WINHTTP_NO_REQUEST_DATA, 0, 0, rawContext)) {
        response.error = networkError(GetLastError()); return response;
    }
    enum class Stage { Sending, Headers, Reading };
    Stage stage = Stage::Sending;
    bool complete = false;
    while (!complete) {
        if (cancel) { response.error = networkError(ERROR_WINHTTP_OPERATION_CANCELLED); break; }
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - UpdateClock::now()).count();
        if (remaining <= 0) { response.error = networkError(ERROR_WINHTTP_TIMEOUT); break; }
        if (context->overflow) { response.error = networkError(ERROR_WINHTTP_INTERNAL_ERROR); break; }
        HttpEvent event;
        if (!context->pop(event)) {
            const DWORD waited = WaitForSingleObject(context->wake, DWORD(remaining < 50 ? remaining : 50));
            if (waited == WAIT_FAILED) { response.error = networkError(GetLastError()); break; }
            continue;
        }
        if (event.status == WINHTTP_CALLBACK_STATUS_REQUEST_ERROR) { response.error = networkError(event.error); break; }
        if (event.status == WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE && stage == Stage::Sending) {
            stage = Stage::Headers;
            if (!WinHttpReceiveResponse(handles.request, nullptr)) { response.error = networkError(GetLastError()); break; }
        } else if (event.status == WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE && stage == Stage::Headers) {
            DWORD status = 0, bytes = sizeof(status);
            if (!WinHttpQueryHeaders(handles.request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                    WINHTTP_HEADER_NAME_BY_INDEX, &status, &bytes, WINHTTP_NO_HEADER_INDEX)) {
                response.error = networkError(GetLastError()); break;
            }
            response.status = status;
            if (status != 200) {
                if (status == 403 || status == 429) response.retryAfterSeconds = retryDelay(handles.request);
                break;
            }
            DWORD length = 0; bytes = sizeof(length);
            if (WinHttpQueryHeaders(handles.request, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                    WINHTTP_HEADER_NAME_BY_INDEX, &length, &bytes, WINHTTP_NO_HEADER_INDEX) && length > updateMaxResponseBytes) {
                response.error = L"Die Releaseinformation ist zu groß."; break;
            }
            stage = Stage::Reading;
            if (!WinHttpReadData(handles.request, context->buffer.data(), DWORD(context->buffer.size()), nullptr)) {
                response.error = networkError(GetLastError()); break;
            }
        } else if (event.status == WINHTTP_CALLBACK_STATUS_READ_COMPLETE && stage == Stage::Reading) {
            if (!event.bytes) { complete = true; continue; }
            if (event.bytes > context->buffer.size() || response.body.size() + event.bytes > updateMaxResponseBytes) {
                response.error = L"Die Releaseinformation ist zu groß."; break;
            }
            response.body.append(context->buffer.data(), event.bytes);
            if (!WinHttpReadData(handles.request, context->buffer.data(), DWORD(context->buffer.size()), nullptr)) {
                response.error = networkError(GetLastError()); break;
            }
        } else { response.error = networkError(ERROR_WINHTTP_INCORRECT_HANDLE_STATE); break; }
    }
    if (!handles.close()) response.error = L"Die Updateprüfung konnte nicht sauber beendet werden.";
    return response;
}
} // namespace update_detail

class UpdateChecker {
public:
    using Deadline = std::chrono::steady_clock::time_point;
    using Fetch = std::function<UpdateResponse(Version, const std::atomic<bool>&, Deadline)>;
    explicit UpdateChecker(Fetch fetch = update_detail::fetchLatest) : fetch_(std::move(fetch)) {}
    UpdateChecker(const UpdateChecker&) = delete;
    UpdateChecker& operator=(const UpdateChecker&) = delete;
    ~UpdateChecker() { shutdown(); }
    bool start(HWND notifyWindow, UINT message, Version current) {
        std::lock_guard<std::mutex> lifecycle(lifecycleMutex_);
        if (running_) return false;
        if (worker_.joinable()) worker_.join();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (update_detail::UpdateClock::now() < retryAt_) {
                result_ = {UpdateState::Failed, L"GitHub begrenzt derzeit die Abfragen. Bitte später erneut versuchen.", {}};
                if (notifyWindow) PostMessageW(notifyWindow, message, 0, 0);
                return false;
            }
            cancelling_ = false;
            result_ = {UpdateState::Checking, L"Suche nach Updates ...", {}};
        }
        running_ = true;
        try { worker_ = std::thread([this, notifyWindow, message, current] { run(notifyWindow, message, current); }); }
        catch (...) {
            running_ = false;
            std::lock_guard<std::mutex> lock(mutex_);
            result_ = {UpdateState::Failed, L"Die Updateprüfung konnte nicht gestartet werden.", {}};
            if (notifyWindow) PostMessageW(notifyWindow, message, 0, 0);
            return false;
        }
        return true;
    }
    void cancel() { std::lock_guard<std::mutex> lock(mutex_); cancelling_ = true; }
    void shutdown() {
        std::lock_guard<std::mutex> lifecycle(lifecycleMutex_);
        cancel();
        if (worker_.joinable()) worker_.join();
    }
    UpdateResult result() const { std::lock_guard<std::mutex> lock(mutex_); return result_; }
    bool running() const { return running_; }
private:
    Fetch fetch_;
    mutable std::mutex mutex_;
    std::mutex lifecycleMutex_;
    std::thread worker_;
    std::atomic<bool> cancelling_{false}, running_{false};
    UpdateResult result_;
    Deadline retryAt_{};
    void run(HWND notifyWindow, UINT message, Version current) {
        UpdateResult next;
        UpdateResponse response;
        const auto deadline = update_detail::UpdateClock::now() + std::chrono::seconds(10);
        try {
            response = fetch_(current, cancelling_, deadline);
            if (update_detail::UpdateClock::now() >= deadline)
                response.error = update_detail::networkError(ERROR_WINHTTP_TIMEOUT);
            if (!response.error.empty()) next = {UpdateState::Failed, response.error, {}};
            else if (response.status == 403 || response.status == 429)
                next = {UpdateState::Failed, L"GitHub begrenzt derzeit die Abfragen. Bitte später erneut versuchen.", {}};
            else if (response.status == 404)
                next = {UpdateState::Failed, L"Auf GitHub ist derzeit keine Releaseinformation verfügbar.", {}};
            else if (response.status != 200)
                next = {UpdateState::Failed, L"Die Updateprüfung ist fehlgeschlagen (HTTP " + std::to_wstring(response.status) + L").", {}};
            else {
                ReleaseInfo release;
                if (!parseLatestRelease(response.body, release) || release.draft || release.prerelease)
                    next = {UpdateState::Failed, L"Die Releaseinformation hat ein unbekanntes oder ungültiges Format.", {}};
                else {
                    const std::string latest = versionString(release.version);
                    const std::wstring label(latest.begin(), latest.end());
                    next = compareVersions(release.version, current) > 0
                        ? UpdateResult{UpdateState::Available, L"Version " + label + L" ist verfügbar.", latest}
                        : UpdateResult{UpdateState::Current, L"Es ist kein neueres Release verfügbar.", latest};
                }
            }
        } catch (...) { next = {UpdateState::Failed, L"Die Updateprüfung konnte nicht abgeschlossen werden.", {}}; }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            running_ = false;
            if (cancelling_) result_ = {UpdateState::Idle, L"Updateprüfung abgebrochen.", {}};
            else {
                result_ = std::move(next);
                if (response.status == 403 || response.status == 429)
                    retryAt_ = update_detail::UpdateClock::now() + std::chrono::seconds(response.retryAfterSeconds ? response.retryAfterSeconds : 60);
                if (notifyWindow) PostMessageW(notifyWindow, message, 0, 0);
            }
        }
    }
};
} // namespace morse

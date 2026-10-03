// SPDX-License-Identifier: GPL-3.0-only
#include "../windows/update_core.hpp"
#include <cstdlib>
#include <iostream>
#include <vector>
#if defined(_WIN32) && !defined(MB_UPDATE_CORE_ONLY)
#include "../windows/update_client.hpp"
#include <condition_variable>
#endif

namespace {
int checks = 0;
void require(bool condition, const char* name) {
    ++checks;
    if (!condition) { std::cerr << "FAIL: " << name << '\n'; std::exit(1); }
}
std::string release(std::string_view tag) {
    return "{\"tag_name\":\"" + std::string(tag) + "\",\"draft\":false,\"prerelease\":false}";
}
#if defined(_WIN32) && !defined(MB_UPDATE_CORE_ONLY)
void finished(morse::UpdateChecker& checker) {
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (checker.running() && std::chrono::steady_clock::now() < until) Sleep(1);
    require(!checker.running(), "injected transport finishes promptly");
}
struct Gate {
    std::mutex mutex;
    std::condition_variable changed;
    bool entered = false;
    bool deadlineValid = false;
    void wait() {
        std::unique_lock<std::mutex> lock(mutex);
        require(changed.wait_for(lock, std::chrono::seconds(3), [&] { return entered; }), "transport entered");
    }
};
#endif
}
int main() {
    using namespace morse;
    Version version;
    for (const auto& text : {"1.2.2", "v1.2.2", "1.2.2p", "v1.2.2p"}) {
        require(parseVersion(text, version) && compareVersions(version, {1,2,2}) == 0, "version prefix and historical public suffix");
    }
    require(parseVersion("4294967295.0.0", version) && version.major == 4294967295u, "uint32 version maximum");
    for (const auto& text : {"", "v", "1", "1.2", "1.2.2.0", "1.2.2-beta", "1.2.2+build", "V1.2.2", "1.2.2P",
            "01.2.2", "1.02.2", "1.2.02", "-1.2.2", "1.-2.2", " 1.2.2", "1.2.2 ", "4294967296.0.0", "1.2.2pp"}) {
        version = {7,8,9};
        require(!parseVersion(text, version) && compareVersions(version, {7,8,9}) == 0, "invalid version rejected without modifying output");
    }
    require(compareVersions({1,2,10}, {1,2,9}) > 0, "numeric patch ordering");
    require(compareVersions({1,10,0}, {1,9,99}) > 0, "numeric minor ordering");
    require(compareVersions({2,0,0}, {1,99,99}) > 0, "major ordering");
    require(compareVersions({1,2,2}, {1,2,2}) == 0, "equal versions");
    require(compareVersions({1,2,1}, {1,2,2}) < 0, "older version");
    require(versionString({1,2,2}) == "1.2.2", "canonical version string");
    ReleaseInfo info;
    require(parseLatestRelease(release("v1.2.3"), info) && info.tag == "v1.2.3" &&
        compareVersions(info.version, {1,2,3}) == 0 && !info.draft && !info.prerelease, "release fields read");
    require(parseLatestRelease(" \n{\"prerelease\":false,\"body\":\"\\\"tag_name\\\":\\\"v99.0.0\\\"\","
        "\"author\":{\"tag_name\":\"v88.0.0\"},\"tag_name\":\"v1.2.3\",\"draft\":false,"
        "\"assets\":[null,true,false,-0.1e+2,{},[]]}\r\n", info) && compareVersions(info.version, {1,2,3}) == 0,
        "nested keys and body text cannot override root version");
    require(parseLatestRelease("{\"\\u0074ag_name\":\"\\u00761.2.3\",\"draft\":false,\"prerelease\":false,"
        "\"body\":\"\\uD83D\\uDE00 ä 😀 \\\\ \\/ \\b \\f \\n \\r \\t\"}", info), "Unicode and escaped key/tag are parsed");
    require(parseLatestRelease("{\"tag_name\":\"v1.2.3\",\"draft\":true,\"prerelease\":true}", info) &&
        info.draft && info.prerelease, "release flags retained for update policy");
    for (const auto& json : {"", "[]", "{}", "{\"tag_name\":\"v1.2.3\"}",
        "{\"tag_name\":null,\"draft\":false,\"prerelease\":false}",
        "{\"tag_name\":\"v1.2.3\",\"draft\":0,\"prerelease\":false}",
        "{\"tag_name\":\"v1.2.3\",\"draft\":false,\"prerelease\":\"false\"}",
        "{\"tag_name\":\"v1.2.3\",\"tag_name\":\"v9.0.0\",\"draft\":false,\"prerelease\":false}",
        "{\"tag_name\":\"v1.2.3\",\"\\u0074ag_name\":\"v9.0.0\",\"draft\":false,\"prerelease\":false}",
        "{\"tag_name\":\"v1.2.3\",\"draft\":false,\"draft\":false,\"prerelease\":false}",
        "{\"tag_name\":\"v1.2.3\",\"draft\":false,\"prerelease\":false,\"prerelease\":false}",
        "{\"tag_name\":\"unknown\",\"draft\":false,\"prerelease\":false}",
        "{\"tag_name\":\"v1.2.3\",\"draft\":false,\"prerelease\":false,}",
        "{\"tag_name\":\"v1.2.3\",\"draft\":false,\"prerelease\":false}{}"}) {
        require(!parseLatestRelease(json, info), "missing duplicate mistyped or malformed release rejected");
    }
    const std::string base = release("v1.2.3");
    for (size_t length = 0; length < base.size(); ++length)
        require(!parseLatestRelease(std::string_view(base).substr(0, length), info), "every truncated document rejected");
    auto withExtra = [&](const std::string& extra) { return base.substr(0, base.size()-1) + ",\"extra\":" + extra + "}"; };
    for (const auto& token : {"01", "-01", "1.", "1e", "1e+", "+1", ".1", "NaN", "undefined", "truex", "[1,]", "{\"x\":}",
            "\"\\x20\"", "\"\\uZZZZ\"", "\"\\uD800\"", "\"\\uDC00\"", "\"\\uD800\\u0041\""})
        require(!parseLatestRelease(withExtra(token), info), "malformed unknown values rejected");
    for (const auto& bytes : {std::string("\xc0\xaf",2), std::string("\xe0\x80\xaf",3), std::string("\xed\xa0\x80",3),
            std::string("\xf4\x90\x80\x80",4), std::string("\x80",1), std::string("\xe2\x82",2), std::string("\n",1)})
        require(!parseLatestRelease(withExtra("\"" + bytes + "\""), info), "invalid UTF8 and raw control characters rejected");
    require(parseLatestRelease(withExtra("{\"" + std::string(100, 'x') + "\":42}"), info), "long unknown field names safely skipped");
    require(parseLatestRelease(withExtra(std::string(31,'[') + "null" + std::string(31,']')), info), "JSON depth boundary accepted");
    require(!parseLatestRelease(withExtra(std::string(32,'[') + "null" + std::string(32,']')), info), "JSON depth overflow rejected");
    std::string large = withExtra("\"\"");
    large.insert(large.size()-2, updateMaxResponseBytes - large.size(), 'x');
    require(large.size() == updateMaxResponseBytes && parseLatestRelease(large, info), "response size boundary accepted");
    large.insert(large.size()-2, 1, 'x');
    require(!parseLatestRelease(large, info), "response size overflow rejected");
    require(!parseLatestRelease(release(std::string(65,'1')), info), "overlong tag rejected");
#if defined(_WIN32) && !defined(MB_UPDATE_CORE_ONLY)
    auto checkerFor = [](UpdateResponse response) {
        return UpdateChecker([response](Version, const std::atomic<bool>&, UpdateChecker::Deadline) { return response; });
    };
    for (const auto& item : std::vector<std::pair<std::string,UpdateState>>{
            {"v1.2.3",UpdateState::Available}, {"v1.2.2",UpdateState::Current}, {"v1.2.1p",UpdateState::Current}}) {
        auto checker = checkerFor({200, release(item.first), {}, 0});
        require(checker.start(nullptr, WM_APP+13, {1,2,2}), "checker starts injected fetch");
        finished(checker);
        require(checker.result().state == item.second && !checker.result().latestVersion.empty(), "correct check result and latest version");
    }
    for (const auto& response : std::vector<UpdateResponse>{{404,{}, {}, 0}, {500,{}, {}, 0}, {200,"{}", {}, 0},
            {200,"{\"tag_name\":\"v2.0.0\",\"draft\":true,\"prerelease\":false}", {}, 0},
            {200,"{\"tag_name\":\"v2.0.0\",\"draft\":false,\"prerelease\":true}", {}, 0}, {0,{},L"offline",0}}) {
        auto checker = checkerFor(response);
        require(checker.start(nullptr, WM_APP+13, {1,2,2}), "error check starts");
        finished(checker);
        require(checker.result().state == UpdateState::Failed && checker.result().latestVersion.empty(), "invalid release and transport errors reported");
    }
    for (unsigned status : {403u,429u}) {
        auto checker = checkerFor({status,{}, {}, 60});
        require(checker.start(nullptr, WM_APP+13, {1,2,2}), "rate limit fetch starts");
        finished(checker);
        require(checker.result().state == UpdateState::Failed && !checker.start(nullptr, WM_APP+13, {1,2,2}), "rate limit cooldown prevents repeated request");
    }
    {
        auto checker = checkerFor({200,release("v1.2.3"),{},0});
        HWND target = CreateWindowExW(0,L"STATIC",L"",0,0,0,0,0,HWND_MESSAGE,nullptr,GetModuleHandleW(nullptr),nullptr);
        require(target != nullptr, "completion message target created");
        constexpr UINT message = WM_APP + 14;
        for (int repeat = 0; repeat < 3; ++repeat) {
            require(checker.start(target,message,{1,2,2}), "completed checker can be restarted");
            MSG queued{};
            const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(3);
            bool notified = false;
            while (std::chrono::steady_clock::now() < until) {
                if (PeekMessageW(&queued,target,message,message,PM_REMOVE)) { notified = true; break; }
                Sleep(1);
            }
            require(notified && !checker.running() && checker.result().state == UpdateState::Available,
                "completion publishes ready state before notifying GUI");
        }
        checker.shutdown(); DestroyWindow(target);
    }
    {
        Gate gate;
        UpdateChecker checker([&](Version, const std::atomic<bool>& cancel, UpdateChecker::Deadline deadline) {
            {
                std::lock_guard<std::mutex> lock(gate.mutex);
                const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
                gate.deadlineValid = left > 9000 && left <= 10000;
                gate.entered = true; gate.changed.notify_one();
            }
            while (!cancel) Sleep(1);
            return UpdateResponse{200,release("v9.0.0"),{},0};
        });
        HWND target = CreateWindowExW(0,L"STATIC",L"",0,0,0,0,0,HWND_MESSAGE,nullptr,GetModuleHandleW(nullptr),nullptr);
        require(target != nullptr, "message target created");
        constexpr UINT message = WM_APP + 13;
        require(checker.start(target,message,{1,2,2}), "cancellable transport starts");
        gate.wait();
        require(gate.deadlineValid, "fetch receives ten second deadline");
        require(checker.result().state == UpdateState::Checking && !checker.start(target,message,{1,2,2}), "only one check at a time");
        const auto before = std::chrono::steady_clock::now();
        checker.cancel(); checker.shutdown();
        require(std::chrono::steady_clock::now() - before < std::chrono::seconds(1), "shutdown cancels and joins promptly");
        require(!checker.running() && checker.result().state == UpdateState::Idle, "cancel discards returned newer release");
        MSG queued{};
        require(!PeekMessageW(&queued,target,message,message,PM_REMOVE), "cancelled check posts no late result");
        DestroyWindow(target);
    }
    {
        UpdateChecker checker([](Version,const std::atomic<bool>&,UpdateChecker::Deadline) -> UpdateResponse {
            throw std::runtime_error("simulated transport failure");
        });
        require(checker.start(nullptr, WM_APP+13, {1,2,2}), "throwing fetch starts");
        finished(checker);
        require(checker.result().state == UpdateState::Failed, "transport exception contained");
    }
    {
        UpdateChecker checker([](Version,const std::atomic<bool>& cancel,UpdateChecker::Deadline deadline) {
            while (!cancel && std::chrono::steady_clock::now() < deadline) Sleep(1);
            return UpdateResponse{200,release("v9.0.0"),{},0};
        });
        require(checker.start(nullptr,WM_APP+13,{1,2,2}), "deadline transport starts");
        const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(12);
        while (checker.running() && std::chrono::steady_clock::now() < until) Sleep(1);
        require(!checker.running() && checker.result().state == UpdateState::Failed && checker.result().latestVersion.empty(),
            "a result arriving after the total deadline is never offered as an update");
    }
#endif
    std::cout << "PASS: " << checks << " version / release / update lifecycle checks\n";
}

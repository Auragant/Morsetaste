// SPDX-License-Identifier: GPL-3.0-only
#include "../windows/bridge_core.hpp"
#ifdef _WIN32
#include "../windows/keyboard_output.hpp"
#endif
#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

int checks = 0;
void require(bool condition, const char* name) {
    ++checks;
    if (!condition) { std::cerr << "FAIL: " << name << '\n'; std::exit(1); }
}

int main() {
    using morse::Bridge;
    using morse::FrameParser;
    {
        morse::PulseHistory history;
        history.receive(true, 0); history.receive(false, 100);
        require(history.samples.empty(), "initial held contact has no known start");
        history.receive(true, 200); history.receive(true, 220); history.receive(false, 247);
        history.receive(false, 260);
        require(history.samples.size() == 1 && history.samples.front().duration == 47,
                "47ms pulse counted once despite heartbeats");
        history.receive(true, 300); history.receive(false, 450);
        require(history.samples.back().duration == 150, "150ms pulse measured independently");
        history.receive(true, 500); history.interrupt(); history.receive(false, 900);
        require(history.samples.size() == 2, "disconnect does not complete a pulse");
        history.prune(300246);
        require(history.samples.size() == 2, "history survives live timeline and expires at boundary");
        history.prune(300247);
        require(history.samples.size() == 1, "five minute boundary removes oldest sample");
        history.prune(300450);
        require(history.samples.empty(), "idle history expires completely");
        history.receive(true, 300500); history.receive(false, 300500);
        require(history.samples.size() == 1 && history.samples.front().duration == 0,
                "same read zero duration remains visible");
        for (uint64_t i=0; i<20000; ++i) {
            history.receive(true, 300501+i*2); history.receive(false, 300502+i*2);
        }
        require(history.samples.size() == 16384, "pulse history memory bounded");
        history.prune(700000);
        history.receive(true, 700000); history.receive(false, 701000);
        require(history.samples.size() == 1 && history.samples.front().duration == 1000,
                "exactly one second remains in histogram");
        history.receive(true, 702000); history.receive(false, 703001);
        history.receive(true, 704000); history.receive(false, 714000);
        require(history.samples.size() == 1, "holds over one second never enter histogram");
        history.receive(true, 715000); history.receive(false, 715047);
        require(history.samples.size() == 2 && history.samples.back().duration == 47,
                "normal pulse after excluded hold still recorded");
    }
    {
        const std::string stream = "bootloader?\nJUNKER/1 U\r\nJUNKER/1 D\nJUNKER/1 U\n";
        for (size_t split=0; split<=stream.size(); ++split) {
            FrameParser parser; std::vector<bool> frames;
            auto receive = [&](bool d) { frames.push_back(d); };
            parser.feed(stream.data(), split, receive);
            parser.feed(stream.data()+split,stream.size()-split,receive);
            require(frames == std::vector<bool>({false,true,false}), "arbitrary USB split / CRLF / noise");
        }
        FrameParser parser; int count = 0;
        const std::string bad = "D\nU\nJUNKER/2 D\nJUNKER/1 D EXTRA\nJUNKER/1 \rD\n" +
            std::string(80,'x') + "JUNKER/1 D\n";
        parser.feed(bad.data(),bad.size(),[&](bool) { ++count; });
        require(count == 0, "reject foreign, corrupt and overlong lines");
        parser.feed("JUNKER/1 U\n",11,[&](bool d) { require(!d,"recover after garbage"); ++count; });
        require(count == 1,"parser recovery");
    }
    {
        std::vector<bool> events;
        Bridge b([&](bool d) { events.push_back(d); return true; });
        b.receive(true,0); b.receive(true,250);
        require(events.empty(),"held key at connect must not inject");
        b.receive(false,300); b.receive(true,310); b.receive(true,560); b.receive(false,600);
        require(events == std::vector<bool>({true,false}),"one down/up, no heartbeat repeat");
        b.disconnect(); require(events.size()==2,"no unsolicited key-up");
        b.receive(true,650); require(events.size()==2,"reconnect requires release");
    }
    {
        std::vector<bool> events;
        Bridge b([&](bool d) { events.push_back(d); return true; });
        FrameParser parser;
        const std::string batch = "JUNKER/1 U\nJUNKER/1 D\nJUNKER/1 U\nJUNKER/1 D\nJUNKER/1 U\n";
        parser.feed(batch.data(),batch.size(),[&](bool d) { b.receive(d,10); });
        require(events == std::vector<bool>({true,false,true,false}),"multiple short pulses in one USB read preserved");
    }
    {
        std::vector<bool> events;
        Bridge b([&](bool d) { events.push_back(d); return true; });
        b.receive(false,0); b.receive(true,10);
        b.tick(1009); require(b.outputDown,"watchdog not early");
        b.tick(1010); require(!b.connected && !b.outputDown,"exact watchdog timeout releases");
        require(events == std::vector<bool>({true,false}),"timeout key-up");
        b.receive(false,2000); b.receive(true,2010);
        for (uint64_t t=2250; t<10000; t+=250) { b.receive(true,t); b.tick(t); }
        require(b.outputDown,"long intentional hold preserved with heartbeat");
        b.disconnect(); require(!b.outputDown,"disconnect / close releases");
    }
    {
        std::vector<bool> events;
        Bridge b([&](bool d) { events.push_back(d); return true; });
        b.receive(false,0); b.receive(true,1); b.enable(false);
        require(!b.outputDown,"pause immediately releases");
        b.receive(false,2); b.receive(true,3); require(events.size()==2,"paused states still monitored, never sent");
        b.enable(true); b.receive(true,4); require(events.size()==2,"resume held must wait");
        b.receive(false,5); b.receive(true,6); require(b.outputDown,"resume after release");
        b.block(true); require(!b.outputDown,"focus / lock / suspend releases");
        b.block(false); b.receive(true,7); require(!b.outputDown,"focus change held must wait");
        b.receive(false,8); b.receive(true,9); require(b.outputDown,"focus rearm after release");
    }
    {
        bool fail = true; std::vector<bool> accepted;
        Bridge b([&](bool d) { if (fail) return false; accepted.push_back(d); return true; });
        b.receive(false,0); b.receive(true,1);
        require(b.fault && !b.outputDown,"failed SendInput never shown as sent");
        fail = false; b.receive(false,2); b.receive(true,3);
        require(accepted.empty(),"output fault stays latched");
        b.enable(false); b.enable(true); b.receive(false,4); b.receive(true,5);
        require(b.outputDown && !b.fault,"pause/resume clears down fault");
        fail = true; b.disconnect(); require(b.outputDown && b.fault,"failed key-up retains ownership");
        fail = false; b.tick(6); require(!b.outputDown,"retry failed key-up");
        require(accepted == std::vector<bool>({true,false}),"only owned down/up pair");
    }
    {
        unsigned activeKey = 32;
        std::vector<std::pair<unsigned, bool>> events;
        Bridge b([&](bool down) { events.emplace_back(activeKey, down); return true; });
        b.receive(false, 0); b.receive(true, 1);
        if (b.prepareOutputChange()) activeKey = 0xa2;
        require(activeKey == 0xa2 && !b.outputDown && !b.armed,
                "key change releases old output and disarms held contact");
        require(events == std::vector<std::pair<unsigned, bool>>({{32, true}, {32, false}}),
                "key change releases the old key before changing identity");
        b.receive(true, 2);
        require(events.size() == 2, "held contact cannot press the newly selected key");
        b.receive(false, 3); b.receive(true, 4); b.receive(false, 5);
        require(events == std::vector<std::pair<unsigned, bool>>(
                    {{32, true}, {32, false}, {0xa2, true}, {0xa2, false}}),
                "new output key works only after contact release");
        require(b.prepareOutputChange() && !b.armed,
                "idle output change also requires a fresh released contact");
    }
    {
        unsigned activeKey = 32;
        bool failUp = true;
        std::vector<std::pair<unsigned, bool>> events;
        Bridge b([&](bool down) {
            if (!down && failUp) return false;
            events.emplace_back(activeKey, down);
            return true;
        });
        b.receive(false, 0); b.receive(true, 1);
        if (b.prepareOutputChange()) activeKey = 0xa2;
        require(activeKey == 32 && b.outputDown && b.fault && !b.armed,
                "failed change key-up retains old output identity and ownership");
        b.receive(true, 2); b.tick(3);
        require(events.size() == 1 && activeKey == 32,
                "failed key-up never presses requested replacement key");
        failUp = false;
        if (b.prepareOutputChange()) activeKey = 0xa2;
        require(activeKey == 0xa2 && !b.outputDown && events.back() == std::make_pair(32u, false),
                "retry releases original key before replacement becomes active");
        b.enable(false); b.enable(true); b.receive(true, 4);
        require(events.size() == 2, "fault reset with held contact does not press replacement");
        b.receive(false, 5); b.receive(true, 6); b.disconnect();
        require(events == std::vector<std::pair<unsigned, bool>>(
                    {{32, true}, {32, false}, {0xa2, true}, {0xa2, false}}),
                "replacement down/up pair follows successful fault reset and release");
    }
#ifdef _WIN32
    {
        std::vector<INPUT> events;
        unsigned mapCalls = 0;
        morse::KeyboardOutput output([&](const INPUT& input) { events.push_back(input); return true; },
            [&](unsigned vk) {
                ++mapCalls;
                if (vk == VK_SPACE) return 0x39u;
                if (vk == VK_LCONTROL) return 0x1du;
                if (vk == VK_RCONTROL) return 0xe01du;
                return 0u;
            });
        require(output.send(VK_SPACE, false) && events.empty(), "adapter emits no unowned key-up");
        require(output.send(VK_SPACE, true), "adapter accepts SPACE down");
        require(events.size() == 1 && events.back().type == INPUT_KEYBOARD &&
                events.back().ki.wVk == 0 && events.back().ki.wScan == 0x39 &&
                events.back().ki.dwFlags == KEYEVENTF_SCANCODE,
                "SPACE uses one scan-code key-down event");
        require(!output.send(VK_LCONTROL, true) && events.size() == 1 && mapCalls == 1,
                "adapter rejects a second down while it owns a key");
        require(output.send(VK_LCONTROL, false) && events.size() == 2 && mapCalls == 1 &&
                events.back().ki.wScan == 0x39 &&
                events.back().ki.dwFlags == (KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP),
                "key-up retains owned scan code even when requested virtual key changed");
        require(output.send(VK_SPACE, false) && events.size() == 2,
                "duplicate key-up creates no event");
        require(output.send(VK_LCONTROL, true) && events.back().ki.wScan == 0x1d &&
                events.back().ki.dwFlags == KEYEVENTF_SCANCODE,
                "Left CTRL is the non-extended control scan code");
        require(output.send(VK_LCONTROL, false) &&
                events.back().ki.dwFlags == (KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP),
                "Left CTRL release is non-extended");
        require(output.send(VK_RCONTROL, true) && events.back().ki.wScan == 0x1d &&
                events.back().ki.dwFlags == (KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY),
                "Right CTRL preserves E0 extended flag");
        require(output.send(VK_RCONTROL, false) && events.back().ki.wScan == 0x1d &&
                events.back().ki.dwFlags == (KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP),
                "Right CTRL release preserves E0 extended flag");
    }
    {
        bool fail = true;
        unsigned mapping = 0xe01d, mapCalls = 0;
        std::vector<INPUT> attempts;
        morse::KeyboardOutput output([&](const INPUT& input) { attempts.push_back(input); return !fail; },
            [&](unsigned) { ++mapCalls; return mapping; });
        require(!output.send(VK_RCONTROL, true) && attempts.size() == 1,
                "failed injected down does not gain ownership");
        require(output.send(VK_RCONTROL, false) && attempts.size() == 1,
                "failed down never causes unsolicited release");
        fail = false;
        require(output.send(VK_RCONTROL, true), "down retries after failed emission");
        mapping = 0x2a; fail = true;
        require(!output.send(VK_LCONTROL, false) && attempts.back().ki.wScan == 0x1d &&
                attempts.back().ki.dwFlags == (KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP),
                "failed release uses original scan and extended flag after layout change");
        const auto attemptCount = attempts.size();
        require(!output.send(VK_LCONTROL, true) && attempts.size() == attemptCount && mapCalls == 2,
                "failed release retains ownership and blocks a new down");
        fail = false;
        require(output.send(VK_LCONTROL, false) && mapCalls == 2 &&
                attempts.back().ki.wScan == 0x1d &&
                attempts.back().ki.dwFlags == (KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP),
                "successful retry still releases original layout identity");
        require(output.send(VK_LCONTROL, true) && mapCalls == 3 && attempts.back().ki.wScan == 0x2a &&
                attempts.back().ki.dwFlags == KEYEVENTF_SCANCODE,
                "fresh down uses new layout mapping after successful release");
        require(output.send(VK_LCONTROL, false), "new layout key is released");
    }
    {
        unsigned mapping = 0, emitCalls = 0, mapCalls = 0;
        morse::KeyboardOutput output([&](const INPUT&) { ++emitCalls; return true; },
            [&](unsigned) { ++mapCalls; return mapping; });
        require(!output.send(VK_PAUSE, true), "unmapped virtual key is rejected");
        mapping = 0xe11d;
        require(!output.send(VK_PAUSE, true), "E1 multi-byte mapping is rejected");
        mapping = 0xe000;
        require(!output.send(VK_PAUSE, true), "zero scan code with E0 prefix is rejected");
        mapping = 0xe21d;
        require(!output.send(VK_PAUSE, true), "unsupported mapping prefix is rejected");
        require(!output.send(0, true) && !output.send(0x100, true) && mapCalls == 4 && emitCalls == 0,
                "invalid virtual keys never reach mapping or emission");
        require(output.send(VK_PAUSE, false) && emitCalls == 0,
                "rejected mappings create no owned key to release");
    }
#endif
    std::cout << "PASS: " << checks << " protocol / output checks\n";
}

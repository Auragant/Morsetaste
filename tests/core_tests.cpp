// SPDX-License-Identifier: GPL-3.0-only
#include "../windows/bridge_core.hpp"
#include <cstdlib>
#include <iostream>
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
    std::cout << "PASS: " << checks << " protocol / output checks\n";
}

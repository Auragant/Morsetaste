// SPDX-License-Identifier: GPL-3.0-only
#include <Arduino.h>
#include "../firmware/JunkerSpace/JunkerSpace.ino"
#include <cstdlib>
#include <iostream>
int checks=0;
void require(bool value,const char* name) {
    ++checks;
    if (!value) { std::cerr << "FAIL: " << name << '\n'; std::exit(1); }
}
void step(uint32_t time,bool contact) { fakeNow=time; fakeContact=contact; loop(); }
void boot(uint32_t time,bool contact,bool enable) {
    fakeNow=time; fakeContact=contact; fakeBuzzerSwitchClosed=enable;
    Serial.lines.clear(); fakeBuzzer=HIGH; // setup must actively clear the output.
    setup();
}
void buzzerStep(uint32_t time,bool contact,bool enable) {
    fakeBuzzerSwitchClosed=enable; step(time,contact);
}
int main() {
    setup();
    require(fakeKeyMode==INPUT_PULLUP && Serial.baud==115200,"D2 pullup and baud rate");
    require(Serial.lines==std::vector<std::string>({"JUNKER/1 U"}),"initial identification / released");
    step(10,true); step(11,false); step(12,true); step(16,true);
    require(Serial.lines.size()==1,"bounce produces no early edge");
    step(17,true);
    require(Serial.lines.back()=="JUNKER/1 D" && Serial.lines.size()==2 && fakeLed==HIGH,"press after 5ms stable");
    step(20,false); step(21,true); step(22,false); step(27,false);
    require(Serial.lines.back()=="JUNKER/1 U" && Serial.lines.size()==3 && fakeLed==LOW,"debounced release");
    step(276,false); require(Serial.lines.size()==3,"heartbeat not early");
    step(277,false); require(Serial.lines.size()==4 && Serial.lines.back()=="JUNKER/1 U","heartbeat refreshes state");
    step(300,true); step(302,false); step(309,false);
    require(Serial.lines.size()==4,"sub-debounce glitch ignored");
    fakeNow=0xfffffffdU; fakeContact=false; Serial.lines.clear(); setup();
    step(0xfffffffeU,true); step(2,true); require(Serial.lines.size()==1,"wrap debounce not early");
    step(3,true); require(Serial.lines.back()=="JUNKER/1 D" && Serial.lines.size()==2,"millis wrap handled");
    step(253,true); require(Serial.lines.size()==3,"heartbeat after wrap");
    fakeNow=0; fakeContact=true; Serial.lines.clear(); setup();
    require(Serial.lines.back()=="JUNKER/1 D" && fakeLed==HIGH,"startup held truthfully reported");
    require(fakeBuzzer==LOW,"absent/open enable switch keeps buzzer off even with key held");

    boot(0,false,false);
    require(fakeBuzzerMode==OUTPUT && fakeBuzzerSwitchMode==INPUT_PULLUP,"D8 driver output and D4 enable pullup");
    require(fakeBuzzer==LOW,"buzzer is actively off at startup");
    buzzerStep(10,false,true); buzzerStep(29,false,true);
    require(!stableBuzzerEnabled && fakeBuzzer==LOW,"enable debounce not early");
    buzzerStep(30,false,true);
    require(stableBuzzerEnabled && fakeBuzzer==LOW,"enabled but released key remains silent");
    require(Serial.lines==std::vector<std::string>({"JUNKER/1 U"}),"switch changes do not create key events");
    buzzerStep(35,true,true); buzzerStep(39,true,true);
    require(fakeBuzzer==LOW && fakeLed==LOW,"buzzer uses same key debounce as LED and serial");
    buzzerStep(40,true,true);
    require(fakeBuzzer==HIGH && fakeLed==HIGH && Serial.lines.back()=="JUNKER/1 D","active buzzer follows accepted key-down");
    buzzerStep(45,false,true); buzzerStep(50,false,true);
    require(fakeBuzzer==LOW && fakeLed==LOW && Serial.lines.back()=="JUNKER/1 U","release stops buzzer with serial release");

    buzzerStep(60,true,true); buzzerStep(65,true,true);
    const size_t beforeSwitch=Serial.lines.size();
    buzzerStep(70,true,false); buzzerStep(89,true,false);
    require(fakeBuzzer==HIGH,"switch off is independently debounced");
    buzzerStep(90,true,false);
    require(fakeBuzzer==LOW && fakeLed==HIGH && stableDown,"switch off while held stops only sound");
    require(Serial.lines.size()==beforeSwitch,"switch off does not release Space");
    buzzerStep(95,true,true); buzzerStep(115,true,true);
    require(fakeBuzzer==HIGH && Serial.lines.size()==beforeSwitch,"switch on while held resumes only sound");
    buzzerStep(315,true,true);
    require(fakeBuzzer==HIGH && Serial.lines.size()==beforeSwitch+1 && Serial.lines.back()=="JUNKER/1 D","buzzer hold preserves heartbeat");

    boot(0,false,false);
    buzzerStep(10,false,true); buzzerStep(12,false,false); buzzerStep(14,false,true);
    buzzerStep(20,true,true); buzzerStep(25,true,true);
    require(fakeLed==HIGH && fakeBuzzer==LOW && Serial.lines.back()=="JUNKER/1 D","switch bounce never delays Morse event");
    buzzerStep(33,true,true); require(fakeBuzzer==LOW,"switch debounce restarts on last bounce");
    buzzerStep(34,true,true); require(fakeBuzzer==HIGH,"stable switch eventually enables held contact");
    buzzerStep(40,false,false); buzzerStep(42,false,true); buzzerStep(45,false,true);
    require(fakeBuzzer==LOW && Serial.lines.back()=="JUNKER/1 U","key release stops sound during switch bounce");

    boot(0,true,true);
    require(fakeBuzzer==LOW,"boot with key and switch closed still starts silent");
    buzzerStep(19,true,true); require(fakeBuzzer==LOW,"boot waits full switch debounce");
    buzzerStep(20,true,true); require(fakeBuzzer==HIGH,"boot held tones after stable enable");
    boot(0,false,true); buzzerStep(20,false,true);
    require(fakeBuzzer==LOW,"boot with enabled switch alone never beeps");

    boot(0xfffffff0U,true,true);
    buzzerStep(3,true,true); require(fakeBuzzer==LOW,"switch debounce across millis wrap not early");
    buzzerStep(4,true,true); require(fakeBuzzer==HIGH,"switch debounce handles millis wrap");
    boot(0,true,false); buzzerStep(5000,true,false);
    require(fakeBuzzer==LOW && fakeLed==HIGH && Serial.lines.back()=="JUNKER/1 D","long hold with missing switch keeps serial and LED working silently");
    std::cout << "PASS: " << checks << " actual sketch checks (Arduino I/O stub)\n";
}

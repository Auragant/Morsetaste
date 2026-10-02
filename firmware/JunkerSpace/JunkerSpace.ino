// SPDX-License-Identifier: GPL-3.0-only
// LLM-assisted implementation; Arduino core has separate licenses. See LICENSE and DISCLAIMER.md.
/*
 * JunkerSpace 1.1.0p: Junker M.T. -> Arduino Nano / ATmega328P -> MorseBridge
 * Potentialfreier Kontakt zwischen D2 und GND. Keine externe Spannung!
 * Optional: Schalter D4 <-> GND gibt den Mithoerton frei (geschlossen = ein).
 * D8 steuert einen aktiven 5-V-Summer ueber eine NPN-Transistorstufe.
 * TMB12A05 nicht direkt aus D8 speisen! Beschaltung: docs/SUMMER.md.
 * Board: Arduino Nano; Prozessor: ATmega328P (ggf. Old Bootloader).
 * Keine zusaetzlichen Arduino-Bibliotheken erforderlich.
 */
#include <Arduino.h>

constexpr uint8_t KEY_PIN = 2;
constexpr uint8_t BUZZER_ENABLE_PIN = 4;
constexpr uint8_t BUZZER_PIN = 8;
constexpr uint32_t DEBOUNCE_MS = 5;
constexpr uint32_t SWITCH_DEBOUNCE_MS = 20;
constexpr uint32_t HEARTBEAT_MS = 250;

bool rawDown = false;
bool stableDown = false;
uint32_t rawChangedAt = 0;
uint32_t lastSentAt = 0;
bool rawBuzzerEnabled = false;
bool stableBuzzerEnabled = false;
uint32_t buzzerSwitchChangedAt = 0;

void updateBuzzer(uint32_t now) {
  const bool switchReading = digitalRead(BUZZER_ENABLE_PIN) == LOW;
  if (switchReading != rawBuzzerEnabled) {
    rawBuzzerEnabled = switchReading;
    buzzerSwitchChangedAt = now;
  }
  if (rawBuzzerEnabled != stableBuzzerEnabled &&
      uint32_t(now - buzzerSwitchChangedAt) >= SWITCH_DEBOUNCE_MS) {
    stableBuzzerEnabled = rawBuzzerEnabled;
  }
  // Aktiver Summer: Gleichspannung ein/aus, kein tone() und keine PWM.
  // Er folgt dem entprellten Morse-Kontakt, unabhaengig von PC/Space/Pause.
  digitalWrite(BUZZER_PIN, stableBuzzerEnabled && stableDown ? HIGH : LOW);
}

void sendState(uint32_t now) {
  // Jede Meldung identifiziert die Firmware und enthaelt den ganzen Zustand.
  // Der PC sendet keine Abfragen; auch eine neu gestartete App erkennt uns.
  Serial.println(stableDown ? F("JUNKER/1 D") : F("JUNKER/1 U"));
  lastSentAt = now;
}

void setup() {
  // Ausgangslatch vor dem Umschalten auf OUTPUT auf LOW setzen.
  // Ein externer 10-kOhm-Basis-Pulldown haelt den Summer auch beim Reset aus.
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(KEY_PIN, INPUT_PULLUP);
  pinMode(BUZZER_ENABLE_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  rawDown = digitalRead(KEY_PIN) == LOW;
  stableDown = rawDown;
  rawChangedAt = millis();
  rawBuzzerEnabled = digitalRead(BUZZER_ENABLE_PIN) == LOW;
  stableBuzzerEnabled = false; // Beim Start erst nach stabiler Freigabe toenen.
  buzzerSwitchChangedAt = rawChangedAt;
  digitalWrite(LED_BUILTIN, stableDown ? HIGH : LOW);
  sendState(rawChangedAt);
}

void loop() {
  const uint32_t now = millis();
  const bool reading = digitalRead(KEY_PIN) == LOW;
  if (reading != rawDown) {
    rawDown = reading;
    rawChangedAt = now;
  }
  // Unsigned-Differenzen funktionieren auch beim millis()-Ueberlauf.
  if (rawDown != stableDown && uint32_t(now - rawChangedAt) >= DEBOUNCE_MS) {
    stableDown = rawDown;
    digitalWrite(LED_BUILTIN, stableDown ? HIGH : LOW);
    sendState(now);
  }
  if (uint32_t(now - lastSentAt) >= HEARTBEAT_MS) {
    sendState(now);
  }
  updateBuzzer(now);
}

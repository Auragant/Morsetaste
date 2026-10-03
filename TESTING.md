# Prüfstand

## Version 1.2.1 – 04.10.2026

Der Eigentümer bestätigte den erforderlichen User-Test mit „Test bestanden“.
Getesteter Aufbau: Windows mit digitalem optischem Audioausgang.
Die veröffentlichte EXE ist unverändert die getestete Datei:

```text
Version: 1.2.1
Quellstand der getesteten EXE: 5c23c9c7aab0eff1245e4210f71c566bbf302fd1
SHA-256: 7cc1fad98cdec34650a05424c763dfdad279ee7f87b71ae1f8cfc21f7a44c1e0
```

Spätere Änderungen zur Veröffentlichung betreffen Dokumentation und Paketbau.
Der berichtete User-Test gilt für diesen Aufbau; einzelne Testschritte oder
weitere Audiogeräte wurden nicht gesondert bestätigt.

- Windows-Build mit Warnungen als Fehlern; **85 Kerncode-, 16 Sinus-/Hüllkurven-,
  150 Audioformat-/Puffer-, 30 Audio-Thread- und 37 Firmwareprüfungen bestanden**.
- Die Audio-Thread-Prüfungen führen den echten Thread gegen einen simulierten
  Transport aus: Startfüllung, freie Pufferbereiche, Kontakt/Stereo/Silence,
  Aus-/Einschalten, Suspend/Resume, Beenden und Fehler ohne Neustartschleife.
  Dabei werden keine realen Audiogeräte geöffnet.
- Formate: 44,1/48/96/192 kHz, Float32/64, PCM16/24/32 und 24 gültige Bits im
  32-Bit-Container; Frequenz, Kanalzuordnung, Hüllkurve und Schreibgrenzen geprüft.
- GUI-/Demo-Worker-Test bestanden und Darstellung visuell geprüft: Frequenzeingaben,
  Checkbox, Pause/Kontaktweitergabe und vollständiges Beenden. Keine COM-Zugriffe
  oder echten Tastatureingaben. EXE-/Produktversion 1.2.1; nur System-DLLs.
- **11 Benachrichtigungs-Szenarien bestanden**, unter anderem Neuanlage, Zuweisung,
  Wiederholungsfehler, Auflösung und Schutz vor veralteten/teilweisen Läufen.
- Echter Nano-Build: **2.466 Byte Flash / 204 Byte RAM**, Arduino AVR Boards 1.8.8,
  AVR-GCC 7.3.0-atmel3.6.1-arduino7, FQBN `arduino:avr:nano:cpu=atmega328`.
  Firmwarefunktion und HEX unverändert; erneutes Flashen ist nicht erforderlich.
- Das Komplettpaket enthält eigene und verwendete Arduino-Core-/Variant-Quellen,
  Buildskripte, Tests, EXE/HEX und vollständige Lizenzhinweise. Es enthält keine
  Toolprogramme, Git-Historie, Treiber, Buildreste oder Bootloader-Binärdateien.

Vor einem Release müssen der User-Test sowie GitHub Security checks und CodeQL
für den zu veröffentlichenden Quellstand erfolgreich sein. Die Prüfungen sind
unter [GitHub Actions](https://github.com/Auragant/Morsetaste/actions) einsehbar.

## Frühere Praxistests und gemeldeter Audiofehler

Die Nano-/Junker-/TMB12A05-Kette samt Transistorstufe und dem konkreten Zielprogramm
wurde am 03.10.2026 vom Eigentümer erfolgreich getestet.

Zu **1.2.0p** meldete der Eigentümer anschließend rauen/unpassenden PC-Ton und ein
Einfrieren von Windows. Der folgende Neustart wurde mit Bugcheck **0xD1**
protokolliert. Betroffener Treiber und Zusammenhang mit dem Mithörton sind nicht
geklärt. Der damalige API-/Sinustest belegte keine saubere hörbare Wiedergabe.
Ein Warnhinweis wurde veröffentlicht; die alten Binärdownloads sind zurückgezogen.
In einer vorhandenen 1.2.0p-Kopie den PC-Mithörton ausgeschaltet lassen und auf
1.2.1 aktualisieren. Der bestandene 1.2.1-Test ist keine Zusicherung gegen
Windows-/Treiberabstürze auf anderen Systemen.

## Security und Grenzen

Cppcheck prüft C++ und den tatsächlichen Sketch über dessen I/O-Teststub.
Gitleaks prüft die vollständige ausgecheckte Git-Historie mit geschwärzter Ausgabe;
der Scanner ist per Version und SHA-256 festgelegt. Gemeinsamer Kerncode,
Tongenerator, Audioformate und Firmwaretests laufen zusätzlich unter Linux mit
AddressSanitizer/UndefinedBehaviorSanitizer. CodeQL prüft C/C++, JavaScript und Actions.

Alias-/GitHub-Noreply-Metadaten, private Angaben im Release-Inventar, Lizenztexte
und Quellenumfang wurden vor der ersten öffentlichen Ausgabe geprüft.
Private vulnerability reporting, Secret Scanning und Push Protection wurden
aktiviert. Der tatsächliche Empfang der GitHub-Security-Testbenachrichtigung
wurde am 03.10.2026 vom Eigentümer bestätigt. Die Einordnung eines früheren
CodeQL-Testhinweises und die Grenzen der Scanner stehen in `SECURITY.md`.

Die Tests ersetzen keine unabhängige elektrische Zertifizierung oder Prüfung
aller Windows-/Treiber-/Zielprogrammvarianten. Scanner prüfen weder weltweite
Urheberrechte noch Patente und garantieren keine Fehlerfreiheit.

# Prüfstand

## Version 1.2.1 – Teststand – 04.10.2026

- Windows-Build mit Warnungen als Fehlern; 85 Kerncode-, 16 Sinus-/Hüllkurven-,
  150 Format-/Puffer- und 37 Firmwareprüfungen bestanden.
- 30 stumme Prüfungen führen den echten Audio-Thread gegen einen simulierten
  Transport aus: Startfüllung, freie Pufferbereiche, Kontakt/Stereo/Silence,
  Aus-/Einschalten, Suspend/Resume, Beenden und Fehler ohne Neustartschleife.
- Formate: 44,1/48/96/192 kHz, Float32/64, PCM16/24/32 und 24 gültige Bits im
  32-Bit-Container; Sinusform, Frequenz, Kanalzuordnung, Hüllkurve und
  Schreibgrenzen geprüft. Diese Tests prüfen keine realen Audiotreiber.
- GUI-/Demo-Worker-Test erfolgreich und Darstellung visuell geprüft. Frequenz-
  eingaben, Checkbox, Pause/Kontaktweitergabe und vollständiges Beenden geprüft.
  EXE-/Produktversion 1.2.1; nur Windows-System-DLLs, jetzt OLE32/AVRT für Audio.
- 11 Benachrichtigungs-Szenarien erfolgreich. Firmwarefunktion unverändert;
  die bisherige HEX-Datei wird mit dem passenden Sketch/Quellenpaket mitgeliefert.
- Kein erneuter realer Audio-Geräte-/Hörtest durch den Agenten nach dem gemeldeten
  Einfrieren. Der User-Test am optischen Windows-Ausgang ist noch ausstehend;
  ohne diesen Test keine Veröffentlichung von 1.2.1.

### Nachmeldung zu 1.2.0p

Der Eigentümer meldete rauen/unpassenden Klang und ein Einfrieren von Windows.
Der nachfolgende Neustart wurde mit Bugcheck 0xD1 protokolliert. Welcher Treiber
betroffen war und ob der Mithörton den Absturz auslöste, ist nicht geklärt.
Die damaligen API-/Sinustests belegen keine durchgehend saubere Wiedergabe.
Das GitHub-Release trägt einen Hinweis, den PC-Ton in 1.2.0p ausgeschaltet zu lassen.

## Version 1.2.0p – Histogramm und Windows-Mithörton – 03.10.2026

- Windows-Build mit Warnungen als Fehlern erfolgreich; 85 Kerncode-,
  16 Sinus-/Hüllkurven- und 37 Firmwareprüfungen bestanden. Neue Prüfungen decken 47/150-ms-Impulse,
  Heartbeats, unbekannte Startzustände, Verbindungsabbrüche, Ablauf nach
  fünf Minuten, die Speichergrenze und Ausschluss von Impulsen über 1.000 ms ab.
- Der reale Windows-Standardausgang wurde mit drei kurzen PCM-Sinustönen
  (650/400/1.000 Hz) geöffnet und gespeist; Ausgabe, Aus-/Einschalten,
  Suspend und Thread-/Geräteabschluss geprüft. Das ist ein API-/Gerätetest,
  keine subjektive Hörprüfung oder Messung der End-to-End-Latenz.
- GUI-/Demo-Worker-Test erfolgreich: COM-Auswahl, Pause/Fortsetzen und
  vollständiges Beenden, Kontaktweitergabe bei pausierter Space-Ausgabe sowie
  Checkbox und Frequenzeingaben (Grenzwerte und ungültige Texte).
  Histogramm mit simulierten Häufungen um 50/150 ms und Leerzustand gerendert;
  Demo-Darstellung einschließlich neuer Audio-Bedienelemente visuell geprüft.
- Nano-Build weiterhin 2.466 Byte Flash / 204 Byte RAM; serielles Protokoll
  und Firmwarefunktion unverändert. Windows-EXE-/Produktversion 1.2.0p,
  nur Windows-System-DLLs (zusätzlich WINMM für Audio) erforderlich.
- 11 Tests der Benachrichtigungslogik erfolgreich; Linux-CI prüft auch den
  gemeinsam genutzten Tongenerator mit Sanitizern.
- Zeitmessung bleibt PC-seitig. Kein neuer Hardwaretest durchgeführt.

## Security-Prüfungen – 03.10.2026

- Öffentlicher Quellstand: ausschließlich Alias-/GitHub-Noreply-Metadaten.
  Quellen, EXE, HEX, Vorschau und das entpackte Release-Inventar wurden
  auf bekannte persönliche Angaben des Eigentümers geprüft: keine gefunden.
  Notwendige fremde Copyright-/Lizenzhinweise bleiben erhalten.
- Alle vier Jobs des eigenen Security-
  Workflows erfolgreich; Private vulnerability reporting per API aktiviert
  und mit `enabled: true` bestätigt. Secret Scanning und Push Protection aktiv.
  CodeQL-Default-Setup mit erweiterter Abfragesuite zusätzlich eingerichtet.
- Erster CodeQL-Lauf für C/C++, JavaScript/TypeScript und Actions erfolgreich.
  Ein `js/code-injection`-Hinweis im lokalen Test wurde im tatsächlichen
  Vertrauenskontext geprüft und als Fehlalarm eingeordnet; Begründung und
  Grenzen stehen in `SECURITY.md`. Kein Scanner-/Regelausschluss eingerichtet.
- GitHub-[Security-Prüfungen](https://github.com/Auragant/Morsetaste/actions/workflows/security.yml)
  erfolgreich: Cppcheck **2.13.0** auf Windows-Code und tatsächlichem Sketch
  über den Teststub; keine Befunde in den aktivierten Kategorien.
- Gitleaks **8.30.1**, verpflichtend SHA-256-geprüft: gesamte ausgecheckte
  Historie, **keine gefundenen Zugangsdaten**.
- **73 Kerncode- und 37 Firmwareprüfungen** zusätzlich unter Linux mit
  AddressSanitizer/UndefinedBehaviorSanitizer erfolgreich.
- **11 Benachrichtigungs-Szenarien** prüfen den echten Workflow-Scriptblock
  ohne Netzwerk: Neuanlage, Zuweisung/Erwähnung, Wiederholungsfehler, Auflösung,
  abgebrochene/übersprungene und veraltete Läufe, fremde Issues/PRs sowie Testmodus.
  Lokal ausführbar mit `node tests/security_notify_tests.cjs`.
- Manueller Zustelltest ebenfalls grün: `github-actions[bot]` hat ein
  eindeutig gekennzeichnetes Test-Issue erstellt, `Auragant` zugewiesen und
  erwähnt. Das ist **kein Sicherheitsbefund**.
  Tatsächlichen Empfang der GitHub-Benachrichtigung hat der Eigentümer am
  03.10.2026 bestätigt; der konkrete Zustellkanal wurde nicht gesondert angegeben.
- Windows- und Nano-Build erneut erfolgreich: **2.466 Byte Flash / 204 Byte RAM**;
  GUI-Test mit echtem Demo-Worker, Pause/Fortsetzen und vollständigem Beenden
  wieder erfolgreich, ohne COM-Zugriffe oder echte Tastatureingaben.
- Neuer Paketbau lokal geprüft: GPL-/Fremdlizenzen, Disclaimer, Core-/Variant-
  Quellen, Plattformdefinitionen, Buildskripte und Binärdateien vorhanden;
  keine Toolprogramme, Git-Historie, Treiber oder Bootloader eingebettet.
  Das vollständige Quellen-/Binärpaket wird als `MorseBridge-1.1.0p-win64.zip` angeboten.

Keine Urheberrechts-/Patentprüfung durch diese Scanner und keine Garantie für
Sicherheit, Fehlerfreiheit oder vollständige Lizenzkonformität.

## Version 1.1.0p – 03.10.2026

- Windows-EXE 1.1.0p und Nano-Firmware erfolgreich gebaut.
- **73 Protokoll-/Ausgabeprüfungen + 37 Firmware-Prüfungen bestanden**.
  Die neuen Fälle prüfen Pinbelegung, stummen Start, offenen/fehlenden Schalter,
  Freigabe ohne Morse-Kontakt, getrennte Entprellung, Schalterprellen parallel
  zu Morseereignissen, Ein/Aus während eines gehaltenen Impulses, Heartbeat,
  Start mit bereits geschlossenen Kontakten und `millis()`-Überlauf.
- Der aktive Summer wird als digitaler Ausgang geprüft; es wird kein
  `tone()`/PWM verwendet. Serielle Tastenzustände bleiben unabhängig vom Schalter.
- Echter Nano-AVR-Build: **2.466 Byte Flash, 204 Byte RAM**,
  FQBN `arduino:avr:nano:cpu=atmega328`, Arduino AVR Boards 1.8.8.
- GUI-/Hintergrundthread-Test erneut bestanden: Darstellung, COM-Auswahl,
  Pause/Fortsetzen, Schließen und vollständiges Beenden. Vorschau visuell geprüft.
- EXE-Datei-/Produktversion `1.1.0p`, numerische Windows-Version `1.1.0.1`;
  weiterhin nur Windows-System-DLLs erforderlich.
- Praxistest der Nano-/Junker-/TMB12A05-Kette samt Transistorstufe und des
  konkreten Zielprogramms vom Eigentümer am 03.10.2026 erfolgreich bestätigt.
  Dieser berichtete Test ist keine unabhängige elektrische Zertifizierung.

## Basisstand 1.0.0 – 01.10.2026

- Windows-x64-EXE mit GCC 16.2.0 / w64devkit 2.10.0 gebaut.
  C++17, `-Wall -Wextra -Wpedantic -Werror`, statische C++-Laufzeit.
- Abhängigkeiten geprüft: ausschließlich Windows-System-DLLs
  (ADVAPI32, COMCTL32, GDI32, GDI+, KERNEL32, msvcrt, SETUPAPI,
  SHELL32, USER32, WTSAPI32). Kein separates Runtime-Paket nötig.
- **73 erfolgreiche Protokoll-/Ausgabeprüfungen**: Fragmentierung an jeder
  Paketgrenze, ungültige/überlange Zeilen, mehrere schnelle Pulse in einem
  Lesepaket, Heartbeat ohne Key-repeat, Erstverbindung bei gedrückter Taste,
  1-s-Timeout, längeres Halten mit Heartbeat, Wiederverbindung, Pause,
  Fokus-/Sperrzustand und Rückmeldungen einer fehlgeschlagenen Eingabe.
- **12 erfolgreiche Firmware-Prüfungen**: Der tatsächliche `.ino`-Sketch
  wurde gegen simulierte Arduino-I/O gebaut und mit Kontaktprellen,
  Kurzimpulsen, Heartbeat, LED und `millis()`-Überlauf geprüft.
- Echter AVR-Build mit Arduino AVR Boards **1.8.8** und AVR-GCC
  **7.3.0-atmel3.6.1-arduino7**, FQBN `arduino:avr:nano:cpu=atmega328`:
  **2.272 Byte Flash, 198 Byte RAM**. Vier Compilerwarnungen stammen aus
  unbenutzten Parametern von `cores/arduino/new.cpp`, nicht aus dem Sketch.
- GUI-Zustände Suche, gedrückt und Pause als PNG gerendert und visuell geprüft.
  Die Bilder sind ausdrücklich Vorschauen mit simulierten Daten.
- GUI-Test prüft außerdem die COM-Auswahl, startet den echten Hintergrundthread
  im Demo-Modus, pausiert und aktiviert ihn über die normalen Fensterbefehle und
  beendet ihn über die Schließen-Schaltfläche. Exitcode **0**, kein verbleibender
  Worker. Dieser Test öffnet keine seriellen Ports und sendet keine Tastendrücke.

Die reinen Zustandstests ersetzen keine Hardware-/Integrationstests.
Für den aktuellen Stand wurde ein erfolgreicher Praxistest des konkreten
Aufbaus vom Eigentümer bestätigt (siehe Abschnitt 1.1.0p).

# Prüfstand

## Version 1.2.2 – 04.10.2026

Der Eigentümer hat den lokal bereitgestellten 1.2.2-Stand am 04.10.2026 mit
„Das passt jetzt soweit“ freigegeben. Einzelne Hardware-, Monitorwechsel-,
Kontrastmodus- und Bedienprüfungen wurden damit nicht gesondert bestätigt.
Das bestehende Icon wurde ausdrücklich unverändert akzeptiert. Security-,
CodeQL- und Windows-Kompatibilitätsprüfungen des Veröffentlichungsstands folgen
vor dem Release.

Lokale Prüfung am 04.10.2026, nach Einbindung des App-Icons erneut ausgeführt:

- Windows-Build mit Warnungen als Fehlern erfolgreich. **559 Prüfungen bestanden**:
  85 Kern-, 16 Sinus-, 150 Audioformat-/Puffer-, 30 Audio-Thread-, 37 Firmware-,
  184 Versions-/Update- und 57 Einstellungsprüfungen.
- Zusätzlich 11 Benachrichtigungs-Szenarien bestanden, ohne GitHub-Schreibzugriff.
- App-Icon in der EXE: alle sieben PNG-/RGBA-Ressourcen (16/24/32/48/64/128/256)
  stimmen bytegleich mit der ICO-Vorlage überein. Große/kleine Icons für
  100/150/200 % DPI erfolgreich über Windows geladen; About-Bildexport zeigt
  das eigene Icon in der Titelleiste. Ein praktischer Monitorwechsel wurde nicht
  gesondert bestätigt.
- GUI-/Demo-Worker-Test bestanden, einschließlich Theme-Vorschau/Abbrechen/OK,
  Updateanzeige mit simuliertem Transport, Abbruch, Pause und vollständigem Beenden.
  Bildexporte für sechs Darstellungsoptionen bei 100/150/200 % Skalierung vorhanden;
  die synthetische Skalierung ersetzt keinen Test beim realen Monitorwechsel.
- Echte öffentliche WinHTTP-HTTPS-Abfrage erfolgreich: Der Server meldete 1.2.1;
  die lokale 1.2.2 wurde korrekt als ohne neueres verfügbares Release eingeordnet.
- Nano-Sketch erneut gebaut: 2.466 Byte Flash / 204 Byte RAM. HEX ist bytegleich
  mit 1.2.1; es wurde kein Board geflasht.
- Sichtprüfung im isolierten Demo-Modus bestätigte gespeicherte 777 Hz, automatische
  COM-Auswahl, ausgeschaltete Ton-/Vordergrundoptionen und das Matrix-Menü.
  Ein dort entdeckter Button-Zeichenfehler wurde behoben und der GUI-Test erneut
  bestanden. Der Nutzer beendete die anschließende Computer-Use-Prüfung mit Escape;
  auch nach erneuter Freigabe meldete das Tool den Abbruch weiterhin.
  Die allgemeine Nutzerabnahme bestätigt diese einzelnen automatisierten
  Sicht-/Tastatur-/Dropdownprüfungen nicht zusätzlich. Systemwechsel und
  Windows-Kontrastmodus bleiben unbestätigte praktische Prüfgrenzen.

SHA-256 der lokal geprüften Test-EXE:

```text
514ec7e6b8b877697ebd34881a4002e6f38fbe069161a6a30ca0c8b785abb7a9
```

Quellstand der Test-EXE: `6367112f772ecadb1e7dcf4eb5189743a3265dd2`
(lokaler Git-Commit, ausgehend von `f801cff1bdf816a765b57966b255c5d8f8d907f9`).
Danach wurden Dokumentation und Build-/Prüfskripte ergänzt, einschließlich der
monatlichen Windows-Kompatibilitätsprüfung. Eine lokale Icon-Variable wurde
wegen des Microsoft-SDK-Makros `small` in `smallIcon` umbenannt; diese reine
Bezeichneränderung verändert das Verhalten nicht. Icon und EXE bleiben
unverändert. Prüfsummen liegen im Paket und daneben unter `dist/SHA256SUMS.txt`.
Der MinGW-Vergleichsbuild nach der Umbenennung erzeugt eine bytegleiche
`.text`-Sektion; veröffentlicht wird weiterhin die vollständig unveränderte
freigegebene EXE mit obiger SHA-256.

Build-Zeitpunkt dieser EXE: `2026-10-03T23:41:18Z` (UTC). Sie ersetzt das frühere
lokale Testpaket ohne App-Icon. Ablauf und dokumentierte Nutzerabnahme stehen
in `docs/USER_TEST_1.2.2.md`. Version 1.2.1 samt Git-Historie und Downloads bleibt erhalten.
Die bisher leere GCC Runtime Library Exception wurde mit dem vollständigen
Originaltext ergänzt. Der Paketbau verwirft fehlende/leere Pflicht-Lizenztexte.

Testumfang und praktische Prüfgrenzen:

- Windows-Build mit Warnungen als Fehlern und sämtliche Kern-, Sinus-,
  Audioformat-/Puffer-, Audio-Thread-, Firmware-, Update- und Einstellungsprüfungen.
  Updateprüfungen verwenden gespeicherte Antworten/simulierten Transport statt
  eines öffentlichen Netzwerkdienstes; Einstellungen verwenden temporäre Pfade.
- GUI-/Demo-Worker-Test sowie Sichtprüfung aller Farbschemata einschließlich
  Menü, Dropdown, Checkbox, Spin-Control, About und Einstellungen. Mindestgröße,
  100/150/200 % DPI, Tastatur-/Fokuszustände und System-Hell/Dunkel-Wechsel prüfen.
- User-Test der neuen EXE: Nano/Junker/Zielprogramm, PC-Mithörton, gehaltene Taste
  beim Wechsel in Menü/Dialog/Browser, Loslassen zum erneuten Scharfstellen,
  Pause-Hotkey, USB-Abziehen, Sperren/Standby und vollständiges Beenden.
- Theme-Vorschau und Abbrechen, Neustart mit gespeichertem Farbschema/Frequenz,
  ungültige Frequenz und fehlende/beschädigte/nicht schreibbare Einstellungsdatei.
  Beim Start bleibt der PC-Mithörton aus.
- Manuelle Updateprüfung: neuere/gleiche/ältere Version, Offline, Timeout,
  Rate-Limit und ungültige Antwort; App bleibt bedienbar und beendet sich auch
  während einer Anfrage. GitHub-/Release-Link im Standardbrowser prüfen.
- Security checks, CodeQL und Windows-Kompatibilität für den
  Veröffentlichungsstand vor dem Release erfolgreich prüfen.
- Version 1.2.2 in About, Footer, EXE-/Produktversion und ZIP konsistent;
  Build-Zeitpunkt in UTC eingebettet. SHA-256 und Quellstand der konkret
  getesteten EXE dokumentieren und diese Datei unverändert paketieren.

Firmware und serielles Protokoll bleiben unverändert; ein Windows-Update auf
1.2.2 erfordert kein erneutes Flashen. Das Paket bleibt ein vollständiges Quellen-
und Binärpaket mit verwendeten Arduino-Core-/Variant-Quellen und Lizenzen.

## Monatliche Windows-Kompatibilität

Der Workflow [Windows compatibility](https://github.com/Auragant/Morsetaste/actions/workflows/compatibility.yml)
läuft am 5. jedes Monats um 07:41 UTC, auf `main`-Pushes, Pull Requests und manuell.
Der monatliche Lauf verwendet den aktuellen Default-Branch und frisch bereitgestellte
GitHub-Windows-Images. `windows-latest` folgt GitHubs neuester stabiler Windows-
Runnergeneration; `windows-2022` dient als zusätzliche ältere Vergleichsumgebung.
Die tatsächliche OS-/Image-Version wird bei jedem Lauf protokolliert.

1. Der unveränderte Produktionsbuild mit festgelegtem, SHA-256-geprüftem MinGW
   wird auf beiden Windows-Runnern gebaut; alle sieben Softwaretests laufen dort.
2. Der eingebaute `--render-test` prüft die gebaute EXE: Windows-GUI, Controls,
   Frequenzfelder, Demo-/Hintergrundthread, Pause/Fortsetzen und vollständiges Beenden.
   Ein Hänger bricht nach 60 Sekunden mit Fehler ab.
3. Der neueste veröffentlichte Windows-ZIP-Download wird gegen GitHubs
   Asset-SHA-256 geprüft. Die enthaltene, unveränderte EXE durchläuft denselben
   GUI-Test. Release-Version und EXE-Prüfsumme werden protokolliert. Ein fehlendes
   Release, Downloadfehler oder fehlende Prüfsumme lässt die Prüfung fehlschlagen.
4. `scripts/build-msvc.ps1` baut Ressourcen, Anwendung und Softwaretests zusätzlich
   mit den auf `windows-latest` installierten Microsoft-C++-/Windows-SDK-Werkzeugen
   und führt Tests sowie GUI-Prüfung aus. Dieser Vergleichsbuild wird nicht veröffentlicht.

Die Windows-System-DLLs werden beim Start geladen. Audioformate und der echte
Audio-Thread werden gegen simulierte Geräte geprüft; kein echter Audiostream,
COM-Port oder künstlicher Tastendruck wird erzeugt. GUI-PNGs bleiben 14 Tage als
Actions-Artefakte verfügbar. Die Nano-Firmware wird im Softwaretest mitgeprüft;
ein Board wird weder neu geflasht noch ein Arduino-Core heruntergeladen.

Ein fehlgeschlagener Windows- oder SDK-Job auf dem Default-Branch erzeugt das
zugewiesene Issue **[Kompatibilität] Windows-Prüfung benötigt Aufmerksamkeit**.
Weitere Fehler kommentieren dasselbe Issue. Nur ein vollständig erfolgreicher
Lauf schließt es; fehlende, abgebrochene, übersprungene oder veraltete Quellstände
tun dies nicht. Der separate Benachrichtigungsjob führt keinen Checkout durch
und erhält als einziger Issues-Schreibrechte; Pull Requests erhalten sie nicht.
Die Zustellung folgt denselben GitHub-Einstellungen wie in [SECURITY.md](SECURITY.md#benachrichtigungen).

**Grenzen:** GitHubs Standard-Windows-Runner laufen unter Windows Server. Sie
bilden weder alle Windows-10/11-Funktionsupdates noch CH340-/Audiotreiber,
USB-Geräte und Zielprogramme des eigenen PCs ab. Der Test erkennt Build-,
DLL-Lade-, GUI- und Softwarefehler in den getesteten Umgebungen; nach größeren
Windows-/Treiberupdates bleibt ein kurzer Praxistest am eigenen Aufbau sinnvoll.
Die geprüften SDK-Versionen sind die auf dem Runner installierten Versionen,
keine Zusage, dass jedes gerade veröffentlichte Microsoft-SDK bereits enthalten ist.

Geplante Läufe können verspätet starten. GitHub deaktiviert sie in öffentlichen
Repositories nach 60 Tagen ohne Repository-Aktivität; zur Reaktivierung unter
Actions den Workflow aktivieren. Normale Workflow-Läufe sind kein verlässlicher
Ersatz für Repository-Aktivität. Actions muss eingeschaltet bleiben.
Quellen: [GitHub: Zeitpläne und Inaktivitätslimit](https://docs.github.com/en/actions/reference/workflows-and-actions/events-that-trigger-workflows#schedule),
[GitHub: Runner-Images und Aktualisierungen](https://github.com/actions/runner-images#image-definitions),
[Microsoft: C++-Buildwerkzeuge](https://learn.microsoft.com/en-us/cpp/build/building-on-the-command-line).

Die Einrichtung wurde am 04.10.2026 lokal und auf GitHub geprüft:
beide Windows-Runner samt unveränderter Release-EXE sowie Microsofts Compiler
und Windows-SDK **10.0.26100.0** waren erfolgreich
([erster GitHub-Prüflauf](https://github.com/Auragant/Morsetaste/actions/runs/37160064213)).
Zusätzlich bestanden 13 simulierte Kompatibilitäts-Benachrichtigungsszenarien,
die PowerShell-Syntaxprüfung und actionlint 1.7.12. Der CodeQL-Testhinweis und
seine Einordnung stehen in [SECURITY.md](SECURITY.md#einordnung-des-ersten-codeql-hinweises).

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

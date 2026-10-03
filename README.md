# MorseBridge – Junker M.T. → Space

Version **1.2.2 in Vorbereitung** – Menüleiste, manuelle Updateprüfung, Themes und App-Icon.
User-Test und GitHub-Prüfungen für 1.2.2 stehen vor der Veröffentlichung noch aus.
Der lokale Testablauf steht in [User-Test 1.2.2](docs/USER_TEST_1.2.2.md).
Die zuletzt veröffentlichte Version **1.2.1** hat den User-Test am 04.10.2026
bestanden. Seit 1.2.1 entfällt der bisherige Zusatz `p`.

**Hinweis zu 1.2.0p:** Rauer/verzerrter PC-Mithörton und ein Windows-Absturz
wurden gemeldet. Der Zusammenhang mit dem Absturz ist ungeklärt.
Den PC-Mithörton in **1.2.0p ausgeschaltet lassen** und auf 1.2.1 aktualisieren.
Die WASAPI-Ausgabe von 1.2.1 wurde vom Eigentümer am optischen Windows-Ausgang
erfolgreich getestet. Alte Binärdownloads wurden zurückgezogen.

Die Junker-Morsetaste steuert über einen Arduino Nano mit ATmega328P und CH340C
die Leertaste des aktiven Windows-Programms. Die kleine C++-Anwendung läuft auch
im Hintergrund oder minimiert. Fenster schließen beendet sie vollständig.

Projekt: [Auragant/Morsetaste](https://github.com/Auragant/Morsetaste) ·
[Fertige Programme / Releases](https://github.com/Auragant/Morsetaste/releases/latest).
Das Komplettpaket enthält die Windows-EXE unter `dist/`, den Nano-Sketch,
eine kompilierte HEX-Datei und den vollständigen Quellcode.

**LLM-Hinweis:** Code, Firmware und Teile der Dokumentation wurden wesentlich
mit einem LLM/KI erstellt. Experimentelles Hobbyprojekt, nicht für
sicherheitskritische Anwendungen. Bitte [Disclaimer](DISCLAIMER.md) beachten.
Eigener Code: **GPLv3**; [Lizenz](LICENSE) und
[Drittanbieter-Hinweise](THIRD_PARTY_NOTICES.md).

## Schnellstart

1. Die **beiden potentialfreien Schaltkontakte** der Junker mit **D2** und **GND**
   am Nano verbinden. Polung egal. Den Nano per USB-Datenkabel anschließen.
   Die Morsetaste dabei von Funkgerät, Sender und anderen Spannungsquellen trennen.
2. `firmware/JunkerSpace/JunkerSpace.ino` in der Arduino IDE öffnen.
   Board **Arduino Nano**, Prozessor **ATmega328P**, passenden COM-Port auswählen
   und hochladen. Bei Synchronisationsfehlern **ATmega328P (Old Bootloader)** versuchen.
   Zusätzliche Sketch-Bibliotheken sind nicht nötig.
3. Seriellen Monitor und andere Programme schließen, die den COM-Port belegen.
4. **`dist/MorseBridge.exe`** starten. Die EXE benötigt keine Installation,
   kein Python, kein .NET und keine separat installierte C++-Laufzeit.
5. Auf **„Nano verbunden · COM…“** warten. Morsetaste einmal loslassen,
   dann das gewünschte Morseprogramm aktivieren. Taste drücken = Space halten;
   Taste loslassen = Space freigeben.

```text
Junker-Kontakt 1 ───── D2   Arduino Nano
Junker-Kontakt 2 ───── GND  ATmega328P / CH340C
                            │ USB
                            └──── Windows-PC / MorseBridge.exe
```

Der interne Pull-up ist aktiviert. Ein zusätzlicher Widerstand ist bei kurzen
Leitungen normalerweise nicht nötig. Die eingebaute Nano-LED leuchtet bei
geschlossenem Kontakt. Die CH340C-Ausführung des Nano braucht das PC-Programm;
der USB-Chip selbst meldet sich weiterhin als COM-Port an.

## Optionaler aktiver Summer

Ab Version **1.1**: Ein Schalter zwischen **D4 und GND** gibt den Mithörton frei.
Geschlossen = Ton beim Drücken der Junker; offen oder nicht angeschlossen = stumm.
**D8** schaltet einen aktiven 5-V-Summer über eine Transistorstufe. Für den
**TMB12A05** sind Verdrahtung und Bauteile im [Summer-Anschlussplan](docs/SUMMER.md)
beschrieben; ihn nicht direkt aus einem Nano-Ausgang versorgen.

Die Space-Funktion bleibt unverändert. Der Ton folgt lokal dem Kontakt und
funktioniert auch ohne PC-Programm. Die Pause-Schaltfläche der Windows-App
pausiert ausschließlich die Tastaturausgabe; der Hardwareschalter schaltet den Ton.

## Bedienung

- **Verbindung:** automatisch erkannter Nano und COM-Port. Bei Bedarf im
  Auswahlfeld einen vorhandenen COM-Port manuell wählen und „Neu suchen“ drücken.
- **Morsetaste:** entprellter Kontakt, Anzahl der Betätigungen und Dauer des
  letzten Impulses. Die angezeigte Dauer wird am PC gemessen, nicht im Nano.
- **Space:** Zustand der von Windows angenommenen künstlichen Tasteneingabe.
  Das Zielprogramm kann künstliche Eingaben trotzdem ignorieren.
- **Live-Verlauf:** Kontakt und Space der letzten acht Sekunden; im hellen
  Standard-Farbschema grün und blau, in den übrigen Themes passend eingefärbt.
- **Impulslängen:** Dynamisches Histogramm der vollständigen Kontaktimpulse aus den
  letzten fünf Minuten, auch bei pausierter Ausgabe. Normalerweise 5-ms-Klassen;
  bei längeren Impulsen passt sich die Skala an. Nur im Arbeitsspeicher, beim
  Beenden gelöscht. Unterbrochene oder beim Verbinden schon gehaltene Impulse
  werden nicht gezählt. Maximal 16.384 Impulse. Die Zeiten stammen vom PC-Empfang
  und können durch USB-/Windows-Verzögerungen beeinflusst werden.
  Impulse **über 1.000 ms** werden ausgeschlossen; genau 1.000 ms werden gezählt.
- **PC-Mithörton:** Häkchen oben rechts aktiviert einen Sinuston über das normale
  Windows-Audiogerät. Ohne gespeicherten Wert **650 Hz**, per Zähler/Pfeilen oder Zahleneingabe
  einstellbar von **400 bis 1.000 Hz**. Ungültige Eingaben werden beim Verlassen
  des Felds auf die letzte gültige Frequenz zurückgesetzt. Die gültige Frequenz
  wird beim Verlassen des Felds oder normalen Beenden lokal gespeichert.
  Der PC-Mithörton ist beim Start ausgeschaltet; das Häkchen gilt nur für die
  laufende Sitzung. Der Ton folgt dem Kontakt, auch
  im eigenen Fenster, im Hintergrund und bei pausierter Space-Ausgabe. Abbruch,
  Sperre, Standby und Beenden stoppen ihn. Lautstärke über Windows steuern.
  USB-, Windows- und Audiogeräte-Latenz beeinflussen den hörbaren Zeitpunkt.
  Ab 1.2.1 erfolgt die Ausgabe mit WASAPI im gemeinsam genutzten Windows-Modus,
  im Format des Standardgeräts und mit dessen Puffertakt. Audiofehler schalten
  den PC-Mithörton ab; erneutes Einschalten versucht den Gerätezugriff erneut.
- **Ausgabe pausieren:** gibt Space frei; Kontakt und Verlauf werden weiter angezeigt.
  **Strg + Alt + F12** schaltet die Pause auch im Hintergrund um. Ist diese
  Tastenkombination schon belegt, bleibt die Schaltfläche verfügbar.
- **Immer im Vordergrund:** Anzeige bleibt sichtbar, ohne den Fokus beim
  Tastendrücken zu übernehmen. Funktioniert auch bei aktiviertem Zielfenster.
- **Schließen / X:** gibt ein von der App gehaltenes Space frei und beendet die App.
  Kein Tray-Prozess und kein Windows-Dienst bleiben zurück.
- **Menüleiste:** „Datei → Beenden“ schließt die Anwendung.
  „Hilfe → Über MorseBridge“ zeigt Version, Build-Zeitpunkt in UTC und GitHub-Link.
- **Einstellungen:** „Optionen → Einstellungen“ bietet **System, Hell, Dunkel,
  Mitternacht, Amber und Matrix**. Eine Auswahl wird sofort als Vorschau sichtbar;
  „OK“ speichert sie, „Abbrechen“ stellt das vorherige Farbschema wieder her.
  **System** folgt automatisch dem hellen/dunklen Windows-App-Modus.
- **Lokal gespeichert:** Farbschema und gültige Frequenz liegen in
  `%LOCALAPPDATA%\MorseBridge\settings.ini`, unabhängig vom EXE-Ordner.
  Ohne gültige Datei startet die App mit System und 650 Hz. COM-Auswahl,
  Ausgabe-Pause, Mithörton-Häkchen und „Immer im Vordergrund“ sind Sitzungseinstellungen.
- **Updates:** „Hilfe → Nach Updates suchen“ fragt manuell den neuesten stabilen
  Release von **Auragant/Morsetaste** ab. Währenddessen bleibt die App bedienbar.
  Ist eine neuere Version verfügbar, lässt sich die zugehörige GitHub-Releaseseite
  im Browser öffnen. Download und Austausch der EXE erfolgen dort manuell;
  MorseBridge vorher normal beenden. Es gibt weder eine Prüfung beim Start noch
  automatische Installation. Ohne Internet bleiben alle lokalen Funktionen nutzbar.

Solange MorseBridge selbst das aktive Fenster ist, wird nur der Kontakt angezeigt;
es wird kein Space an die eigenen Bedienelemente gesendet. Für die Ausgabe das
Zielfenster anklicken. Nach Verbindung, Fortsetzen oder Fensterwechsel während
eines Impulses muss die Morsetaste losgelassen werden, bevor ein neuer Impuls
gesendet wird. Eine gehaltene Taste wird dabei nicht ungewollt neu gedrückt.

USB-Abziehen, Ausbleiben gültiger Meldungen für **1 Sekunde**, Windows-Sperre
und Standby lösen die Freigabe aus. Anschließend wird automatisch neu verbunden.
Die Freigabe ist ein Windows-Eingabeereignis; Absturz, erzwungenes Beenden oder
von Windows gesperrte Eingaben lassen sich damit nicht garantiert abfangen.

## Demo ohne Nano

`MorseBridge.exe --demo` startet einen simulierten SOS-Verlauf. In diesem Modus
werden weder COM-Ports geöffnet noch echte Tastendrücke erzeugt.
Der PC-Mithörton kann auch in der Demo aktiviert werden.

Für isolierte lokale UI-Prüfungen steht `--ui-test <Einstellungsdatei>` bereit.
Dieser Modus simuliert den Nano und die Updateantwort (Version 1.2.3), verwendet
nur die angegebene Datei und erzeugt keine echte Space-Ausgabe. Die simulierte
Updateanzeige ist keine Aussage über eine veröffentlichte Version.

## Wenn etwas nicht funktioniert

- **Kein Nano:** USB-Datenkabel, Geräte-Manager und CH340-Treiber prüfen.
  In der automatischen Suche werden nur CH340-IDs `1A86:7523` und `1A86:5523`
  geöffnet. Andere USB-Seriell-Varianten lassen sich manuell auswählen.
- **COM-Port belegt:** Arduino-Seriellmonitor bzw. andere serielle Anwendung schließen.
  Zum erneuten Hochladen MorseBridge vollständig schließen; Pause allein lässt
  den COM-Port geöffnet.
- **Keine JunkerSpace-Firmware:** der Port existiert, aber die eindeutige Kennung
  fehlt. Sketch hochladen; nach dem Öffnen können bis zu 4,5 Sekunden vergehen.
- **Andere CH340-Geräte:** die automatische Suche öffnet deren Ports ebenfalls.
  Dabei kann DTR einen Reset auslösen. Für einen solchen Aufbau den Nano-Port
  gezielt manuell wählen. Die App sendet keine seriellen Befehle an diese Geräte.
- **Kontakt funktioniert, Ziel reagiert nicht:** Ziel muss das aktive Fenster sein.
  Programme mit Administratorrechten können Eingaben einer normal gestarteten
  App blockieren. Wenn das Ziel zwingend erhöht läuft, MorseBridge ebenfalls
  entsprechend starten. Nach einem angezeigten Eingabefehler Pause / Fortsetzen
  betätigen. Manche Spiele/Raw-Input-Anwendungen akzeptieren `SendInput` nicht.
- **Halten im Texteditor:** es wird genau ein Key-down und ein Key-up gesendet,
  ohne künstlichen Autorepeat. Es werden daher nicht absichtlich Leerzeichenketten
  erzeugt. Für Morseprogramme sind die Druck- und Loslasszeitpunkte entscheidend.
- **Latenz:** 5 ms Entprellung plus USB-/Windows-Verzögerung. Das ist keine
  Echtzeitverbindung; kleine Timing-Schwankungen sind möglich.

## Selbst bauen

Windows 10/11, 64 Bit. Die fertige EXE liegt unter `dist/`.
Quellcode: C++17, Win32, SetupAPI, überlappende serielle Lesezugriffe, `SendInput`.
Es werden nur Windows-System-DLLs dynamisch benötigt.

In PowerShell im Projektordner:

```powershell
./scripts/get-tools.ps1                 # einmalig: portable Buildwerkzeuge
./scripts/build.ps1                     # EXE + automatisierte Tests
./scripts/build-firmware.ps1 -InstallCore # einmalig: AVR-Core laden und bauen
./scripts/build-firmware.ps1            # danach ohne erneute Installation
./scripts/test-gui.ps1                  # GUI-/Hintergrundthread-Test ohne Eingaben
./scripts/package.ps1                   # versioniertes Komplettpaket erstellen
./scripts/test-audio.ps1                # Audio-Thread gegen simuliertes Gerät prüfen (stumm)
```

Werkzeuge und Arduino-Pakete bleiben unter `.tools/`. Es erfolgt keine
systemweite Installation und **kein automatisches Flashen eines Boards**.
Ein vorhandener MinGW-w64-Compiler kann mit `build.ps1 -Compiler <Pfad>`
verwendet werden. Arduino IDE ist alternativ nur für den Sketch nötig.
`build-firmware.ps1 -ToolsRoot <Pfad>` und `package.ps1 -ToolsRoot <Pfad>`
können einen bereits vorhandenen Werkzeug-/Arduino-Core-Ordner verwenden.
Die zentrale Windows-Version steht in `windows/version.hpp`; `build.ps1`
erzeugt Build-Zeitpunkt in UTC und Manifest unter `build/`.
Der Paketbau prüft EXE-/Produktversion und erzeugt das ZIP ohne erneuten EXE-Build.

Grafikprüfung ohne COM-Port oder Tastatureingaben:

```powershell
./scripts/test-gui.ps1
```

## Protokoll und Prüfung

115200 Baud, 8N1, keine Flusssteuerung. ASCII-Zeilen:

```text
JUNKER/1 D    gedrückt
JUNKER/1 U    losgelassen
```

Die Erläuterung hinter den Zeilen gehört nicht zum Protokoll. Die Firmware sendet
bei Zustandswechseln und spätestens alle 250 ms; jede Zeile endet mit CRLF.
Der PC akzeptiert nur exakte Zustandszeilen. Überlange, fremde oder beschädigte
Zeilen werden verworfen. Wiederholte Zustandsmeldungen erzeugen keine weiteren
Key-downs. Kurze aufeinanderfolgende Pulse bleiben auch innerhalb desselben
USB-Lesepakets erhalten.

Die Tests prüfen Protokollfragmentierung, Kontaktprellen anhand des tatsächlichen
Sketches mit simulierten Arduino-Ein-/Ausgaben, Timeout, Wiederverbindung,
Pause, Freigabe und fehlgeschlagene Eingabeereignisse. Der Sketch wird zusätzlich
mit dem echten AVR-Compiler für den Nano übersetzt. Der Eigentümer hat die reale
Nano-/Junker-/Summer-Kette mit seinem Windows-Zielprogramm erfolgreich praktisch
getestet und dies am 03.10.2026 bestätigt. Dies gilt für den getesteten Aufbau,
nicht als Garantie für alle Geräte oder Zielprogramme.
Den User-Test der überarbeiteten PC-Audioausgabe von 1.2.1 hat er am
04.10.2026 als bestanden bestätigt; die veröffentlichte EXE ist dieselbe Datei.
Dieser Test bestätigt noch nicht die neue 1.2.2-EXE. Vor deren Veröffentlichung
sind der erneute User-Test sowie Security checks und CodeQL für den
Veröffentlichungsstand erforderlich.

Prüfergebnisse: [TESTING.md](TESTING.md) · Änderungen: [CHANGELOG.md](CHANGELOG.md).

## Security und Veröffentlichung

GitHub prüft C++-Code, Git-Historie auf Zugangsdaten sowie Protokoll-/Firmwaretests
bei Änderungen und wöchentlich. Fehler erzeugen ein dem Eigentümer zugewiesenes
GitHub-Issue. Prüfungen, Grenzen und E-Mail-Einstellungen:
[SECURITY.md](SECURITY.md).

[Rechtliche Vorprüfung und Veröffentlichungs-Checkliste](docs/PUBLICATION_REVIEW.md).
Das für 1.2.2 vorgesehene Komplettpaket `MorseBridge-1.2.2-win64.zip` enthält zusätzlich
die verwendeten Arduino-Core-Quellen und vollständigen Lizenzhinweise.
Das Projekt wird unter dem GitHub-Alias **Auragant** veröffentlicht; es nennt
keine persönliche Kontaktadresse des Projektverantwortlichen.

Technische Referenzen:
[Arduino Nano](https://docs.arduino.cc/hardware/nano/),
[Nano-Prozessorauswahl](https://support.arduino.cc/hc/en-us/articles/4401874304274-Select-the-right-processor-for-Arduino-Nano),
[Microsoft SendInput](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput),
[serielle Timeouts](https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-commtimeouts).

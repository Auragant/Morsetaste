# MorseBridge – Tastkontakt → PC-Taste

Version **1.3.0** – wählbare Ausgabetaste und eigene Tastenliste.
Der Eigentümer hat den parallelen Test am 27.10.2026 als erfolgreich bewertet
und die Veröffentlichung freigegeben. Veröffentlicht wird dieselbe getestete EXE.
[Release-Notizen](docs/releases/v1.3.0.md) · [Nutzertest 1.3.0](docs/USER_TEST_1.3.0.md).

Der bisherige Stand **1.2.2** brachte Menüleiste, manuelle Updateprüfung, Themes
und App-Icon. Der Eigentümer hat ihn am 04.10.2026 freigegeben; die Freigabe gilt
für die damalige getestete EXE. Testumfang und Prüfgrenzen stehen in
[TESTING.md](TESTING.md) und [User-Test 1.2.2](docs/USER_TEST_1.2.2.md).
Version **1.2.1** samt Release, Downloads und Git-Historie bleibt erhalten.
Seit 1.2.1 entfällt der bisherige Zusatz `p`.

**Hinweis zu 1.2.0p:** Rauer/verzerrter PC-Mithörton und ein Windows-Absturz
wurden gemeldet. Der Zusammenhang mit dem Absturz ist ungeklärt.
Den PC-Mithörton in **1.2.0p ausgeschaltet lassen** und auf die aktuelle Version aktualisieren.
Die WASAPI-Ausgabe von 1.2.1 wurde vom Eigentümer am optischen Windows-Ausgang
erfolgreich getestet. Binärdownloads aus Versionen vor 1.2.1 wurden zurückgezogen.

MorseBridge setzt einen **potentialfreien Tast- oder Schaltkontakt** über einen
Arduino Nano mit ATmega328P und CH340C in eine wählbare Taste des aktiven
Windows-Programms um; standardmäßig die Leertaste. Geeignet sind beispielsweise
gerade Morsetasten (Direkt-Keys), Taster und Fußtaster mit einem potentialfreien
Schließerkontakt. Kontakt schließen hält die gewählte PC-Taste, Kontakt öffnen
gibt sie frei. Die kleine C++-Anwendung läuft auch im Hintergrund oder minimiert.
Fenster schließen beendet sie vollständig.

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

1. Die **beiden Anschlüsse des potentialfreien Schließerkontakts** mit **D2** und
   **GND** am Nano verbinden. Polung egal. Den Nano per USB-Datenkabel anschließen.
   Den Kontakt dabei von anderen Geräten und externen Spannungsquellen trennen,
   bei einer Morsetaste beispielsweise von Funkgerät oder Sender.
2. `firmware/JunkerSpace/JunkerSpace.ino` in der Arduino IDE öffnen.
   Board **Arduino Nano**, Prozessor **ATmega328P**, passenden COM-Port auswählen
   und hochladen. Bei Synchronisationsfehlern **ATmega328P (Old Bootloader)** versuchen.
   Zusätzliche Sketch-Bibliotheken sind nicht nötig.
3. Seriellen Monitor und andere Programme schließen, die den COM-Port belegen.
4. **`dist/MorseBridge.exe`** starten. Die EXE benötigt keine Installation,
   kein Python, kein .NET und keine separat installierte C++-Laufzeit.
5. Auf **„Nano verbunden · COM…“** warten und im Hauptfenster die Ausgabetaste
   auswählen: **SPACE** oder **Left CTRL** (linke Strg-Taste). Weitere Tasten lassen
   sich unter „Optionen → Einstellungen“ ergänzen. Den Kontakt einmal öffnen,
   dann das gewünschte Zielprogramm aktivieren. Kontakt schließen hält die
   gewählte PC-Taste; Kontakt öffnen gibt sie frei.

```text
Kontaktanschluss 1 ─── D2   Arduino Nano
Kontaktanschluss 2 ─── GND  ATmega328P / CH340C
                           │ USB
                           └──── Windows-PC / MorseBridge.exe
```

Der interne Pull-up ist aktiviert. Ein zusätzlicher Widerstand ist bei kurzen
Leitungen normalerweise nicht nötig. Die eingebaute Nano-LED leuchtet bei
geschlossenem Kontakt. Die CH340C-Ausführung des Nano braucht das PC-Programm;
der USB-Chip selbst meldet sich weiterhin als COM-Port an.

## Optionaler aktiver Summer

Ab Version **1.1**: Ein Schalter zwischen **D4 und GND** gibt den Mithörton frei.
Freigabeschalter geschlossen = Ton bei geschlossenem Eingangskontakt;
Freigabeschalter offen oder nicht angeschlossen = stumm.
**D8** schaltet einen aktiven 5-V-Summer über eine Transistorstufe. Für den
**TMB12A05** sind Verdrahtung und Bauteile im [Summer-Anschlussplan](docs/SUMMER.md)
beschrieben; ihn nicht direkt aus einem Nano-Ausgang versorgen.

Der Ton folgt lokal dem Kontakt und
funktioniert auch ohne PC-Programm. Die Pause-Schaltfläche der Windows-App
pausiert ausschließlich die Tastaturausgabe; der Hardwareschalter schaltet den Ton.

## Bedienung

- **Verbindung:** automatisch erkannter Nano und COM-Port. Bei Bedarf im
  Auswahlfeld einen vorhandenen COM-Port manuell wählen und „Neu suchen“ drücken.
- **Eingangskontakt (Anzeige „Morsetaste“):** entprellter Kontakt, Anzahl der
  Betätigungen und Dauer des letzten Impulses. Die angezeigte Dauer wird am PC
  gemessen, nicht im Nano.
- **Ausgabetaste:** Drop-down im Hauptfenster mit **SPACE**, **Left CTRL** und der
  eigenen Tastenliste. Die Auswahl wird sofort übernommen und lokal gespeichert.
  Beim Wechsel wird eine gehaltene alte Taste zuerst freigegeben; die neue Taste
  wird erst nach dem Öffnen des Eingangskontakts für den nächsten Impuls scharfgestellt.
  Die Ausgabe verwendet den zum Tastendruck passenden Scan-Code des aktiven
  Tastaturlayouts und denselben Code zum Loslassen.
- **Windows-Ausgabe:** Zustand der von Windows angenommenen künstlichen Tasteneingabe.
  Das Zielprogramm kann künstliche Eingaben trotzdem ignorieren.
- **Live-Verlauf:** Kontakt und Tastaturausgabe der letzten acht Sekunden; im hellen
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
  im eigenen Fenster, im Hintergrund und bei pausierter Tastaturausgabe. Abbruch,
  Sperre, Standby und Beenden stoppen ihn. Lautstärke über Windows steuern.
  USB-, Windows- und Audiogeräte-Latenz beeinflussen den hörbaren Zeitpunkt.
  Ab 1.2.1 erfolgt die Ausgabe mit WASAPI im gemeinsam genutzten Windows-Modus,
  im Format des Standardgeräts und mit dessen Puffertakt. Audiofehler schalten
  den PC-Mithörton ab; erneutes Einschalten versucht den Gerätezugriff erneut.
- **Ausgabe pausieren:** gibt die gehaltene Ausgabetaste frei; Kontakt und Verlauf werden weiter angezeigt.
  **Strg + Alt + F12** schaltet die Pause auch im Hintergrund um. Ist diese
  Tastenkombination schon belegt, bleibt die Schaltfläche verfügbar.
- **Immer im Vordergrund:** Anzeige bleibt sichtbar, ohne den Fokus beim
  Tastendrücken zu übernehmen. Funktioniert auch bei aktiviertem Zielfenster.
- **Schließen / X:** gibt die von der App gehaltene Ausgabetaste frei und beendet die App.
  Kein Tray-Prozess und kein Windows-Dienst bleiben zurück.
- **Menüleiste:** „Datei → Beenden“ schließt die Anwendung.
  „Hilfe → Über MorseBridge“ zeigt Version, Build-Zeitpunkt in UTC und GitHub-Link.
- **Einstellungen:** „Optionen → Einstellungen“ bietet **System, Hell, Dunkel,
  Mitternacht, Amber und Matrix**. Eine Auswahl wird sofort als Vorschau sichtbar;
  „OK“ speichert sie, „Abbrechen“ stellt das vorherige Farbschema wieder her.
  **System** folgt automatisch dem hellen/dunklen Windows-App-Modus.
- **Eigene Tastenliste:** Im selben Einstellungsdialog einen Namen eingeben oder
  aus der Liste wählen und „Hinzufügen“ drücken. Unterstützt werden einzelne
  Buchstaben **A–Z**, Ziffern **0–9**, **F1–F24**, linke/rechte **CTRL**, **SHIFT**
  und **ALT**, Navigationstasten und die aufgeführten NUMPAD-Tasten. Beispiele:
  `F1`, `A`, `Right CTRL`, `ENTER`, `PAGE DOWN`, `NUMPAD 0`. Die Auswahlliste zeigt
  alle unterstützten Namen. Tastenkombinationen und doppelte Einträge werden
  nicht übernommen; höchstens **32** eigene Tasten. „Entfernen“ löscht den
  markierten Eintrag. „OK“ übernimmt die Liste in das Hauptfenster;
  „Abbrechen“ verwirft die Änderungen. Wird die aktive Zusatztaste entfernt,
  wechselt die Ausgabe bei „OK“ auf **SPACE**. SPACE und Left CTRL bleiben verfügbar.
- **Lokal gespeichert:** Farbschema, gültige Frequenz, eigene Tastenliste und
  gewählte Ausgabetaste liegen in
  `%LOCALAPPDATA%\MorseBridge\settings.ini`, unabhängig vom EXE-Ordner.
  Die Liste wird als `CustomOutputKeys=F1;Right CTRL;A` und die Auswahl etwa als
  `OutputKey=F1` gespeichert. Ohne gültige Datei startet die App mit System,
  650 Hz, SPACE und leerer Zusatzliste. Bestehende Dateien aus 1.2.2 funktionieren
  weiterhin; ungültige oder nicht verfügbare Ausgabetasten fallen auf SPACE zurück.
  COM-Auswahl,
  Ausgabe-Pause, Mithörton-Häkchen und „Immer im Vordergrund“ sind Sitzungseinstellungen.
- **Updates:** „Hilfe → Nach Updates suchen“ fragt manuell den neuesten stabilen
  Release von **Auragant/Morsetaste** ab. Währenddessen bleibt die App bedienbar.
  Ist eine neuere Version verfügbar, lässt sich die zugehörige GitHub-Releaseseite
  im Browser öffnen. Download und Austausch der EXE erfolgen dort manuell;
  MorseBridge vorher normal beenden. Es gibt weder eine Prüfung beim Start noch
  automatische Installation. Ohne Internet bleiben alle lokalen Funktionen nutzbar.

Solange MorseBridge selbst das aktive Fenster ist, wird nur der Kontakt angezeigt;
es wird keine Ausgabetaste an die eigenen Bedienelemente gesendet. Für die Ausgabe das
Zielfenster anklicken. Nach Verbindung, Fortsetzen oder Fensterwechsel während
eines Impulses muss der Eingangskontakt geöffnet werden, bevor ein neuer Impuls
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
Dieser Modus simuliert den Nano und die Updateantwort (Version 1.3.1), verwendet
nur die angegebene Datei und erzeugt keine echte Tastaturausgabe. Die simulierte
Updateanzeige ist keine Aussage über eine veröffentlichte Version.

## Wenn etwas nicht funktioniert

- **Kein Nano:** USB-Datenkabel, Geräte-Manager und CH340-Treiber prüfen.
  In der automatischen Suche werden nur CH340-IDs `1A86:7523` und `1A86:5523`
  geöffnet. Andere USB-Seriell-Varianten lassen sich manuell auswählen.
- **COM-Port belegt:** Arduino-Seriellmonitor bzw. andere serielle Anwendung schließen.
  Zum erneuten Hochladen MorseBridge vollständig schließen; Pause allein lässt
  den COM-Port geöffnet.
- **Keine passende Firmware:** der Port existiert, aber die eindeutige Kennung
  fehlt. Den mitgelieferten Sketch `JunkerSpace` hochladen; nach dem Öffnen können
  bis zu 4,5 Sekunden vergehen.
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
Den 1.2.2-Stand hat er am 04.10.2026 mit „Das passt jetzt soweit“ freigegeben.
Einzelne praktische Tests wurden dabei nicht gesondert bestätigt; deren Grenzen
und die zugehörigen GitHub-Prüfungen stehen in `TESTING.md`.

Prüfergebnisse: [TESTING.md](TESTING.md) · Änderungen: [CHANGELOG.md](CHANGELOG.md).

## Monatliche Windows-Kompatibilitätsprüfung

[Windows compatibility](https://github.com/Auragant/Morsetaste/actions/workflows/compatibility.yml)
prüft jeweils am **5. des Monats um 07:41 UTC** (08:41 Uhr MEZ / 09:41 Uhr MESZ),
bei Änderungen auf `main`, bei Pull Requests und auf manuellen Aufruf:

- Produktionsbuild und Softwaretests auf `windows-latest` und `windows-2022`.
- Start und GUI-/Thread-Prüfung sowohl der neu gebauten als auch der zuletzt
  veröffentlichten EXE auf den aktualisierten Windows-Systemen.
- Zusätzlicher Build und Tests mit Microsofts installiertem C++-Compiler und
  Windows-SDK auf `windows-latest`, um SDK-Änderungen zu erkennen.

Fehler auf `main` erzeugen ein zugewiesenes GitHub-Issue; ein vollständiger grüner
Folgelauf schließt es wieder. Windows-/Runner-Versionen, Logs und GUI-Bilder
helfen bei der Diagnose. Die Anwendung verwendet direkt Windows-Systembibliotheken;
.NET-Updates betreffen keine von ihr benötigte Laufzeit.

GitHub-Runner verwenden Windows Server und prüfen weder den eigenen Windows-10/11-PC
noch CH340-/Audiotreiber, reale USB-Hardware oder die hörbare Audioqualität.
GitHub kann Zeitpläne in öffentlichen Projekten **nach 60 Tagen ohne
Repository-Aktivität deaktivieren**; laufende Prüfungen allein sichern keine
dauerhafte Aktivierung. Dann unter Actions den Workflow wieder aktivieren.
Details und Quellen: [TESTING.md](TESTING.md#monatliche-windows-kompatibilität).

## Security und Veröffentlichung

GitHub prüft C++-Code, Git-Historie auf Zugangsdaten sowie Protokoll-/Firmwaretests
bei Änderungen und wöchentlich. Fehler erzeugen ein dem Eigentümer zugewiesenes
GitHub-Issue. Prüfungen, Grenzen und E-Mail-Einstellungen:
[SECURITY.md](SECURITY.md).

[Rechtliche Vorprüfung und Veröffentlichungs-Checkliste](docs/PUBLICATION_REVIEW.md).
Das Komplettpaket `MorseBridge-1.3.0-win64.zip` enthält zusätzlich
die verwendeten Arduino-Core-Quellen und vollständigen Lizenzhinweise.
Das Projekt wird unter dem GitHub-Alias **Auragant** veröffentlicht; es nennt
keine persönliche Kontaktadresse des Projektverantwortlichen.

Technische Referenzen:
[Arduino Nano](https://docs.arduino.cc/hardware/nano/),
[Nano-Prozessorauswahl](https://support.arduino.cc/hc/en-us/articles/4401874304274-Select-the-right-processor-for-Arduino-Nano),
[Microsoft SendInput](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput),
[serielle Timeouts](https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-commtimeouts).

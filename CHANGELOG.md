# Änderungen

## 1.2.2 – 04.10.2026

- Eigenes App-Icon: Leertaste mit Morseimpulsen, eingebettet für EXE,
  Hauptfenster und Dialoge in mehreren Windows-Icongrößen.
- Klassische Menüleiste mit „Datei“, „Optionen“ und „Hilfe“. „Über MorseBridge“
  zeigt Version, Build-Zeitpunkt in UTC und den Link zum GitHub-Projekt.
- Manuelle Updateprüfung gegen öffentliche stabile GitHub-Releases. Ein neuer
  Release lässt sich im Browser öffnen; keine automatische Installation,
  keine Hintergrundprüfung beim Start und keine Download-/Firmwareautomatik.
- Erscheinung wahlweise System, Hell, Dunkel, Mitternacht, Amber oder Matrix.
  Einstellungen bieten direkte Vorschau; „OK“ übernimmt, „Abbrechen“ verwirft
  die Vorschau. System folgt dem hellen/dunklen Windows-App-Modus.
- Farbschema und gültige Mithörfrequenz bleiben lokal unter
  `%LOCALAPPDATA%\MorseBridge\settings.ini` erhalten. Standard: System und
  650 Hz; PC-Mithörton beim Start weiterhin aus. COM-Auswahl, Pause und
  „Immer im Vordergrund“ werden nicht dauerhaft gespeichert.
- Versionsquelle für EXE, About und Paketbau vereinheitlicht; Build-Metadaten
  und Manifest werden beim Build erzeugt. Paketbau prüft die EXE-Version und
  baut die getestete EXE nicht erneut.
- Firmware und serielles Protokoll bleiben unverändert; erneutes Flashen ist
  für dieses Windows-Update nicht erforderlich. Der Eigentümer hat den Stand
  am 04.10.2026 mit „Das passt jetzt soweit“ freigegeben; dieselbe EXE wird veröffentlicht.
  Version 1.2.1 samt Tag, Release und Downloads bleibt erhalten.

## Wartung – 04.10.2026

- Monatlicher GitHub-Windows-Kompatibilitätstest: Produktionsbuild, Softwaretests
  und GUI-/Thread-Prüfung auf aktueller und älterer Windows-Runnergeneration.
- Die zuletzt veröffentlichte EXE wird zusätzlich unverändert getestet;
  Vergleichsbuild mit Microsofts C++-Compiler und installiertem Windows-SDK.
- Fehler erzeugen ein zugewiesenes Issue; grüne Folgeläufe schließen es.
  Laufzeitumgebungen und Prüfgrenzen sind in `TESTING.md` dokumentiert.

## 1.2.1 – 04.10.2026

- PC-Mithörton von waveOut auf ereignisgesteuertes WASAPI im Shared-Modus umgestellt.
  Geräteformat und Puffertakt stammen von der Windows-Audioengine; Multimedia-
  Scheduling für den Audio-Thread. Die festen drei 5-ms-Puffer entfallen.
- Sinus mit weicher 3-ms-Kosinus-Hüllkurve; Unterstützung für gerätespezifische
  Abtastraten, Float- und PCM-Formate, Stereoausgabe auf Front links/rechts.
- Puffer vor dem Start gefüllt; Nachfüllen nur im verfügbaren Bereich. Audiofehler
  deaktivieren die Ausgabe ohne automatische Geräte-Neustartschleife.
- Stumme Audio-Thread-/Formatprüfungen erweitert. Der Eigentümer hat den
  User-Test am optischen Windows-Ausgang am 04.10.2026 als bestanden bestätigt.
- Warnhinweis zum PC-Ton von 1.2.0p veröffentlicht: Rauer Klang und Windows-
  Einfrieren/Neustart mit 0xD1 gemeldet; kausaler Zusammenhang nicht nachgewiesen.
- Versionszusatz `p` entfällt ab 1.2.1. Alte Binärreleases, Testkopien und
  doppelte Dokumentation entfernt; Quellhistorie und historische Tags erhalten.
- Download auf ein vollständiges ZIP mit Prüfsummen beschränkt. Paketbau entfernt
  seine temporäre Arbeitskopie nach erfolgreichem Abschluss.

## 1.2.0p – Histogramm und PC-Mithörton – 03.10.2026

- Dynamisches Histogramm der Kontaktimpulslängen über die letzten fünf Minuten.
  Speicherung nur im RAM, mit 5-ms-Klassen bei üblichen Morsegeschwindigkeiten;
  längere Impulse erweitern die Skala. Verbindungsabbrüche erzeugen keine Messwerte.
- Impulse über 1.000 ms werden aus dem Histogramm ausgeschlossen.
- Optionaler Windows-PC-Mithörton als Sinus: Checkbox und Frequenzzähler oben
  rechts, 650 Hz voreingestellt, zulässiger Bereich 400–1.000 Hz.
- Ton folgt dem Kontakt unabhängig von der Space-Pause. Kurze Ein-/Ausblendung,
  eigener Audio-Thread, Stoppen bei Verbindungsabbruch, Sperre, Standby und Beenden.
- Histogramm und Audioeinstellungen bleiben ausschließlich im Arbeitsspeicher.
- Neue Prüfungen für Impulsgrenze, Sinusfrequenzen, Ein-/Ausblendung und GUI-Eingaben.

## 1.1.0p – Public-Ausgabe – 03.10.2026

- GitHub Actions: Cppcheck, geschwärzter Gitleaks-History-Scan und Tests mit Sanitizern.
- Fehler erzeugen/aktualisieren ein dem Eigentümer zugewiesenes GitHub-Issue;
  manuell auslösbarer Zustelltest, automatische Auflösung nach grünem Lauf.
- Dependabot für GitHub Actions und Repository-Sicherheitswarnungen eingerichtet.
- GPLv3, LLM-Disclaimer und rechtliche Hinweise zur Veröffentlichung.
- Vollständiges Quellen-/Binärpaket mit AVR-Core-Quellen und Runtime-Lizenzhinweisen.
- Einheitliche Version 1.1.0p in Paketnamen, GUI, Firmware und EXE-Metadaten.
- Optionaler lokaler Mithörton für aktive 5-V-Summer, insbesondere TMB12A05.
- Freigabeschalter an D4/GND; D8 steuert eine externe NPN-Transistorstufe.
- Summer ertönt nur bei freigegebenem Schalter und gedrückter Morsetaste.
- Separate, nicht blockierende Schalterentprellung; Startzustand aus.
- Bestehendes serielles Protokoll und Space-Ausgabe bleiben kompatibel.
- Anschlussplan und zusätzliche Firmware-Tests für Summer und Schalter.
- Versioniertes Komplettpaket enthält EXE, Sketch, HEX, Quellen und Dokumentation.

Frühere Versionen verwendeten den Zusatz `p` für Public; ab 1.2.1 entfällt er.

## 1.0.0 – 01.10.2026

- Nano-Firmware mit D2/GND-Kontakt, Entprellung und Zustands-Heartbeat.
- Native Windows-GUI mit COM-Erkennung, globaler Space-Ausgabe, Verlauf,
  Pause, Wiederverbindung und Freigabe bei normalem Beenden.
- Portable Buildskripte und automatisierte Softwareprüfungen.

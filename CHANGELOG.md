# Änderungen

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

`p` steht für Public und ist ein projektspezifischer Versionszusatz.

## 1.0.0 – 01.10.2026

- Nano-Firmware mit D2/GND-Kontakt, Entprellung und Zustands-Heartbeat.
- Native Windows-GUI mit COM-Erkennung, globaler Space-Ausgabe, Verlauf,
  Pause, Wiederverbindung und Freigabe bei normalem Beenden.
- Portable Buildskripte und automatisierte Softwareprüfungen.

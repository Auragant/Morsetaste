# Quellen zu Binärdateien und erneutes Linken

Neue Komplettpakete enthalten neben eigenem Code, Buildskripten und Lizenzen
auch die tatsächlich verwendeten Arduino-Core-/Variant-Quellen unter
`third_party/arduino-avr-1.8.8/`. Diese Dateien behalten ihre ursprünglichen
Lizenz-/Copyright-Hinweise. Der Firmware-Build enthält keinen Bootloader.

Die üblichen Buildbefehle stehen in `README.md`; Compiler und Arduino CLI werden
separat heruntergeladen, nicht als Tools im ZIP weiterverteilt. FQBN und Core-
Version sind im Buildskript angegeben. Quellen und Binärdateien immer als
zueinander gehörendes Paket weitergeben; bei Online-Einzeldownloads das passende
vollständige Quellenpaket gleichwertig und kostenfrei am selben Ort anbieten.

## Firmware mit einem veränderten Arduino-Core bauen

1. Komplettpaket entpacken, `scripts/get-tools.ps1` ausführen und anschließend
   `scripts/build-firmware.ps1 -InstallCore`. Dies installiert den ursprünglichen
   Core lokal unter `.tools/arduino-data/packages/arduino/hardware/avr/1.8.8/`.
2. Die mitgelieferten Quellen unter `third_party/arduino-avr-1.8.8/` nach Bedarf
   ändern. Copyright-/Lizenzhinweise erhalten und Änderungen kennzeichnen.
3. Geänderte Dateien an den entsprechenden Pfaden im lokal installierten Core
   einsetzen, insbesondere unter `cores/arduino/` und `variants/`. Dies betrifft
   nur die lokale Kopie dieses Projekts, nicht eine systemweite Arduino-Installation.
4. `scripts/build-firmware.ps1` **ohne** `-InstallCore` erneut ausführen. Die
   Firmware wird mit diesem Core neu kompiliert und verlinkt; die Sketch-HEX
   landet unter `dist/firmware/`. Zum Hochladen die Arduino IDE/CLI nutzen.
   Ein eventuell vorhandener Buildcache darf geänderte Dateien nicht verdecken;
   im Zweifel den Arduino-CLI-Build mit `compile --clean` wiederholen.

Das Projekt setzt keine Signaturpflicht oder eigene Flash-Sperre voraus.
Für die Windows-Anwendung stehen sämtliche eigenen C++-Quellen, Ressourcen
und das MinGW-Buildskript im selben Paket. Zusätzliche Runtime- und
Toolchain-Bedingungen: `THIRD_PARTY_NOTICES.md`.

Die Windows-Version wird zentral in `windows/version.hpp` festgelegt.
`scripts/build.ps1` liest sie für das Manifest und erzeugt den UTC-Buildzeitpunkt
unter `build/`. EXE-Metadaten und About verwenden dieselbe Versionsquelle.
Build-Metadaten sind generiert und werden für einen neuen Build neu erstellt;
statische Analyse benötigt diese Datei nicht. `scripts/package.ps1` gleicht
EXE-/Produktversion mit der Quelle ab und baut die EXE nicht erneut, damit die
getestete Datei in das Release gelangt. Ein optionales `-Version` muss ebenfalls
mit der zentralen Version übereinstimmen.

Vorhandene Werkzeuge können mit `build.ps1 -Compiler <g++.exe>` sowie
`build-firmware.ps1 -ToolsRoot <Ordner>` und `package.ps1 -ToolsRoot <Ordner>`
verwendet werden. `ToolsRoot` bezeichnet den Ordner mit `arduino-cli/` und
`arduino-data/`; Standard bleibt der lokale `.tools/`-Ordner.

Der Eigentümer hat den parallelen praktischen Test von `1.3.0` am 27.10.2026
allgemein als erfolgreich bewertet und die Veröffentlichung freigegeben.
Einzelne Punkte der praktischen Prüfliste wurden damit nicht gesondert bestätigt;
Details stehen in [USER_TEST_1.3.0.md](USER_TEST_1.3.0.md), automatisierte
Nachweise in [TESTING.md](../TESTING.md).
Das vollständige Quellen- und Binärpaket heißt
`MorseBridge-1.3.0-win64.zip`. Die Windows-App ergänzt die wählbare Ausgabetaste
(SPACE, Left CTRL und eigene Einzeltasten); Firmware und Arduino-Core bleiben
unverändert. Ein Windows-Update erfordert kein erneutes Flashen.

Die getestete EXE wird unverändert übernommen, mit SHA-256
`2f461e4f581b02203b42da7db092e10c99e142cd943348ff94bde327003b4973`.
Der erneute Paketbau ergänzt nur die aktualisierte Dokumentation und baut
die EXE nicht neu.

Bei separater Weitergabe von EXE oder HEX immer
auch den gleichwertigen Zugriff auf die passenden vollständigen Quellen
und Lizenzhinweise sicherstellen.

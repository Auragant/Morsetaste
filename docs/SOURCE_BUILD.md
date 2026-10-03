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

Zum Release `1.2.1` gehört das vollständige Quellen- und Binärpaket
`MorseBridge-1.2.1-win64.zip`. Bei separater Weitergabe von EXE oder HEX immer
auch den gleichwertigen Zugriff auf die passenden vollständigen Quellen
und Lizenzhinweise sicherstellen.

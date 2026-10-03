# Lizenzen und Drittanbieter-Komponenten

## Eigener Projektcode

Windows-Anwendung, Nano-Sketch, Tests und Buildskripte werden unter der
**GNU General Public License, Version 3 (GPL-3.0-only)** angeboten. Der vollständige
Text steht in [LICENSE](LICENSE). Eigene Projektdokumentation steht ebenfalls
unter dieser Lizenz, soweit daran eigene Rechte bestehen. Unveränderte fremde
Lizenztexte und Drittanbieter-Code behalten ihre jeweiligen Bedingungen.

Das App-Icon und seine PNG-/ICO-Dateien unter `windows/assets/` wurden mit
dem integrierten OpenAI Imagegen erzeugt. Motivwahl und Einbindung stammen aus
diesem Projekt; Generierungsprompt und PNG-Master sind beigefügt. Die Dateien
werden ebenfalls unter GPL-3.0-only angeboten, soweit eigene Rechte bestehen.

Copyright (C) 2026 Auragant, soweit urheberrechtlich geschützte eigene
Beiträge vorliegen. Die LLM-unterstützte Entstehung ist in
[DISCLAIMER.md](DISCLAIMER.md) offengelegt; damit wird kein ausschließlicher
Urheberrechtsschutz für rein maschinell erzeugte Bestandteile behauptet.

Bei Weitergabe kompilierter GPL-Programme müssen die zugehörigen vollständigen
Quellen einschließlich erforderlicher Buildskripte auf lizenzkonforme Weise
zugänglich sein. Ein isolierter Download der EXE oder HEX ohne zugehöriges
Quellenangebot reicht nicht als Veröffentlichungspaket. Änderungen und fremde
Hinweise erhalten; kein Verbot der GPL-erlaubten Änderung/Weitergabe hinzufügen.

## Arduino AVR Core 1.8.8

Die Firmware bindet den Arduino AVR Core statisch ein. Seine verwendeten
Arduino-/Wiring-Dateien sind überwiegend **LGPL-2.1-or-later**; einzelne Dateien
enthalten andere freie Lizenzhinweise. Diese Originalhinweise bleiben erhalten.
Die Lizenz der eigenen Firmware ersetzt nicht die Fremdlizenzen.

Für neue Komplettpakete kopiert `scripts/package.ps1` die tatsächlich verwendeten,
unveränderten Core- und Variant-Quellen samt Plattform-/Boarddefinitionen nach
`third_party/arduino-avr-1.8.8/`. Der LGPL-2.1-Text steht in
[licenses/LGPL-2.1.txt](licenses/LGPL-2.1.txt), GPLv3 in `LICENSE`.
Die Core-Quellen stammen aus dem installierten Paket `arduino:avr@1.8.8`;
Buildanleitung: `README.md` und `scripts/build-firmware.ps1`.
Das Vorgehen für einen veränderten Core ist in
[docs/SOURCE_BUILD.md](docs/SOURCE_BUILD.md) beschrieben.

Bezugsquelle: [ArduinoCore-avr 1.8.8](https://github.com/arduino/ArduinoCore-avr/tree/1.8.8).
Ein vollständiges Quellenpaket mit frei änderbarer GPLv3-Firmware ermöglicht
erneutes Kompilieren/Linken mit einem geänderten Core. Es werden keine
Bootloader-Binärdateien mitverteilt; verwendet wird nur die Sketch-HEX.

## AVR-Laufzeit

AVR-GCC **7.3.0-atmel3.6.1-arduino7** und avr-libc **2.0.0** werden beim Firmware-
Build verwendet. GCC-Laufzeitanteile stehen unter GPL mit der
[GCC Runtime Library Exception 3.1](licenses/GCC-exception-3.1.txt).
avr-libc enthält BSD-artig lizenzierte Bestandteile; seine vollständigen
Copyright-/Lizenzhinweise stehen in [licenses/avr-libc-2.0.0.txt](licenses/avr-libc-2.0.0.txt).
Die Compilerprogramme selbst werden nicht im Komplettpaket mitverteilt.

## Windows-Laufzeit

Die EXE wird mit **w64devkit 2.10.0 / GCC 16.2.0** gebaut und verwendet statische
GCC-/C++-Laufzeitbestandteile (GPL mit GCC Runtime Library Exception) sowie
MinGW-w64-Laufzeitbestandteile einschließlich der zugehörigen weiteren freien
Lizenzbedingungen. Die mit dem verwendeten Toolchain-Paket ausgelieferten
vollständigen Hinweise sind unverändert in
[licenses/MinGW-w64-runtime.txt](licenses/MinGW-w64-runtime.txt) enthalten.

Microsoft-Windows-System-DLLs werden nur referenziert, nicht mitverteilt.
Die EXE ist kein Microsoft-Produkt und benötigt ein zulässig genutztes Windows.

## Nur Build-/Prüfwerkzeuge, nicht Teil des Programms

Arduino CLI 1.5.1, w64devkit, Cppcheck, Gitleaks und GitHub Actions werden als
Werkzeuge genutzt, aber nicht in das Release-ZIP eingebettet. Ihre Lizenzpflichten
sind gesondert zu prüfen, falls jemand die Werkzeuge selbst weiterverteilt.
Ein Compilerwechsel oder zusätzliche Bibliotheken erfordern eine erneute
Prüfung dieser Liste. Sie ist kein automatischer oder lückenloser SBOM-Scan.

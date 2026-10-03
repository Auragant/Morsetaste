# Prüfung vor einer öffentlichen Veröffentlichung

Stand: **03.10.2026**. Annahme: nichtkommerzielles Hobbyprojekt, zunächst nur
Quellcode und kostenlose Downloads, keine verkauften Fertiggeräte. Dies ist
eine dokumentierte Vorprüfung anhand öffentlicher Primärquellen, **keine
anwaltliche Rechtsberatung, vollständige Rechteklärung oder Freigabebescheinigung**.
Verantwortlich für dieses Projekt ist der GitHub-Alias **Auragant**.

## Lizenzierung und Release-Umfang

1. **Eigener Projektcode: GPLv3.** Lizenzumfang und Hinweise stehen in
   `LICENSE`, SPDX-Hinweisen im Code und
   `THIRD_PARTY_NOTICES.md`. Ein KI-Hinweis allein erlaubt keine Weiterverwendung.
   Rechte unbekannter Dritter werden nicht mitlizenziert.
2. **Firmware-HEX enthält fremden Arduino-Code.** Die geprüften lokalen Dateien
   `Arduino.h`/`wiring.c` nennen LGPL 2.1 oder später. Neue Komplettpakete enthalten
   deshalb die verwendeten Core-/Variant-Quellen und Lizenztexte zusätzlich zum
   Sketch und den Buildskripten. Ein vollständiges Quellenangebot muss auch die
   eingebundenen Komponenten berücksichtigen. Fremde Copyright-Vermerke bleiben erhalten.
   [Arduino: Software-Lizenzierung](https://support.arduino.cc/hc/en-us/articles/4415094490770-Licensing-for-products-based-on-Arduino),
   [LGPL 2.1, insbesondere §§ 3 und 6](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html).
3. **Statische Laufzeiten benötigen Hinweise.** Die Win32-EXE enthält MinGW-w64-
   und GCC-Laufzeitteile, die Firmware AVR-Laufzeitteile. Die vorhandenen
   Toolchain-Hinweise, avr-libc-Lizenz, GPL und GCC Runtime Library Exception
   werden dem neuen Komplettpaket beigefügt. GCC-Nutzung macht eigenen Code
   nicht automatisch GPL-pflichtig; die GPLv3-Auswahl hier ist bewusst getroffen.
   [GNU: GCC Runtime Library Exception und FAQ](https://www.gnu.org/licenses/gcc-exception-3.1-faq.html).
4. **Vollständiges Quellen-/Binärpaket.** Das jeweilige
   Komplett-ZIP enthält Programm, Firmware, eigene und verwendete Core-Quellen,
   Buildskripte sowie Lizenzhinweise. Zu separat angebotenen EXE-/HEX-Dateien
   wird dieses Paket am selben Downloadort kostenfrei bereitgestellt. Der
   Versionsname ist keine rechtliche Freigabebescheinigung.

## LLM, Urheberrecht und mögliche fremde Rechte

Das UrhG verlangt für Werksschutz eine persönliche geistige Schöpfung;
Computerprogramme haben eine entsprechende Schutzvoraussetzung. Daraus folgt
als vorsichtige Einordnung: Rein maschinell erzeugte Teile haben nicht schon
wegen ihrer Erzeugung exklusiven menschlichen Urheberrechtsschutz. Individuelle
menschliche Auswahl, Bearbeitung und Beiträge sind separat zu beurteilen.
Die GPL wird nur im Umfang bestehender eigener Rechte angeboten.
[§ 2 UrhG](https://www.gesetze-im-internet.de/urhg/__2.html),
[§ 69a UrhG](https://www.gesetze-im-internet.de/urhg/__69a.html).

LLM-Ausgaben können trotzdem fremdem geschütztem Code ähneln. Compiler,
Cppcheck, Gitleaks und Dependabot prüfen **keine** Urheberrechtsherkunft oder
Patentrechte. Es wurde keine weltweite Ähnlichkeits-, Patent- oder Marken-
Recherche durchgeführt. Auffällige übernommene Passagen vor Veröffentlichung
auf ihre Herkunft prüfen; bei unklaren Rechten fachkundigen Rat einholen.
`DISCLAIMER.md` legt die LLM-Mitwirkung offen, garantiert aber keine Rechtefreiheit.

## Haftung, Marken und Daten

Eine pauschale Aussage „keinerlei Haftung“ wäre keine sichere Lösung. Insbesondere
Vorsatz lässt sich nicht im Voraus ausschließen; bei AGB bestehen weitere
Schranken für Personenschäden/grobes Verschulden. Der Disclaimer erhält deshalb
zwingende Rechte ausdrücklich und erklärt Risiken statt völliger Haftungsfreiheit.
[§ 276 Abs. 3 BGB](https://www.gesetze-im-internet.de/bgb/__276.html),
[§ 309 Nr. 7 BGB](https://www.gesetze-im-internet.de/bgb/__309.html).

Arduino-/Junker-/Windows-Bezeichnungen nur zur Kompatibilitätsbeschreibung
verwenden, keine Herstellerfreigabe suggerieren und keine fremden Logos oder
Datenblattabbildungen übernehmen. Im Projekt sind Datenblätter verlinkt, nicht
als fremde PDF-/Bilddateien mitverteilt; Arduino-Markenhinweis ergänzt.
[Arduino: Trademark & Copyright](https://www.arduino.cc/en/trademark).

Die Anwendung hat nach dem geprüften Quellstand keine Telemetrie und benötigt
keinen Netzwerkzugriff; sie verarbeitet COM- und Tastenzustände lokal. Das ist
keine generelle Befreiung von Datenschutzpflichten bei späteren Änderungen.
Zur Veröffentlichung werden Quellen, Git-Metadaten, Screenshots und Downloads
auf private Angaben geprüft. Eigene Beiträge verwenden den Alias **Auragant**;
Git benötigt eine technische Autoradresse, dafür wird ausschließlich die
GitHub-Noreply-Adresse verwendet, keine persönliche Kontaktadresse.
Erforderliche Urheber-/Lizenzhinweise fremder Komponenten bleiben unverändert.

## Falls daraus ein kommerzielles Angebot wird

Bei verkauften Geräten/Bausätzen oder einem geschäftlichen Downloadangebot
sind zusätzliche Pflichten separat zu prüfen: Produktsicherheit, EMV/CE,
RoHS/Elektrorecht, Verbraucherrechte, Datenschutz und gegebenenfalls Impressum.
Diese Vorprüfung erteilt keine Konformitätsfreigabe für Hardware. Für
geschäftsmäßige digitale Dienste enthält § 5 DDG Informationspflichten;
nicht jedes rein private Hobby-Repository ist dadurch automatisch ein Geschäft.
[§ 5 DDG](https://www.gesetze-im-internet.de/ddg/__5.html),
[EU: CE-Kennzeichnung](https://europa.eu/youreurope/business/product-requirements/labels-markings/ce-marking/index_de.htm).

Beim Cyber Resilience Act hängt die Einordnung unter anderem davon ab, ob freie
Open-Source-Software im Rahmen einer kommerziellen Tätigkeit auf dem Markt
bereitgestellt wird. Die Kommission beschreibt eine Ausnahme für nichtkommerziell
bereitgestellte freie Open-Source-Software. Für Produkte im Anwendungsbereich
gelten Meldepflichten seit 11.09.2026, die wesentlichen übrigen Pflichten ab
11.12.2027. Ein späterer Verkauf ist deshalb erneut zu prüfen.
[EU-Kommission: CRA und Open Source](https://digital-strategy.ec.europa.eu/en/policies/cra-open-source),
[CRA-Zeitplan](https://digital-strategy.ec.europa.eu/en/policies/cyber-resilience-act).

## Freigabe-Checkliste

- [x] GPLv3 gewählt; vollständiger Text und Drittanbieter-Hinweise ergänzt.
- [x] LLM-Mitwirkung, technische Grenzen und zwingende Haftung offengelegt.
- [x] Paketbau um Core-Quellen und Lizenztexte erweitert.
- [x] Neuen Paketbau lokal mit Binärdateien, Core-Quellen und Lizenztexten geprüft.
- [x] Ersten geprüften Komplettrelease aus dem zugehörigen Quellstand angelegt.
- [x] Veröffentlichung ausschließlich unter dem Alias Auragant beauftragt.
- [x] Bereinigte Quellen, Binärdateien und Komplettpaket ohne gefundene persönliche Angaben des Eigentümers geprüft.
- [x] Eigene Git-Commits und Tag-Metadaten ausschließlich Auragant/GitHub-Noreply.
- [x] GitHub-Security-Lauf erfolgreich; Test-Issue erstellt, zugewiesen und erwähnt.
- [x] Tatsächlichen Empfang der GitHub-Benachrichtigung vom Eigentümer bestätigt.
- [x] Reale Nano-/Junker-/Summer-Kette mit dem Zielprogramm vom Eigentümer bestätigt.
- [x] Vertraulichen Meldekanal aktiviert und per GitHub-API als eingeschaltet geprüft.
- [x] Native CodeQL-Analyse eingerichtet; Secret Scanning und Push Protection aktiviert.
- [x] Öffentliche Sichtbarkeit vom Eigentümer ausdrücklich freigegeben.
- [x] Repository öffentlich unter Auragant/Morsetaste bereitgestellt.

### Datenschutz und Meldeweg

Es werden keine persönlichen Kontaktdaten des Projektverantwortlichen als
Kontaktweg veröffentlicht. Vertrauliche Sicherheitsmeldungen sollen über
**Security → Report a vulnerability** auf GitHub erfolgen. Die Identität und
Kontodaten hinter einem GitHub-Konto sowie etwaige bereits vorhandene fremde
Kopien sind keine durch dieses Repository kontrollierbaren Daten.
Eine vollständige Anonymität oder weltweite Löschung aller Kopien wird nicht
zugesichert. Der vertrauliche Meldekanal ist im öffentlichen Repository aktiviert;
die GitHub-API bestätigt `enabled: true`.
[GitHub: Private vulnerability reporting](https://docs.github.com/en/code-security/how-tos/report-and-fix-vulnerabilities/configure-vulnerability-reporting/configure-for-a-repository).

Hardwarefunktion und tatsächlicher Benachrichtigungsempfang wurden am
03.10.2026 vom Eigentümer ausdrücklich bestätigt. Dies ist ein berichteter
Praxistest des konkreten Aufbaus, keine unabhängige Zertifizierung aller
Hardware- oder Windows-Varianten.

Bei kostenlosem privatem Hobbyumfang sind die Punkte zu kommerziellem Angebot
oben bedingt relevant, nicht pauschal zusätzliche Release-Blocker. Eine spätere
kommerzielle Bereitstellung oder Hardwareverteilung erfordert eine neue Prüfung.

Ergebnis: Die Freigabepunkte für diesen nichtkommerziellen Hobbyumfang sind
abgearbeitet. Lizenztexte, LLM-Hinweis und die zugehörigen Quellen sind
Bestandteil des Release-Pakets. Eine uneingeschränkte rechtliche Unbedenklichkeit
wird nicht behauptet; kommerzielle Bereitstellung oder Hardwareverteilung
erfordern eine neue Prüfung.

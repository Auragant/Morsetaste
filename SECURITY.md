# Sicherheit

## Automatische Prüfungen auf GitHub

Der Workflow [Security checks](https://github.com/Auragant/Morsetaste/actions/workflows/security.yml)
läuft bei Änderungen auf `main`, bei Pull Requests, montags um 06:23 UTC und
manuell über **Actions → Security checks → Run workflow**. Geplante Läufe
können bei GitHub verzögert werden; Actions muss aktiviert bleiben und das
Minutenkontingent des Kontos darf nicht erschöpft sein. Es wurde kein kostenpflichtiges
Security-Abonnement eingerichtet und kein Ausgabenlimit geändert.

- Cppcheck: statische C++-Prüfung der Win32-Anwendung und der Firmware über den
  Arduino-I/O-Teststub. Das ersetzt weder einen vollständigen Win32-/AVR-Build
  noch Hardwaretests.
- Gitleaks: bekannte Muster für Zugangsdaten in der vollständigen ausgecheckten
  Git-Historie. Werte werden in den Logs geschwärzt. Auch gelöschte Geheimnisse
  müssen bei einem Befund widerrufen/ersetzt werden.
- Kern-/Firmware-/Update-/Einstellungstests: tatsächlicher plattformunabhängiger
  Code mit AddressSanitizer und UndefinedBehaviorSanitizer unter Linux.
  Windows-spezifische Transport- und Dateisystempfade werden zusätzlich lokal
  unter Windows getestet. Die Updateprüfungen sind offline.
- Dependabot: wöchentliche Update-Vorschläge für GitHub Actions. Repository-
  Sicherheitswarnungen und automatische Sicherheitsupdates sind aktiviert.
  Manuell heruntergeladene Compiler, Arduino-Core und Gitleaks werden dadurch
  **nicht** automatisch auf alle bekannten Schwachstellen geprüft/aktualisiert.
- GitHubs natives CodeQL: Default-Setup mit erweiterter Abfragesuite für C/C++,
  JavaScript/TypeScript und GitHub Actions; lokale und entfernte Eingabequellen.
  Das ist eine zusätzliche Analyse, kein Ersatz für einen Windows-/AVR-Build.
- GitHubs natives Secret Scanning und Push Protection sind aktiviert. Diese
  prüfen unterstützte Geheimnismuster und bieten keinen vollständigen Schutz
  vor der Veröffentlichung persönlicher Angaben.

Actions sind an vollständige Commit-SHAs gebunden; der Gitleaks-Download muss
seinen fest hinterlegten SHA-256 erfüllen. Prüfjobs erhalten nur lesende
Repository-Rechte und keine dauerhaft gespeicherten Git-Zugangsdaten. Nur der
getrennte Benachrichtigungsjob darf Issues schreiben; er checkt keinen Code aus
und läuft nicht für Pull Requests oder andere Branches als den Default-Branch.

Die Ergebnisse des eigenen Workflows stehen unter **Actions**. Die zusätzlichen
nativen CodeQL-/Secret-Scanning-Ergebnisse stehen unter **Security**. Cppcheck
ist kein CodeQL-Scan und lädt selbst keine CodeQL-Alerts in den Security-Tab hoch.
Der Aktivierungsstatus wird in der Veröffentlichungs-Checkliste dokumentiert.
Quelle: [GitHub: Verfügbarkeit von Code Scanning](https://docs.github.com/en/code-security/concepts/code-scanning/code-scanning).

### Einordnung des ersten CodeQL-Hinweises

Am 03.10.2026 meldete CodeQL `js/code-injection` im lokalen
`tests/security_notify_tests.cjs`. Der Test liest ausschließlich den festen Pfad
`.github/workflows/security.yml` aus demselben Checkout und führt dessen
Benachrichtigungs-Script absichtlich mit simulierten GitHub-Objekten aus. Es
gibt keinen Netzwerkdienst, frei wählbaren Dateipfad oder externen Eingabewert.
Wer diesen Repository-Code verändern kann, kann bereits den Test oder Workflow
selbst verändern. Hier wird keine zusätzliche Vertrauensgrenze überschritten.
Der Hinweis wird deshalb mit dieser Begründung als **Fehlalarm** eingeordnet,
nicht durch Ausschluss des Tests oder Abschalten der Regel unterdrückt.

`node:vm` ist ausdrücklich **keine Sicherheits-Sandbox**. Lokale Tests nur aus
einem vertrauenswürdigen Checkout ausführen. Der GitHub-Testjob hat keine Secrets,
keine Issues-Schreibrechte und checkt ohne persistierte Zugangsdaten aus; der
separate privilegierte Benachrichtigungsjob führt diesen Test nicht aus.
Diese Einordnung gilt nicht für spätere Erweiterungen um fremde Eingaben.
[CodeQL: Code injection](https://codeql.github.com/codeql-query-help/javascript/js-code-injection/),
[Node.js: Grenzen von node:vm](https://nodejs.org/api/vm.html).

Der neue Kompatibilitäts-Benachrichtigungstest verwendet ab 04.10.2026 eine
statisch importierte Funktion aus `.github/scripts/compatibility-notify.cjs`.
Er prüft ihre Übereinstimmung mit dem eingebetteten Workflow-Code und führt
ausschließlich diese Funktion mit simulierten GitHub-Objekten aus. JavaScript
aus der YAML-Datei wird nicht dynamisch ausgeführt. Damit wurde der zu Beginn
gemeldete [CodeQL-Hinweis 2](https://github.com/Auragant/Morsetaste/security/code-scanning/2)
durch eine Codeänderung behoben; keine Regel oder Prüfung wird deaktiviert.
Der privilegierte Job benötigt weiterhin keinen Checkout.

## Benachrichtigungen

Bei einem fehlgeschlagenen Prüflauf auf dem Default-Branch erstellt
`github-actions[bot]` ein Issue **[Security] Prüflauf benötigt Aufmerksamkeit**,
weist es dem Repository-Eigentümer zu und erwähnt ihn. Weitere fehlgeschlagene
Läufe kommentieren dasselbe offene Issue. Ein vollständiger grüner Lauf schließt
es; abgebrochene/übersprungene Läufe tun dies nicht. Ein Scannerfehler ist nicht
automatisch ein nachgewiesenes Sicherheitsproblem. Auch Tool-/Downloadfehler
werden gemeldet, damit die Überwachung nicht unbemerkt ausfällt.

Für einen Zustelltest gibt es den manuellen Workflow-Schalter
`test_notification`: Er erstellt ein eindeutig als **TEST** bezeichnetes,
zugewiesenes Issue ohne künstlichen Sicherheitsbefund. Dieses Issue danach
schließen. Die Scanner laufen auch beim Zustelltest normal.

Die tatsächliche Zustellung hängt von deinen persönlichen GitHub-Einstellungen
ab. Unter [GitHub → Settings → Notifications](https://github.com/settings/notifications)
für **Participating, @mentions and custom** mindestens **On GitHub** und für
E-Mail zusätzlich **Email** einschalten und eine verifizierte Adresse verwenden.
Dependabot-Warnungen haben dort eigene Einstellungen; Actions-Meldungen können
zusätzlich auf fehlgeschlagene Läufe beschränkt werden. Das Repository darf
nicht auf **Ignore** stehen. Für die zusätzlichen nativen CodeQL-/Secret-
Scanning-Warnungen auch die Security-Benachrichtigungen auf GitHub beachten.
Die persönlichen E-Mail-/Push-Einstellungen werden
von diesem Repository nicht verändert und eine E-Mail-Zustellung ist nicht
garantiert.
Quelle: [GitHub: Benachrichtigungen konfigurieren](https://docs.github.com/en/subscriptions-and-notifications/get-started/configuring-notifications).

## Manuelle Updateprüfung und lokale Einstellungen

Ab 1.2.2 startet nur „Hilfe → Nach Updates suchen“ eine HTTPS-Anfrage an die
öffentliche GitHub-Releases-API für `Auragant/Morsetaste`. Dabei erhält GitHub
übliche Verbindungsdaten und eine Programm-/Versionskennung. COM-Port,
Kontaktzustände und Histogramm werden nicht übertragen. Beim Start und während
des normalen Betriebs erfolgt keine automatische Anfrage und keine Telemetrie.
Die App öffnet bei Bedarf eine Release-Seite im Browser; sie installiert oder
ersetzt weder EXE noch Firmware automatisch.

Theme und gültige Mithörfrequenz werden ausschließlich lokal unter
`%LOCALAPPDATA%\MorseBridge\settings.ini` gespeichert. Fehlerhafte Einstellungen
führen zu den Standardwerten System/650 Hz; Speicherfehler verhindern weder
den Start noch den lokalen Betrieb. Der PC-Mithörton startet weiterhin aus.
Update- und Einstellungsprüfungen laufen mit Testantworten und isolierten
temporären Verzeichnissen, ohne reale GitHub-Anfragen oder Benutzerdateien.

## Probleme melden

Nicht sensible Fehler können als GitHub-Issue gemeldet werden. Zugangsdaten,
personenbezogene Daten und noch nicht behobene ausnutzbare Schwachstellen nicht
in öffentliche Issues oder Workflow-Logs schreiben. **Private vulnerability
reporting** ist aktiviert. Vertrauliche Meldungen über
[Security → Report a vulnerability](https://github.com/Auragant/Morsetaste/security/advisories/new)
senden; dazu ist eine GitHub-Anmeldung erforderlich. Den geprüften
Aktivierungsstatus dokumentiert die [Veröffentlichungs-Checkliste](docs/PUBLICATION_REVIEW.md).
Es wird keine persönliche Kontaktadresse des Projektverantwortlichen veröffentlicht.

Es gibt aktuell keinen verbindlich zugesagten Wartungszeitraum oder SLA.
Ein grüner Scan ist kein Sicherheitsaudit, keine Rechtsprüfung und keine
Garantie, dass alle Schwachstellen gefunden werden.

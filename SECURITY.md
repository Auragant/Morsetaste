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
- Protokoll-/Firmwaretests: tatsächlicher Kerncode mit AddressSanitizer und
  UndefinedBehaviorSanitizer unter Linux.
- Dependabot: wöchentliche Update-Vorschläge für GitHub Actions. Repository-
  Sicherheitswarnungen und automatische Sicherheitsupdates sind aktiviert.
  Manuell heruntergeladene Compiler, Arduino-Core und Gitleaks werden dadurch
  **nicht** automatisch auf alle bekannten Schwachstellen geprüft/aktualisiert.

Actions sind an vollständige Commit-SHAs gebunden; der Gitleaks-Download muss
seinen fest hinterlegten SHA-256 erfüllen. Prüfjobs erhalten nur lesende
Repository-Rechte und keine dauerhaft gespeicherten Git-Zugangsdaten. Nur der
getrennte Benachrichtigungsjob darf Issues schreiben; er checkt keinen Code aus
und läuft nicht für Pull Requests oder andere Branches als den Default-Branch.

Die Ergebnisse dieses Workflows stehen unter **Actions**. Cppcheck ist kein
CodeQL-Scan und lädt keine CodeQL-Alerts in den Security-Tab hoch. Für öffentliche
Repositories ist zusätzlich natives CodeQL und Secret Scanning verfügbar;
der Aktivierungsstatus wird in der Veröffentlichungs-Checkliste dokumentiert.
Quelle: [GitHub: Verfügbarkeit von Code Scanning](https://docs.github.com/en/code-security/concepts/code-scanning/code-scanning).

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
nicht auf **Ignore** stehen. Die persönlichen E-Mail-/Push-Einstellungen werden
von diesem Repository nicht verändert und eine E-Mail-Zustellung ist nicht
garantiert.
Quelle: [GitHub: Benachrichtigungen konfigurieren](https://docs.github.com/en/subscriptions-and-notifications/get-started/configuring-notifications).

## Probleme melden

Nicht sensible Fehler können als GitHub-Issue gemeldet werden. Zugangsdaten,
personenbezogene Daten und noch nicht behobene ausnutzbare Schwachstellen nicht
in öffentliche Issues oder Workflow-Logs schreiben. Für vertrauliche Meldungen
ist **Private vulnerability reporting** vorgesehen: Nach dessen Aktivierung
unter **Security → Report a vulnerability** melden. Den geprüften
Aktivierungsstatus dokumentiert die [Veröffentlichungs-Checkliste](docs/PUBLICATION_REVIEW.md).
Es wird keine persönliche Kontaktadresse des Projektverantwortlichen veröffentlicht.

Es gibt aktuell keinen verbindlich zugesagten Wartungszeitraum oder SLA.
Ein grüner Scan ist kein Sicherheitsaudit, keine Rechtsprüfung und keine
Garantie, dass alle Schwachstellen gefunden werden.

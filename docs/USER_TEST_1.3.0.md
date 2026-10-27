# Nutzertest und Freigabe für MorseBridge 1.3.0

Der Eigentümer hat am **27.10.2026** den parallel durchgeführten praktischen
Test allgemein als erfolgreich bewertet und die Veröffentlichung freigegeben:
„Der parallele Test sah gut aus. Ich würde das so veröffentlichen“.

Diese allgemeine Bestätigung belegt nicht zusätzlich jeden einzelnen Punkt
der untenstehenden Prüfliste. Automatisierte Prüfergebnisse werden getrennt
in [TESTING.md](../TESTING.md) dokumentiert; sie ersetzen keine Bestätigung
einzelner praktischer Hardware- oder Bedienprüfungen.

Die lokal automatisiert geprüfte EXE liegt unter `dist/MorseBridge.exe`;
das Komplettpaket heißt `dist/MorseBridge-1.3.0-win64.zip`.
Vorhandene MorseBridge-Instanzen zum Wechseln der EXE normal schließen.
Firmware und Verkabelung bleiben unverändert; erneutes Flashen ist nicht nötig.

SHA-256 der getesteten und zur Veröffentlichung freigegebenen EXE:

```text
2f461e4f581b02203b42da7db092e10c99e142cd943348ff94bde327003b4973
```

Für die Veröffentlichung wird genau diese EXE unverändert übernommen.
Das Paket wird für die ergänzte Freigabe- und Release-Dokumentation erneut
erstellt; die getestete EXE wird dabei nicht neu gebaut.

Die folgende Prüfliste bleibt als Referenz für den Testumfang erhalten;
die allgemeine Freigabe bestätigt ihre Punkte nicht einzeln:

1. Mit dem verwendeten Tastkontakt, Nano und dem gewünschten Zielprogramm SPACE
   wählen. Halten und Freigeben müssen dem Schließen und Öffnen des Kontakts
   folgen. Danach denselben Test mit Left CTRL durchführen; gemeint ist die
   linke Strg-Taste.
2. Unter „Optionen → Einstellungen“ etwa F1 und Right CTRL hinzufügen und mit OK
   übernehmen. Beide müssen im Hauptdropdown erscheinen und funktionieren.
   Hinzufügen/Entfernen mit Abbrechen darf die gespeicherte Liste nicht verändern.
3. Eine eigene Taste wählen, die App normal beenden und neu starten. Liste und
   Auswahl müssen erhalten bleiben. Wird die aktive eigene Taste entfernt und
   mit OK übernommen, muss wieder SPACE ausgewählt sein.
4. Während eines gehaltenen Impulses ins MorseBridge-Fenster wechseln und eine
   andere Ausgabetaste wählen. Die bisherige Ausgabe muss freigegeben werden.
   Die neue Taste darf erst nach Öffnen und erneutem Schließen des Kontakts senden.
5. Mit Left CTRL und einer eigenen Taste jeweils Pause/Fortsetzen, USB-Abziehen,
   Sperren/Entsperren, Standby/Aufwecken und normales Beenden prüfen. Es darf keine
   von MorseBridge gehaltene Taste zurückbleiben. Nach Unterbrechungen zuerst
   den Kontakt öffnen, dann einen neuen Impuls senden.
6. PC-Mithörton, Themes und manuelle Updateprüfung kurz prüfen. In der normalen
   Anwendung wird der tatsächliche GitHub-Release abgefragt; die simulierte
   Updateversion im UI-Test ist kein veröffentlichter Release.

Quellstand, Security-/CodeQL-/Windows-Prüfungen sowie die automatisierten
Ergebnisse und ihre Grenzen stehen in [TESTING.md](../TESTING.md).

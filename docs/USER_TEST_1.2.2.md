# Lokaler User-Test für MorseBridge 1.2.2

Der Eigentümer hat den lokalen Stand am 04.10.2026 mit „Das passt jetzt soweit“
freigegeben. Einzelne Schritte wurden nicht gesondert bestätigt; die folgende
Liste hält den vorgesehenen Testumfang fest. Die getestete `dist/MorseBridge.exe`
wird unverändert veröffentlicht.
Der Nano braucht für 1.2.2 keine neue Firmware.

1. **Start und Verbindung:** About zeigt 1.2.2 und den UTC-Buildzeitpunkt;
   eigenes App-Icon in Explorer, Taskleiste, Hauptfenster und Dialogen prüfen.
   Nano automatisch erkennen lassen. Mithörton und „Immer im Vordergrund“
   müssen beim Start ausgeschaltet sein.
2. **Frequenz und Neustart:** eine gültige Tonhöhe einstellen, Eingabe mit Enter
   oder Verlassen des Felds abschließen und App normal beenden. Beim Neustart
   muss die Tonhöhe erhalten bleiben; COM neu suchen, Ton und Vordergrund aus.
   Ungültige Eingaben dürfen den letzten gültigen Wert nicht überschreiben.
3. **Themes und Bedienung:** alle sechs Themes einschließlich Menü, Dropdowns,
   Checkboxen, Einstellungen und About prüfen. Vorschau mit Abbrechen verwerfen
   und mit OK speichern. Tab, Enter, Escape und Menübedienung mit Alt prüfen.
   Wenn möglich 100/150/200 % Skalierung, System-Hell/Dunkel-Wechsel und den
   Windows-Kontrastmodus testen. Themewechsel dürfen Ton oder COM-Verbindung
   nicht unterbrechen.
4. **Nano, Zielprogramm und Audio:** Kontakt/Space, Mithörton und Pause-Hotkey
   `Strg + Alt + F12` im tatsächlichen Zielprogramm testen. Auch während
   gehaltener Taste Menü, Dialog und GitHub-Link öffnen; nach Rückkehr muss
   Loslassen vor erneuter Ausgabe erforderlich sein. USB-Abziehen, Sperren,
   Standby und Beenden müssen die Ausgabe sicher freigeben.
5. **Manuelle Updateprüfung:** Hilfe-Menü und About-Schaltfläche ausprobieren.
   Ohne neueren stabilen Release meldet die Prüfung keine verfügbare Aktualisierung.
   Offline und Abbrechen prüfen; App muss bedienbar bleiben und sich während
   einer Anfrage vollständig beenden lassen.

Bei einem Problem bitte konkreten Schritt, Theme/Skalierung und beobachtetes
Verhalten nennen. Nutzerfreigabe, automatische Prüfergebnisse und praktische
Prüfgrenzen stehen in `TESTING.md`.

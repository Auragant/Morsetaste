# User-Test für MorseBridge 1.2.1

Status: lokale Testversion. **Vor einem GitHub-Release muss der Eigentümer
Klang, Timing und Stabilität an seinem optischen Windows-Ausgang prüfen.**
Der gemeldete Windows-Absturz von 1.2.0p ist nicht abschließend aufgeklärt;
dieser Test ist keine Zusicherung, dass der Treiberfehler behoben wurde.

1. Alte MorseBridge-Version schließen. Die Test-EXE starten und unten **v1.2.1**
   prüfen. PC-Mithörton muss zunächst aus sein. Den gewünschten optischen Ausgang
   in Windows wählen; mit moderater Lautstärke beginnen.
2. PC-Mithörton bei **650 Hz** einschalten. Einige kurze Dits und Dahs geben,
   zunächst mit MorseBridge im Vordergrund. Klang soll gleichmäßig und sauber
   sein, ohne Rauigkeit, Knattern oder Knacken an den Impulsgrenzen.
3. **400 Hz und 1.000 Hz** kurz prüfen. Danach die bevorzugte Frequenz einstellen.
   Bei pausierter Space-Ausgabe muss der Ton dem Kontakt weiterhin folgen.
4. Im normalen Morse-Zielprogramm testen. Für die Klangbeurteilung einen dort
   möglicherweise vorhandenen zweiten Mithörton abschalten. Impulsanfang/-ende
   und gefühlte Verzögerung prüfen, anschließend einige Minuten normal morsen.
5. Checkbox ausschalten, wieder einschalten, danach die App während eines Tons
   schließen. Ton und Programm müssen vollständig enden.
6. Histogramm prüfen: normale Impulse erscheinen; Halten über 1 Sekunde fließt
   weiterhin nicht ein. Die Audiofunktion darf diese Auswertung nicht verändern.

Bitte Ergebnis zurückmelden: **Klang**, **gefühlte Verzögerung**, **Stabilität**
und **gegebenenfalls sichtbare Fehlermeldung**. Bei erneutem Einfrieren oder
deutlichen Audiofehlern den Test abbrechen; das Release bleibt dann zurückgestellt.

Die Veröffentlichung als **v1.2.1** erfolgt erst nach einem positiven User-Test.

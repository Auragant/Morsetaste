# Optionaler aktiver Summer (TMB12A05)

Ab JunkerSpace **1.1** kann ein aktiver Summer lokal am Nano mitlaufen.
Ein Schalter zwischen **D4 und GND** gibt ihn frei. Es gilt immer:

**Summer an = Schalter geschlossen UND Morsetaste gedrückt.**

Bei offenem oder nicht angeschlossenem Schalter bleibt der Summer aus.
Der Schalter schaltet den Mithörton frei, er erzeugt keinen Dauerton und
schaltet die Space-Ausgabe nicht ab. Sein Zustand wird direkt ausgewertet;
ein kurz betätigter Taster würde den Ton deshalb nur während des Festhaltens
freigeben. Für dauerhaft Ein/Aus einen rastenden Schalter oder einen mechanisch
entsprechend gehaltenen Mikroschalter verwenden.

## Anschluss

Der TMB12A05 ist in der hier zugrunde gelegten Huaneng-Ausführung ein
**aktiver elektromagnetischer 5-V-Summer mit bis zu 30 mA**. Der Transistor
übernimmt seinen Versorgungsstrom; D8 steuert nur dessen Basis.
Der Summer wird **nicht direkt zwischen D8 und GND angeschlossen**.
[TMB12A05-Herstellerdatenblatt bei LCSC](https://datasheet.lcsc.com/lcsc/1811141116_Jiangsu-Huaneng-Elec-TMB12A05_C96093.pdf)

Benötigt: TMB12A05, NPN-Transistor **BC337**, **1 kΩ**, **10 kΩ**, Diode
**1N4148** sowie der Ein/Aus-Schalter. Verdrahtung bei abgezogenem USB-Kabel.

| Verbindung | Anschluss |
| --- | --- |
| Junker-Taste | Unverändert zwischen **D2 und GND** |
| Summer-Freigabeschalter | Zwischen **D4 und GND**, bei drei Kontakten **COM und NO** |
| Summer **+** | Nano **5V**, nicht VIN |
| Summer **−** | **Kollektor (C)** des BC337 |
| BC337 **Emitter (E)** | Nano **GND** |
| BC337 **Basis (B)** | Über **1 kΩ** an Nano **D8** |
| **10 kΩ** | Zwischen Basis (B) und Emitter/GND |
| Schutzdiode | Parallel zum Summer: **Kathode / Ring an +5V**, Anode an Summer − / Kollektor |

```text
Nano 5V --------+-------- Summer (+)
                |        Summer (-) -----+------ C  BC337
                |                        |
                +---- K [1N4148] A -------+
                      Ring

Nano D8 ----[1 kΩ]----+-------------------------- B  BC337
                     |
                   [10 kΩ]
                     |
Nano GND ------------+-------------------------- E  BC337

Nano D4 -------- Ein/Aus-Schalter -------- Nano GND
Nano D2 -------- Junker-Kontakt ---------- Nano GND
```

Alle mit B/C/E bezeichneten Anschlüsse gehören zu demselben Transistor.
Die tatsächliche Beinchenanordnung am konkreten Bauteil prüfen; die Tabelle
beschreibt die elektrische Funktion, nicht die räumliche Reihenfolge.
[BC337-Datenblatt von onsemi](https://www.onsemi.cn/download/data-sheet/pdf/bc337-fsc-d.pdf)

Der 10-kΩ-Widerstand hält den Transistor auch während Reset und Bootloader
ausgeschaltet, solange D8 noch hochohmig ist. Der 1-kΩ-Widerstand begrenzt den
Basisstrom auf wenige mA. Ein zusätzlicher **100-nF-Keramikkondensator** nahe
der Summer-Schaltstufe zwischen 5V und GND ist bei längeren Leitungen sinnvoll.
Der D4-Schalter nutzt den internen Pull-up; dafür ist kein weiterer Widerstand nötig.

## Verhalten der Firmware

- Morsetaste: unverändert **5 ms** Entprellung, LED und serielles D/U-Signal.
- Freigabeschalter: separat **20 ms** Entprellung in beide Richtungen.
- D8 liefert **HIGH für Ton an**, **LOW für Ton aus**. Keine PWM und kein `tone()`;
  der aktive Summer erzeugt seine Frequenz selbst.
- Ausschalten während eines gehaltenen Morseimpulses stoppt nach der
  Schalter-Entprellung nur den Ton; das serielle „gedrückt“ bleibt bestehen.
- Einschalten bei bereits gedrückter Morsetaste startet nach der Entprellung den Ton.
- Nach Reset ist D8 zunächst LOW. Auch ein bereits geschlossener Schalter wird
  erst nach 20 ms stabiler Freigabe wirksam.
- Keine Wartepausen in der Schleife: Schalterprellen blockiert keine Morseereignisse.
- Der Ton wird vollständig im Nano erzeugt und funktioniert mit USB-Versorgung
  auch ohne laufende Windows-App. **Pause und Fokus der Windows-App beeinflussen
  den lokalen Ton nicht.** Zum Stummschalten den Hardwareschalter öffnen.
- Das Protokoll bleibt `JUNKER/1 D` / `JUNKER/1 U`. Die bisherige Windows-EXE
  ist daher weiterhin kompatibel.

## Inbetriebnahme

1. MorseBridge und den seriellen Monitor schließen. Den aktualisierten Sketch
   `firmware/JunkerSpace/JunkerSpace.ino` auf den Nano hochladen.
2. Ohne Freigabeschalter testen: Morsetaste betätigen → LED/Space wie bisher,
   Summer bleibt aus.
3. D4 mit GND über den Schalter verbinden: Nur beim Drücken der Junker muss
   der Ton ertönen, beim Loslassen verstummen.
4. Junker gedrückt halten und den Freigabeschalter öffnen: Ton aus,
   LED/Kontaktstatus weiterhin an.

Der Aufbau ist softwareseitig getestet und für diese 5-V-Schaltstufe ausgelegt.
Der Eigentümer hat den Praxistest seines Nano-/Junker-/TMB12A05-Aufbaus mit
Transistorstufe am 03.10.2026 erfolgreich bestätigt. Jeden eigenen Aufbau
trotzdem vor der Nutzung auf richtige Verdrahtung und Funktion prüfen.

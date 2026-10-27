# Herkunft und Sicherheitshinweise

Der Quellcode, die Firmware, Buildskripte und Teile der Dokumentation dieses
Projekts wurden mit wesentlicher Unterstützung eines Large Language Models
(LLM/KI, OpenAI Codex) erstellt und überarbeitet. Dies ist keine ausschließlich
von Hand geschriebene oder unabhängig zertifizierte Implementierung.

Automatisierte Softwaretests, Compilerprüfungen und statische Analysen wurden
bzw. werden durchgeführt. Sie beweisen weder Fehlerfreiheit noch elektrische
Sicherheit. Tests mit dem tatsächlich verwendeten Kontakt-/Nano-Aufbau und dem
konkreten Windows-Zielprogramm sind gesondert erforderlich.

MorseBridge ist ein experimentelles Hobbyprojekt. Es erzeugt globale
Tastaturereignisse für die ausgewählte PC-Taste im aktiven Windows-Programm.
Fehlbedienung oder Fehler können dort unbeabsichtigte Aktionen auslösen. Nicht
für sicherheitskritische Steuerungen, medizinische Geräte, Maschinen oder andere Anwendungen einsetzen,
bei denen ein falscher oder ausbleibender Tastendruck Schäden verursachen kann.
Nicht benötigte Ausgabe pausieren oder das Programm schließen.

Am Nano nur potentialfreie Kontakte verwenden; den angeschlossenen Taster oder
Schaltkontakt vorher von anderen Geräten und externen Spannungsquellen trennen.
Bei Morsetasten gilt dies insbesondere für Sender und Funkgeräte. Der aktive
TMB12A05 benötigt die dokumentierte Transistorstufe und Schutzbeschaltung. Keine
höhere Spannung auf die Eingänge geben, um einen schlechten Kontakt zu überbrücken.

Es wird keine Zusicherung einer bestimmten Eignung oder Fehlerfreiheit gegeben.
Die Gewährleistungs- und Haftungsregelungen der GPLv3 gelten nur im gesetzlich
zulässigen Umfang. Zwingende gesetzliche Rechte und Haftungstatbestände werden
durch diese Hinweise nicht ausgeschlossen oder beschränkt.

Der LLM-Hinweis ist keine Freistellung von Rechten Dritter. Eine Herkunfts-,
Lizenz- und Rechtsprüfung wird dadurch nicht ersetzt.

Arduino® ist eine Marke von Arduino S.r.l. Weitere genannte Produkt- und
Markennamen dienen ausschließlich der Beschreibung des kompatiblen Aufbaus.
Dieses Projekt ist kein offizielles Produkt und wird nicht als von Arduino,
Junker, Microsoft oder anderen genannten Herstellern geprüft oder unterstützt
angeboten.

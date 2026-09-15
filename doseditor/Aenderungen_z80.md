# Syntax-Highlighting fuer den Z80-Simulator

Grundlage: `G:\Code\C\doseditor\doseditor\doseditor.c`, gelesen am 15.09.2026.
Die Quelldatei an diesem Ort wurde nicht geaendert. Die beiliegende `doseditor.c`
ist die vollstaendige angepasste Version zum Einsetzen in das bestehende Projekt.

## Farbschema

| Bestandteil | Farbe | Beispiele |
|---|---|---|
| Schluesselwoerter | Hellblau | static, if, return, switch |
| Typen | Hellcyan | Z80, uint16_t, GtkWidget, gpointer |
| Funktionen | Goldgelb | z80_step, printf |
| Konstanten und Makronamen | Hellrot | FLAG_Z, RAM_GROESSE, Z80_TESTS, NULL |
| Strukturfelder | Cyan | cpu->pc, cpu.f |
| Zahlen | Hellgelb | 0xFFFFu, 0b1010, 1e-3, 0x1.fp+2 |
| Strings, Zeichen und Include-Pfade | Hellgruen | "Text", 'A', <gtk/gtk.h> |
| Escape-Sequenzen | Hellmagenta | \n, \x1b, \u1234 |
| Praeprozessoranweisungen | Hellmagenta | #include, #define, #if |
| Kommentare | Grau | // und /* ... */ |
| Aufgabenhinweise in Kommentaren | Hellgelb | TODO, FIXME, BUG, HACK, XXX |
| Operatoren / Klammern | Hellweiss / Hellgrau | +=, &, -> / (), {}, [] |

Die Farben werden zentral in `syntaxfarbe_ermitteln()` eingestellt.
Es werden ANSI-Vordergrundfarben verwendet; die genaue Farbpalette stammt
aus der Windows-Konsole. Die bestehende Markierung per Invertierung bleibt erhalten.

## Verhalten

- Aktivierung weiterhin anhand der vorhandenen Dateiendungen .c, .h, .cpp, .hpp, .cc.
- Praeprozessorzeilen werden in einzelne Bestandteile zerlegt: Der Wert eines
  Makros bleibt als Zahl, String oder Ausdruck erkennbar.
- Mehrzeilige Kommentare, mit Backslash fortgesetzte Strings, Zeilenkommentare
  und Makros behalten ihren Zustand auch ausserhalb des sichtbaren Ausschnitts.
- Ein gemeinsamer Scanner berechnet sowohl die Farben als auch den Zeilenstatus.
- Keine Aenderungen an Menues, Tastatur-/Mausbedienung, Dateifunktionen oder Z80-Dateien.

Die Erkennung arbeitet lexikalisch, ohne Compiler-Symboltabelle. Grossgeschriebene
Namen gelten als Konstanten, Namen mit _t sowie Gtk-/Gdk-Typnamen als Typen.
Funktionen werden an einer folgenden Klammer in derselben Zeile erkannt;
z80_-Namen werden auch ohne diese Klammer als Funktionen eingefaerbt.
Das sind bewusst Heuristiken, keine semantische C/C++-Analyse. Insbesondere
C++-Raw-Strings und deaktivierte #if-Zweige erhalten keine Sonderbehandlung.
Es handelt sich um Highlighting fuer C-Quellcode, nicht um einen Z80-Assembler-Parser.

## Pruefung

- Gesamte Datei mit GCC, C11, -Wall -Wextra -Wpedantic -Werror -O2 gebaut.
- 34 gezielte Tokenpruefungen fuer C/Z80/GTK, Zahlen, Praeprozessor, Kommentare,
  Strings, Escape-Sequenzen und Zeilenfortsetzungen bestanden.
- 32.000 Vergleiche fuer Scannerzustand und Puffergrenzen mit wechselnden
  Eingabebytes und Eingangszustaenden bestanden.
- Kein Test im Visual-Studio-Compiler und keine interaktive Sichtpruefung
  in der Benutzerkonsole durchgefuehrt.

# Z80-Assembler-Highlighting

Die vollstaendige doseditor.c enthaelt jetzt einen eigenen Z80-Assembler-Modus.
Die vorherige C-Erweiterung bleibt fuer C-Dateien separat erhalten.
Die Originaldatei auf G: wurde nicht veraendert.

## Verwenden

Die neue doseditor.c in dein Editorprojekt uebernehmen und neu bauen.
Eine Datei mit .asm, .z80, .s oder .inc oeffnen; in der Statuszeile erscheint [Z80].
Gross-/Kleinschreibung der Endung und der Assemblerbefehle ist egal.
Die beiliegende Highlighting_Demo.asm zeigt die Farben. Sie enthaelt absichtlich
mehrere Assemblerdialekte fuer Zahlen und ist kein portables Build-Testprogramm.

## Farben

- Hellblau: Z80-Mnemonik (LD, JP, CALL, BIT, LDIR usw.; auch SLL/SL1).
- Hellcyan: Register, einschliesslich IXH/IXL, IYH/IYL und AF'.
- Hellrot: Sprungbedingungen wie NZ, Z, NC, C, PO, PE, P und M.
- Hellgruen: Labeldefinitionen sowie Strings und Zeichenliterale.
- Goldgelb: Symbolreferenzen/Sprungziele und unbekannte Makronamen.
- Hellmagenta: Direktiven wie ORG, EQU, DB, DW, DS, INCLUDE, MACRO und IF.
- Hellgelb: Zahlen sowie TODO/FIXME-Hinweise in Zeilenkommentaren.
- Grau: Kommentare ab Semikolon, alternativ // und mehrzeilige /* ... */.
- Hellweiss/Hellgrau: Operatoren und Klammern.

AF' wird als Register behandelt und startet keine Zeichenkette.
Semikolons in Strings starten keinen Kommentar. Labels mit Doppelpunkt werden
erkannt, ebenso Labels ohne Doppelpunkt vor einem bekannten Befehl/einer Direktive.
Lokale Labels wie .loop sind moeglich. Direktiven duerfen einen Punkt voranstellen.

Zahlen: dezimal, 0FFH, $FF, #FF, 0xFF, %1010, 1010B, 0b1010, 377Q/377O und 42D.
Ein einzelnes $ wird als aktueller Adresszaehler hervorgehoben.

Das Highlighting ist lexikalisch. Es assembliert nicht, prueft keine gueltigen
Operandkombinationen und kennt keine Makro-Symboltabelle. Assemblerdialekte
unterscheiden sich; unbekannte Direktiven koennen bei Bedarf ergaenzt werden.
.inc wird hier als Z80-Include angenommen, .s als Z80-Assembler.

Die Wortlisten stehen in z80_befehle, z80_direktiven, z80_register und
z80_bedingungen. Die Farben werden in syntaxfarbe_ermitteln eingestellt.

## Pruefung

Gesamtdatei mit GCC C11, -Wall -Wextra -Wpedantic -Werror -O2 gebaut.
105 Tokenpruefungen, sechs Dateityp-Pruefungen und 10.000 Scannervergleiche
fuer Zustandskonsistenz und Puffergrenzen bestanden.
Keine interaktive Sichtpruefung oder Visual-Studio-Kompilierung durchgefuehrt.

Befehlsreferenz: https://www.zilog.com/docs/z80/z80cpu_um.pdf

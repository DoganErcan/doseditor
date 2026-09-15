# Funktionsnamen: Alt → Neu

84 Funktionen umbenannt; `main` bleibt als vorgeschriebener C-Programmeinstieg erhalten. Die 70 nummerierten Abschnitte und die Funktionsreihenfolge bleiben erhalten. Zusätzliche Hilfsfunktionen sind vollständig erfasst. Die Überschrift von Abschnitt 51 wurde an die dort tatsächlich definierte Funktion angepasst.

| Altname | Neuname |
|---|---|
| `line_delete_char` | `zeile_zeichen_loeschen` |
| `remove_line` | `zeile_entfernen` |
| `ed_sel_normalize` | `markierung_normalisieren` |
| `ed_pos_in_sel` | `position_in_markierung` |
| `ed_search_range` | `bereich_durchsuchen` |
| `ed_detect_filetype` | `dateityp_erkennen` |
| `lines_free` | `zeilen_freigeben` |
| `undo_free` | `verlaufszustand_freigeben` |
| `undo_clear` | `verlaufsstapel_leeren` |
| `ed_clear_buffer` | `editor_puffer_leeren` |
| `term_restore` | `terminal_wiederherstellen` |
| `die` | `mit_fehler_beenden` |
| `xrealloc` | `speicher_sicher_neu_reservieren` |
| `undo_push` | `verlaufszustand_ablegen` |
| `ed_save_undo` | `rueckgaengig_zustand_sichern` |
| `line_ensure` | `zeile_speicher_sicherstellen` |
| `line_insert_char` | `zeile_zeichen_einfuegen` |
| `line_append` | `zeile_text_anhaengen` |
| `lines_ensure` | `zeilen_speicher_sicherstellen` |
| `insert_line` | `zeile_einfuegen` |
| `ed_insert_char` | `zeichen_einfuegen` |
| `ed_insert_newline` | `zeilenumbruch_einfuegen` |
| `ed_delete_char` | `zeichen_links_loeschen` |
| `ed_delete_selection` | `markierung_loeschen` |
| `term_get_size` | `terminal_groesse_ermitteln` |
| `term_enable_vt` | `terminal_steuerung_aktivieren` |
| `ab_append` | `ausgabepuffer_daten_anhaengen` |
| `ab_puts` | `ausgabepuffer_text_anhaengen` |
| `cx_to_rx` | `zeichenindex_zu_bildschirmspalte` |
| `is_separator` | `ist_trennzeichen` |
| `word_matches` | `wort_in_liste` |
| `hl_color` | `syntaxfarbe_ermitteln` |
| `ed_compute_line_hl` | `zeile_syntaxhervorhebung_berechnen` |
| `ed_update_syntax_state` | `syntaxzustand_aktualisieren` |
| `vlines_ensure` | `bildschirmzeilen_speicher_sicherstellen` |
| `ed_update_vlines` | `bildschirmzeilen_aktualisieren` |
| `ed_find_vline` | `bildschirmzeile_finden` |
| `vline_cx_to_rx` | `segment_zeichenindex_zu_bildschirmspalte` |
| `vline_rx_to_cx` | `segment_bildschirmspalte_zu_zeichenindex` |
| `ed_scroll` | `bildausschnitt_anpassen` |
| `ed_draw_rows` | `textzeilen_zeichnen` |
| `ed_draw_status` | `statuszeile_zeichnen` |
| `ed_draw_message` | `meldungszeile_zeichnen` |
| `ed_draw_permanent_menu_bar` | `feste_menueleiste_zeichnen` |
| `ed_refresh` | `anzeige_aktualisieren` |
| `ed_set_status` | `statusmeldung_setzen` |
| `ed_restore_history` | `verlaufszustand_wiederherstellen` |
| `ed_perform_undo` | `rueckgaengig_machen` |
| `ed_copy_line` | `aktuelle_zeile_kopieren` |
| `ed_cut_line` | `aktuelle_zeile_ausschneiden` |
| `ed_paste` | `zwischenablage_einfuegen` |
| `ed_duplicate_line` | `aktuelle_zeile_duplizieren` |
| `ed_delete_line` | `aktuelle_zeile_loeschen` |
| `ed_copy_selection` | `markierung_kopieren` |
| `ed_cut_selection` | `markierung_ausschneiden` |
| `ed_open` | `datei_oeffnen` |
| `ed_save_to` | `datei_an_pfad_speichern` |
| `read_key` | `eingabe_lesen` |
| `ed_prompt_ex` | `eingabedialog_erweitert` |
| `ed_prompt` | `eingabedialog` |
| `ed_save_as` | `datei_speichern_unter` |
| `ed_save` | `datei_speichern` |
| `ed_confirm_discard` | `verwerfen_bestaetigen` |
| `ed_open_prompt` | `datei_oeffnen_dialog` |
| `ed_new` | `dokument_neu_anlegen` |
| `ed_select_all` | `alles_markieren` |
| `ed_search` | `text_suchen` |
| `ed_search_replace` | `text_suchen_ersetzen` |
| `ed_goto_line` | `zu_zeile_springen` |
| `ed_move` | `cursor_bewegen` |
| `ed_handle_mouse_click` | `mausklick_verarbeiten` |
| `is_shift_key` | `ist_markierungstaste` |
| `is_nav_key` | `ist_navigationstaste` |
| `menu_write_at` | `menue_text_an_position_schreiben` |
| `menu_write_at_inverse` | `menue_text_an_position_invers_schreiben` |
| `menu_draw_box` | `menue_rahmen_zeichnen` |
| `menu_draw_box_inverse` | `menue_rahmen_invers_zeichnen` |
| `ed_show_message_box` | `meldungsfenster_anzeigen` |
| `ed_menu_draw` | `menue_zeichnen` |
| `ed_menu_action` | `menue_aktion_ausfuehren` |
| `menu_main_at` | `hauptmenue_an_position` |
| `ed_open_menu` | `menue_oeffnen` |
| `ed_handle_key` | `taste_verarbeiten` |
| `ed_process_key` | `benutzereingabe_verarbeiten` |
| `main` | `main` |

## Prüfung

- Keine alten Funktionsbezeichner im Code verblieben, einschließlich Funktionszeiger (z. B. bei `atexit`).
- Keine doppelten neuen Namen oder Kollisionen mit bestehenden Codebezeichnern.
- Code nach Rückübersetzung der Namen außerhalb von Kommentaren exakt identisch. API-Aufrufe, Typen, Makros, Variablen und Zeichenketten unverändert.
- UTF-8-Kodierung, CRLF-Zeilenenden und ursprüngliche Formatierung erhalten. Originaldatei unverändert.
- GCC-Syntaxprüfung: erfolgreich, ohne Warnungen.
- Windows-Kompilierung und Linken: erfolgreich, ohne Warnungen.
- Kein interaktiver Laufzeittest des Editors durchgeführt.

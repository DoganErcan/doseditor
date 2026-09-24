/* ================================================================
 * doseditor (Komplett & Erweitert mit Maus-Support)
 * ================================================================
 * Datum: 15.09.2026 - 17:00 Uhr
 * Ein einfacher Konsolen-Texteditor für Windows.
 * ------------------------------------------------
 * Mausbedienung (NEU!)
 * ------------------------------------------------
 * Linksklick          Cursor setzen
 * Linksklick+Ziehen   Text markieren
 * Mausrad             Scrollen (3 Zeilen pro Raster)
 * ------------------------------------------------
 * Tastenkombinationen (Standard & Erweitert)
 * ------------------------------------------------
 * Pfeiltasten     Cursor bewegen
 * Pos1 / Ende     Anfang / Ende der Zeile
 * Bild auf/ab     Seitenweise scrollen
 * Entf            Zeichen rechts löschen
 * Backspace       Zeichen links löschen
 * Enter           Neue Zeile
 *
 * Shift+Pfeiltasten   Blockmarkierung erweitern
 * Shift+Home/End      Bis Zeilenanfang/-ende markieren
 * Shift+Bild auf/ab   Seitenweise markieren
 *
 * Strg+O          Datei öffnen
 * Strg+S          Speichern
 * Strg+A          Speichern unter
 * Strg+Q          Beenden
 *
 * -- DYNAMISCHES MENÜ & ERWEITERUNGEN --
 * F10             Oeffnet das grafische Menue!
 * STRG halten     Blendet alle verfügbaren Shortcuts unten ein!
 * ESC             Bricht eine laufende Funktion (Suche/Öffnen...) ab
 *
 * Strg+C          Kopieren (Markierung oder ganze Zeile)
 * Strg+X          Ausschneiden (Markierung oder ganze Zeile)
 * Strg+V          Einfügen aus Zwischenablage
 * Strg+D          Aktuelle Zeile duplizieren
 * Strg+K          Aktuelle Zeile löschen
 * Strg+F          Suchen (vorwärts)
 * Strg+N          Nächsten Suchtreffer anspringen
 * Strg+H          Suchen & Ersetzen
 * Strg+G          Zu Zeilennummer springen
 * Strg+Z          Rueckgaengig (bis zu 32 Schritte)
 * Strg+Y          Wiederholen
 * Strg+Umschalt+A Alles markieren
 * Strg+Umschalt+S Speichern unter (auch weiterhin Strg+A)
 * Strg+Umschalt+N Neues Dokument
 * F1 / F3         Kurzhilfe / naechster Suchtreffer
 * Strg+W          Zeilenumbruch ein/aus (Word-Wrap)
 * Strg+Pos1       Zum Dokumentanfang springen
 * Strg+Ende       Zum Dokumentende springen
 * ------------------------------------------------
 * Syntax-Highlighting: Automatisch für .c/.h/.cpp/.hpp
 * ================================================================ */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#define WIN32_LEAN_AND_MEAN
#define TAB_STOP 4  /* Tabulatorbreite in Spalten */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdint.h>
#include <errno.h>

#define UNDO_LIMIT 32
#define UNDO_BYTES ( 64u * 1024u * 1024u )


 /* ================================================================
  * Syntax-Highlighting Definitionen
  * ================================================================ */

enum
{
	HL_NORMAL = 0,
	HL_KEYWORD,     /* Sprachschlüsselwörter (blau) */
	HL_TYPE,        /* Datentypen (cyan) */
	HL_STRING,      /* Zeichenketten "..." und '.' (grün) */
	HL_COMMENT,     /* Einzeilige Kommentare // (grau) */
	HL_MLCOMMENT,   /* Mehrzeilige Kommentare (grau) */
	HL_NUMBER,      /* Zahlen (gelb) */
	HL_LABEL, HL_CONDITION, HL_FUNCTION, HL_CONSTANT, HL_MEMBER, HL_OPERATOR, HL_BRACKET, HL_ESCAPE, HL_TODO,
	HL_PREPROC      /* Präprozessor #include etc. (magenta) */
};

static const char* c_keywords [ ] = {
	"auto", "break", "case", "const", "continue", "default", "do",
	"else", "enum", "extern", "for", "goto", "if", "inline",
	"register", "restrict", "return", "sizeof", "static", "struct",
	"switch", "typedef", "union", "volatile", "while",
	"_Alignas", "_Alignof", "_Atomic", "_Generic", "_Noreturn", "_Static_assert",
	"_Thread_local", "alignas", "alignof", "constexpr", "static_assert",
	"class", "namespace", "public", "private", "protected", "template", "typename",
	"using", "new", "delete", "try", "catch", "throw", "virtual", "override", "noexcept", NULL
};

static const char* c_types [ ] = {
	"Z80", "bool", "_Bool", "_Complex", "gboolean", "gchar", "guchar",
	"gint", "guint", "glong", "gulong", "gsize", "gssize", "gpointer", "gconstpointer",
	"gfloat", "gdouble", "gint8", "guint8", "gint16", "guint16", "gint32", "guint32",
	"gint64", "guint64", "GObject", "GError", "GApplication", "GFile", "GList", "GSList",
	"char", "double", "float", "int", "long", "short", "signed",
	"unsigned", "void", "size_t", "ssize_t", "ptrdiff_t",
	"uint8_t", "uint16_t", "uint32_t", "uint64_t",
	"int8_t", "int16_t", "int32_t", "int64_t",
	"BOOL", "BYTE", "CHAR", "DWORD", "HANDLE", "HINSTANCE",
	"HMODULE", "HWND", "INT", "LONG", "LPARAM", "LPCSTR",
	"LPCTSTR", "LPDWORD", "LPSTR", "LPVOID", "LRESULT",
	"UINT", "ULONG", "VOID", "WCHAR", "WORD", "WPARAM",
	"FILE", "COORD", "SMALL_RECT", "CONSOLE_SCREEN_BUFFER_INFO",
	"INPUT_RECORD", "KEY_EVENT_RECORD", "MOUSE_EVENT_RECORD", NULL
};

/* ================================================================
 * Datenstrukturen
 * ================================================================ */

typedef struct
{
	char* chars;
	size_t len;
	size_t cap;
	int eol;              /* Zeilenende: 1=LF, 2=CRLF, 3=CR */
	int hl_open_comment;  /* Syntax-Fortsetzungszustand: Kommentar, String, Makro. */
} Line;

/* Virtuelle Zeile für Word-Wrap: bildet ein visuelles Segment ab */
typedef struct
{
	size_t logical_row;   /* Logische Zeile in E.lines */
	size_t char_start;    /* Start-Zeichenindex */
	size_t char_len;      /* Zeichenanzahl in diesem Segment */
} VLine;

/* Vollstaendige Textzustaende; Anzahl und Gesamtspeicher sind begrenzt. */
typedef struct
{
	Line* lines;
	size_t nlines;
	size_t cx, cy;
	size_t cap, bytes;
	uint64_t revision;
} UndoState;

/* Zentraler Zustand des Editors. */
typedef struct
{
	Line* lines;
	size_t nlines;
	size_t cap;

	size_t cx, cy;
	size_t rowoff, coloff;
	size_t goal_rx;       /* Gewuenschte Bildschirmspalte bei vertikaler Bewegung */
	int goal_valid;
	int screen_rows;
	int screen_cols;
	int gutter_width;  /* Breite des Zeilennummern-Gutter */

	char filename [ MAX_PATH ];
	int dirty;

	char status [ 256 ];
	ULONGLONG status_time;
	DWORD status_duration;  /* Anzeigedauer der Statusmeldung in ms */
	int ctrl_pressed;  /* Hält fest, ob STRG gerade gedrückt ist */

	/* Interne Zwischenablage */
	char* clipboard;
	size_t clip_len;
	int clip_is_line;  /* 1=Ganze Zeile, 0=Textfragment */

	/* Suchpuffer */
	char last_search [ 128 ];
	int last_match_row;
	int last_match_col;

	/* Syntax-Highlighting */
	int syntax;  /* 0=aus, 1=C, 2=Z80-Assembler */

	/* Blockmarkierung */
	int sel_active;
	size_t sel_ax, sel_ay;  /* Anker-Position der Markierung */

	/* Prompt-Modus (Cursor in Eingabezeile) */
	int in_prompt;
	int prompt_cursor_col;  /* 1-indizierte Spalte in der Message-Bar */

	/* Undo-Speicher */
	UndoState undo [ UNDO_LIMIT ], redo [ UNDO_LIMIT ];
	size_t undo_count, redo_count;
	int undo_group; /* 0=einzeln, 1=Gruppe beginnt, 2=bereits gesichert */
	uint64_t revision, next_revision, saved_revision;
	uint64_t syntax_revision, wrap_revision;
	int cached_syntax, cached_cols, cached_wrap;
	int eol;
	int quit_pending;

	/* Word-Wrap */
	int word_wrap;          /* 0=aus, 1=ein */
	VLine* vlines;          /* Array der virtuellen Zeilen */
	size_t nvlines;
	size_t vlines_cap;

	/* Maus-Positionsspeicher */
	int mouse_x;
	int mouse_y;
} Editor;

static Editor E;
static HANDLE hIn, hOut;
static DWORD origInMode, origOutMode;
static UINT origOutCP;

/* ================================================================
 * Gemeinsame Typen und Konstanten
 * ================================================================
 * Keine Funktions-Prototypen:
 * Hier stehen nur Typen, Konstanten und Menue-Daten, mit_fehler_beenden von mehreren
 * Funktionen benoetigt werden.
 * ================================================================ */

typedef struct { char* b; size_t len; size_t cap; } Abuf;

enum
{
	KEY_F10 = 900,
	KEY_HELP, KEY_REDO, KEY_SELECT_ALL, KEY_SAVE_AS, KEY_NEW,
	KEY_NONE = 0,
	KEY_FORCED_REFRESH,
	KEY_ESC,
	KEY_UP = 1000,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_HOME,
	KEY_END,
	KEY_PGUP,
	KEY_PGDN,
	KEY_DEL,
	KEY_BACKSPACE,
	KEY_ENTER,
	KEY_CTRL_S,
	KEY_CTRL_Q,
	KEY_CTRL_O,
	KEY_CTRL_A,
	KEY_CTRL_C,
	KEY_CTRL_X,
	KEY_CTRL_V,
	KEY_CTRL_D,
	KEY_CTRL_K,
	KEY_CTRL_F,
	KEY_CTRL_H,
	KEY_CTRL_N,
	KEY_CTRL_G,
	KEY_CTRL_Z,
	KEY_CTRL_W,
	KEY_CTRL_HOME,
	KEY_CTRL_END,

	/* Maus Events */
	KEY_MOUSE_CLICK,
	KEY_MOUSE_DRAG,
	KEY_MOUSE_WHEEL_UP,
	KEY_MOUSE_WHEEL_DOWN,

	/* Shift-Varianten für Blockmarkierung */
	KEY_SHIFT_UP,
	KEY_SHIFT_DOWN,
	KEY_SHIFT_LEFT,
	KEY_SHIFT_RIGHT,
	KEY_SHIFT_HOME,
	KEY_SHIFT_END,
	KEY_SHIFT_PGUP,
	KEY_SHIFT_PGDN
};

/* ================================================================
 * Menue-Daten
 * ================================================================ */

static const char* menu_main [ ] = { "Datei", "Bearbeiten", "Suchen", "Hilfe" };
enum { menu_main_count = sizeof menu_main / sizeof menu_main [ 0 ] };
static const char* menu_datei [ ] = { "Neu", "Oeffnen", "Speichern", "Speichern unter", "Beenden" };
static const char* menu_bearbeiten [ ] = { "Rueckgaengig", "Kopieren", "Ausschneiden", "Einfuegen","Zeile duplizieren", "Zeile loeschen", "Word-Wrap", "Wiederholen", "Alles markieren" };
static const char* menu_suchen [ ] = { "Suchen...", "Naechster Treffer", "Ersetzen...", "Gehe zu Zeile..." };
static const char* menu_hilfe [ ] = { "Tastenkombinationen", "Ueber DOS-Edit" };
static const char** menu_sub [ ] = { menu_datei, menu_bearbeiten, menu_suchen, menu_hilfe };
static const int menu_sub_count [ ] = { 5, 9, 4, 2 };

/* ================================================================
 * FUNKTIONEN – KOMPILERFREUNDLICH VON UNTEN NACH OBEN
 * ================================================================
 * Jede Funktion steht NACH allen Funktionen, mit_fehler_beenden sie selbst aufruft.
 * Dadurch ist kein zentraler Forward-Deklarationsblock notwendig.
 * main() steht als oberste Programmebene ganz am Ende.
 * ================================================================ */

 /* ---------------------------------------------------------------
  * 01. zeile_zeichen_loeschen()
  * --------------------------------------------------------------- */
static void zeile_zeichen_loeschen ( Line* l, size_t at )
{
	if ( at >= l->len ) return;
	memmove ( l->chars + at, l->chars + at + 1, l->len - at - 1 );
	l->len--;
}

/* ---------------------------------------------------------------
 * 02. zeile_entfernen()
 * --------------------------------------------------------------- */
static void zeile_entfernen ( size_t at )
{
	if ( at >= E.nlines ) return;
	free ( E.lines [ at ].chars );
	memmove ( &E.lines [ at ], &E.lines [ at + 1 ], ( E.nlines - at - 1 ) * sizeof ( Line ) );
	E.nlines--;
	memset ( &E.lines [ E.nlines ], 0, sizeof ( Line ) );
}

/* ---------------------------------------------------------------
 * 03. markierung_normalisieren()
 * --------------------------------------------------------------- */
static void markierung_normalisieren ( size_t* sr, size_t* sc, size_t* er, size_t* ec )
{
	if ( E.sel_ay < E.cy || ( E.sel_ay == E.cy && E.sel_ax <= E.cx ) )
	{
		*sr = E.sel_ay; *sc = E.sel_ax;
		*er = E.cy;     *ec = E.cx;
	} else
	{
		*sr = E.cy;     *sc = E.cx;
		*er = E.sel_ay; *ec = E.sel_ax;
	}
}

/* ---------------------------------------------------------------
 * 04. position_in_markierung()
 * --------------------------------------------------------------- */
static int position_in_markierung ( size_t row, size_t col )
{
	if ( !E.sel_active ) return 0;
	size_t sr, sc, er, ec;
	markierung_normalisieren ( &sr, &sc, &er, &ec );
	if ( sr == er && sc == ec ) return 0;
	if ( row < sr || row > er ) return 0;
	if ( row == sr && row == er ) return ( col >= sc && col < ec );
	if ( row == sr ) return col >= sc;
	if ( row == er ) return col < ec;
	return 1;
}

/* ---------------------------------------------------------------
 * 05. bereich_durchsuchen()
 * --------------------------------------------------------------- */
static int bereich_durchsuchen ( size_t start_row, size_t start_col, size_t end_row )
{
	size_t len = strlen ( E.last_search );
	if ( !len ) return 0;
	for ( size_t i = start_row; i <= end_row && i < E.nlines; i++ )
	{
		Line* l = &E.lines [ i ];
		if ( len <= l->len )
		{
			for ( size_t j = start_col; j <= l->len - len; j++ )
			{
				if ( memcmp ( l->chars + j, E.last_search, len ) != 0 ) continue;
				E.cy = i; E.cx = j;
				E.last_match_row = ( int ) i; E.last_match_col = ( int ) j;
				E.sel_active = 0;
				return 1;
			}
		}
		start_col = 0;
	}
	return 0;
}

/* ---------------------------------------------------------------
 * 06. dateityp_erkennen()
 * --------------------------------------------------------------- */
static void dateityp_erkennen ( void )
{
	E.syntax = 0;
	if ( E.filename [ 0 ] == '\0' ) return;
	const char* dot = strrchr ( E.filename, '.' );
	if ( !dot ) return;
	if ( _stricmp ( dot, ".asm" ) == 0 || _stricmp ( dot, ".z80" ) == 0 ||
		 _stricmp ( dot, ".s" ) == 0 || _stricmp ( dot, ".inc" ) == 0 )
	{
		E.syntax = 2;
		return;
	}
	if ( _stricmp ( dot, ".c" ) == 0 || _stricmp ( dot, ".h" ) == 0 ||
		 _stricmp ( dot, ".cpp" ) == 0 || _stricmp ( dot, ".hpp" ) == 0 ||
		 _stricmp ( dot, ".cc" ) == 0 )
	{
		E.syntax = 1;
	}
}

/* ---------------------------------------------------------------
 * 07. editor_puffer_leeren()
 * --------------------------------------------------------------- */
static void zeilen_freigeben ( _In_reads_ ( count ) Line* lines, size_t count )
{
	for ( size_t i = 0; i < count; i++ ) free ( lines [ i ].chars );
	free ( lines );
}

static void verlaufszustand_freigeben ( UndoState* state )
{
	zeilen_freigeben ( state->lines, state->nlines );
	memset ( state, 0, sizeof * state );
}

static void verlaufsstapel_leeren ( UndoState* stack, size_t* count )
{
	while ( *count ) verlaufszustand_freigeben ( &stack [ --*count ] );
}

static void editor_puffer_leeren ( void )
{
	zeilen_freigeben ( E.lines, E.nlines );
	E.lines = NULL; E.nlines = 0; E.cap = 0;
	free ( E.vlines ); E.vlines = NULL; E.nvlines = 0; E.vlines_cap = 0;
	E.cx = 0; E.cy = 0; E.rowoff = 0; E.coloff = 0; E.dirty = 0;
	verlaufsstapel_leeren ( E.undo, &E.undo_count );
	verlaufsstapel_leeren ( E.redo, &E.redo_count );
	E.sel_active = 0; E.quit_pending = 0; E.syntax = 0;
	E.goal_valid = 0;
	E.last_search [ 0 ] = '\0'; E.last_match_row = E.last_match_col = -1;
	E.revision = ++E.next_revision; E.saved_revision = E.revision;
	E.syntax_revision = E.wrap_revision = UINT64_MAX;
	E.eol = 2;
}

/* ---------------------------------------------------------------
 * 08. terminal_wiederherstellen()
 * --------------------------------------------------------------- */
static void terminal_wiederherstellen ( void )
{
	SetConsoleMode ( hIn, origInMode );
	DWORD n;
	const char* s = "\x1b[?25h\x1b[?1049l";
	WriteConsoleA ( hOut, s, ( DWORD ) strlen ( s ), &n, NULL );
	SetConsoleMode ( hOut, origOutMode );
	if ( origOutCP ) SetConsoleOutputCP ( origOutCP );
}

/* ---------------------------------------------------------------
 * 09. mit_fehler_beenden()
 * --------------------------------------------------------------- */
static void mit_fehler_beenden ( const char* msg )
{
	terminal_wiederherstellen ( );  /* Terminal wiederherstellen, bevor Fehler ausgegeben wird */
	fprintf ( stderr, "Fehler: %s (%lu)\n", msg, GetLastError ( ) );
	ExitProcess ( 1 );
}

/* ---------------------------------------------------------------
 * 10. speicher_sicher_neu_reservieren()
 * --------------------------------------------------------------- */
static void* speicher_sicher_neu_reservieren ( void* p, size_t n )
{
	void* r = realloc ( p, n ? n : 1 );
	if ( !r ) mit_fehler_beenden ( "Speicherfehler" );
	return r;
}

/* ---------------------------------------------------------------
 * 11. rueckgaengig_zustand_sichern()
 * --------------------------------------------------------------- */
 /* Aelteste Zustaende freigeben; ein einzelner grosser Zustand bleibt nutzbar. */
static void verlaufszustand_ablegen ( UndoState* stack, size_t* count, UndoState state )
{
	size_t bytes = state.bytes;
	for ( size_t i = 0; i < *count; i++ ) bytes += stack [ i ].bytes;
	while ( *count && ( *count == UNDO_LIMIT || bytes > UNDO_BYTES ) )
	{
		bytes -= stack [ 0 ].bytes;
		verlaufszustand_freigeben ( &stack [ 0 ] );
		memmove ( stack, stack + 1, ( --*count ) * sizeof * stack );
	}
	stack [ ( *count )++ ] = state;
}

static void rueckgaengig_zustand_sichern ( void )
{
	if ( E.undo_group == 2 ) return;
	UndoState state = { 0 };
	state.nlines = state.cap = E.nlines;
	state.cx = E.cx; state.cy = E.cy; state.revision = E.revision;
	state.bytes = E.nlines * sizeof ( Line );
	state.lines = calloc ( state.nlines ? state.nlines : 1, sizeof ( Line ) );

	if ( !state.lines ) mit_fehler_beenden ( "Undo-Speicher" );
	for ( size_t i = 0; i < state.nlines; i++ )
	{
		state.lines [ i ] = E.lines [ i ];
		state.lines [ i ].cap = E.lines [ i ].len;
		state.lines [ i ].chars = speicher_sicher_neu_reservieren ( NULL, E.lines [ i ].len );
		if ( E.lines [ i ].len ) memcpy ( state.lines [ i ].chars, E.lines [ i ].chars, E.lines [ i ].len );
		state.bytes += E.lines [ i ].len;
	}

	verlaufszustand_ablegen ( E.undo, &E.undo_count, state );
	verlaufsstapel_leeren ( E.redo, &E.redo_count );
	E.revision = ++E.next_revision;
	E.goal_valid = 0;
	if ( E.undo_group ) E.undo_group = 2;
}

/* ---------------------------------------------------------------
 * 12. zeile_speicher_sicherstellen()
 * --------------------------------------------------------------- */
static void zeile_speicher_sicherstellen ( Line* l, size_t need )
{
	if ( l->cap >= need ) return;
	size_t c = l->cap ? l->cap : 16;
	while ( c < need )
	{
		if ( c > SIZE_MAX / 2 ) { c = need; break; }
		c *= 2;
	}
	l->chars = speicher_sicher_neu_reservieren ( l->chars, c );
	l->cap = c;
}

/* ---------------------------------------------------------------
 * 13. zeile_zeichen_einfuegen()
 * --------------------------------------------------------------- */
static void zeile_zeichen_einfuegen ( Line* l, size_t at, char c )
{
	if ( at > l->len ) at = l->len;
	zeile_speicher_sicherstellen ( l, l->len + 1 );
	memmove ( l->chars + at + 1, l->chars + at, l->len - at );
	l->chars [ at ] = c;
	l->len++;
}

/* ---------------------------------------------------------------
 * 14. zeile_text_anhaengen()
 * --------------------------------------------------------------- */
static void zeile_text_anhaengen ( Line* l, const char* s, size_t n )
{
	if ( !n ) return;
	if ( n > SIZE_MAX - l->len ) mit_fehler_beenden ( "Zeile ist zu lang" );
	zeile_speicher_sicherstellen ( l, l->len + n );
	memcpy ( l->chars + l->len, s, n );
	l->len += n;
}

/* ---------------------------------------------------------------
 * 15. zeilen_speicher_sicherstellen()
 * --------------------------------------------------------------- */
static void zeilen_speicher_sicherstellen ( size_t need )
{
	if ( E.cap >= need ) return;
	if ( need > SIZE_MAX / sizeof ( Line ) ) mit_fehler_beenden ( "Zu viele Zeilen" );
	size_t c = E.cap ? E.cap : 64;
	while ( c < need )
	{
		if ( c > SIZE_MAX / sizeof ( Line ) / 2 ) { c = need; break; }
		c *= 2;
	}
	E.lines = speicher_sicher_neu_reservieren ( E.lines, c * sizeof ( Line ) );
	memset ( E.lines + E.cap, 0, ( c - E.cap ) * sizeof ( Line ) );
	E.cap = c;
}

/* ---------------------------------------------------------------
 * 16. zeile_einfuegen()
 * --------------------------------------------------------------- */
static void zeile_einfuegen ( size_t at, const char* s, size_t n )
{
	if ( at > E.nlines ) at = E.nlines;
	zeilen_speicher_sicherstellen ( E.nlines + 1 );
	memmove ( &E.lines [ at + 1 ], &E.lines [ at ], ( E.nlines - at ) * sizeof ( Line ) );
	memset ( &E.lines [ at ], 0, sizeof ( Line ) );
	E.lines [ at ].eol = E.eol ? E.eol : 2;

	if ( n )
	{
		zeile_speicher_sicherstellen ( &E.lines [ at ], n );
		memcpy ( E.lines [ at ].chars, s, n );
		E.lines [ at ].len = n;
	}
	E.nlines++;
}

/* ---------------------------------------------------------------
 * 17. zeichen_einfuegen()
 * --------------------------------------------------------------- */
static void zeichen_einfuegen ( char c )
{
	rueckgaengig_zustand_sichern ( );
	if ( E.cy == E.nlines ) zeile_einfuegen ( E.nlines, "", 0 );
	zeile_zeichen_einfuegen ( &E.lines [ E.cy ], E.cx, c );
	E.cx++;
	E.dirty = 1;
}

/* ---------------------------------------------------------------
 * 18. zeilenumbruch_einfuegen()
 * --------------------------------------------------------------- */
static void zeilenumbruch_einfuegen ( void )
{
	rueckgaengig_zustand_sichern ( );
	if ( E.cy == E.nlines ) zeile_einfuegen ( E.nlines, "", 0 );
	Line* cur = &E.lines [ E.cy ];

	/* Auto-Indent: Einrückung der aktuellen Zeile ermitteln */
	size_t indent = 0;
	int old_eol = cur->eol;
	while ( indent < E.cx && indent < cur->len && ( cur->chars [ indent ] == ' ' || cur->chars [ indent ] == '\t' ) )
	{
		indent++;
	}

	if ( E.cx >= cur->len ) { zeile_einfuegen ( E.cy + 1, "", 0 ); } else
	{
		zeile_einfuegen ( E.cy + 1, cur->chars + E.cx, cur->len - E.cx );
		cur = &E.lines [ E.cy ];
		cur->len = E.cx;
	}
	E.lines [ E.cy ].eol = E.eol;
	E.cy++;
	E.lines [ E.cy ].eol = old_eol;

	/* Einrückung in mit_fehler_beenden neue Zeile übernehmen */
	if ( indent > 0 )
	{
		Line* newline = &E.lines [ E.cy ];
		size_t existing_len = newline->len;
		zeile_speicher_sicherstellen ( newline, indent + existing_len );
		memmove ( newline->chars + indent, newline->chars, existing_len );
		memcpy ( newline->chars, E.lines [ E.cy - 1 ].chars, indent );
		newline->len += indent;
		E.cx = indent;
	} else
	{
		E.cx = 0;
	}
	E.dirty = 1;
}

/* ---------------------------------------------------------------
 * 19. zeichen_links_loeschen()
 * --------------------------------------------------------------- */
static void zeichen_links_loeschen ( void )
{
	if ( E.cy == E.nlines ) return;
	if ( E.cx == 0 && E.cy == 0 ) return;

	rueckgaengig_zustand_sichern ( );
	Line* cur = &E.lines [ E.cy ];

	if ( E.cx > 0 ) { zeile_zeichen_loeschen ( cur, E.cx - 1 ); E.cx--; } else
	{
		Line* prev = &E.lines [ E.cy - 1 ];
		size_t newcx = prev->len;
		zeile_text_anhaengen ( prev, cur->chars, cur->len );
		prev->eol = cur->eol;
		zeile_entfernen ( E.cy );
		E.cy--;
		E.cx = newcx;
	}
	E.dirty = 1;
}

/* ---------------------------------------------------------------
 * 20. markierung_loeschen()
 * --------------------------------------------------------------- */
static void markierung_loeschen ( void )
{
	if ( !E.sel_active ) return;
	size_t sr, sc, er, ec;
	markierung_normalisieren ( &sr, &sc, &er, &ec );

	if ( sr == er && sc == ec ) { E.sel_active = 0; return; }
	rueckgaengig_zustand_sichern ( );

	if ( sr == er )
	{
		/* Selektion innerhalb einer Zeile */
		Line* l = &E.lines [ sr ];
		if ( ec > l->len ) ec = l->len;
		if ( sc < ec )
		{
			memmove ( l->chars + sc, l->chars + ec, l->len - ec );
			l->len -= ( ec - sc );
		}
	} else
	{
		/* Mehrzeilige Selektion */
		Line* first = &E.lines [ sr ];
		Line* last = &E.lines [ er ];
		first->eol = last->eol;
		if ( ec > last->len ) ec = last->len;

		size_t remaining = ( ec < last->len ) ? last->len - ec : 0;
		first->len = sc;
		if ( remaining > 0 )
		{
			zeile_speicher_sicherstellen ( first, sc + remaining );
			memcpy ( first->chars + sc, last->chars + ec, remaining );
			first->len = sc + remaining;
		}

		/* Zeilen dazwischen + letzte Zeile entfernen (von hinten) */
		for ( size_t i = er; i > sr; i-- ) { zeile_entfernen ( i ); }
	}

	E.cy = sr;
	E.cx = sc;
	E.sel_active = 0;
	E.dirty = 1;
}

/* ---------------------------------------------------------------
 * 21. terminal_groesse_ermitteln()
 * --------------------------------------------------------------- */
static void terminal_groesse_ermitteln ( void )
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if ( !GetConsoleScreenBufferInfo ( hOut, &csbi ) ) mit_fehler_beenden ( "GetConsoleScreenBufferInfo" );

	E.screen_cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;

	/*  Die Konsole ist jetzt fest in drei Bereiche aufgeteilt:
	*   Zeile 1               = permanente Menueleiste
	*   Zeile 2 ...           = Editor
	*   vorletzte Zeile       = Statuszeile
	*   letzte Zeile          = Meldungs-/Hilfszeile
	*   E.screen_rows enthaelt deshalb NUR mit_fehler_beenden Hoehe des Editors. */

	int total_rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	if ( total_rows < 4 ) total_rows = 4;

	E.screen_rows = total_rows - 3;
	if ( E.screen_rows < 1 ) E.screen_rows = 1;
}

/* ---------------------------------------------------------------
 * 22. terminal_steuerung_aktivieren()
 * --------------------------------------------------------------- */
static void terminal_steuerung_aktivieren ( void )
{
	hIn = GetStdHandle ( STD_INPUT_HANDLE );
	hOut = GetStdHandle ( STD_OUTPUT_HANDLE );
	if ( !GetConsoleMode ( hIn, &origInMode ) || !GetConsoleMode ( hOut, &origOutMode ) ) mit_fehler_beenden ( "GetConsoleMode" );

	DWORD in = origInMode & ~( ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT );
	/* GANZ WICHTIG: ENABLE_MOUSE_INPUT für mit_fehler_beenden Maus, ~ENABLE_QUICK_EDIT_MODE um Blockieren zu verhindern! */
	in |= ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS;
	in &= ~ENABLE_QUICK_EDIT_MODE;
	if ( !SetConsoleMode ( hIn, in ) ) mit_fehler_beenden ( "Konsoleneingabe" );

	DWORD out = origOutMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
	if ( !SetConsoleMode ( hOut, out ) ) mit_fehler_beenden ( "Konsolenausgabe" );
	origOutCP = GetConsoleOutputCP ( );
	SetConsoleOutputCP ( CP_UTF8 );

	DWORD n;
	WriteConsoleA ( hOut, "\x1b[3 q", 5, &n, NULL );
}

/* ---------------------------------------------------------------
 * 23. ausgabepuffer_daten_anhaengen()
 * --------------------------------------------------------------- */
static void ausgabepuffer_daten_anhaengen ( Abuf* a, const char* s, size_t n )
{
	if ( !n ) return;
	if ( n > SIZE_MAX - a->len ) mit_fehler_beenden ( "Ausgabepuffer ist zu gross" );
	if ( a->len + n > a->cap )
	{
		size_t c = a->cap ? a->cap : 4096;
		while ( c < a->len + n )
		{
			if ( c > SIZE_MAX / 2 ) { c = a->len + n; break; }
			c *= 2;
		}
		a->b = speicher_sicher_neu_reservieren ( a->b, c ); a->cap = c;
	}
	memcpy ( a->b + a->len, s, n ); a->len += n;
}

/* ---------------------------------------------------------------
 * ausgabepuffer_text_anhaengen() - Text an den Ausgabepuffer anhaengen
 * --------------------------------------------------------------- */
static void ausgabepuffer_text_anhaengen ( Abuf* a, const char* s ) { ausgabepuffer_daten_anhaengen ( a, s, strlen ( s ) ); }

/* ---------------------------------------------------------------
 * 24. zeichenindex_zu_bildschirmspalte()
 * --------------------------------------------------------------- */
static size_t zeichenindex_zu_bildschirmspalte ( Line* l, size_t cx )
{
	size_t rx = 0;
	for ( size_t j = 0; j < cx && j < l->len; j++ )
	{
		if ( l->chars [ j ] == '\t' )			rx += TAB_STOP - ( rx % TAB_STOP );	else rx++;
	}
	return rx;
}

/* ---------------------------------------------------------------
 * 25. ist_trennzeichen()
 * --------------------------------------------------------------- */
 /* ---------------------------------------------------------------
  * 26. wort_in_liste()
  * --------------------------------------------------------------- */
static int wort_in_liste ( const char* word, size_t wlen, const char** list )
{
	for ( int i = 0; list [ i ]; i++ )
	{
		size_t klen = strlen ( list [ i ] );
		if ( klen == wlen && memcmp ( word, list [ i ], wlen ) == 0 ) return 1;
	}
	return 0;
}

/* ---------------------------------------------------------------
 * 27. syntaxfarbe_ermitteln()
 * --------------------------------------------------------------- */
static const char* syntaxfarbe_ermitteln ( int hl )
{
	/* Helle ANSI-Farben auf dunklem Grund; Auswahl-Invertierung bleibt erhalten. */
	switch ( hl )
	{
	case HL_KEYWORD: return "\x1b[94m";
	case HL_TYPE: return "\x1b[96m";
	case HL_STRING: return "\x1b[92m";
	case HL_COMMENT:
	case HL_MLCOMMENT: return "\x1b[90m";
	case HL_NUMBER: return "\x1b[93m";
	case HL_PREPROC: return "\x1b[95m";
	case HL_LABEL: return "\x1b[92m";
	case HL_CONDITION: return "\x1b[91m";
	case HL_FUNCTION: return "\x1b[33m";
	case HL_CONSTANT: return "\x1b[91m";
	case HL_MEMBER: return "\x1b[36m";
	case HL_OPERATOR: return "\x1b[97m";
	case HL_BRACKET: return "\x1b[37m";
	case HL_ESCAPE: return "\x1b[95m";
	case HL_TODO: return "\x1b[93m";
	default: return "\x1b[39m";
	}
}

/* ---------------------------------------------------------------
 * 28. zeile_syntaxhervorhebung_berechnen()
 * --------------------------------------------------------------- */
 /* Gemeinsamer Scanner fuer Anzeige und Zeilenstatus; hl darf NULL sein.
  * Zustand im bisherigen hl_open_comment-Feld: Kommentar, String und Makro. */
enum
{
	SYN_COMMENT = 1, SYN_STRING = 2, SYN_CHAR = 4,
	SYN_LINE_COMMENT = 8, SYN_PREPROC = 16
};

static void syntax_markieren ( uint8_t* hl, size_t begin, size_t end, int kind )
{
	if ( hl && end > begin ) memset ( hl + begin, kind, end - begin );
}

static int syntax_wortzeichen ( unsigned char c )
{
	return isalnum ( c ) || c == '_';
}

static int syntax_grosskonstante ( const char* word, size_t len )
{
	int letter = 0;
	for ( size_t i = 0; i < len; i++ )
	{
		unsigned char c = ( unsigned char ) word [ i ];
		if ( islower ( c ) ) return 0;
		if ( isupper ( c ) ) letter = 1;
	}
	return letter;
}

/* Z80-Assembler: unabhaengig von Gross-/Kleinschreibung. */
static const char* z80_befehle [ ] = {
 "ADC","ADD","AND","BIT","CALL","CCF","CP","CPD","CPDR","CPI","CPIR",
 "CPL","DAA","DEC","DI","DJNZ","EI","EX","EXX","HALT","IM","IN","INC",
 "IND","INDR","INI","INIR","JP","JR","LD","LDD","LDDR","LDI","LDIR",
 "NEG","NOP","OR","OTDR","OTIR","OUT","OUTD","OUTI","POP","PUSH","RES",
 "RET","RETI","RETN","RL","RLA","RLC","RLCA","RLD","RR","RRA","RRC",
 "RRCA","RRD","RST","SBC","SCF","SET","SLA","SRA","SRL","SUB","XOR",
 "SLL","SL1",NULL
};
static const char* z80_direktiven [ ] = {
 "ORG","EQU","DEFL","DB","DEFB","BYTE","DW","DEFW","WORD","DD","DEFD",
 "DS","DEFS","DEFM","DM","DC","TEXT","ASCII","ASCIZ","INCLUDE","INCBIN",
 "END","ENDM","MACRO","LOCAL","GLOBAL","PUBLIC","EXTERN","EXTRN","EXPORT",
 "IMPORT","IF","IFDEF","IFNDEF","ELSE","ELIF","ELSEIF","ENDIF","ENDC",
 "REPT","REPEAT","ENDR","ENDREPEAT","SECTION","SEGMENT","ALIGN","ASSERT",
 "DEVICE","SAVEBIN","SAVESNA","OUTPUT","ENT","DISP","ENDDISP",NULL
};
static const char* z80_register [ ] = {
 "A","F","B","C","D","E","H","L","AF","BC","DE","HL","IX","IY",
 "SP","PC","I","R","IXH","IXL","IYH","IYL",NULL
};
static const char* z80_bedingungen [ ] = { "NZ","Z","NC","C","PO","PE","P","M",NULL };

static int asm_wort_in_liste ( const char* s, size_t len, const char** list )
{
	for ( size_t i = 0; list [ i ]; i++ )
	{
		if ( strlen ( list [ i ] ) != len ) continue;
		size_t j = 0; while ( j < len && toupper ( ( unsigned char ) s [ j ] ) == list [ i ][ j ] )j++;
		if ( j == len )return 1;
	}
	return 0;
}
static int asm_symbolzeichen ( unsigned char c )
{
	return isalnum ( c ) || c == '_' || c == '.' || c == '@' || c == '?' || c == '$';
}
static int asm_zahl ( const char* s, size_t len )
{
	size_t a = 0, b = len; int base = 10;
	if ( !len )return 0;
	if ( s [ 0 ] == '$' || s [ 0 ] == '#' ) { base = 16; a = 1; } else if ( s [ 0 ] == '%' ) { base = 2; a = 1; } else if ( len > 2 && s [ 0 ] == '0' && ( s [ 1 ] == 'x' || s [ 1 ] == 'X' ) ) { base = 16; a = 2; } else if ( len > 2 && s [ 0 ] == '0' && ( s [ 1 ] == 'b' || s [ 1 ] == 'B' ) ) { base = 2; a = 2; } else if ( isdigit ( ( unsigned char ) s [ 0 ] ) )
	{
		int suffix = toupper ( ( unsigned char ) s [ len - 1 ] );
		if ( suffix == 'H' ) { base = 16; b--; } else if ( suffix == 'B' ) { base = 2; b--; } else if ( suffix == 'O' || suffix == 'Q' ) { base = 8; b--; } else if ( suffix == 'D' )b--;
	} else return 0;
	if ( a == b )return 0;
	for ( size_t i = a; i < b; i++ )
	{
		int c = toupper ( ( unsigned char ) s [ i ] ); int digit = c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10 : 99;
		if ( digit >= base )return 0;
	}
	return 1;
}
static void asm_zeile_faerben ( uint8_t* hl, Line* l, int previous, int* out )
{
	const char* s = l->chars; size_t n = l->len, i = 0;
	int first = 1, condition = 0, in_comment = previous != 0;
	syntax_markieren ( hl, 0, n, HL_NORMAL );
	while ( i < n )
	{
		size_t start = i; unsigned char c = ( unsigned char ) s [ i ];
		if ( in_comment )
		{
			if ( c == '*' && i + 1 < n && s [ i + 1 ] == '/' )
			{
				syntax_markieren ( hl, i, i + 2, HL_MLCOMMENT ); i += 2; in_comment = 0;
			} else { syntax_markieren ( hl, i, i + 1, HL_MLCOMMENT ); i++; }
			continue;
		}
		if ( isspace ( c ) ) { i++; continue; }
		if ( c == ';' || ( c == '/' && i + 1 < n && s [ i + 1 ] == '/' ) )
		{
			syntax_markieren ( hl, i, n, HL_COMMENT );
			while ( i < n )
			{
				if ( isalpha ( ( unsigned char ) s [ i ] ) )
				{
					start = i; while ( i < n && asm_symbolzeichen ( ( unsigned char ) s [ i ] ) )i++;
					static const char* tasks [ ] = { "TODO","FIXME","BUG","HACK","XXX",NULL };
					if ( asm_wort_in_liste ( s + start, i - start, tasks ) )syntax_markieren ( hl, start, i, HL_TODO );
				} else i++;
			}
			break;
		}
		if ( c == '/' && i + 1 < n && s [ i + 1 ] == '*' )
		{
			syntax_markieren ( hl, i, i + 2, HL_MLCOMMENT ); i += 2; in_comment = 1; continue;
		}
		if ( c == '"' || c == '\'' )
		{
			char quote = ( char ) c; i++; syntax_markieren ( hl, start, i, HL_STRING );
			while ( i < n )
			{
				start = i;
				if ( s [ i ] == '\\' && i + 1 < n ) { i += 2; syntax_markieren ( hl, start, i, HL_ESCAPE ); continue; }
				if ( s [ i++ ] == quote )
				{
					if ( i < n && s [ i ] == quote ) { i++; syntax_markieren ( hl, start, i, HL_STRING ); continue; }
					syntax_markieren ( hl, start, i, HL_STRING ); break;
				}
				syntax_markieren ( hl, start, i, HL_STRING );
			}
			first = 0; continue;
		}
		if ( asm_symbolzeichen ( c ) || ( ( c == '#' || c == '%' ) && i + 1 < n && isxdigit ( ( unsigned char ) s [ i + 1 ] ) ) )
		{
			i++; while ( i < n && asm_symbolzeichen ( ( unsigned char ) s [ i ] ) )i++;
			size_t len = i - start, next = i;
			while ( next < n && isspace ( ( unsigned char ) s [ next ] ) )next++;
			const char* word = s + start; size_t wlen = len;
			if ( wlen > 1 && word [ 0 ] == '.' ) { word++; wlen--; }
			int kind = HL_FUNCTION; /* Symbol / Sprungziel */
			if ( next < n && s [ next ] == ':' )kind = HL_LABEL;
			else if ( first && asm_wort_in_liste ( word, wlen, z80_befehle ) )
			{
				kind = HL_KEYWORD;
				static const char* jumps [ ] = { "JP","JR","CALL","RET",NULL };
				condition = asm_wort_in_liste ( word, wlen, jumps ); first = 0;
			} else if ( first && asm_wort_in_liste ( word, wlen, z80_direktiven ) ) { kind = HL_PREPROC; first = 0; } else if ( asm_zahl ( s + start, len ) )kind = HL_NUMBER;
			else if ( len == 1 && c == '$' )kind = HL_CONSTANT;
			else if ( first )
			{
				/* Label ohne Doppelpunkt vor bekanntem Befehl/Direktive. */
				size_t end = next; while ( end < n && asm_symbolzeichen ( ( unsigned char ) s [ end ] ) )end++;
				const char* p = s + next; size_t count = end - next; if ( count > 1 && *p == '.' ) { p++; count--; }
				if ( asm_wort_in_liste ( p, count, z80_befehle ) || asm_wort_in_liste ( p, count, z80_direktiven ) )kind = HL_LABEL;
				else first = 0; /* unbekanntes Makro */
			} else if ( condition && asm_wort_in_liste ( word, wlen, z80_bedingungen ) && ( next == n || s [ next ] == ',' || s [ next ] == ';' ) )
			{
				kind = HL_CONDITION; condition = 0;
			} else if ( asm_wort_in_liste ( word, wlen, z80_register ) )
			{
				kind = HL_TYPE;
				/* AF' ist ein Register, kein Beginn einer Zeichenkette. */
				if ( wlen == 2 && toupper ( ( unsigned char ) word [ 0 ] ) == 'A' && toupper ( ( unsigned char ) word [ 1 ] ) == 'F' && i < n && s [ i ] == '\'' )i++;
			}
			syntax_markieren ( hl, start, i, kind ); continue;
		}
		if ( c == ':' ) { first = 1; syntax_markieren ( hl, i, i + 1, HL_LABEL ); } else if ( strchr ( "()[]", c ) )syntax_markieren ( hl, i, i + 1, HL_BRACKET );
		else if ( strchr ( "+-*/%=<>!~&|^,#", c ) )syntax_markieren ( hl, i, i + 1, HL_OPERATOR );
		i++;
	}
	*out = in_comment;
}

static void zeile_syntaxhervorhebung_berechnen ( uint8_t* hl, Line* l, int prev_open, int* out_open )
{
	if ( E.syntax == 2 ) { asm_zeile_faerben ( hl, l, prev_open, out_open ); return; }
	const char* s = l->chars;
	size_t n = l->len, i = 0;
	int state = prev_open & ( SYN_COMMENT | SYN_STRING | SYN_CHAR | SYN_LINE_COMMENT );
	int preproc = ( prev_open & SYN_PREPROC ) != 0;
	int directive = 0, macro_name = 0, include_name = 0;
	int first = !preproc;
	int continued = n && s [ n - 1 ] == '\\';
	syntax_markieren ( hl, 0, n, HL_NORMAL );
	while ( i < n )
	{
		size_t begin = i;
		unsigned char c = ( unsigned char ) s [ i ];
		if ( state & ( SYN_COMMENT | SYN_LINE_COMMENT ) )
		{
			int kind = state & SYN_COMMENT ? HL_MLCOMMENT : HL_COMMENT;
			if ( ( state & SYN_COMMENT ) && c == '*' && i + 1 < n && s [ i + 1 ] == '/' )
			{
				syntax_markieren ( hl, i, i + 2, kind ); i += 2; state = 0; continue;
			}
			if ( isalpha ( c ) && ( !i || !syntax_wortzeichen ( ( unsigned char ) s [ i - 1 ] ) ) )
			{
				while ( i < n && syntax_wortzeichen ( ( unsigned char ) s [ i ] ) ) i++;
				static const char* tasks [ ] = { "TODO", "FIXME", "BUG", "HACK", "XXX", NULL };
				if ( wort_in_liste ( s + begin, i - begin, tasks ) ) kind = HL_TODO;
			} else i++;
			syntax_markieren ( hl, begin, i, kind ); continue;
		}
		if ( state & ( SYN_STRING | SYN_CHAR ) )
		{
			char quote = state & SYN_STRING ? '"' : '\'';
			int kind = HL_STRING;
			if ( c == '\\' )
			{
				kind = HL_ESCAPE; i++;
				if ( i < n )
				{
					char escape = s [ i++ ];
					if ( escape == 'x' ) while ( i < n && isxdigit ( ( unsigned char ) s [ i ] ) ) i++;
					else if ( escape == 'u' || escape == 'U' )
					{
						int limit = escape == 'u' ? 4 : 8;
						while ( limit-- && i < n && isxdigit ( ( unsigned char ) s [ i ] ) ) i++;
					} else if ( escape >= '0' && escape <= '7' )
					{
						int limit = 2;
						while ( limit-- && i < n && s [ i ] >= '0' && s [ i ] <= '7' ) i++;
					}
				}
			} else { i++; if ( c == ( unsigned char ) quote ) state = 0; }
			syntax_markieren ( hl, begin, i, kind ); continue;
		}
		if ( isspace ( c ) ) { i++; continue; }
		if ( c == '/' && i + 1 < n && ( s [ i + 1 ] == '/' || s [ i + 1 ] == '*' ) )
		{
			state = s [ i + 1 ] == '/' ? SYN_LINE_COMMENT : SYN_COMMENT;
			syntax_markieren ( hl, i, i + 2, state == SYN_COMMENT ? HL_MLCOMMENT : HL_COMMENT );
			i += 2; continue;
		}
		if ( c == '#' && first )
		{
			preproc = directive = 1; first = 0;
			syntax_markieren ( hl, i, i + 1, HL_PREPROC ); i++; continue;
		}
		first = 0;
		if ( include_name && c == '<' )
		{
			i++; while ( i < n && s [ i ] != '>' ) i++;
			if ( i < n ) i++;
			syntax_markieren ( hl, begin, i, HL_STRING ); include_name = 0; continue;
		}
		if ( c == '"' || c == '\'' )
		{
			state = c == '"' ? SYN_STRING : SYN_CHAR;
			syntax_markieren ( hl, i, i + 1, HL_STRING ); i++; include_name = 0; continue;
		}
		if ( isdigit ( c ) || ( c == '.' && i + 1 < n && isdigit ( ( unsigned char ) s [ i + 1 ] ) ) )
		{
			int hex = c == '0' && i + 1 < n && ( s [ i + 1 ] == 'x' || s [ i + 1 ] == 'X' );
			i++;
			/* C-Zahlentoken inkl. Suffix, Exponent und Binaerschreibweise. */
			while ( i < n )
			{
				unsigned char d = ( unsigned char ) s [ i ];
				char last = s [ i - 1 ];
				if ( syntax_wortzeichen ( d ) || d == '.' ) { i++; continue; }
				if ( ( d == '+' || d == '-' ) &&
					 ( hex ? ( last == 'p' || last == 'P' ) : ( last == 'e' || last == 'E' ) ) )
				{
					i++; continue;
				}
				break;
			}
			syntax_markieren ( hl, begin, i, HL_NUMBER ); continue;
		}
		if ( isalpha ( c ) || c == '_' )
		{
			while ( i < n && syntax_wortzeichen ( ( unsigned char ) s [ i ] ) ) i++;
			size_t len = i - begin, next = i, before = begin;
			while ( next < n && isspace ( ( unsigned char ) s [ next ] ) ) next++;
			while ( before && isspace ( ( unsigned char ) s [ before - 1 ] ) ) before--;
			int kind = HL_NORMAL;
			if ( directive )
			{
				kind = HL_PREPROC; directive = 0;
				macro_name = ( len == 6 && !memcmp ( s + begin, "define", 6 ) ) ||
					( len == 5 && !memcmp ( s + begin, "undef", 5 ) );
				include_name = len == 7 && !memcmp ( s + begin, "include", 7 );
			} else if ( macro_name ) { kind = HL_CONSTANT; macro_name = 0; } else if ( wort_in_liste ( s + begin, len, c_keywords ) ) kind = HL_KEYWORD;
			else if ( wort_in_liste ( s + begin, len, c_types ) ||
				( len > 2 && !memcmp ( s + i - 2, "_t", 2 ) ) ||
				( len > 3 && ( !memcmp ( s + begin, "Gtk", 3 ) || !memcmp ( s + begin, "Gdk", 3 ) ) && isupper ( ( unsigned char ) s [ begin + 3 ] ) ) ) kind = HL_TYPE;
			else if ( before && ( s [ before - 1 ] == '.' || ( before >= 2 && s [ before - 2 ] == '-' && s [ before - 1 ] == '>' ) ) ) kind = HL_MEMBER;
			else if ( syntax_grosskonstante ( s + begin, len ) ||
				( len == 4 && !memcmp ( s + begin, "true", 4 ) ) ||
				( len == 5 && !memcmp ( s + begin, "false", 5 ) ) ||
				( len == 7 && !memcmp ( s + begin, "nullptr", 7 ) ) ) kind = HL_CONSTANT;
			else if ( next < n && s [ next ] == '(' ) kind = HL_FUNCTION;
			else if ( len > 4 && !memcmp ( s + begin, "z80_", 4 ) ) kind = HL_FUNCTION;
			syntax_markieren ( hl, begin, i, kind ); continue;
		}
		if ( strchr ( "+-*/%=!<>~&|^?:", c ) ) syntax_markieren ( hl, i, i + 1, HL_OPERATOR );
		else if ( strchr ( "(){}[]", c ) ) syntax_markieren ( hl, i, i + 1, HL_BRACKET );
		else if ( preproc && ( c == '#' || c == '\\' ) ) syntax_markieren ( hl, i, i + 1, HL_PREPROC );
		i++;
	}
	*out_open = ( state & SYN_COMMENT ) | ( continued ? state | ( preproc ? SYN_PREPROC : 0 ) : 0 );
}

static void syntaxzustand_aktualisieren ( void )
{
	if ( !E.syntax ) return;
	if ( E.syntax_revision == E.revision && E.cached_syntax == E.syntax ) return;
	E.syntax_revision = E.revision; E.cached_syntax = E.syntax;
	int open = 0;
	for ( size_t i = 0; i < E.nlines; i++ )
	{
		zeile_syntaxhervorhebung_berechnen ( NULL, &E.lines [ i ], open, &open );
		E.lines [ i ].hl_open_comment = open;
	}
}


/* ---------------------------------------------------------------
 * 30. bildschirmzeilen_speicher_sicherstellen()
 * --------------------------------------------------------------- */
static void bildschirmzeilen_speicher_sicherstellen ( size_t need )
{
	if ( E.vlines_cap >= need ) return;
	if ( need > SIZE_MAX / sizeof ( VLine ) ) mit_fehler_beenden ( "Zu viele Bildschirmzeilen" );
	size_t c = E.vlines_cap ? E.vlines_cap : 256;
	while ( c < need )
	{
		if ( c > SIZE_MAX / sizeof ( VLine ) / 2 ) { c = need; break; }
		c *= 2;
	}
	E.vlines = speicher_sicher_neu_reservieren ( E.vlines, c * sizeof ( VLine ) );
	E.vlines_cap = c;
}

/* ---------------------------------------------------------------
 * 31. bildschirmzeilen_aktualisieren()
 * --------------------------------------------------------------- */
static void bildschirmzeilen_aktualisieren ( void )
{
	int width = E.screen_cols - E.gutter_width;
	if ( E.nvlines && E.wrap_revision == E.revision &&
		 E.cached_cols == width && E.cached_wrap == E.word_wrap ) return;
	E.wrap_revision = E.revision; E.cached_cols = width; E.cached_wrap = E.word_wrap;
	E.nvlines = 0;

	int text_cols = E.screen_cols - E.gutter_width;
	if ( text_cols < 1 ) text_cols = 1;

	for ( size_t i = 0; i < E.nlines; i++ )
	{
		Line* l = &E.lines [ i ];

		if ( !E.word_wrap || l->len == 0 )
		{
			bildschirmzeilen_speicher_sicherstellen ( E.nvlines + 1 );
			E.vlines [ E.nvlines ].logical_row = i;
			E.vlines [ E.nvlines ].char_start = 0;
			E.vlines [ E.nvlines ].char_len = l->len;
			E.nvlines++;
			continue;
		}

		/* Zeile anhand der Render-Breite in Segmente aufteilen */
		size_t j = 0;
		do
		{
			size_t seg_start = j;
			size_t rx = 0;

			while ( j < l->len && ( int ) rx < text_cols )
			{
				if ( l->chars [ j ] == '\t' )
				{
					size_t tw = TAB_STOP - ( rx % TAB_STOP );
					if ( ( int ) ( rx + tw ) > text_cols && rx > 0 ) break;
					rx += tw;
				} else { rx++; } j++;
			}

			bildschirmzeilen_speicher_sicherstellen ( E.nvlines + 1 );
			E.vlines [ E.nvlines ].logical_row = i;
			E.vlines [ E.nvlines ].char_start = seg_start;
			E.vlines [ E.nvlines ].char_len = j - seg_start;
			E.nvlines++;
		} while ( j < l->len );
	}

	/* Mindestens eine VLine garantieren */
	if ( E.nvlines == 0 )
	{
		bildschirmzeilen_speicher_sicherstellen ( 1 );
		E.vlines [ 0 ].logical_row = 0;
		E.vlines [ 0 ].char_start = 0;
		E.vlines [ 0 ].char_len = 0;
		E.nvlines = 1;
	}
}

/* ---------------------------------------------------------------
 * 32. bildschirmzeile_finden()
 * --------------------------------------------------------------- */
static size_t bildschirmzeile_finden ( size_t cy, size_t cx )
{
	size_t lo = 0, hi = E.nvlines;
	while ( lo < hi )
	{
		size_t mid = lo + ( hi - lo ) / 2;
		VLine* v = &E.vlines [ mid ];
		if ( v->logical_row < cy || ( v->logical_row == cy && v->char_start <= cx ) ) lo = mid + 1;
		else hi = mid;
	}
	return lo ? lo - 1 : 0;
}

/* ---------------------------------------------------------------
 * 33. segment_zeichenindex_zu_bildschirmspalte()
 * --------------------------------------------------------------- */
static size_t segment_zeichenindex_zu_bildschirmspalte ( Line* l, size_t seg_start, size_t cx )
{
	size_t rx = 0;
	for ( size_t j = seg_start; j < cx && j < l->len; j++ )
	{
		if ( l->chars [ j ] == '\t' ) rx += TAB_STOP - ( rx % TAB_STOP ); else rx++;
	}
	return rx;
}

/* ---------------------------------------------------------------
 * 34. segment_bildschirmspalte_zu_zeichenindex()
 * --------------------------------------------------------------- */
static size_t segment_bildschirmspalte_zu_zeichenindex ( Line* l, size_t seg_start, size_t seg_len, size_t target_rx )
{
	size_t rx = 0;
	size_t end = seg_start + seg_len;
	if ( end > l->len ) end = l->len;
	for ( size_t j = seg_start; j < end; j++ )
	{
		size_t next_rx;
		if ( l->chars [ j ] == '\t' ) next_rx = rx + TAB_STOP - ( rx % TAB_STOP ); else next_rx = rx + 1;
		if ( next_rx > target_rx ) return j;
		rx = next_rx;
	}
	return end;
}

/* ---------------------------------------------------------------
 * 35. bildausschnitt_anpassen()
 * --------------------------------------------------------------- */
static void bildausschnitt_anpassen ( void )
{
	/* Gutter-Breite berechnen */
	int digits = 1;
	size_t n = E.nlines;
	while ( n >= 10 ) { n /= 10; digits++; }
	E.gutter_width = digits + 2;

	int text_cols = E.screen_cols - E.gutter_width;
	if ( text_cols < 1 ) text_cols = 1;

	bildschirmzeilen_aktualisieren ( );

	if ( E.word_wrap )
	{
		E.coloff = 0;
		size_t cur_vl = bildschirmzeile_finden ( E.cy, E.cx );
		if ( cur_vl < E.rowoff ) E.rowoff = cur_vl;
		if ( cur_vl >= E.rowoff + ( size_t ) E.screen_rows )
			E.rowoff = cur_vl - E.screen_rows + 1;
	} else
	{
		if ( E.cy < E.rowoff ) E.rowoff = E.cy;
		if ( E.cy >= E.rowoff + ( size_t ) E.screen_rows ) E.rowoff = E.cy - E.screen_rows + 1;
		if ( E.cx < E.coloff ) E.coloff = E.cx;
		if ( E.cx >= E.coloff + ( size_t ) text_cols ) E.coloff = E.cx - text_cols + 1;
	}
}

/* ---------------------------------------------------------------
 * 36. textzeilen_zeichnen()
 * --------------------------------------------------------------- */
static void textzeilen_zeichnen ( Abuf* a )
{
	int text_cols = E.screen_cols - E.gutter_width;
	if ( text_cols < 1 ) text_cols = 1;
	int num_width = E.gutter_width - 2;

	for ( int y = 0; y < E.screen_rows; y++ )
	{
		size_t vline_idx = E.rowoff + ( size_t ) y;
		if ( vline_idx < E.nvlines )
		{
			VLine* vl = &E.vlines [ vline_idx ];
			size_t filerow = vl->logical_row;
			Line* l = &E.lines [ filerow ];

			/* Zeilennummer zeichnen */
			char numbuf [ 48 ];
			if ( vl->char_start == 0 )
			{
				if ( filerow == E.cy )
				{
					snprintf ( numbuf, sizeof numbuf, "\x1b[33m%*zu \x1b[90m|\x1b[m", num_width, filerow + 1 );
				} else { snprintf ( numbuf, sizeof numbuf, "\x1b[90m%*zu |\x1b[m", num_width, filerow + 1 ); }
			} else
			{
				/* Für Folgesegmente ein leeres Gutter mit Umbruch-Indikator '+' zeichnen */
				snprintf ( numbuf, sizeof numbuf, "\x1b[90m%*s\x1b[36m+\x1b[90m |\x1b[m", num_width - 1, "" );
			}
			ausgabepuffer_text_anhaengen ( a, numbuf );

			/* Syntax-Highlighting für diese Zeile berechnen */
			uint8_t* hl = NULL;
			if ( E.syntax && l->len > 0 )
			{
				hl = calloc ( l->len, 1 );	if ( hl )
				{
					int prev_open = ( filerow > 0 ) ? E.lines [ filerow - 1 ].hl_open_comment : 0;
					int out_open = 0;
					zeile_syntaxhervorhebung_berechnen ( hl, l, prev_open, &out_open );
				}
			}

			/* Zeileninhalt mit Tab-Expansion, Highlighting und Selektion rendern */
			size_t rx = 0;
			int printed = 0;
			int last_hl = -1;
			int in_sel = 0;


			size_t seg_end = vl->char_start + vl->char_len;
			if ( seg_end > l->len ) seg_end = l->len;

			for ( size_t j = vl->char_start; j < seg_end && printed < text_cols; j++ )
			{
				int cur_hl = hl ? hl [ j ] : HL_NORMAL;
				int sel = position_in_markierung ( filerow, j );

				if ( l->chars [ j ] == '\t' )
				{
					size_t spaces = TAB_STOP - ( rx % TAB_STOP );
					for ( size_t s = 0; s < spaces; s++ )
					{
						if ( !E.word_wrap && rx < E.coloff )
						{
							rx++;
							continue;
						}
						if ( printed < text_cols )
						{
							if ( sel != in_sel )
							{
								ausgabepuffer_text_anhaengen ( a, sel ? "\x1b[7m" : "\x1b[27m" );
								in_sel = sel;
							}
							if ( cur_hl != last_hl )
							{
								ausgabepuffer_text_anhaengen ( a, syntaxfarbe_ermitteln ( cur_hl ) );
								last_hl = cur_hl;
							}
							ausgabepuffer_text_anhaengen ( a, " " );
							printed++;
						}
						rx++;
					}
				} else
				{
					if ( !E.word_wrap && rx < E.coloff )
					{
						rx++;
						continue;
					}
					if ( printed < text_cols )
					{
						if ( sel != in_sel )
						{
							ausgabepuffer_text_anhaengen ( a, sel ? "\x1b[7m" : "\x1b[27m" );
							in_sel = sel;
						}
						if ( cur_hl != last_hl )
						{
							ausgabepuffer_text_anhaengen ( a, syntaxfarbe_ermitteln ( cur_hl ) );
							last_hl = cur_hl;
						}
						/* Steuerzeichen duerfen keine Terminalbefehle ausloesen. */
						unsigned char ch = ( unsigned char ) l->chars [ j ];
						if ( ch < 32 || ch == 127 ) ausgabepuffer_text_anhaengen ( a, "." );
						else ausgabepuffer_daten_anhaengen ( a, &l->chars [ j ], 1 );
						printed++;
					}
					rx++;
				}
			}

			/* Attribute zurücksetzen */
			if ( in_sel || last_hl > 0 ) ausgabepuffer_text_anhaengen ( a, "\x1b[m" );
			free ( hl );
		} else
		{
			/* Leere Zeile nach Dateiende */
			char numbuf [ 48 ];
			snprintf ( numbuf, sizeof numbuf, "\x1b[90m%*s |\x1b[m", num_width, "~" );
			ausgabepuffer_text_anhaengen ( a, numbuf );
		}
		ausgabepuffer_text_anhaengen ( a, "\x1b[K\r\n" );
	}
}

/* ---------------------------------------------------------------
 * 37. statuszeile_zeichnen()
 * --------------------------------------------------------------- */
static void statuszeile_zeichnen ( Abuf* a )
{
	ausgabepuffer_text_anhaengen ( a, "\x1b[7m" );
	char left [ 256 ], right [ 64 ];
	const char* syn_label = E.syntax == 2 ? " [Z80]" : E.syntax ? " [C]" : "";
	const char* wrap_label = E.word_wrap ? " [Wrap]" : "";
	int ln = snprintf ( left, sizeof left, " %.70s%s%s%s ",
					   E.filename [ 0 ] ? E.filename : "[Neu]",
					   E.dirty ? " *" : "", syn_label, wrap_label );
	int rn = snprintf ( right, sizeof right, " Z %zu/%zu Sp %zu ",
					   E.cy + 1, E.nlines ? E.nlines : 1, E.cx + 1 );
	if ( ln > E.screen_cols ) ln = E.screen_cols;
	ausgabepuffer_daten_anhaengen ( a, left, ( size_t ) ln );
	for ( int i = ln; i < E.screen_cols - rn; i++ ) ausgabepuffer_text_anhaengen ( a, " " );
	if ( rn > 0 && ln + rn <= E.screen_cols ) ausgabepuffer_daten_anhaengen ( a, right, ( size_t ) rn );
	ausgabepuffer_text_anhaengen ( a, "\x1b[m\r\n" );
}

/* ---------------------------------------------------------------
 * 38. meldungszeile_zeichnen()
 * --------------------------------------------------------------- */
static void meldungszeile_zeichnen ( Abuf* a )
{
	ausgabepuffer_text_anhaengen ( a, "\x1b[K" );

	if ( E.status [ 0 ] && GetTickCount64 ( ) - E.status_time < E.status_duration )
	{
		int n = ( int ) strlen ( E.status );
		if ( n > E.screen_cols ) n = E.screen_cols;
		ausgabepuffer_daten_anhaengen ( a, E.status, ( size_t ) n );
	} else if ( E.ctrl_pressed )
	{
		const char* help = "^S:Speichern | ^A:Speichern unter | ^O:Oeffnen | ^Q:Ende | ^Z:Zurueck | ^Y:Wiederholen | F1:Hilfe";
		int n = ( int ) strlen ( help );
		if ( n > E.screen_cols ) n = E.screen_cols;
		ausgabepuffer_daten_anhaengen ( a, help, ( size_t ) n );
	} else if ( E.sel_active )
	{
		size_t sr, sc, er, ec;
		markierung_normalisieren ( &sr, &sc, &er, &ec );
		char sel_info [ 128 ];
		if ( sr == er )
		{
			snprintf ( sel_info, sizeof sel_info, "[Markierung: %zu Zeichen]  Strg+C=Kopieren  Strg+X=Ausschneiden  ESC=Abbrechen", ec - sc );
		} else
		{
			snprintf ( sel_info, sizeof sel_info, "[Markierung: %zu Zeilen]  Strg+C=Kopieren  Strg+X=Ausschneiden  ESC=Abbrechen", er - sr + 1 );
		}
		int n = ( int ) strlen ( sel_info );
		if ( n > E.screen_cols ) n = E.screen_cols;
		ausgabepuffer_daten_anhaengen ( a, sel_info, ( size_t ) n );
	} else
	{
		const char* hint = "F10: Menue | F1: Hilfe | F3: Weitersuchen | Strg: Tastenhilfe";
		int n = ( int ) strlen ( hint );
		if ( n > E.screen_cols ) n = E.screen_cols;
		ausgabepuffer_daten_anhaengen ( a, hint, ( size_t ) n );
	}
}

/* ---------------------------------------------------------------
 * 39. feste_menueleiste_zeichnen()
 * --------------------------------------------------------------- */
static void feste_menueleiste_zeichnen ( Abuf* a )
{
	/* Ganze Zeile zuerst im normalen Konsolenstil loeschen. */
	ausgabepuffer_text_anhaengen ( a, "\x1b[1;1H\x1b[m\x1b[K" );

	/* Der helle Menuebalken beginnt erst in Spalte 5.
	   Die vier Zeichen links davon bleiben dunkel. */
	ausgabepuffer_text_anhaengen ( a, "\x1b[1;5H\x1b[47m\x1b[30m" );
	ausgabepuffer_text_anhaengen ( a, "Datei    Bearbeiten    Suchen    Hilfe " );

	/* Rest der Zeile weiterhin hell fuellen. */
	ausgabepuffer_text_anhaengen ( a, "\x1b[K\x1b[m" );
}

/* ---------------------------------------------------------------
 * 40. anzeige_aktualisieren()
 * --------------------------------------------------------------- */
static void anzeige_aktualisieren ( void )
{
	terminal_groesse_ermitteln ( );
	bildausschnitt_anpassen ( );
	syntaxzustand_aktualisieren ( );
	Abuf ab = { 0 };

	/* Cursor ausblenden, feste Menueleiste in Zeile 1 zeichnen,
	   danach den Editor bewusst erst in Zeile 2 beginnen lassen. */
	ausgabepuffer_text_anhaengen ( &ab, "\x1b[?25l" );
	feste_menueleiste_zeichnen ( &ab );
	ausgabepuffer_text_anhaengen ( &ab, "\x1b[2;1H" );

	textzeilen_zeichnen ( &ab );
	statuszeile_zeichnen ( &ab );
	meldungszeile_zeichnen ( &ab );

	char cur [ 32 ];
	if ( E.in_prompt )
	{
		/* Cursor in der Eingabezeile (Message-Bar) positionieren */
		int prompt_row = E.screen_rows + 3;
		snprintf ( cur, sizeof cur, "\x1b[%d;%dH", prompt_row, E.prompt_cursor_col );
	} else
	{
		int cy_screen;
		int cx_screen;
		if ( E.word_wrap )
		{
			size_t cur_vl = bildschirmzeile_finden ( E.cy, E.cx );
			cy_screen = ( int ) ( cur_vl - E.rowoff ) + 2;
			size_t rx_in_vl = segment_zeichenindex_zu_bildschirmspalte ( &E.lines [ E.cy ], E.vlines [ cur_vl ].char_start, E.cx );
			cx_screen = ( int ) rx_in_vl + 1 + E.gutter_width;
		} else
		{
			cy_screen = ( int ) ( E.cy - E.rowoff ) + 2;
			size_t rx = E.cx;
			if ( E.cy < E.nlines )
			{
				rx = zeichenindex_zu_bildschirmspalte ( &E.lines [ E.cy ], E.cx );
			}
			cx_screen = ( int ) ( rx - E.coloff ) + 1 + E.gutter_width;
		}
		if ( cx_screen > E.screen_cols ) cx_screen = E.screen_cols;
		snprintf ( cur, sizeof cur, "\x1b[%d;%dH", cy_screen, cx_screen );
	}
	ausgabepuffer_text_anhaengen ( &ab, cur );
	ausgabepuffer_text_anhaengen ( &ab, "\x1b[?25h" );

	DWORD n;
	WriteConsoleA ( hOut, ab.b, ( DWORD ) ab.len, &n, NULL );
	free ( ab.b );
}

/* ---------------------------------------------------------------
 * 41. statusmeldung_setzen()
 * --------------------------------------------------------------- */
static void statusmeldung_setzen ( const char* fmt, ... )
{
	va_list ap; va_start ( ap, fmt );
	vsnprintf ( E.status, sizeof E.status, fmt, ap );
	va_end ( ap );
	E.status_time = GetTickCount64 ( );
	E.status_duration = 5000;
}

/* ---------------------------------------------------------------
 * 42. rueckgaengig_machen()
 * --------------------------------------------------------------- */
 /* Beim Wiederherstellen werden Puffer getauscht, nicht erneut kopiert. */
static void verlaufszustand_wiederherstellen ( int redo )
{
	UndoState* source = redo ? E.redo : E.undo;
	size_t* count = redo ? &E.redo_count : &E.undo_count;
	if ( !*count ) { statusmeldung_setzen ( "Keine weitere Aktion vorhanden." ); return; }
	UndoState current = { 0 };
	current.lines = E.lines; current.nlines = E.nlines; current.cap = E.cap;
	current.cx = E.cx; current.cy = E.cy; current.revision = E.revision;
	current.bytes = E.cap * sizeof ( Line );
	for ( size_t i = 0; i < E.nlines; i++ ) current.bytes += E.lines [ i ].cap;
	UndoState state = source [ --*count ];
	verlaufszustand_ablegen ( redo ? E.undo : E.redo, redo ? &E.undo_count : &E.redo_count, current );
	E.lines = state.lines; E.nlines = state.nlines; E.cap = state.cap;
	E.cx = state.cx; E.cy = state.cy; E.revision = state.revision;
	E.dirty = E.revision != E.saved_revision;
	E.sel_active = 0; E.goal_valid = 0;
	E.syntax_revision = E.wrap_revision = UINT64_MAX;
	statusmeldung_setzen ( redo ? "Aktion wiederholt." : "Aktion rueckgaengig gemacht." );
}

static void rueckgaengig_machen ( void ) { verlaufszustand_wiederherstellen ( 0 ); }

/* ---------------------------------------------------------------
 * 43. aktuelle_zeile_kopieren()
 * --------------------------------------------------------------- */
static void aktuelle_zeile_kopieren ( void )
{
	if ( E.cy >= E.nlines ) return;
	free ( E.clipboard );

	Line* cur = &E.lines [ E.cy ];
	E.clip_len = cur->len;
	E.clipboard = speicher_sicher_neu_reservieren ( NULL, E.clip_len + 1 );
	if ( !E.clipboard ) { E.clip_len = 0; return; }
	if ( E.clip_len > 0 )
	{
		memcpy ( E.clipboard, cur->chars, E.clip_len );
	}
	E.clipboard [ E.clip_len ] = '\0';
	E.clip_is_line = 1;
	statusmeldung_setzen ( "Zeile kopiert!" );
	E.status_duration = 1500;
}

/* ---------------------------------------------------------------
 * 44. aktuelle_zeile_ausschneiden()
 * --------------------------------------------------------------- */
static void aktuelle_zeile_ausschneiden ( void )
{
	if ( E.cy >= E.nlines ) return;
	rueckgaengig_zustand_sichern ( );
	aktuelle_zeile_kopieren ( );
	zeile_entfernen ( E.cy );
	if ( E.nlines == 0 ) zeile_einfuegen ( 0, "", 0 );
	if ( E.cy >= E.nlines ) E.cy = E.nlines - 1;
	E.cx = 0;
	E.dirty = 1;
	statusmeldung_setzen ( "Zeile ausgeschnitten." );
	E.status_duration = 1500;
}

/* ---------------------------------------------------------------
 * 45. zwischenablage_einfuegen()
 * --------------------------------------------------------------- */
static void zwischenablage_einfuegen ( void )
{
	if ( !E.clipboard || ( !E.clip_is_line && E.clip_len == 0 ) )
	{
		statusmeldung_setzen ( "Zwischenablage ist leer!" );
		E.status_duration = 1500;
		return;
	}
	rueckgaengig_zustand_sichern ( );
	if ( E.sel_active ) markierung_loeschen ( );

	if ( E.clip_is_line )
	{
		/* Ganze Zeile einfügen (alte Verhaltensweise) */
		zeile_einfuegen ( E.cy, E.clipboard, E.clip_len );
		E.cy++;
		E.cx = 0;
	} else
	{
		/* Textfragment an Cursorposition einfügen */
		if ( E.cy >= E.nlines ) zeile_einfuegen ( E.nlines, "", 0 );
		for ( size_t i = 0; i < E.clip_len; i++ )
		{
			if ( E.clipboard [ i ] == '\n' )
			{
				/* Neue Zeile: Rest der aktuellen Zeile abtrennen */
				Line* cur = &E.lines [ E.cy ];
				size_t tail_len = ( E.cx < cur->len ) ? cur->len - E.cx : 0;
				int old_eol = cur->eol;
				zeile_einfuegen ( E.cy + 1, tail_len ? cur->chars + E.cx : NULL, tail_len );
				E.lines [ E.cy + 1 ].eol = old_eol;
				E.lines [ E.cy ].eol = E.eol;
				cur = &E.lines [ E.cy ];  /* Pointer kann sich verschoben haben */
				cur->len = E.cx;
				E.cy++;
				E.cx = 0;
			} else
			{
				/* Zusammenhaengenden Text in einem Schritt einschieben. */
				size_t start = i;
				while ( i < E.clip_len && E.clipboard [ i ] != '\n' ) i++;
				size_t len = i - start;
				Line* cur = &E.lines [ E.cy ];
				if ( len > SIZE_MAX - cur->len ) mit_fehler_beenden ( "Zwischenablage ist zu gross" );
				zeile_speicher_sicherstellen ( cur, cur->len + len );
				memmove ( cur->chars + E.cx + len, cur->chars + E.cx, cur->len - E.cx );
				memcpy ( cur->chars + E.cx, E.clipboard + start, len );
				cur->len += len; E.cx += len;
				i--;
			}
		}
	}
	E.dirty = 1;
	statusmeldung_setzen ( "Eingefügt." );
	E.status_duration = 1500;
}

/* ---------------------------------------------------------------
 * 46. aktuelle_zeile_duplizieren()
 * --------------------------------------------------------------- */
static void aktuelle_zeile_duplizieren ( void )
{
	if ( E.cy >= E.nlines ) return;
	rueckgaengig_zustand_sichern ( );
	Line* cur = &E.lines [ E.cy ];
	zeile_einfuegen ( E.cy + 1, cur->chars, cur->len );
	E.cy++;
	E.dirty = 1;
	E.sel_active = 0;
	statusmeldung_setzen ( "Zeile dupliziert." );
	E.status_duration = 1500;
}

/* ---------------------------------------------------------------
 * 47. aktuelle_zeile_loeschen()
 * --------------------------------------------------------------- */
static void aktuelle_zeile_loeschen ( void )
{
	if ( E.nlines == 0 ) return;
	rueckgaengig_zustand_sichern ( );
	zeile_entfernen ( E.cy );
	if ( E.nlines == 0 ) zeile_einfuegen ( 0, "", 0 );
	if ( E.cy >= E.nlines ) E.cy = E.nlines - 1;
	E.cx = 0;
	E.dirty = 1;
	E.sel_active = 0;
	statusmeldung_setzen ( "Zeile gelöscht." );
	E.status_duration = 1500;
}

/* ---------------------------------------------------------------
 * 48. markierung_kopieren()
 * --------------------------------------------------------------- */
static void markierung_kopieren ( void )
{
	if ( !E.sel_active ) return;
	size_t sr, sc, er, ec;
	markierung_normalisieren ( &sr, &sc, &er, &ec );
	if ( sr == er && sc == ec ) return;

	/* Gesamtlänge berechnen */
	size_t total = 0;
	for ( size_t i = sr; i <= er && i < E.nlines; i++ )
	{
		Line* l = &E.lines [ i ];
		size_t start = ( i == sr ) ? sc : 0;
		size_t end = ( i == er ) ? ec : l->len;
		if ( end > l->len ) end = l->len;
		if ( start > end ) start = end;
		total += ( end - start );
		if ( i < er ) total++;  /* \n zwischen Zeilen */
	}

	free ( E.clipboard );
	E.clipboard = speicher_sicher_neu_reservieren ( NULL, total + 1 );
	if ( !E.clipboard ) { E.clip_len = 0; return; }

	size_t pos = 0;
	for ( size_t i = sr; i <= er && i < E.nlines; i++ )
	{
		Line* l = &E.lines [ i ];
		size_t start = ( i == sr ) ? sc : 0;
		size_t end = ( i == er ) ? ec : l->len;
		if ( end > l->len ) end = l->len;
		if ( start > end ) start = end;
		size_t len = end - start;
		if ( len > 0 ) memcpy ( E.clipboard + pos, l->chars + start, len );
		pos += len;
		if ( i < er ) E.clipboard [ pos++ ] = '\n';
	}
	E.clipboard [ pos ] = '\0';
	E.clip_len = total;
	E.clip_is_line = 0;

	statusmeldung_setzen ( "Block kopiert (%zu Zeichen).", total );
	E.status_duration = 1500;
}

/* ---------------------------------------------------------------
 * 49. markierung_ausschneiden()
 * --------------------------------------------------------------- */
static void markierung_ausschneiden ( void )
{
	if ( !E.sel_active ) return;
	markierung_kopieren ( );
	markierung_loeschen ( );
	statusmeldung_setzen ( "Block ausgeschnitten." );
	E.status_duration = 1500;
}

/* ---------------------------------------------------------------
 * 50. datei_oeffnen()
 * --------------------------------------------------------------- */
static int datei_oeffnen ( const char* path )
{
	if ( strlen ( path ) >= MAX_PATH ) { statusmeldung_setzen ( "Dateipfad ist zu lang." ); return 0; }
	FILE* fp = fopen ( path, "rb" );
	if ( !fp ) { statusmeldung_setzen ( "Oeffnen fehlgeschlagen: %s", strerror ( errno ) ); return 0; }
	/* Erst vollstaendig lesen; bei Fehlern bleibt das aktuelle Dokument erhalten. */
	Abuf data = { 0 };
	char block [ 4096 ];
	size_t n;
	while ( ( n = fread ( block, 1, sizeof block, fp ) ) != 0 ) ausgabepuffer_daten_anhaengen ( &data, block, n );
	int failed = ferror ( fp );
	if ( fclose ( fp ) != 0 ) failed = 1;
	if ( failed ) { free ( data.b ); statusmeldung_setzen ( "Lesefehler: Dokument bleibt erhalten." ); return 0; }
	editor_puffer_leeren ( );
	memmove ( E.filename, path, strlen ( path ) + 1 );
	size_t start = 0;
	int first_eol = 0;
	for ( size_t i = 0; i < data.len; i++ )
	{
		if ( data.b [ i ] != '\r' && data.b [ i ] != '\n' ) continue;
		int eol = data.b [ i ] == '\n' ? 1 : 3;
		zeile_einfuegen ( E.nlines, data.b + start, i - start );
		if ( eol == 3 && i + 1 < data.len && data.b [ i + 1 ] == '\n' ) { eol = 2; i++; }
		E.lines [ E.nlines - 1 ].eol = eol;
		if ( !first_eol ) E.eol = first_eol = eol;
		start = i + 1;
	}
	/* Die letzte leere Zeile bildet einen vorhandenen Abschlussumbruch ab. */
	zeile_einfuegen ( E.nlines, data.len > start ? data.b + start : NULL, data.len - start );
	free ( data.b );
	dateityp_erkennen ( );
	statusmeldung_setzen ( "Geladen: %s (%zu Zeilen)", path, E.nlines );
	return 1;
}

/* ---------------------------------------------------------------
 * 51. datei_an_pfad_speichern()
 * --------------------------------------------------------------- */
static int datei_an_pfad_speichern ( const char* path )
{
	char target [ MAX_PATH ], directory [ MAX_PATH ], temporary [ MAX_PATH ];
	if ( !*path || strlen ( path ) >= MAX_PATH )
	{
		statusmeldung_setzen ( "Ungueltiger oder zu langer Dateipfad." ); return 0;
	}
	char* part = NULL;
	DWORD length = GetFullPathNameA ( path, MAX_PATH, target, &part );
	if ( !length || length >= MAX_PATH || !part || !*part )
	{
		statusmeldung_setzen ( "Ungueltiger oder zu langer Dateipfad." ); return 0;
	}
	strcpy ( directory, target );
	directory [ part - target ] = '\0';
	if ( !GetTempFileNameA ( directory, "dos", 0, temporary ) )
	{
		statusmeldung_setzen ( "Temporaere Datei nicht moeglich (Fehler %lu).", GetLastError ( ) ); return 0;
	}
	FILE* fp = fopen ( temporary, "wb" );
	if ( !fp ) { DeleteFileA ( temporary ); statusmeldung_setzen ( "Speichern fehlgeschlagen." ); return 0; }
	size_t total = 0;
	int failed = 0;
	for ( size_t i = 0; i < E.nlines && !failed; i++ )
	{
		Line* l = &E.lines [ i ];
		if ( l->len && fwrite ( l->chars, 1, l->len, fp ) != l->len ) failed = 1;
		total += l->len;
		if ( i + 1 < E.nlines )
		{
			const char* end = l->eol == 1 ? "\n" : l->eol == 3 ? "\r" : "\r\n";
			size_t len = strlen ( end );
			if ( fwrite ( end, 1, len, fp ) != len ) failed = 1;
			total += len;
		}
	}
	if ( fflush ( fp ) != 0 ) failed = 1;
	if ( fclose ( fp ) != 0 ) failed = 1;
	/* Erst mit_fehler_beenden vollstaendig geschriebene Datei ersetzt das bisherige Ziel. */
	if ( !failed && !MoveFileExA ( temporary, target, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) ) failed = 1;
	if ( failed )
	{
		DeleteFileA ( temporary );
		statusmeldung_setzen ( "Speichern fehlgeschlagen; bisherige Zieldatei bleibt erhalten." ); return 0;
	}
	memmove ( E.filename, path, strlen ( path ) + 1 );
	dateityp_erkennen ( );
	E.saved_revision = E.revision; E.dirty = 0; E.quit_pending = 0;
	statusmeldung_setzen ( "Gespeichert: %s (%zu Bytes)", path, total );
	return 1;
}

/* ---------------------------------------------------------------
 * 52. eingabe_lesen()
 * --------------------------------------------------------------- */
static int eingabe_lesen ( void )
{
	INPUT_RECORD rec; DWORD read;
	for ( ;;)
	{
		if ( !ReadConsoleInputW ( hIn, &rec, 1, &read ) ) mit_fehler_beenden ( "Konsoleneingabe fehlgeschlagen" );
		if ( read == 0 ) continue;
		if ( rec.EventType == WINDOW_BUFFER_SIZE_EVENT ) return KEY_FORCED_REFRESH;

		/* Mauseingaben abfangen */
		if ( rec.EventType == MOUSE_EVENT )
		{
			MOUSE_EVENT_RECORD* me = &rec.Event.MouseEvent;
			CONSOLE_SCREEN_BUFFER_INFO info;
			if ( GetConsoleScreenBufferInfo ( hOut, &info ) )
			{
				/* Mauseingaben verwenden Pufferkoordinaten, der Editor Fensterkoordinaten. */
				me->dwMousePosition.X -= info.srWindow.Left;
				me->dwMousePosition.Y -= info.srWindow.Top;
			}

			if ( me->dwEventFlags == MOUSE_WHEELED )
			{
				int delta = ( short ) HIWORD ( me->dwButtonState );
				if ( delta > 0 ) return KEY_MOUSE_WHEEL_UP;
				else return KEY_MOUSE_WHEEL_DOWN;
			} else if ( me->dwEventFlags == 0 || me->dwEventFlags == DOUBLE_CLICK )
			{
				if ( me->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED )
				{
					E.mouse_x = me->dwMousePosition.X;
					E.mouse_y = me->dwMousePosition.Y;
					return KEY_MOUSE_CLICK;
				}
			} else if ( me->dwEventFlags == MOUSE_MOVED )
			{
				if ( me->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED )
				{
					E.mouse_x = me->dwMousePosition.X;
					E.mouse_y = me->dwMousePosition.Y;
					return KEY_MOUSE_DRAG;
				}
			}
		}

		if ( rec.EventType == KEY_EVENT )
		{
			WORD vk = rec.Event.KeyEvent.wVirtualKeyCode;
			int is_down = rec.Event.KeyEvent.bKeyDown;

			/* Überwachung der STRG-Taste */
			if ( vk == VK_CONTROL )
			{
				if ( E.ctrl_pressed != is_down )
				{
					E.ctrl_pressed = is_down;
					return KEY_FORCED_REFRESH;
				}
			}

			if ( !is_down ) continue;

			DWORD mods = rec.Event.KeyEvent.dwControlKeyState;
			WCHAR wc = rec.Event.KeyEvent.uChar.UnicodeChar;
			int ctrl = ( mods & ( LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED ) ) != 0;
			int shift = ( mods & SHIFT_PRESSED ) != 0;

			if ( vk == VK_ESCAPE ) return KEY_ESC;
			if ( vk == VK_F10 ) return KEY_F10;
			if ( vk == VK_F1 ) return KEY_HELP;
			if ( vk == VK_F3 ) return KEY_CTRL_N;
			if ( ctrl && shift && vk == 'S' ) return KEY_SAVE_AS;
			if ( ctrl && shift && vk == 'A' ) return KEY_SELECT_ALL;
			if ( ctrl && shift && vk == 'N' ) return KEY_NEW;
			if ( ctrl && shift && vk == 'Z' ) return KEY_REDO;

			if ( ctrl && vk == VK_HOME ) return KEY_CTRL_HOME;
			if ( ctrl && vk == VK_END ) return KEY_CTRL_END;

			/* Shift+Navigationstasten für Blockmarkierung */
			if ( shift && !ctrl )
			{
				switch ( vk )
				{
				case VK_UP:    return KEY_SHIFT_UP;
				case VK_DOWN:  return KEY_SHIFT_DOWN;
				case VK_LEFT:  return KEY_SHIFT_LEFT;
				case VK_RIGHT: return KEY_SHIFT_RIGHT;
				case VK_HOME:  return KEY_SHIFT_HOME;
				case VK_END:   return KEY_SHIFT_END;
				case VK_PRIOR: return KEY_SHIFT_PGUP;
				case VK_NEXT:  return KEY_SHIFT_PGDN;
				}
			}

			switch ( vk )
			{
			case VK_UP:    return KEY_UP;
			case VK_DOWN:  return KEY_DOWN;
			case VK_LEFT:  return KEY_LEFT;
			case VK_RIGHT: return KEY_RIGHT;
			case VK_HOME:  return KEY_HOME;
			case VK_END:   return KEY_END;
			case VK_PRIOR: return KEY_PGUP;
			case VK_NEXT:  return KEY_PGDN;
			case VK_DELETE:return KEY_DEL;
			case VK_BACK:  return KEY_BACKSPACE;
			case VK_RETURN:return KEY_ENTER;
			}

			if ( ctrl )
			{
				switch ( vk )
				{
				case 'S': return KEY_CTRL_S;
				case 'Q': return KEY_CTRL_Q;
				case 'O': return KEY_CTRL_O;
				case 'A': return KEY_CTRL_A;
				case 'C': return KEY_CTRL_C;
				case 'X': return KEY_CTRL_X;
				case 'V': return KEY_CTRL_V;
				case 'D': return KEY_CTRL_D;
				case 'K': return KEY_CTRL_K;
				case 'F': return KEY_CTRL_F;
				case 'H': return KEY_CTRL_H;
				case 'N': return KEY_CTRL_N;
				case 'G': return KEY_CTRL_G;
				case 'Z': return KEY_CTRL_Z;
				case 'Y': return KEY_REDO;
				case 'W': return KEY_CTRL_W;
				}
			}
			if ( wc >= 32 && wc < 127 ) return ( int ) wc;
			if ( wc == '\t' ) return '\t';
		}
	}
}

/* ---------------------------------------------------------------
 * 53. eingabedialog_erweitert()
 * --------------------------------------------------------------- */
static int eingabedialog_erweitert ( const char* prompt, const char* hint, char* buf, size_t buflen, int keep, int allow_empty )
{
	if ( !buflen ) return 0;
	if ( !keep ) buf [ 0 ] = '\0';
	buf [ buflen - 1 ] = '\0';
	size_t len = strlen ( buf ), pos = len;
	for ( ;; )
	{
		terminal_groesse_ermitteln ( );
		/* ASCII-Beschriftungen halten Byte- und Bildschirmspalten identisch. */
		size_t label = strlen ( prompt );
		if ( label > ( size_t ) E.screen_cols / 2 ) label = ( size_t ) E.screen_cols / 2;
		/* Eingabe vor dem Hinweis; lange Eingaben scrollen im eigenen Feld. */
		size_t available = ( size_t ) E.screen_cols - label;
		size_t hint_len = strlen ( hint );
		size_t gap = available > 4 ? 2 : 0;
		size_t hint_max = available > gap + 2 ? available - gap - 2 : 0;
		if ( hint_len > hint_max ) hint_len = hint_max;
		size_t width = available - gap - hint_len;
		if ( width < 1 ) width = 1;
		size_t off = pos >= width ? pos - width + 1 : 0;
		size_t visible = len - off;
		if ( visible >= width ) visible = width - 1;
		char seq [ 1024 ]; DWORD n;
		snprintf ( seq, sizeof seq, "\x1b[%d;1H\x1b[m%.*s%.*s%*s%.*s\x1b[K\x1b[%d;%zuH",
			E.screen_rows + 3, ( int ) label, prompt, ( int ) visible, buf + off,
			( int ) gap + 1, "", ( int ) hint_len, hint,
			E.screen_rows + 3, label + pos - off + 1 );
		WriteConsoleA ( hOut, seq, ( DWORD ) strlen ( seq ), &n, NULL );
		int c = eingabe_lesen ( );
		if ( c == KEY_ENTER && ( len || allow_empty ) ) return 1;
		if ( c == KEY_ESC || c == KEY_CTRL_Q ) return 0;
		if ( c == KEY_FORCED_REFRESH ) { anzeige_aktualisieren ( ); continue; }
		if ( c == KEY_LEFT && pos ) pos--;
		else if ( c == KEY_RIGHT && pos < len ) pos++;
		else if ( c == KEY_HOME ) pos = 0;
		else if ( c == KEY_END ) pos = len;
		else if ( c == KEY_BACKSPACE && pos )
		{
			memmove ( buf + pos - 1, buf + pos, len - pos + 1 ); pos--; len--;
		} else if ( c == KEY_DEL && pos < len )
		{
			memmove ( buf + pos, buf + pos + 1, len - pos ); len--;
		} else if ( c >= 32 && c < 127 && len + 1 < buflen )
		{
			memmove ( buf + pos + 1, buf + pos, len - pos + 1 );
			buf [ pos++ ] = ( char ) c; len++;
		}
	}
}

static int eingabedialog ( const char* prompt, const char* hint, char* buf, size_t buflen )
{
	return eingabedialog_erweitert ( prompt, hint, buf, buflen, 0, 0 );
}

static int datei_speichern_unter ( void )
{
	char name [ MAX_PATH ];
	strcpy ( name, E.filename );
	if ( !eingabedialog_erweitert ( "Datei: ", "speichern unter!  (Esc=Abbruch)", name, sizeof name, 1, 0 ) ) return 0;
	if ( _stricmp ( name, E.filename ) && GetFileAttributesA ( name ) != INVALID_FILE_ATTRIBUTES )
	{
		char answer [ 8 ];
		if ( !eingabedialog ( "Antwort: ", "Datei ersetzen? (j/n, Esc=Abbruch)", answer, sizeof answer ) ||
			 ( answer [ 0 ] != 'j' && answer [ 0 ] != 'J' ) ) return 0;
	}
	return datei_an_pfad_speichern ( name );
}

static int datei_speichern ( void )
{
	return E.filename [ 0 ] ? datei_an_pfad_speichern ( E.filename ) : datei_speichern_unter ( );
}

static int verwerfen_bestaetigen ( void )
{
	if ( !E.dirty ) return 1;
	char answer [ 8 ];
	if ( !eingabedialog ( "Antwort: ", "Aenderungen: (s)peichern / (v)erwerfen (Esc=Abbruch)", answer, sizeof answer ) ) return 0;
	if ( answer [ 0 ] == 's' || answer [ 0 ] == 'S' ) return datei_speichern ( );
	return answer [ 0 ] == 'v' || answer [ 0 ] == 'V';
}

static void datei_oeffnen_dialog ( void )
{
	if ( !verwerfen_bestaetigen ( ) ) return;
	char name [ MAX_PATH ];
	if ( eingabedialog ( "Datei: ", "oeffnen!  (Esc=Abbruch)", name, sizeof name ) ) datei_oeffnen ( name );
}

static void dokument_neu_anlegen ( void )
{
	if ( !verwerfen_bestaetigen ( ) ) return;
	editor_puffer_leeren ( ); zeile_einfuegen ( 0, NULL, 0 ); E.filename [ 0 ] = '\0';
	statusmeldung_setzen ( "Neues Dokument." );
}

static void alles_markieren ( void )
{
	E.sel_ay = E.sel_ax = 0;
	E.cy = E.nlines - 1; E.cx = E.lines [ E.cy ].len; E.sel_active = 1;
	E.goal_valid = 0;
}

/* ---------------------------------------------------------------
 * 54. text_suchen()
 * --------------------------------------------------------------- */
static void text_suchen ( int find_next )
{
	if ( !find_next )
	{
		char query [ 64 ];
		if ( !eingabedialog ( "Text: ", "suchen!  (Esc=Abbruch)", query, sizeof ( query ) ) )
		{
			return;
		}
		strncpy ( E.last_search, query, sizeof ( E.last_search ) - 1 );
		E.last_match_row = -1;
		E.last_match_col = -1;
	}

	if ( E.last_search [ 0 ] == '\0' )
	{
		statusmeldung_setzen ( "Kein Suchbegriff definiert." );
		E.status_duration = 1500;
		return;
	}

	size_t start_row = E.cy;
	size_t start_col = E.cx + ( find_next ? 1 : 0 );

	/* Phase 1: Von aktueller Position bis Ende suchen */
	if ( bereich_durchsuchen ( ( size_t ) start_row, start_col, E.nlines - 1 ) )
	{
		statusmeldung_setzen ( "Treffer bei Zeile %zu, Spalte %zu", E.cy + 1, E.cx + 1 );
		E.status_duration = 1500;
		return;
	}

	/* Phase 2: Wrap-Around – vom Anfang bis zur Startposition suchen */
	if ( start_row > 0 || start_col > 0 )
	{
		if ( bereich_durchsuchen ( 0, 0, ( size_t ) start_row ) )
		{
			statusmeldung_setzen ( "Wrap-Around: Treffer bei Zeile %zu, Spalte %zu", E.cy + 1, E.cx + 1 );
			E.status_duration = 1500;
			return;
		}
	}

	E.last_match_row = -1;
	E.last_match_col = -1;
	statusmeldung_setzen ( "Suchbegriff '%s' nicht gefunden.", E.last_search );
	E.status_duration = 1500;
}

/* ---------------------------------------------------------------
 * 55. text_suchen_ersetzen()
 * --------------------------------------------------------------- */
static void text_suchen_ersetzen ( void )
{
	char query [ 64 ], replacement [ 64 ];
	if ( !eingabedialog ( "Text: ", "suchen zum Ersetzen!  (Esc=Abbruch)", query, sizeof ( query ) ) ) return;
	if ( !eingabedialog_erweitert ( "Ersatz: ", "ersetzen durch!  (auch leer, Esc=Abbruch)", replacement, sizeof replacement, 0, 1 ) ) return;

	strncpy ( E.last_search, query, sizeof ( E.last_search ) - 1 );
	size_t qlen = strlen ( query );
	size_t rlen = strlen ( replacement );
	int count = 0;

	for ( size_t i = 0; i < E.nlines; i++ )
	{
		Line* l = &E.lines [ i ];
		size_t j = 0;
		while ( j + qlen <= l->len )
		{
			if ( memcmp ( l->chars + j, query, qlen ) == 0 )
			{
				if ( !count ) rueckgaengig_zustand_sichern ( );
				if ( rlen > qlen )
				{
					zeile_speicher_sicherstellen ( l, l->len + ( rlen - qlen ) );
					memmove ( l->chars + j + rlen, l->chars + j + qlen, l->len - j - qlen );
					l->len += ( rlen - qlen );
				} else if ( rlen < qlen )
				{
					memmove ( l->chars + j + rlen, l->chars + j + qlen, l->len - j - qlen );
					l->len -= ( qlen - rlen );
				}
				memcpy ( l->chars + j, replacement, rlen );
				j += rlen;
				count++;
			} else { j++; }
		}
	}

	if ( count > 0 )
	{
		E.dirty = 1; E.sel_active = 0;
		if ( E.cx > E.lines [ E.cy ].len ) E.cx = E.lines [ E.cy ].len;
	}
	statusmeldung_setzen ( "%d Ersetzung(en) durchgeführt.", count );
	E.status_duration = 2000;
}

/* ---------------------------------------------------------------
 * 56. zu_zeile_springen()
 * --------------------------------------------------------------- */
static void zu_zeile_springen ( void )
{
	char numbuf [ 32 ];
	if ( eingabedialog ( "Zeile: ", "anspringen!  (Esc=Abbruch)", numbuf, sizeof ( numbuf ) ) )
	{
		char* end;
		errno = 0;
		unsigned long long target = strtoull ( numbuf, &end, 10 );
		if ( errno || *end || numbuf [ 0 ] == '-' || !target || target > E.nlines )
		{
			statusmeldung_setzen ( "Ungültige Zeilennummer!" );
			E.status_duration = 1500;
			return;
		}
		size_t tgt = ( size_t ) target - 1;
		if ( tgt >= E.nlines )
		{
			E.cy = E.nlines - 1;
		} else { E.cy = tgt; }
		E.cx = 0; E.sel_active = 0;
		statusmeldung_setzen ( "Sprung zu Zeile %llu", target );
		E.status_duration = 1500;
	}
}

/* ---------------------------------------------------------------
 * 57. cursor_bewegen()
 * --------------------------------------------------------------- */
static void cursor_bewegen ( int k )
{
	Line* row = ( E.cy < E.nlines ) ? &E.lines [ E.cy ] : NULL;
	int vertical = k == KEY_UP || k == KEY_DOWN || k == KEY_PGUP || k == KEY_PGDN ||
		k == KEY_SHIFT_UP || k == KEY_SHIFT_DOWN || k == KEY_SHIFT_PGUP || k == KEY_SHIFT_PGDN;
	if ( vertical && !E.goal_valid )
	{
		E.goal_rx = row ? zeichenindex_zu_bildschirmspalte ( row, E.cx ) : 0;
		if ( row && E.word_wrap ) E.goal_rx = segment_zeichenindex_zu_bildschirmspalte ( row, E.vlines [ bildschirmzeile_finden ( E.cy, E.cx ) ].char_start, E.cx );
		E.goal_valid = 1;
	} else if ( !vertical ) E.goal_valid = 0;

	if ( E.word_wrap && ( k == KEY_UP || k == KEY_SHIFT_UP ||
						  k == KEY_DOWN || k == KEY_SHIFT_DOWN ||
						  k == KEY_PGUP || k == KEY_SHIFT_PGUP ||
						  k == KEY_PGDN || k == KEY_SHIFT_PGDN ) )
	{
		size_t cur_vl = bildschirmzeile_finden ( E.cy, E.cx );
		size_t target_vl = cur_vl;
		if ( k == KEY_UP || k == KEY_SHIFT_UP )
		{
			if ( cur_vl > 0 ) target_vl = cur_vl - 1;
		} else if ( k == KEY_DOWN || k == KEY_SHIFT_DOWN )
		{
			if ( cur_vl + 1 < E.nvlines ) target_vl = cur_vl + 1;
		} else if ( k == KEY_PGUP || k == KEY_SHIFT_PGUP )
		{
			target_vl = ( cur_vl > ( size_t ) E.screen_rows ) ? cur_vl - E.screen_rows : 0;
		} else if ( k == KEY_PGDN || k == KEY_SHIFT_PGDN )
		{
			target_vl = cur_vl + E.screen_rows;
			if ( target_vl >= E.nvlines ) target_vl = E.nvlines - 1;
		}

		if ( target_vl != cur_vl )
		{
			E.cy = E.vlines [ target_vl ].logical_row;
			E.cx = segment_bildschirmspalte_zu_zeichenindex ( &E.lines [ E.cy ], E.vlines [ target_vl ].char_start, E.vlines [ target_vl ].char_len, E.goal_rx );
		}
		return;
	}

	switch ( k )
	{
	case KEY_LEFT:

	case KEY_SHIFT_LEFT:
	if ( E.cx > 0 ) E.cx--;
	else if ( E.cy > 0 ) { E.cy--; E.cx = E.lines [ E.cy ].len; }
	break;
	case KEY_RIGHT:

	case KEY_SHIFT_RIGHT:
	if ( row && E.cx < row->len ) E.cx++;
	else if ( row && E.cy + 1 < E.nlines ) { E.cy++; E.cx = 0; }
	break;

	case KEY_UP:

	case KEY_SHIFT_UP:
	if ( E.cy > 0 ) E.cy--;
	break;

	case KEY_DOWN:

	case KEY_SHIFT_DOWN:
	if ( E.cy + 1 < E.nlines ) E.cy++;
	break;

	case KEY_HOME:

	case KEY_SHIFT_HOME:
	E.cx = 0;
	break;

	case KEY_END:

	case KEY_SHIFT_END:
	if ( E.cy < E.nlines ) E.cx = E.lines [ E.cy ].len;
	break;
	case KEY_PGUP:

	case KEY_SHIFT_PGUP:
	E.cy = ( E.cy > ( size_t ) E.screen_rows ) ? E.cy - E.screen_rows : 0;
	break;

	case KEY_PGDN:

	case KEY_SHIFT_PGDN:
	E.cy += E.screen_rows;
	if ( E.cy >= E.nlines ) E.cy = E.nlines ? E.nlines - 1 : 0;
	break;

	case KEY_CTRL_HOME:
	E.cy = 0;
	E.cx = 0;
	break;

	case KEY_CTRL_END:
	if ( E.nlines > 0 ) { E.cy = E.nlines - 1;	E.cx = E.lines [ E.cy ].len; }break;
	}
	row = ( E.cy < E.nlines ) ? &E.lines [ E.cy ] : NULL;
	if ( vertical && row ) E.cx = segment_bildschirmspalte_zu_zeichenindex ( row, 0, row->len, E.goal_rx );
	size_t rl = row ? row->len : 0;
	if ( E.cx > rl ) E.cx = rl;
}

/* ---------------------------------------------------------------
 * 58. mausklick_verarbeiten()
 * --------------------------------------------------------------- */
static void mausklick_verarbeiten ( int x, int y )
{
	/* Bildschirmzeile 0 ist jetzt ausschliesslich mit_fehler_beenden Menueleiste. */
	if ( y <= 0 ) return;

	/* Editor belegt mit_fehler_beenden Bildschirmzeilen 1 bis E.screen_rows.
	   Alles darunter sind mit_fehler_beenden beiden Statuszeilen. */
	if ( y > E.screen_rows ) return;
	E.goal_valid = 0;

	int editor_y = y - 1;
	int text_x = x - E.gutter_width;
	if ( text_x < 0 ) text_x = 0; /* Klick in mit_fehler_beenden Zeilennummern fangen wir sicher ab */

	if ( E.word_wrap )
	{
		size_t vline_idx = E.rowoff + ( size_t ) editor_y;
		if ( vline_idx >= E.nvlines )
		{
			E.cy = E.nlines > 0 ? E.nlines - 1 : 0;
			E.cx = E.nlines > 0 ? E.lines [ E.cy ].len : 0;
		} else
		{
			VLine* vl = &E.vlines [ vline_idx ];
			E.cy = vl->logical_row;
			Line* l = &E.lines [ E.cy ];
			E.cx = segment_bildschirmspalte_zu_zeichenindex ( l, vl->char_start, vl->char_len, ( size_t ) text_x );
		}
	} else
	{
		size_t filerow = E.rowoff + ( size_t ) editor_y;
		if ( filerow >= E.nlines )
		{
			E.cy = E.nlines > 0 ? E.nlines - 1 : 0;
			E.cx = E.nlines > 0 ? E.lines [ E.cy ].len : 0;
		} else
		{
			E.cy = filerow;
			Line* l = &E.lines [ E.cy ];
			size_t target_rx = E.coloff + ( size_t ) text_x;

			size_t rx = 0;
			size_t cx = 0;
			for ( ; cx < l->len; cx++ )
			{
				if ( l->chars [ cx ] == '\t' ) rx += TAB_STOP - ( rx % TAB_STOP );
				else rx++;
				if ( rx > target_rx ) break;
			}
			E.cx = cx;
		}
	}
}

/* ---------------------------------------------------------------
 * 59. ist_markierungstaste()
 * --------------------------------------------------------------- */
static int ist_markierungstaste ( int k ) { return k >= KEY_SHIFT_UP && k <= KEY_SHIFT_PGDN; }

/* ---------------------------------------------------------------
 * 60. ist_navigationstaste()
 * --------------------------------------------------------------- */
static int ist_navigationstaste ( int k ) { return ( k >= KEY_UP && k <= KEY_PGDN ) || k == KEY_CTRL_HOME || k == KEY_CTRL_END; }

/* ---------------------------------------------------------------
 * 61. menue_text_an_position_schreiben()
 * --------------------------------------------------------------- */
static void menue_text_an_position_schreiben ( int x, int y, const char* text, int selected )
{
	char seq [ 64 ];
	snprintf ( seq, sizeof seq, "\x1b[%d;%dH", y + 1, x + 1 );
	DWORD n;
	WriteConsoleA ( hOut, seq, ( DWORD ) strlen ( seq ), &n, NULL );
	if ( selected ) WriteConsoleA ( hOut, "\x1b[7m", 4, &n, NULL );
	WriteConsoleA ( hOut, text, ( DWORD ) strlen ( text ), &n, NULL );
	if ( selected ) WriteConsoleA ( hOut, "\x1b[27m", 5, &n, NULL );
}

/* ---------------------------------------------------------------
 * 62. menue_text_an_position_invers_schreiben()
 * --------------------------------------------------------------- */
static void menue_text_an_position_invers_schreiben ( int x, int y, const char* text )
{
	char seq [ 64 ];
	snprintf ( seq, sizeof seq, "\x1b[%d;%dH", y + 1, x + 1 );
	DWORD n;
	WriteConsoleA ( hOut, seq, ( DWORD ) strlen ( seq ), &n, NULL );
	WriteConsoleA ( hOut, "\x1b[7m", 4, &n, NULL );
	WriteConsoleA ( hOut, text, ( DWORD ) strlen ( text ), &n, NULL );
	WriteConsoleA ( hOut, "\x1b[27m", 5, &n, NULL );
}

/* ---------------------------------------------------------------
 * 63. menue_rahmen_zeichnen()
 * --------------------------------------------------------------- */
static void menue_rahmen_zeichnen ( int x, int y, int width, int height )
{
	char buf [ 128 ];

	menue_text_an_position_schreiben ( x, y, "┌", 0 );
	menue_text_an_position_schreiben ( x + width - 1, y, "┐", 0 );
	menue_text_an_position_schreiben ( x, y + height - 1, "└", 0 );
	menue_text_an_position_schreiben ( x + width - 1, y + height - 1, "┘", 0 );

	for ( int i = 1; i < width - 1; i++ )
	{
		menue_text_an_position_schreiben ( x + i, y, "─", 0 );
		menue_text_an_position_schreiben ( x + i, y + height - 1, "─", 0 );
	}

	for ( int j = 1; j < height - 1; j++ )
	{
		menue_text_an_position_schreiben ( x, y + j, "│", 0 );
		menue_text_an_position_schreiben ( x + width - 1, y + j, "│", 0 );

		int inner_width = width - 2;
		if ( inner_width > 0 && inner_width < ( int ) sizeof ( buf ) - 1 )
		{
			memset ( buf, ' ', inner_width );
			buf [ inner_width ] = '\0';
			menue_text_an_position_schreiben ( x + 1, y + j, buf, 0 );
		}
	}
}

/* ---------------------------------------------------------------
 * 64. menue_rahmen_invers_zeichnen()
 * --------------------------------------------------------------- */
static void menue_rahmen_invers_zeichnen ( int x, int y, int width, int height )
{
	char buf [ 128 ];

	menue_text_an_position_invers_schreiben ( x, y, "┌" );
	menue_text_an_position_invers_schreiben ( x + width - 1, y, "┐" );
	menue_text_an_position_invers_schreiben ( x, y + height - 1, "└" );
	menue_text_an_position_invers_schreiben ( x + width - 1, y + height - 1, "┘" );

	for ( int i = 1; i < width - 1; i++ )
	{
		menue_text_an_position_invers_schreiben ( x + i, y, "─" );
		menue_text_an_position_invers_schreiben ( x + i, y + height - 1, "─" );
	}

	for ( int j = 1; j < height - 1; j++ )
	{
		menue_text_an_position_invers_schreiben ( x, y + j, "│" );
		menue_text_an_position_invers_schreiben ( x + width - 1, y + j, "│" );

		int inner_width = width - 2;
		if ( inner_width > 0 && inner_width < ( int ) sizeof ( buf ) - 1 )
		{
			memset ( buf, ' ', inner_width );
			buf [ inner_width ] = '\0';
			menue_text_an_position_invers_schreiben ( x + 1, y + j, buf );
		}
	}
}

/* ---------------------------------------------------------------
 * 65. meldungsfenster_anzeigen()
 * --------------------------------------------------------------- */
static void meldungsfenster_anzeigen ( const char* title, const char* text1, const char* text2 )
{
	int width = 50;
	int height = 7;
	int x = ( E.screen_cols - width ) / 2;
	int y = ( E.screen_rows - height ) / 2;

	// 1. Box zeichnen
	menue_rahmen_zeichnen ( x, y, width, height );

	// 2. Titel zentriert oben in mit_fehler_beenden Box schreiben
	menue_text_an_position_schreiben ( x + ( width - ( int ) strlen ( title ) ) / 2, y, title, 1 );

	// 3. Textzeilen in mit_fehler_beenden Box schreiben
	menue_text_an_position_schreiben ( x + 3, y + 2, text1, 0 );
	if ( text2 && text2 [ 0 ] != '\0' ) { menue_text_an_position_schreiben ( x + 3, y + 3, text2, 0 ); }

	// 4. Schließen-Hinweis unten zentriert
	const char* footer = " [ Enter / Esc ] ";
	menue_text_an_position_schreiben ( x + ( width - ( int ) strlen ( footer ) ) / 2, y + height - 1, footer, 0 );

	// 5. Warten, bis der Benutzer Enter oder Esc drückt
	for ( ;;) { int k = eingabe_lesen ( );	if ( k == KEY_ENTER || k == KEY_ESC || k == KEY_CTRL_Q ) { break; } }
}

/* ---------------------------------------------------------------
 * 66. menue_zeichnen()
 * --------------------------------------------------------------- */
static void menue_zeichnen ( int active_main, int active_sub )
{
	if ( active_main < 0 || active_main >= menu_main_count ) return;
	char seq [ 128 ];
	DWORD n;

	/* Erst mit_fehler_beenden komplette normale Oberflaeche zeichnen. Dadurch bleibt mit_fehler_beenden
	   feste Menueleiste/Editor/Status-Aufteilung auch im F10-Modus korrekt. */
	anzeige_aktualisieren ( );

	// 1. Oberste Menüleiste zeichnen
	// Zuerst mit_fehler_beenden komplette Zeile im normalen Stil loeschen.
	snprintf ( seq, sizeof seq, "\x1b[1;1H\x1b[m\x1b[K" );
	WriteConsoleA ( hOut, seq, ( DWORD ) strlen ( seq ), &n, NULL );

	// Der helle Menuebalken beginnt erst in Spalte 5.
	snprintf ( seq, sizeof seq, "\x1b[1;5H\x1b[47m\x1b[30m\x1b[K" );
	WriteConsoleA ( hOut, seq, ( DWORD ) strlen ( seq ), &n, NULL );

	int current_x = 5;
	for ( int i = 0; i < menu_main_count; i++ )
	{
		snprintf ( seq, sizeof seq, "\x1b[1;%dH", current_x );
		WriteConsoleA ( hOut, seq, ( DWORD ) strlen ( seq ), &n, NULL );

		if ( i == active_main )
		{
			WriteConsoleA ( hOut, "\x1b[7m", 4, &n, NULL ); // Invers für aktiven Hauptpunkt
		}
		WriteConsoleA ( hOut, menu_main [ i ], ( DWORD ) strlen ( menu_main [ i ] ), &n, NULL );
		if ( i == active_main )
		{
			WriteConsoleA ( hOut, "\x1b[27m", 5, &n, NULL );
		}
		current_x += ( int ) strlen ( menu_main [ i ] ) + 4;
	}
	WriteConsoleA ( hOut, "\x1b[m", 3, &n, NULL );

	// 2. Untermenü-Box berechnen und öffnen
	int box_x = 4;
	for ( int i = 0; i < active_main; i++ )
	{
		box_x += ( int ) strlen ( menu_main [ i ] ) + 4;
	}
	int box_y = 1;
	int max_len = 5;
	for ( int i = 0; i < menu_sub_count [ active_main ]; i++ )
	{
		int l = ( int ) strlen ( menu_sub [ active_main ][ i ] );
		if ( l > max_len ) max_len = l;
	}
	int box_w = max_len + 4;
	int box_h = menu_sub_count [ active_main ] + 2;

	// Saubere Box zeichnen
	menue_rahmen_invers_zeichnen ( box_x, box_y, box_w, box_h );

	// Einträge im Untermenü zeichnen
	for ( int i = 0; i < menu_sub_count [ active_main ]; i++ )
	{
		char item_buf [ 128 ];
		int inner_w = box_w - 2;
		snprintf ( item_buf, sizeof item_buf, " %-*s ", inner_w - 2, menu_sub [ active_main ][ i ] );

		if ( i == active_sub )
		{
			/* Aktiver Punkt normal auf hellem Hintergrund, damit er sich vom
			   inversen Pulldown-Fenster sichtbar abhebt. */
			menue_text_an_position_schreiben ( box_x + 1, box_y + 1 + i, item_buf, 0 );
		} else
		{
			menue_text_an_position_invers_schreiben ( box_x + 1, box_y + 1 + i, item_buf );
		}
	}
}

/* ---------------------------------------------------------------
 * 67. menue_aktion_ausfuehren()
 * --------------------------------------------------------------- */
static void menue_aktion_ausfuehren ( int active_main, int active_sub, int* quit )
{
	if ( active_main < 0 || active_main >= menu_main_count ||
		 active_sub < 0 || active_sub >= menu_sub_count [ active_main ] ) return;
	if ( active_main == 0 )
	{
		switch ( active_sub )
		{
		case 0: dokument_neu_anlegen ( ); break;
		case 1: datei_oeffnen_dialog ( ); break;
		case 2: datei_speichern ( ); break;
		case 3: datei_speichern_unter ( ); break;
		case 4: if ( verwerfen_bestaetigen ( ) ) *quit = 1; break;
		}
	} else if ( active_main == 1 )
	{
		switch ( active_sub )
		{
		case 0: rueckgaengig_machen ( ); break;
		case 1: if ( E.sel_active ) markierung_kopieren ( ); else aktuelle_zeile_kopieren ( ); break;
		case 2: if ( E.sel_active ) markierung_ausschneiden ( ); else aktuelle_zeile_ausschneiden ( ); break;
		case 3: zwischenablage_einfuegen ( ); break;
		case 4: aktuelle_zeile_duplizieren ( ); break;
		case 5: aktuelle_zeile_loeschen ( ); break;
		case 6: E.word_wrap = !E.word_wrap; break;
		case 7: verlaufszustand_wiederherstellen ( 1 ); break;
		case 8: alles_markieren ( ); break;
		}
	} else if ( active_main == 2 )
	{
		if ( active_sub == 0 ) text_suchen ( 0 );
		else if ( active_sub == 1 ) text_suchen ( 1 );
		else if ( active_sub == 2 ) text_suchen_ersetzen ( );
		else if ( active_sub == 3 ) zu_zeile_springen ( );
	} else if ( active_main == 3 )
	{
		if ( active_sub == 0 ) meldungsfenster_anzeigen ( " Tastenkombinationen ",
			"F3: Weitersuchen   Strg+Y: Wiederholen", "Strg+Umschalt+A: Alles markieren" );
		else meldungsfenster_anzeigen ( " Ueber DOS-Edit ", "doseditor - Windows-Konsoleneditor", "Entwickelt von Ercan Dogan" );
	}
}

/* ---------------------------------------------------------------
 * 68. menue_oeffnen()
 * --------------------------------------------------------------- */
static int hauptmenue_an_position ( int x )
{
	int left = 4;
	for ( int i = 0; i < menu_main_count; i++ )
	{
		int right = left + ( int ) strlen ( menu_main [ i ] ) + 4;
		if ( x >= left && x < right ) return i;
		left = right;
	}
	return -1;
}

static void menue_oeffnen ( int* quit, int active_main )
{
	if ( active_main < 0 || active_main >= menu_main_count ) return;
	int active_sub = 0;
	DWORD n;

	// 1. Cursor beim Öffnen des Menüs verstecken
	WriteConsoleA ( hOut, "\x1b[?25l", 6, &n, NULL );

	for ( ;;)
	{
		if ( active_main < 0 || active_main >= menu_main_count ) return;
		menue_zeichnen ( active_main, active_sub );
		WriteConsoleA ( hOut, "\x1b[?25l", 6, &n, NULL );
		int k = eingabe_lesen ( );
		if ( k == KEY_FORCED_REFRESH ) continue;
		if ( k == KEY_MOUSE_CLICK )
		{
			int hit = hauptmenue_an_position ( E.mouse_x );
			if ( E.mouse_y == 0 && hit >= 0 && hit < menu_main_count ) { active_main = hit; active_sub = 0; continue; }
			int left = 4, width = 5;
			for ( int i = 0; i < active_main; i++ ) left += ( int ) strlen ( menu_main [ i ] ) + 4;
			for ( int i = 0; i < menu_sub_count [ active_main ]; i++ )
			{
				int len = ( int ) strlen ( menu_sub [ active_main ][ i ] );
				if ( len > width ) width = len;
			}
			if ( E.mouse_x > left && E.mouse_x < left + width + 3 &&
				 E.mouse_y >= 2 && E.mouse_y < 2 + menu_sub_count [ active_main ] )
			{
				active_sub = E.mouse_y - 2; k = KEY_ENTER;
			} else k = KEY_ESC;
		}

		if ( k == KEY_ESC || k == KEY_CTRL_Q || k == KEY_F10 )
		{
			// 2. Cursor vor dem Verlassen wieder einschalten!
			WriteConsoleA ( hOut, "\x1b[?25h", 6, &n, NULL );
			return;
		}

		if ( k == KEY_LEFT )
		{
			active_main = ( active_main - 1 + menu_main_count ) % menu_main_count;
			active_sub = 0;
		} else if ( k == KEY_RIGHT )
		{
			active_main = ( active_main + 1 ) % menu_main_count;
			active_sub = 0;
		} else if ( k == KEY_UP )
		{
			active_sub = ( active_sub - 1 + menu_sub_count [ active_main ] ) % menu_sub_count [ active_main ];
		} else if ( k == KEY_DOWN )
		{
			active_sub = ( active_sub + 1 ) % menu_sub_count [ active_main ];
		} else if ( k == KEY_ENTER )
		{
			/* Auch mit_fehler_beenden per Maus gewaehlte Zeile vor der Aktion markieren. */
			menue_zeichnen ( active_main, active_sub );
			// 3. Cursor vor der Aktionsausführung wieder einschalten!
			WriteConsoleA ( hOut, "\x1b[?25h", 6, &n, NULL );
			menue_aktion_ausfuehren ( active_main, active_sub, quit );
			return;
		}
	}
}

/* ---------------------------------------------------------------
 * 69. benutzereingabe_verarbeiten()
 * --------------------------------------------------------------- */
static void taste_verarbeiten ( int k, int* quit )
{
	if ( k == KEY_NONE || k == KEY_FORCED_REFRESH ) return;
	if ( k != KEY_CTRL_Q ) E.quit_pending = 0;
	if ( k == KEY_HELP ) { menue_aktion_ausfuehren ( 3, 0, quit ); return; }

	if ( k == KEY_F10 ) { E.goal_valid = 0; menue_oeffnen ( quit, 0 ); return; }
	if ( k == KEY_MOUSE_CLICK && E.mouse_y == 0 )
	{
		int hit = hauptmenue_an_position ( E.mouse_x );
		if ( hit >= 0 ) menue_oeffnen ( quit, hit );
		return;
	}

	/* Maus-Ereignisse verarbeiten, bevor mit_fehler_beenden Textlogik startet */
	if ( k == KEY_MOUSE_WHEEL_UP ) { for ( int i = 0; i < 3; i++ ) cursor_bewegen ( KEY_UP );	return; }
	if ( k == KEY_MOUSE_WHEEL_DOWN ) { for ( int i = 0; i < 3; i++ ) cursor_bewegen ( KEY_DOWN );	return; }
	if ( k == KEY_MOUSE_CLICK ) { E.sel_active = 0;	mausklick_verarbeiten ( E.mouse_x, E.mouse_y );	return; }
	if ( k == KEY_MOUSE_DRAG )
	{
		if ( !E.sel_active ) { E.sel_active = 1;		E.sel_ax = E.cx;		E.sel_ay = E.cy; }
		mausklick_verarbeiten ( E.mouse_x, E.mouse_y );
		return;
	}

	/* Tastatur Selektion verwalten */
	if ( ist_markierungstaste ( k ) )
	{
		if ( !E.sel_active ) { E.sel_active = 1;	E.sel_ax = E.cx;	E.sel_ay = E.cy; }	cursor_bewegen ( k );	return;
	}
	if ( ist_navigationstaste ( k ) ) { E.sel_active = 0;	cursor_bewegen ( k );	return; }

	/* Bei Texteingabe mit aktiver Selektion: zuerst löschen */
	if ( E.sel_active && ( ( k >= 32 && k < 127 ) || k == '\t' || k == KEY_ENTER ) ) { markierung_loeschen ( ); }

	/* Bei Backspace/DEL mit aktiver Selektion: nur löschen */
	if ( E.sel_active && ( k == KEY_BACKSPACE || k == KEY_DEL ) ) { markierung_loeschen ( );	return; }

	switch ( k )
	{

	case KEY_NONE: break;
	case KEY_FORCED_REFRESH: break;

	case KEY_ESC:
	E.sel_active = 0;
	break;

	case KEY_CTRL_Q:
	if ( E.dirty && !E.quit_pending )
	{
		statusmeldung_setzen ( "Ungespeicherte Änderungen! Nochmal Strg+Q zum Beenden." );
		E.quit_pending = 1; break;
	}
	*quit = 1; break;

	case KEY_CTRL_S: datei_speichern ( ); break;

	case KEY_CTRL_A:
	case KEY_SAVE_AS: datei_speichern_unter ( ); break;
	case KEY_SELECT_ALL: alles_markieren ( ); break;
	case KEY_NEW: dokument_neu_anlegen ( ); break;
	case KEY_REDO: verlaufszustand_wiederherstellen ( 1 ); break;
	case KEY_CTRL_O: datei_oeffnen_dialog ( ); break;

	case KEY_CTRL_C:
	if ( E.sel_active )
	{
		markierung_kopieren ( );
	} else
	{
		aktuelle_zeile_kopieren ( );
	}
	break;

	case KEY_CTRL_X:
	if ( E.sel_active ) { markierung_ausschneiden ( ); } else { aktuelle_zeile_ausschneiden ( ); }
	break;

	case KEY_CTRL_V:
	zwischenablage_einfuegen ( );
	break;

	case KEY_CTRL_D: aktuelle_zeile_duplizieren ( ); break;
	case KEY_CTRL_K: aktuelle_zeile_loeschen ( ); break;
	case KEY_CTRL_F: text_suchen ( 0 ); break;
	case KEY_CTRL_H: text_suchen_ersetzen ( ); break;
	case KEY_CTRL_N: text_suchen ( 1 ); break;
	case KEY_CTRL_G: zu_zeile_springen ( ); break;
	case KEY_CTRL_Z: rueckgaengig_machen ( ); break;

	case KEY_CTRL_W:
	E.word_wrap = !E.word_wrap;
	statusmeldung_setzen ( "Zeilenumbruch (Word-Wrap) %s.", E.word_wrap ? "AN" : "AUS" );
	E.status_duration = 1500;
	break;

	case KEY_ENTER:     zeilenumbruch_einfuegen ( ); break;
	case KEY_BACKSPACE: zeichen_links_loeschen ( ); break;

	case KEY_DEL:
	if ( E.cy < E.nlines )
	{
		Line* cur = &E.lines [ E.cy ];
		if ( E.cx < cur->len ) { rueckgaengig_zustand_sichern ( );	zeile_zeichen_loeschen ( cur, E.cx );	E.dirty = 1; } else if ( E.cy + 1 < E.nlines )
		{
			rueckgaengig_zustand_sichern ( );
			zeile_text_anhaengen ( cur, E.lines [ E.cy + 1 ].chars, E.lines [ E.cy + 1 ].len );
			cur->eol = E.lines [ E.cy + 1 ].eol;
			zeile_entfernen ( E.cy + 1 );
			E.dirty = 1;
		}
	}
	break;

	default:
	if ( k >= 32 && k < 127 ) zeichen_einfuegen ( ( char ) k );
	else if ( k == '\t' )     zeichen_einfuegen ( '\t' );
	break;
	}

	/* Zeilenaktionen koennen einen bisherigen Markierungsanker ungueltig machen. */
	if ( k == KEY_CTRL_D || k == KEY_CTRL_K ) E.sel_active = 0;
}

/* Eine Benutzereingabe bildet genau einen Rueckgaengig-Schritt. */
static void benutzereingabe_verarbeiten ( int* quit )
{
	int k = eingabe_lesen ( );
	E.undo_group = 1;
	taste_verarbeiten ( k, quit );
	E.undo_group = 0;
}

/* ---------------------------------------------------------------
 * 70. main() - Hauptprogramm (C-Programmeinstieg)
 * --------------------------------------------------------------- */
int main ( int argc, char** argv )
{
	memset ( &E, 0, sizeof E );
	terminal_steuerung_aktivieren ( );

	DWORD n;
	const char* init_seq = "\x1b[?1049h\x1b[H";
	WriteConsoleA ( hOut, init_seq, ( DWORD ) strlen ( init_seq ), &n, NULL );
	atexit ( terminal_wiederherstellen );
	terminal_groesse_ermitteln ( );

	editor_puffer_leeren ( );
	zeile_einfuegen ( 0, NULL, 0 );
	if ( argc >= 2 )
	{
		DWORD attributes = GetFileAttributesA ( argv [ 1 ] );
		DWORD error = GetLastError ( );
		if ( attributes == INVALID_FILE_ATTRIBUTES && error == ERROR_FILE_NOT_FOUND && strlen ( argv [ 1 ] ) < MAX_PATH )
		{
			strcpy ( E.filename, argv [ 1 ] ); dateityp_erkennen ( );
			statusmeldung_setzen ( "Neue Datei: %s", E.filename );
		} else datei_oeffnen ( argv [ 1 ] );
	}

	int quit = 0;
	while ( !quit ) { anzeige_aktualisieren ( );	benutzereingabe_verarbeiten ( &quit ); }

	editor_puffer_leeren ( );
	free ( E.clipboard );

	return 0;
}

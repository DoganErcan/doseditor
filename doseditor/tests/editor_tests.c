/* Regressionstests fuer Textmodell, Bedienbefehle und sichere Dateioperationen. */
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <string.h>

/* Dialoge erhalten reproduzierbare Konsolenereignisse ohne ein sichtbares Fenster. */
static INPUT_RECORD events[ 1024 ];
static size_t event_count, event_pos;

static BOOL WINAPI test_read( HANDLE handle, PINPUT_RECORD records, DWORD length, LPDWORD read ) {
	(void) handle; (void) length;
	assert( event_pos < event_count );
	*records = events[ event_pos++ ]; *read = 1; return TRUE;
}

static BOOL WINAPI test_size( HANDLE handle, PCONSOLE_SCREEN_BUFFER_INFO info ) {
	(void) handle;
	memset( info, 0, sizeof *info );
	info->srWindow.Right = 79; info->srWindow.Bottom = 24;
	return TRUE;
}

static BOOL WINAPI test_write( HANDLE handle, const VOID* text, DWORD length, LPDWORD written, LPVOID reserved ) {
	(void) handle; (void) text; (void) reserved;
	*written = length; return TRUE;
}

#define ReadConsoleInputW test_read
#define GetConsoleScreenBufferInfo test_size
#define WriteConsoleA test_write
#define main doseditor_main
#include "../doseditor.c"
#undef main
#include <assert.h>

static void reset_text( const char* text ) {
	ed_clear_buffer( );
	E.filename[ 0 ] = '\0';
	E.undo_group = 0;
	insert_line( 0, text, strlen( text ) );
}

static void key( int k ) {
	int quit = 0;
	E.undo_group = 1;
	ed_handle_key( k, &quit );
	E.undo_group = 0;
	assert( !quit );
}

static void expect_line( size_t row, const char* text ) {
	assert( row < E.nlines );
	assert( E.lines[ row ].len == strlen( text ) );
	assert( !E.lines[ row ].len || !memcmp( E.lines[ row ].chars, text, E.lines[ row ].len ) );
}

static void test_history( void ) {
	reset_text( "" );
	key( 'a' ); key( 'b' ); key( 'c' );
	key( KEY_CTRL_Z ); expect_line( 0, "ab" );
	key( KEY_CTRL_Z ); expect_line( 0, "a" );
	key( KEY_CTRL_Z ); expect_line( 0, "" ); assert( !E.dirty );
	key( KEY_REDO ); key( KEY_REDO ); key( KEY_REDO ); expect_line( 0, "abc" );
	key( KEY_CTRL_Z ); key( 'x' ); assert( E.redo_count == 0 );
	expect_line( 0, "abx" );
	for ( int i = 0; i < 40; i++ ) key( 'z' );
	assert( E.undo_count == UNDO_LIMIT );
	for ( int i = 0; i < UNDO_LIMIT; i++ ) key( KEY_CTRL_Z );
	assert( E.lines[ 0 ].len == 11 );
}

static void test_selection( void ) {
	reset_text( "abcdef" );
	E.sel_active = 1; E.sel_ax = 1; E.sel_ay = 0; E.cx = 4;
	key( 'X' ); expect_line( 0, "aXef" );
	assert( E.undo_count == 1 );
	key( KEY_CTRL_Z ); expect_line( 0, "abcdef" ); assert( !E.sel_active );
	key( KEY_REDO ); expect_line( 0, "aXef" );
	reset_text( "eins" ); insert_line( 1, "zwei", 4 ); insert_line( 2, "drei", 4 );
	key( KEY_SELECT_ALL ); key( KEY_DEL );
	assert( E.nlines == 1 ); expect_line( 0, "" );
	key( KEY_CTRL_Z ); assert( E.nlines == 3 ); expect_line( 2, "drei" );
	E.sel_active = 1; E.sel_ay = E.cy; E.sel_ax = E.cx;
	size_t count = E.undo_count;
	key( KEY_DEL ); assert( E.undo_count == count );
	reset_text( "bleibt" ); key( KEY_SELECT_ALL );
	free( E.clipboard ); E.clipboard = NULL; E.clip_len = 0;
	key( KEY_CTRL_V ); expect_line( 0, "bleibt" ); assert( E.undo_count == 0 );
	E.sel_active = 0; E.cx = 0;
	key( KEY_CTRL_C ); key( KEY_SELECT_ALL ); key( KEY_CTRL_V );
	key( KEY_CTRL_Z ); expect_line( 0, "bleibt" ); assert( E.nlines == 1 );
}

static void test_lines( void ) {
	reset_text( "    text" );
	E.cx = 2; key( KEY_ENTER );
	expect_line( 0, "  " ); expect_line( 1, "    text" ); assert( E.cx == 2 );
	key( KEY_CTRL_Z ); expect_line( 0, "    text" );
	E.cx = 0; key( KEY_ENTER ); expect_line( 0, "" ); expect_line( 1, "    text" );
	key( KEY_BACKSPACE ); expect_line( 0, "    text" ); assert( E.nlines == 1 );
	reset_text( "" ); key( KEY_CTRL_C ); key( KEY_CTRL_V ); assert( E.nlines == 2 );
	key( KEY_CTRL_Z ); assert( E.nlines == 1 );
	reset_text( "a" );
	for ( int i = 0; i < 100; i++ ) key( KEY_CTRL_D );
	assert( E.nlines == 101 ); expect_line( 100, "a" );
}

static void write_bytes( const char* path, const char* text, size_t len ) {
	FILE* fp = fopen( path, "wb" ); assert( fp );
	assert( fwrite( text, 1, len, fp ) == len ); assert( !fclose( fp ) );
}

static void expect_bytes( const char* path, const char* text, size_t len ) {
	FILE* fp = fopen( path, "rb" ); assert( fp );
	for ( size_t i = 0; i < len; i++ ) assert( fgetc( fp ) == ( unsigned char ) text[ i ] );
	assert( fgetc( fp ) == EOF ); assert( !ferror( fp ) ); fclose( fp );
}

static void test_files( void ) {
	const char* inputs[ ] = { "", "ohne Abschluss", "a\n", "\n\n", "a\r\nb\r\n", "a\rb", "a\r\nb\nc\rd", "\xef\xbb\xbfGr\xc3\xbc\xc3\x9f\x65\n" };
	const char* input = "eingabe.tmp";
	const char* output = "ausgabe.tmp";
	for ( size_t i = 0; i < sizeof inputs / sizeof inputs[ 0 ]; i++ ) {
		write_bytes( input, inputs[ i ], strlen( inputs[ i ] ) );
		assert( ed_open( input ) ); assert( E.nlines );
		assert( ed_save_to( output ) );
		expect_bytes( output, inputs[ i ], strlen( inputs[ i ] ) );
	}
	const char binary[ ] = { 'a', 0, 27, 'b', '\n' };
	write_bytes( input, binary, sizeof binary ); assert( ed_open( input ) );
	assert( ed_save_to( output ) ); expect_bytes( output, binary, sizeof binary );
	reset_text( "wichtig" ); key( 'X' );
	assert( !ed_open( "fehlt/datei.txt" ) ); expect_line( 0, "Xwichtig" ); assert( E.dirty );
	assert( E.undo_count == 1 );
	assert( ed_save_to( output ) ); assert( !E.dirty );
	key( 'Y' ); key( KEY_CTRL_Z ); assert( !E.dirty );
	key( KEY_CTRL_Z ); assert( E.dirty ); key( KEY_REDO ); assert( !E.dirty );
	key( 'Z' );
	assert( SetFileAttributesA( output, FILE_ATTRIBUTE_READONLY ) );
	assert( !ed_save_to( output ) ); assert( E.dirty );
	expect_bytes( output, "Xwichtig", 8 );
	assert( SetFileAttributesA( output, FILE_ATTRIBUTE_NORMAL ) );
	assert( !ed_save_to( "fehlt/ziel.txt" ) ); assert( !strcmp( E.filename, output ) );
	assert( ed_open( input ) ); assert( E.undo_count == 0 && E.redo_count == 0 );
	DeleteFileA( input ); DeleteFileA( output );
}

static void test_search_and_layout( void ) {
	reset_text( "alpha beta alpha" );
	strcpy( E.last_search, "alpha" );
	assert( ed_search_range( 0, 1, 0 ) ); assert( E.cx == 11 );
	assert( !ed_search_range( 0, 12, 0 ) );
	E.cx = 0; ed_search( 1 ); assert( E.cx == 11 );
	ed_search( 1 ); assert( E.cx == 0 );
	E.screen_cols = 8; E.gutter_width = 3; E.word_wrap = 1;
	ed_update_vlines( ); assert( E.nvlines == 4 );
	assert( ed_find_vline( 0, 5 ) == 1 );
	key( 'z' ); ed_update_vlines( ); assert( E.vlines[ E.nvlines - 1 ].char_len == 2 );
	reset_text( "ab\tcd\te" ); E.screen_cols = 8; E.gutter_width = 3;
	ed_update_vlines( );
	assert( E.vlines[ 0 ].char_len == 4 );
	assert( vline_rx_to_cx( &E.lines[ 0 ], 4, 3, 4 ) == 6 );
	reset_text( "\x1b[31m" ); E.screen_rows = 1; E.screen_cols = 20; E.gutter_width = 3;
	ed_update_vlines( ); Abuf output = { 0 }; ed_draw_rows( &output ); ab_append( &output, "", 1 );
	assert( !strstr( output.b, "\x1b[31m" ) ); free( output.b );
	reset_text( "/* offen" ); E.syntax = 1; ed_update_syntax_state( );
	assert( E.lines[ 0 ].hl_open_comment ); E.cx = E.lines[ 0 ].len;
	key( '*' ); key( '/' ); ed_update_syntax_state( ); assert( !E.lines[ 0 ].hl_open_comment );
}

static void test_quit( void ) {
	reset_text( "" ); key( 'x' );
	int quit = 0;
	ed_handle_key( KEY_CTRL_Q, &quit ); assert( !quit && E.quit_pending );
	ed_handle_key( KEY_LEFT, &quit ); assert( !E.quit_pending );
	ed_handle_key( KEY_CTRL_Q, &quit ); assert( !quit );
	ed_handle_key( KEY_FORCED_REFRESH, &quit ); assert( E.quit_pending );
	ed_handle_key( KEY_CTRL_Q, &quit ); assert( quit );
}

static void feed_key( WORD vk, WCHAR wc, DWORD mods ) {
	assert( event_count < sizeof events / sizeof events[ 0 ] );
	INPUT_RECORD* rec = &events[ event_count++ ];
	memset( rec, 0, sizeof *rec ); rec->EventType = KEY_EVENT;
	rec->Event.KeyEvent.bKeyDown = TRUE; rec->Event.KeyEvent.wRepeatCount = 1;
	rec->Event.KeyEvent.wVirtualKeyCode = vk; rec->Event.KeyEvent.uChar.UnicodeChar = wc;
	rec->Event.KeyEvent.dwControlKeyState = mods;
}

static void feed_text( const char* text ) {
	for ( size_t i = 0; text[ i ]; i++ ) {
		if ( text[ i ] == '\n' ) feed_key( VK_RETURN, '\r', 0 );
		else feed_key( isalnum( ( unsigned char ) text[ i ] ) ? ( WORD ) toupper( ( unsigned char ) text[ i ] ) : 0, ( WCHAR ) text[ i ], 0 );
	}
}

static void feed_mouse( int x, int y ) {
	INPUT_RECORD* rec = &events[ event_count++ ];
	memset( rec, 0, sizeof *rec ); rec->EventType = MOUSE_EVENT;
	rec->Event.MouseEvent.dwMousePosition.X = ( SHORT ) x;
	rec->Event.MouseEvent.dwMousePosition.Y = ( SHORT ) y;
	rec->Event.MouseEvent.dwButtonState = FROM_LEFT_1ST_BUTTON_PRESSED;
}

static void test_dialogs_and_menu( void ) {
	reset_text( "abc abc" );
	char value[ 32 ] = "vorlage";
	feed_key( VK_HOME, 0, 0 ); feed_key( VK_DELETE, 0, 0 ); feed_text( "V\n" );
	assert( ed_prompt_ex( "Name: ", value, sizeof value, 1, 0 ) );
	assert( !strcmp( value, "Vorlage" ) );
	feed_key( VK_ESCAPE, 0, 0 ); assert( !ed_prompt( "Name: ", value, sizeof value ) );
	feed_text( "\n" ); assert( ed_prompt_ex( "Leer: ", value, sizeof value, 0, 1 ) ); assert( !*value );
	feed_text( "abc\n\n" ); E.cx = 7;
	ed_search_replace( ); expect_line( 0, " " ); assert( E.cx == 1 );
	key( KEY_CTRL_Z ); expect_line( 0, "abc abc" );
	size_t count = E.undo_count;
	feed_text( "fehlt\nx\n" ); ed_search_replace( ); assert( E.undo_count == count );
	feed_text( "12x\n" ); ed_goto_line( ); assert( E.cy == 0 );
	feed_key( VK_F3, 0, 0 ); assert( read_key( ) == KEY_CTRL_N );
	feed_key( 'A', 0, LEFT_CTRL_PRESSED | SHIFT_PRESSED ); assert( read_key( ) == KEY_SELECT_ALL );
	feed_key( 'S', 0, LEFT_CTRL_PRESSED | SHIFT_PRESSED ); assert( read_key( ) == KEY_SAVE_AS );
	feed_key( 'Y', 0, LEFT_CTRL_PRESSED ); assert( read_key( ) == KEY_REDO );
	reset_text( "behalten" ); key( 'x' );
	int quit = 0;
	feed_key( VK_ESCAPE, 0, 0 ); ed_menu_action( 0, 1, &quit ); expect_line( 0, "xbehalten" );
	feed_key( VK_ESCAPE, 0, 0 ); ed_menu_action( 0, 0, &quit ); expect_line( 0, "xbehalten" );
	feed_key( VK_ESCAPE, 0, 0 ); ed_menu_action( 0, 4, &quit ); assert( !quit );
	feed_text( "v\n" ); ed_menu_action( 0, 0, &quit ); expect_line( 0, "" ); assert( !E.dirty );
	key( 'n' ); feed_text( "dialog.tmp\n" ); assert( ed_save( ) );
	expect_bytes( "dialog.tmp", "n", 1 ); DeleteFileA( "dialog.tmp" );
	reset_text( "markieren" ); E.cx = 4; E.sel_ax = 0; E.sel_ay = 0; E.sel_active = 1;
	ed_menu_action( 1, 1, &quit ); assert( E.clip_len == 4 && !E.clip_is_line );
	ed_menu_action( 1, 4, &quit ); assert( !E.sel_active );
	E.screen_rows = 22; E.screen_cols = 80;
	feed_mouse( 17, 10 ); ed_open_menu( &quit, 1 ); assert( E.sel_active && E.sel_ax == 0 );
	assert( event_pos == event_count );
}

static void test_vertical_column( void ) {
	reset_text( "123456789" ); insert_line( 1, "x", 1 ); insert_line( 2, "\t12345", 6 );
	E.word_wrap = 0; E.cx = 7;
	key( KEY_DOWN ); assert( E.cx == 1 );
	key( KEY_DOWN ); assert( E.cx == 4 );
	key( KEY_UP ); key( KEY_UP ); assert( E.cx == 7 );
	key( KEY_LEFT ); key( KEY_DOWN ); key( KEY_DOWN ); assert( E.cx == 3 );
}

int main( void ) {
	test_history( ); test_selection( ); test_lines( ); test_files( );
	test_search_and_layout( ); test_quit( );
	test_dialogs_and_menu( ); test_vertical_column( );
	ed_clear_buffer( ); free( E.clipboard );
	puts( "Alle Regressionstests bestanden." );
	return 0;
}

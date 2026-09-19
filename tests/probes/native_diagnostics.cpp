#include <stdio.h>
#include <stdarg.h>
[[maybe_unused]] static int TestVsprintf( char *, const char *, va_list );
[[maybe_unused]] static int TestVsnprintf( char *, size_t, const char *, va_list );
#define vsprintf TestVsprintf
#define vsnprintf TestVsnprintf
#if defined(PROBE_GAME)
#include "../../game/game/g_main.cpp"
#elif defined(PROBE_CGAME)
#include "../../game/cgame/cg_main.cpp"
#elif defined(PROBE_UI)
#include "../../game/ui/ui_atoms.cpp"
#elif defined(PROBE_BOT)
#include "../../game/game/ai_main.cpp"
#endif
#undef vsprintf
#undef vsnprintf
#include <assert.h>

static char output[4096];
static size_t expectedCapacity;
static int formats, unbounded;

static int TestVsprintf( char *text, const char *format, va_list args ) {
	// Small ordinary text only; observe the missing capacity without unsafe writes.
	++unbounded;
	return vsnprintf( text, 128, format, args );
}
static int TestVsnprintf( char *text, size_t size, const char *format, va_list args ) {
	assert( size == expectedCapacity );
	++formats;
	return vsnprintf( text, size, format, args );
}

void trap_Error( const char *text ) { strcpy( output, text ); }
#if defined(PROBE_GAME)
void trap_Printf( const char *text ) { strcpy( output, text ); }
void trap_FS_Write( const void *text, int length, fileHandle_t ) {
	assert( length == (int)strlen( (const char *)text ) );
	strcpy( output, (const char *)text );
}
void QDECL Com_sprintf( char *text, int size, const char *format, ... ) {
	va_list args;
	va_start( args, format );
	vsnprintf( text, size, format, args );
	va_end( args );
}
#elif defined(PROBE_BOT)
void QDECL G_Printf( const char *format, ... ) {
	va_list args;
	va_start( args, format );
	vsnprintf( output, sizeof( output ), format, args );
	va_end( args );
}
void QDECL G_Error( const char *, ... ) { abort(); }
#else
void trap_Print( const char *text ) { strcpy( output, text ); }
#endif
#if defined(PROBE_UI)
char *QDECL va( char *format, ... ) {
	static char text[2048];
	va_list args;
	va_start( args, format );
	vsnprintf( text, sizeof( text ), format, args );
	va_end( args );
	return text;
}
#endif
int main( int argc, char **argv ) {
	assert( argc == 2 );
	expectedCapacity = 1024;
#if defined(PROBE_GAME)
	if ( !strcmp( argv[1], "print" ) ) G_Printf( "%s:%d", "hello", 17 );
	else if ( !strcmp( argv[1], "error" ) ) G_Error( "%s:%d", "hello", 17 );
	else if ( !strcmp( argv[1], "shared-print" ) ) Com_Printf( "%s:%d", "hello", 17 );
	else if ( !strcmp( argv[1], "shared-error" ) ) Com_Error( ERR_DROP, "%s:%d", "hello", 17 );
	else {
		expectedCapacity = 1017;
		level.time = 175000;
		level.logFile = 1;
		G_LogPrintf( "%s:%d", "hello", 17 );
		assert( !strcmp( output, "  2:55 hello:17" ) );
	}
#elif defined(PROBE_CGAME)
	if ( !strcmp( argv[1], "print" ) ) CG_Printf( "%s:%d", "hello", 17 );
	else if ( !strcmp( argv[1], "error" ) ) CG_Error( "%s:%d", "hello", 17 );
	else if ( !strcmp( argv[1], "shared-print" ) ) Com_Printf( "%s:%d", "hello", 17 );
	else Com_Error( ERR_DROP, "%s:%d", "hello", 17 );
#elif defined(PROBE_UI)
	if ( !strcmp( argv[1], "print" ) ) Com_Printf( "%s:%d", "hello", 17 );
	else Com_Error( ERR_DROP, "%s:%d", "hello", 17 );
#else
	expectedCapacity = 2048;
	BotAI_Print( PRT_MESSAGE, (char *)"%s:%d", "hello", 17 );
#endif
	if ( strcmp( argv[1], "log" ) ) assert( !strcmp( output, "hello:17" ) );
	if ( unbounded || !formats ) {
		fprintf( stderr, "formatter was not given the destination capacity\n" );
		return 1;
	}
	return 0;
}

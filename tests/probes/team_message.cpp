#include <stdio.h>
#include <stdarg.h>
[[maybe_unused]] static int TestVsprintf( char *, const char *, va_list );
[[maybe_unused]] static int TestVsnprintf( char *, size_t, const char *, va_list );
#define vsprintf TestVsprintf
#define vsnprintf TestVsnprintf
#include "../../game/game/g_team.cpp"
#undef vsprintf
#undef vsnprintf
#include <assert.h>

gentity_t g_entities[MAX_GENTITIES];
static const char *mode;
static int sent;

static int format_result( char *text, const char *format, va_list args ) {
	// Intercept the formatter result with small text; never make an oversized write.
	int result = vsnprintf( text, 1024, format, args );
	if ( !strcmp( mode, "full" ) ) return 1024;
	if ( !strcmp( mode, "error" ) ) return -1;
	if ( !strcmp( mode, "fit" ) ) return 1023;
	return result;
}

static int TestVsprintf( char *text, const char *format, va_list args ) {
	return format_result( text, format, args );
}
static int TestVsnprintf( char *text, size_t size, const char *format, va_list args ) {
	assert( size == 1024 );
	return format_result( text, format, args );
}

void QDECL G_Error( const char *message, ... ) {
	exit( !strcmp( message, "PrintMsg overrun" ) ? 42 : 43 );
}
char *QDECL va( char *format, ... ) {
	static char text[2048];
	va_list args;
	va_start( args, format );
	vsnprintf( text, sizeof( text ), format, args );
	va_end( args );
	return text;
}
void trap_SendServerCommand( int client, const char *text ) {
	assert( client == -1 );
	assert( !strcmp( text, "print \"hello 'game':17\"" ) );
	++sent;
}
int main( int argc, char **argv ) {
	assert( argc == 2 );
	mode = argv[1];
	PrintMsg( NULL, "%s:%d", "hello \"game\"", 17 );
	assert( sent == 1 );
	return 0;
}

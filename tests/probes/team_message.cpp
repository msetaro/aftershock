#include <cmath>
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
// ASan retains the new callback identity table; these unrelated paths must not run.
vmCvar_t g_gametype;
#ifdef MISSIONPACK
level_locals_t level;
vmCvar_t g_obeliskHealth, g_obeliskRegenPeriod, g_obeliskRegenAmount;
void G_AddEvent( gentity_t *, int, int ) {
	abort();
}
void AddScore( gentity_t *, vec3_t, int ) {
	abort();
}
#endif
void trap_SetConfigstring( int, const char * ) {
	abort();
}
gentity_t *G_Find( gentity_t *, int, const char * ) {
	abort();
}
void G_FreeEntity( gentity_t * ) {
	abort();
}
void RespawnItem( gentity_t * ) {
	abort();
}
gentity_t *G_TempEntity( vec3_t, int ) {
	abort();
}
void QDECL G_Printf( const char *, ... ) {
	abort();
}
static const char *mode;
static int sent;

static int format_result( char *text, const char *format, va_list args ) {
	// Intercept the formatter result with small text; never make an oversized write.
	int result = vsnprintf( text, 1024, format, args );
	if ( !strcmp( mode, "full" ) )
		return 1024;
	if ( !strcmp( mode, "error" ) )
		return -1;
	if ( !strcmp( mode, "fit" ) )
		return 1023;
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

#include "q_shared.h"

static int warnings;

#ifdef ENGINE_PROBE
void QDECL Com_Error( errorParm_t code, const char *message, ... ) {
#else
void QDECL Com_Error( int code, const char *message, ... ) {
#endif
	exit( code == ERR_FATAL && !strcmp( message, "Com_sprintf: overflowed bigbuffer" ) ? 42 : 43 );
}

void QDECL Com_Printf( const char *message, ... ) {
	(void)message;
	++warnings;
}

int main( int argc, char **argv ) {
	static char input[32001];
	static char output[32001];
	int lengths[] = { 0, 1, 31, 31999 };
	unsigned i;
	assert( argc == 2 );
	if ( !strcmp( argv[1], "overflow" ) ) {
		memset( input, 'x', sizeof( input ) - 1 );
		Com_sprintf( output, sizeof( output ), "%s", input );
		return 1;
	}
	for ( i = 0; i < sizeof( lengths ) / sizeof( lengths[0] ); ++i ) {
		int length = lengths[i];
		memset( input, 'x', length );
		input[length] = '\0';
		memset( output, '!', sizeof( output ) );
#ifdef ENGINE_PROBE
		assert( Com_sprintf( output, length + 1, "%s", input ) == length );
#else
		Com_sprintf( output, length + 1, "%s", input );
#endif
		assert( !strcmp( input, output ) && output[length + 1] == '!' );
	}
	assert( warnings == 0 );
	Com_sprintf( output, 8, "%s", "123456789" );
	assert( !strcmp( output, "1234567" ) && warnings == 1 );
	strcpy( output, "shared" );
	Com_sprintf( output, sizeof( output ), "%s:%d:%.2f", output, 17, 1.25 );
	assert( !strcmp( output, "shared:17:1.25" ) );
	return 0;
}

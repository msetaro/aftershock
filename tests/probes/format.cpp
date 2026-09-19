#include "q_shared.h"

static int warnings;

#ifdef ENGINE_PROBE
void QDECL Com_Error( errorParm_t code, const char *message, ... ) {
#else
void QDECL Com_Error( int code, const char *message, ... ) {
#endif
	exit( code == ERR_FATAL && (!strcmp( message, "Com_sprintf: overflowed bigbuffer" ) || !strcmp( message, "va: overflowed buffer" )) ? 42 : 43 );
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
	if ( !strcmp( argv[1], "va-overflow" ) ) {
		memset( input, 'x', sizeof( input ) - 1 );
		va( (char *)"%s", input );
		va( (char *)"%s", input );
		return 1;
	}
	if ( !strcmp( argv[1], "va-valid" ) ) {
		const char *first, *second, *third;
		for ( i = 0; i < sizeof( lengths ) / sizeof( lengths[0] ); ++i ) {
			memset( input, 'x', lengths[i] );
			input[lengths[i]] = '\0';
			assert( !strcmp( va( (char *)"%s", input ), input ) );
		}
		first = va( (char *)"%s:%d", "first", 17 );
		second = va( (char *)"%.2f", 1.25 );
		assert( first != second && !strcmp( first, "first:17" ) && !strcmp( second, "1.25" ) );
		third = va( (char *)"third" );
		assert( third == first && !strcmp( third, "third" ) && !strcmp( second, "1.25" ) );
		return 0;
	}
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

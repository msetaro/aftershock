#include "../../game/bg/q_shared.h"
#include <stdint.h>

static uint32_t state = 0x12345678;
static uint32_t hash = 2166136261u;

static void bytes( const void *data, size_t size ) {
	const unsigned char *p = (const unsigned char *)data;
	while ( size-- ) {
		hash = ( hash ^ *p++ ) * 16777619u;
	}
}

static float sample( void ) {
	state = state * 1664525u + 1013904223u;
	return ( (int)( state >> 8 ) - 8388608 ) * ( 1.0f / 8192.0f );
}

void QDECL Com_Error( int level, const char *error, ... ) {
	(void)level;
	fprintf( stderr, "%s\n", error );
	abort();
}

void QDECL Com_Printf( const char *format, ... ) {
	va_list args;
	va_start( args, format );
	vfprintf( stderr, format, args );
	va_end( args );
}

int main( void ) {
	int i, j;
	for ( i = 0; i < 4096; i++ ) {
		vec3_t angles, forward, right, up, axis[3], normalized;
		float length;
		for ( j = 0; j < 3; j++ ) {
			angles[j] = i < 4 ? i * 90.0f : sample();
		}
		AngleVectors( angles, forward, right, up );
		bytes( forward, sizeof( forward ) );
		bytes( right, sizeof( right ) );
		bytes( up, sizeof( up ) );
		AnglesToAxis( angles, axis );
		bytes( axis, sizeof( axis ) );
		length = VectorNormalize2( angles, normalized );
		bytes( &length, sizeof( length ) );
		bytes( normalized, sizeof( normalized ) );
		length = Q_rsqrt( length + 1.0f );
		bytes( &length, sizeof( length ) );
	}
	printf( "math %08x\n", hash );
	hash = 2166136261u;
	for ( i = 1; i < 256; i++ ) {
		char text[2] = { (char)i, 0 };
		Q_strlwr( text );
		bytes( text, sizeof( text ) );
		text[0] = (char)i;
		Q_strupr( text );
		bytes( text, sizeof( text ) );
	}
	printf( "case %08x\n", hash );
	return 0;
}

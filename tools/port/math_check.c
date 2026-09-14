/* G5 fixed-input probe; compiled in each language, never part of the engine. */
#include "../../code/qcommon/q_shared.h"

void QDECL Com_Error( errorParm_t level, const char *fmt, ... )
{
	(void)level;
	(void)fmt;
	exit( 1 );
}

void QDECL Com_Printf( const char *fmt, ... )
{
	(void)fmt;
}

static unsigned hash_bytes( unsigned hash, const void *data, size_t size )
{
	const unsigned char *bytes = (const unsigned char *)data;
	size_t i;
	for ( i = 0; i < size; i++ )
		hash = ( hash ^ bytes[i] ) * 16777619U;
	return hash;
}

int main( void )
{
	unsigned rotate = 2166136261U, angles = rotate, vectors = rotate, roots = rotate;
	vec3_t axis = { 0, 0, 1 }, point = { 1, 2, 3 }, out, forward, right, up;
	float input, root;
	int i;
	for ( i = 1; i <= 10000; i++ ) {
		input = (float)i / 37.0f;
		RotatePointAroundVector( out, axis, point, input );
		rotate = hash_bytes( rotate, out, sizeof( out ) );
		point[0] = input;
		vectoangles( point, out );
		angles = hash_bytes( angles, out, sizeof( out ) );
		AngleVectors( out, forward, right, up );
		vectors = hash_bytes( vectors, forward, sizeof( forward ) );
		vectors = hash_bytes( vectors, right, sizeof( right ) );
		vectors = hash_bytes( vectors, up, sizeof( up ) );
		root = Q_rsqrt( input );
		roots = hash_bytes( roots, &root, sizeof( root ) );
	}
	printf( "RotatePointAroundVector %08x\nvectoangles %08x\nAngleVectors %08x\nQ_rsqrt %08x\n", rotate, angles, vectors, roots );
	return 0;
}

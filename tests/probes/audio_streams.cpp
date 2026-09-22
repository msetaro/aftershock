#include "../../engine/qcommon/q_shared.h"
#include "../../engine/qcommon/qcommon_public.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main( int argc, char **argv ) {
	assert( argc == 2 );
	const char *path = argv[1];
	FILE *existing = fopen( path, "wb" );
	assert( existing );
	assert( fwrite( "keep", 1, 4, existing ) == 4 );
	fclose( existing );
	assert( !Sys_OpenTemporaryFile( path ) );
	existing = fopen( path, "rb" );
	char kept[4];
	assert( existing && fread( kept, 1, 4, existing ) == 4 && !memcmp( kept, "keep", 4 ) );
	fclose( existing );
	assert( remove( path ) == 0 );
	FILE *stream = Sys_OpenTemporaryFile( path );
	assert( stream );
	char buffering[4096], source[1024], output[1024];
	assert( setvbuf( stream, buffering, _IOFBF, sizeof( buffering ) ) == 0 );
	for ( unsigned int i = 0; i < sizeof( source ); ++i )
		source[i] = char( i % 127 );
	assert( fwrite( source, 1, sizeof( source ), stream ) == sizeof( source ) );
	for ( int loop = 0; loop < 200; ++loop ) {
		assert( fseek( stream, 0, SEEK_SET ) == 0 );
		assert( fread( output, 1, sizeof( output ), stream ) == sizeof( output ) );
		assert( !memcmp( source, output, sizeof( source ) ) );
	}
	fclose( stream );
	assert( !fopen( path, "rb" ) );
	puts( "PASS: exclusive temporary PCM file, retained buffer, 200 rewinds and automatic removal" );
}

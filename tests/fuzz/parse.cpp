#include "common.h"

extern "C" int LLVMFuzzerTestOneInput( const uint8_t *data, size_t size )
{
	static char text[65537];
	const char *cursor;
	if ( size > sizeof( text ) - 1 ) return 0;
	memcpy( text, data, size );
	text[size] = '\0';
	cursor = text;
	if ( setjmp( fuzz_error ) ) return 0;
	COM_BeginParseSession( "fuzz" );
	while ( cursor ) COM_ParseExt( &cursor, qtrue );
	return 0;
}

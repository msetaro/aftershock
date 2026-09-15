/* Exercise the private zlib allocator callbacks without reading content. */
#include "../../third_party/minizip/unzip.cpp"
#include <assert.h>

void TestInflateAllocations( void )
{
	z_stream stream = { 0 };
	assert( inflateInit2( &stream, 15 ) == Z_OK );
	assert( stream.state != NULL );
	assert( inflateEnd( &stream ) == Z_OK );
	assert( stream.state == NULL );
}

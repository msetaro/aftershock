#include "../../engine/qcommon/q_shared.h"
#include "../../engine/qcommon/qcommon_public.h"
#include <cassert>
#include <cstdio>

// Use the production connection encoder for the local UDP handshake test.
int main() {
	byte data[4096] = {};
	const size_t size = fread( data, 1, MAX_INFO_STRING, stdin );
	assert( size > 12 && size < MAX_INFO_STRING );
	msg_t message = {};
	message.data = data;
	message.maxsize = sizeof( data );
	message.cursize = (int)size;
	Huff_Compress( &message, 12 );
	assert( fwrite( data, 1, (size_t)message.cursize, stdout ) == (size_t)message.cursize );
}

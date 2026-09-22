#include "../../engine/botlib/be_ai_chat.cpp"
#include <cassert>

botlib_import_t botimport;
static int allocations;

void *GetClearedMemory( size_t size ) {
	void *memory = calloc( 1, size );
	assert( memory );
	++allocations;
	return memory;
}
void FreeMemory( void *memory ) {
	if ( memory )
		--allocations;
	free( memory );
}
float LibVarGetValue( const char * ) {
	return 0;
}
void Q_strncpyz( char *out, const char *in, int size ) {
	assert( size > 0 );
	size_t count = strlen( in );
	if ( count >= (size_t)size )
		count = size - 1;
	memcpy( out, in, count );
	out[count] = 0;
}

int main( void ) {
	for ( int cycle = 0; cycle < 3; ++cycle ) {
		for ( int handle = 1; handle <= MAX_CLIENTS; ++handle )
			assert( BotAllocChatState() == handle );
		assert( BotAllocChatState() == 0 );
		BotShutdownChatAI();
		for ( int handle = 1; handle <= MAX_CLIENTS; ++handle ) {
			if ( botchatstates[handle] ) {
				fprintf( stderr, "FAIL: shutdown retained chat handle %d\n", handle );
				BotFreeChatState( handle );
				return 1;
			}
		}
		assert( allocations == 0 );
		BotShutdownChatAI();
		assert( allocations == 0 );
	}
	return 0;
}

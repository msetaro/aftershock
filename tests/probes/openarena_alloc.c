#include "g_local.h"
#include <assert.h>

void QDECL Com_Error( int level, const char *format, ... ) {
	abort();
}

int main( void ) {
	unsigned int sizes[] = { 16, 24, 25, 28, 32, 33, 63, 64, sizeof(gentity_t), sizeof(gclient_t) };
	void *blocks[10];
	static void *full[65536];
	gentity_t *entity;
	unsigned int i, j, count;

	BG_InitMemory();
	entity = BG_Alloc( sizeof(*entity) );
	entity->client = NULL;
	entity->health = 17;
	assert( entity->health == 17 );
	BG_Free( entity );

	for ( i = 0; i < 10; i++ ) {
		assert( BG_CanAlloc( sizes[i] ) );
		blocks[i] = BG_Alloc( sizes[i] );
		*(void **)blocks[i] = blocks[i];
		assert( *(void **)blocks[i] == blocks[i] );
		memset( blocks[i], i + 1, sizes[i] );
	}
	for ( i = 0; i < 10; i += 2 ) {
		BG_Free( blocks[i] );
	}
	for ( i = 0; i < 10; i += 2 ) {
		blocks[i] = BG_Alloc( sizes[i] );
		*(void **)blocks[i] = blocks[i];
		memset( blocks[i], i + 1, sizes[i] );
	}
	for ( i = 0; i < 10; i++ ) {
		for ( j = 0; j < sizes[i]; j++ ) {
			assert( ((unsigned char *)blocks[i])[j] == i + 1 );
		}
		BG_Free( blocks[i] );
	}
	BG_DefragmentMemory();
	assert( BG_CanAlloc( sizeof(gentity_t) ) );
	BG_InitMemory();
	for ( count = 0; BG_CanAlloc(16); count++ ) {
		assert( count < sizeof(full) / sizeof(full[0]) );
		full[count] = BG_Alloc(16);
	}
	assert( count );
	for ( i = count; i > 0; i-- ) {
		BG_Free( full[i - 1] );
	}
	for ( i = 0; i < count; i++ ) {
		assert( BG_CanAlloc(16) );
		full[i] = BG_Alloc(16);
	}
	assert( !BG_CanAlloc(16) );
	while ( count ) {
		BG_Free( full[--count] );
	}
	return 0;
}

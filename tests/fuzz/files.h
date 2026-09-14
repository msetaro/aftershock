/* Bounded test allocator and in-memory file input; reclaimed after each case. */
#include "common.h"

static const uint8_t *file_data;
static size_t file_size, allocated;
static void *allocations[4096];

void *Z_Malloc( size_t size )
{
	int i;
	if ( size > 64 * 1024 * 1024 || allocated + size > 64 * 1024 * 1024 )
		Com_Error( ERR_DROP, "fuzz allocation budget" );
	for ( i = 0; i < ARRAY_LEN( allocations ); i++ ) if ( !allocations[i] ) break;
	if ( i == ARRAY_LEN( allocations ) ) Com_Error( ERR_DROP, "fuzz allocation count" );
	allocations[i] = calloc( size ? size : 1, 1 );
	if ( !allocations[i] ) Com_Error( ERR_DROP, "fuzz allocation failure" );
	allocated += size;
	return allocations[i];
}
void Z_Free( void *p )
{
	int i;
	if ( !p ) return;
	for ( i = 0; i < ARRAY_LEN( allocations ); i++ ) if ( allocations[i] == p ) break;
	if ( i == ARRAY_LEN( allocations ) ) abort();
	free( p ); allocations[i] = NULL;
}
void *Hunk_AllocateTempMemory( size_t size ) { return Z_Malloc( size ); }
void Hunk_FreeTempMemory( void *p ) { Z_Free( p ); }
void *Hunk_Alloc( size_t size, ha_pref pref ) { (void)pref; return Z_Malloc( size ); }
void *S_Malloc( size_t size ) { return Z_Malloc( size ); }
void *Z_TagMalloc( size_t size, memtag_t tag ) { (void)tag; return Z_Malloc( size ); }
char *CopyString( const char *s ) { char *p = (char *)Z_Malloc( strlen( s ) + 1 ); strcpy( p, s ); return p; }
int FS_ReadFile( const char *name, void **buffer )
{
	(void)name;
	if ( buffer ) {
		*buffer = Z_Malloc( file_size + 1 );
		memcpy( *buffer, file_data, file_size );
	}
	return (int)file_size;
}
void FS_FreeFile( void *p ) { Z_Free( p ); }
void FS_WriteFile( const char *name, const void *buffer, int size ) { (void)name; (void)buffer; (void)size; abort(); }
static void ResetAllocations( void )
{
	int i;
	for ( i = 0; i < ARRAY_LEN( allocations ); i++ ) { free( allocations[i] ); allocations[i] = NULL; }
	allocated = 0;
}

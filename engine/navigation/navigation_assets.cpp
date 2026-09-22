#include "navigation_public.h"
#include "behavior_public.h"
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon_public.h"

navWorld_t *Nav_LoadFile( const char *path, uint32_t collisionChecksum, uint32_t maxAgents, uint8_t hash[32] ) {
	fileHandle_t file = 0;
	const int size = FS_FOpenFileRead( path, &file, qtrue );
	if ( !file || size < 112 || size > 16 * 1024 * 1024 ) {
		if ( file )
			FS_FCloseFile( file );
		return nullptr;
	}
	void *bytes = Z_Malloc( size_t( size ) );
	const int read = FS_Read( bytes, size, file );
	FS_FCloseFile( file );
	navWorld_t *world = read == size ? Nav_Open( bytes, size_t( size ), collisionChecksum, maxAgents ) : nullptr;
	if ( world )
		memcpy( hash, static_cast<const uint8_t *>( bytes ) + 16, 32 );
	Z_Free( bytes );
	return world;
}
bool AI_LoadBehavior( const char *path, aiBehavior_t *out, uint8_t hash[32] ) {
	uint8_t bytes[48 + sizeof( aiBehavior_t )];
	fileHandle_t file = 0;
	const int size = FS_FOpenFileRead( path, &file, qtrue );
	if ( !file || size < 48 || size > int( sizeof( bytes ) ) ) {
		if ( file )
			FS_FCloseFile( file );
		return false;
	}
	const int read = FS_Read( bytes, size, file );
	FS_FCloseFile( file );
	if ( read != size || !AI_ReadBehavior( bytes, size_t( size ), out ) )
		return false;
	memcpy( hash, bytes + 16, 32 );
	return true;
}

#include "animation_public.h"
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon_public.h"

void *Anim_LoadFile( const char *path, animAsset_t *asset ) {
	*asset = {};
	fileHandle_t file = 0;
	const int size = FS_FOpenFileRead( path, &file, qtrue );
	if ( size < 48 || size > ( 16 << 20 ) + 48 || !file ) {
		if ( file )
			FS_FCloseFile( file );
		return nullptr;
	}
	void *storage = Z_TagMalloc( (size_t)size, TAG_GENERAL );
	const int read = FS_Read( storage, size, file );
	FS_FCloseFile( file );
	if ( read != size || !Anim_Open( storage, (size_t)size, asset ) ) {
		Z_Free( storage );
		return nullptr;
	}
	return storage;
}
void Anim_FreeFile( void *storage ) {
	if ( storage )
		Z_Free( storage );
}

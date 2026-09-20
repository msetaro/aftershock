#include "weapons_public.h"
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon_public.h"

bool Weapon_LoadFile( const char *path, weaponDef_t *definition, uint8_t hash[32] ) {
	byte bytes[48 + sizeof( weaponDef_t )];
	fileHandle_t file = 0;
	const int size = FS_FOpenFileRead( path, &file, qtrue );
	if ( size != sizeof( bytes ) || !file ) {
		if ( file )
			FS_FCloseFile( file );
		return false;
	}
	const int read = FS_Read( bytes, size, file );
	FS_FCloseFile( file );
	if ( read != size || !Weapon_Open( bytes, sizeof( bytes ), definition ) )
		return false;
	memcpy( hash, bytes + 16, 32 );
	return true;
}

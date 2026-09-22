#include "content_public.h"
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon_public.h"
#include <cstdio>

bool Sys_OpenContentFile( const char *path, sysContentFile_t *out ) {
	if ( !path || !out || out->handle )
		return false;
	FILE *file = Sys_FOpen( path, "rb" );
	if ( !file )
		return false;
#ifdef _WIN32
	const int seek = _fseeki64( file, 0, SEEK_END );
	const int64_t length = _ftelli64( file );
#else
	static_assert( sizeof( off_t ) >= 8 );
	const int seek = fseeko( file, 0, SEEK_END );
	const int64_t length = ftello( file );
#endif
	if ( seek || length < 0 ) {
		fclose( file );
		return false;
	}
	*out = { file, uint64_t( length ) };
	return true;
}
bool Sys_ReadContentFile( const sysContentFile_t &file, uint64_t offset, void *data, size_t size ) {
	if ( !file.handle || offset > file.size || size > file.size - offset || ( size && !data ) || offset > INT64_MAX )
		return false;
	if ( !size )
		return true;
	auto *stream = static_cast<FILE *>( file.handle );
#ifdef _WIN32
	if ( _fseeki64( stream, int64_t( offset ), SEEK_SET ) )
#else
	if ( fseeko( stream, off_t( offset ), SEEK_SET ) )
#endif
		return false;
	return fread( data, 1, size, stream ) == size;
}
void Sys_CloseContentFile( sysContentFile_t *file ) {
	if ( file ) {
		if ( file->handle )
			fclose( static_cast<FILE *>( file->handle ) );
		*file = {};
	}
}

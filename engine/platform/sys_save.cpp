#include "save_public.h"
#include "../qcommon/filesystem_public.h"
#include <cstdio>
#include <cstring>

static saveProvider_t saveProvider;
static bool saveProviderUsed;
static constexpr int SAVE_CAPACITY = 64 * 1024 * 1024;

bool Sys_InstallSaveProvider( const saveProvider_t *provider ) {
	if ( saveProviderUsed || ( provider && ( !provider->read || !provider->create ) ) )
		return false;
	saveProvider = provider ? *provider : saveProvider_t{};
	return true;
}
static bool SaveNameCharacter( char c ) {
	return ( c >= 'a' && c <= 'z' ) || ( c >= '0' && c <= '9' ) || c == '_' || c == '-';
}
bool Sys_SaveRevision( saveKind_t kind, const char *name, const void *data, int size, char *path, size_t capacity ) {
	if ( path && capacity )
		path[0] = 0;
	if ( !name || !name[0] || std::strlen( name ) > 31 || !data || size <= 0 || size > SAVE_CAPACITY || !path || capacity < 64 ||
		 ( kind != saveKind_t::Profile && kind != saveKind_t::Game ) )
		return false;
	for ( const char *p = name; *p; ++p )
		if ( !SaveNameCharacter( *p ) )
			return false;
	saveProviderUsed = true;
	for ( int revision = 0; revision < 1000; ++revision ) {
		std::snprintf( path, capacity, "%s/%s.%03d.asstate", kind == saveKind_t::Profile ? "profiles" : "saves", name, revision );
		const auto result = saveProvider.create ? saveProvider.create( path, data, size ) : FS_CreateSave( path, data, size );
		if ( result == saveWriteResult_t::Written )
			return true;
		if ( result != saveWriteResult_t::Exists )
			break;
	}
	path[0] = 0;
	return false;
}
int Sys_ReadSave( const char *path, void *data, int capacity ) {
	if ( !path || std::strlen( path ) >= 64 || capacity < 0 || capacity > SAVE_CAPACITY )
		return -1;
	const char *name;
	if ( !std::strncmp( path, "profiles/", 9 ) )
		name = path + 9;
	else if ( !std::strncmp( path, "saves/", 6 ) )
		name = path + 6;
	else
		return -1;
	const size_t length = std::strlen( name );
	if ( length <= 8 || std::strcmp( name + length - 8, ".asstate" ) || std::strstr( name, ".." ) || !SaveNameCharacter( *name ) )
		return -1;
	for ( const char *p = name; *p; ++p )
		if ( !SaveNameCharacter( *p ) && *p != '.' )
			return -1;
	saveProviderUsed = true;
	const int size = saveProvider.read ? saveProvider.read( path, data, capacity ) : FS_ReadSave( path, data, capacity );
	return size > 0 && size <= SAVE_CAPACITY && ( !data || size <= capacity ) ? size : -1;
}

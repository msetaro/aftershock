#include "q_shared.h"
#include "qcommon_public.h"
#include "keys_public.h"
#include "../public/state_public.h"

// Version 1 capacities are file-format limits, independent of registry growth.
// ponytail: fixed tables use under 5 MiB per revision; compact active rows if profile IO becomes measurable.
static constexpr uint32_t PROFILE_CVARS = 2048, PROFILE_KEYS = 512, PROFILE_STRING = 1024;
struct profile_t {
	uint32_t cvarCount, keyCount;
	char names[PROFILE_CVARS][PROFILE_STRING], values[PROFILE_CVARS][PROFILE_STRING];
	char keys[PROFILE_KEYS][32], bindings[PROFILE_KEYS][PROFILE_STRING];
};
static_assert( sizeof( profile_t ) == 8 + PROFILE_CVARS * PROFILE_STRING * 2 + PROFILE_KEYS * ( 32 + PROFILE_STRING ) );
static_assert( offsetof( profile_t, names ) == 8 && offsetof( profile_t, bindings ) == 8 + PROFILE_CVARS * PROFILE_STRING * 2 + PROFILE_KEYS * 32 );
static constexpr stateField_t profileFields[] = {
	{ "cvar_count", offsetof( profile_t, cvarCount ), 1, stateType_t::UInt32 },
	{ "key_count", offsetof( profile_t, keyCount ), 1, stateType_t::UInt32 },
	{ "cvar_names", offsetof( profile_t, names ), sizeof( profile_t::names ), stateType_t::Bytes },
	{ "cvar_values", offsetof( profile_t, values ), sizeof( profile_t::values ), stateType_t::Bytes },
	{ "key_names", offsetof( profile_t, keys ), sizeof( profile_t::keys ), stateType_t::Bytes },
	{ "bindings", offsetof( profile_t, bindings ), sizeof( profile_t::bindings ), stateType_t::Bytes }
};
static constexpr stateSchema_t profileSchema = { "profile", 1, 1, sizeof( profile_t ), profileFields, ARRAY_LEN( profileFields ) };

static bool CaptureProfile( profile_t *profile ) {
	for ( const cvar_t *var = Cvar_First(); var; var = var->next ) {
		if ( !var->name || !( var->flags & CVAR_ARCHIVE ) || ( var->flags & CVAR_PRIVATE ) || !Q_stricmp( var->name, "cl_cdkey" ) )
			continue;
		const char *value = var->latchedString ? var->latchedString : var->string;
		if ( profile->cvarCount == PROFILE_CVARS || strlen( var->name ) >= PROFILE_STRING || strlen( value ) >= PROFILE_STRING )
			return false;
		Q_strncpyz( profile->names[profile->cvarCount], var->name, PROFILE_STRING );
		Q_strncpyz( profile->values[profile->cvarCount++], value, PROFILE_STRING );
	}
	for ( int key = 0; key < MAX_KEYS; ++key ) {
		const char *binding = Key_GetBinding( key );
		if ( !binding || !*binding )
			continue;
		const char *name = Key_KeynumToString( key );
		if ( profile->keyCount == PROFILE_KEYS || strlen( name ) >= sizeof( profile->keys[0] ) || strlen( binding ) >= PROFILE_STRING )
			return false;
		Q_strncpyz( profile->keys[profile->keyCount], name, sizeof( profile->keys[0] ) );
		Q_strncpyz( profile->bindings[profile->keyCount++], binding, PROFILE_STRING );
	}
	return true;
}
static bool ValidProfile( const profile_t &profile ) {
	if ( profile.cvarCount > PROFILE_CVARS || profile.keyCount > PROFILE_KEYS )
		return false;
	int variables = 0;
	for ( const cvar_t *var = Cvar_First(); var; var = var->next )
		++variables;
	for ( uint32_t i = 0; i < profile.cvarCount; ++i ) {
		if ( !memchr( profile.names[i], 0, PROFILE_STRING ) || !memchr( profile.values[i], 0, PROFILE_STRING ) ||
			 !profile.names[i][0] || !Cvar_ValidateName( profile.names[i] ) || !Q_stricmp( profile.names[i], "cl_cdkey" ) || ( Cvar_Flags( profile.names[i] ) & CVAR_PRIVATE ) )
			return false;
		for ( uint32_t j = 0; j < i; ++j )
			if ( !Q_stricmp( profile.names[i], profile.names[j] ) )
				return false;
		if ( Cvar_Flags( profile.names[i] ) == CVAR_NONEXISTENT )
			++variables;
	}
	if ( variables > Cvar_Capacity() )
		return false;
	int keys[PROFILE_KEYS];
	for ( uint32_t i = 0; i < profile.keyCount; ++i ) {
		if ( !memchr( profile.keys[i], 0, sizeof( profile.keys[i] ) ) || !memchr( profile.bindings[i], 0, PROFILE_STRING ) )
			return false;
		keys[i] = Key_StringToKeynum( profile.keys[i] );
		if ( keys[i] < 0 || keys[i] >= MAX_KEYS )
			return false;
		for ( uint32_t j = 0; j < i; ++j )
			if ( keys[i] == keys[j] )
				return false;
	}
	return true;
}
static bool SaveProfile( const char *name, profile_t *profile ) {
	if ( !*name || strlen( name ) > 31 )
		return false;
	for ( const char *p = name; *p; ++p )
		if ( !( *p >= 'a' && *p <= 'z' ) && !( *p >= '0' && *p <= '9' ) && *p != '_' && *p != '-' )
			return false;
	if ( !CaptureProfile( profile ) || !ValidProfile( *profile ) )
		return false;
	char path[MAX_QPATH];
	int revision;
	for ( revision = 0; revision < 1000; ++revision ) {
		Com_sprintf( path, sizeof( path ), "profiles/%s.%03d.asstate", name, revision );
		if ( !FS_FileExists( path ) )
			break;
	}
	if ( revision == 1000 )
		return false;
	const size_t capacity = sizeof( profile_t ) + 1024;
	void *data = Z_Malloc( capacity );
	const size_t size = State_Write( profileSchema, profile, data, capacity );
	const fileHandle_t file = size ? FS_FOpenFileWrite( path ) : 0;
	const bool success = file && FS_Write( data, int( size ), file ) == int( size );
	if ( file )
		FS_FCloseFile( file );
	Z_Free( data );
	if ( success )
		Com_Printf( "Profile saved: %s\n", path );
	return success;
}
static bool LoadProfile( const char *path, profile_t *profile ) {
	if ( strncmp( path, "profiles/", 9 ) || strlen( path ) >= MAX_QPATH || strstr( path, ".." ) || strchr( path, '\\' ) )
		return false;
	fileHandle_t file;
	const int length = FS_FOpenFileRead( path, &file, qtrue );
	if ( !file )
		return false;
	if ( length <= 0 || size_t( length ) > sizeof( profile_t ) + 1024 ) {
		FS_FCloseFile( file );
		return false;
	}
	void *data = Z_Malloc( size_t( length ) );
	const bool complete = FS_Read( data, length, file ) == length;
	FS_FCloseFile( file );
	uint32_t version;
	const bool valid = complete && State_Read( profileSchema, data, size_t( length ), profile, &version ) && ValidProfile( *profile );
	Z_Free( data );
	if ( !valid )
		return false;
	for ( uint32_t i = 0; i < profile->cvarCount; ++i ) {
		if ( cvar_t *var = Cvar_Set2( profile->names[i], profile->values[i], qfalse ) )
			var->flags |= CVAR_ARCHIVE;
	}
	cvar_modifiedFlags |= CVAR_ARCHIVE;
	for ( int key = 0; key < MAX_KEYS; ++key )
		Key_SetBinding( key, "" );
	for ( uint32_t i = 0; i < profile->keyCount; ++i )
		Key_SetBinding( Key_StringToKeynum( profile->keys[i] ), profile->bindings[i] );
	Com_Printf( "Profile loaded: %s (version %u)\n", path, version );
	return true;
}
static void ProfileCommand() {
	const bool save = !strcmp( Cmd_Argv( 0 ), "saveprofile" );
	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "saveprofile <name> | loadprofile profiles/<name>.<revision>.asstate\n" );
		return;
	}
	auto *profile = (profile_t *)Z_Malloc( sizeof( profile_t ) );
	const bool success = save ? SaveProfile( Cmd_Argv( 1 ), profile ) : LoadProfile( Cmd_Argv( 1 ), profile );
	Z_Free( profile );
	if ( !success )
		Com_Printf( "Profile rejected: check path, record/string capacity, revision limit and storage.\n" );
}
void Com_InitProfileCommands() {
	Cmd_AddCommand( "saveprofile", ProfileCommand );
	Cmd_AddCommand( "loadprofile", ProfileCommand );
}

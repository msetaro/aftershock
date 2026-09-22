#include "q_shared.h"
#include "qcommon_public.h"
#include "keys_public.h"
#include "../platform/save_public.h"
#include "../public/state_public.h"
#include "../public/dev_public.h"
#include <cmath>
#if defined( AFTERSHOCK_DEVTOOLS ) && !defined( DEDICATED )
#include "../devtools/devtools_public.h"
#endif

// Version 1 capacities are file-format limits, independent of registry growth.
// ponytail: fixed tables use under 5 MiB per revision; compact active rows if profile IO becomes measurable.
static constexpr uint32_t PROFILE_CVARS = 2048, PROFILE_KEYS = 512, PROFILE_STRING = 1024;
struct profile_t {
	uint32_t cvarCount, keyCount;
	char names[PROFILE_CVARS][PROFILE_STRING], values[PROFILE_CVARS][PROFILE_STRING];
	char keys[PROFILE_KEYS][32], bindings[PROFILE_KEYS][PROFILE_STRING];
	devWorkspace_t workspace;
};
static_assert( sizeof( profile_t ) == 8 + PROFILE_CVARS * PROFILE_STRING * 2 + PROFILE_KEYS * ( 32 + PROFILE_STRING ) + sizeof( devWorkspace_t ) );
static_assert( offsetof( profile_t, names ) == 8 && offsetof( profile_t, bindings ) == 8 + PROFILE_CVARS * PROFILE_STRING * 2 + PROFILE_KEYS * 32 );
static_assert( sizeof( devWorkspace_t ) == 5556 && offsetof( devWorkspace_t, enabled ) == 5536 && offsetof( devWorkspace_t, radius ) == 5552 );
static constexpr stateField_t profileFields[] = {
	{ "cvar_count", offsetof( profile_t, cvarCount ), 1, stateType_t::UInt32 },
	{ "key_count", offsetof( profile_t, keyCount ), 1, stateType_t::UInt32 },
	{ "cvar_names", offsetof( profile_t, names ), sizeof( profile_t::names ), stateType_t::Bytes },
	{ "cvar_values", offsetof( profile_t, values ), sizeof( profile_t::values ), stateType_t::Bytes },
	{ "key_names", offsetof( profile_t, keys ), sizeof( profile_t::keys ), stateType_t::Bytes },
	{ "bindings", offsetof( profile_t, bindings ), sizeof( profile_t::bindings ), stateType_t::Bytes },
	{ "editor_panel", offsetof( profile_t, workspace.panel ), sizeof( devWorkspace_t::panel ), stateType_t::Bytes, 2 },
	{ "editor_cvar", offsetof( profile_t, workspace.cvar ), sizeof( devWorkspace_t::cvar ), stateType_t::Bytes, 2 },
	{ "editor_filters", offsetof( profile_t, workspace.filters ), sizeof( devWorkspace_t::filters ), stateType_t::Bytes, 2 },
	{ "editor_layout", offsetof( profile_t, workspace.layout ), sizeof( devWorkspace_t::layout ), stateType_t::Bytes, 2 },
	{ "editor_flags", offsetof( profile_t, workspace.enabled ), 4, stateType_t::UInt32, 2 },
	{ "editor_radius", offsetof( profile_t, workspace.radius ), 1, stateType_t::Float32, 2 }
};
static constexpr stateSchema_t profileSchema = { "profile", 2, 1, sizeof( profile_t ), profileFields, ARRAY_LEN( profileFields ) };

static void DefaultWorkspace( devWorkspace_t *workspace ) {
	*workspace = {};
	Q_strncpyz( workspace->panel, "Console", sizeof( workspace->panel ) );
	workspace->radius = 512;
}
static bool CaptureProfile( profile_t *profile ) {
	DefaultWorkspace( &profile->workspace );
#if defined( AFTERSHOCK_DEVTOOLS ) && !defined( DEDICATED )
	if ( !DevTools_CaptureWorkspace( &profile->workspace ) )
		return false;
#endif
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
	const auto &workspace = profile.workspace;
	if ( !memchr( workspace.panel, 0, sizeof( workspace.panel ) ) || !memchr( workspace.cvar, 0, sizeof( workspace.cvar ) ) ||
		 !memchr( workspace.layout, 0, sizeof( workspace.layout ) ) || !std::isfinite( workspace.radius ) || workspace.radius < 64 || workspace.radius > 2048 )
		return false;
	for ( const auto &filter : workspace.filters )
		if ( !memchr( filter, 0, sizeof( filter ) ) )
			return false;
	const uint32_t flags[] = { workspace.enabled, workspace.collision, workspace.navigation, workspace.entities };
	for ( uint32_t flag : flags )
		if ( flag > 1 )
			return false;
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
	int keyNumbers[PROFILE_KEYS];
	for ( uint32_t i = 0; i < profile.keyCount; ++i ) {
		if ( !memchr( profile.keys[i], 0, sizeof( profile.keys[i] ) ) || !memchr( profile.bindings[i], 0, PROFILE_STRING ) )
			return false;
		keyNumbers[i] = Key_StringToKeynum( profile.keys[i] );
		if ( keyNumbers[i] < 0 || keyNumbers[i] >= MAX_KEYS )
			return false;
		for ( uint32_t j = 0; j < i; ++j )
			if ( keyNumbers[i] == keyNumbers[j] )
				return false;
	}
	return true;
}
static bool SaveProfile( const char *name, profile_t *profile ) {
	if ( !CaptureProfile( profile ) || !ValidProfile( *profile ) )
		return false;
	char path[MAX_QPATH];
	const size_t capacity = sizeof( profile_t ) + 1024;
	void *data = Z_Malloc( capacity );
	const size_t size = State_Write( profileSchema, profile, data, capacity );
	const bool success = size && Sys_SaveRevision( saveKind_t::Profile, name, data, int( size ), path, sizeof( path ) );
	Z_Free( data );
	if ( success )
		Com_Printf( "Profile saved: %s\n", path );
	return success;
}
static bool LoadProfile( const char *path, profile_t *profile ) {
	if ( strncmp( path, "profiles/", 9 ) || strlen( path ) >= MAX_QPATH || strstr( path, ".." ) || strchr( path, '\\' ) )
		return false;
	const int length = Sys_ReadSave( path, nullptr, 0 );
	if ( length <= 0 || size_t( length ) > sizeof( profile_t ) + 1024 )
		return false;
	void *data = Z_Malloc( size_t( length ) );
	const bool complete = Sys_ReadSave( path, data, length ) == length;
	uint32_t version = 0;
	bool valid = complete && State_Read( profileSchema, data, size_t( length ), profile, &version );
	// Version 1 had no workspace; give the new fields explicit, stable defaults.
	if ( valid && version == 1 )
		DefaultWorkspace( &profile->workspace );
	valid = valid && ValidProfile( *profile );
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
#if defined( AFTERSHOCK_DEVTOOLS ) && !defined( DEDICATED )
	DevTools_RestoreWorkspace( profile->workspace );
#endif
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

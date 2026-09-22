#include "server.h"
#include "identity_public.h"
#include "../platform/runtime_public.h"
#include "../qcommon/json.h"
#include <charconv>
#include <cstring>

static struct {
	char match[65], key[65];
	uint64_t players[64];
	uint32_t count;
} joinConfig;
static joinReplay_t joinReplay;
static bool joinRequired;

bool SV_SetJoinConfig( const char *match, const char *key, const uint64_t *players, uint32_t count ) {
	if ( !match || !key || !players || !count || count > 64 || !match[0] || strlen( match ) > 64 || strlen( key ) != 64 )
		return false;
	for ( const char *p = match; *p; ++p )
		if ( !( ( *p >= 'a' && *p <= 'z' ) || ( *p >= 'A' && *p <= 'Z' ) ||
				 ( *p >= '0' && *p <= '9' ) || *p == '_' || *p == '-' ) )
			return false;
	for ( const char *p = key; *p; ++p )
		if ( !( ( *p >= '0' && *p <= '9' ) || ( *p >= 'a' && *p <= 'f' ) ) )
			return false;
	for ( uint32_t i = 0; i < count; ++i ) {
		if ( !players[i] )
			return false;
		for ( uint32_t j = 0; j < i; ++j )
			if ( players[j] == players[i] )
				return false;
	}
	// Same-match reloads must not turn an already consumed ticket into a new one.
	if ( strcmp( joinConfig.match, match ) != 0 || strcmp( joinConfig.key, key ) != 0 )
		joinReplay = {};
	memcpy( joinConfig.match, match, strlen( match ) + 1 );
	memcpy( joinConfig.key, key, 65 );
	memcpy( joinConfig.players, players, count * sizeof( players[0] ) );
	joinConfig.count = count;
	joinRequired = true;
	return true;
}
bool SV_JoinRequired() {
	return joinRequired;
}
const char *SV_JoinMatch() {
	return joinConfig.match;
}
static bool Expected( uint64_t player ) {
	for ( uint32_t i = 0; i < joinConfig.count; ++i )
		if ( joinConfig.players[i] == player )
			return true;
	return false;
}
bool SV_ValidateJoin( const char *token, joinClaims_t *out ) {
	if ( !out )
		return false;
	*out = {};
	const auto now = Sys_Time( nullptr );
	joinClaims_t claims;
	if ( !SV_JoinRequired() || now <= 0 || !Join_Verify( joinConfig.key, token, joinConfig.match, (uint64_t)now, &claims ) || !Expected( claims.player ) )
		return false;
	*out = claims;
	return true;
}
bool SV_JoinAvailable( const joinClaims_t &claims ) {
	if ( !Expected( claims.player ) || strcmp( claims.match, joinConfig.match ) != 0 )
		return false;
	for ( int i = 0; i < sv.maxclients; ++i )
		if ( svs.clients[i].identityId == claims.player &&
			 ( svs.clients[i].identityState == IDENTITY_PENDING || svs.clients[i].identityState == IDENTITY_VERIFIED ) )
			return false;
	const auto now = Sys_Time( nullptr );
	if ( now <= 0 || (uint64_t)now < claims.issued || (uint64_t)now >= claims.expires )
		return false;
	bool available = false;
	for ( const auto &used : joinReplay.used ) {
		if ( used.expires <= (uint64_t)now )
			available = true;
		else if ( !memcmp( used.nonce, claims.nonce, sizeof( claims.nonce ) ) )
			return false;
	}
	return available;
}
bool SV_ApplyJoin( int clientNum, const joinClaims_t &claims ) {
	if ( !SV_IdentitySession( clientNum ) || !SV_JoinAvailable( claims ) )
		return false;
	auto *client = &svs.clients[clientNum];
	if ( client->identityState != IDENTITY_ANONYMOUS )
		return false;
	const auto now = Sys_Time( nullptr );
	if ( now <= 0 || !Join_Consume( &joinReplay, claims, (uint64_t)now ) )
		return false;
	client->identityId = claims.player;
	client->identityProvider = SERVICE_BACKEND;
	client->identityState = IDENTITY_VERIFIED;
	memcpy( client->joinNonce, claims.nonce, sizeof( client->joinNonce ) );
	return true;
}
bool SV_JoinRetry( int clientNum, const joinClaims_t &claims, int challenge ) {
	if ( !SV_IdentitySession( clientNum ) )
		return false;
	const auto *client = &svs.clients[clientNum];
	return client->state == CS_CONNECTED && client->identityState == IDENTITY_VERIFIED && client->identityProvider == SERVICE_BACKEND &&
		   client->identityId == claims.player && client->challenge == challenge && !memcmp( client->joinNonce, claims.nonce, sizeof( client->joinNonce ) );
}

// The controller writes this private file before opening admission. Never echo its contents.
bool SV_LoadJoinConfig( const char *data, uint32_t size ) {
	joinRequired = true;
	if ( !data || !size || size > 8192 || !JSON_ValidateObject( data, data + size ) )
		return false;
	const char *end = data + size;
	JSON_Whitespace( data, end );
	++data;
	uint32_t seen = 0, count = 0;
	char match[65] = {}, key[65] = {};
	uint64_t players[64] = {};
	while ( data < end ) {
		JSON_Whitespace( data, end );
		if ( *data == '}' )
			break;
		char name[32];
		if ( !JSON_ReadString( data, end, name, sizeof( name ) ) )
			return false;
		JSON_Whitespace( data, end );
		++data; // Colon and delimiters were validated above.
		JSON_Whitespace( data, end );
		const char *value = data;
		uint32_t field;
		if ( !strcmp( name, "version" ) ) {
			field = 1;
			if ( *value != '1' || JSON_SkipValue( value, end ) != value + 1 )
				return false;
		} else if ( !strcmp( name, "match_id" ) || !strcmp( name, "join_key" ) ) {
			field = !strcmp( name, "match_id" ) ? 2 : 4;
			if ( !JSON_ReadString( value, end, field == 2 ? match : key, sizeof( match ) ) )
				return false;
		} else if ( !strcmp( name, "expected_players" ) ) {
			field = 8;
			const char *entries[64];
			count = JSON_ArrayGetIndex( value, end, entries, 64 );
			if ( !count || count > 64 )
				return false;
			for ( uint32_t i = 0; i < count; ++i ) {
				char id[21];
				if ( !JSON_ReadString( entries[i], end, id, sizeof( id ) ) || id[0] < '1' || id[0] > '9' )
					return false;
				const char *stop = id + strlen( id );
				const auto result = std::from_chars( id, stop, players[i] );
				if ( result.ec != std::errc{} || result.ptr != stop )
					return false;
			}
		} else
			return false;
		if ( seen & field )
			return false;
		seen |= field;
		data = JSON_SkipValue( data, end );
		JSON_Whitespace( data, end );
		if ( *data == ',' )
			++data;
	}
	return seen == 15 && SV_SetJoinConfig( match, key, players, count );
}
void SV_JoinConfig_f() {
	joinRequired = true;
	fileHandle_t file = FS_INVALID_HANDLE;
	const int size = FS_SV_FOpenFileRead( "match-join.json", &file );
	char data[8192];
	const bool read = file != FS_INVALID_HANDLE && size > 0 && size <= (int)sizeof( data ) && FS_Read( data, size, file ) == size;
	if ( file != FS_INVALID_HANDLE )
		FS_FCloseFile( file );
	if ( !read || !SV_LoadJoinConfig( data, (uint32_t)size ) ) {
		Com_Printf( "Join configuration rejected; authenticated admission remains required.\n" );
		return;
	}
	Com_Printf( "Join configuration ready: %s\n", joinConfig.match );
}

#include "server.h"
#include "identity_public.h"
#include "../platform/runtime_public.h"
#include <cstring>

static struct {
	char match[65], key[65];
	uint64_t players[64];
	uint32_t count;
} joinConfig;
static joinReplay_t joinReplay;

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
	return true;
}
bool SV_JoinRequired() {
	return joinConfig.count != 0;
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
bool SV_ApplyJoin( int clientNum, const joinClaims_t &claims ) {
	if ( !SV_IdentitySession( clientNum ) || !Expected( claims.player ) || strcmp( claims.match, joinConfig.match ) != 0 )
		return false;
	auto *client = &svs.clients[clientNum];
	if ( client->identityState != IDENTITY_ANONYMOUS )
		return false;
	for ( int i = 0; i < sv.maxclients; ++i )
		if ( svs.clients[i].identityId == claims.player &&
			 ( svs.clients[i].identityState == IDENTITY_PENDING || svs.clients[i].identityState == IDENTITY_VERIFIED ) )
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

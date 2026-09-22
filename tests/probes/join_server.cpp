#include "../../engine/server/sv_join.cpp"
#include <assert.h>
#include <cstring>
#include <cstdio>
#include <initializer_list>

server_t sv;
serverStatic_t svs;
static client_t clients[2];
static serviceAuthEvent_t event;
static int ends, drops;
static time_t now = 2050;
time_t Sys_Time( time_t *result ) {
	if ( result )
		*result = now;
	return now;
}
void SV_DropClient( client_t *client, const char * ) {
	++drops;
	SV_CloseIdentity( client );
	client->state = CS_ZOMBIE;
}
static void Connect( int index ) {
	clients[index] = {};
	clients[index].state = CS_CONNECTED;
	clients[index].challenge = 17;
	SV_OpenIdentity( &clients[index] );
}
int main( int argc, char **argv ) {
	assert( argc == 3 );
	sv.maxclients = 2;
	svs.clients = clients;
	const char *key = "4242424242424242424242424242424242424242424242424242424242424242";
	const uint64_t players[] = { UINT64_MAX, 123 };
	assert( !SV_JoinRequired() );
	assert( !SV_SetJoinConfig( "match-1", "bad", players, 2 ) && !SV_JoinRequired() );
	assert( !SV_SetJoinConfig( "match-1", key, players, 0 ) );
	// Failed file configuration closes admission even before a valid config exists.
	assert( !SV_LoadJoinConfig( "{}", 2 ) && SV_JoinRequired() );
	joinClaims_t empty;
	assert( !SV_ValidateJoin( argv[1], &empty ) && !empty.player );
	char config[512];
	snprintf( config, sizeof( config ), "{\"version\":1,\"match_id\":\"match-1\",\"join_key\":\"%s\",\"expected_players\":[\"18446744073709551615\",\"123\"]}", key );
	assert( SV_LoadJoinConfig( config, (uint32_t)strlen( config ) ) );
	assert( SV_SetJoinConfig( "match-1", key, players, 2 ) && SV_JoinRequired() );
	serviceProvider_t provider = {
		[]( uint64_t, uint64_t, const void *, uint32_t ) { return false; },
		[]( uint64_t, uint64_t ) { ++ends; },
		[]( serviceAuthEvent_t *out ) { *out = event; event = {}; return out->session != 0; },
		[]( uint64_t, serviceSearch_t, const char * ) { return false; },
		[]( serviceServer_t * ) { return false; },
		[]( uint64_t ) {}
	};
	assert( Sys_InstallServices( &provider ) );
	joinClaims_t claims, another;
	assert( SV_ValidateJoin( argv[1], &claims ) );
	assert( SV_ValidateJoin( argv[2], &another ) );
	Connect( 0 );
	assert( SV_ApplyJoin( 0, claims ) );
	uint64_t id;
	assert( SV_PlayerIdentity( 0, &id ) == SERVICE_BACKEND && id == UINT64_MAX );
	assert( SV_JoinRetry( 0, claims, 17 ) && !SV_JoinRetry( 0, claims, 18 ) );
	assert( !SV_JoinRetry( 0, another, 17 ) );
	clients[0].state = CS_ACTIVE;
	assert( !SV_JoinRetry( 0, claims, 17 ) );
	Connect( 1 );
	assert( !SV_ApplyJoin( 1, another ) ); // One authenticated connection per account.
	SV_CloseIdentity( &clients[0] );
	assert( !ends ); // Backend sessions never end an unrelated SDK session.
	assert( SV_ApplyJoin( 1, another ) );
	event = { SV_IdentitySession( 1 ), UINT64_MAX, SERVICE_AUTH_REJECTED };
	SV_PollIdentities();
	assert( !drops && SV_PlayerIdentity( 1, &id ) == SERVICE_BACKEND );
	SV_CloseIdentity( &clients[1] );
	assert( !ends );
	Connect( 0 );
	assert( !SV_ApplyJoin( 0, claims ) ); // Disconnect does not erase replay protection.
	assert( SV_SetJoinConfig( "match-1", key, players, 2 ) );
	assert( !SV_ApplyJoin( 0, claims ) ); // Same-match config reload does not either.
	assert( SV_LoadJoinConfig( config, (uint32_t)strlen( config ) ) );
	assert( !SV_ApplyJoin( 0, claims ) ); // File reload preserves consumed nonces too.
	for ( const char *bad : { "[]", "{", "{\"version\":1,}", "{}{}", "{\"version\":1,\"version\":1}",
			  "{\"version\":2}", "{\"version\":1,\"expected_players\":[\"01\"]}" } ) {
		assert( !SV_LoadJoinConfig( bad, (uint32_t)strlen( bad ) ) );
		assert( SV_ValidateJoin( argv[1], &claims ) );
	}
	char invalid[512];
	memcpy( invalid, config, strlen( config ) + 1 );
	strcpy( invalid + strlen( invalid ) - 1, ",\"extra\":true}" );
	assert( !SV_LoadJoinConfig( invalid, (uint32_t)strlen( invalid ) ) );
	memcpy( invalid, config, strlen( config ) + 1 );
	strcpy( invalid + strlen( invalid ) - 1, ",\"version\":1}" );
	assert( !SV_LoadJoinConfig( invalid, (uint32_t)strlen( invalid ) ) );
	assert( !SV_LoadJoinConfig( nullptr, 0 ) && !SV_LoadJoinConfig( config, 8193 ) );
	const uint64_t duplicate[] = { 123, 123 };
	assert( !SV_SetJoinConfig( "match-1", key, duplicate, 2 ) );
	assert( SV_ValidateJoin( argv[1], &claims ) ); // Invalid config preserves the last valid one.
	assert( SV_SetJoinConfig( "match-1", key, players + 1, 1 ) );
	assert( !SV_ValidateJoin( argv[1], &claims ) && !claims.player );
	assert( SV_SetJoinConfig( "match-1", key, players, 2 ) );
	now = 2120;
	assert( !SV_ValidateJoin( argv[1], &claims ) );
	puts( "PASS: expected-player join identity, handshake retry, SDK isolation and replay retention across disconnect/config reload" );
}

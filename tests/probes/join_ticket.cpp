#include "../../engine/qcommon/join_public.h"
#include <assert.h>
#include <cstring>
#include <cstdio>
#include <type_traits>

static_assert( std::is_trivially_destructible_v<joinClaims_t> && std::is_trivially_destructible_v<joinReplay_t> );
int main( int argc, char **argv ) {
	assert( argc == 2 );
	const char *key = "4242424242424242424242424242424242424242424242424242424242424242";
	const char *token = argv[1];
	joinClaims_t claims;
	assert( Join_Verify( key, token, "match-1", 2050, &claims ) );
	assert( claims.player == UINT64_MAX && claims.issued == 2000 && claims.expires == 2120 );
	assert( !strcmp( claims.match, "match-1" ) );
	for ( unsigned char byte : claims.nonce )
		assert( byte == 0xab );
	joinClaims_t rejected;
	assert( !Join_Verify( key, token, "match-2", 2050, &rejected ) && !rejected.player );
	assert( !Join_Verify( key, token, "match-1", 1999, &rejected ) );
	assert( !Join_Verify( key, token, "match-1", 2120, &rejected ) );
	assert( !Join_Verify( "42", token, "match-1", 2050, &rejected ) );
	assert( !Join_Verify( nullptr, token, "match-1", 2050, &rejected ) );
	assert( !Join_Verify( key, nullptr, "match-1", 2050, &rejected ) );
	assert( !Join_Verify( key, token, nullptr, 2050, &rejected ) );
	assert( !Join_Verify( key, token, "match-1", 2050, nullptr ) );
	char bad[257];
	assert( strlen( token ) < sizeof( bad ) );
	strcpy( bad, token );
	bad[strlen( bad ) - 1] = bad[strlen( bad ) - 1] == '0' ? '1' : '0';
	assert( !Join_Verify( key, bad, "match-1", 2050, &rejected ) );
	strcpy( bad, token );
	bad[0] = '2';
	assert( !Join_Verify( key, bad, "match-1", 2050, &rejected ) );
	memset( bad, 'x', sizeof( bad ) - 1 );
	bad[sizeof( bad ) - 1] = 0;
	assert( !Join_Verify( key, bad, "match-1", 2050, &rejected ) );

	joinReplay_t replay = {};
	assert( Join_Consume( &replay, claims, 2050 ) );
	assert( !Join_Consume( &replay, claims, 2050 ) );
	assert( !Join_Consume( &replay, claims, 2120 ) );
	replay = {};
	for ( uint32_t i = 0; i < 256; ++i ) {
		claims.nonce[0] = (uint8_t)i;
		claims.nonce[1] = 0;
		assert( Join_Consume( &replay, claims, 2050 ) );
	}
	claims.nonce[1] = 1;
	assert( !Join_Consume( &replay, claims, 2050 ) ); // Full cache fails closed.
	claims.issued = 2200;
	claims.expires = 2320;
	assert( Join_Consume( &replay, claims, 2250 ) ); // Expired entries may be reused.
	assert( !Join_Consume( &replay, claims, 2250 ) );
	puts( "PASS: native join-ticket signature, identity/match/time bounds and bounded one-use nonce retention" );
}

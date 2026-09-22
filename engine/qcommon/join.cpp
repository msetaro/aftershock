#include "join_public.h"
#include "../../third_party/sha256/sha-256.h"
#include <cstring>

static bool Identifier( const char *text ) {
	const size_t size = strlen( text );
	if ( !size || size > 64 )
		return false;
	for ( const char *p = text; *p; ++p )
		if ( !( ( *p >= 'a' && *p <= 'z' ) || ( *p >= 'A' && *p <= 'Z' ) ||
				 ( *p >= '0' && *p <= '9' ) || *p == '-' || *p == '_' ) )
			return false;
	return true;
}
static bool Decimal( const char *text, uint64_t *out ) {
	if ( text[0] < '1' || text[0] > '9' )
		return false;
	uint64_t value = 0;
	for ( const char *p = text; *p; ++p ) {
		if ( *p < '0' || *p > '9' )
			return false;
		const uint32_t digit = (uint32_t)( *p - '0' );
		if ( value > ( UINT64_MAX - digit ) / 10 )
			return false;
		value = value * 10 + digit;
	}
	*out = value;
	return true;
}
static bool Hex( const char *text, uint8_t *out, size_t bytes ) {
	if ( strlen( text ) != bytes * 2 )
		return false;
	for ( size_t i = 0; i < bytes; ++i ) {
		uint32_t value = 0;
		for ( size_t j = 0; j < 2; ++j ) {
			const char c = text[i * 2 + j];
			if ( c >= '0' && c <= '9' )
				value = value * 16 + (uint32_t)( c - '0' );
			else if ( c >= 'a' && c <= 'f' )
				value = value * 16 + (uint32_t)( c - 'a' + 10 );
			else
				return false;
		}
		out[i] = (uint8_t)value;
	}
	return true;
}
static void Signature( const uint8_t key[32], const char *payload, size_t size, uint8_t out[32] ) {
	constexpr char domain[] = "aftershock/join/v1\n";
	uint8_t inner[64 + sizeof( domain ) - 1 + 256], outer[64 + 32];
	memset( inner, 0x36, 64 );
	memset( outer, 0x5c, 64 );
	for ( uint32_t i = 0; i < 32; ++i ) {
		inner[i] ^= key[i];
		outer[i] ^= key[i];
	}
	memcpy( inner + 64, domain, sizeof( domain ) - 1 );
	memcpy( inner + 64 + sizeof( domain ) - 1, payload, size );
	calc_sha_256( outer + 64, inner, 64 + sizeof( domain ) - 1 + size );
	calc_sha_256( out, outer, sizeof( outer ) );
}
bool Join_Verify( const char *key, const char *token, const char *match, uint64_t now, joinClaims_t *out ) {
	if ( !out )
		return false;
	*out = {};
	if ( !key || !token || !match || !Identifier( match ) )
		return false;
	const size_t size = strlen( token );
	if ( size > 256 )
		return false;
	char copy[257];
	memcpy( copy, token, size + 1 );
	char *fields[7] = { copy };
	uint32_t count = 1;
	for ( size_t i = 0; i < size; ++i ) {
		if ( copy[i] != '.' )
			continue;
		if ( count == 7 )
			return false;
		copy[i] = 0;
		fields[count++] = copy + i + 1;
	}
	if ( count != 7 || strcmp( fields[0], "1" ) != 0 || strcmp( fields[2], match ) != 0 )
		return false;
	joinClaims_t claims = {};
	uint8_t secret[32], supplied[32], expected[32];
	if ( !Decimal( fields[1], &claims.player ) || !Decimal( fields[3], &claims.issued ) || !Decimal( fields[4], &claims.expires ) ||
		 claims.issued > UINT64_C( 999999999999 ) || claims.expires > UINT64_C( 999999999999 ) ||
		 claims.expires <= claims.issued || claims.expires - claims.issued > 120 || now < claims.issued || now >= claims.expires ||
		 !Hex( fields[5], claims.nonce, sizeof( claims.nonce ) ) || !Hex( fields[6], supplied, sizeof( supplied ) ) || !Hex( key, secret, sizeof( secret ) ) )
		return false;
	Signature( secret, token, (size_t)( fields[6] - copy ) - 1, expected );
	uint32_t difference = 0;
	for ( uint32_t i = 0; i < 32; ++i )
		difference |= supplied[i] ^ expected[i];
	if ( difference )
		return false;
	memcpy( claims.match, match, strlen( match ) + 1 );
	*out = claims;
	return true;
}
bool Join_Consume( joinReplay_t *replay, const joinClaims_t &claims, uint64_t now ) {
	if ( !replay || now < claims.issued || now >= claims.expires )
		return false;
	uint32_t available = UINT32_MAX;
	for ( uint32_t i = 0; i < 256; ++i ) {
		if ( replay->used[i].expires <= now ) {
			if ( available == UINT32_MAX )
				available = i;
		} else if ( !memcmp( replay->used[i].nonce, claims.nonce, sizeof( claims.nonce ) ) ) {
			return false;
		}
	}
	if ( available == UINT32_MAX )
		return false;
	replay->used[available].expires = claims.expires;
	memcpy( replay->used[available].nonce, claims.nonce, sizeof( claims.nonce ) );
	return true;
}

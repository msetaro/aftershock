#ifndef JOIN_PUBLIC_H
#define JOIN_PUBLIC_H
#include <stdint.h>

struct joinClaims_t {
	uint64_t player, issued, expires;
	char match[65];
	uint8_t nonce[16];
};
struct joinReplay_t {
	struct {
		uint64_t expires;
		uint8_t nonce[16];
	} used[256];
};
// Inputs are terminated strings. Key is 32 bytes in lower-case hex. The compact
// v1 contract is shared with tools/match/contracts; invalid output is cleared.
bool Join_Verify( const char *key, const char *token, const char *match, uint64_t now, joinClaims_t *out );
// Main-thread only; consume already verified claims after expected-player checks.
// Keep this store for the whole match, including map restarts and disconnects.
// A full live store fails closed. Expired entries can be reused.
bool Join_Consume( joinReplay_t *replay, const joinClaims_t &claims, uint64_t now );
#endif

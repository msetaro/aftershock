#ifndef SERVER_IDENTITY_PUBLIC_H
#define SERVER_IDENTITY_PUBLIC_H
#include <stdint.h>

// Main-thread bridge for #23's ticket transport. A session is a connection generation,
// not proof of identity. Never accept a session number supplied by an untrusted peer:
// the transport obtains it for the client whose reliable channel delivered the ticket.
uint64_t SV_IdentitySession( int clientNum );
bool SV_SubmitIdentityTicket( uint64_t session, uint64_t claimedId, const void *ticket, uint32_t size );
// Returns the provider (0 anonymous, 1 Steam), and clears id unless authenticated.
int SV_PlayerIdentity( int clientNum, uint64_t *id );
#endif

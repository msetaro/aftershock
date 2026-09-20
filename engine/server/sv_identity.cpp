#include "server.h"
#include "identity_public.h"

static uint64_t identitySerial;

void SV_CloseIdentity( client_t *client ) {
	if ( client->identityState == IDENTITY_PENDING || client->identityState == IDENTITY_VERIFIED )
		Sys_EndAuth( client->identitySession, client->identityId );
	client->identitySession = client->identityId = 0;
	client->identityState = IDENTITY_ANONYMOUS;
}
void SV_OpenIdentity( client_t *client ) {
	SV_CloseIdentity( client );
	// Never reuse a token, even across server restarts or the theoretical wrap boundary.
	if ( identitySerial != UINT64_MAX )
		client->identitySession = ++identitySerial;
}
uint64_t SV_IdentitySession( int clientNum ) {
	if ( clientNum < 0 || clientNum >= sv.maxclients || svs.clients[clientNum].state < CS_CONNECTED ||
		 svs.clients[clientNum].netchan.remoteAddress.type == NA_BOT )
		return 0;
	return svs.clients[clientNum].identitySession;
}
int SV_PlayerIdentity( int clientNum, uint64_t *id ) {
	if ( !id )
		return SERVICE_ANONYMOUS;
	*id = 0;
	if ( SV_IdentitySession( clientNum ) && svs.clients[clientNum].identityState == IDENTITY_VERIFIED ) {
		*id = svs.clients[clientNum].identityId;
		return SERVICE_STEAM;
	}
	return SERVICE_ANONYMOUS;
}
bool SV_SubmitIdentityTicket( uint64_t session, uint64_t claimedId, const void *ticket, uint32_t size ) {
	if ( !session || !claimedId || !ticket || !size || size > SERVICE_MAX_TICKET || !Sys_AuthAvailable() )
		return false;
	client_t *client = nullptr;
	for ( int i = 0; i < sv.maxclients; ++i ) {
		if ( SV_IdentitySession( i ) == session )
			client = &svs.clients[i];
		if ( svs.clients[i].identityId == claimedId &&
			 ( svs.clients[i].identityState == IDENTITY_PENDING || svs.clients[i].identityState == IDENTITY_VERIFIED ) )
			return false;
	}
	if ( !client || client->identityState != IDENTITY_ANONYMOUS )
		return false;
	// One attempt per connection; the provider's synchronous acceptance is only pending.
	client->identityState = IDENTITY_REJECTED;
	if ( !Sys_BeginAuth( session, claimedId, ticket, size ) )
		return false;
	client->identityId = claimedId;
	client->identityState = IDENTITY_PENDING;
	client->identityStart = (uint32_t)svs.time;
	return true;
}
void SV_PollIdentities() {
	serviceAuthEvent_t event;
	// Bound provider work per frame. Additional queued callbacks wait for the next frame.
	for ( int n = 0; n < MAX_CLIENTS * 2 && Sys_PollAuth( &event ); ++n ) {
		for ( int i = 0; i < sv.maxclients; ++i ) {
			client_t *client = &svs.clients[i];
			if ( !SV_IdentitySession( i ) || client->identitySession != event.session || client->identityId != event.id )
				continue;
			if ( event.result == SERVICE_AUTH_REJECTED )
				SV_DropClient( client, "Platform identity rejected or revoked" );
			else if ( client->identityState == IDENTITY_PENDING )
				client->identityState = IDENTITY_VERIFIED;
			break;
		}
	}
	for ( int i = 0; i < sv.maxclients; ++i ) {
		client_t *client = &svs.clients[i];
		if ( client->identityState == IDENTITY_PENDING && (uint32_t)svs.time - client->identityStart >= 30000u )
			SV_DropClient( client, "Platform identity verification timed out" );
	}
}

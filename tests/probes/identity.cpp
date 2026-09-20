#include "../../engine/server/sv_identity.cpp"
#include <assert.h>
#include <cstring>

server_t sv;
serverStatic_t svs;
static client_t clients[2];
static serviceAuthEvent_t event;
static serviceServer_t result;
static int starts, ends, drops, searches, cancels;
static bool accept = true;
static uint64_t searchToken;
void SV_DropClient( client_t *client, const char * ) {
	++drops;
	SV_CloseIdentity( client );
	client->state = CS_ZOMBIE;
}
static bool Begin( uint64_t session, uint64_t id, const void *ticket, uint32_t size ) {
	assert( session && id && ticket && size == 4 );
	++starts;
	return accept;
}
static void End( uint64_t session, uint64_t id ) {
	assert( session && id );
	++ends;
}
static bool Poll( serviceAuthEvent_t *out ) {
	*out = event;
	event = {};
	return out->session != 0;
}
static bool Search( uint64_t request, serviceSearch_t mode, const char *filter ) {
	assert( mode == SERVICE_BROWSE || mode == SERVICE_MATCH );
	assert( !strcmp( filter, "dm" ) );
	searchToken = request;
	++searches;
	return true;
}
static bool Next( serviceServer_t *out ) {
	*out = result;
	result = {};
	return out->request != 0;
}
static void Cancel( uint64_t request ) {
	assert( request );
	++cancels;
}
static void Connect() {
	clients[0] = {};
	clients[0].state = CS_CONNECTED;
	SV_OpenIdentity( &clients[0] );
}
int main() {
	sv.maxclients = 2;
	svs.clients = clients;
	const byte ticket[4] = { 1, 2, 3, 4 };
	uint64_t id = 123;
	Connect();
	uint64_t token = SV_IdentitySession( 0 );
	assert( token && !SV_PlayerIdentity( 0, &id ) && !id );
	assert( !SV_SubmitIdentityTicket( token, 123, ticket, sizeof( ticket ) ) );
	assert( !Sys_StartServerSearch( SERVICE_BROWSE, "dm" ) );
	serviceProvider_t provider = { Begin, End, Poll, Search, Next, Cancel };
	assert( Sys_InstallServices( &provider ) );
	assert( !SV_SubmitIdentityTicket( token, 0, ticket, sizeof( ticket ) ) );
	assert( !SV_SubmitIdentityTicket( token, 123, ticket, SERVICE_MAX_TICKET + 1 ) );
	assert( SV_SubmitIdentityTicket( token, 123, ticket, sizeof( ticket ) ) );
	assert( starts == 1 && !SV_PlayerIdentity( 0, &id ) && !id ); // Begin is not verification.
	assert( !SV_SubmitIdentityTicket( token, 123, ticket, sizeof( ticket ) ) );
	assert( !Sys_InstallServices( nullptr ) ); // No provider replacement under sessions.
	event = { token, 999, SERVICE_AUTH_VERIFIED };
	SV_PollIdentities();
	assert( !SV_PlayerIdentity( 0, &id ) && !id );
	event = { token, 123, SERVICE_AUTH_VERIFIED };
	SV_PollIdentities();
	assert( SV_PlayerIdentity( 0, &id ) == SERVICE_STEAM && id == 123 );
	event = { token, 123, SERVICE_AUTH_REJECTED };
	SV_PollIdentities();
	assert( drops == 1 && ends == 1 && !SV_PlayerIdentity( 0, &id ) && !id );
	Connect();
	assert( SV_IdentitySession( 0 ) != token );
	event = { token, 123, SERVICE_AUTH_VERIFIED };
	SV_PollIdentities();
	assert( !SV_PlayerIdentity( 0, &id ) && !id ); // Stale callbacks cannot authenticate a reused slot.
	token = SV_IdentitySession( 0 );
	assert( SV_SubmitIdentityTicket( token, 123, ticket, sizeof( ticket ) ) );
	svs.time = 30001;
	SV_PollIdentities();
	assert( drops == 2 && ends == 2 ); // Pending authentication expires.
	Connect();
	token = SV_IdentitySession( 0 );
	accept = false;
	assert( !SV_SubmitIdentityTicket( token, 123, ticket, sizeof( ticket ) ) );
	assert( !SV_PlayerIdentity( 0, &id ) && !id );
	SV_CloseIdentity( &clients[0] );
	assert( ends == 2 ); // Rejected Begin did not create a provider session.

	accept = true;
	Connect();
	token = SV_IdentitySession( 0 );
	assert( SV_SubmitIdentityTicket( token, 123, ticket, sizeof( ticket ) ) );
	clients[1].state = CS_CONNECTED;
	SV_OpenIdentity( &clients[1] );
	assert( !SV_SubmitIdentityTicket( SV_IdentitySession( 1 ), 123, ticket, sizeof( ticket ) ) );
	SV_CloseIdentity( &clients[0] );
	SV_CloseIdentity( &clients[0] );
	assert( ends == 3 ); // Disconnect ends a pending session exactly once.
	assert( !SV_SubmitIdentityTicket( token, 123, ticket, sizeof( ticket ) ) );
	clients[1].netchan.remoteAddress.type = NA_BOT;
	assert( !SV_IdentitySession( 1 ) );

	const uint64_t browse = Sys_StartServerSearch( SERVICE_BROWSE, "dm" );
	assert( browse && searchToken == browse );
	const uint64_t match = Sys_StartServerSearch( SERVICE_MATCH, "dm" );
	assert( match != browse && cancels == 1 && searches == 2 );
	serviceServer_t output;
	result = { browse, 1, SERVICE_SERVER, "127.0.0.1:27960", "stale" };
	assert( !Sys_NextServer( match, &output ) );
	result = { match, 1, SERVICE_SERVER, "127.0.0.1:27960", "local" };
	assert( Sys_NextServer( match, &output ) && !strcmp( output.name, "local" ) );
	result = { match, 1, SERVICE_SERVER, "", "bad" };
	assert( !Sys_NextServer( match, &output ) );
	result = { match, 1, SERVICE_SERVER, "127.0.0.1;quit", "bad" };
	assert( !Sys_NextServer( match, &output ) );
	result = { match, 0, SERVICE_SEARCH_DONE, "", "" };
	assert( Sys_NextServer( match, &output ) && output.state == SERVICE_SEARCH_DONE );
	assert( cancels == 2 );
	Sys_StopServerSearch( match );
	assert( cancels == 2 );
	puts( "PASS: anonymous null provider, pending/verified/revoked/expired identity, slot reuse, provider ownership and discovery generations" );
}

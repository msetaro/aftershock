#include "../../engine/platform/services_public.h"
#include <assert.h>
#include <cstring>
#include <cstdio>

static bool badOutput;
static uint32_t calls, cancels;
static uint64_t lobbyRequest;
static bool User( serviceUser_t *out ) {
	*out = { SERVICE_STEAM, 123, "test player" };
	if ( badOutput )
		memset( out->name, 'x', sizeof( out->name ) );
	return true;
}
static bool RequestTicket( const char *remote ) {
	assert( !strcmp( remote, "ip:127.0.0.1:27960" ) );
	++calls;
	return true;
}
static bool Ticket( serviceTicket_t *out ) {
	*out = {};
	out->id = 123;
	out->size = badOutput ? SERVICE_MAX_TICKET + 1 : 4;
	memcpy( out->bytes, "test", 4 );
	out->accepted = true;
	return true;
}
static bool Presence( const char *key, const char *value ) {
	assert( !strcmp( key, "status" ) && !strcmp( value, "In lobby" ) );
	++calls;
	return true;
}
static bool CreateLobby( uint64_t request, uint32_t members, bool friendsOnly ) {
	assert( members == 8 && friendsOnly );
	lobbyRequest = request;
	++calls;
	return true;
}
static bool JoinLobby( uint64_t request, uint64_t lobby ) {
	assert( lobby == 456 );
	lobbyRequest = request;
	++calls;
	return true;
}
static bool Invite( uint64_t user ) {
	assert( user == 789 );
	++calls;
	return true;
}
static bool Event( serviceEvent_t *out ) {
	*out = { SERVICE_LOBBY_READY, lobbyRequest, 456, 0, true };
	if ( badOutput )
		out->type = (serviceEventType_t)99;
	return true;
}
static bool Achievement( const char *name, bool unlock, bool *achieved ) {
	assert( !strcmp( name, "TEST_WIN" ) );
	*achieved = unlock;
	++calls;
	return true;
}
static bool CloudWrite( const char *name, const void *data, uint32_t size ) {
	assert( !strcmp( name, "profile.asstate" ) && size == 4 && !memcmp( data, "save", 4 ) );
	++calls;
	return true;
}
static int64_t CloudRead( const char *name, void *data, uint32_t capacity ) {
	assert( !strcmp( name, "profile.asstate" ) && capacity == 4 );
	memcpy( data, "save", 4 );
	++calls;
	return badOutput ? 5 : 4;
}
static bool Workshop( uint32_t index, serviceWorkshopItem_t *out ) {
	assert( index == 0 );
	*out = { 42, 4096, "/installed/item", true };
	if ( badOutput )
		memset( out->path, 'x', sizeof( out->path ) );
	return true;
}

static void Exercise( bool available ) {
	serviceUser_t user;
	assert( Sys_LocalUser( &user ) == available );
	assert( user.id == ( available ? 123u : 0u ) );
	assert( user.provider == ( available ? SERVICE_STEAM : SERVICE_ANONYMOUS ) );
	assert( Sys_RequestTicket( "ip:127.0.0.1:27960" ) == available );
	serviceTicket_t ticket;
	assert( Sys_PollTicket( &ticket ) == available );
	assert( ticket.size == ( available ? 4u : 0u ) );
	assert( Sys_SetPresence( "status", "In lobby" ) == available );
	const uint64_t created = Sys_CreateLobby( 8, true );
	assert( ( created != 0 ) == available );
	const uint64_t joined = Sys_JoinLobby( 456 );
	assert( ( joined != 0 ) == available && ( !available || joined != created ) );
	serviceEvent_t event;
	assert( Sys_PollServiceEvent( &event ) == available );
	assert( !available || event.request == joined );
	assert( Sys_InviteToLobby( 789 ) == available );
	bool achieved = true;
	assert( Sys_Achievement( "TEST_WIN", false, &achieved ) == available && !achieved );
	assert( Sys_Achievement( "TEST_WIN", true, &achieved ) == available && achieved == available );
	assert( Sys_CloudWrite( "profile.asstate", "save", 4 ) == available );
	char data[4] = {};
	assert( Sys_CloudRead( "profile.asstate", data, sizeof( data ) ) == ( available ? 4 : -1 ) );
	assert( !available || !memcmp( data, "save", 4 ) );
	serviceWorkshopItem_t item;
	assert( Sys_WorkshopItem( 0, &item ) == available );
	assert( item.id == ( available ? 42u : 0u ) );
	Sys_CancelTicket();
	Sys_LeaveLobby();
}
int main() {
	Exercise( false );
	serviceProvider_t provider = {};
	provider.beginAuth = []( uint64_t, uint64_t, const void *, uint32_t ) { return false; };
	provider.endAuth = []( uint64_t, uint64_t ) {};
	provider.pollAuth = []( serviceAuthEvent_t * ) { return false; };
	provider.startSearch = []( uint64_t, serviceSearch_t, const char * ) { return false; };
	provider.nextServer = []( serviceServer_t * ) { return false; };
	provider.stopSearch = []( uint64_t ) {};
	provider.localUser = User;
	provider.requestTicket = RequestTicket;
	provider.pollTicket = Ticket;
	provider.cancelTicket = []() { ++cancels; };
	provider.setPresence = Presence;
	provider.createLobby = CreateLobby;
	provider.joinLobby = JoinLobby;
	provider.leaveLobby = []() { ++cancels; };
	provider.invite = Invite;
	provider.pollEvent = Event;
	provider.achievement = Achievement;
	provider.cloudWrite = CloudWrite;
	provider.cloudRead = CloudRead;
	provider.workshopItem = Workshop;
	assert( Sys_InstallServices( &provider ) );
	Exercise( true );
	assert( cancels == 2 && !Sys_InstallServices( nullptr ) );
	const uint32_t before = calls;
	char data[4];
	bool achieved;
	assert( !Sys_RequestTicket( "" ) && !Sys_RequestTicket( "ip:127.0.0.1;quit" ) );
	assert( !Sys_SetPresence( "", "value" ) && !Sys_SetPresence( "status", nullptr ) );
	assert( !Sys_CreateLobby( 0, true ) && !Sys_CreateLobby( 251, true ) && !Sys_JoinLobby( 0 ) && !Sys_InviteToLobby( 0 ) );
	assert( !Sys_Achievement( "../bad", true, &achieved ) && !Sys_Achievement( "TEST_WIN", true, nullptr ) );
	assert( !Sys_CloudWrite( "../profile", "save", 4 ) && !Sys_CloudWrite( "profile", nullptr, 4 ) );
	assert( !Sys_CloudWrite( "profile", "save", SERVICE_MAX_CLOUD + 1 ) );
	assert( Sys_CloudRead( "../profile", data, sizeof( data ) ) == -1 );
	assert( !Sys_LocalUser( nullptr ) && !Sys_PollTicket( nullptr ) && !Sys_PollServiceEvent( nullptr ) );
	assert( !Sys_WorkshopItem( 0, nullptr ) && calls == before );
	badOutput = true;
	serviceUser_t user;
	serviceTicket_t ticket;
	serviceEvent_t event;
	serviceWorkshopItem_t item;
	assert( !Sys_LocalUser( &user ) && !user.id );
	assert( !Sys_PollTicket( &ticket ) && !ticket.size );
	assert( !Sys_PollServiceEvent( &event ) );
	assert( !Sys_WorkshopItem( 0, &item ) && !item.id );
	assert( Sys_CloudRead( "profile.asstate", data, sizeof( data ) ) == -1 );
	puts( "PASS: null and installed services share bounded identity/ticket, presence, lobby, achievements, cloud and workshop contracts" );
}

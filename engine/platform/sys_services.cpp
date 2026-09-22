#include "services_public.h"
#include <cstring>

static serviceProvider_t services;
static bool servicesUsed;
static uint64_t searchSerial, activeSearch;

bool Sys_InstallServices( const serviceProvider_t *provider ) {
	if ( servicesUsed )
		return false;
	if ( provider && ( !provider->beginAuth || !provider->endAuth || !provider->pollAuth ||
						 !provider->startSearch || !provider->nextServer || !provider->stopSearch ) )
		return false;
	services = provider ? *provider : serviceProvider_t{};
	return true;
}
bool Sys_AuthAvailable() {
	return services.beginAuth != nullptr;
}
bool Sys_BeginAuth( uint64_t session, uint64_t id, const void *ticket, uint32_t size ) {
	if ( !services.beginAuth || !session || !id || !ticket || !size || size > SERVICE_MAX_TICKET )
		return false;
	servicesUsed = true;
	return services.beginAuth( session, id, ticket, size );
}
void Sys_EndAuth( uint64_t session, uint64_t id ) {
	if ( services.endAuth && session && id )
		services.endAuth( session, id );
}
bool Sys_PollAuth( serviceAuthEvent_t *event ) {
	if ( !event )
		return false;
	*event = {};
	if ( !services.pollAuth || !services.pollAuth( event ) )
		return false;
	return event->session && event->id && ( event->result == SERVICE_AUTH_VERIFIED || event->result == SERVICE_AUTH_REJECTED );
}
uint64_t Sys_StartServerSearch( serviceSearch_t mode, const char *filter ) {
	if ( !services.startSearch || ( mode != SERVICE_BROWSE && mode != SERVICE_MATCH ) || !filter || searchSerial == UINT64_MAX )
		return 0;
	// Small opaque filter, copied by the provider; no engine console interpretation.
	if ( std::strlen( filter ) > 127 )
		return 0;
	Sys_StopServerSearch( activeSearch );
	servicesUsed = true;
	const uint64_t request = ++searchSerial;
	if ( !services.startSearch( request, mode, filter ) )
		return 0;
	activeSearch = request;
	return request;
}
bool Sys_NextServer( uint64_t request, serviceServer_t *server ) {
	if ( !server )
		return false;
	*server = {};
	if ( !request || request != activeSearch )
		return false;
	serviceServer_t result = {};
	if ( !services.nextServer( &result ) || result.request != request ||
		 result.state < SERVICE_SERVER || result.state > SERVICE_SEARCH_FAILED )
		return false;
	if ( result.state == SERVICE_SERVER && ( !result.address[0] ||
											   !std::memchr( result.address, 0, sizeof( result.address ) ) || !std::memchr( result.name, 0, sizeof( result.name ) ) ) )
		return false;
	if ( result.state == SERVICE_SERVER ) {
		for ( const char *p = result.address; *p; ++p ) {
			if ( !( ( *p >= 'a' && *p <= 'z' ) || ( *p >= 'A' && *p <= 'Z' ) || ( *p >= '0' && *p <= '9' ) ||
					 *p == '.' || *p == '-' || *p == '_' || *p == ':' || *p == '[' || *p == ']' ) )
				return false;
		}
	} else {
		result.address[0] = result.name[0] = 0;
	}
	*server = result;
	if ( result.state != SERVICE_SERVER )
		Sys_StopServerSearch( request );
	return true;
}
void Sys_StopServerSearch( uint64_t request ) {
	if ( request && request == activeSearch ) {
		activeSearch = 0;
		services.stopSearch( request );
	}
}

static uint64_t lobbySerial, lobbyRequest, currentLobby;

static bool ServiceName( const char *text, uint32_t maximum ) {
	if ( !text || !text[0] || text[0] == '.' || std::strlen( text ) > maximum )
		return false;
	for ( const char *p = text; *p; ++p )
		if ( !( ( *p >= 'a' && *p <= 'z' ) || ( *p >= 'A' && *p <= 'Z' ) ||
				 ( *p >= '0' && *p <= '9' ) || *p == '_' || *p == '-' || *p == '.' ) )
			return false;
	return true;
}
bool Sys_LocalUser( serviceUser_t *user ) {
	if ( !user )
		return false;
	*user = {};
	serviceUser_t result = {};
	if ( !services.localUser )
		return false;
	servicesUsed = true;
	if ( !services.localUser( &result ) || result.provider != SERVICE_STEAM || !result.id ||
		 !std::memchr( result.name, 0, sizeof( result.name ) ) )
		return false;
	*user = result;
	return true;
}
bool Sys_RequestTicket( const char *remote ) {
	if ( !services.requestTicket || !remote || !remote[0] || std::strlen( remote ) > 127 )
		return false;
	for ( const char *p = remote; *p; ++p )
		if ( !( ( *p >= 'a' && *p <= 'z' ) || ( *p >= 'A' && *p <= 'Z' ) || ( *p >= '0' && *p <= '9' ) ||
				 *p == '.' || *p == '-' || *p == '_' || *p == ':' || *p == '[' || *p == ']' ) )
			return false;
	servicesUsed = true;
	return services.requestTicket( remote );
}
bool Sys_PollTicket( serviceTicket_t *ticket ) {
	if ( !ticket )
		return false;
	*ticket = {};
	serviceTicket_t result = {};
	if ( !services.pollTicket )
		return false;
	servicesUsed = true;
	if ( !services.pollTicket( &result ) || result.size > SERVICE_MAX_TICKET ||
		 ( result.accepted && ( !result.id || !result.size ) ) )
		return false;
	if ( result.accepted )
		*ticket = result;
	return true;
}
void Sys_CancelTicket() {
	if ( services.cancelTicket )
		services.cancelTicket();
}
bool Sys_SetPresence( const char *key, const char *value ) {
	if ( !services.setPresence || !ServiceName( key, 63 ) || !value || std::strlen( value ) > 255 )
		return false;
	servicesUsed = true;
	return services.setPresence( key, value );
}
uint64_t Sys_CreateLobby( uint32_t members, bool friendsOnly ) {
	if ( !services.createLobby || !members || members > 250 || lobbySerial == UINT64_MAX )
		return 0;
	servicesUsed = true;
	const uint64_t request = ++lobbySerial;
	if ( !services.createLobby( request, members, friendsOnly ) )
		return 0;
	currentLobby = 0;
	lobbyRequest = request;
	return request;
}
uint64_t Sys_JoinLobby( uint64_t lobby ) {
	if ( !services.joinLobby || !lobby || lobbySerial == UINT64_MAX )
		return 0;
	servicesUsed = true;
	const uint64_t request = ++lobbySerial;
	if ( !services.joinLobby( request, lobby ) )
		return 0;
	currentLobby = 0;
	lobbyRequest = request;
	return request;
}
void Sys_LeaveLobby() {
	currentLobby = lobbyRequest = 0;
	if ( services.leaveLobby )
		services.leaveLobby();
}
bool Sys_InviteToLobby( uint64_t user ) {
	if ( !services.invite || !user || !currentLobby )
		return false;
	servicesUsed = true;
	return services.invite( user );
}
bool Sys_PollServiceEvent( serviceEvent_t *event ) {
	if ( !event )
		return false;
	*event = {};
	serviceEvent_t result = {};
	if ( !services.pollEvent )
		return false;
	servicesUsed = true;
	if ( !services.pollEvent( &result ) )
		return false;
	switch ( result.type ) {
	case SERVICE_LOBBY_READY:
		if ( !result.request || result.request != lobbyRequest || ( result.accepted && !result.lobby ) )
			return false;
		currentLobby = result.accepted ? result.lobby : 0;
		lobbyRequest = 0;
		break;
	case SERVICE_LOBBY_INVITE:
		if ( !result.lobby || !result.user )
			return false;
		break;
	case SERVICE_LOBBY_LEFT:
		if ( !result.lobby || result.lobby != currentLobby )
			return false;
		currentLobby = 0;
		break;
	default:
		return false;
	}
	*event = result;
	return true;
}
bool Sys_Achievement( const char *name, bool unlock, bool *achieved ) {
	if ( !achieved )
		return false;
	*achieved = false;
	if ( !services.achievement || !ServiceName( name, 127 ) )
		return false;
	servicesUsed = true;
	bool result = false;
	if ( !services.achievement( name, unlock, &result ) )
		return false;
	*achieved = result;
	return true;
}
bool Sys_CloudWrite( const char *name, const void *data, uint32_t size ) {
	if ( !services.cloudWrite || !ServiceName( name, 127 ) || ( size && !data ) || size > SERVICE_MAX_CLOUD )
		return false;
	servicesUsed = true;
	return services.cloudWrite( name, data, size );
}
int64_t Sys_CloudRead( const char *name, void *data, uint32_t capacity ) {
	if ( !services.cloudRead || !ServiceName( name, 127 ) || !data || !capacity || capacity > SERVICE_MAX_CLOUD )
		return -1;
	servicesUsed = true;
	const int64_t size = services.cloudRead( name, data, capacity );
	return size >= 0 && size <= capacity ? size : -1;
}
bool Sys_WorkshopItem( uint32_t index, serviceWorkshopItem_t *item ) {
	if ( !item )
		return false;
	*item = {};
	serviceWorkshopItem_t result = {};
	if ( !services.workshopItem || index >= SERVICE_MAX_WORKSHOP )
		return false;
	servicesUsed = true;
	if ( !services.workshopItem( index, &result ) || !result.id ||
		 !std::memchr( result.path, 0, sizeof( result.path ) ) || ( result.installed && !result.path[0] ) )
		return false;
	*item = result;
	return true;
}

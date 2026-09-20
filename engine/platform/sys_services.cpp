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

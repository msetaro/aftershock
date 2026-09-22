#ifndef SERVICES_PUBLIC_H
#define SERVICES_PUBLIC_H

#include <stdint.h>

// Main-thread only. The provider owns SDK objects and queues callbacks for polling.
// #23 installs Steamworks; an absent provider is anonymous and cannot authenticate.
constexpr uint32_t SERVICE_MAX_TICKET = 2048;
enum serviceIdentity_t { SERVICE_ANONYMOUS,
	SERVICE_STEAM,
	SERVICE_BACKEND };
enum serviceAuthResult_t { SERVICE_AUTH_VERIFIED = 1,
	SERVICE_AUTH_REJECTED };
struct serviceAuthEvent_t {
	uint64_t session, id;
	serviceAuthResult_t result;
};
enum serviceSearch_t { SERVICE_BROWSE,
	SERVICE_MATCH };
enum serviceSearchResult_t { SERVICE_SERVER = 1,
	SERVICE_SEARCH_DONE,
	SERVICE_SEARCH_FAILED };
struct serviceServer_t {
	uint64_t request, id;
	serviceSearchResult_t state;
	char address[128], name[128];
};
struct serviceProvider_t {
	// true means verification started, never that identity was verified. Copy ticket before returning.
	bool ( *beginAuth )( uint64_t session, uint64_t claimedId, const void *ticket, uint32_t size );
	void ( *endAuth )( uint64_t session, uint64_t id );
	bool ( *pollAuth )( serviceAuthEvent_t *event );
	bool ( *startSearch )( uint64_t request, serviceSearch_t mode, const char *filter );
	bool ( *nextServer )( serviceServer_t *server );
	void ( *stopSearch )( uint64_t request );
};
// Installation is immutable after first use. Callbacks never call Com_Error or reenter the engine.
bool Sys_InstallServices( const serviceProvider_t *provider );
bool Sys_AuthAvailable();
bool Sys_BeginAuth( uint64_t session, uint64_t id, const void *ticket, uint32_t size );
void Sys_EndAuth( uint64_t session, uint64_t id );
bool Sys_PollAuth( serviceAuthEvent_t *event );
uint64_t Sys_StartServerSearch( serviceSearch_t mode, const char *filter );
bool Sys_NextServer( uint64_t request, serviceServer_t *server );
void Sys_StopServerSearch( uint64_t request );

#endif

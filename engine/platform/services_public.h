#ifndef SERVICES_PUBLIC_H
#define SERVICES_PUBLIC_H

#include <stdint.h>

// Main-thread only. The provider owns SDK objects and queues callbacks for polling.
// #180 installs Steamworks; an absent provider is anonymous and cannot authenticate.
constexpr uint32_t SERVICE_MAX_TICKET = 2048;
enum serviceIdentity_t { SERVICE_ANONYMOUS,
	SERVICE_STEAM };
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
constexpr uint32_t SERVICE_MAX_CLOUD = 64 * 1024 * 1024;
constexpr uint32_t SERVICE_MAX_WORKSHOP = 256;
struct serviceUser_t {
	serviceIdentity_t provider;
	uint64_t id;
	char name[128];
};
struct serviceTicket_t {
	uint64_t id;
	uint32_t size;
	unsigned char bytes[SERVICE_MAX_TICKET];
	bool accepted;
};
enum serviceEventType_t : uint32_t { SERVICE_LOBBY_READY = 1,
	SERVICE_LOBBY_INVITE,
	SERVICE_LOBBY_LEFT };
struct serviceEvent_t {
	serviceEventType_t type;
	uint64_t request, lobby, user;
	bool accepted;
};
struct serviceWorkshopItem_t {
	uint64_t id, bytes;
	char path[1024];
	bool installed;
};
struct serviceProvider_t {
	// true means verification started, never that identity was verified. Copy ticket before returning.
	bool ( *beginAuth )( uint64_t session, uint64_t claimedId, const void *ticket, uint32_t size );
	void ( *endAuth )( uint64_t session, uint64_t id );
	bool ( *pollAuth )( serviceAuthEvent_t *event );
	bool ( *startSearch )( uint64_t request, serviceSearch_t mode, const char *filter );
	bool ( *nextServer )( serviceServer_t *server );
	void ( *stopSearch )( uint64_t request );
	// Optional user services. An absent callback explicitly means unavailable.
	bool ( *localUser )( serviceUser_t *user ) = nullptr;
	bool ( *requestTicket )( const char *remote ) = nullptr;
	bool ( *pollTicket )( serviceTicket_t *ticket ) = nullptr;
	void ( *cancelTicket )() = nullptr;
	bool ( *setPresence )( const char *key, const char *value ) = nullptr;
	bool ( *createLobby )( uint64_t request, uint32_t members, bool friendsOnly ) = nullptr;
	bool ( *joinLobby )( uint64_t request, uint64_t lobby ) = nullptr;
	void ( *leaveLobby )() = nullptr;
	bool ( *invite )( uint64_t user ) = nullptr;
	bool ( *pollEvent )( serviceEvent_t *event ) = nullptr;
	bool ( *achievement )( const char *name, bool unlock, bool *achieved ) = nullptr;
	bool ( *cloudWrite )( const char *name, const void *data, uint32_t size ) = nullptr;
	int64_t ( *cloudRead )( const char *name, void *data, uint32_t capacity ) = nullptr;
	bool ( *workshopItem )( uint32_t index, serviceWorkshopItem_t *item ) = nullptr;
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

// All strings are terminated caller-owned inputs; outputs are owned POD copies.
// Null services return false/-1 and never manufacture an authenticated identity.
bool Sys_LocalUser( serviceUser_t *user );
bool Sys_RequestTicket( const char *remote );
bool Sys_PollTicket( serviceTicket_t *ticket );
void Sys_CancelTicket();
bool Sys_SetPresence( const char *key, const char *value );
uint64_t Sys_CreateLobby( uint32_t members, bool friendsOnly );
uint64_t Sys_JoinLobby( uint64_t lobby );
void Sys_LeaveLobby();
bool Sys_InviteToLobby( uint64_t user );
bool Sys_PollServiceEvent( serviceEvent_t *event );
bool Sys_Achievement( const char *name, bool unlock, bool *achieved );
// Cloud names are flat portable filenames; operations run only on explicit user actions.
bool Sys_CloudWrite( const char *name, const void *data, uint32_t size );
int64_t Sys_CloudRead( const char *name, void *data, uint32_t capacity );
bool Sys_WorkshopItem( uint32_t index, serviceWorkshopItem_t *item );

#endif

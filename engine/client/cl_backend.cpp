#include "client.h"
#include "../platform/http_public.h"
#include "../platform/services_public.h"
#include "../qcommon/json.h"
#include <charconv>
#include <cstring>

// Only public presentation/configuration is exposed to UI/cvars. Credentials
// remain in this process and the one platform-owned request until completion.
enum backendOperation_t { BACKEND_NONE,
	BACKEND_TICKET,
	BACKEND_LOGIN,
	BACKEND_PROFILE,
	BACKEND_QUEUE,
	BACKEND_RESULTS,
	BACKEND_LOGOUT };
static struct {
	char session[65], player[21], name[65], status[96], match[65], ticket[257];
	char score[32], kills[32], deaths[32];
	netadr_t address;
	backendOperation_t operation;
	uint32_t nextPoll;
	bool queued;
} backend;
static cvar_t *backendURL, *backendCA, *backendMap;

static void BackendStatus( const char *text ) {
	Q_strncpyz( backend.status, text, sizeof( backend.status ) );
}
static bool BackendIdentifier( const char *text ) {
	if ( !*text )
		return false;
	for ( const unsigned char *p = (const unsigned char *)text; *p; ++p )
		if ( !( ( *p >= 'a' && *p <= 'z' ) || ( *p >= 'A' && *p <= 'Z' ) || ( *p >= '0' && *p <= '9' ) || *p == '_' || *p == '-' ) )
			return false;
	return true;
}
template <size_t N>
static bool BackendString( const char *json, const char *end, const char *key, char ( &out )[N] ) {
	const char *value = JSON_ObjectGetNamedValue( json, end, key );
	return value && JSON_ReadString( value, end, out, uint32_t( N ) );
}
template <typename T>
static bool BackendNumber( const char *json, const char *end, const char *key, T &out ) {
	const char *value = JSON_ObjectGetNamedValue( json, end, key );
	if ( !value )
		return false;
	const char *last = JSON_SkipValue( value, end );
	if ( !last )
		return false;
	const auto result = std::from_chars( value, last, out );
	return result.ec == std::errc{} && result.ptr == last;
}
static bool BackendRequest( backendOperation_t operation, const char *method, const char *path, const char *body ) {
	char url[1024];
	if ( !backendURL || !backendURL->string[0] || strlen( backendURL->string ) + strlen( path ) >= sizeof( url ) ) {
		BackendStatus( "Online service unavailable" );
		return false;
	}
	Com_sprintf( url, sizeof( url ), "%s%s", backendURL->string, path );
	if ( !Sys_HTTPStart( url, method, backend.session, body, backendCA->string ) ) {
		BackendStatus( "Online service unavailable" );
		return false;
	}
	backend.operation = operation;
	return true;
}
static void BackendLoginTicket( const unsigned char *ticket, uint32_t size ) {
	if ( !size || size > SERVICE_MAX_TICKET ) {
		BackendStatus( "Sign-in failed" );
		Sys_CancelTicket();
		return;
	}
	char body[SERVICE_MAX_TICKET * 2 + 16];
	memcpy( body, "{\"ticket\":\"", 11 );
	static const char hex[] = "0123456789abcdef";
	for ( uint32_t i = 0; i < size; ++i ) {
		body[11 + i * 2] = hex[ticket[i] >> 4];
		body[12 + i * 2] = hex[ticket[i] & 15];
	}
	memcpy( body + 11 + size * 2, "\"}", 3 );
	if ( !BackendRequest( BACKEND_LOGIN, "POST", "/v1/login", body ) )
		Sys_CancelTicket();
	memset( body, 0, sizeof( body ) );
}
static bool BackendReply( backendOperation_t operation, const httpResult_t &reply ) {
	const char *json = reply.body, *end = json + reply.size;
	int version = 0;
	if ( !JSON_ValidateObject( json, end ) || !BackendNumber( json, end, "version", version ) || version != 1 )
		return false;
	if ( operation == BACKEND_LOGIN ) {
		char token[65], player[21];
		uint64_t id = 0;
		if ( !BackendString( json, end, "token", token ) || strlen( token ) != 64 || strspn( token, "0123456789abcdef" ) != 64 ||
			 !BackendString( json, end, "player_id", player ) || player[0] == '0' )
			return false;
		const auto parsed = std::from_chars( player, player + strlen( player ), id );
		if ( parsed.ec != std::errc{} || parsed.ptr != player + strlen( player ) || !id )
			return false;
		Q_strncpyz( backend.session, token, sizeof( backend.session ) );
		Q_strncpyz( backend.player, player, sizeof( backend.player ) );
		BackendStatus( "Signed in" );
		return true;
	}
	if ( operation == BACKEND_PROFILE ) {
		char player[21], name[65];
		if ( !BackendString( json, end, "player_id", player ) || strcmp( player, backend.player ) || !BackendString( json, end, "display_name", name ) )
			return false;
		for ( const unsigned char *p = (const unsigned char *)name; *p; ++p )
			if ( *p < 32 || *p == 127 )
				return false;
		Q_strncpyz( backend.name, name, sizeof( backend.name ) );
		BackendStatus( "Profile loaded" );
		return true;
	}
	if ( operation == BACKEND_QUEUE ) {
		char state[16], match[65];
		if ( !BackendString( json, end, "state", state ) )
			return false;
		if ( !strcmp( state, "idle" ) ) {
			backend.queued = false;
			BackendStatus( "Signed in" );
			return true;
		}
		if ( !BackendString( json, end, "match_id", match ) || !BackendIdentifier( match ) )
			return false;
		if ( reply.status == 202 && ( !strcmp( state, "queued" ) || !strcmp( state, "starting" ) ) ) {
			Q_strncpyz( backend.match, match, sizeof( backend.match ) );
			backend.queued = true;
			backend.nextPoll = uint32_t( cls.realtime ) + 1000;
			BackendStatus( "Waiting for match" );
			return true;
		}
		char address[128], ticket[257], prefix[96];
		netadr_t target;
		if ( reply.status != 200 || strcmp( state, "allocated" ) || !BackendString( json, end, "address", address ) || !BackendString( json, end, "ticket", ticket ) )
			return false;
		if ( strspn( address, "0123456789abcdefABCDEF.:[]" ) != strlen( address ) || strchr( address, ' ' ) || !strchr( address, ':' ) )
			return false;
		if ( !NET_StringToAdr( address, &target, NA_UNSPEC ) || ( target.type != NA_IP && target.type != NA_IP6 ) )
			return false;
		Com_sprintf( prefix, sizeof( prefix ), "1.%s.%s.", backend.player, match );
		if ( strncmp( ticket, prefix, strlen( prefix ) ) || strspn( ticket, "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_.-" ) != strlen( ticket ) )
			return false;
		char command[160];
		Com_sprintf( command, sizeof( command ), "connect %s\n", address );
		backend.queued = false;
		Cbuf_ExecuteText( EXEC_NOW, command ); // Ordinary connect clears the previous ticket first.
		if ( cls.state != CA_CONNECTING && cls.state != CA_CHALLENGING )
			return false;
		if ( !NET_CompareAdr( &target, &clc.serverAddress ) )
			return false;
		backend.address = target;
		Q_strncpyz( backend.ticket, ticket, sizeof( backend.ticket ) );
		Q_strncpyz( backend.match, match, sizeof( backend.match ) );
		BackendStatus( "Joining match" );
		return true;
	}
	if ( operation == BACKEND_RESULTS ) {
		const char *array = JSON_ObjectGetNamedValue( json, end, "results" );
		if ( !array || JSON_ValueGetType( array, end ) != JSONTYPE_ARRAY )
			return false;
		int count = 0, selectedScore = 0, selectedKills = 0, selectedDeaths = 0;
		char selectedMatch[65] = {};
		bool found = false;
		for ( const char *row = JSON_ArrayGetFirstValue( array, end ); row; row = JSON_ArrayGetNextValue( row, end ) ) {
			if ( ++count > 20 )
				return false;
			char match[65];
			int score, kills, deaths;
			const char *stats = JSON_ObjectGetNamedValue( row, end, "stats" );
			if ( !BackendString( row, end, "match", match ) || !BackendIdentifier( match ) || !stats || !BackendNumber( stats, end, "score", score ) || !BackendNumber( stats, end, "kills", kills ) || !BackendNumber( stats, end, "deaths", deaths ) || kills < 0 || deaths < 0 )
				return false;
			if ( !found && ( !backend.match[0] || !strcmp( match, backend.match ) ) ) {
				selectedScore = score;
				selectedKills = kills;
				selectedDeaths = deaths;
				Q_strncpyz( selectedMatch, match, sizeof( selectedMatch ) );
				found = true;
			}
		}
		backend.score[0] = backend.kills[0] = backend.deaths[0] = 0;
		if ( found ) {
			Com_sprintf( backend.score, sizeof( backend.score ), "%d", selectedScore );
			Com_sprintf( backend.kills, sizeof( backend.kills ), "%d", selectedKills );
			Com_sprintf( backend.deaths, sizeof( backend.deaths ), "%d", selectedDeaths );
			Q_strncpyz( backend.match, selectedMatch, sizeof( backend.match ) );
		}
		BackendStatus( found ? "Results loaded" : "No results yet" );
		return true;
	}
	return false;
}
void CL_BackendAction( const char *action ) {
	if ( !action )
		return;
	if ( !strcmp( action, "logout" ) ) {
		Sys_HTTPCancel();
		Sys_CancelTicket();
		backend.operation = BACKEND_NONE;
		backend.queued = false;
		const bool sent = backend.session[0] && BackendRequest( BACKEND_LOGOUT, "POST", "/v1/logout", "{}" );
		backend = {};
		backend.operation = sent ? BACKEND_LOGOUT : BACKEND_NONE;
		BackendStatus( "Signed out" );
		return;
	}
	if ( backend.operation != BACKEND_NONE )
		return;
	if ( !strcmp( action, "login" ) ) {
		backend = {};
		if ( Sys_RequestTicket( "aftershock" ) ) {
			backend.operation = BACKEND_TICKET;
			backend.nextPoll = uint32_t( cls.realtime ) + 10000;
			BackendStatus( "Signing in" );
		} else
			BackendStatus( "Platform sign-in unavailable" );
		return;
	}
	if ( !backend.session[0] ) {
		BackendStatus( "Sign in first" );
		return;
	}
	if ( !strcmp( action, "queue" ) ) {
		if ( !BackendIdentifier( backendMap->string ) || strlen( backendMap->string ) > 48 ) {
			BackendStatus( "Invalid match map" );
			return;
		}
		char body[96];
		Com_sprintf( body, sizeof( body ), "{\"map\":\"%s\",\"mode\":0}", backendMap->string );
		if ( BackendRequest( BACKEND_QUEUE, "POST", "/v1/queue", body ) )
			BackendStatus( "Finding match" );
	} else if ( !strcmp( action, "profile" ) )
		BackendRequest( BACKEND_PROFILE, "GET", "/v1/profile", "" );
	else if ( !strcmp( action, "results" ) ) {
		char path[96];
		Com_sprintf( path, sizeof( path ), "/v1/results%s%s", backend.match[0] ? "?match=" : "", backend.match );
		BackendRequest( BACKEND_RESULTS, "GET", path, "" );
	}
}
void CL_BackendFrame() {
	if ( backend.operation == BACKEND_TICKET ) {
		if ( uint32_t( uint32_t( cls.realtime ) - backend.nextPoll ) < 0x80000000u ) {
			Sys_CancelTicket();
			backend.operation = BACKEND_NONE;
			BackendStatus( "Sign-in timed out" );
			return;
		}
		serviceTicket_t ticket;
		if ( !Sys_PollTicket( &ticket ) )
			return;
		backend.operation = BACKEND_NONE;
		if ( ticket.accepted )
			BackendLoginTicket( ticket.bytes, ticket.size );
		else {
			BackendStatus( "Sign-in failed" );
			Sys_CancelTicket();
		}
		memset( &ticket, 0, sizeof( ticket ) );
	}
	static httpResult_t reply;
	if ( backend.operation != BACKEND_NONE && Sys_HTTPPoll( &reply ) ) {
		const auto operation = backend.operation;
		backend.operation = BACKEND_NONE;
		if ( operation == BACKEND_LOGIN )
			Sys_CancelTicket();
		if ( operation == BACKEND_LOGOUT )
			BackendStatus( "Signed out" );
		else if ( reply.failed || reply.status < 200 || reply.status >= 300 ) {
			if ( operation == BACKEND_QUEUE && ( reply.failed || reply.status == 503 ) ) {
				backend.queued = true;
				backend.nextPoll = uint32_t( cls.realtime ) + 1000;
				BackendStatus( "Waiting for match" );
			} else {
				backend.queued = false;
				BackendStatus( reply.status == 401 ? "Sign-in expired" : "Online request failed" );
				if ( reply.status == 401 )
					memset( backend.session, 0, sizeof( backend.session ) );
			}
		} else if ( !BackendReply( operation, reply ) ) {
			backend.queued = false;
			BackendStatus( "Invalid service response" );
		}
		memset( &reply, 0, sizeof( reply ) );
	}
	if ( backend.operation == BACKEND_NONE && backend.queued && uint32_t( uint32_t( cls.realtime ) - backend.nextPoll ) < 0x80000000u ) {
		backend.nextPoll = uint32_t( cls.realtime ) + 1000;
		BackendRequest( BACKEND_QUEUE, "GET", "/v1/queue", "" );
	}
}
bool CL_BackendConnectInfo( const netadr_t &address, char *info, int capacity ) {
	Info_RemoveKey( info, "as_ticket" );
	if ( !backend.ticket[0] )
		return true;
	return NET_CompareAdr( &backend.address, &address ) && Info_SetValueForKey_s( info, capacity, "as_ticket", backend.ticket );
}
void CL_BackendDisconnected() {
	memset( backend.ticket, 0, sizeof( backend.ticket ) );
}
void CL_BackendConnected() {
	CL_BackendDisconnected();
	if ( backend.session[0] )
		BackendStatus( "Playing" );
}
const char *CL_BackendValue( const char *name ) {
	if ( !strcmp( name, "backend_status" ) )
		return backend.status;
	if ( !strcmp( name, "backend_name" ) )
		return backend.name;
	if ( !strcmp( name, "backend_match" ) )
		return backend.match;
	if ( !strcmp( name, "backend_score" ) )
		return backend.score;
	if ( !strcmp( name, "backend_kills" ) )
		return backend.kills;
	if ( !strcmp( name, "backend_deaths" ) )
		return backend.deaths;
	return "";
}
static void BackendLogin() {
	CL_BackendAction( "login" );
}
static void BackendQueue() {
	CL_BackendAction( "queue" );
}
static void BackendProfile() {
	CL_BackendAction( "profile" );
}
static void BackendResults() {
	CL_BackendAction( "results" );
}
static void BackendLogout() {
	CL_BackendAction( "logout" );
}
#ifdef AFTERSHOCK_DEVTOOLS
static void BackendDevLogin() {
	Sys_HTTPCancel();
	Sys_CancelTicket();
	backend = {};
	fileHandle_t file;
	const int length = FS_SV_FOpenFileRead( "backend-ticket.bin", &file );
	unsigned char ticket[SERVICE_MAX_TICKET];
	bool valid = file != FS_INVALID_HANDLE && length > 0 && length <= int( sizeof( ticket ) );
	if ( file != FS_INVALID_HANDLE ) {
		valid = valid && FS_Read( ticket, length, file ) == length;
		FS_FCloseFile( file );
	}
	if ( valid )
		BackendLoginTicket( ticket, uint32_t( length ) );
	else
		BackendStatus( "Test ticket unavailable" );
	memset( ticket, 0, sizeof( ticket ) );
}
#endif
void CL_BackendInit() {
	backend = {};
	BackendStatus( "Signed out" );
	backendURL = Cvar_Get( "backend_url", "", CVAR_ARCHIVE | CVAR_INIT );
	backendCA = Cvar_Get( "backend_ca", "", CVAR_INIT );
	backendMap = Cvar_Get( "backend_map", "two_lane", CVAR_ARCHIVE );
	Cmd_AddCommand( "backend_login", BackendLogin );
	Cmd_AddCommand( "backend_queue", BackendQueue );
	Cmd_AddCommand( "backend_profile", BackendProfile );
	Cmd_AddCommand( "backend_results", BackendResults );
	Cmd_AddCommand( "backend_logout", BackendLogout );
#ifdef AFTERSHOCK_DEVTOOLS
	Cmd_AddCommand( "backend_dev_login", BackendDevLogin );
#endif
}
void CL_BackendShutdown() {
	Sys_HTTPCancel();
	Sys_CancelTicket();
	backend = {};
	Cmd_RemoveCommand( "backend_login" );
	Cmd_RemoveCommand( "backend_queue" );
	Cmd_RemoveCommand( "backend_profile" );
	Cmd_RemoveCommand( "backend_results" );
	Cmd_RemoveCommand( "backend_logout" );
#ifdef AFTERSHOCK_DEVTOOLS
	Cmd_RemoveCommand( "backend_dev_login" );
#endif
}

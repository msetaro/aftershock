#include "../../engine/client/cl_backend.cpp"
#include <assert.h>
#include <cstdio>
#include <cstring>

clientStatic_t cls;
clientConnection_t clc;
static cvar_t variables[8];
static char variableNames[8][64], variableValues[8][1024];
static int variableCount;
static bool ticketRequested, replyReady;
static httpResult_t nextReply;
static char requestURL[1024], requestToken[129], requestData[8193], consoleCommand[1024];
static int requests;

cvar_t *Cvar_Get( const char *name, const char *value, int flags ) {
	for ( int i = 0; i < variableCount; ++i )
		if ( !strcmp( variableNames[i], name ) )
			return &variables[i];
	assert( variableCount < 8 );
	const int i = variableCount++;
	strcpy( variableNames[i], name );
	strcpy( variableValues[i], !strcmp( name, "backend_url" ) ? "https://backend.test" : value );
	variables[i].name = variableNames[i];
	variables[i].string = variableValues[i];
	variables[i].flags = flags;
	return &variables[i];
}
void Cvar_CheckRange( cvar_t *, const char *, const char *, cvarValidator_t ) {
}
void Cmd_AddCommand( const char *, xcommand_t ) {
}
void Cmd_RemoveCommand( const char * ) {
}
void QDECL Com_Printf( const char *, ... ) {
}
void QDECL Com_DPrintf( const char *, ... ) {
}
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}
bool Sys_RequestTicket( const char *remote ) {
	assert( !strcmp( remote, "aftershock" ) );
	ticketRequested = true;
	return true;
}
bool Sys_PollTicket( serviceTicket_t *out ) {
	if ( !ticketRequested )
		return false;
	*out = {};
	out->accepted = true;
	out->id = 123;
	out->size = 2;
	out->bytes[0] = 0xaa;
	ticketRequested = false;
	return true;
}
void Sys_CancelTicket() {
	ticketRequested = false;
}
bool Sys_HTTPStart( const char *url, const char *, const char *token, const char *body, const char * ) {
	strcpy( requestURL, url );
	strcpy( requestToken, token );
	strcpy( requestData, body );
	++requests;
	return true;
}
bool Sys_HTTPPoll( httpResult_t *out ) {
	if ( !replyReady )
		return false;
	*out = nextReply;
	replyReady = false;
	return true;
}
void Sys_HTTPCancel() {
	replyReady = false;
}
int NET_StringToAdr( const char *text, netadr_t *address, netadrtype_t ) {
	if ( strcmp( text, "127.0.0.1:27960" ) )
		return 0;
	*address = {};
	address->type = NA_IP;
	address->ip[0] = 127;
	address->ip[3] = 1;
	address->port = 1234;
	return 1;
}
qboolean NET_CompareAdr( const netadr_t *a, const netadr_t *b ) {
	return !memcmp( a, b, sizeof( *a ) ) ? qtrue : qfalse;
}
void Cbuf_ExecuteText( cbufExec_t, const char *text ) {
	strcpy( consoleCommand, text );
	CL_BackendDisconnected();
	assert( NET_StringToAdr("127.0.0.1:27960",&clc.serverAddress,NA_UNSPEC) );
	cls.state = CA_CHALLENGING;
}
static void Reply( int status, const char *body ) {
	nextReply = {};
	nextReply.status = status;
	nextReply.size = uint32_t( strlen( body ) );
	strcpy( nextReply.body, body );
	replyReady = true;
	CL_BackendFrame();
}
int main() {
	CL_BackendInit();
	CL_BackendAction( "queue" );
	assert(requests==0);
	CL_BackendAction( "login" );
	CL_BackendFrame();
	assert(requests==1 && !strcmp(requestURL,"https://backend.test/v1/login"));
	assert(!strcmp(requestData,"{\"ticket\":\"aa00\"}") && !*requestToken);
	Reply( 200, "{\"version\":1,\"player_id\":\"123\",\"token\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\"}" );
	assert(!strcmp(CL_BackendValue("backend_status"),"Signed in"));
	assert(!*CL_BackendValue("token"));
	CL_BackendAction( "profile" );
	assert(strlen(requestToken)==64);
	Reply( 200, "{\"version\":1,\"player_id\":\"456\",\"display_name\":\"Wrong owner\",\"loadout\":null}" );
	assert(strcmp(CL_BackendValue("backend_name"),"Wrong owner"));
	CL_BackendAction( "queue" );
	assert(strstr(requestURL,"/v1/queue") && strstr(requestData,"two_lane"));
	Reply( 202, "{\"version\":1,\"state\":\"starting\",\"match_id\":\"match-1\"}" );
	cls.realtime += 1001;
	CL_BackendFrame();
	assert(!strcmp(requestData,""));
	const char *ticket = "1.123.match-1.2000.2120.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
	char json[1024];
	snprintf( json, sizeof( json ), "{\"version\":1,\"state\":\"allocated\",\"match_id\":\"match-1\",\"address\":\"127.0.0.1:27960\",\"ticket\":\"%s\",\"expires\":2120}", ticket );
	Reply( 200, json );
	assert(!strcmp(consoleCommand,"connect 127.0.0.1:27960\n") && !strstr(consoleCommand,ticket));
	char info[1024] = "\\name\\Player";
	netadr_t wrong = clc.serverAddress;
	wrong.port++;
	assert(!CL_BackendConnectInfo(wrong,info,sizeof(info)) && !strstr(info,"as_ticket"));
	assert(CL_BackendConnectInfo(clc.serverAddress,info,sizeof(info)) && !strcmp(Info_ValueForKey(info,"as_ticket"),ticket));
	strcpy( info, "\\name\\Player" );
	assert(CL_BackendConnectInfo(clc.serverAddress,info,sizeof(info)) && strstr(info,ticket));
	CL_BackendConnected();
	strcpy( info, "\\name\\Player" );
	assert(CL_BackendConnectInfo(clc.serverAddress,info,sizeof(info)) && !strstr(info,"as_ticket"));
	CL_BackendAction( "logout" );
	Reply( 503, "{}" );
	const int before = requests;
	CL_BackendAction( "profile" );
	assert(requests==before);
	CL_BackendShutdown();
	puts( "PASS: native backend login ownership, ephemeral address-bound ticket retries and logout" );
}

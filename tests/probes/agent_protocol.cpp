// Exercise the production command layer without a display or running server.
#include "../../engine/devtools/dev_agent.cpp"
#include <assert.h>

void Com_DeveloperMemory( devMemory_t *memory ) {
	*memory = {};
	for ( auto &name : memory->names )
		name = "stub";
}
int Key_StringToKeynum( const char * ) {
	return -1;
}
void Sys_QueEvent( int, sysEventType_t, int, int, int, void * ) {
}
const refexport_t *DevTools_Renderer( void ) {
	return nullptr;
}
bool DevTools_SelectAsset( const char *, int ) {
	return false;
}
bool DevTools_MaterialPreview( int, bool ) {
	return false;
}
bool DevTools_SetMaterial( int, const materialParams_t * ) {
	return false;
}
bool DevTools_SelectPanel( const char * ) {
	return false;
}
bool DevTools_SelectCvar( const char * ) {
	return false;
}
bool DevTools_Filter( const char *, const char * ) {
	return false;
}
const cvar_t *Cvar_First( void ) {
	return nullptr;
}
bool DevTools_SetWorld( bool, bool, bool, float ) {
	return false;
}
bool DevTools_LoadAnimation( const char *, const char * ) {
	return false;
}
bool DevTools_SelectEntity( int ) {
	return true;
}
int DevTools_PickCrosshair( void ) {
	return -1;
}
bool DevTools_EntityAtCamera( float * ) {
	return false;
}
bool DevTools_ReloadEntities( void ) {
	return false;
}
bool DevTools_Range( const char *, const char *, int ) {
	return true;
}
bool DevTools_Graph( const char *, const char *, float ) {
	return true;
}
const animAsset_t *DevTools_GraphAsset( const float ** ) {
	return nullptr;
}
bool DevTools_SetAnimation( const char *, float ) {
	return false;
}
void DevTools_EditorState( devEditorState_t *state ) {
	*state = {};
}
int DevTools_ViewClient( void ) {
	return -1;
}
const devGameTools_t *DevTools_Game( void ) {
	return nullptr;
}
bool DevTools_SaveEntities( void ) {
	return false;
}
uint32_t DevTools_CpuTimings( const devCpuTiming_t **cpu ) {
	*cpu = nullptr;
	return 0;
}
const devNetwork_t *DevTools_Network( void ) {
	static devNetwork_t network;
	return &network;
}
qboolean FS_FileExists( const char * ) {
	return qfalse;
}
int FS_ReadFile( const char *, void ** ) {
	return -1;
}
const refdef_t *DevTools_View( void ) {
	return nullptr;
}
bool CL_AgentPlayer( playerState_t * ) {
	return false;
}

static char value[128] = "initial";
static char queued[256];
static cvar_t variable;

unsigned Cvar_Flags( const char *name ) {
	if ( !strcmp( name, "readonly" ) )
		return CVAR_ROM;
	return !strcmp( name, "example" ) ? 0 : CVAR_NONEXISTENT;
}
const char *Cvar_VariableString( const char * ) {
	return value;
}
cvar_t *Cvar_Set2( const char *name, const char *text, qboolean force ) {
	assert( !force );
	if ( !strcmp( name, "readonly" ) )
		return &variable;
	Q_strncpyz( value, text, sizeof( value ) );
	variable.string = value;
	return &variable;
}
void Cbuf_AddText( const char *text ) {
	Q_strcat( queued, sizeof( queued ), text );
}
void QDECL Com_Printf( const char *, ... ) {
	// Responses must use the caller's buffer, never console prose.
	abort();
}
void Com_Quit_f( void ) {
	abort();
}
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}
static void request( const char *text ) {
	char response[4096];
	assert( DevTools_AgentRequest( text, (uint32_t)strlen( text ), response, sizeof( response ) ) );
	puts( response );
}
bool Sys_AgentWrite( const char *text, uint32_t length ) {
	const bool written = fwrite( text, 1, length, stdout ) == length;
	fflush( stdout );
	return written;
}
int main( int argc, char **argv ) {
	if ( argc > 1 ) {
		DevTools_AgentEnable();
		char response[1024];
		const char *subscribe = R"({"id":1,"op":"subscribe","enabled":true})";
		assert( DevTools_AgentRequest( subscribe, (uint32_t)strlen( subscribe ), response, sizeof( response ) ) );
		if ( argc > 2 )
			for ( int i = 0; i < 257; ++i )
				Dev_AgentEvent( "warning", -1, -1, 0, "queued before assert" );
		if ( !strcmp( argv[1], "--error" ) ) {
			Dev_AgentEvent( "error", -1, -1, 0, "agent error contract" );
			DevTools_AgentFlushEvents();
			return 0;
		}
		Q_ASSERT( false && "agent assertion contract" );
		return 2;
	}
	request( R"({"id":1,"op":"hello"})" );
	request( R"({"id":2,"op":"cvar.get","name":"example"})" );
	request( R"({"id":3,"op":"cvar.set","name":"example","value":"quote \" slash \\ newline\n"})" );
	assert( !strcmp( value, "quote \" slash \\ newline\n" ) );
	request( R"({"id":4,"op":"cvar.get","name":"example"})" );
	request( R"({"id":5,"op":"cvar.set","name":"readonly","value":"changed"})" );
	assert( !strcmp( value, "quote \" slash \\ newline\n" ) );
	request( R"({"id":6,"op":"exec","command":"echo agent"})" );
	assert( !strcmp( queued, "echo agent\n" ) );
	request( R"({"id":7,"op":"missing"})" );
	request( R"({"id":8,"op":"cvar.set","name":"example","value":42})" );
	assert( !strcmp( value, "quote \" slash \\ newline\n" ) );
	request( R"({"id":9,"op":"cvar.get","name":"missing"})" );
	char tiny[2] = { 'x', 'y' };
	const char *text = R"({"id":10,"op":"exec","command":"must not run"})";
	assert( !DevTools_AgentRequest( text, (uint32_t)strlen( text ), tiny, sizeof( tiny ) ) );
	assert( tiny[1] == 'y' && !strcmp( queued, "echo agent\n" ) );
}

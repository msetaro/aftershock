// Exercise the production command layer without a display or running server.
#include "../../engine/devtools/dev_agent.cpp"
#include <assert.h>

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
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}
static void request( const char *text ) {
	char response[4096];
	assert( DevTools_AgentRequest( text, (uint32_t)strlen( text ), response, sizeof( response ) ) );
	puts( response );
}
int main() {
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

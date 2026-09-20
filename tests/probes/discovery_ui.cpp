#include "../../engine/client/cl_ui.cpp"
#include <assert.h>

void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}

static uint64_t active;
static int cancellations;
uint64_t Sys_StartServerSearch( serviceSearch_t mode, const char *filter ) {
	assert( mode == SERVICE_BROWSE && !strcmp( filter, "dm" ) );
	return active = 7;
}
void Sys_StopServerSearch( uint64_t token ) {
	if ( token && token == active ) {
		++cancellations;
		active = 0;
	}
}
bool Sys_NextServer( uint64_t request, serviceServer_t *out ) {
	if ( request != active || !active )
		return false;
	*out = { request, 0, SERVICE_SERVER, "127.0.0.1:27960", "local" };
	return true;
}
int main() {
	char address[128], name[128];
	const auto request = UIImport_StartServerSearch( 0, "dm" );
	assert( UIImport_NextServerSearch( request, address, sizeof( address ), name, sizeof( name ) ) == SERVICE_SERVER );
	assert( !strcmp( address, "127.0.0.1:27960" ) && !strcmp( name, "local" ) );
	assert( UIImport_NextServerSearch( request + 1, address, sizeof( address ), name, sizeof( name ) ) == 0 );
	assert( !address[0] && !name[0] );
	assert( UIImport_NextServerSearch( request, address, 2, name, sizeof( name ) ) == SERVICE_SEARCH_FAILED );
	assert( !address[0] && !name[0] && cancellations == 1 && !uiServiceSearch );
	UIImport_StopServerSearch( request );
	assert( cancellations == 1 );
	puts( "PASS: UI discovery copies, stale requests and terminal buffer failure cancel ownership" );
}

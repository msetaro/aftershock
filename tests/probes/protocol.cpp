#include "../../engine/qcommon/q_shared.h"
#include "../../engine/qcommon/qcommon_public.h"
#include <assert.h>

cvar_t *cl_shownet;
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}
void QDECL Com_Printf( const char *, ... ) {
}
void QDECL Com_DPrintf( const char *, ... ) {
}

int main( int argc, char **argv ) {
	const char *schema = MSG_ReplicationSchema();
	assert( NET_ProtocolCompatible( XSTRING( AFTERSHOCK_NET_VERSION ), schema ) );
	assert( !NET_ProtocolCompatible( "", schema ) );
	assert( !NET_ProtocolCompatible( "999", schema ) );
	assert( !NET_ProtocolCompatible( XSTRING( AFTERSHOCK_NET_VERSION ), "different-schema" ) );
	if ( argc == 3 ) {
		assert( !NET_ProtocolCompatible( argv[1], argv[2] ) );
	}
	printf( "%s %s\n", XSTRING( AFTERSHOCK_NET_VERSION ), schema );
}

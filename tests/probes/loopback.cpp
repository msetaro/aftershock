/* Test-only link wrappers. Both sides remain in the engine's in-process loopback. */
#include "../../engine/qcommon/q_shared.h"
#include "../../engine/qcommon/qcommon_public.h"
#include <stdint.h>

extern "C" qboolean __real__Z17NET_GetLoopPacket8netsrc_tP8netadr_tP5msg_t( netsrc_t sock, netadr_t *from, msg_t *msg );
extern "C" void __real__Z9Com_Frame8qboolean( qboolean noDelay );
extern "C" void __real__Z8NET_Initv( void );

static struct {
	qboolean used;
	netsrc_t side;
	uint64_t due;
	uint32_t serial;
	int length;
	byte data[MAX_MSGLEN];
} packets[256];
static uint64_t now;
static uint32_t rng = 1, serial, received[2], delivered[2], last[2], dropped, reordered;

static uint32_t Random( void ) {
	rng = rng * 1664525U + 1013904223U;
	return rng;
}

static void Report( void ) {
	Com_Printf( "TEST_LOOPBACK received=%u,%u delivered=%u,%u dropped=%u reordered=%u\n",
		received[0], received[1], delivered[0], delivered[1], dropped, reordered );
}

extern "C" void __wrap__Z8NET_Initv( void ) {
	__real__Z8NET_Initv();
	if ( Cvar_VariableIntegerValue( "net_enabled" ) != 0 )
		Com_Error( ERR_FATAL, "loopback test requires net_enabled 0" );
	Cmd_AddCommand( "test_loopback_report", Report );
}

extern "C" void __wrap__Z9Com_Frame8qboolean( qboolean noDelay ) {
	now += 50;
	__real__Z9Com_Frame8qboolean( noDelay );
}

extern "C" qboolean __wrap__Z17NET_GetLoopPacket8netsrc_tP8netadr_tP5msg_t( netsrc_t sock, netadr_t *from, msg_t *msg ) {
	int i, selected = -1;
	while ( __real__Z17NET_GetLoopPacket8netsrc_tP8netadr_tP5msg_t( sock, from, msg ) ) {
		received[sock]++;
		if ( from->type != NA_LOOPBACK || msg->cursize > MAX_MSGLEN )
			Com_Error( ERR_FATAL, "invalid test loopback input" );
		/* Five-percent loss; independent 20..140ms delays also reorder packets. */
		if ( Random() % 100 < 5 ) {
			dropped++;
			continue;
		}
		for ( i = 0; i < ARRAY_LEN( packets ); i++ )
			if ( !packets[i].used )
				break;
		if ( i == ARRAY_LEN( packets ) )
			Com_Error( ERR_FATAL, "test loopback queue overflow" );
		packets[i].used = qtrue;
		packets[i].side = sock;
		packets[i].due = now + 20 + Random() % 121;
		packets[i].serial = ++serial;
		packets[i].length = msg->cursize;
		memcpy( packets[i].data, msg->data, msg->cursize );
	}
	for ( i = 0; i < ARRAY_LEN( packets ); i++ ) {
		if ( packets[i].used && packets[i].side == sock && packets[i].due <= now &&
			 ( selected < 0 || packets[i].due < packets[selected].due ) )
			selected = i;
	}
	if ( selected < 0 )
		return qfalse;
	if ( packets[selected].length > msg->maxsize )
		Com_Error( ERR_FATAL, "test receive buffer too small" );
	if ( packets[selected].serial < last[sock] )
		reordered++;
	last[sock] = packets[selected].serial;
	delivered[sock]++;
	memset( from, 0, sizeof( *from ) );
	from->type = NA_LOOPBACK;
	memcpy( msg->data, packets[selected].data, packets[selected].length );
	msg->cursize = packets[selected].length;
	packets[selected].used = qfalse;
	return qtrue;
}

#pragma once

#ifdef AFTERSHOCK_DEVTOOLS
#include "../qcommon/q_shared.h"
#include "../public/dev_public.h"
#include "../public/dev_game_public.h"
#include "../renderercommon/tr_public.h"

struct devCpuTiming_t {
	char name[48];
	int64_t microseconds;
};
struct devNetwork_t {
	uint64_t bytes[2], packets[2], snapshots, predictions;
	uint32_t lastPacket[2], snapshotBits;
	float predictionError;
	bool delta;
};
void DevTools_BeginFrame( bool enabled );
uint32_t DevTools_CpuTimings( const devCpuTiming_t **timings );
void DevTools_Packet( bool outgoing, uint32_t bytes );
void DevTools_Snapshot( uint32_t bits, bool delta );
const devNetwork_t *DevTools_Network( void );

const devGameTools_t *DevTools_Game( void );
void DevTools_SetView( const refdef_t *view );
const refdef_t *DevTools_View( void );
bool DevTools_SaveEntities( void );
void DevTools_InitEntities( void );

void DevTools_Init( void );
void DevTools_Reset( void );
void DevTools_Draw( const refexport_t *renderer, int width, int height, int milliseconds );
bool DevTools_Key( int key, bool down );
bool DevTools_Char( uint32_t character );
bool DevTools_Mouse( int dx, int dy );
void DevTools_Log( const char *text );
const char *DevTools_Console( void );
#endif

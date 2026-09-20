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
void DevTools_SetView( const refdef_t *view, int clientEntity = -1 );
int DevTools_ViewClient( void );
const refdef_t *DevTools_View( void );
bool DevTools_SaveEntities( void );
void DevTools_InitEntities( void );

struct devLine_t {
	float start[3], end[3];
	uint32_t color, expires;
};
struct devText_t {
	float origin[3];
	char text[64];
	uint32_t color, expires;
};
void DevTools_BeginDebugFrame( bool enabled, uint32_t milliseconds );
void DevTools_ClearWorld( void );
void DevTools_InitWorld( void );
void DevTools_RebuildWorld( bool collision, bool navigation, float radius );
uint32_t DevTools_Lines( const devLine_t **lines, bool world );
uint32_t DevTools_Text( const devText_t **text );
uint32_t DevTools_DebugDropped( void );
bool DevTools_Project( const refdef_t *view, const float *point, float *screen );
int DevTools_PickEntity( float x, float y );
void DevTools_Init( void );
void DevTools_Reset( void );
void DevTools_Draw( const refexport_t *renderer, int width, int height, int milliseconds );
bool DevTools_Key( int key, bool down );
bool DevTools_Char( uint32_t character );
bool DevTools_Mouse( int dx, int dy );
void DevTools_Log( const char *text );
const char *DevTools_Console( void );
#endif

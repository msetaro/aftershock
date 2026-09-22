#pragma once

#ifdef AFTERSHOCK_DEVTOOLS
#include "../qcommon/q_shared.h"
#include "../public/dev_public.h"
#include "../public/dev_game_public.h"
#include "../renderercommon/tr_public.h"

struct devCpuTiming_t {
	char name[48];
	int64_t microseconds, selfMicroseconds;
	uint32_t parent;
};
struct devCpuFrame_t {
	uint32_t serial, count, dropped;
	int64_t microseconds;
	devCpuTiming_t scopes[128];
};
struct devNetworkPacket_t {
	uint32_t milliseconds, bytes;
	bool outgoing;
};
struct devNetworkField_t {
	char name[64];
	uint64_t bits[2], samples[2];
};
struct devNetwork_t {
	uint64_t bytes[2], packets[2], snapshots, predictions;
	uint32_t lastPacket[2], snapshotBits;
	float predictionError, predictionPeak;
	double predictionSum;
	uint64_t rewindReports, rewindHits, rewindClamped;
	uint32_t rewindAge, rewindLimit;
	bool delta;
};
struct devAgentInput_t {
	int32_t forward, right, up, buttons, weapon;
	float pitch, yaw;
	int32_t angles[3];
	bool active, raw;
};
const float *DevTools_AgentCamera( void );
void DevTools_AgentView( refdef_t *view );
void DevTools_AgentInput( usercmd_t *command, float *viewangles, const int32_t *deltaAngles );
bool CL_AgentPlayer( playerState_t *player );
void DevTools_AgentEnable( void );
bool DevTools_AgentActive( void );
int DevTools_AgentTime( void );
int DevTools_AgentSeed( void );
bool DevTools_AgentNextFrame( void );
void DevTools_AgentFlushEvents( void );
void DevTools_AgentEndFrame( void );
int Sys_AgentRead( char *line, uint32_t capacity );
bool Sys_AgentWrite( const char *text, uint32_t length );
bool DevTools_AgentRequest( const char *request, uint32_t length, char *response, uint32_t capacity );
void DevTools_BeginFrame( bool enabled );
uint32_t DevTools_CpuTimings( const devCpuTiming_t **timings );
// Age zero is the latest completed frame; the bounded history holds 240 frames.
const devCpuFrame_t *DevTools_CpuFrame( uint32_t age );
const devCpuFrame_t *DevTools_CpuPeak();
void DevTools_ClearCpuHistory();
void DevTools_SelectCpuFrame( const devCpuFrame_t *frame );
const devCpuFrame_t *DevTools_CpuSelection();
void DevTools_Packet( bool outgoing, uint32_t bytes );
void DevTools_Snapshot( uint32_t bits, bool delta );
const devNetwork_t *DevTools_Network( void );
const devNetworkPacket_t *DevTools_NetworkPacket( uint32_t age );
uint32_t DevTools_NetworkFields( const devNetworkField_t **fields );
void DevTools_NetworkField( bool outgoing, bool player, uint32_t index, const char *name, int bits );
void DevTools_ClearNetwork();

const devGameTools_t *DevTools_Game( void );
void DevTools_SetView( const refdef_t *view, int clientEntity = -1 );
int DevTools_ViewClient( void );
const refdef_t *DevTools_View( void );
const sceneLight_t *DevTools_SceneLight( void );
bool DevTools_SaveEntities( void );
bool DevTools_SaveDefinitions();
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
struct devEditorState_t {
	char cvar[MAX_STRING_CHARS], filters[3][128];
	char graphTab[16], effectResult[32];
	bool effectDirty;
	char panel[32], clip[64], graphState[64], graphEvent[64], graphResult[32];
	uint32_t frames, lines, labels, worldLines, animationPreviews, allocations;
	uint64_t arena;
	bool enabled, inputCaptured;
	int32_t model, animationFrame, selectedEntity, viewport[4];
	uint32_t graphPreviews, graphTime;
	bool graphDirty, graphPlay;
	bool rangeLoaded, rangeAds;
	int32_t rangeSlot, rangeAttachments;
	char rangeName[64];
	bool collision, navigation, entities, animationPlay;
};
bool DevTools_SelectPanel( const char *name );
bool DevTools_SelectCvar( const char *name );
bool DevTools_Filter( const char *kind, const char *value );
bool DevTools_SelectEntity( int entity );
int DevTools_PickCrosshair( void );
bool DevTools_EntityAtCamera( float *origin );
bool DevTools_ReloadEntities( void );
bool DevTools_SetWorld( bool collision, bool navigation, bool entities, float radius );
bool DevTools_LoadAnimation( const char *path, const char *skin );
bool DevTools_SetAnimation( const char *field, float value );
bool DevTools_EffectEditor( const char *action, const char *text );
bool DevTools_Graph( const char *action, const char *text, float value );
const animAsset_t *DevTools_GraphAsset( const float **parameters );
// Both consumers read validated loaded records without unaligned structure casts.
template <typename T>
T DevTools_GraphRecord( const animAsset_t *asset, animSectionIndex_t section, uint32_t index ) {
	T record;
	memcpy( &record, asset->data + asset->header.sections[section].offset + index * sizeof( T ), sizeof( record ) );
	return record;
}
bool DevTools_Range( const char *action, const char *path, int value );
const refexport_t *DevTools_Renderer( void );
bool DevTools_SelectAsset( const char *kind, int index );
bool DevTools_MaterialPreview( int index, bool enabled );
bool DevTools_SetMaterial( int index, const materialParams_t *params );
void DevTools_EditorState( devEditorState_t *state );
bool DevTools_CaptureWorkspace( devWorkspace_t *workspace );
void DevTools_RestoreWorkspace( const devWorkspace_t &workspace );
void DevTools_Init( void );
void DevTools_Reset( void );
void DevTools_Draw( const refexport_t *renderer, int width, int height, int milliseconds );
bool DevTools_Key( int key, bool down );
bool DevTools_Char( uint32_t character );
bool DevTools_Mouse( int dx, int dy );
void DevTools_Log( const char *text );
const char *DevTools_Console( void );
#endif

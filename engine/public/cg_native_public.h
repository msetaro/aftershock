/* Typed native module interface. */
#ifndef CG_NATIVE_PUBLIC_H
#define CG_NATIVE_PUBLIC_H

#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
int CGameImport_DrawAuthoredHUD( int health, int armor, int ammo, int ping );
void CGameImport_Print( const char *fmt );
void CGameImport_Error( const char *fmt );
int CGameImport_Milliseconds( void );
void CGameImport_Cvar_Register( void *vmCvar, const char *varName, const char *defaultValue, int flags );
void CGameImport_Cvar_Update( void *vmCvar );
void CGameImport_Cvar_Set( const char *var_name, const char *value );
void CGameImport_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );
int CGameImport_Argc( void );
void CGameImport_Argv( int n, char *buffer, int bufferLength );
void CGameImport_Args( char *buffer, int bufferLength );
int CGameImport_FS_FOpenFile( const char *qpath, void *f, int mode );
void CGameImport_FS_Read( void *buffer, int len, int f );
void CGameImport_FS_Write( const void *buffer, int len, int f );
void CGameImport_FS_FCloseFile( int f );
int CGameImport_FS_Seek( int f, int64_t offset, int origin );
void CGameImport_SendConsoleCommand( const char *text );
void CGameImport_AddCommand( const char *cmdName );
void CGameImport_RemoveCommand( const char *cmdName );
void CGameImport_SendClientCommand( const char *s );
void CGameImport_UpdateScreen( void );
void CGameImport_CM_LoadMap( const char *mapname );
int CGameImport_CM_NumInlineModels( void );
int CGameImport_CM_InlineModel( int index );
int CGameImport_CM_TempBoxModel( const float *mins, const float *maxs );
int CGameImport_CM_TempCapsuleModel( const float *mins, const float *maxs );
int CGameImport_CM_PointContents( const float *p, int model );
int CGameImport_CM_TransformedPointContents( const float *p, int model, const float *origin, const float *angles );
void CGameImport_CM_BoxTrace( void *results, const float *start, const float *end, const float *mins, const float *maxs, int model, int brushmask );
void CGameImport_CM_CapsuleTrace( void *results, const float *start, const float *end, const float *mins, const float *maxs, int model, int brushmask );
void CGameImport_CM_TransformedBoxTrace( void *results, const float *start, const float *end, const float *mins, const float *maxs, int model, int brushmask, const float *origin, const float *angles );
void CGameImport_CM_TransformedCapsuleTrace( void *results, const float *start, const float *end, const float *mins, const float *maxs, int model, int brushmask, const float *origin, const float *angles );
int CGameImport_CM_MarkFragments( int numPoints, const void *points, const float *projection, int maxPoints, float *pointBuffer, int maxFragments, void *fragmentBuffer );
void CGameImport_S_StartSound( float *origin, int entityNum, int entchannel, int sfx );
void CGameImport_S_StartLocalSound( int sfx, int channelNum );
void CGameImport_S_ClearLoopingSounds( int killall );
void CGameImport_S_AddLoopingSound( int entityNum, const float *origin, const float *velocity, int sfx );
void CGameImport_S_AddRealLoopingSound( int entityNum, const float *origin, const float *velocity, int sfx );
void CGameImport_S_StopLoopingSound( int entityNum );
void CGameImport_S_UpdateEntityPosition( int entityNum, const float *origin );
void CGameImport_S_Respatialize( int entityNum, const float *origin, void *axis, int inwater );
int CGameImport_S_RegisterSound( const char *sample, int compressed );
void CGameImport_S_StartBackgroundTrack( const char *intro, const char *loop );
void CGameImport_R_LoadWorldMap( const char *mapname );
int CGameImport_R_RegisterModel( const char *name );
int CGameImport_R_RegisterSkin( const char *name );
int CGameImport_R_RegisterShader( const char *name );
int CGameImport_R_RegisterEffect( const char *name );
uint32_t CGameImport_R_StartEffect( int handle, const float *origin, const float axis[3][3], uint32_t seed );
int CGameImport_R_RegisterShaderNoMip( const char *name );
void CGameImport_R_RegisterFont( const char *fontName, int pointSize, void *font );
void CGameImport_R_ClearScene( void );
void CGameImport_R_AddRefEntityToScene( const void *entity );
bool CGameImport_R_AddSkeletalEntityToScene( const void *entity, const void *pose, const uint8_t modelHash[32] );
bool CGameImport_R_AddTemporalEntityToScene( const void *entity, uint64_t identity, const void *instance, const void *pose, const uint8_t modelHash[32] );
bool CGameImport_R_AddMaterialEntityToScene( const void *entity, const void *instance, const void *pose, const uint8_t modelHash[32] );
void CGameImport_R_AddPolyToScene( int hShader, int numVerts, const void *verts );
void CGameImport_R_AddPolysToScene( int hShader, int numVerts, const void *verts, int num );
int CGameImport_R_LightForPoint( float *point, float *ambientLight, float *directedLight, float *lightDir );
void CGameImport_R_AddLightToScene( const float *org, float intensity, float r, float g, float b );
bool CGameImport_R_AddSceneLight( const void *light );
void CGameImport_R_AddAdditiveLightToScene( const float *org, float intensity, float r, float g, float b );
void CGameImport_R_RenderScene( const void *fd );
void CGameImport_R_SetColor( const float *rgba );
void CGameImport_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, int hShader );
void CGameImport_R_ModelBounds( int model, float *mins, float *maxs );
int CGameImport_R_LerpTag( void *tag, int mod, int startFrame, int endFrame, float frac, const char *tagName );
void CGameImport_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset );
void CGameImport_GetGlconfig( void *glconfig );
void CGameImport_GetGameState( void *gamestate );
void CGameImport_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime );
int CGameImport_GetSnapshot( int snapshotNumber, void *snapshot );
int CGameImport_GetServerCommand( int serverCommandNumber );
int CGameImport_GetCurrentCmdNumber( void );
int CGameImport_GetUserCmd( int cmdNumber, void *ucmd );
void CGameImport_SetUserCmdValue( int stateValue, float sensitivityScale );
int CGameImport_MemoryRemaining( void );
int CGameImport_Key_IsDown( int keynum );
int CGameImport_Key_GetCatcher( void );
void CGameImport_Key_SetCatcher( int catcher );
int CGameImport_Key_GetKey( const char *binding );
int CGameImport_PC_AddGlobalDefine( char *define );
int CGameImport_PC_LoadSource( const char *filename );
int CGameImport_PC_FreeSource( int handle );
int CGameImport_PC_ReadToken( int handle, void *pc_token );
int CGameImport_PC_SourceFileAndLine( int handle, char *filename, int *line );
void CGameImport_S_StopBackgroundTrack( void );
int CGameImport_RealTime( void *qtime );
void CGameImport_SnapVector( float *v );
int CGameImport_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits );
int CGameImport_CIN_StopCinematic( int handle );
int CGameImport_CIN_RunCinematic( int handle );
void CGameImport_CIN_DrawCinematic( int handle );
void CGameImport_CIN_SetExtents( int handle, int x, int y, int w, int h );
int CGameImport_loadCamera( const char *name );
void CGameImport_startCamera( int time );
int CGameImport_getCameraInfo( int time, void *origin, void *angles );
int CGameImport_GetEntityToken( char *buffer, int bufferSize );
int CGameImport_R_inPVS( const float *p1, const float *p2 );
extern int NativeCGame_CallDepth;
extern bool NativeCGame_Running;
void NativeCGame_Init( int message, int command, int client );
void NativeCGame_Shutdown( void );
int NativeCGame_ConsoleCommand( void );
void NativeCGame_DrawActiveFrame( int time, int stereo, int demo );
int NativeCGame_CrosshairPlayer( void );
int NativeCGame_LastAttacker( void );
void NativeCGame_KeyEvent( int key, int down );
void NativeCGame_MouseEvent( int dx, int dy );
void NativeCGame_EventHandling( int type );
#ifdef __cplusplus
}
#endif

#endif

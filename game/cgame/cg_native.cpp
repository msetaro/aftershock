/* Native service adapters; preserve the imported game ABI. */
#include "cg_local.h"
void trap_Print( const char *fmt ) {
	CGameImport_Print( fmt );
}
void trap_Error( const char *fmt ) {
	CGameImport_Error( fmt );
}
int trap_Milliseconds( void ) {
	return CGameImport_Milliseconds();
}
void trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags ) {
	CGameImport_Cvar_Register( vmCvar, varName, defaultValue, flags );
}
void trap_Cvar_Update( vmCvar_t *vmCvar ) {
	CGameImport_Cvar_Update( vmCvar );
}
void trap_Cvar_Set( const char *var_name, const char *value ) {
	CGameImport_Cvar_Set( var_name, value );
}
void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {
	CGameImport_Cvar_VariableStringBuffer( var_name, buffer, bufsize );
}
int trap_Argc( void ) {
	return CGameImport_Argc();
}
void trap_Argv( int n, char *buffer, int bufferLength ) {
	CGameImport_Argv( n, buffer, bufferLength );
}
void trap_Args( char *buffer, int bufferLength ) {
	CGameImport_Args( buffer, bufferLength );
}
int trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	return CGameImport_FS_FOpenFile( qpath, f, mode );
}
void trap_FS_Read( void *buffer, int len, fileHandle_t f ) {
	CGameImport_FS_Read( buffer, len, f );
}
void trap_FS_Write( const void *buffer, int len, fileHandle_t f ) {
	CGameImport_FS_Write( buffer, len, f );
}
void trap_FS_FCloseFile( fileHandle_t f ) {
	CGameImport_FS_FCloseFile( f );
}
int trap_FS_Seek( fileHandle_t f, long offset, int origin ) {
	return CGameImport_FS_Seek( f, offset, origin );
}
void trap_SendConsoleCommand( const char *text ) {
	CGameImport_SendConsoleCommand( text );
}
void trap_AddCommand( const char *cmdName ) {
	CGameImport_AddCommand( cmdName );
}
void trap_RemoveCommand( const char *cmdName ) {
	CGameImport_RemoveCommand( cmdName );
}
void trap_SendClientCommand( const char *s ) {
	CGameImport_SendClientCommand( s );
}
void trap_UpdateScreen( void ) {
	CGameImport_UpdateScreen();
}
void trap_CM_LoadMap( const char *mapname ) {
	CGameImport_CM_LoadMap( mapname );
}
int trap_CM_NumInlineModels( void ) {
	return CGameImport_CM_NumInlineModels();
}
clipHandle_t trap_CM_InlineModel( int index ) {
	return CGameImport_CM_InlineModel( index );
}
clipHandle_t trap_CM_TempBoxModel( const vec3_t mins, const vec3_t maxs ) {
	return CGameImport_CM_TempBoxModel( mins, maxs );
}
clipHandle_t trap_CM_TempCapsuleModel( const vec3_t mins, const vec3_t maxs ) {
	return CGameImport_CM_TempCapsuleModel( mins, maxs );
}
int trap_CM_PointContents( const vec3_t p, clipHandle_t model ) {
	return CGameImport_CM_PointContents( p, model );
}
int trap_CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles ) {
	return CGameImport_CM_TransformedPointContents( p, model, origin, angles );
}
void trap_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
	const vec3_t mins, const vec3_t maxs,
	clipHandle_t model, int brushmask ) {
	CGameImport_CM_BoxTrace( results, start, end, mins, maxs, model, brushmask );
}
void trap_CM_CapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end,
	const vec3_t mins, const vec3_t maxs,
	clipHandle_t model, int brushmask ) {
	CGameImport_CM_CapsuleTrace( results, start, end, mins, maxs, model, brushmask );
}
void trap_CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
	const vec3_t mins, const vec3_t maxs,
	clipHandle_t model, int brushmask,
	const vec3_t origin, const vec3_t angles ) {
	CGameImport_CM_TransformedBoxTrace( results, start, end, mins, maxs, model, brushmask, origin, angles );
}
void trap_CM_TransformedCapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end,
	const vec3_t mins, const vec3_t maxs,
	clipHandle_t model, int brushmask,
	const vec3_t origin, const vec3_t angles ) {
	CGameImport_CM_TransformedCapsuleTrace( results, start, end, mins, maxs, model, brushmask, origin, angles );
}
int trap_CM_MarkFragments( int numPoints, const vec3_t *points,
	const vec3_t projection,
	int maxPoints, vec3_t pointBuffer,
	int maxFragments, markFragment_t *fragmentBuffer ) {
	return CGameImport_CM_MarkFragments( numPoints, points, projection, maxPoints, pointBuffer, maxFragments, fragmentBuffer );
}
void trap_S_StartSound( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx ) {
	CGameImport_S_StartSound( origin, entityNum, entchannel, sfx );
}
void trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum ) {
	CGameImport_S_StartLocalSound( sfx, channelNum );
}
void trap_S_ClearLoopingSounds( qboolean killall ) {
	CGameImport_S_ClearLoopingSounds( killall );
}
void trap_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx ) {
	CGameImport_S_AddLoopingSound( entityNum, origin, velocity, sfx );
}
void trap_S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx ) {
	CGameImport_S_AddRealLoopingSound( entityNum, origin, velocity, sfx );
}
void trap_S_StopLoopingSound( int entityNum ) {
	CGameImport_S_StopLoopingSound( entityNum );
}
void trap_S_UpdateEntityPosition( int entityNum, const vec3_t origin ) {
	CGameImport_S_UpdateEntityPosition( entityNum, origin );
}
void trap_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater ) {
	CGameImport_S_Respatialize( entityNum, origin, axis, inwater );
}
sfxHandle_t trap_S_RegisterSound( const char *sample, qboolean compressed ) {
	return CGameImport_S_RegisterSound( sample, compressed );
}
void trap_S_StartBackgroundTrack( const char *intro, const char *loop ) {
	CGameImport_S_StartBackgroundTrack( intro, loop );
}
void trap_R_LoadWorldMap( const char *mapname ) {
	CGameImport_R_LoadWorldMap( mapname );
}
qhandle_t trap_R_RegisterModel( const char *name ) {
	return CGameImport_R_RegisterModel( name );
}
qhandle_t trap_R_RegisterSkin( const char *name ) {
	return CGameImport_R_RegisterSkin( name );
}
qhandle_t trap_R_RegisterShader( const char *name ) {
	return CGameImport_R_RegisterShader( name );
}
qhandle_t trap_R_RegisterShaderNoMip( const char *name ) {
	return CGameImport_R_RegisterShaderNoMip( name );
}
void trap_R_RegisterFont( const char *fontName, int pointSize, fontInfo_t *font ) {
	CGameImport_R_RegisterFont( fontName, pointSize, font );
}
void trap_R_ClearScene( void ) {
	CGameImport_R_ClearScene();
}
void trap_R_AddRefEntityToScene( const refEntity_t *re ) {
	CGameImport_R_AddRefEntityToScene( re );
}
void trap_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts ) {
	CGameImport_R_AddPolyToScene( hShader, numVerts, verts );
}
void trap_R_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int num ) {
	CGameImport_R_AddPolysToScene( hShader, numVerts, verts, num );
}
int trap_R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir ) {
	return CGameImport_R_LightForPoint( point, ambientLight, directedLight, lightDir );
}
void trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {
	CGameImport_R_AddLightToScene( org, intensity, r, g, b );
}
void trap_R_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {
	CGameImport_R_AddAdditiveLightToScene( org, intensity, r, g, b );
}
void trap_R_RenderScene( const refdef_t *fd ) {
	CGameImport_R_RenderScene( fd );
}
void trap_R_SetColor( const float *rgba ) {
	CGameImport_R_SetColor( rgba );
}
void trap_R_DrawStretchPic( float x, float y, float w, float h,
	float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	CGameImport_R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, hShader );
}
void trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs ) {
	CGameImport_R_ModelBounds( model, mins, maxs );
}
int trap_R_LerpTag( orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame,
	float frac, const char *tagName ) {
	return CGameImport_R_LerpTag( tag, mod, startFrame, endFrame, frac, tagName );
}
void trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset ) {
	CGameImport_R_RemapShader( oldShader, newShader, timeOffset );
}
void trap_GetGlconfig( glconfig_t *glconfig ) {
	CGameImport_GetGlconfig( glconfig );
}
void trap_GetGameState( gameState_t *gamestate ) {
	CGameImport_GetGameState( gamestate );
}
void trap_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime ) {
	CGameImport_GetCurrentSnapshotNumber( snapshotNumber, serverTime );
}
qboolean trap_GetSnapshot( int snapshotNumber, snapshot_t *snapshot ) {
	return (qboolean)CGameImport_GetSnapshot( snapshotNumber, snapshot );
}
qboolean trap_GetServerCommand( int serverCommandNumber ) {
	return (qboolean)CGameImport_GetServerCommand( serverCommandNumber );
}
int trap_GetCurrentCmdNumber( void ) {
	return CGameImport_GetCurrentCmdNumber();
}
qboolean trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd ) {
	return (qboolean)CGameImport_GetUserCmd( cmdNumber, ucmd );
}
void trap_SetUserCmdValue( int stateValue, float sensitivityScale ) {
	CGameImport_SetUserCmdValue( stateValue, sensitivityScale );
}
int trap_MemoryRemaining( void ) {
	return CGameImport_MemoryRemaining();
}
qboolean trap_Key_IsDown( int keynum ) {
	return (qboolean)CGameImport_Key_IsDown( keynum );
}
int trap_Key_GetCatcher( void ) {
	return CGameImport_Key_GetCatcher();
}
void trap_Key_SetCatcher( int catcher ) {
	CGameImport_Key_SetCatcher( catcher );
}
int trap_Key_GetKey( const char *binding ) {
	return CGameImport_Key_GetKey( binding );
}
int trap_PC_AddGlobalDefine( char *define ) {
	return CGameImport_PC_AddGlobalDefine( define );
}
int trap_PC_LoadSource( const char *filename ) {
	return CGameImport_PC_LoadSource( filename );
}
int trap_PC_FreeSource( int handle ) {
	return CGameImport_PC_FreeSource( handle );
}
int trap_PC_ReadToken( int handle, pc_token_t *pc_token ) {
	return CGameImport_PC_ReadToken( handle, pc_token );
}
int trap_PC_SourceFileAndLine( int handle, char *filename, int *line ) {
	return CGameImport_PC_SourceFileAndLine( handle, filename, line );
}
void trap_S_StopBackgroundTrack( void ) {
	CGameImport_S_StopBackgroundTrack();
}
int trap_RealTime( qtime_t *qtime ) {
	return CGameImport_RealTime( qtime );
}
void trap_SnapVector( float *v ) {
	CGameImport_SnapVector( v );
}
int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits ) {
	return CGameImport_CIN_PlayCinematic( arg0, xpos, ypos, width, height, bits );
}
e_status trap_CIN_StopCinematic( int handle ) {
	return (e_status)CGameImport_CIN_StopCinematic( handle );
}
e_status trap_CIN_RunCinematic( int handle ) {
	return (e_status)CGameImport_CIN_RunCinematic( handle );
}
void trap_CIN_DrawCinematic( int handle ) {
	CGameImport_CIN_DrawCinematic( handle );
}
void trap_CIN_SetExtents( int handle, int x, int y, int w, int h ) {
	CGameImport_CIN_SetExtents( handle, x, y, w, h );
}
qboolean trap_loadCamera( const char *name ) {
	return (qboolean)CGameImport_loadCamera( name );
}
void trap_startCamera( int time ) {
	CGameImport_startCamera( time );
}
qboolean trap_getCameraInfo( int time, vec3_t *origin, vec3_t *angles ) {
	return (qboolean)CGameImport_getCameraInfo( time, origin, angles );
}
qboolean trap_GetEntityToken( char *buffer, int bufferSize ) {
	return (qboolean)CGameImport_GetEntityToken( buffer, bufferSize );
}
qboolean trap_R_inPVS( const vec3_t p1, const vec3_t p2 ) {
	return (qboolean)CGameImport_R_inPVS( p1, p2 );
}

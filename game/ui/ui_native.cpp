/* Native service adapters; preserve the imported game ABI. */
#include "ui_local.h"
void trap_Print( const char *string ) {
	UIImport_Print( string );
}
void trap_Error( const char *string ) {
	UIImport_Error( string );
}
int trap_Milliseconds( void ) {
	return UIImport_Milliseconds();
}
void trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags ) {
	UIImport_Cvar_Register( cvar, var_name, value, flags );
}
void trap_Cvar_Update( vmCvar_t *cvar ) {
	UIImport_Cvar_Update( cvar );
}
void trap_Cvar_Set( const char *var_name, const char *value ) {
	UIImport_Cvar_Set( var_name, value );
}
float trap_Cvar_VariableValue( const char *var_name ) {
	return UIImport_Cvar_VariableValue( var_name );
}
void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {
	UIImport_Cvar_VariableStringBuffer( var_name, buffer, bufsize );
}
void trap_Cvar_SetValue( const char *var_name, float value ) {
	UIImport_Cvar_SetValue( var_name, value );
}
void trap_Cvar_Reset( const char *name ) {
	UIImport_Cvar_Reset( name );
}
void trap_Cvar_Create( const char *var_name, const char *var_value, int flags ) {
	UIImport_Cvar_Create( var_name, var_value, flags );
}
void trap_Cvar_InfoStringBuffer( int bit, char *buffer, int bufsize ) {
	UIImport_Cvar_InfoStringBuffer( bit, buffer, bufsize );
}
int trap_Argc( void ) {
	return UIImport_Argc();
}
void trap_Argv( int n, char *buffer, int bufferLength ) {
	UIImport_Argv( n, buffer, bufferLength );
}
void trap_Cmd_ExecuteText( int exec_when, const char *text ) {
	UIImport_Cmd_ExecuteText( exec_when, text );
}
int trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	return UIImport_FS_FOpenFile( qpath, f, mode );
}
void trap_FS_Read( void *buffer, int len, fileHandle_t f ) {
	UIImport_FS_Read( buffer, len, f );
}
void trap_FS_Write( const void *buffer, int len, fileHandle_t f ) {
	UIImport_FS_Write( buffer, len, f );
}
void trap_FS_FCloseFile( fileHandle_t f ) {
	UIImport_FS_FCloseFile( f );
}
int trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize ) {
	return UIImport_FS_GetFileList( path, extension, listbuf, bufsize );
}
int trap_FS_Seek( fileHandle_t f, int64_t offset, int origin ) {
	return UIImport_FS_Seek( f, offset, origin );
}
qhandle_t trap_R_RegisterModel( const char *name ) {
	return UIImport_R_RegisterModel( name );
}
qhandle_t trap_R_RegisterSkin( const char *name ) {
	return UIImport_R_RegisterSkin( name );
}
void trap_R_RegisterFont( const char *fontName, int pointSize, fontInfo_t *font ) {
	UIImport_R_RegisterFont( fontName, pointSize, font );
}
qhandle_t trap_R_RegisterShaderNoMip( const char *name ) {
	return UIImport_R_RegisterShaderNoMip( name );
}
void trap_R_ClearScene( void ) {
	UIImport_R_ClearScene();
}
void trap_R_AddRefEntityToScene( const refEntity_t *re ) {
	UIImport_R_AddRefEntityToScene( re );
}
void trap_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts ) {
	UIImport_R_AddPolyToScene( hShader, numVerts, verts );
}
void trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {
	UIImport_R_AddLightToScene( org, intensity, r, g, b );
}
void trap_R_RenderScene( const refdef_t *fd ) {
	UIImport_R_RenderScene( fd );
}
void trap_R_SetColor( const float *rgba ) {
	UIImport_R_SetColor( rgba );
}
void trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	UIImport_R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, hShader );
}
void trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs ) {
	UIImport_R_ModelBounds( model, mins, maxs );
}
void trap_UpdateScreen( void ) {
	UIImport_UpdateScreen();
}
int trap_CM_LerpTag( orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame, float frac, const char *tagName ) {
	return UIImport_CM_LerpTag( tag, mod, startFrame, endFrame, frac, tagName );
}
void trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum ) {
	UIImport_S_StartLocalSound( sfx, channelNum );
}
sfxHandle_t trap_S_RegisterSound( const char *sample, qboolean compressed ) {
	return UIImport_S_RegisterSound( sample, compressed );
}
void trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen ) {
	UIImport_Key_KeynumToStringBuf( keynum, buf, buflen );
}
void trap_Key_GetBindingBuf( int keynum, char *buf, int buflen ) {
	UIImport_Key_GetBindingBuf( keynum, buf, buflen );
}
void trap_Key_SetBinding( int keynum, const char *binding ) {
	UIImport_Key_SetBinding( keynum, binding );
}
qboolean trap_Key_IsDown( int keynum ) {
	return (qboolean)UIImport_Key_IsDown( keynum );
}
qboolean trap_Key_GetOverstrikeMode( void ) {
	return (qboolean)UIImport_Key_GetOverstrikeMode();
}
void trap_Key_SetOverstrikeMode( qboolean state ) {
	UIImport_Key_SetOverstrikeMode( state );
}
void trap_Key_ClearStates( void ) {
	UIImport_Key_ClearStates();
}
int trap_Key_GetCatcher( void ) {
	return UIImport_Key_GetCatcher();
}
void trap_Key_SetCatcher( int catcher ) {
	UIImport_Key_SetCatcher( catcher );
}
void trap_GetClipboardData( char *buf, int bufsize ) {
	UIImport_GetClipboardData( buf, bufsize );
}
void trap_GetClientState( uiClientState_t *state ) {
	UIImport_GetClientState( state );
}
void trap_GetGlconfig( glconfig_t *glconfig ) {
	UIImport_GetGlconfig( glconfig );
}
int trap_GetConfigString( int index, char *buff, int buffsize ) {
	return UIImport_GetConfigString( index, buff, buffsize );
}
int trap_LAN_GetServerCount( int source ) {
	return UIImport_LAN_GetServerCount( source );
}
void trap_LAN_GetServerAddressString( int source, int n, char *buf, int buflen ) {
	UIImport_LAN_GetServerAddressString( source, n, buf, buflen );
}
void trap_LAN_GetServerInfo( int source, int n, char *buf, int buflen ) {
	UIImport_LAN_GetServerInfo( source, n, buf, buflen );
}
int trap_LAN_GetServerPing( int source, int n ) {
	return UIImport_LAN_GetServerPing( source, n );
}
int trap_LAN_GetPingQueueCount( void ) {
	return UIImport_LAN_GetPingQueueCount();
}
int trap_LAN_ServerStatus( const char *serverAddress, char *serverStatus, int maxLen ) {
	return UIImport_LAN_ServerStatus( serverAddress, serverStatus, maxLen );
}
void trap_LAN_SaveCachedServers() {
	UIImport_LAN_SaveCachedServers();
}
void trap_LAN_LoadCachedServers() {
	UIImport_LAN_LoadCachedServers();
}
void trap_LAN_ResetPings( int n ) {
	UIImport_LAN_ResetPings( n );
}
void trap_LAN_ClearPing( int n ) {
	UIImport_LAN_ClearPing( n );
}
void trap_LAN_GetPing( int n, char *buf, int buflen, int *pingtime ) {
	UIImport_LAN_GetPing( n, buf, buflen, pingtime );
}
void trap_LAN_GetPingInfo( int n, char *buf, int buflen ) {
	UIImport_LAN_GetPingInfo( n, buf, buflen );
}
void trap_LAN_MarkServerVisible( int source, int n, qboolean visible ) {
	UIImport_LAN_MarkServerVisible( source, n, visible );
}
int trap_LAN_ServerIsVisible( int source, int n ) {
	return UIImport_LAN_ServerIsVisible( source, n );
}
qboolean trap_LAN_UpdateVisiblePings( int source ) {
	return (qboolean)UIImport_LAN_UpdateVisiblePings( source );
}
int trap_LAN_AddServer( int source, const char *name, const char *addr ) {
	return UIImport_LAN_AddServer( source, name, addr );
}
void trap_LAN_RemoveServer( int source, const char *addr ) {
	UIImport_LAN_RemoveServer( source, addr );
}
int trap_LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 ) {
	return UIImport_LAN_CompareServers( source, sortKey, sortDir, s1, s2 );
}
int trap_MemoryRemaining( void ) {
	return UIImport_MemoryRemaining();
}
void trap_GetCDKey( char *buf, int buflen ) {
	UIImport_GetCDKey( buf, buflen );
}
void trap_SetCDKey( char *buf ) {
	UIImport_SetCDKey( buf );
}
int trap_PC_AddGlobalDefine( char *define ) {
	return UIImport_PC_AddGlobalDefine( define );
}
int trap_PC_LoadSource( const char *filename ) {
	return UIImport_PC_LoadSource( filename );
}
int trap_PC_FreeSource( int handle ) {
	return UIImport_PC_FreeSource( handle );
}
int trap_PC_ReadToken( int handle, pc_token_t *pc_token ) {
	return UIImport_PC_ReadToken( handle, pc_token );
}
int trap_PC_SourceFileAndLine( int handle, char *filename, int *line ) {
	return UIImport_PC_SourceFileAndLine( handle, filename, line );
}
void trap_S_StopBackgroundTrack( void ) {
	UIImport_S_StopBackgroundTrack();
}
void trap_S_StartBackgroundTrack( const char *intro, const char *loop ) {
	UIImport_S_StartBackgroundTrack( intro, loop );
}
int trap_RealTime( qtime_t *qtime ) {
	return UIImport_RealTime( qtime );
}
int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits ) {
	return UIImport_CIN_PlayCinematic( arg0, xpos, ypos, width, height, bits );
}
e_status trap_CIN_StopCinematic( int handle ) {
	return (e_status)UIImport_CIN_StopCinematic( handle );
}
e_status trap_CIN_RunCinematic( int handle ) {
	return (e_status)UIImport_CIN_RunCinematic( handle );
}
void trap_CIN_DrawCinematic( int handle ) {
	UIImport_CIN_DrawCinematic( handle );
}
void trap_CIN_SetExtents( int handle, int x, int y, int w, int h ) {
	UIImport_CIN_SetExtents( handle, x, y, w, h );
}
void trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset ) {
	UIImport_R_RemapShader( oldShader, newShader, timeOffset );
}
qboolean trap_VerifyCDKey( const char *key, const char *chksum ) {
	return (qboolean)UIImport_VerifyCDKey( key, chksum );
}
void trap_SetPbClStatus( int status ) {
	UIImport_SetPbClStatus( status );
}

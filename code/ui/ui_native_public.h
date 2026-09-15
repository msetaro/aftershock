/* Typed native module interface. */
#ifndef UI_NATIVE_PUBLIC_H
#define UI_NATIVE_PUBLIC_H

#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
void UIImport_Print( const char * string );
void UIImport_Error( const char * string );
int UIImport_Milliseconds( void );
void UIImport_Cvar_Register( void * cvar, const char * var_name, const char * value, int flags );
void UIImport_Cvar_Update( void * cvar );
void UIImport_Cvar_Set( const char * var_name, const char * value );
float UIImport_Cvar_VariableValue( const char * var_name );
void UIImport_Cvar_VariableStringBuffer( const char * var_name, char * buffer, int bufsize );
void UIImport_Cvar_SetValue( const char * var_name, float value );
void UIImport_Cvar_Reset( const char * name );
void UIImport_Cvar_Create( const char * var_name, const char * var_value, int flags );
void UIImport_Cvar_InfoStringBuffer( int bit, char * buffer, int bufsize );
int UIImport_Argc( void );
void UIImport_Argv( int n, char * buffer, int bufferLength );
void UIImport_Cmd_ExecuteText( int exec_when, const char * text );
int UIImport_FS_FOpenFile( const char * qpath, void * f, int mode );
void UIImport_FS_Read( void * buffer, int len, int f );
void UIImport_FS_Write( const void * buffer, int len, int f );
void UIImport_FS_FCloseFile( int f );
int UIImport_FS_GetFileList( const char * path, const char * extension, char * listbuf, int bufsize );
int UIImport_FS_Seek( int f, int64_t offset, int origin );
int UIImport_R_RegisterModel( const char * name );
int UIImport_R_RegisterSkin( const char * name );
void UIImport_R_RegisterFont( const char * fontName, int pointSize, void * font );
int UIImport_R_RegisterShaderNoMip( const char * name );
void UIImport_R_ClearScene( void );
void UIImport_R_AddRefEntityToScene( const void * entity );
void UIImport_R_AddPolyToScene( int hShader, int numVerts, const void * verts );
void UIImport_R_AddLightToScene( const float * org, float intensity, float r, float g, float b );
void UIImport_R_RenderScene( const void * fd );
void UIImport_R_SetColor( const float * rgba );
void UIImport_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, int hShader );
void UIImport_R_ModelBounds( int model, float * mins, float * maxs );
void UIImport_UpdateScreen( void );
int UIImport_CM_LerpTag( void * tag, int mod, int startFrame, int endFrame, float frac, const char * tagName );
void UIImport_S_StartLocalSound( int sfx, int channelNum );
int UIImport_S_RegisterSound( const char * sample, int compressed );
void UIImport_Key_KeynumToStringBuf( int keynum, char * buf, int buflen );
void UIImport_Key_GetBindingBuf( int keynum, char * buf, int buflen );
void UIImport_Key_SetBinding( int keynum, const char * binding );
int UIImport_Key_IsDown( int keynum );
int UIImport_Key_GetOverstrikeMode( void );
void UIImport_Key_SetOverstrikeMode( int state );
void UIImport_Key_ClearStates( void );
int UIImport_Key_GetCatcher( void );
void UIImport_Key_SetCatcher( int catcher );
void UIImport_GetClipboardData( char * buf, int bufsize );
void UIImport_GetClientState( void * state );
void UIImport_GetGlconfig( void * glconfig );
int UIImport_GetConfigString( int index, char* buff, int buffsize );
int UIImport_LAN_GetServerCount( int source );
void UIImport_LAN_GetServerAddressString( int source, int n, char * buf, int buflen );
void UIImport_LAN_GetServerInfo( int source, int n, char * buf, int buflen );
int UIImport_LAN_GetServerPing( int source, int n );
int UIImport_LAN_GetPingQueueCount( void );
int UIImport_LAN_ServerStatus( const char * serverAddress, char * serverStatus, int maxLen );
void UIImport_LAN_SaveCachedServers( void );
void UIImport_LAN_LoadCachedServers( void );
void UIImport_LAN_ResetPings( int n );
void UIImport_LAN_ClearPing( int n );
void UIImport_LAN_GetPing( int n, char * buf, int buflen, int * pingtime );
void UIImport_LAN_GetPingInfo( int n, char * buf, int buflen );
void UIImport_LAN_MarkServerVisible( int source, int n, int visible );
int UIImport_LAN_ServerIsVisible( int source, int n );
int UIImport_LAN_UpdateVisiblePings( int source );
int UIImport_LAN_AddServer( int source, const char * name, const char * addr );
void UIImport_LAN_RemoveServer( int source, const char * addr );
int UIImport_LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 );
int UIImport_MemoryRemaining( void );
void UIImport_GetCDKey( char * buf, int buflen );
void UIImport_SetCDKey( char * buf );
int UIImport_PC_AddGlobalDefine( char * define );
int UIImport_PC_LoadSource( const char * filename );
int UIImport_PC_FreeSource( int handle );
int UIImport_PC_ReadToken( int handle, void * pc_token );
int UIImport_PC_SourceFileAndLine( int handle, char * filename, int * line );
void UIImport_S_StopBackgroundTrack( void );
void UIImport_S_StartBackgroundTrack( const char * intro, const char * loop );
int UIImport_RealTime( void * qtime );
int UIImport_CIN_PlayCinematic( const char * arg0, int xpos, int ypos, int width, int height, int bits );
int UIImport_CIN_StopCinematic( int handle );
int UIImport_CIN_RunCinematic( int handle );
void UIImport_CIN_DrawCinematic( int handle );
void UIImport_CIN_SetExtents( int handle, int x, int y, int w, int h );
void UIImport_R_RemapShader( const char * oldShader, const char * newShader, const char * timeOffset );
int UIImport_VerifyCDKey( const char * key, const char * chksum );
void UIImport_SetPbClStatus( int status );
extern int NativeUI_CallDepth;
extern bool NativeUI_Running;
void NativeUI_Init( int connected );
void NativeUI_Shutdown( void );
void NativeUI_KeyEvent( int key, int down );
void NativeUI_MouseEvent( int dx, int dy );
void NativeUI_Refresh( int time );
int NativeUI_IsFullscreen( void );
void NativeUI_SetActiveMenu( int menu );
int NativeUI_ConsoleCommand( int time );
void NativeUI_DrawConnectScreen( int overlay );
#ifdef __cplusplus
}
#endif

#endif

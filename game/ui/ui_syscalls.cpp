/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
#include "ui_local.h"

// this file is only included when building a dll
// syscalls.asm is included instead when building a qvm
#ifdef Q3_VM
#error "Do not use in VM build"
#endif

static intptr_t( QDECL *syscall )( intptr_t arg, ... ) = ( intptr_t( QDECL * )( intptr_t, ... ) ) - 1;

Q_EXTERN_C void dllEntry( intptr_t( QDECL *syscallptr )( intptr_t arg, ... ) ) {
	syscall = syscallptr;
}

int PASSFLOAT( float x ) {
	float floatTemp;
	floatTemp = x;
	return *(int *)&floatTemp;
}

void trap_Print( const char *string ) {
	NATIVE_SYSCALL( (intptr_t)( UI_PRINT ), (intptr_t)( string ) );
}

void trap_Error( const char *string ) {
	NATIVE_SYSCALL( (intptr_t)( UI_ERROR ), (intptr_t)( string ) );
}

int trap_Milliseconds( void ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_MILLISECONDS ) );
}

void trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags ) {
	NATIVE_SYSCALL( (intptr_t)( UI_CVAR_REGISTER ), (intptr_t)( cvar ), (intptr_t)( var_name ), (intptr_t)( value ), (intptr_t)( flags ) );
}

void trap_Cvar_Update( vmCvar_t *cvar ) {
	NATIVE_SYSCALL( (intptr_t)( UI_CVAR_UPDATE ), (intptr_t)( cvar ) );
}

void trap_Cvar_Set( const char *var_name, const char *value ) {
	NATIVE_SYSCALL( (intptr_t)( UI_CVAR_SET ), (intptr_t)( var_name ), (intptr_t)( value ) );
}

float trap_Cvar_VariableValue( const char *var_name ) {
	int temp;
	temp = NATIVE_SYSCALL( (intptr_t)( UI_CVAR_VARIABLEVALUE ), (intptr_t)( var_name ) );
	return ( *(float *)&temp );
}

void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {
	NATIVE_SYSCALL( (intptr_t)( UI_CVAR_VARIABLESTRINGBUFFER ), (intptr_t)( var_name ), (intptr_t)( buffer ), (intptr_t)( bufsize ) );
}

void trap_Cvar_SetValue( const char *var_name, float value ) {
	NATIVE_SYSCALL( (intptr_t)( UI_CVAR_SETVALUE ), (intptr_t)( var_name ), (intptr_t)( PASSFLOAT( value ) ) );
}

void trap_Cvar_Reset( const char *name ) {
	NATIVE_SYSCALL( (intptr_t)( UI_CVAR_RESET ), (intptr_t)( name ) );
}

void trap_Cvar_Create( const char *var_name, const char *var_value, int flags ) {
	NATIVE_SYSCALL( (intptr_t)( UI_CVAR_CREATE ), (intptr_t)( var_name ), (intptr_t)( var_value ), (intptr_t)( flags ) );
}

void trap_Cvar_InfoStringBuffer( int bit, char *buffer, int bufsize ) {
	NATIVE_SYSCALL( (intptr_t)( UI_CVAR_INFOSTRINGBUFFER ), (intptr_t)( bit ), (intptr_t)( buffer ), (intptr_t)( bufsize ) );
}

int trap_Argc( void ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_ARGC ) );
}

void trap_Argv( int n, char *buffer, int bufferLength ) {
	NATIVE_SYSCALL( (intptr_t)( UI_ARGV ), (intptr_t)( n ), (intptr_t)( buffer ), (intptr_t)( bufferLength ) );
}

void trap_Cmd_ExecuteText( int exec_when, const char *text ) {
	NATIVE_SYSCALL( (intptr_t)( UI_CMD_EXECUTETEXT ), (intptr_t)( exec_when ), (intptr_t)( text ) );
}

int trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_FS_FOPENFILE ), (intptr_t)( qpath ), (intptr_t)( f ), (intptr_t)( mode ) );
}

void trap_FS_Read( void *buffer, int len, fileHandle_t f ) {
	NATIVE_SYSCALL( (intptr_t)( UI_FS_READ ), (intptr_t)( buffer ), (intptr_t)( len ), (intptr_t)( f ) );
}

void trap_FS_Write( const void *buffer, int len, fileHandle_t f ) {
	NATIVE_SYSCALL( (intptr_t)( UI_FS_WRITE ), (intptr_t)( buffer ), (intptr_t)( len ), (intptr_t)( f ) );
}

void trap_FS_FCloseFile( fileHandle_t f ) {
	NATIVE_SYSCALL( (intptr_t)( UI_FS_FCLOSEFILE ), (intptr_t)( f ) );
}

int trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_FS_GETFILELIST ), (intptr_t)( path ), (intptr_t)( extension ), (intptr_t)( listbuf ), (intptr_t)( bufsize ) );
}

int trap_FS_Seek( fileHandle_t f, long offset, int origin ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_FS_SEEK ), (intptr_t)( f ), (intptr_t)( offset ), (intptr_t)( origin ) );
}

qhandle_t trap_R_RegisterModel( const char *name ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_R_REGISTERMODEL ), (intptr_t)( name ) );
}

qhandle_t trap_R_RegisterSkin( const char *name ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_R_REGISTERSKIN ), (intptr_t)( name ) );
}

void trap_R_RegisterFont( const char *fontName, int pointSize, fontInfo_t *font ) {
	NATIVE_SYSCALL( (intptr_t)( UI_R_REGISTERFONT ), (intptr_t)( fontName ), (intptr_t)( pointSize ), (intptr_t)( font ) );
}

qhandle_t trap_R_RegisterShaderNoMip( const char *name ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_R_REGISTERSHADERNOMIP ), (intptr_t)( name ) );
}

void trap_R_ClearScene( void ) {
	NATIVE_SYSCALL( (intptr_t)( UI_R_CLEARSCENE ) );
}

void trap_R_AddRefEntityToScene( const refEntity_t *re ) {
	NATIVE_SYSCALL( (intptr_t)( UI_R_ADDREFENTITYTOSCENE ), (intptr_t)( re ) );
}

void trap_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts ) {
	NATIVE_SYSCALL( (intptr_t)( UI_R_ADDPOLYTOSCENE ), (intptr_t)( hShader ), (intptr_t)( numVerts ), (intptr_t)( verts ) );
}

void trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {
	NATIVE_SYSCALL( (intptr_t)( UI_R_ADDLIGHTTOSCENE ), (intptr_t)( org ), (intptr_t)( PASSFLOAT( intensity ) ), (intptr_t)( PASSFLOAT( r ) ), (intptr_t)( PASSFLOAT( g ) ), (intptr_t)( PASSFLOAT( b ) ) );
}

void trap_R_RenderScene( const refdef_t *fd ) {
	NATIVE_SYSCALL( (intptr_t)( UI_R_RENDERSCENE ), (intptr_t)( fd ) );
}

void trap_R_SetColor( const float *rgba ) {
	NATIVE_SYSCALL( (intptr_t)( UI_R_SETCOLOR ), (intptr_t)( rgba ) );
}

void trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	NATIVE_SYSCALL( (intptr_t)( UI_R_DRAWSTRETCHPIC ), (intptr_t)( PASSFLOAT( x ) ), (intptr_t)( PASSFLOAT( y ) ), (intptr_t)( PASSFLOAT( w ) ), (intptr_t)( PASSFLOAT( h ) ), (intptr_t)( PASSFLOAT( s1 ) ), (intptr_t)( PASSFLOAT( t1 ) ), (intptr_t)( PASSFLOAT( s2 ) ), (intptr_t)( PASSFLOAT( t2 ) ), (intptr_t)( hShader ) );
}

void trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs ) {
	NATIVE_SYSCALL( (intptr_t)( UI_R_MODELBOUNDS ), (intptr_t)( model ), (intptr_t)( mins ), (intptr_t)( maxs ) );
}

void trap_UpdateScreen( void ) {
	NATIVE_SYSCALL( (intptr_t)( UI_UPDATESCREEN ) );
}

int trap_CM_LerpTag( orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame, float frac, const char *tagName ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_CM_LERPTAG ), (intptr_t)( tag ), (intptr_t)( mod ), (intptr_t)( startFrame ), (intptr_t)( endFrame ), (intptr_t)( PASSFLOAT( frac ) ), (intptr_t)( tagName ) );
}

void trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum ) {
	NATIVE_SYSCALL( (intptr_t)( UI_S_STARTLOCALSOUND ), (intptr_t)( sfx ), (intptr_t)( channelNum ) );
}

sfxHandle_t trap_S_RegisterSound( const char *sample, qboolean compressed ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_S_REGISTERSOUND ), (intptr_t)( sample ), (intptr_t)( compressed ) );
}

void trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen ) {
	NATIVE_SYSCALL( (intptr_t)( UI_KEY_KEYNUMTOSTRINGBUF ), (intptr_t)( keynum ), (intptr_t)( buf ), (intptr_t)( buflen ) );
}

void trap_Key_GetBindingBuf( int keynum, char *buf, int buflen ) {
	NATIVE_SYSCALL( (intptr_t)( UI_KEY_GETBINDINGBUF ), (intptr_t)( keynum ), (intptr_t)( buf ), (intptr_t)( buflen ) );
}

void trap_Key_SetBinding( int keynum, const char *binding ) {
	NATIVE_SYSCALL( (intptr_t)( UI_KEY_SETBINDING ), (intptr_t)( keynum ), (intptr_t)( binding ) );
}

qboolean trap_Key_IsDown( int keynum ) {
	return (qboolean)NATIVE_SYSCALL( (intptr_t)( UI_KEY_ISDOWN ), (intptr_t)( keynum ) );
}

qboolean trap_Key_GetOverstrikeMode( void ) {
	return (qboolean)NATIVE_SYSCALL( (intptr_t)( UI_KEY_GETOVERSTRIKEMODE ) );
}

void trap_Key_SetOverstrikeMode( qboolean state ) {
	NATIVE_SYSCALL( (intptr_t)( UI_KEY_SETOVERSTRIKEMODE ), (intptr_t)( state ) );
}

void trap_Key_ClearStates( void ) {
	NATIVE_SYSCALL( (intptr_t)( UI_KEY_CLEARSTATES ) );
}

int trap_Key_GetCatcher( void ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_KEY_GETCATCHER ) );
}

void trap_Key_SetCatcher( int catcher ) {
	NATIVE_SYSCALL( (intptr_t)( UI_KEY_SETCATCHER ), (intptr_t)( catcher ) );
}

void trap_GetClipboardData( char *buf, int bufsize ) {
	NATIVE_SYSCALL( (intptr_t)( UI_GETCLIPBOARDDATA ), (intptr_t)( buf ), (intptr_t)( bufsize ) );
}

void trap_GetClientState( uiClientState_t *state ) {
	NATIVE_SYSCALL( (intptr_t)( UI_GETCLIENTSTATE ), (intptr_t)( state ) );
}

void trap_GetGlconfig( glconfig_t *glconfig ) {
	NATIVE_SYSCALL( (intptr_t)( UI_GETGLCONFIG ), (intptr_t)( glconfig ) );
}

int trap_GetConfigString( int index, char *buff, int buffsize ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_GETCONFIGSTRING ), (intptr_t)( index ), (intptr_t)( buff ), (intptr_t)( buffsize ) );
}

int trap_LAN_GetServerCount( int source ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_LAN_GETSERVERCOUNT ), (intptr_t)( source ) );
}

void trap_LAN_GetServerAddressString( int source, int n, char *buf, int buflen ) {
	NATIVE_SYSCALL( (intptr_t)( UI_LAN_GETSERVERADDRESSSTRING ), (intptr_t)( source ), (intptr_t)( n ), (intptr_t)( buf ), (intptr_t)( buflen ) );
}

void trap_LAN_GetServerInfo( int source, int n, char *buf, int buflen ) {
	NATIVE_SYSCALL( (intptr_t)( UI_LAN_GETSERVERINFO ), (intptr_t)( source ), (intptr_t)( n ), (intptr_t)( buf ), (intptr_t)( buflen ) );
}

int trap_LAN_GetServerPing( int source, int n ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_LAN_GETSERVERPING ), (intptr_t)( source ), (intptr_t)( n ) );
}

int trap_LAN_GetPingQueueCount( void ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_LAN_GETPINGQUEUECOUNT ) );
}

int trap_LAN_ServerStatus( const char *serverAddress, char *serverStatus, int maxLen ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_LAN_SERVERSTATUS ), (intptr_t)( serverAddress ), (intptr_t)( serverStatus ), (intptr_t)( maxLen ) );
}

void trap_LAN_SaveCachedServers() {
	NATIVE_SYSCALL( (intptr_t)( UI_LAN_SAVECACHEDSERVERS ) );
}

void trap_LAN_LoadCachedServers() {
	NATIVE_SYSCALL( (intptr_t)( UI_LAN_LOADCACHEDSERVERS ) );
}

void trap_LAN_ResetPings( int n ) {
	NATIVE_SYSCALL( (intptr_t)( UI_LAN_RESETPINGS ), (intptr_t)( n ) );
}

void trap_LAN_ClearPing( int n ) {
	NATIVE_SYSCALL( (intptr_t)( UI_LAN_CLEARPING ), (intptr_t)( n ) );
}

void trap_LAN_GetPing( int n, char *buf, int buflen, int *pingtime ) {
	NATIVE_SYSCALL( (intptr_t)( UI_LAN_GETPING ), (intptr_t)( n ), (intptr_t)( buf ), (intptr_t)( buflen ), (intptr_t)( pingtime ) );
}

void trap_LAN_GetPingInfo( int n, char *buf, int buflen ) {
	NATIVE_SYSCALL( (intptr_t)( UI_LAN_GETPINGINFO ), (intptr_t)( n ), (intptr_t)( buf ), (intptr_t)( buflen ) );
}

void trap_LAN_MarkServerVisible( int source, int n, qboolean visible ) {
	NATIVE_SYSCALL( (intptr_t)( UI_LAN_MARKSERVERVISIBLE ), (intptr_t)( source ), (intptr_t)( n ), (intptr_t)( visible ) );
}

int trap_LAN_ServerIsVisible( int source, int n ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_LAN_SERVERISVISIBLE ), (intptr_t)( source ), (intptr_t)( n ) );
}

qboolean trap_LAN_UpdateVisiblePings( int source ) {
	return (qboolean)NATIVE_SYSCALL( (intptr_t)( UI_LAN_UPDATEVISIBLEPINGS ), (intptr_t)( source ) );
}

int trap_LAN_AddServer( int source, const char *name, const char *addr ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_LAN_ADDSERVER ), (intptr_t)( source ), (intptr_t)( name ), (intptr_t)( addr ) );
}

void trap_LAN_RemoveServer( int source, const char *addr ) {
	NATIVE_SYSCALL( (intptr_t)( UI_LAN_REMOVESERVER ), (intptr_t)( source ), (intptr_t)( addr ) );
}

int trap_LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_LAN_COMPARESERVERS ), (intptr_t)( source ), (intptr_t)( sortKey ), (intptr_t)( sortDir ), (intptr_t)( s1 ), (intptr_t)( s2 ) );
}

int trap_MemoryRemaining( void ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_MEMORY_REMAINING ) );
}

void trap_GetCDKey( char *buf, int buflen ) {
	NATIVE_SYSCALL( (intptr_t)( UI_GET_CDKEY ), (intptr_t)( buf ), (intptr_t)( buflen ) );
}

void trap_SetCDKey( char *buf ) {
	NATIVE_SYSCALL( (intptr_t)( UI_SET_CDKEY ), (intptr_t)( buf ) );
}

int trap_PC_AddGlobalDefine( char *define ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_PC_ADD_GLOBAL_DEFINE ), (intptr_t)( define ) );
}

int trap_PC_LoadSource( const char *filename ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_PC_LOAD_SOURCE ), (intptr_t)( filename ) );
}

int trap_PC_FreeSource( int handle ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_PC_FREE_SOURCE ), (intptr_t)( handle ) );
}

int trap_PC_ReadToken( int handle, pc_token_t *pc_token ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_PC_READ_TOKEN ), (intptr_t)( handle ), (intptr_t)( pc_token ) );
}

int trap_PC_SourceFileAndLine( int handle, char *filename, int *line ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_PC_SOURCE_FILE_AND_LINE ), (intptr_t)( handle ), (intptr_t)( filename ), (intptr_t)( line ) );
}

void trap_S_StopBackgroundTrack( void ) {
	NATIVE_SYSCALL( (intptr_t)( UI_S_STOPBACKGROUNDTRACK ) );
}

void trap_S_StartBackgroundTrack( const char *intro, const char *loop ) {
	NATIVE_SYSCALL( (intptr_t)( UI_S_STARTBACKGROUNDTRACK ), (intptr_t)( intro ), (intptr_t)( loop ) );
}

int trap_RealTime( qtime_t *qtime ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_REAL_TIME ), (intptr_t)( qtime ) );
}

// this returns a handle.  arg0 is the name in the format "idlogo.roq", set arg1 to NULL, alteredstates to qfalse (do not alter gamestate)
int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits ) {
	return NATIVE_SYSCALL( (intptr_t)( UI_CIN_PLAYCINEMATIC ), (intptr_t)( arg0 ), (intptr_t)( xpos ), (intptr_t)( ypos ), (intptr_t)( width ), (intptr_t)( height ), (intptr_t)( bits ) );
}

// stops playing the cinematic and ends it.  should always return FMV_EOF
// cinematics must be stopped in reverse order of when they are started
e_status trap_CIN_StopCinematic( int handle ) {
	return (e_status)NATIVE_SYSCALL( (intptr_t)( UI_CIN_STOPCINEMATIC ), (intptr_t)( handle ) );
}


// will run a frame of the cinematic but will not draw it.  Will return FMV_EOF if the end of the cinematic has been reached.
e_status trap_CIN_RunCinematic( int handle ) {
	return (e_status)NATIVE_SYSCALL( (intptr_t)( UI_CIN_RUNCINEMATIC ), (intptr_t)( handle ) );
}


// draws the current frame
void trap_CIN_DrawCinematic( int handle ) {
	NATIVE_SYSCALL( (intptr_t)( UI_CIN_DRAWCINEMATIC ), (intptr_t)( handle ) );
}


// allows you to resize the animation dynamically
void trap_CIN_SetExtents( int handle, int x, int y, int w, int h ) {
	NATIVE_SYSCALL( (intptr_t)( UI_CIN_SETEXTENTS ), (intptr_t)( handle ), (intptr_t)( x ), (intptr_t)( y ), (intptr_t)( w ), (intptr_t)( h ) );
}


void trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset ) {
	NATIVE_SYSCALL( (intptr_t)( UI_R_REMAP_SHADER ), (intptr_t)( oldShader ), (intptr_t)( newShader ), (intptr_t)( timeOffset ) );
}

qboolean trap_VerifyCDKey( const char *key, const char *chksum ) {
	return (qboolean)NATIVE_SYSCALL( (intptr_t)( UI_VERIFY_CDKEY ), (intptr_t)( key ), (intptr_t)( chksum ) );
}

void trap_SetPbClStatus( int status ) {
	NATIVE_SYSCALL( (intptr_t)( UI_SET_PBCLSTATUS ), (intptr_t)( status ) );
}

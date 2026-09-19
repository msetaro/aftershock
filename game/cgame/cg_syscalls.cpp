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
// cg_syscalls.c -- this file is only included when building a dll
// cg_syscalls.asm is included instead when building a qvm
#ifdef Q3_VM
#error "Do not use in VM build"
#endif

#include "cg_local.h"

static intptr_t( QDECL *syscall )( intptr_t arg, ... ) = ( intptr_t( QDECL * )( intptr_t, ... ) ) - 1;


Q_EXTERN_C void dllEntry( intptr_t( QDECL *syscallptr )( intptr_t arg, ... ) ) {
	syscall = syscallptr;
}


int PASSFLOAT( float x ) {
	float floatTemp;
	floatTemp = x;
	return *(int *)&floatTemp;
}

void trap_Print( const char *fmt ) {
	NATIVE_SYSCALL( (intptr_t)( CG_PRINT ), (intptr_t)( fmt ) );
}

void trap_Error( const char *fmt ) {
	NATIVE_SYSCALL( (intptr_t)( CG_ERROR ), (intptr_t)( fmt ) );
}

int trap_Milliseconds( void ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_MILLISECONDS ) );
}

void trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags ) {
	NATIVE_SYSCALL( (intptr_t)( CG_CVAR_REGISTER ), (intptr_t)( vmCvar ), (intptr_t)( varName ), (intptr_t)( defaultValue ), (intptr_t)( flags ) );
}

void trap_Cvar_Update( vmCvar_t *vmCvar ) {
	NATIVE_SYSCALL( (intptr_t)( CG_CVAR_UPDATE ), (intptr_t)( vmCvar ) );
}

void trap_Cvar_Set( const char *var_name, const char *value ) {
	NATIVE_SYSCALL( (intptr_t)( CG_CVAR_SET ), (intptr_t)( var_name ), (intptr_t)( value ) );
}

void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {
	NATIVE_SYSCALL( (intptr_t)( CG_CVAR_VARIABLESTRINGBUFFER ), (intptr_t)( var_name ), (intptr_t)( buffer ), (intptr_t)( bufsize ) );
}

int trap_Argc( void ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_ARGC ) );
}

void trap_Argv( int n, char *buffer, int bufferLength ) {
	NATIVE_SYSCALL( (intptr_t)( CG_ARGV ), (intptr_t)( n ), (intptr_t)( buffer ), (intptr_t)( bufferLength ) );
}

void trap_Args( char *buffer, int bufferLength ) {
	NATIVE_SYSCALL( (intptr_t)( CG_ARGS ), (intptr_t)( buffer ), (intptr_t)( bufferLength ) );
}

int trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_FS_FOPENFILE ), (intptr_t)( qpath ), (intptr_t)( f ), (intptr_t)( mode ) );
}

void trap_FS_Read( void *buffer, int len, fileHandle_t f ) {
	NATIVE_SYSCALL( (intptr_t)( CG_FS_READ ), (intptr_t)( buffer ), (intptr_t)( len ), (intptr_t)( f ) );
}

void trap_FS_Write( const void *buffer, int len, fileHandle_t f ) {
	NATIVE_SYSCALL( (intptr_t)( CG_FS_WRITE ), (intptr_t)( buffer ), (intptr_t)( len ), (intptr_t)( f ) );
}

void trap_FS_FCloseFile( fileHandle_t f ) {
	NATIVE_SYSCALL( (intptr_t)( CG_FS_FCLOSEFILE ), (intptr_t)( f ) );
}

int trap_FS_Seek( fileHandle_t f, int64_t offset, int origin ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_FS_SEEK ), (intptr_t)( f ), (intptr_t)( offset ), (intptr_t)( origin ) );
}

void trap_SendConsoleCommand( const char *text ) {
	NATIVE_SYSCALL( (intptr_t)( CG_SENDCONSOLECOMMAND ), (intptr_t)( text ) );
}

void trap_AddCommand( const char *cmdName ) {
	NATIVE_SYSCALL( (intptr_t)( CG_ADDCOMMAND ), (intptr_t)( cmdName ) );
}

void trap_RemoveCommand( const char *cmdName ) {
	NATIVE_SYSCALL( (intptr_t)( CG_REMOVECOMMAND ), (intptr_t)( cmdName ) );
}

void trap_SendClientCommand( const char *s ) {
	NATIVE_SYSCALL( (intptr_t)( CG_SENDCLIENTCOMMAND ), (intptr_t)( s ) );
}

void trap_UpdateScreen( void ) {
	NATIVE_SYSCALL( (intptr_t)( CG_UPDATESCREEN ) );
}

void trap_CM_LoadMap( const char *mapname ) {
	NATIVE_SYSCALL( (intptr_t)( CG_CM_LOADMAP ), (intptr_t)( mapname ) );
}

int trap_CM_NumInlineModels( void ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_CM_NUMINLINEMODELS ) );
}

clipHandle_t trap_CM_InlineModel( int index ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_CM_INLINEMODEL ), (intptr_t)( index ) );
}

clipHandle_t trap_CM_TempBoxModel( const vec3_t mins, const vec3_t maxs ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_CM_TEMPBOXMODEL ), (intptr_t)( mins ), (intptr_t)( maxs ) );
}

clipHandle_t trap_CM_TempCapsuleModel( const vec3_t mins, const vec3_t maxs ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_CM_TEMPCAPSULEMODEL ), (intptr_t)( mins ), (intptr_t)( maxs ) );
}

int trap_CM_PointContents( const vec3_t p, clipHandle_t model ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_CM_POINTCONTENTS ), (intptr_t)( p ), (intptr_t)( model ) );
}

int trap_CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_CM_TRANSFORMEDPOINTCONTENTS ), (intptr_t)( p ), (intptr_t)( model ), (intptr_t)( origin ), (intptr_t)( angles ) );
}

void trap_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
	const vec3_t mins, const vec3_t maxs,
	clipHandle_t model, int brushmask ) {
	NATIVE_SYSCALL( (intptr_t)( CG_CM_BOXTRACE ), (intptr_t)( results ), (intptr_t)( start ), (intptr_t)( end ), (intptr_t)( mins ), (intptr_t)( maxs ), (intptr_t)( model ), (intptr_t)( brushmask ) );
}

void trap_CM_CapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end,
	const vec3_t mins, const vec3_t maxs,
	clipHandle_t model, int brushmask ) {
	NATIVE_SYSCALL( (intptr_t)( CG_CM_CAPSULETRACE ), (intptr_t)( results ), (intptr_t)( start ), (intptr_t)( end ), (intptr_t)( mins ), (intptr_t)( maxs ), (intptr_t)( model ), (intptr_t)( brushmask ) );
}

void trap_CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
	const vec3_t mins, const vec3_t maxs,
	clipHandle_t model, int brushmask,
	const vec3_t origin, const vec3_t angles ) {
	NATIVE_SYSCALL( (intptr_t)( CG_CM_TRANSFORMEDBOXTRACE ), (intptr_t)( results ), (intptr_t)( start ), (intptr_t)( end ), (intptr_t)( mins ), (intptr_t)( maxs ), (intptr_t)( model ), (intptr_t)( brushmask ), (intptr_t)( origin ), (intptr_t)( angles ) );
}

void trap_CM_TransformedCapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end,
	const vec3_t mins, const vec3_t maxs,
	clipHandle_t model, int brushmask,
	const vec3_t origin, const vec3_t angles ) {
	NATIVE_SYSCALL( (intptr_t)( CG_CM_TRANSFORMEDCAPSULETRACE ), (intptr_t)( results ), (intptr_t)( start ), (intptr_t)( end ), (intptr_t)( mins ), (intptr_t)( maxs ), (intptr_t)( model ), (intptr_t)( brushmask ), (intptr_t)( origin ), (intptr_t)( angles ) );
}

int trap_CM_MarkFragments( int numPoints, const vec3_t *points,
	const vec3_t projection,
	int maxPoints, vec3_t pointBuffer,
	int maxFragments, markFragment_t *fragmentBuffer ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_CM_MARKFRAGMENTS ), (intptr_t)( numPoints ), (intptr_t)( points ), (intptr_t)( projection ), (intptr_t)( maxPoints ), (intptr_t)( pointBuffer ), (intptr_t)( maxFragments ), (intptr_t)( fragmentBuffer ) );
}

void trap_S_StartSound( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx ) {
	NATIVE_SYSCALL( (intptr_t)( CG_S_STARTSOUND ), (intptr_t)( origin ), (intptr_t)( entityNum ), (intptr_t)( entchannel ), (intptr_t)( sfx ) );
}

void trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum ) {
	NATIVE_SYSCALL( (intptr_t)( CG_S_STARTLOCALSOUND ), (intptr_t)( sfx ), (intptr_t)( channelNum ) );
}

void trap_S_ClearLoopingSounds( qboolean killall ) {
	NATIVE_SYSCALL( (intptr_t)( CG_S_CLEARLOOPINGSOUNDS ), (intptr_t)( killall ) );
}

void trap_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx ) {
	NATIVE_SYSCALL( (intptr_t)( CG_S_ADDLOOPINGSOUND ), (intptr_t)( entityNum ), (intptr_t)( origin ), (intptr_t)( velocity ), (intptr_t)( sfx ) );
}

void trap_S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx ) {
	NATIVE_SYSCALL( (intptr_t)( CG_S_ADDREALLOOPINGSOUND ), (intptr_t)( entityNum ), (intptr_t)( origin ), (intptr_t)( velocity ), (intptr_t)( sfx ) );
}

void trap_S_StopLoopingSound( int entityNum ) {
	NATIVE_SYSCALL( (intptr_t)( CG_S_STOPLOOPINGSOUND ), (intptr_t)( entityNum ) );
}

void trap_S_UpdateEntityPosition( int entityNum, const vec3_t origin ) {
	NATIVE_SYSCALL( (intptr_t)( CG_S_UPDATEENTITYPOSITION ), (intptr_t)( entityNum ), (intptr_t)( origin ) );
}

void trap_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater ) {
	NATIVE_SYSCALL( (intptr_t)( CG_S_RESPATIALIZE ), (intptr_t)( entityNum ), (intptr_t)( origin ), (intptr_t)( axis ), (intptr_t)( inwater ) );
}

sfxHandle_t trap_S_RegisterSound( const char *sample, qboolean compressed ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_S_REGISTERSOUND ), (intptr_t)( sample ), (intptr_t)( compressed ) );
}

void trap_S_StartBackgroundTrack( const char *intro, const char *loop ) {
	NATIVE_SYSCALL( (intptr_t)( CG_S_STARTBACKGROUNDTRACK ), (intptr_t)( intro ), (intptr_t)( loop ) );
}

void trap_R_LoadWorldMap( const char *mapname ) {
	NATIVE_SYSCALL( (intptr_t)( CG_R_LOADWORLDMAP ), (intptr_t)( mapname ) );
}

qhandle_t trap_R_RegisterModel( const char *name ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_R_REGISTERMODEL ), (intptr_t)( name ) );
}

qhandle_t trap_R_RegisterSkin( const char *name ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_R_REGISTERSKIN ), (intptr_t)( name ) );
}

qhandle_t trap_R_RegisterShader( const char *name ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_R_REGISTERSHADER ), (intptr_t)( name ) );
}

qhandle_t trap_R_RegisterShaderNoMip( const char *name ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_R_REGISTERSHADERNOMIP ), (intptr_t)( name ) );
}

void trap_R_RegisterFont( const char *fontName, int pointSize, fontInfo_t *font ) {
	NATIVE_SYSCALL( (intptr_t)( CG_R_REGISTERFONT ), (intptr_t)( fontName ), (intptr_t)( pointSize ), (intptr_t)( font ) );
}

void trap_R_ClearScene( void ) {
	NATIVE_SYSCALL( (intptr_t)( CG_R_CLEARSCENE ) );
}

void trap_R_AddRefEntityToScene( const refEntity_t *re ) {
	NATIVE_SYSCALL( (intptr_t)( CG_R_ADDREFENTITYTOSCENE ), (intptr_t)( re ) );
}

void trap_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts ) {
	NATIVE_SYSCALL( (intptr_t)( CG_R_ADDPOLYTOSCENE ), (intptr_t)( hShader ), (intptr_t)( numVerts ), (intptr_t)( verts ) );
}

void trap_R_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int num ) {
	NATIVE_SYSCALL( (intptr_t)( CG_R_ADDPOLYSTOSCENE ), (intptr_t)( hShader ), (intptr_t)( numVerts ), (intptr_t)( verts ), (intptr_t)( num ) );
}

int trap_R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_R_LIGHTFORPOINT ), (intptr_t)( point ), (intptr_t)( ambientLight ), (intptr_t)( directedLight ), (intptr_t)( lightDir ) );
}

void trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {
	NATIVE_SYSCALL( (intptr_t)( CG_R_ADDLIGHTTOSCENE ), (intptr_t)( org ), (intptr_t)( PASSFLOAT( intensity ) ), (intptr_t)( PASSFLOAT( r ) ), (intptr_t)( PASSFLOAT( g ) ), (intptr_t)( PASSFLOAT( b ) ) );
}

void trap_R_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {
	NATIVE_SYSCALL( (intptr_t)( CG_R_ADDADDITIVELIGHTTOSCENE ), (intptr_t)( org ), (intptr_t)( PASSFLOAT( intensity ) ), (intptr_t)( PASSFLOAT( r ) ), (intptr_t)( PASSFLOAT( g ) ), (intptr_t)( PASSFLOAT( b ) ) );
}

void trap_R_RenderScene( const refdef_t *fd ) {
	NATIVE_SYSCALL( (intptr_t)( CG_R_RENDERSCENE ), (intptr_t)( fd ) );
}

void trap_R_SetColor( const float *rgba ) {
	NATIVE_SYSCALL( (intptr_t)( CG_R_SETCOLOR ), (intptr_t)( rgba ) );
}

void trap_R_DrawStretchPic( float x, float y, float w, float h,
	float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	NATIVE_SYSCALL( (intptr_t)( CG_R_DRAWSTRETCHPIC ), (intptr_t)( PASSFLOAT( x ) ), (intptr_t)( PASSFLOAT( y ) ), (intptr_t)( PASSFLOAT( w ) ), (intptr_t)( PASSFLOAT( h ) ), (intptr_t)( PASSFLOAT( s1 ) ), (intptr_t)( PASSFLOAT( t1 ) ), (intptr_t)( PASSFLOAT( s2 ) ), (intptr_t)( PASSFLOAT( t2 ) ), (intptr_t)( hShader ) );
}

void trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs ) {
	NATIVE_SYSCALL( (intptr_t)( CG_R_MODELBOUNDS ), (intptr_t)( model ), (intptr_t)( mins ), (intptr_t)( maxs ) );
}

int trap_R_LerpTag( orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame,
	float frac, const char *tagName ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_R_LERPTAG ), (intptr_t)( tag ), (intptr_t)( mod ), (intptr_t)( startFrame ), (intptr_t)( endFrame ), (intptr_t)( PASSFLOAT( frac ) ), (intptr_t)( tagName ) );
}

void trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset ) {
	NATIVE_SYSCALL( (intptr_t)( CG_R_REMAP_SHADER ), (intptr_t)( oldShader ), (intptr_t)( newShader ), (intptr_t)( timeOffset ) );
}

void trap_GetGlconfig( glconfig_t *glconfig ) {
	NATIVE_SYSCALL( (intptr_t)( CG_GETGLCONFIG ), (intptr_t)( glconfig ) );
}

void trap_GetGameState( gameState_t *gamestate ) {
	NATIVE_SYSCALL( (intptr_t)( CG_GETGAMESTATE ), (intptr_t)( gamestate ) );
}

void trap_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime ) {
	NATIVE_SYSCALL( (intptr_t)( CG_GETCURRENTSNAPSHOTNUMBER ), (intptr_t)( snapshotNumber ), (intptr_t)( serverTime ) );
}

qboolean trap_GetSnapshot( int snapshotNumber, snapshot_t *snapshot ) {
	return (qboolean)NATIVE_SYSCALL( (intptr_t)( CG_GETSNAPSHOT ), (intptr_t)( snapshotNumber ), (intptr_t)( snapshot ) );
}

qboolean trap_GetServerCommand( int serverCommandNumber ) {
	return (qboolean)NATIVE_SYSCALL( (intptr_t)( CG_GETSERVERCOMMAND ), (intptr_t)( serverCommandNumber ) );
}

int trap_GetCurrentCmdNumber( void ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_GETCURRENTCMDNUMBER ) );
}

qboolean trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd ) {
	return (qboolean)NATIVE_SYSCALL( (intptr_t)( CG_GETUSERCMD ), (intptr_t)( cmdNumber ), (intptr_t)( ucmd ) );
}

void trap_SetUserCmdValue( int stateValue, float sensitivityScale ) {
	NATIVE_SYSCALL( (intptr_t)( CG_SETUSERCMDVALUE ), (intptr_t)( stateValue ), (intptr_t)( PASSFLOAT( sensitivityScale ) ) );
}


int trap_MemoryRemaining( void ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_MEMORY_REMAINING ) );
}

qboolean trap_Key_IsDown( int keynum ) {
	return (qboolean)NATIVE_SYSCALL( (intptr_t)( CG_KEY_ISDOWN ), (intptr_t)( keynum ) );
}

int trap_Key_GetCatcher( void ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_KEY_GETCATCHER ) );
}

void trap_Key_SetCatcher( int catcher ) {
	NATIVE_SYSCALL( (intptr_t)( CG_KEY_SETCATCHER ), (intptr_t)( catcher ) );
}

int trap_Key_GetKey( const char *binding ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_KEY_GETKEY ), (intptr_t)( binding ) );
}

int trap_PC_AddGlobalDefine( char *define ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_PC_ADD_GLOBAL_DEFINE ), (intptr_t)( define ) );
}

int trap_PC_LoadSource( const char *filename ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_PC_LOAD_SOURCE ), (intptr_t)( filename ) );
}

int trap_PC_FreeSource( int handle ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_PC_FREE_SOURCE ), (intptr_t)( handle ) );
}

int trap_PC_ReadToken( int handle, pc_token_t *pc_token ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_PC_READ_TOKEN ), (intptr_t)( handle ), (intptr_t)( pc_token ) );
}

int trap_PC_SourceFileAndLine( int handle, char *filename, int *line ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_PC_SOURCE_FILE_AND_LINE ), (intptr_t)( handle ), (intptr_t)( filename ), (intptr_t)( line ) );
}

void trap_S_StopBackgroundTrack( void ) {
	NATIVE_SYSCALL( (intptr_t)( CG_S_STOPBACKGROUNDTRACK ) );
}

int trap_RealTime( qtime_t *qtime ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_REAL_TIME ), (intptr_t)( qtime ) );
}

void trap_SnapVector( float *v ) {
	NATIVE_SYSCALL( (intptr_t)( CG_SNAPVECTOR ), (intptr_t)( v ) );
}

// this returns a handle.  arg0 is the name in the format "idlogo.roq", set arg1 to NULL, alteredstates to qfalse (do not alter gamestate)
int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits ) {
	return NATIVE_SYSCALL( (intptr_t)( CG_CIN_PLAYCINEMATIC ), (intptr_t)( arg0 ), (intptr_t)( xpos ), (intptr_t)( ypos ), (intptr_t)( width ), (intptr_t)( height ), (intptr_t)( bits ) );
}

// stops playing the cinematic and ends it.  should always return FMV_EOF
// cinematics must be stopped in reverse order of when they are started
e_status trap_CIN_StopCinematic( int handle ) {
	return (e_status)NATIVE_SYSCALL( (intptr_t)( CG_CIN_STOPCINEMATIC ), (intptr_t)( handle ) );
}


// will run a frame of the cinematic but will not draw it.  Will return FMV_EOF if the end of the cinematic has been reached.
e_status trap_CIN_RunCinematic( int handle ) {
	return (e_status)NATIVE_SYSCALL( (intptr_t)( CG_CIN_RUNCINEMATIC ), (intptr_t)( handle ) );
}


// draws the current frame
void trap_CIN_DrawCinematic( int handle ) {
	NATIVE_SYSCALL( (intptr_t)( CG_CIN_DRAWCINEMATIC ), (intptr_t)( handle ) );
}


// allows you to resize the animation dynamically
void trap_CIN_SetExtents( int handle, int x, int y, int w, int h ) {
	NATIVE_SYSCALL( (intptr_t)( CG_CIN_SETEXTENTS ), (intptr_t)( handle ), (intptr_t)( x ), (intptr_t)( y ), (intptr_t)( w ), (intptr_t)( h ) );
}

/*
qboolean trap_loadCamera( const char *name ) {
	return (qboolean)NATIVE_SYSCALL( (intptr_t)( CG_LOADCAMERA ), (intptr_t)( name ) );
}

void trap_startCamera(int time) {
	NATIVE_SYSCALL( (intptr_t)( CG_STARTCAMERA ), (intptr_t)( time ) );
}

qboolean trap_getCameraInfo( int time, vec3_t *origin, vec3_t *angles) {
	return (qboolean)NATIVE_SYSCALL( (intptr_t)( CG_GETCAMERAINFO ), (intptr_t)( time ), (intptr_t)( origin ), (intptr_t)( angles ) );
}
*/

qboolean trap_GetEntityToken( char *buffer, int bufferSize ) {
	return (qboolean)NATIVE_SYSCALL( (intptr_t)( CG_GET_ENTITY_TOKEN ), (intptr_t)( buffer ), (intptr_t)( bufferSize ) );
}

qboolean trap_R_inPVS( const vec3_t p1, const vec3_t p2 ) {
	return (qboolean)NATIVE_SYSCALL( (intptr_t)( CG_R_INPVS ), (intptr_t)( p1 ), (intptr_t)( p2 ) );
}

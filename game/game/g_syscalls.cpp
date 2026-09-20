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
#include "g_local.h"

// this file is only included when building a dll
// g_syscalls.asm is included instead when building a qvm
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

void trap_Printf( const char *fmt ) {
	NATIVE_SYSCALL( (intptr_t)( G_PRINT ), (intptr_t)( fmt ) );
}

void trap_Error( const char *fmt ) {
	NATIVE_SYSCALL( (intptr_t)( G_ERROR ), (intptr_t)( fmt ) );
}

int trap_Milliseconds( void ) {
	return NATIVE_SYSCALL( (intptr_t)( G_MILLISECONDS ) );
}
int trap_Argc( void ) {
	return NATIVE_SYSCALL( (intptr_t)( G_ARGC ) );
}

void trap_Argv( int n, char *buffer, int bufferLength ) {
	NATIVE_SYSCALL( (intptr_t)( G_ARGV ), (intptr_t)( n ), (intptr_t)( buffer ), (intptr_t)( bufferLength ) );
}

int trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	return NATIVE_SYSCALL( (intptr_t)( G_FS_FOPEN_FILE ), (intptr_t)( qpath ), (intptr_t)( f ), (intptr_t)( mode ) );
}

void trap_FS_Read( void *buffer, int len, fileHandle_t f ) {
	NATIVE_SYSCALL( (intptr_t)( G_FS_READ ), (intptr_t)( buffer ), (intptr_t)( len ), (intptr_t)( f ) );
}

void trap_FS_Write( const void *buffer, int len, fileHandle_t f ) {
	NATIVE_SYSCALL( (intptr_t)( G_FS_WRITE ), (intptr_t)( buffer ), (intptr_t)( len ), (intptr_t)( f ) );
}

void trap_FS_FCloseFile( fileHandle_t f ) {
	NATIVE_SYSCALL( (intptr_t)( G_FS_FCLOSE_FILE ), (intptr_t)( f ) );
}

int trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize ) {
	return NATIVE_SYSCALL( (intptr_t)( G_FS_GETFILELIST ), (intptr_t)( path ), (intptr_t)( extension ), (intptr_t)( listbuf ), (intptr_t)( bufsize ) );
}

int trap_FS_Seek( fileHandle_t f, fsOffset_t offset, int origin ) {
	return NATIVE_SYSCALL( (intptr_t)( G_FS_SEEK ), (intptr_t)( f ), (intptr_t)( offset ), (intptr_t)( origin ) );
}

void trap_SendConsoleCommand( int exec_when, const char *text ) {
	NATIVE_SYSCALL( (intptr_t)( G_SEND_CONSOLE_COMMAND ), (intptr_t)( exec_when ), (intptr_t)( text ) );
}

void trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags ) {
	NATIVE_SYSCALL( (intptr_t)( G_CVAR_REGISTER ), (intptr_t)( cvar ), (intptr_t)( var_name ), (intptr_t)( value ), (intptr_t)( flags ) );
}

void trap_Cvar_Update( vmCvar_t *cvar ) {
	NATIVE_SYSCALL( (intptr_t)( G_CVAR_UPDATE ), (intptr_t)( cvar ) );
}

void trap_Cvar_Set( const char *var_name, const char *value ) {
	NATIVE_SYSCALL( (intptr_t)( G_CVAR_SET ), (intptr_t)( var_name ), (intptr_t)( value ) );
}

int trap_Cvar_VariableIntegerValue( const char *var_name ) {
	return NATIVE_SYSCALL( (intptr_t)( G_CVAR_VARIABLE_INTEGER_VALUE ), (intptr_t)( var_name ) );
}

void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {
	NATIVE_SYSCALL( (intptr_t)( G_CVAR_VARIABLE_STRING_BUFFER ), (intptr_t)( var_name ), (intptr_t)( buffer ), (intptr_t)( bufsize ) );
}


void trap_LocateGameData( gentity_t *gEnts, int numGEntities, int sizeofGEntity_t,
	playerState_t *clients, int sizeofGClient ) {
	NATIVE_SYSCALL( (intptr_t)( G_LOCATE_GAME_DATA ), (intptr_t)( gEnts ), (intptr_t)( numGEntities ), (intptr_t)( sizeofGEntity_t ), (intptr_t)( clients ), (intptr_t)( sizeofGClient ) );
}

void trap_DropClient( int clientNum, const char *reason ) {
	NATIVE_SYSCALL( (intptr_t)( G_DROP_CLIENT ), (intptr_t)( clientNum ), (intptr_t)( reason ) );
}

void trap_SendServerCommand( int clientNum, const char *text ) {
	NATIVE_SYSCALL( (intptr_t)( G_SEND_SERVER_COMMAND ), (intptr_t)( clientNum ), (intptr_t)( text ) );
}

void trap_SetConfigstring( int num, const char *string ) {
	NATIVE_SYSCALL( (intptr_t)( G_SET_CONFIGSTRING ), (intptr_t)( num ), (intptr_t)( string ) );
}

void trap_GetConfigstring( int num, char *buffer, int bufferSize ) {
	NATIVE_SYSCALL( (intptr_t)( G_GET_CONFIGSTRING ), (intptr_t)( num ), (intptr_t)( buffer ), (intptr_t)( bufferSize ) );
}

void trap_GetUserinfo( int num, char *buffer, int bufferSize ) {
	NATIVE_SYSCALL( (intptr_t)( G_GET_USERINFO ), (intptr_t)( num ), (intptr_t)( buffer ), (intptr_t)( bufferSize ) );
}

void trap_SetUserinfo( int num, const char *buffer ) {
	NATIVE_SYSCALL( (intptr_t)( G_SET_USERINFO ), (intptr_t)( num ), (intptr_t)( buffer ) );
}

void trap_GetServerinfo( char *buffer, int bufferSize ) {
	NATIVE_SYSCALL( (intptr_t)( G_GET_SERVERINFO ), (intptr_t)( buffer ), (intptr_t)( bufferSize ) );
}

void trap_SetBrushModel( gentity_t *ent, const char *name ) {
	NATIVE_SYSCALL( (intptr_t)( G_SET_BRUSH_MODEL ), (intptr_t)( ent ), (intptr_t)( name ) );
}

void trap_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask ) {
	NATIVE_SYSCALL( (intptr_t)( G_TRACE ), (intptr_t)( results ), (intptr_t)( start ), (intptr_t)( mins ), (intptr_t)( maxs ), (intptr_t)( end ), (intptr_t)( passEntityNum ), (intptr_t)( contentmask ) );
}

void trap_TraceCapsule( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask ) {
	NATIVE_SYSCALL( (intptr_t)( G_TRACECAPSULE ), (intptr_t)( results ), (intptr_t)( start ), (intptr_t)( mins ), (intptr_t)( maxs ), (intptr_t)( end ), (intptr_t)( passEntityNum ), (intptr_t)( contentmask ) );
}

int trap_PointContents( const vec3_t point, int passEntityNum ) {
	return NATIVE_SYSCALL( (intptr_t)( G_POINT_CONTENTS ), (intptr_t)( point ), (intptr_t)( passEntityNum ) );
}


qboolean trap_InPVS( const vec3_t p1, const vec3_t p2 ) {
	return (qboolean)NATIVE_SYSCALL( (intptr_t)( G_IN_PVS ), (intptr_t)( p1 ), (intptr_t)( p2 ) );
}

qboolean trap_InPVSIgnorePortals( const vec3_t p1, const vec3_t p2 ) {
	return (qboolean)NATIVE_SYSCALL( (intptr_t)( G_IN_PVS_IGNORE_PORTALS ), (intptr_t)( p1 ), (intptr_t)( p2 ) );
}

void trap_AdjustAreaPortalState( gentity_t *ent, qboolean open ) {
	NATIVE_SYSCALL( (intptr_t)( G_ADJUST_AREA_PORTAL_STATE ), (intptr_t)( ent ), (intptr_t)( open ) );
}

qboolean trap_AreasConnected( int area1, int area2 ) {
	return (qboolean)NATIVE_SYSCALL( (intptr_t)( G_AREAS_CONNECTED ), (intptr_t)( area1 ), (intptr_t)( area2 ) );
}

void trap_LinkEntity( gentity_t *ent ) {
	NATIVE_SYSCALL( (intptr_t)( G_LINKENTITY ), (intptr_t)( ent ) );
}

void trap_UnlinkEntity( gentity_t *ent ) {
	NATIVE_SYSCALL( (intptr_t)( G_UNLINKENTITY ), (intptr_t)( ent ) );
}

int trap_EntitiesInBox( const vec3_t mins, const vec3_t maxs, int *list, int maxcount ) {
	return NATIVE_SYSCALL( (intptr_t)( G_ENTITIES_IN_BOX ), (intptr_t)( mins ), (intptr_t)( maxs ), (intptr_t)( list ), (intptr_t)( maxcount ) );
}

qboolean trap_EntityContact( const vec3_t mins, const vec3_t maxs, const gentity_t *ent ) {
	return (qboolean)NATIVE_SYSCALL( (intptr_t)( G_ENTITY_CONTACT ), (intptr_t)( mins ), (intptr_t)( maxs ), (intptr_t)( ent ) );
}

qboolean trap_EntityContactCapsule( const vec3_t mins, const vec3_t maxs, const gentity_t *ent ) {
	return (qboolean)NATIVE_SYSCALL( (intptr_t)( G_ENTITY_CONTACTCAPSULE ), (intptr_t)( mins ), (intptr_t)( maxs ), (intptr_t)( ent ) );
}

int trap_BotAllocateClient( void ) {
	return NATIVE_SYSCALL( (intptr_t)( G_BOT_ALLOCATE_CLIENT ) );
}

void trap_BotFreeClient( int clientNum ) {
	NATIVE_SYSCALL( (intptr_t)( G_BOT_FREE_CLIENT ), (intptr_t)( clientNum ) );
}

void trap_GetUsercmd( int clientNum, usercmd_t *cmd ) {
	NATIVE_SYSCALL( (intptr_t)( G_GET_USERCMD ), (intptr_t)( clientNum ), (intptr_t)( cmd ) );
}

qboolean trap_GetEntityToken( char *buffer, int bufferSize ) {
	return (qboolean)NATIVE_SYSCALL( (intptr_t)( G_GET_ENTITY_TOKEN ), (intptr_t)( buffer ), (intptr_t)( bufferSize ) );
}

int trap_DebugPolygonCreate( int color, int numPoints, vec3_t *points ) {
	return NATIVE_SYSCALL( (intptr_t)( G_DEBUG_POLYGON_CREATE ), (intptr_t)( color ), (intptr_t)( numPoints ), (intptr_t)( points ) );
}

void trap_DebugPolygonDelete( int id ) {
	NATIVE_SYSCALL( (intptr_t)( G_DEBUG_POLYGON_DELETE ), (intptr_t)( id ) );
}

int trap_RealTime( qtime_t *qtime ) {
	return NATIVE_SYSCALL( (intptr_t)( G_REAL_TIME ), (intptr_t)( qtime ) );
}

void trap_SnapVector( float *v ) {
	NATIVE_SYSCALL( (intptr_t)( G_SNAPVECTOR ), (intptr_t)( v ) );
	return;
}

// BotLib traps start here
int trap_BotLibSetup( void ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_SETUP ) );
}

int trap_BotLibShutdown( void ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_SHUTDOWN ) );
}

int trap_BotLibVarSet( char *var_name, char *value ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_LIBVAR_SET ), (intptr_t)( var_name ), (intptr_t)( value ) );
}

int trap_BotLibVarGet( char *var_name, char *value, int size ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_LIBVAR_GET ), (intptr_t)( var_name ), (intptr_t)( value ), (intptr_t)( size ) );
}

int trap_BotLibDefine( char *string ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_PC_ADD_GLOBAL_DEFINE ), (intptr_t)( string ) );
}

int trap_BotLibStartFrame( float time ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_START_FRAME ), (intptr_t)( PASSFLOAT( time ) ) );
}

int trap_BotLibLoadMap( const char *mapname ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_LOAD_MAP ), (intptr_t)( mapname ) );
}

int trap_BotLibUpdateEntity( int ent, void /* struct bot_updateentity_s */ *bue ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_UPDATENTITY ), (intptr_t)( ent ), (intptr_t)( bue ) );
}

int trap_BotLibTest( int parm0, char *parm1, vec3_t parm2, vec3_t parm3 ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_TEST ), (intptr_t)( parm0 ), (intptr_t)( parm1 ), (intptr_t)( parm2 ), (intptr_t)( parm3 ) );
}

int trap_BotGetSnapshotEntity( int clientNum, int sequence ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_GET_SNAPSHOT_ENTITY ), (intptr_t)( clientNum ), (intptr_t)( sequence ) );
}

int trap_BotGetServerCommand( int clientNum, char *message, int size ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_GET_CONSOLE_MESSAGE ), (intptr_t)( clientNum ), (intptr_t)( message ), (intptr_t)( size ) );
}

void trap_BotUserCommand( int clientNum, usercmd_t *ucmd ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_USER_COMMAND ), (intptr_t)( clientNum ), (intptr_t)( ucmd ) );
}

void trap_AAS_EntityInfo( int entnum, void /* struct aas_entityinfo_s */ *info ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AAS_ENTITY_INFO ), (intptr_t)( entnum ), (intptr_t)( info ) );
}

int trap_AAS_Initialized( void ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AAS_INITIALIZED ) );
}

void trap_AAS_PresenceTypeBoundingBox( int presencetype, vec3_t mins, vec3_t maxs ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX ), (intptr_t)( presencetype ), (intptr_t)( mins ), (intptr_t)( maxs ) );
}

float trap_AAS_Time( void ) {
	int temp;
	temp = NATIVE_SYSCALL( (intptr_t)( BOTLIB_AAS_TIME ) );
	return ( *(float *)&temp );
}

int trap_AAS_PointAreaNum( vec3_t point ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AAS_POINT_AREA_NUM ), (intptr_t)( point ) );
}

int trap_AAS_PointReachabilityAreaIndex( vec3_t point ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AAS_POINT_REACHABILITY_AREA_INDEX ), (intptr_t)( point ) );
}

int trap_AAS_TraceAreas( vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AAS_TRACE_AREAS ), (intptr_t)( start ), (intptr_t)( end ), (intptr_t)( areas ), (intptr_t)( points ), (intptr_t)( maxareas ) );
}

int trap_AAS_BBoxAreas( vec3_t absmins, vec3_t absmaxs, int *areas, int maxareas ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AAS_BBOX_AREAS ), (intptr_t)( absmins ), (intptr_t)( absmaxs ), (intptr_t)( areas ), (intptr_t)( maxareas ) );
}

int trap_AAS_AreaInfo( int areanum, void /* struct aas_areainfo_s */ *info ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AAS_AREA_INFO ), (intptr_t)( areanum ), (intptr_t)( info ) );
}

int trap_AAS_PointContents( vec3_t point ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AAS_POINT_CONTENTS ), (intptr_t)( point ) );
}

int trap_AAS_NextBSPEntity( int ent ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AAS_NEXT_BSP_ENTITY ), (intptr_t)( ent ) );
}

int trap_AAS_ValueForBSPEpairKey( int ent, char *key, char *value, int size ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY ), (intptr_t)( ent ), (intptr_t)( key ), (intptr_t)( value ), (intptr_t)( size ) );
}

int trap_AAS_VectorForBSPEpairKey( int ent, char *key, vec3_t v ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY ), (intptr_t)( ent ), (intptr_t)( key ), (intptr_t)( v ) );
}

int trap_AAS_FloatForBSPEpairKey( int ent, char *key, float *value ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY ), (intptr_t)( ent ), (intptr_t)( key ), (intptr_t)( value ) );
}

int trap_AAS_IntForBSPEpairKey( int ent, char *key, int *value ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY ), (intptr_t)( ent ), (intptr_t)( key ), (intptr_t)( value ) );
}

int trap_AAS_AreaReachability( int areanum ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AAS_AREA_REACHABILITY ), (intptr_t)( areanum ) );
}

int trap_AAS_AreaTravelTimeToGoalArea( int areanum, vec3_t origin, int goalareanum, int travelflags ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA ), (intptr_t)( areanum ), (intptr_t)( origin ), (intptr_t)( goalareanum ), (intptr_t)( travelflags ) );
}

int trap_AAS_EnableRoutingArea( int areanum, int enable ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AAS_ENABLE_ROUTING_AREA ), (intptr_t)( areanum ), (intptr_t)( enable ) );
}

int trap_AAS_PredictRoute( void /*struct aas_predictroute_s*/ *route, int areanum, vec3_t origin,
	int goalareanum, int travelflags, int maxareas, int maxtime,
	int stopevent, int stopcontents, int stoptfl, int stopareanum ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AAS_PREDICT_ROUTE ), (intptr_t)( route ), (intptr_t)( areanum ), (intptr_t)( origin ), (intptr_t)( goalareanum ), (intptr_t)( travelflags ), (intptr_t)( maxareas ), (intptr_t)( maxtime ), (intptr_t)( stopevent ), (intptr_t)( stopcontents ), (intptr_t)( stoptfl ), (intptr_t)( stopareanum ) );
}

int trap_AAS_AlternativeRouteGoals( vec3_t start, int startareanum, vec3_t goal, int goalareanum, int travelflags,
	void /*struct aas_altroutegoal_s*/ *altroutegoals, int maxaltroutegoals,
	int type ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AAS_ALTERNATIVE_ROUTE_GOAL ), (intptr_t)( start ), (intptr_t)( startareanum ), (intptr_t)( goal ), (intptr_t)( goalareanum ), (intptr_t)( travelflags ), (intptr_t)( altroutegoals ), (intptr_t)( maxaltroutegoals ), (intptr_t)( type ) );
}

int trap_AAS_Swimming( vec3_t origin ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AAS_SWIMMING ), (intptr_t)( origin ) );
}

int trap_AAS_PredictClientMovement( void /* struct aas_clientmove_s */ *move, int entnum, vec3_t origin, int presencetype, int onground, vec3_t velocity, vec3_t cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT ), (intptr_t)( move ), (intptr_t)( entnum ), (intptr_t)( origin ), (intptr_t)( presencetype ), (intptr_t)( onground ), (intptr_t)( velocity ), (intptr_t)( cmdmove ), (intptr_t)( cmdframes ), (intptr_t)( maxframes ), (intptr_t)( PASSFLOAT( frametime ) ), (intptr_t)( stopevent ), (intptr_t)( stopareanum ), (intptr_t)( visualize ) );
}

void trap_EA_Say( int client, char *str ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_SAY ), (intptr_t)( client ), (intptr_t)( str ) );
}

void trap_EA_SayTeam( int client, char *str ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_SAY_TEAM ), (intptr_t)( client ), (intptr_t)( str ) );
}

void trap_EA_Command( int client, char *command ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_COMMAND ), (intptr_t)( client ), (intptr_t)( command ) );
}

void trap_EA_Action( int client, int action ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_ACTION ), (intptr_t)( client ), (intptr_t)( action ) );
}

void trap_EA_Gesture( int client ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_GESTURE ), (intptr_t)( client ) );
}

void trap_EA_Talk( int client ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_TALK ), (intptr_t)( client ) );
}

void trap_EA_Attack( int client ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_ATTACK ), (intptr_t)( client ) );
}

void trap_EA_Use( int client ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_USE ), (intptr_t)( client ) );
}

void trap_EA_Respawn( int client ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_RESPAWN ), (intptr_t)( client ) );
}

void trap_EA_Crouch( int client ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_CROUCH ), (intptr_t)( client ) );
}

void trap_EA_MoveUp( int client ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_MOVE_UP ), (intptr_t)( client ) );
}

void trap_EA_MoveDown( int client ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_MOVE_DOWN ), (intptr_t)( client ) );
}

void trap_EA_MoveForward( int client ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_MOVE_FORWARD ), (intptr_t)( client ) );
}

void trap_EA_MoveBack( int client ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_MOVE_BACK ), (intptr_t)( client ) );
}

void trap_EA_MoveLeft( int client ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_MOVE_LEFT ), (intptr_t)( client ) );
}

void trap_EA_MoveRight( int client ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_MOVE_RIGHT ), (intptr_t)( client ) );
}

void trap_EA_SelectWeapon( int client, int weapon ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_SELECT_WEAPON ), (intptr_t)( client ), (intptr_t)( weapon ) );
}

void trap_EA_Jump( int client ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_JUMP ), (intptr_t)( client ) );
}

void trap_EA_DelayedJump( int client ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_DELAYED_JUMP ), (intptr_t)( client ) );
}

void trap_EA_Move( int client, vec3_t dir, float speed ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_MOVE ), (intptr_t)( client ), (intptr_t)( dir ), (intptr_t)( PASSFLOAT( speed ) ) );
}

void trap_EA_View( int client, vec3_t viewangles ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_VIEW ), (intptr_t)( client ), (intptr_t)( viewangles ) );
}

void trap_EA_EndRegular( int client, float thinktime ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_END_REGULAR ), (intptr_t)( client ), (intptr_t)( PASSFLOAT( thinktime ) ) );
}

void trap_EA_GetInput( int client, float thinktime, void /* struct bot_input_s */ *input ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_GET_INPUT ), (intptr_t)( client ), (intptr_t)( PASSFLOAT( thinktime ) ), (intptr_t)( input ) );
}

void trap_EA_ResetInput( int client ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_EA_RESET_INPUT ), (intptr_t)( client ) );
}

int trap_BotLoadCharacter( char *charfile, float skill ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_LOAD_CHARACTER ), (intptr_t)( charfile ), (intptr_t)( PASSFLOAT( skill ) ) );
}

void trap_BotFreeCharacter( int character ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_FREE_CHARACTER ), (intptr_t)( character ) );
}

float trap_Characteristic_Float( int character, int index ) {
	int temp;
	temp = NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_CHARACTERISTIC_FLOAT ), (intptr_t)( character ), (intptr_t)( index ) );
	return ( *(float *)&temp );
}

float trap_Characteristic_BFloat( int character, int index, float min, float max ) {
	int temp;
	temp = NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_CHARACTERISTIC_BFLOAT ), (intptr_t)( character ), (intptr_t)( index ), (intptr_t)( PASSFLOAT( min ) ), (intptr_t)( PASSFLOAT( max ) ) );
	return ( *(float *)&temp );
}

int trap_Characteristic_Integer( int character, int index ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_CHARACTERISTIC_INTEGER ), (intptr_t)( character ), (intptr_t)( index ) );
}

int trap_Characteristic_BInteger( int character, int index, int min, int max ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_CHARACTERISTIC_BINTEGER ), (intptr_t)( character ), (intptr_t)( index ), (intptr_t)( min ), (intptr_t)( max ) );
}

void trap_Characteristic_String( int character, int index, char *buf, int size ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_CHARACTERISTIC_STRING ), (intptr_t)( character ), (intptr_t)( index ), (intptr_t)( buf ), (intptr_t)( size ) );
}

int trap_BotAllocChatState( void ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_ALLOC_CHAT_STATE ) );
}

void trap_BotFreeChatState( int handle ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_FREE_CHAT_STATE ), (intptr_t)( handle ) );
}

void trap_BotQueueConsoleMessage( int chatstate, int type, char *message ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_QUEUE_CONSOLE_MESSAGE ), (intptr_t)( chatstate ), (intptr_t)( type ), (intptr_t)( message ) );
}

void trap_BotRemoveConsoleMessage( int chatstate, int handle ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_REMOVE_CONSOLE_MESSAGE ), (intptr_t)( chatstate ), (intptr_t)( handle ) );
}

int trap_BotNextConsoleMessage( int chatstate, void /* struct bot_consolemessage_s */ *cm ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_NEXT_CONSOLE_MESSAGE ), (intptr_t)( chatstate ), (intptr_t)( cm ) );
}

int trap_BotNumConsoleMessages( int chatstate ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_NUM_CONSOLE_MESSAGE ), (intptr_t)( chatstate ) );
}

void trap_BotInitialChat( int chatstate, char *type, int mcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_INITIAL_CHAT ), (intptr_t)( chatstate ), (intptr_t)( type ), (intptr_t)( mcontext ), (intptr_t)( var0 ), (intptr_t)( var1 ), (intptr_t)( var2 ), (intptr_t)( var3 ), (intptr_t)( var4 ), (intptr_t)( var5 ), (intptr_t)( var6 ), (intptr_t)( var7 ) );
}

int trap_BotNumInitialChats( int chatstate, char *type ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_NUM_INITIAL_CHATS ), (intptr_t)( chatstate ), (intptr_t)( type ) );
}

int trap_BotReplyChat( int chatstate, char *message, int mcontext, int vcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_REPLY_CHAT ), (intptr_t)( chatstate ), (intptr_t)( message ), (intptr_t)( mcontext ), (intptr_t)( vcontext ), (intptr_t)( var0 ), (intptr_t)( var1 ), (intptr_t)( var2 ), (intptr_t)( var3 ), (intptr_t)( var4 ), (intptr_t)( var5 ), (intptr_t)( var6 ), (intptr_t)( var7 ) );
}

int trap_BotChatLength( int chatstate ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_CHAT_LENGTH ), (intptr_t)( chatstate ) );
}

void trap_BotEnterChat( int chatstate, int client, int sendto ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_ENTER_CHAT ), (intptr_t)( chatstate ), (intptr_t)( client ), (intptr_t)( sendto ) );
}

void trap_BotGetChatMessage( int chatstate, char *buf, int size ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_GET_CHAT_MESSAGE ), (intptr_t)( chatstate ), (intptr_t)( buf ), (intptr_t)( size ) );
}

int trap_StringContains( char *str1, char *str2, int casesensitive ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_STRING_CONTAINS ), (intptr_t)( str1 ), (intptr_t)( str2 ), (intptr_t)( casesensitive ) );
}

int trap_BotFindMatch( char *str, void /* struct bot_match_s */ *match, uint64_t context ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_FIND_MATCH ), (intptr_t)( str ), (intptr_t)( match ), (intptr_t)( context ) );
}

void trap_BotMatchVariable( void /* struct bot_match_s */ *match, int variable, char *buf, int size ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_MATCH_VARIABLE ), (intptr_t)( match ), (intptr_t)( variable ), (intptr_t)( buf ), (intptr_t)( size ) );
}

void trap_UnifyWhiteSpaces( char *string ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_UNIFY_WHITE_SPACES ), (intptr_t)( string ) );
}

void trap_BotReplaceSynonyms( char *string, uint64_t context ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_REPLACE_SYNONYMS ), (intptr_t)( string ), (intptr_t)( context ) );
}

int trap_BotLoadChatFile( int chatstate, char *chatfile, char *chatname ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_LOAD_CHAT_FILE ), (intptr_t)( chatstate ), (intptr_t)( chatfile ), (intptr_t)( chatname ) );
}

void trap_BotSetChatGender( int chatstate, int gender ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_SET_CHAT_GENDER ), (intptr_t)( chatstate ), (intptr_t)( gender ) );
}

void trap_BotSetChatName( int chatstate, char *name, int client ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_SET_CHAT_NAME ), (intptr_t)( chatstate ), (intptr_t)( name ), (intptr_t)( client ) );
}

void trap_BotResetGoalState( int goalstate ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_RESET_GOAL_STATE ), (intptr_t)( goalstate ) );
}

void trap_BotResetAvoidGoals( int goalstate ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_RESET_AVOID_GOALS ), (intptr_t)( goalstate ) );
}

void trap_BotRemoveFromAvoidGoals( int goalstate, int number ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_REMOVE_FROM_AVOID_GOALS ), (intptr_t)( goalstate ), (intptr_t)( number ) );
}

void trap_BotPushGoal( int goalstate, void /* struct bot_goal_s */ *goal ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_PUSH_GOAL ), (intptr_t)( goalstate ), (intptr_t)( goal ) );
}

void trap_BotPopGoal( int goalstate ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_POP_GOAL ), (intptr_t)( goalstate ) );
}

void trap_BotEmptyGoalStack( int goalstate ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_EMPTY_GOAL_STACK ), (intptr_t)( goalstate ) );
}

void trap_BotDumpAvoidGoals( int goalstate ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_DUMP_AVOID_GOALS ), (intptr_t)( goalstate ) );
}

void trap_BotDumpGoalStack( int goalstate ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_DUMP_GOAL_STACK ), (intptr_t)( goalstate ) );
}

void trap_BotGoalName( int number, char *name, int size ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_GOAL_NAME ), (intptr_t)( number ), (intptr_t)( name ), (intptr_t)( size ) );
}

int trap_BotGetTopGoal( int goalstate, void /* struct bot_goal_s */ *goal ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_GET_TOP_GOAL ), (intptr_t)( goalstate ), (intptr_t)( goal ) );
}

int trap_BotGetSecondGoal( int goalstate, void /* struct bot_goal_s */ *goal ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_GET_SECOND_GOAL ), (intptr_t)( goalstate ), (intptr_t)( goal ) );
}

int trap_BotChooseLTGItem( int goalstate, vec3_t origin, int *inventory, int travelflags ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_CHOOSE_LTG_ITEM ), (intptr_t)( goalstate ), (intptr_t)( origin ), (intptr_t)( inventory ), (intptr_t)( travelflags ) );
}

int trap_BotChooseNBGItem( int goalstate, vec3_t origin, int *inventory, int travelflags, void /* struct bot_goal_s */ *ltg, float maxtime ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_CHOOSE_NBG_ITEM ), (intptr_t)( goalstate ), (intptr_t)( origin ), (intptr_t)( inventory ), (intptr_t)( travelflags ), (intptr_t)( ltg ), (intptr_t)( PASSFLOAT( maxtime ) ) );
}

int trap_BotTouchingGoal( vec3_t origin, void /* struct bot_goal_s */ *goal ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_TOUCHING_GOAL ), (intptr_t)( origin ), (intptr_t)( goal ) );
}

int trap_BotItemGoalInVisButNotVisible( int viewer, vec3_t eye, vec3_t viewangles, void /* struct bot_goal_s */ *goal ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE ), (intptr_t)( viewer ), (intptr_t)( eye ), (intptr_t)( viewangles ), (intptr_t)( goal ) );
}

int trap_BotGetLevelItemGoal( int index, char *classname, void /* struct bot_goal_s */ *goal ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_GET_LEVEL_ITEM_GOAL ), (intptr_t)( index ), (intptr_t)( classname ), (intptr_t)( goal ) );
}

int trap_BotGetNextCampSpotGoal( int num, void /* struct bot_goal_s */ *goal ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL ), (intptr_t)( num ), (intptr_t)( goal ) );
}

int trap_BotGetMapLocationGoal( char *name, void /* struct bot_goal_s */ *goal ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_GET_MAP_LOCATION_GOAL ), (intptr_t)( name ), (intptr_t)( goal ) );
}

float trap_BotAvoidGoalTime( int goalstate, int number ) {
	int temp;
	temp = NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_AVOID_GOAL_TIME ), (intptr_t)( goalstate ), (intptr_t)( number ) );
	return ( *(float *)&temp );
}

void trap_BotSetAvoidGoalTime( int goalstate, int number, float avoidtime ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_SET_AVOID_GOAL_TIME ), (intptr_t)( goalstate ), (intptr_t)( number ), (intptr_t)( PASSFLOAT( avoidtime ) ) );
}

void trap_BotInitLevelItems( void ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_INIT_LEVEL_ITEMS ) );
}

void trap_BotUpdateEntityItems( void ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_UPDATE_ENTITY_ITEMS ) );
}

int trap_BotLoadItemWeights( int goalstate, char *filename ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_LOAD_ITEM_WEIGHTS ), (intptr_t)( goalstate ), (intptr_t)( filename ) );
}

void trap_BotFreeItemWeights( int goalstate ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_FREE_ITEM_WEIGHTS ), (intptr_t)( goalstate ) );
}

void trap_BotInterbreedGoalFuzzyLogic( int parent1, int parent2, int child ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC ), (intptr_t)( parent1 ), (intptr_t)( parent2 ), (intptr_t)( child ) );
}

void trap_BotSaveGoalFuzzyLogic( int goalstate, char *filename ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC ), (intptr_t)( goalstate ), (intptr_t)( filename ) );
}

void trap_BotMutateGoalFuzzyLogic( int goalstate, float range ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC ), (intptr_t)( goalstate ), (intptr_t)( range ) );
}

int trap_BotAllocGoalState( int state ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_ALLOC_GOAL_STATE ), (intptr_t)( state ) );
}

void trap_BotFreeGoalState( int handle ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_FREE_GOAL_STATE ), (intptr_t)( handle ) );
}

void trap_BotResetMoveState( int movestate ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_RESET_MOVE_STATE ), (intptr_t)( movestate ) );
}

void trap_BotAddAvoidSpot( int movestate, vec3_t origin, float radius, int type ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_ADD_AVOID_SPOT ), (intptr_t)( movestate ), (intptr_t)( origin ), (intptr_t)( PASSFLOAT( radius ) ), (intptr_t)( type ) );
}

void trap_BotMoveToGoal( void /* struct bot_moveresult_s */ *result, int movestate, void /* struct bot_goal_s */ *goal, int travelflags ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_MOVE_TO_GOAL ), (intptr_t)( result ), (intptr_t)( movestate ), (intptr_t)( goal ), (intptr_t)( travelflags ) );
}

int trap_BotMoveInDirection( int movestate, vec3_t dir, float speed, int type ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_MOVE_IN_DIRECTION ), (intptr_t)( movestate ), (intptr_t)( dir ), (intptr_t)( PASSFLOAT( speed ) ), (intptr_t)( type ) );
}

void trap_BotResetAvoidReach( int movestate ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_RESET_AVOID_REACH ), (intptr_t)( movestate ) );
}

void trap_BotResetLastAvoidReach( int movestate ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_RESET_LAST_AVOID_REACH ), (intptr_t)( movestate ) );
}

int trap_BotReachabilityArea( vec3_t origin, int testground ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_REACHABILITY_AREA ), (intptr_t)( origin ), (intptr_t)( testground ) );
}

int trap_BotMovementViewTarget( int movestate, void /* struct bot_goal_s */ *goal, int travelflags, float lookahead, vec3_t target ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_MOVEMENT_VIEW_TARGET ), (intptr_t)( movestate ), (intptr_t)( goal ), (intptr_t)( travelflags ), (intptr_t)( PASSFLOAT( lookahead ) ), (intptr_t)( target ) );
}

int trap_BotPredictVisiblePosition( vec3_t origin, int areanum, void /* struct bot_goal_s */ *goal, int travelflags, vec3_t target ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_PREDICT_VISIBLE_POSITION ), (intptr_t)( origin ), (intptr_t)( areanum ), (intptr_t)( goal ), (intptr_t)( travelflags ), (intptr_t)( target ) );
}

int trap_BotAllocMoveState( void ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_ALLOC_MOVE_STATE ) );
}

void trap_BotFreeMoveState( int handle ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_FREE_MOVE_STATE ), (intptr_t)( handle ) );
}

void trap_BotInitMoveState( int handle, void /* struct bot_initmove_s */ *initmove ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_INIT_MOVE_STATE ), (intptr_t)( handle ), (intptr_t)( initmove ) );
}

int trap_BotChooseBestFightWeapon( int weaponstate, int *inventory ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON ), (intptr_t)( weaponstate ), (intptr_t)( inventory ) );
}

void trap_BotGetWeaponInfo( int weaponstate, int weapon, void /* struct weaponinfo_s */ *weaponinfo ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_GET_WEAPON_INFO ), (intptr_t)( weaponstate ), (intptr_t)( weapon ), (intptr_t)( weaponinfo ) );
}

int trap_BotLoadWeaponWeights( int weaponstate, char *filename ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_LOAD_WEAPON_WEIGHTS ), (intptr_t)( weaponstate ), (intptr_t)( filename ) );
}

int trap_BotAllocWeaponState( void ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_ALLOC_WEAPON_STATE ) );
}

void trap_BotFreeWeaponState( int weaponstate ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_FREE_WEAPON_STATE ), (intptr_t)( weaponstate ) );
}

void trap_BotResetWeaponState( int weaponstate ) {
	NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_RESET_WEAPON_STATE ), (intptr_t)( weaponstate ) );
}

int trap_GeneticParentsAndChildSelection( int numranks, float *ranks, int *parent1, int *parent2, int *child ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION ), (intptr_t)( numranks ), (intptr_t)( ranks ), (intptr_t)( parent1 ), (intptr_t)( parent2 ), (intptr_t)( child ) );
}

int trap_PC_LoadSource( const char *filename ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_PC_LOAD_SOURCE ), (intptr_t)( filename ) );
}

int trap_PC_FreeSource( int handle ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_PC_FREE_SOURCE ), (intptr_t)( handle ) );
}

int trap_PC_ReadToken( int handle, pc_token_t *pc_token ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_PC_READ_TOKEN ), (intptr_t)( handle ), (intptr_t)( pc_token ) );
}

int trap_PC_SourceFileAndLine( int handle, char *filename, int *line ) {
	return NATIVE_SYSCALL( (intptr_t)( BOTLIB_PC_SOURCE_FILE_AND_LINE ), (intptr_t)( handle ), (intptr_t)( filename ), (intptr_t)( line ) );
}

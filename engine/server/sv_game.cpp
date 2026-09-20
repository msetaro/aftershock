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
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// sv_game.c -- interface to the game dll

#include "server.h"
#ifdef AFTERSHOCK_DEVTOOLS
#include "../public/dev_game_public.h"
#endif
#include "../public/g_native_public.h"

#include "../botlib/botlib_public.h"

botlib_export_t *botlib_export;

// these functions must be used instead of pointer arithmetic, because
// the game allocates gentities with private information after the server shared part
int SV_NumForGentity( sharedEntity_t *ent ) {
	int num;

	num = (int)( ( (byte *)ent - (byte *)sv.gentities ) / sv.gentitySize );

	return num;
}


sharedEntity_t *SV_GentityNum( int num ) {
	sharedEntity_t *ent;

	if ( num < 0 || num >= MAX_GENTITIES ) {
		Com_Error( ERR_DROP, "%s: bad num %d", __func__, num );
	}
	ent = (sharedEntity_t *)( (byte *)sv.gentities + sv.gentitySize * ( num ) );

	return ent;
}


playerState_t *SV_GameClientNum( int num ) {
	playerState_t *ps;

	ps = (playerState_t *)( (byte *)sv.gameClients + sv.gameClientSize * ( num ) );

	return ps;
}


svEntity_t *SV_SvEntityForGentity( sharedEntity_t *gEnt ) {
	if ( !gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES ) {
		Com_Error( ERR_DROP, "SV_SvEntityForGentity: bad gEnt" );
	}
	return &sv.svEntities[gEnt->s.number];
}


sharedEntity_t *SV_GEntityForSvEntity( svEntity_t *svEnt ) {
	int num;

	num = (int)( svEnt - sv.svEntities );
	return SV_GentityNum( num );
}


/*
===============
SV_GameSendServerCommand

Sends a command string to a client
===============
*/
static void SV_GameSendServerCommand( int clientNum, const char *text ) {
	if ( clientNum == -1 ) {
		SV_SendServerCommand( NULL, "%s", text );
	} else {
		if ( clientNum < 0 || clientNum >= sv.maxclients ) {
			return;
		}
		SV_SendServerCommand( svs.clients + clientNum, "%s", text );
	}
}


/*
===============
SV_GameDropClient

Disconnects the client with a message
===============
*/
static void SV_GameDropClient( int clientNum, const char *reason ) {
	if ( clientNum < 0 || clientNum >= sv.maxclients ) {
		return;
	}
	SV_DropClient( svs.clients + clientNum, reason );
}


/*
=================
SV_SetBrushModel

sets mins and maxs for inline bmodels
=================
*/
static void SV_SetBrushModel( sharedEntity_t *ent, const char *name ) {
	clipHandle_t h;
	vec3_t mins, maxs;

	if ( !name ) {
		Com_Error( ERR_DROP, "SV_SetBrushModel: NULL" );
	}

	if ( name[0] != '*' ) {
		Com_Error( ERR_DROP, "SV_SetBrushModel: %s isn't a brush model", name );
	}

	ent->s.modelindex = atoi( name + 1 );

	h = CM_InlineModel( ent->s.modelindex );
	CM_ModelBounds( h, mins, maxs );
	VectorCopy( mins, ent->r.mins );
	VectorCopy( maxs, ent->r.maxs );
	ent->r.bmodel = qtrue;

	ent->r.contents = -1; // we don't know exactly what is in the brushes

	SV_LinkEntity( ent ); // FIXME: remove
}


/*
=================
SV_inPVS

Also checks portalareas so that doors block sight
=================
*/
qboolean SV_inPVS( const vec3_t p1, const vec3_t p2 ) {
	int leafnum;
	int cluster;
	int area1, area2;
	byte *mask;

	leafnum = CM_PointLeafnum( p1 );
	cluster = CM_LeafCluster( leafnum );
	if ( cluster < 0 )
		return qfalse;
	area1 = CM_LeafArea( leafnum );
	mask = CM_ClusterPVS( cluster );

	leafnum = CM_PointLeafnum( p2 );
	cluster = CM_LeafCluster( leafnum );
	if ( cluster < 0 )
		return qfalse;
	area2 = CM_LeafArea( leafnum );
	if ( mask && ( !( mask[cluster >> 3] & ( 1 << ( cluster & 7 ) ) ) ) )
		return qfalse;
	if ( !CM_AreasConnected( area1, area2 ) )
		return qfalse; // a door blocks sight
	return qtrue;
}


/*
=================
SV_inPVSIgnorePortals

Does NOT check portalareas
=================
*/
static qboolean SV_inPVSIgnorePortals( const vec3_t p1, const vec3_t p2 ) {
	int leafnum;
	int cluster;
	byte *mask;

	leafnum = CM_PointLeafnum( p1 );
	cluster = CM_LeafCluster( leafnum );
	if ( cluster < 0 )
		return qfalse;
	mask = CM_ClusterPVS( cluster );

	leafnum = CM_PointLeafnum( p2 );
	cluster = CM_LeafCluster( leafnum );
	if ( cluster < 0 )
		return qfalse;

	if ( mask && ( !( mask[cluster >> 3] & ( 1 << ( cluster & 7 ) ) ) ) )
		return qfalse;

	return qtrue;
}


/*
========================
SV_AdjustAreaPortalState
========================
*/
static void SV_AdjustAreaPortalState( sharedEntity_t *ent, qboolean open ) {
	svEntity_t *svEnt;

	svEnt = SV_SvEntityForGentity( ent );
	if ( svEnt->areanum2 == -1 ) {
		return;
	}
	CM_AdjustAreaPortalState( svEnt->areanum, svEnt->areanum2, open );
}


/*
==================
SV_EntityContact
==================
*/
static qboolean SV_EntityContact( const vec3_t mins, const vec3_t maxs, const sharedEntity_t *gEnt, const int capsule ) {
	const float *origin, *angles;
	clipHandle_t ch;
	trace_t trace;

	// check for exact collision
	origin = gEnt->r.currentOrigin;
	angles = gEnt->r.currentAngles;

	ch = SV_ClipHandleForEntity( gEnt );
	CM_TransformedBoxTrace( &trace, vec3_origin, vec3_origin, mins, maxs, ch, -1, origin, angles, (qboolean)capsule );

	return trace.startsolid;
}


/*
===============
SV_GetServerinfo
===============
*/
static void SV_GetServerinfo( char *buffer, int bufferSize ) {

	if ( bufferSize < 1 ) {
		Com_Error( ERR_DROP, "SV_GetServerinfo: bufferSize == %i", bufferSize );
	}
	if ( sv.state != SS_GAME || !sv.configstrings[CS_SERVERINFO] ) {
		Q_strncpyz( buffer, Cvar_InfoString( CVAR_SERVERINFO, NULL ), bufferSize );
	} else {
		Q_strncpyz( buffer, sv.configstrings[CS_SERVERINFO], bufferSize );
	}
}


/*
===============
SV_LocateGameData

===============
*/
static void SV_LocateGameData( sharedEntity_t *gEnts, unsigned numGEntities, unsigned sizeofGEntity_t,
	playerState_t *clients, unsigned sizeofGameClient ) {

	if ( numGEntities > MAX_GENTITIES || sizeofGEntity_t < sizeof( sharedEntity_t ) || sizeofGameClient < sizeof( playerState_t ) ) {
		Com_Error( ERR_DROP, "%s: invalid native game layout", __func__ );
	}

	sv.gentities = gEnts;
	sv.gentitySize = sizeofGEntity_t;
	sv.num_entities = numGEntities;

	sv.gameClients = clients;
	sv.gameClientSize = sizeofGameClient;
}


/*
===============
SV_GetUsercmd
===============
*/
static void SV_GetUsercmd( int clientNum, usercmd_t *cmd ) {
	if ( (unsigned)clientNum < (unsigned int)sv.maxclients ) {
		*cmd = svs.clients[clientNum].lastUsercmd;
	} else {
		Com_Error( ERR_DROP, "%s(): bad clientNum: %i", __func__, clientNum );
	}
}


//==============================================


void GameImport_Printf( const char *fmt ) {
	Com_Printf( "%s", (const char *)fmt );
	return;
}
void GameImport_Error( const char *fmt ) {
	Com_Error( ERR_DROP, "%s", (const char *)fmt );
	return;
}
int GameImport_Milliseconds( void ) {
	return Sys_Milliseconds();
}
int GameImport_Argc( void ) {
	return Cmd_Argc();
}
void GameImport_Argv( int n, char *buffer, int bufferLength ) {

	Cmd_ArgvBuffer( n, (char *)buffer, bufferLength );
	return;
}
int GameImport_FS_FOpenFile( const char *qpath, void *f, int mode ) {
	return FS_VM_OpenFile( (const char *)qpath, (fileHandle_t *)f, (fsMode_t)mode, H_QAGAME );
}
void GameImport_FS_Read( void *buffer, int len, int f ) {
	if ( f == 0 ) // UrT may pass this with len=-1 and cause false bounds check error
		return;

	FS_VM_ReadFile( buffer, len, f, H_QAGAME );
	return;
}
void GameImport_FS_Write( const void *buffer, int len, int f ) {

	FS_VM_WriteFile( (void *)buffer, len, f, H_QAGAME );
	return;
}
void GameImport_FS_FCloseFile( int f ) {
	FS_VM_CloseFile( f, H_QAGAME );
	return;
}
int GameImport_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize ) {

	return FS_GetFileList( (const char *)path, (const char *)extension, (char *)listbuf, bufsize );
}
int GameImport_FS_Seek( int f, int64_t offset, int origin ) {
	return FS_VM_SeekFile( f, (fsOffset_t)offset, (fsOrigin_t)origin, H_QAGAME );
}
void GameImport_SendConsoleCommand( int exec_when, const char *text ) {
	Cbuf_ExecuteText( (cbufExec_t)exec_when, (const char *)text );
	return;
}
void GameImport_Cvar_Register( void *cvar, const char *var_name, const char *value, int flags ) {
	Cvar_Register( (vmCvar_t *)cvar, (const char *)var_name, (const char *)value, flags, 0 );
	return;
}
void GameImport_Cvar_Update( void *cvar ) {
	Cvar_Update( (vmCvar_t *)cvar, 0 );
	return;
}
void GameImport_Cvar_Set( const char *var_name, const char *value ) {
	Cvar_SetSafe( (const char *)var_name, (const char *)value );
	return;
}
int GameImport_Cvar_VariableIntegerValue( const char *var_name ) {
	return Cvar_VariableIntegerValue( (const char *)var_name );
}
void GameImport_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {

	Cvar_VariableStringBufferSafe( (const char *)var_name, (char *)buffer, bufsize, 0 );
	return;
}
void GameImport_LocateGameData( void *gEnts, int numGEntities, int sizeofGEntity_t, void *clients, int sizeofGClient ) {
	SV_LocateGameData( (sharedEntity_t *)gEnts, numGEntities, sizeofGEntity_t, (playerState_t *)clients, sizeofGClient );
	return;
}
void GameImport_DropClient( int clientNum, const char *reason ) {
	SV_GameDropClient( clientNum, (const char *)reason );
	return;
}
void GameImport_SendServerCommand( int clientNum, const char *text ) {
	SV_GameSendServerCommand( clientNum, (const char *)text );
	return;
}
void GameImport_SetConfigstring( int num, const char *string ) {
	SV_SetConfigstring( num, (const char *)string );
	return;
}
void GameImport_GetConfigstring( int num, char *buffer, int bufferSize ) {

	SV_GetConfigstring( num, (char *)buffer, bufferSize );
	return;
}
void GameImport_GetUserinfo( int num, char *buffer, int bufferSize ) {

	SV_GetUserinfo( num, (char *)buffer, bufferSize );
	return;
}
void GameImport_SetUserinfo( int num, const char *buffer ) {
	SV_SetUserinfo( num, (const char *)buffer );
	return;
}
void GameImport_GetServerinfo( char *buffer, int bufferSize ) {

	SV_GetServerinfo( (char *)buffer, bufferSize );
	return;
}
void GameImport_SetBrushModel( void *ent, const char *name ) {
	SV_SetBrushModel( (sharedEntity_t *)ent, (const char *)name );
	return;
}
void GameImport_Trace( void *results, const float *start, const float *mins, const float *maxs, const float *end, int passEntityNum, int contentmask ) {
	SV_Trace( (trace_t *)results, (const vec_t *)start, (const vec_t *)mins, (const vec_t *)maxs, (const vec_t *)end, passEntityNum, contentmask, /*int capsule*/ qfalse );
	return;
}
int GameImport_SetEntityReplication( int number, int priority, float radius ) {
	return (int)SV_SetEntityReplication( number, priority, radius );
}
void *GameImport_AllocLevelMemory( uint32_t bytes ) {
	if ( !bytes || bytes > INT32_MAX )
		Com_Error( ERR_DROP, "Invalid native level allocation" );
	return Hunk_Alloc( (int)bytes, h_high );
}
void GameImport_TraceFiltered( void *results, const float *start, const float *end, int passEntityNum, int contentmask, const uint8_t *ignored ) {
	SV_Trace( (trace_t *)results, start, nullptr, nullptr, end, passEntityNum, contentmask, qfalse, ignored );
}

void GameImport_TraceCapsule( void *results, const float *start, const float *mins, const float *maxs, const float *end, int passEntityNum, int contentmask ) {
	SV_Trace( (trace_t *)results, (const vec_t *)start, (const vec_t *)mins, (const vec_t *)maxs, (const vec_t *)end, passEntityNum, contentmask, /*int capsule*/ qtrue );
	return;
}
int GameImport_PointContents( const float *point, int passEntityNum ) {
	return SV_PointContents( (const vec_t *)point, passEntityNum );
}
int GameImport_InPVS( const float *p1, const float *p2 ) {
	return SV_inPVS( (const vec_t *)p1, (const vec_t *)p2 );
}
int GameImport_InPVSIgnorePortals( const float *p1, const float *p2 ) {
	return SV_inPVSIgnorePortals( (const vec_t *)p1, (const vec_t *)p2 );
}
void GameImport_AdjustAreaPortalState( void *ent, int open ) {
	SV_AdjustAreaPortalState( (sharedEntity_t *)ent, (qboolean)open );
	return;
}
int GameImport_AreasConnected( int area1, int area2 ) {
	return CM_AreasConnected( area1, area2 );
}
void GameImport_LinkEntity( void *ent ) {
	SV_LinkEntity( (sharedEntity_t *)ent );
	return;
}
void GameImport_UnlinkEntity( void *ent ) {
	SV_UnlinkEntity( (sharedEntity_t *)ent );
	return;
}
int GameImport_EntitiesInBox( const float *mins, const float *maxs, int *list, int maxcount ) {

	return SV_AreaEntities( (const vec_t *)mins, (const vec_t *)maxs, (int *)list, maxcount );
}
int GameImport_EntityContact( const float *mins, const float *maxs, const void *ent ) {
	return SV_EntityContact( (const vec_t *)mins, (const vec_t *)maxs, (const sharedEntity_t *)ent, /*int capsule*/ qfalse );
}
int GameImport_EntityContactCapsule( const float *mins, const float *maxs, const void *ent ) {
	return SV_EntityContact( (const vec_t *)mins, (const vec_t *)maxs, (const sharedEntity_t *)ent, /*int capsule*/ qtrue );
}
int GameImport_BotAllocateClient( void ) {
	return SV_BotAllocateClient();
}
void GameImport_BotFreeClient( int clientNum ) {
	SV_BotFreeClient( clientNum );
	return;
}
void GameImport_GetUsercmd( int clientNum, void *cmd ) {
	SV_GetUsercmd( clientNum, (usercmd_t *)cmd );
	return;
}
int GameImport_GetEntityToken( char *buffer, int bufferSize ) {
	{
		char *s = (char *)COM_Parse( &sv.entityParsePoint );

		//Q_strncpyz( buffer, s, bufferSize );
		// we can't use our optimized Q_strncpyz() function
		// because of uninitialized memory bug in defrag mod
		{
			char *dst = (char *)buffer;
			const int size = bufferSize - 1;
			if ( size >= 0 ) {
				Q_strncpy( dst, s, size );
				dst[size] = '\0';
			}
		}
		if ( !sv.entityParsePoint && s[0] == '\0' ) {
			return qfalse;
		} else {
			return qtrue;
		}
	}
}
int GameImport_DebugPolygonCreate( int color, int numPoints, void *points ) {
	return BotImport_DebugPolygonCreate( color, numPoints, (vec_t( * )[3])points );
}
void GameImport_DebugPolygonDelete( int id ) {
	BotImport_DebugPolygonDelete( id );
	return;
}
int GameImport_RealTime( void *qtime ) {
	return Com_RealTime( (qtime_t *)qtime );
}
void GameImport_SnapVector( float *v ) {
	Sys_SnapVector( (float *)v );
	return;

	//====================================
}
int GameImport_BotLibSetup( void ) {
	return SV_BotLibSetup();
}
int GameImport_BotLibShutdown( void ) {
	return SV_BotLibShutdown();
}
int GameImport_BotLibVarSet( char *var_name, char *value ) {
	return botlib_export->BotLibVarSet( (const char *)var_name, (const char *)value );
}
int GameImport_BotLibVarGet( char *var_name, char *value, int size ) {

	return botlib_export->BotLibVarGet( (const char *)var_name, (char *)value, size );
}
int GameImport_BotLibDefine( char *string ) {
	return botlib_export->PC_AddGlobalDefine( (const char *)string );
}
int GameImport_BotLibStartFrame( float time ) {
	return botlib_export->BotLibStartFrame( time );
}
int GameImport_BotLibLoadMap( const char *mapname ) {
	return botlib_export->BotLibLoadMap( (const char *)mapname );
}
int GameImport_BotLibUpdateEntity( int ent, void *bue ) {
	return botlib_export->BotLibUpdateEntity( ent, (bot_entitystate_t *)bue );
}
int GameImport_BotLibTest( int parm0, char *parm1, float *parm2, float *parm3 ) {
	return botlib_export->Test( parm0, (char *)parm1, (vec_t *)parm2, (vec_t *)parm3 );
}
int GameImport_BotGetSnapshotEntity( int clientNum, int sequence ) {
	return SV_BotGetSnapshotEntity( clientNum, sequence );
}
int GameImport_BotGetServerCommand( int clientNum, char *message, int size ) {

	return SV_BotGetConsoleMessage( clientNum, (char *)message, size );
}
void GameImport_BotUserCommand( int clientNum, void *ucmd ) {
	{
		unsigned clientIndex = clientNum;
		if ( clientIndex < (unsigned int)sv.maxclients ) {
			SV_ClientThink( &svs.clients[clientIndex], (usercmd_t *)ucmd );
		}
	}
	return;
}
void GameImport_AAS_EntityInfo( int entnum, void *info ) {
	botlib_export->aas.AAS_EntityInfo( entnum, (struct aas_entityinfo_s *)info );
	return;
}
int GameImport_AAS_Initialized( void ) {
	return botlib_export->aas.AAS_Initialized();
}
void GameImport_AAS_PresenceTypeBoundingBox( int presencetype, float *mins, float *maxs ) {
	botlib_export->aas.AAS_PresenceTypeBoundingBox( presencetype, (vec_t *)mins, (vec_t *)maxs );
	return;
}
float GameImport_AAS_Time( void ) {
	return botlib_export->aas.AAS_Time();
}
int GameImport_AAS_PointAreaNum( float *point ) {
	return botlib_export->aas.AAS_PointAreaNum( (vec_t *)point );
}
int GameImport_AAS_PointReachabilityAreaIndex( float *point ) {
	return botlib_export->aas.AAS_PointReachabilityAreaIndex( (vec_t *)point );
}
int GameImport_AAS_TraceAreas( float *start, float *end, int *areas, void *points, int maxareas ) {
	return botlib_export->aas.AAS_TraceAreas( (vec_t *)start, (vec_t *)end, (int *)areas, (vec_t( * )[3])points, maxareas );
}
int GameImport_AAS_BBoxAreas( float *absmins, float *absmaxs, int *areas, int maxareas ) {
	return botlib_export->aas.AAS_BBoxAreas( (vec_t *)absmins, (vec_t *)absmaxs, (int *)areas, maxareas );
}
int GameImport_AAS_AreaInfo( int areanum, void *info ) {
	return botlib_export->aas.AAS_AreaInfo( areanum, (struct aas_areainfo_s *)info );
}
int GameImport_AAS_PointContents( float *point ) {
	return botlib_export->aas.AAS_PointContents( (vec_t *)point );
}
int GameImport_AAS_NextBSPEntity( int ent ) {
	return botlib_export->aas.AAS_NextBSPEntity( ent );
}
int GameImport_AAS_ValueForBSPEpairKey( int ent, char *key, char *value, int size ) {

	return botlib_export->aas.AAS_ValueForBSPEpairKey( ent, (const char *)key, (char *)value, size );
}
int GameImport_AAS_VectorForBSPEpairKey( int ent, char *key, float *v ) {
	return botlib_export->aas.AAS_VectorForBSPEpairKey( ent, (const char *)key, (vec_t *)v );
}
int GameImport_AAS_FloatForBSPEpairKey( int ent, char *key, float *value ) {
	return botlib_export->aas.AAS_FloatForBSPEpairKey( ent, (const char *)key, (float *)value );
}
int GameImport_AAS_IntForBSPEpairKey( int ent, char *key, int *value ) {
	return botlib_export->aas.AAS_IntForBSPEpairKey( ent, (const char *)key, (int *)value );
}
int GameImport_AAS_AreaReachability( int areanum ) {
	return botlib_export->aas.AAS_AreaReachability( areanum );
}
int GameImport_AAS_AreaTravelTimeToGoalArea( int areanum, float *origin, int goalareanum, int travelflags ) {
	return botlib_export->aas.AAS_AreaTravelTimeToGoalArea( areanum, (vec_t *)origin, goalareanum, travelflags );
}
int GameImport_AAS_EnableRoutingArea( int areanum, int enable ) {
	return botlib_export->aas.AAS_EnableRoutingArea( areanum, enable );
}
int GameImport_AAS_PredictRoute( void *route, int areanum, float *origin, int goalareanum, int travelflags, int maxareas, int maxtime, int stopevent, int stopcontents, int stoptfl, int stopareanum ) {
	return botlib_export->aas.AAS_PredictRoute( (struct aas_predictroute_s *)route, areanum, (vec_t *)origin, goalareanum, travelflags, maxareas, maxtime, stopevent, stopcontents, stoptfl, stopareanum );
}
int GameImport_AAS_AlternativeRouteGoals( float *start, int startareanum, float *goal, int goalareanum, int travelflags, void *altroutegoals, int maxaltroutegoals, int type ) {
	return botlib_export->aas.AAS_AlternativeRouteGoals( (vec_t *)start, startareanum, (vec_t *)goal, goalareanum, travelflags, (struct aas_altroutegoal_s *)altroutegoals, maxaltroutegoals, type );
}
int GameImport_AAS_Swimming( float *origin ) {
	return botlib_export->aas.AAS_Swimming( (vec_t *)origin );
}
int GameImport_AAS_PredictClientMovement( void *move, int entnum, float *origin, int presencetype, int onground, float *velocity, float *cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize ) {
	return botlib_export->aas.AAS_PredictClientMovement( (struct aas_clientmove_s *)move, entnum, (const vec_t *)origin, presencetype, onground,
		(const vec_t *)velocity, (const vec_t *)cmdmove, cmdframes, maxframes, frametime, stopevent, stopareanum, visualize );
}
void GameImport_EA_Say( int client, char *str ) {
	botlib_export->ea.EA_Say( client, (const char *)str );
	return;
}
void GameImport_EA_SayTeam( int client, char *str ) {
	botlib_export->ea.EA_SayTeam( client, (const char *)str );
	return;
}
void GameImport_EA_Command( int client, char *command ) {
	botlib_export->ea.EA_Command( client, (const char *)command );
	return;
}
void GameImport_EA_Action( int client, int action ) {
	botlib_export->ea.EA_Action( client, action );
	return;
}
void GameImport_EA_Gesture( int client ) {
	botlib_export->ea.EA_Gesture( client );
	return;
}
void GameImport_EA_Talk( int client ) {
	botlib_export->ea.EA_Talk( client );
	return;
}
void GameImport_EA_Attack( int client ) {
	botlib_export->ea.EA_Attack( client );
	return;
}
void GameImport_EA_Use( int client ) {
	botlib_export->ea.EA_Use( client );
	return;
}
void GameImport_EA_Respawn( int client ) {
	botlib_export->ea.EA_Respawn( client );
	return;
}
void GameImport_EA_Crouch( int client ) {
	botlib_export->ea.EA_Crouch( client );
	return;
}
void GameImport_EA_MoveUp( int client ) {
	botlib_export->ea.EA_MoveUp( client );
	return;
}
void GameImport_EA_MoveDown( int client ) {
	botlib_export->ea.EA_MoveDown( client );
	return;
}
void GameImport_EA_MoveForward( int client ) {
	botlib_export->ea.EA_MoveForward( client );
	return;
}
void GameImport_EA_MoveBack( int client ) {
	botlib_export->ea.EA_MoveBack( client );
	return;
}
void GameImport_EA_MoveLeft( int client ) {
	botlib_export->ea.EA_MoveLeft( client );
	return;
}
void GameImport_EA_MoveRight( int client ) {
	botlib_export->ea.EA_MoveRight( client );
	return;
}
void GameImport_EA_SelectWeapon( int client, int weapon ) {
	botlib_export->ea.EA_SelectWeapon( client, weapon );
	return;
}
void GameImport_EA_Jump( int client ) {
	botlib_export->ea.EA_Jump( client );
	return;
}
void GameImport_EA_DelayedJump( int client ) {
	botlib_export->ea.EA_DelayedJump( client );
	return;
}
void GameImport_EA_Move( int client, float *dir, float speed ) {
	botlib_export->ea.EA_Move( client, (vec_t *)dir, speed );
	return;
}
void GameImport_EA_View( int client, float *viewangles ) {
	botlib_export->ea.EA_View( client, (vec_t *)viewangles );
	return;
}
void GameImport_EA_EndRegular( int client, float thinktime ) {
	botlib_export->ea.EA_EndRegular( client, thinktime );
	return;
}
void GameImport_EA_GetInput( int client, float thinktime, void *input ) {
	botlib_export->ea.EA_GetInput( client, thinktime, (bot_input_t *)input );
	return;
}
void GameImport_EA_ResetInput( int client ) {
	botlib_export->ea.EA_ResetInput( client );
	return;
}
int GameImport_BotLoadCharacter( char *charfile, float skill ) {
	return botlib_export->ai.BotLoadCharacter( (const char *)charfile, skill );
}
void GameImport_BotFreeCharacter( int character ) {
	botlib_export->ai.BotFreeCharacter( character );
	return;
}
float GameImport_Characteristic_Float( int character, int index ) {
	return botlib_export->ai.Characteristic_Float( character, index );
}
float GameImport_Characteristic_BFloat( int character, int index, float min, float max ) {
	return botlib_export->ai.Characteristic_BFloat( character, index, min, max );
}
int GameImport_Characteristic_Integer( int character, int index ) {
	return botlib_export->ai.Characteristic_Integer( character, index );
}
int GameImport_Characteristic_BInteger( int character, int index, int min, int max ) {
	return botlib_export->ai.Characteristic_BInteger( character, index, min, max );
}
void GameImport_Characteristic_String( int character, int index, char *buf, int size ) {

	botlib_export->ai.Characteristic_String( character, index, (char *)buf, size );
	return;
}
int GameImport_BotAllocChatState( void ) {
	return botlib_export->ai.BotAllocChatState();
}
void GameImport_BotFreeChatState( int handle ) {
	botlib_export->ai.BotFreeChatState( handle );
	return;
}
void GameImport_BotQueueConsoleMessage( int chatstate, int type, char *message ) {
	botlib_export->ai.BotQueueConsoleMessage( chatstate, type, (const char *)message );
	return;
}
void GameImport_BotRemoveConsoleMessage( int chatstate, int handle ) {
	botlib_export->ai.BotRemoveConsoleMessage( chatstate, handle );
	return;
}
int GameImport_BotNextConsoleMessage( int chatstate, void *cm ) {
	return botlib_export->ai.BotNextConsoleMessage( chatstate, (struct bot_consolemessage_qvm_s *)cm );
}
int GameImport_BotNumConsoleMessages( int chatstate ) {
	return botlib_export->ai.BotNumConsoleMessages( chatstate );
}
void GameImport_BotInitialChat( int chatstate, char *type, int mcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 ) {
	botlib_export->ai.BotInitialChat( chatstate, (const char *)type, mcontext, (const char *)var0, (const char *)var1, (const char *)var2, (const char *)var3, (const char *)var4, (const char *)var5, (const char *)var6, (const char *)var7 );
	return;
}
int GameImport_BotNumInitialChats( int chatstate, char *type ) {
	return botlib_export->ai.BotNumInitialChats( chatstate, (const char *)type );
}
int GameImport_BotReplyChat( int chatstate, char *message, int mcontext, int vcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 ) {
	return botlib_export->ai.BotReplyChat( chatstate, (const char *)message, mcontext, vcontext, (const char *)var0, (const char *)var1, (const char *)var2, (const char *)var3, (const char *)var4, (const char *)var5, (const char *)var6, (const char *)var7 );
}
int GameImport_BotChatLength( int chatstate ) {
	return botlib_export->ai.BotChatLength( chatstate );
}
void GameImport_BotEnterChat( int chatstate, int client, int sendto ) {
	botlib_export->ai.BotEnterChat( chatstate, client, sendto );
	return;
}
void GameImport_BotGetChatMessage( int chatstate, char *buf, int size ) {

	botlib_export->ai.BotGetChatMessage( chatstate, (char *)buf, size );
	return;
}
int GameImport_StringContains( char *str1, char *str2, int casesensitive ) {
	return botlib_export->ai.StringContains( (const char *)str1, (const char *)str2, casesensitive );
}
int GameImport_BotFindMatch( char *str, void *match, uint64_t context ) {
	return botlib_export->ai.BotFindMatch( (const char *)str, (struct bot_match_s *)match, context );
}
void GameImport_BotMatchVariable( void *match, int variable, char *buf, int size ) {

	botlib_export->ai.BotMatchVariable( (struct bot_match_s *)match, variable, (char *)buf, size );
	return;
}
void GameImport_UnifyWhiteSpaces( char *string ) {
	botlib_export->ai.UnifyWhiteSpaces( (char *)string );
	return;
}
void GameImport_BotReplaceSynonyms( char *string, uint64_t context ) {
	botlib_export->ai.BotReplaceSynonyms( (char *)string, MAX_STRING_CHARS, context );
	return;
}
int GameImport_BotLoadChatFile( int chatstate, char *chatfile, char *chatname ) {
	return botlib_export->ai.BotLoadChatFile( chatstate, (const char *)chatfile, (const char *)chatname );
}
void GameImport_BotSetChatGender( int chatstate, int gender ) {
	botlib_export->ai.BotSetChatGender( chatstate, gender );
	return;
}
void GameImport_BotSetChatName( int chatstate, char *name, int client ) {
	botlib_export->ai.BotSetChatName( chatstate, (const char *)name, client );
	return;
}
void GameImport_BotResetGoalState( int goalstate ) {
	botlib_export->ai.BotResetGoalState( goalstate );
	return;
}
void GameImport_BotResetAvoidGoals( int goalstate ) {
	botlib_export->ai.BotResetAvoidGoals( goalstate );
	return;
}
void GameImport_BotRemoveFromAvoidGoals( int goalstate, int number ) {
	botlib_export->ai.BotRemoveFromAvoidGoals( goalstate, number );
	return;
}
void GameImport_BotPushGoal( int goalstate, void *goal ) {
	botlib_export->ai.BotPushGoal( goalstate, (struct bot_goal_s *)goal );
	return;
}
void GameImport_BotPopGoal( int goalstate ) {
	botlib_export->ai.BotPopGoal( goalstate );
	return;
}
void GameImport_BotEmptyGoalStack( int goalstate ) {
	botlib_export->ai.BotEmptyGoalStack( goalstate );
	return;
}
void GameImport_BotDumpAvoidGoals( int goalstate ) {
	botlib_export->ai.BotDumpAvoidGoals( goalstate );
	return;
}
void GameImport_BotDumpGoalStack( int goalstate ) {
	botlib_export->ai.BotDumpGoalStack( goalstate );
	return;
}
void GameImport_BotGoalName( int number, char *name, int size ) {

	botlib_export->ai.BotGoalName( number, (char *)name, size );
	return;
}
int GameImport_BotGetTopGoal( int goalstate, void *goal ) {
	return botlib_export->ai.BotGetTopGoal( goalstate, (struct bot_goal_s *)goal );
}
int GameImport_BotGetSecondGoal( int goalstate, void *goal ) {
	return botlib_export->ai.BotGetSecondGoal( goalstate, (struct bot_goal_s *)goal );
}
int GameImport_BotChooseLTGItem( int goalstate, float *origin, int *inventory, int travelflags ) {
	return botlib_export->ai.BotChooseLTGItem( goalstate, (vec_t *)origin, (int *)inventory, travelflags );
}
int GameImport_BotChooseNBGItem( int goalstate, float *origin, int *inventory, int travelflags, void *ltg, float maxtime ) {
	return botlib_export->ai.BotChooseNBGItem( goalstate, (vec_t *)origin, (int *)inventory, travelflags, (struct bot_goal_s *)ltg, maxtime );
}
int GameImport_BotTouchingGoal( float *origin, void *goal ) {
	return botlib_export->ai.BotTouchingGoal( (const vec_t *)origin, (const struct bot_goal_s *)goal );
}
int GameImport_BotItemGoalInVisButNotVisible( int viewer, float *eye, float *viewangles, void *goal ) {
	return botlib_export->ai.BotItemGoalInVisButNotVisible( viewer, (vec_t *)eye, (vec_t *)viewangles, (struct bot_goal_s *)goal );
}
int GameImport_BotGetLevelItemGoal( int index, char *classname, void *goal ) {
	return botlib_export->ai.BotGetLevelItemGoal( index, (const char *)classname, (struct bot_goal_s *)goal );
}
int GameImport_BotGetNextCampSpotGoal( int num, void *goal ) {
	return botlib_export->ai.BotGetNextCampSpotGoal( num, (struct bot_goal_s *)goal );
}
int GameImport_BotGetMapLocationGoal( char *name, void *goal ) {
	return botlib_export->ai.BotGetMapLocationGoal( (const char *)name, (struct bot_goal_s *)goal );
}
float GameImport_BotAvoidGoalTime( int goalstate, int number ) {
	return botlib_export->ai.BotAvoidGoalTime( goalstate, number );
}
void GameImport_BotSetAvoidGoalTime( int goalstate, int number, float avoidtime ) {
	botlib_export->ai.BotSetAvoidGoalTime( goalstate, number, avoidtime );
	return;
}
void GameImport_BotInitLevelItems( void ) {
	botlib_export->ai.BotInitLevelItems();
	return;
}
void GameImport_BotUpdateEntityItems( void ) {
	botlib_export->ai.BotUpdateEntityItems();
	return;
}
int GameImport_BotLoadItemWeights( int goalstate, char *filename ) {
	return botlib_export->ai.BotLoadItemWeights( goalstate, (const char *)filename );
}
void GameImport_BotFreeItemWeights( int goalstate ) {
	botlib_export->ai.BotFreeItemWeights( goalstate );
	return;
}
void GameImport_BotInterbreedGoalFuzzyLogic( int parent1, int parent2, int child ) {
	botlib_export->ai.BotInterbreedGoalFuzzyLogic( parent1, parent2, child );
	return;
}
void GameImport_BotSaveGoalFuzzyLogic( int goalstate, char *filename ) {
	botlib_export->ai.BotSaveGoalFuzzyLogic( goalstate, (const char *)filename );
	return;
}
void GameImport_BotMutateGoalFuzzyLogic( int goalstate, float range ) {
	botlib_export->ai.BotMutateGoalFuzzyLogic( goalstate, range );
	return;
}
int GameImport_BotAllocGoalState( int state ) {
	return botlib_export->ai.BotAllocGoalState( state );
}
void GameImport_BotFreeGoalState( int handle ) {
	botlib_export->ai.BotFreeGoalState( handle );
	return;
}
void GameImport_BotResetMoveState( int movestate ) {
	botlib_export->ai.BotResetMoveState( movestate );
	return;
}
void GameImport_BotAddAvoidSpot( int movestate, float *origin, float radius, int type ) {
	botlib_export->ai.BotAddAvoidSpot( movestate, (const vec_t *)origin, radius, type );
	return;
}
void GameImport_BotMoveToGoal( void *result, int movestate, void *goal, int travelflags ) {
	botlib_export->ai.BotMoveToGoal( (struct bot_moveresult_s *)result, movestate, (struct bot_goal_s *)goal, travelflags );
	return;
}
int GameImport_BotMoveInDirection( int movestate, float *dir, float speed, int type ) {
	return botlib_export->ai.BotMoveInDirection( movestate, (vec_t *)dir, speed, type );
}
void GameImport_BotResetAvoidReach( int movestate ) {
	botlib_export->ai.BotResetAvoidReach( movestate );
	return;
}
void GameImport_BotResetLastAvoidReach( int movestate ) {
	botlib_export->ai.BotResetLastAvoidReach( movestate );
	return;
}
int GameImport_BotReachabilityArea( float *origin, int testground ) {
	return botlib_export->ai.BotReachabilityArea( (vec_t *)origin, testground );
}
int GameImport_BotMovementViewTarget( int movestate, void *goal, int travelflags, float lookahead, float *target ) {
	return botlib_export->ai.BotMovementViewTarget( movestate, (struct bot_goal_s *)goal, travelflags, lookahead, (vec_t *)target );
}
int GameImport_BotPredictVisiblePosition( float *origin, int areanum, void *goal, int travelflags, float *target ) {
	return botlib_export->ai.BotPredictVisiblePosition( (vec_t *)origin, areanum, (struct bot_goal_s *)goal, travelflags, (vec_t *)target );
}
int GameImport_BotAllocMoveState( void ) {
	return botlib_export->ai.BotAllocMoveState();
}
void GameImport_BotFreeMoveState( int handle ) {
	botlib_export->ai.BotFreeMoveState( handle );
	return;
}
void GameImport_BotInitMoveState( int handle, void *initmove ) {
	botlib_export->ai.BotInitMoveState( handle, (struct bot_initmove_s *)initmove );
	return;
}
int GameImport_BotChooseBestFightWeapon( int weaponstate, int *inventory ) {
	return botlib_export->ai.BotChooseBestFightWeapon( weaponstate, (int *)inventory );
}
void GameImport_BotGetWeaponInfo( int weaponstate, int weapon, void *weaponinfo ) {
	botlib_export->ai.BotGetWeaponInfo( weaponstate, weapon, (struct weaponinfo_s *)weaponinfo );
	return;
}
int GameImport_BotLoadWeaponWeights( int weaponstate, char *filename ) {
	return botlib_export->ai.BotLoadWeaponWeights( weaponstate, (const char *)filename );
}
int GameImport_BotAllocWeaponState( void ) {
	return botlib_export->ai.BotAllocWeaponState();
}
void GameImport_BotFreeWeaponState( int weaponstate ) {
	botlib_export->ai.BotFreeWeaponState( weaponstate );
	return;
}
void GameImport_BotResetWeaponState( int weaponstate ) {
	botlib_export->ai.BotResetWeaponState( weaponstate );
	return;
}
int GameImport_GeneticParentsAndChildSelection( int numranks, float *ranks, int *parent1, int *parent2, int *child ) {
	return botlib_export->ai.GeneticParentsAndChildSelection( numranks, (float *)ranks, (int *)parent1, (int *)parent2, (int *)child );

	// shared syscalls
}
int GameImport_PC_LoadSource( const char *filename ) {
	return botlib_export->PC_LoadSourceHandle( (const char *)filename );
}
int GameImport_PC_FreeSource( int handle ) {
	return botlib_export->PC_FreeSourceHandle( handle );
}
int GameImport_PC_ReadToken( int handle, void *pc_token ) {

	return botlib_export->PC_ReadTokenHandle( handle, (pc_token_t *)pc_token );
}
int GameImport_PC_SourceFileAndLine( int handle, char *filename, int *line ) {
	return botlib_export->PC_SourceFileAndLine( handle, (char *)filename, (int *)line );
}


static bool gameRunning;
bool SV_GameRunning( void ) {
	return gameRunning;
}

/*
===============
SV_ShutdownGameProgs

Called every time a map changes
===============
*/
void SV_ShutdownGameProgs( void ) {
#ifdef AFTERSHOCK_DEVTOOLS
	Dev_RegisterGameTools( nullptr );
#endif
	if ( !gameRunning ) {
		return;
	}
	Game_Shutdown( qfalse );
	gameRunning = false;
	FS_VM_CloseFiles( H_QAGAME );
}


/*
==================
SV_InitNativeGame

Called for both a full init and a restart
==================
*/
static void SV_InitNativeGame( qboolean restart ) {
	int i;

	// start the entity parsing at the beginning
	sv.entityParsePoint = CM_EntityString();
#ifdef AFTERSHOCK_DEVTOOLS
	void *developerEntities = nullptr;
	if ( Cvar_VariableIntegerValue( "dev_loadEntities" ) ) {
		Cvar_Set( "dev_loadEntities", "0" );
		if ( !Cvar_VariableIntegerValue( "sv_cheats" ) )
			Com_Error( ERR_DROP, "Developer entity reload requires devmap" );
		const int length = FS_ReadFile( Cvar_VariableString( "dev_entityFile" ), &developerEntities );
		if ( length <= 0 || length > 8 * 1024 * 1024 ) {
			if ( developerEntities )
				FS_FreeFile( developerEntities );
			Com_Error( ERR_DROP, "Developer entity file missing, empty or too large" );
		}
		sv.entityParsePoint = (const char *)developerEntities;
	}
#endif

	// clear all gentity pointers that might still be set from
	// a previous level
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=522
	// now done before GAME_INIT call
	for ( i = 0; i < sv.maxclients; i++ ) {
		svs.clients[i].gentity = NULL;
	}

	// use the current msec count for a random seed
	// init for this gamestate
	Game_Init( sv.time, Com_Milliseconds(), restart );
#ifdef AFTERSHOCK_DEVTOOLS
	if ( developerEntities ) {
		FS_FreeFile( developerEntities );
		sv.entityParsePoint = nullptr;
	}
#endif
}


/*
===================
SV_RestartGameProgs

Called on a map_restart, but not on a normal map change
===================
*/
void SV_RestartGameProgs( void ) {
	if ( !gameRunning ) {
		return;
	}
	Game_Shutdown( qtrue );

	Hunk_AllocPreference( h_high );


	SV_InitNativeGame( qtrue );

	// load userinfo filters
	SV_LoadFilters( sv_filter->string );
}


/*
===============
SV_InitGameProgs

Called on a normal map change, not on a map_restart
===============
*/
void SV_InitGameProgs( void ) {
	cvar_t *var;
	//FIXME these are temp while I make bots run in vm
	extern int bot_enable;

	var = Cvar_Get( "bot_enable", "1", CVAR_LATCH );
	if ( var ) {
		bot_enable = var->integer;
	} else {
		bot_enable = 0;
	}

	gameRunning = true;
	Com_Printf( "Static game loaded.\n" );

	SV_InitNativeGame( qfalse );

	// load userinfo filters
	SV_LoadFilters( sv_filter->string );
}


/*
====================
SV_GameCommand

See if the current console command is claimed by the game
====================
*/
qboolean SV_GameCommand( void ) {
	if ( sv.state != SS_GAME ) {
		return qfalse;
	}

	return (qboolean)Game_ConsoleCommand();
}

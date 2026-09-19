/* Native service adapters; preserve the imported game ABI. */
#include "g_local.h"
void trap_Printf( const char *fmt ) {
	GameImport_Printf( fmt );
}
void trap_Error( const char *fmt ) {
	GameImport_Error( fmt );
}
int trap_Milliseconds( void ) {
	return GameImport_Milliseconds();
}
int trap_Argc( void ) {
	return GameImport_Argc();
}
void trap_Argv( int n, char *buffer, int bufferLength ) {
	GameImport_Argv( n, buffer, bufferLength );
}
int trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	return GameImport_FS_FOpenFile( qpath, f, mode );
}
void trap_FS_Read( void *buffer, int len, fileHandle_t f ) {
	GameImport_FS_Read( buffer, len, f );
}
void trap_FS_Write( const void *buffer, int len, fileHandle_t f ) {
	GameImport_FS_Write( buffer, len, f );
}
void trap_FS_FCloseFile( fileHandle_t f ) {
	GameImport_FS_FCloseFile( f );
}
int trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize ) {
	return GameImport_FS_GetFileList( path, extension, listbuf, bufsize );
}
int trap_FS_Seek( fileHandle_t f, int64_t offset, int origin ) {
	return GameImport_FS_Seek( f, offset, origin );
}
void trap_SendConsoleCommand( int exec_when, const char *text ) {
	GameImport_SendConsoleCommand( exec_when, text );
}
void trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags ) {
	GameImport_Cvar_Register( cvar, var_name, value, flags );
}
void trap_Cvar_Update( vmCvar_t *cvar ) {
	GameImport_Cvar_Update( cvar );
}
void trap_Cvar_Set( const char *var_name, const char *value ) {
	GameImport_Cvar_Set( var_name, value );
}
int trap_Cvar_VariableIntegerValue( const char *var_name ) {
	return GameImport_Cvar_VariableIntegerValue( var_name );
}
void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {
	GameImport_Cvar_VariableStringBuffer( var_name, buffer, bufsize );
}
void trap_LocateGameData( gentity_t *gEnts, int numGEntities, int sizeofGEntity_t,
	playerState_t *clients, int sizeofGClient ) {
	GameImport_LocateGameData( gEnts, numGEntities, sizeofGEntity_t, clients, sizeofGClient );
}
void trap_DropClient( int clientNum, const char *reason ) {
	GameImport_DropClient( clientNum, reason );
}
void trap_SendServerCommand( int clientNum, const char *text ) {
	GameImport_SendServerCommand( clientNum, text );
}
void trap_SetConfigstring( int num, const char *string ) {
	GameImport_SetConfigstring( num, string );
}
void trap_GetConfigstring( int num, char *buffer, int bufferSize ) {
	GameImport_GetConfigstring( num, buffer, bufferSize );
}
void trap_GetUserinfo( int num, char *buffer, int bufferSize ) {
	GameImport_GetUserinfo( num, buffer, bufferSize );
}
void trap_SetUserinfo( int num, const char *buffer ) {
	GameImport_SetUserinfo( num, buffer );
}
void trap_GetServerinfo( char *buffer, int bufferSize ) {
	GameImport_GetServerinfo( buffer, bufferSize );
}
void trap_SetBrushModel( gentity_t *ent, const char *name ) {
	GameImport_SetBrushModel( ent, name );
}
void trap_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask ) {
	GameImport_Trace( results, start, mins, maxs, end, passEntityNum, contentmask );
}
void trap_TraceCapsule( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask ) {
	GameImport_TraceCapsule( results, start, mins, maxs, end, passEntityNum, contentmask );
}
int trap_PointContents( const vec3_t point, int passEntityNum ) {
	return GameImport_PointContents( point, passEntityNum );
}
qboolean trap_InPVS( const vec3_t p1, const vec3_t p2 ) {
	return (qboolean)GameImport_InPVS( p1, p2 );
}
qboolean trap_InPVSIgnorePortals( const vec3_t p1, const vec3_t p2 ) {
	return (qboolean)GameImport_InPVSIgnorePortals( p1, p2 );
}
void trap_AdjustAreaPortalState( gentity_t *ent, qboolean open ) {
	GameImport_AdjustAreaPortalState( ent, open );
}
qboolean trap_AreasConnected( int area1, int area2 ) {
	return (qboolean)GameImport_AreasConnected( area1, area2 );
}
void trap_LinkEntity( gentity_t *ent ) {
	GameImport_LinkEntity( ent );
}
void trap_UnlinkEntity( gentity_t *ent ) {
	GameImport_UnlinkEntity( ent );
}
int trap_EntitiesInBox( const vec3_t mins, const vec3_t maxs, int *list, int maxcount ) {
	return GameImport_EntitiesInBox( mins, maxs, list, maxcount );
}
qboolean trap_EntityContact( const vec3_t mins, const vec3_t maxs, const gentity_t *ent ) {
	return (qboolean)GameImport_EntityContact( mins, maxs, ent );
}
qboolean trap_EntityContactCapsule( const vec3_t mins, const vec3_t maxs, const gentity_t *ent ) {
	return (qboolean)GameImport_EntityContactCapsule( mins, maxs, ent );
}
int trap_BotAllocateClient( void ) {
	return GameImport_BotAllocateClient();
}
void trap_BotFreeClient( int clientNum ) {
	GameImport_BotFreeClient( clientNum );
}
void trap_GetUsercmd( int clientNum, usercmd_t *cmd ) {
	GameImport_GetUsercmd( clientNum, cmd );
}
qboolean trap_GetEntityToken( char *buffer, int bufferSize ) {
	return (qboolean)GameImport_GetEntityToken( buffer, bufferSize );
}
int trap_DebugPolygonCreate( int color, int numPoints, vec3_t *points ) {
	return GameImport_DebugPolygonCreate( color, numPoints, points );
}
void trap_DebugPolygonDelete( int id ) {
	GameImport_DebugPolygonDelete( id );
}
int trap_RealTime( qtime_t *qtime ) {
	return GameImport_RealTime( qtime );
}
void trap_SnapVector( float *v ) {
	GameImport_SnapVector( v );
}
int trap_BotLibSetup( void ) {
	return GameImport_BotLibSetup();
}
int trap_BotLibShutdown( void ) {
	return GameImport_BotLibShutdown();
}
int trap_BotLibVarSet( char *var_name, char *value ) {
	return GameImport_BotLibVarSet( var_name, value );
}
int trap_BotLibVarGet( char *var_name, char *value, int size ) {
	return GameImport_BotLibVarGet( var_name, value, size );
}
int trap_BotLibDefine( char *string ) {
	return GameImport_BotLibDefine( string );
}
int trap_BotLibStartFrame( float time ) {
	return GameImport_BotLibStartFrame( time );
}
int trap_BotLibLoadMap( const char *mapname ) {
	return GameImport_BotLibLoadMap( mapname );
}
int trap_BotLibUpdateEntity( int ent, void *bue ) {
	return GameImport_BotLibUpdateEntity( ent, bue );
}
int trap_BotLibTest( int parm0, char *parm1, vec3_t parm2, vec3_t parm3 ) {
	return GameImport_BotLibTest( parm0, parm1, parm2, parm3 );
}
int trap_BotGetSnapshotEntity( int clientNum, int sequence ) {
	return GameImport_BotGetSnapshotEntity( clientNum, sequence );
}
int trap_BotGetServerCommand( int clientNum, char *message, int size ) {
	return GameImport_BotGetServerCommand( clientNum, message, size );
}
void trap_BotUserCommand( int clientNum, usercmd_t *ucmd ) {
	GameImport_BotUserCommand( clientNum, ucmd );
}
void trap_AAS_EntityInfo( int entnum, void *info ) {
	GameImport_AAS_EntityInfo( entnum, info );
}
int trap_AAS_Initialized( void ) {
	return GameImport_AAS_Initialized();
}
void trap_AAS_PresenceTypeBoundingBox( int presencetype, vec3_t mins, vec3_t maxs ) {
	GameImport_AAS_PresenceTypeBoundingBox( presencetype, mins, maxs );
}
float trap_AAS_Time( void ) {
	return GameImport_AAS_Time();
}
int trap_AAS_PointAreaNum( vec3_t point ) {
	return GameImport_AAS_PointAreaNum( point );
}
int trap_AAS_PointReachabilityAreaIndex( vec3_t point ) {
	return GameImport_AAS_PointReachabilityAreaIndex( point );
}
int trap_AAS_TraceAreas( vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas ) {
	return GameImport_AAS_TraceAreas( start, end, areas, points, maxareas );
}
int trap_AAS_BBoxAreas( vec3_t absmins, vec3_t absmaxs, int *areas, int maxareas ) {
	return GameImport_AAS_BBoxAreas( absmins, absmaxs, areas, maxareas );
}
int trap_AAS_AreaInfo( int areanum, void *info ) {
	return GameImport_AAS_AreaInfo( areanum, info );
}
int trap_AAS_PointContents( vec3_t point ) {
	return GameImport_AAS_PointContents( point );
}
int trap_AAS_NextBSPEntity( int ent ) {
	return GameImport_AAS_NextBSPEntity( ent );
}
int trap_AAS_ValueForBSPEpairKey( int ent, char *key, char *value, int size ) {
	return GameImport_AAS_ValueForBSPEpairKey( ent, key, value, size );
}
int trap_AAS_VectorForBSPEpairKey( int ent, char *key, vec3_t v ) {
	return GameImport_AAS_VectorForBSPEpairKey( ent, key, v );
}
int trap_AAS_FloatForBSPEpairKey( int ent, char *key, float *value ) {
	return GameImport_AAS_FloatForBSPEpairKey( ent, key, value );
}
int trap_AAS_IntForBSPEpairKey( int ent, char *key, int *value ) {
	return GameImport_AAS_IntForBSPEpairKey( ent, key, value );
}
int trap_AAS_AreaReachability( int areanum ) {
	return GameImport_AAS_AreaReachability( areanum );
}
int trap_AAS_AreaTravelTimeToGoalArea( int areanum, vec3_t origin, int goalareanum, int travelflags ) {
	return GameImport_AAS_AreaTravelTimeToGoalArea( areanum, origin, goalareanum, travelflags );
}
int trap_AAS_EnableRoutingArea( int areanum, int enable ) {
	return GameImport_AAS_EnableRoutingArea( areanum, enable );
}
int trap_AAS_PredictRoute( void *route, int areanum, vec3_t origin,
	int goalareanum, int travelflags, int maxareas, int maxtime,
	int stopevent, int stopcontents, int stoptfl, int stopareanum ) {
	return GameImport_AAS_PredictRoute( route, areanum, origin, goalareanum, travelflags, maxareas, maxtime, stopevent, stopcontents, stoptfl, stopareanum );
}
int trap_AAS_AlternativeRouteGoals( vec3_t start, int startareanum, vec3_t goal, int goalareanum, int travelflags,
	void *altroutegoals, int maxaltroutegoals,
	int type ) {
	return GameImport_AAS_AlternativeRouteGoals( start, startareanum, goal, goalareanum, travelflags, altroutegoals, maxaltroutegoals, type );
}
int trap_AAS_Swimming( vec3_t origin ) {
	return GameImport_AAS_Swimming( origin );
}
int trap_AAS_PredictClientMovement( void *move, int entnum, vec3_t origin, int presencetype, int onground, vec3_t velocity, vec3_t cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize ) {
	return GameImport_AAS_PredictClientMovement( move, entnum, origin, presencetype, onground, velocity, cmdmove, cmdframes, maxframes, frametime, stopevent, stopareanum, visualize );
}
void trap_EA_Say( int client, char *str ) {
	GameImport_EA_Say( client, str );
}
void trap_EA_SayTeam( int client, char *str ) {
	GameImport_EA_SayTeam( client, str );
}
void trap_EA_Command( int client, char *command ) {
	GameImport_EA_Command( client, command );
}
void trap_EA_Action( int client, int action ) {
	GameImport_EA_Action( client, action );
}
void trap_EA_Gesture( int client ) {
	GameImport_EA_Gesture( client );
}
void trap_EA_Talk( int client ) {
	GameImport_EA_Talk( client );
}
void trap_EA_Attack( int client ) {
	GameImport_EA_Attack( client );
}
void trap_EA_Use( int client ) {
	GameImport_EA_Use( client );
}
void trap_EA_Respawn( int client ) {
	GameImport_EA_Respawn( client );
}
void trap_EA_Crouch( int client ) {
	GameImport_EA_Crouch( client );
}
void trap_EA_MoveUp( int client ) {
	GameImport_EA_MoveUp( client );
}
void trap_EA_MoveDown( int client ) {
	GameImport_EA_MoveDown( client );
}
void trap_EA_MoveForward( int client ) {
	GameImport_EA_MoveForward( client );
}
void trap_EA_MoveBack( int client ) {
	GameImport_EA_MoveBack( client );
}
void trap_EA_MoveLeft( int client ) {
	GameImport_EA_MoveLeft( client );
}
void trap_EA_MoveRight( int client ) {
	GameImport_EA_MoveRight( client );
}
void trap_EA_SelectWeapon( int client, int weapon ) {
	GameImport_EA_SelectWeapon( client, weapon );
}
void trap_EA_Jump( int client ) {
	GameImport_EA_Jump( client );
}
void trap_EA_DelayedJump( int client ) {
	GameImport_EA_DelayedJump( client );
}
void trap_EA_Move( int client, vec3_t dir, float speed ) {
	GameImport_EA_Move( client, dir, speed );
}
void trap_EA_View( int client, vec3_t viewangles ) {
	GameImport_EA_View( client, viewangles );
}
void trap_EA_EndRegular( int client, float thinktime ) {
	GameImport_EA_EndRegular( client, thinktime );
}
void trap_EA_GetInput( int client, float thinktime, void *input ) {
	GameImport_EA_GetInput( client, thinktime, input );
}
void trap_EA_ResetInput( int client ) {
	GameImport_EA_ResetInput( client );
}
int trap_BotLoadCharacter( char *charfile, float skill ) {
	return GameImport_BotLoadCharacter( charfile, skill );
}
void trap_BotFreeCharacter( int character ) {
	GameImport_BotFreeCharacter( character );
}
float trap_Characteristic_Float( int character, int index ) {
	return GameImport_Characteristic_Float( character, index );
}
float trap_Characteristic_BFloat( int character, int index, float min, float max ) {
	return GameImport_Characteristic_BFloat( character, index, min, max );
}
int trap_Characteristic_Integer( int character, int index ) {
	return GameImport_Characteristic_Integer( character, index );
}
int trap_Characteristic_BInteger( int character, int index, int min, int max ) {
	return GameImport_Characteristic_BInteger( character, index, min, max );
}
void trap_Characteristic_String( int character, int index, char *buf, int size ) {
	GameImport_Characteristic_String( character, index, buf, size );
}
int trap_BotAllocChatState( void ) {
	return GameImport_BotAllocChatState();
}
void trap_BotFreeChatState( int handle ) {
	GameImport_BotFreeChatState( handle );
}
void trap_BotQueueConsoleMessage( int chatstate, int type, char *message ) {
	GameImport_BotQueueConsoleMessage( chatstate, type, message );
}
void trap_BotRemoveConsoleMessage( int chatstate, int handle ) {
	GameImport_BotRemoveConsoleMessage( chatstate, handle );
}
int trap_BotNextConsoleMessage( int chatstate, void *cm ) {
	return GameImport_BotNextConsoleMessage( chatstate, cm );
}
int trap_BotNumConsoleMessages( int chatstate ) {
	return GameImport_BotNumConsoleMessages( chatstate );
}
void trap_BotInitialChat( int chatstate, char *type, int mcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 ) {
	GameImport_BotInitialChat( chatstate, type, mcontext, var0, var1, var2, var3, var4, var5, var6, var7 );
}
int trap_BotNumInitialChats( int chatstate, char *type ) {
	return GameImport_BotNumInitialChats( chatstate, type );
}
int trap_BotReplyChat( int chatstate, char *message, int mcontext, int vcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 ) {
	return GameImport_BotReplyChat( chatstate, message, mcontext, vcontext, var0, var1, var2, var3, var4, var5, var6, var7 );
}
int trap_BotChatLength( int chatstate ) {
	return GameImport_BotChatLength( chatstate );
}
void trap_BotEnterChat( int chatstate, int client, int sendto ) {
	GameImport_BotEnterChat( chatstate, client, sendto );
}
void trap_BotGetChatMessage( int chatstate, char *buf, int size ) {
	GameImport_BotGetChatMessage( chatstate, buf, size );
}
int trap_StringContains( char *str1, char *str2, int casesensitive ) {
	return GameImport_StringContains( str1, str2, casesensitive );
}
int trap_BotFindMatch( char *str, void *match, uint64_t context ) {
	return GameImport_BotFindMatch( str, match, context );
}
void trap_BotMatchVariable( void *match, int variable, char *buf, int size ) {
	GameImport_BotMatchVariable( match, variable, buf, size );
}
void trap_UnifyWhiteSpaces( char *string ) {
	GameImport_UnifyWhiteSpaces( string );
}
void trap_BotReplaceSynonyms( char *string, uint64_t context ) {
	GameImport_BotReplaceSynonyms( string, context );
}
int trap_BotLoadChatFile( int chatstate, char *chatfile, char *chatname ) {
	return GameImport_BotLoadChatFile( chatstate, chatfile, chatname );
}
void trap_BotSetChatGender( int chatstate, int gender ) {
	GameImport_BotSetChatGender( chatstate, gender );
}
void trap_BotSetChatName( int chatstate, char *name, int client ) {
	GameImport_BotSetChatName( chatstate, name, client );
}
void trap_BotResetGoalState( int goalstate ) {
	GameImport_BotResetGoalState( goalstate );
}
void trap_BotResetAvoidGoals( int goalstate ) {
	GameImport_BotResetAvoidGoals( goalstate );
}
void trap_BotRemoveFromAvoidGoals( int goalstate, int number ) {
	GameImport_BotRemoveFromAvoidGoals( goalstate, number );
}
void trap_BotPushGoal( int goalstate, void *goal ) {
	GameImport_BotPushGoal( goalstate, goal );
}
void trap_BotPopGoal( int goalstate ) {
	GameImport_BotPopGoal( goalstate );
}
void trap_BotEmptyGoalStack( int goalstate ) {
	GameImport_BotEmptyGoalStack( goalstate );
}
void trap_BotDumpAvoidGoals( int goalstate ) {
	GameImport_BotDumpAvoidGoals( goalstate );
}
void trap_BotDumpGoalStack( int goalstate ) {
	GameImport_BotDumpGoalStack( goalstate );
}
void trap_BotGoalName( int number, char *name, int size ) {
	GameImport_BotGoalName( number, name, size );
}
int trap_BotGetTopGoal( int goalstate, void *goal ) {
	return GameImport_BotGetTopGoal( goalstate, goal );
}
int trap_BotGetSecondGoal( int goalstate, void *goal ) {
	return GameImport_BotGetSecondGoal( goalstate, goal );
}
int trap_BotChooseLTGItem( int goalstate, vec3_t origin, int *inventory, int travelflags ) {
	return GameImport_BotChooseLTGItem( goalstate, origin, inventory, travelflags );
}
int trap_BotChooseNBGItem( int goalstate, vec3_t origin, int *inventory, int travelflags, void *ltg, float maxtime ) {
	return GameImport_BotChooseNBGItem( goalstate, origin, inventory, travelflags, ltg, maxtime );
}
int trap_BotTouchingGoal( vec3_t origin, void *goal ) {
	return GameImport_BotTouchingGoal( origin, goal );
}
int trap_BotItemGoalInVisButNotVisible( int viewer, vec3_t eye, vec3_t viewangles, void *goal ) {
	return GameImport_BotItemGoalInVisButNotVisible( viewer, eye, viewangles, goal );
}
int trap_BotGetLevelItemGoal( int index, char *classname, void *goal ) {
	return GameImport_BotGetLevelItemGoal( index, classname, goal );
}
int trap_BotGetNextCampSpotGoal( int num, void *goal ) {
	return GameImport_BotGetNextCampSpotGoal( num, goal );
}
int trap_BotGetMapLocationGoal( char *name, void *goal ) {
	return GameImport_BotGetMapLocationGoal( name, goal );
}
float trap_BotAvoidGoalTime( int goalstate, int number ) {
	return GameImport_BotAvoidGoalTime( goalstate, number );
}
void trap_BotSetAvoidGoalTime( int goalstate, int number, float avoidtime ) {
	GameImport_BotSetAvoidGoalTime( goalstate, number, avoidtime );
}
void trap_BotInitLevelItems( void ) {
	GameImport_BotInitLevelItems();
}
void trap_BotUpdateEntityItems( void ) {
	GameImport_BotUpdateEntityItems();
}
int trap_BotLoadItemWeights( int goalstate, char *filename ) {
	return GameImport_BotLoadItemWeights( goalstate, filename );
}
void trap_BotFreeItemWeights( int goalstate ) {
	GameImport_BotFreeItemWeights( goalstate );
}
void trap_BotInterbreedGoalFuzzyLogic( int parent1, int parent2, int child ) {
	GameImport_BotInterbreedGoalFuzzyLogic( parent1, parent2, child );
}
void trap_BotSaveGoalFuzzyLogic( int goalstate, char *filename ) {
	GameImport_BotSaveGoalFuzzyLogic( goalstate, filename );
}
void trap_BotMutateGoalFuzzyLogic( int goalstate, float range ) {
	GameImport_BotMutateGoalFuzzyLogic( goalstate, range );
}
int trap_BotAllocGoalState( int state ) {
	return GameImport_BotAllocGoalState( state );
}
void trap_BotFreeGoalState( int handle ) {
	GameImport_BotFreeGoalState( handle );
}
void trap_BotResetMoveState( int movestate ) {
	GameImport_BotResetMoveState( movestate );
}
void trap_BotAddAvoidSpot( int movestate, vec3_t origin, float radius, int type ) {
	GameImport_BotAddAvoidSpot( movestate, origin, radius, type );
}
void trap_BotMoveToGoal( void *result, int movestate, void *goal, int travelflags ) {
	GameImport_BotMoveToGoal( result, movestate, goal, travelflags );
}
int trap_BotMoveInDirection( int movestate, vec3_t dir, float speed, int type ) {
	return GameImport_BotMoveInDirection( movestate, dir, speed, type );
}
void trap_BotResetAvoidReach( int movestate ) {
	GameImport_BotResetAvoidReach( movestate );
}
void trap_BotResetLastAvoidReach( int movestate ) {
	GameImport_BotResetLastAvoidReach( movestate );
}
int trap_BotReachabilityArea( vec3_t origin, int testground ) {
	return GameImport_BotReachabilityArea( origin, testground );
}
int trap_BotMovementViewTarget( int movestate, void *goal, int travelflags, float lookahead, vec3_t target ) {
	return GameImport_BotMovementViewTarget( movestate, goal, travelflags, lookahead, target );
}
int trap_BotPredictVisiblePosition( vec3_t origin, int areanum, void *goal, int travelflags, vec3_t target ) {
	return GameImport_BotPredictVisiblePosition( origin, areanum, goal, travelflags, target );
}
int trap_BotAllocMoveState( void ) {
	return GameImport_BotAllocMoveState();
}
void trap_BotFreeMoveState( int handle ) {
	GameImport_BotFreeMoveState( handle );
}
void trap_BotInitMoveState( int handle, void *initmove ) {
	GameImport_BotInitMoveState( handle, initmove );
}
int trap_BotChooseBestFightWeapon( int weaponstate, int *inventory ) {
	return GameImport_BotChooseBestFightWeapon( weaponstate, inventory );
}
void trap_BotGetWeaponInfo( int weaponstate, int weapon, void *weaponinfo ) {
	GameImport_BotGetWeaponInfo( weaponstate, weapon, weaponinfo );
}
int trap_BotLoadWeaponWeights( int weaponstate, char *filename ) {
	return GameImport_BotLoadWeaponWeights( weaponstate, filename );
}
int trap_BotAllocWeaponState( void ) {
	return GameImport_BotAllocWeaponState();
}
void trap_BotFreeWeaponState( int weaponstate ) {
	GameImport_BotFreeWeaponState( weaponstate );
}
void trap_BotResetWeaponState( int weaponstate ) {
	GameImport_BotResetWeaponState( weaponstate );
}
int trap_GeneticParentsAndChildSelection( int numranks, float *ranks, int *parent1, int *parent2, int *child ) {
	return GameImport_GeneticParentsAndChildSelection( numranks, ranks, parent1, parent2, child );
}
int trap_PC_LoadSource( const char *filename ) {
	return GameImport_PC_LoadSource( filename );
}
int trap_PC_FreeSource( int handle ) {
	return GameImport_PC_FreeSource( handle );
}
int trap_PC_ReadToken( int handle, pc_token_t *pc_token ) {
	return GameImport_PC_ReadToken( handle, pc_token );
}
int trap_PC_SourceFileAndLine( int handle, char *filename, int *line ) {
	return GameImport_PC_SourceFileAndLine( handle, filename, line );
}

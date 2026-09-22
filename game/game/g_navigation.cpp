#include "g_local.h"
#include "botlib.h"

void BotInputToUserCommand( bot_input_t *input, usercmd_t *command, int deltaAngles[3], int time );
static navWorld_t *navigationWorld;
static aiBehavior_t navigationBehavior;
static uint8_t navigationHash[32], behaviorHash[32];
static int navigationTime;
struct navigationActor_t {
	bool active;
	int spawn;
	navPath_t path;
	navFollowState_t cursor;
	aiState_t behavior;
	aiAction_t action;
	float goal[3];
};
static navigationActor_t navigationActors[MAX_CLIENTS];

bool G_NavigationEnabled() {
	return navigationWorld != nullptr;
}
void G_ShutdownNavigation() {
	Nav_Close( navigationWorld );
	navigationWorld = nullptr;
	navigationBehavior = {};
	memset( navigationActors, 0, sizeof( navigationActors ) );
}
void G_InitNavigation() {
	G_ShutdownNavigation();
	vmCvar_t path, behavior;
	trap_Cvar_Register( &path, "g_navigation", "", CVAR_LATCH );
	trap_Cvar_Register( &behavior, "g_behavior", "", CVAR_LATCH );
	if ( !path.string[0] && !behavior.string[0] )
		return;
	if ( !path.string[0] || !behavior.string[0] || strlen( path.string ) >= MAX_QPATH || strlen( behavior.string ) >= MAX_QPATH )
		G_Error( "Navigation requires both g_navigation and g_behavior cooked paths" );
	if ( !AI_LoadBehavior( behavior.string, &navigationBehavior, behaviorHash ) )
		G_Error( "Behavior rejected: %s", behavior.string );
	navigationWorld = Nav_LoadFile( path.string, uint32_t( trap_Cvar_VariableIntegerValue( "sv_mapChecksum" ) ), MAX_CLIENTS, navigationHash );
	if ( !navigationWorld )
		G_Error( "Navigation rejected: %s (must match current collision map)", path.string );
	navigationTime = level.time;
	G_Printf( "Navigation ready: %s behavior=%s\n", path.string, navigationBehavior.header.name );
}
static void NavigationPatrol( navigationActor_t *actor, const float feet[3] ) {
	float distance = 0;
	navPath_t candidate;
	// ponytail: bounded scan of existing map spawns; authored patrol schedules can
	// replace this when a game needs more than deterministic point-to-point patrol.
	uint32_t tested = 0;
	for ( int i = MAX_CLIENTS; i < level.num_entities && tested < 64; ++i ) {
		const auto &entity = g_entities[i];
		if ( !entity.inuse || !entity.classname || strcmp( entity.classname, "info_player_deathmatch" ) )
			continue;
		++tested;
		const float squared = DistanceSquared( feet, entity.s.origin );
		if ( squared <= distance || squared < 64 * 64 || !Nav_Path( navigationWorld, feet, entity.s.origin, true, &candidate ) || !candidate.complete )
			continue;
		distance = squared;
		actor->path = candidate;
		actor->cursor = {};
		VectorCopy( candidate.points[candidate.count - 1].position, actor->goal );
	}
}
bool G_NavigationFrame( int time ) {
	if ( !navigationWorld )
		return false;
	if ( time <= navigationTime )
		return true;
	const uint32_t elapsed = uint32_t( time - navigationTime );
	navigationTime = time;
	for ( int owner = 0; owner < level.maxclients; ++owner ) {
		auto &entity = g_entities[owner];
		auto &actor = navigationActors[owner];
		if ( !entity.inuse || !entity.client || entity.client->pers.connected != CON_CONNECTED ||
			 !( entity.r.svFlags & SVF_BOT ) || entity.client->sess.sessionTeam == TEAM_SPECTATOR ) {
			actor = {};
			continue;
		}
		auto &player = entity.client->ps;
		if ( !actor.active || actor.spawn != player.persistant[PERS_SPAWN_COUNT] ) {
			actor = {};
			actor.active = true;
			actor.spawn = player.persistant[PERS_SPAWN_COUNT];
		}
		bot_input_t input{};
		input.weapon = BG_WeaponDefinition( 0 ) ? 1 : WP_MACHINEGUN;
		VectorCopy( player.viewangles, input.viewangles );
		if ( entity.health <= 0 ) {
			input.actionflags = ACTION_RESPAWN;
		} else {
			vec3_t feet;
			VectorCopy( player.origin, feet );
			feet[2] += entity.r.mins[2];
			const aiInputs_t observations = { false, false, false, fminf( 1, float( entity.health ) / 100 ), 0 };
			actor.action = AI_UpdateBehavior( navigationBehavior, observations, elapsed < 100U ? elapsed : 100U, &actor.behavior );
			if ( actor.action == AI_PATROL ) {
				if ( !actor.path.count || actor.cursor.point == actor.path.count )
					NavigationPatrol( &actor, feet );
				navFollowOutput_t following;
				if ( actor.path.count && Nav_Follow( actor.path, feet, player.groundEntityNum != ENTITYNUM_NONE, 16, &actor.cursor, &following ) && !following.arrived ) {
					vec3_t desired;
					VectorSubtract( following.position, feet, desired );
					desired[2] = 0;
					const float distance = VectorNormalize( desired );
					VectorScale( desired, fminf( float( player.speed ), distance * 8 ), desired );
					navObstacle_t obstacles[MAX_CLIENTS];
					uint32_t count = 0;
					for ( int other = 0; other < level.maxclients; ++other ) {
						const auto &neighbor = g_entities[other];
						if ( other == owner || !neighbor.inuse || !neighbor.client || neighbor.health <= 0 || neighbor.client->sess.sessionTeam == TEAM_SPECTATOR )
							continue;
						auto &obstacle = obstacles[count++];
						VectorCopy( neighbor.client->ps.origin, obstacle.position );
						obstacle.position[2] += neighbor.r.mins[2];
						VectorCopy( neighbor.client->ps.velocity, obstacle.velocity );
						obstacle.radius = 15;
					}
					vec3_t velocity;
					VectorCopy( desired, velocity );
					if ( !actor.cursor.phase )
						Nav_Avoid( navigationWorld, feet, player.velocity, desired, float( player.speed ), obstacles, count, velocity );
					const float speed = VectorNormalize( velocity );
					VectorCopy( velocity, input.dir );
					input.speed = player.speed > 0 ? fminf( 400, 400 * speed / float( player.speed ) ) : 0;
					if ( speed > 1 )
						vectoangles( velocity, input.viewangles );
					if ( following.kind == NAV_LINK_JUMP )
						input.actionflags |= ACTION_JUMP;
					if ( actor.cursor.phase == 2 && actor.path.points[actor.cursor.point].kind == NAV_LINK_LAUNCH )
						input.speed = 0;
				}
			}
		}
		usercmd_t command;
		BotInputToUserCommand( &input, &command, player.delta_angles, time );
		trap_BotUserCommand( owner, &command );
	}
	return true;
}
#ifdef AFTERSHOCK_DEVTOOLS
bool G_DevAI( int owner, devAIState_t *out ) {
	if ( !navigationWorld || owner < 0 || owner >= level.maxclients || !navigationActors[owner].active )
		return false;
	const auto &actor = navigationActors[owner];
	*out = {};
	out->path = actor.path;
	out->cursor = actor.cursor;
	out->behavior = actor.behavior;
	out->action = actor.action;
	VectorCopy( g_entities[owner].client->ps.origin, out->position );
	VectorCopy( actor.goal, out->goal );
	Q_strncpyz( out->name, AI_StateName( navigationBehavior, actor.behavior ), sizeof( out->name ) );
	return true;
}
#endif

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

struct navigationHeaderSave_t {
	uint32_t enabled;
	int32_t time;
	uint8_t navigation[32], behavior[32];
};
static_assert( sizeof( navigationHeaderSave_t ) == 72 && offsetof( navigationHeaderSave_t, navigation ) == 8 );
static constexpr stateField_t navigationHeaderFields[] = {
	{ "enabled", offsetof( navigationHeaderSave_t, enabled ), 1, stateType_t::UInt32 },
	{ "time", offsetof( navigationHeaderSave_t, time ), 1, stateType_t::Int32 },
	{ "navigation", offsetof( navigationHeaderSave_t, navigation ), 32, stateType_t::Bytes },
	{ "behavior", offsetof( navigationHeaderSave_t, behavior ), 32, stateType_t::Bytes },
};
static constexpr stateSchema_t navigationHeaderSchema = { "game.navigation", 1, 1, sizeof( navigationHeaderSave_t ), navigationHeaderFields, 4 };
static const gCachedCvar_t savedNavigationCvars[] = {
	{ "g_navigation", nullptr },
	{ "g_behavior", nullptr },
};
static bool NavigationReadHeader( const stateReader_t &reader, navigationHeaderSave_t *header, bool *present ) {
	uint32_t version;
	*header = {};
	const bool valid = State_Find( reader, navigationHeaderSchema, 0, header, &version, present );
	return *present ? valid && header->enabled <= 1 : true;
}
bool G_ReadNavigationCvarState( const stateReader_t &reader, int apply ) {
	navigationHeaderSave_t header;
	bool present;
	if ( apply < 0 || apply > 2 || !NavigationReadHeader( reader, &header, &present ) )
		return false;
	if ( present )
		return G_ReadCachedCvars( reader, "game.cvars.Navigation", savedNavigationCvars, 2, apply );
	// Frozen pre-navigation checkpoints use the legacy controller. Clear a live
	// session's selected nav assets before its fresh map is initialized.
	if ( apply ) {
		trap_Cvar_Set( "g_navigation", "" );
		trap_Cvar_Set( "g_behavior", "" );
	}
	return true;
}
struct navigationActorSave_t {
	uint32_t active;
	int32_t spawn;
	uint32_t action;
	aiState_t behavior;
	uint32_t point, phase, count, complete;
	float goal[3], positions[NAV_MAX_POINTS * 3];
	uint32_t links[NAV_MAX_POINTS], kinds[NAV_MAX_POINTS];
};
static_assert( sizeof( navigationActorSave_t ) == 5176 && offsetof( navigationActorSave_t, positions ) == 56 );
static constexpr stateField_t navigationActorFields[] = {
	{ "active", offsetof( navigationActorSave_t, active ), 1, stateType_t::UInt32 },
	{ "spawn", offsetof( navigationActorSave_t, spawn ), 1, stateType_t::Int32 },
	{ "action", offsetof( navigationActorSave_t, action ), 1, stateType_t::UInt32 },
	{ "state", offsetof( navigationActorSave_t, behavior.current ), 1, stateType_t::UInt32 },
	{ "elapsed", offsetof( navigationActorSave_t, behavior.elapsed ), 1, stateType_t::UInt32 },
	{ "transitions", offsetof( navigationActorSave_t, behavior.transitions ), 1, stateType_t::UInt32 },
	{ "initialized", offsetof( navigationActorSave_t, behavior.initialized ), 1, stateType_t::UInt32 },
	{ "point", offsetof( navigationActorSave_t, point ), 1, stateType_t::UInt32 },
	{ "phase", offsetof( navigationActorSave_t, phase ), 1, stateType_t::UInt32 },
	{ "count", offsetof( navigationActorSave_t, count ), 1, stateType_t::UInt32 },
	{ "complete", offsetof( navigationActorSave_t, complete ), 1, stateType_t::UInt32 },
	{ "goal", offsetof( navigationActorSave_t, goal ), 3, stateType_t::Float32 },
	{ "positions", offsetof( navigationActorSave_t, positions ), NAV_MAX_POINTS * 3, stateType_t::Float32 },
	{ "links", offsetof( navigationActorSave_t, links ), NAV_MAX_POINTS, stateType_t::UInt32 },
	{ "kinds", offsetof( navigationActorSave_t, kinds ), NAV_MAX_POINTS, stateType_t::UInt32 },
};
static constexpr stateSchema_t navigationActorSchema = { "game.navigationActor", 1, 1, sizeof( navigationActorSave_t ), navigationActorFields, 15 };
static bool NavigationRestoreActor( const navigationActorSave_t &saved, navigationActor_t *actor ) {
	if ( saved.active > 1 || saved.action > AI_ATTACK || saved.behavior.initialized > 1 || saved.count > NAV_MAX_POINTS ||
		 saved.point > saved.count || saved.phase > 2 || saved.complete > 1 )
		return false;
	if ( saved.behavior.initialized ) {
		if ( saved.behavior.current >= navigationBehavior.header.stateCount )
			return false;
		for ( uint32_t i = 0; i < navigationBehavior.header.stateCount; ++i )
			if ( navigationBehavior.states[i].parent == saved.behavior.current )
				return false;
	}
	for ( float value : saved.goal )
		if ( !std::isfinite( value ) || fabsf( value ) > 1048576 )
			return false;
	*actor = {};
	actor->active = saved.active != 0;
	actor->spawn = saved.spawn;
	actor->action = aiAction_t( saved.action );
	actor->behavior = saved.behavior;
	actor->cursor = { saved.point, saved.phase };
	actor->path.count = saved.count;
	actor->path.complete = saved.complete != 0;
	VectorCopy( saved.goal, actor->goal );
	for ( uint32_t i = 0; i < saved.count; ++i ) {
		VectorCopy( saved.positions + i * 3, actor->path.points[i].position );
		actor->path.points[i].link = saved.links[i];
		actor->path.points[i].kind = navLinkKind_t( saved.kinds[i] );
	}
	if ( !saved.count )
		return !saved.phase;
	auto cursor = actor->cursor;
	navFollowOutput_t following;
	return Nav_Follow( actor->path, actor->goal, false, 16, &cursor, &following );
}
bool G_WriteNavigationState( stateWriter_t *writer ) {
	if ( !writer )
		return false;
	navigationHeaderSave_t header{};
	if ( navigationWorld ) {
		header.enabled = 1;
		header.time = navigationTime;
		memcpy( header.navigation, navigationHash, 32 );
		memcpy( header.behavior, behaviorHash, 32 );
	}
	if ( !State_Append( writer, navigationHeaderSchema, 0, &header ) ||
		 !G_WriteCachedCvars( writer, "game.cvars.Navigation", savedNavigationCvars, 2 ) )
		return false;
	if ( !navigationWorld )
		return true;
	for ( uint32_t i = 0; i < MAX_CLIENTS; ++i ) {
		const auto &actor = navigationActors[i];
		navigationActorSave_t saved{};
		saved.active = actor.active;
		saved.spawn = actor.spawn;
		saved.action = actor.action;
		saved.behavior = actor.behavior;
		saved.point = actor.cursor.point;
		saved.phase = actor.cursor.phase;
		saved.count = actor.path.count;
		saved.complete = actor.path.complete;
		VectorCopy( actor.goal, saved.goal );
		if ( saved.count > NAV_MAX_POINTS )
			return false;
		for ( uint32_t point = 0; point < saved.count; ++point ) {
			VectorCopy( actor.path.points[point].position, saved.positions + point * 3 );
			saved.links[point] = actor.path.points[point].link;
			saved.kinds[point] = actor.path.points[point].kind;
		}
		navigationActor_t validated;
		if ( !NavigationRestoreActor( saved, &validated ) || !State_Append( writer, navigationActorSchema, i, &saved ) )
			return false;
	}
	return true;
}
bool G_ReadNavigationState( const stateReader_t &reader, bool apply ) {
	navigationHeaderSave_t header;
	bool present;
	if ( !NavigationReadHeader( reader, &header, &present ) || bool( header.enabled ) != bool( navigationWorld ) )
		return false;
	if ( !header.enabled )
		return true;
	if ( memcmp( header.navigation, navigationHash, 32 ) || memcmp( header.behavior, behaviorHash, 32 ) )
		return false;
	static navigationActor_t restored[MAX_CLIENTS];
	uint32_t version;
	for ( uint32_t i = 0; i < MAX_CLIENTS; ++i ) {
		navigationActorSave_t saved;
		if ( !State_Find( reader, navigationActorSchema, i, &saved, &version ) || !NavigationRestoreActor( saved, &restored[i] ) )
			return false;
	}
	if ( apply ) {
		memcpy( navigationActors, restored, sizeof( navigationActors ) );
		navigationTime = header.time;
	}
	return true;
}
bool G_ValidateNavigationReferences( const stateReader_t &reader, const gentity_t *entities, const gclient_t *clients ) {
	if ( !navigationWorld )
		return true;
	uint32_t version;
	for ( uint32_t i = 0; i < MAX_CLIENTS; ++i ) {
		navigationActorSave_t saved;
		if ( !State_Find( reader, navigationActorSchema, i, &saved, &version ) )
			return false;
		const bool active = clients[i].pers.connected == CON_CONNECTED && ( entities[i].r.svFlags & SVF_BOT ) && clients[i].sess.sessionTeam != TEAM_SPECTATOR;
		if ( bool( saved.active ) != active || ( active && saved.spawn != clients[i].ps.persistant[PERS_SPAWN_COUNT] ) )
			return false;
	}
	return true;
}

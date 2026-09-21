#include "g_local.h"

static struct {
	animAsset_t asset;
	void *storage;
	float footHeight[2], rootYaw;
} animationRigs[2];
static struct {
	bool active;
	uint32_t manual[2];
	int spawn;
	uint32_t clock;
	float facing, turnSign;
	animState_t state[2];
	float parameters[2][ANIM_MAX_PARAMETERS];
	gentity_t *entity[2];
	animBox_t boxes[ANIM_MAX_BOXES];
	uint32_t boxCount;
} animationActors[MAX_CLIENTS];
static vmCvar_t animationTrace;
static bool animationEnabled;

#ifdef AFTERSHOCK_DEVTOOLS
bool G_DevAnimation( int owner, int rig, devAnimationState_t *out ) {
	if ( !animationEnabled || owner < 0 || owner >= level.maxclients || rig < 0 || rig > 1 || !animationActors[owner].active )
		return false;
	*out = {};
	out->state = animationActors[owner].state[rig];
	Q_strncpyz( out->name, Anim_StateName( &animationRigs[rig].asset, out->state.current ), sizeof( out->name ) );
	return true;
}
#endif

void G_ShutdownAnimation( void ) {
	for ( auto &rig : animationRigs ) {
		Anim_FreeFile( rig.storage );
		rig = {};
	}
	memset( animationActors, 0, sizeof( animationActors ) );
	animationEnabled = false;
}
void G_InitAnimation( void ) {
	G_ShutdownAnimation();
	trap_Cvar_Register( &animationTrace, "g_animationTrace", "0", 0 );
	const char *names[2] = { "g_animationBody", "g_animationRifle" };
	for ( int i = 0; i < 2; ++i ) {
		vmCvar_t path;
		trap_Cvar_Register( &path, names[i], "", CVAR_LATCH );
		trap_SetConfigstring( CS_ANIMATION_BODY + i, "" );
		if ( !path.string[0] )
			continue;
		auto &rig = animationRigs[i];
		rig.storage = Anim_LoadFile( path.string, &rig.asset );
		if ( !rig.storage )
			G_Error( "Animation rejected: cannot load %s", path.string );
		char hash[65];
		Anim_HashString( rig.asset.hash, hash );
		char config[MAX_QPATH + 66];
		if ( strlen( path.string ) >= MAX_QPATH )
			G_Error( "Animation rejected: graph path too long" );
		Com_sprintf( config, sizeof( config ), "%s %s", path.string, hash );
		trap_SetConfigstring( CS_ANIMATION_BODY + i, config );
	}
	if ( bool( animationRigs[0].storage ) != bool( animationRigs[1].storage ) )
		G_Error( "Animation rejected: both body and rifle graphs are required" );
	animationEnabled = animationRigs[0].storage != nullptr;
	if ( animationEnabled ) {
		const auto *asset = &animationRigs[0].asset;
		animState_t state;
		animPose_t pose;
		float parameters[ANIM_MAX_PARAMETERS];
		Anim_Reset( asset, 0, &state );
		Anim_DefaultParameters( asset, parameters );
		if ( !Anim_Evaluate( asset, &state, parameters, 0, &pose ) )
			G_Error( "Animation rejected: bind stance" );
		animationRigs[0].rootYaw = atan2f( pose.world[0][4], pose.world[0][0] ) * 180 / float( M_PI );
		for ( int side = 0; side < 2; ++side ) {
			const int bone = Anim_BoneIndex( asset, side ? "foot.R" : "foot.L" );
			animationRigs[0].footHeight[side] = bone >= 0 ? pose.world[bone][11] : 0;
		}
	}
}
static void SetAnimationInput( int owner, int rig, const char *name, float value ) {
	const int index = Anim_ParameterIndex( &animationRigs[rig].asset, name );
	if ( index >= 0 )
		animationActors[owner].parameters[rig][index] = value;
}
qboolean G_AnimationCommand( int owner ) {
	if ( !animationEnabled || owner < 0 || owner >= level.maxclients || !animationActors[owner].active )
		return qfalse;
	char name[64], text[32];
	trap_Argv( 1, name, sizeof( name ) );
	trap_Argv( 2, text, sizeof( text ) );
	if ( strcmp( text, "0" ) && strcmp( text, "1" ) )
		return qfalse;
	const char *allowed[] = { "ads", "fire", "reload", "sprint", "jump", "aim_up", "aim_down", "lean_left", "lean_right", "crouch", "prone", "turn" };
	bool permitted = false;
	for ( const char *action : allowed )
		if ( !strcmp( action, name ) )
			permitted = true;
	if ( !permitted )
		return qfalse;
	bool found = false;
	for ( int rig = 0; rig < 2; ++rig ) {
		const int index = Anim_ParameterIndex( &animationRigs[rig].asset, name );
		if ( index >= 0 ) {
			animationActors[owner].parameters[rig][index] = text[0] == '1' ? 1.0f : 0.0f;
			animationActors[owner].manual[rig] |= 1u << index;
			found = true;
		}
	}
	return found ? qtrue : qfalse;
}
void G_RunAnimation( void ) {
	if ( !animationEnabled )
		return;
	trap_Cvar_Update( &animationTrace );
	for ( int owner = 0; owner < level.maxclients; ++owner ) {
		auto &actor = animationActors[owner];
		gentity_t *player = &g_entities[owner];
		const bool live = player->inuse && player->client && player->client->pers.connected == CON_CONNECTED && player->client->sess.sessionTeam != TEAM_SPECTATOR;
		if ( !live ) {
			for ( auto *entity : actor.entity )
				if ( entity )
					G_FreeEntity( entity );
			actor = {};
			continue;
		}
		const playerState_t *ps = &player->client->ps;
		if ( !actor.active || actor.spawn != ps->persistant[PERS_SPAWN_COUNT] ) {
			actor.clock = uint32_t( level.time );
			actor.spawn = ps->persistant[PERS_SPAWN_COUNT];
			actor.active = true;
			actor.facing = ps->viewangles[YAW];
			actor.turnSign = 1;
			for ( int rig = 0; rig < 2; ++rig ) {
				Anim_Reset( &animationRigs[rig].asset, actor.clock, &actor.state[rig] );
				Anim_DefaultParameters( &animationRigs[rig].asset, actor.parameters[rig] );
				actor.manual[rig] = 0;
				if ( !actor.entity[rig] ) {
					actor.entity[rig] = G_Spawn();
					actor.entity[rig]->classname = (char *)"animation_state";
					actor.entity[rig]->r.svFlags = rig ? SVF_SINGLECLIENT : 0;
					actor.entity[rig]->r.singleClient = owner;
				}
			}
		}
		const float pitch = AngleNormalize180( ps->viewangles[PITCH] );
		for ( int direction = 0; direction < 2; ++direction ) {
			const char *name = direction ? "aim_down" : "aim_up";
			const int index = Anim_ParameterIndex( &animationRigs[0].asset, name );
			if ( index >= 0 && !( actor.manual[0] & ( 1u << index ) ) )
				actor.parameters[0][index] = fminf( fmaxf( ( direction ? pitch : -pitch ) / 26, 0 ), 1 );
		}
		const float speed = sqrtf( ps->velocity[0] * ps->velocity[0] + ps->velocity[1] * ps->velocity[1] );
		if ( strcmp( Anim_StateName( &animationRigs[0].asset, actor.state[0].current ), "turn" ) ) {
			if ( speed > 40 )
				actor.facing = ps->viewangles[YAW];
			const float difference = AngleSubtract( ps->viewangles[YAW], actor.facing );
			if ( speed <= 40 && fabsf( difference ) > 45 ) {
				actor.turnSign = difference < 0 ? -1.0f : 1.0f;
				SetAnimationInput( owner, 0, "turn", 1 );
			}
		}
		SetAnimationInput( owner, 0, "move", fminf( speed / 120, 1 ) );
		SetAnimationInput( owner, 0, "run", fminf( fmaxf( ( speed - 120 ) / 200, 0 ), 1 ) );
		if ( player->client->buttons & BUTTON_ATTACK )
			SetAnimationInput( owner, 1, "fire", 1 );
		if ( ps->groundEntityNum == ENTITYNUM_NONE && player->client->pers.cmd.upmove > 0 )
			SetAnimationInput( owner, 1, "jump", 1 );
		// Advance only in 20 ms steps. Render interpolation never feeds these states.
		if ( uint32_t( level.time ) - actor.clock > 5000 )
			G_Error( "Animation rejected: simulation clock jumped" );
		bool advanced = false;
		while ( uint32_t( level.time ) - actor.clock >= 20 || !actor.state[0].initialized ) {
			advanced = true;
			if ( actor.state[0].initialized )
				actor.clock += 20;
			for ( int rig = 0; rig < 2; ++rig ) {
				const auto *asset = &animationRigs[rig].asset;
				animEvents_t events;
				const uint32_t previous = actor.state[rig].current;
				const bool initial = !actor.state[rig].initialized;
				float finalTurn = 0;
				if ( rig == 0 && !strcmp( Anim_StateName( asset, previous ), "turn" ) ) {
					animPose_t pose;
					if ( !Anim_Evaluate( asset, &actor.state[rig], actor.parameters[rig], actor.clock, &pose ) )
						G_Error( "Animation rejected: turn pose" );
					finalTurn = AngleSubtract( atan2f( pose.world[0][4], pose.world[0][0] ) * 180 / float( M_PI ), animationRigs[0].rootYaw );
				}
				if ( !Anim_Tick( asset, actor.parameters[rig], actor.clock, &actor.state[rig], &events ) )
					G_Error( "Animation rejected: state/event capacity" );
				if ( rig == 0 && previous != actor.state[rig].current ) {
					actor.facing = AngleNormalize180( actor.facing + actor.turnSign * finalTurn );
					if ( animationTrace.integer && !strcmp( Anim_StateName( asset, previous ), "turn" ) )
						G_Printf( "Animation body facing: yaw=%.3f view=%.3f\n", double( actor.facing ), double( ps->viewangles[YAW] ) );
				}
				if ( animationTrace.integer && ( initial || previous != actor.state[rig].current ) )
					G_Printf( "Animation server state: owner=%d rig=%d state=%s\n", owner, rig, Anim_StateName( asset, actor.state[rig].current ) );
				for ( uint32_t e = 0; e < events.count; ++e ) {
					const char *name = Anim_EventName( asset, events.items[e].id );
					if ( animationTrace.integer )
						G_Printf( "Animation game event: owner=%d rig=%d name=%s sequence=%u\n", owner, rig, name, events.items[e].sequence );
				}
				// Commands for these actions are one-shot requests; held fire is sampled above.
				if ( rig ) {
					const char *state = Anim_StateName( asset, actor.state[rig].current );
					if ( !strcmp( state, "fire" ) || !strcmp( state, "reload" ) || !strcmp( state, "jump" ) )
						SetAnimationInput( owner, rig, state, 0 );
				} else if ( !strcmp( Anim_StateName( asset, actor.state[rig].current ), "turn" ) )
					SetAnimationInput( owner, rig, "turn", 0 );
			}
		}
		if ( !advanced )
			continue;
		// Ground contacts are authoritative inputs; clients use the replicated
		// offsets instead of doing their own gameplay collision query.
		animPose_t stance;
		const auto *body = &animationRigs[0].asset;
		if ( !Anim_Evaluate( body, &actor.state[0], actor.parameters[0], actor.clock, &stance ) )
			G_Error( "Animation rejected: foot stance" );
		float yaw = actor.facing;
		if ( !strcmp( Anim_StateName( body, actor.state[0].current ), "turn" ) )
			yaw += actor.turnSign * AngleSubtract( atan2f( stance.world[0][4], stance.world[0][0] ) * 180 / float( M_PI ), animationRigs[0].rootYaw );
		SetAnimationInput( owner, 0, "aim_yaw", fminf( fmaxf( AngleSubtract( ps->viewangles[YAW], yaw ), -90 ), 90 ) );
		if ( !Anim_RemoveRootMotion( body, &stance ) )
			G_Error( "Animation rejected: in-place stance" );
		vec3_t bodyAngles = { 0, yaw, 0 }, bodyAxis[3];
		AnglesToAxis( bodyAngles, bodyAxis );
		for ( int side = 0; side < 2; ++side ) {
			const int bone = Anim_BoneIndex( body, side ? "foot.R" : "foot.L" );
			if ( bone < 0 )
				continue;
			vec3_t start, end;
			for ( int a = 0; a < 3; ++a )
				start[a] = ps->origin[a] + bodyAxis[0][a] * stance.world[bone][3] + bodyAxis[1][a] * stance.world[bone][7] + bodyAxis[2][a] * stance.world[bone][11];
			start[2] += MINS_Z;
			const float original = start[2];
			VectorCopy( start, end );
			start[2] += 12;
			end[2] -= 24;
			trace_t trace;
			trap_Trace( &trace, start, nullptr, nullptr, end, owner, MASK_SOLID );
			const float offset = trace.fraction < 1 && !trace.startsolid ? fminf( fmaxf( trace.endpos[2] + animationRigs[0].footHeight[side] - original, -16 ), 16 ) : 0;
			SetAnimationInput( owner, 0, side ? "foot_right" : "foot_left", offset );
		}
		for ( int rig = 0; rig < 2; ++rig ) {
			gentity_t *entity = actor.entity[rig];
			vec3_t origin = { ps->origin[0], ps->origin[1], ps->origin[2] + MINS_Z };
			vec3_t angles = { 0, rig ? ps->viewangles[YAW] : yaw, 0 };
			if ( !BG_AnimationToEntityState( &actor.state[rig], actor.parameters[rig], owner, rig, origin, angles, &entity->s ) )
				G_Error( "Animation rejected: snapshot state" );
			VectorCopy( origin, entity->r.currentOrigin );
			// State visibility follows the player's occupied leaves, including when
			// its root point alone lies outside the viewer's visible leaf.
			VectorCopy( player->r.mins, entity->r.mins );
			VectorCopy( player->r.maxs, entity->r.maxs );
			entity->r.mins[2] -= MINS_Z;
			entity->r.maxs[2] -= MINS_Z;
			trap_LinkEntity( entity );
			if ( rig == 0 ) {
				animPose_t pose;
				vec3_t axis[3];
				AnglesToAxis( entity->s.angles, axis );
				if ( !BG_AnimationPose( &animationRigs[0].asset, &actor.state[0], actor.parameters[0], actor.clock, 0, &pose ) )
					G_Error( "Animation rejected: gameplay pose" );
				actor.boxCount = Anim_HitBoxes( &animationRigs[0].asset, &pose, entity->s.origin, axis, actor.boxes, ANIM_MAX_BOXES );
				if ( !actor.boxCount )
					G_Error( "Animation rejected: body hit boxes required" );
				if ( animationTrace.integer ) {
					char hash[65];
					Anim_BoxHash( actor.boxes, actor.boxCount, hash );
					G_Printf( "Animation server boxes: tick=%u owner=%d hash=%s\n", actor.clock, owner, hash );
				}
			}
		}
	}
}

const animBox_t *G_AnimationHitBoxes( int owner, uint32_t *count ) {
	*count = 0;
	if ( !animationEnabled || owner < 0 || owner >= MAX_CLIENTS || !g_entities[owner].client )
		return nullptr;
	const auto &actor = animationActors[owner];
	if ( !actor.active || actor.spawn != g_entities[owner].client->ps.persistant[PERS_SPAWN_COUNT] || !actor.boxCount )
		return nullptr;
	*count = actor.boxCount;
	return actor.boxes;
}

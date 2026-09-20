#include "g_local.h"

static struct {
	bool active;
	int spawn;
	weaponState_t inventory[2][WEAPON_MAX_DEFINITIONS];
	int selected[2];
	gentity_t *entity[2];
} weaponActors[MAX_CLIENTS];
static vmCvar_t weaponTrace;

void G_InitWeapons( void ) {
	BG_ClearWeapons();
	memset( weaponActors, 0, sizeof( weaponActors ) );
	trap_Cvar_Register( &weaponTrace, "g_weaponTrace", "0", 0 );
	vmCvar_t list;
	trap_Cvar_Register( &list, "g_weapons", "", CVAR_LATCH );
	char *cursor = list.string;
	for ( int index = 0; index < int( WEAPON_MAX_DEFINITIONS ); ++index ) {
		trap_SetConfigstring( CS_WEAPONS + index, "" );
		const char *path = COM_Parse( &cursor );
		if ( !path[0] )
			continue;
		char hash[65], config[MAX_QPATH + 66];
		if ( strlen( path ) >= MAX_QPATH || !BG_LoadWeapon( index, path, hash ) )
			G_Error( "Weapon rejected: cannot load %s", path );
		Com_sprintf( config, sizeof( config ), "%s %s", path, hash );
		trap_SetConfigstring( CS_WEAPONS + index, config );
		G_Printf( "Weapon server definition: index=%d name=%s\n", index, BG_WeaponDefinition( index )->name );
	}
	if ( COM_Parse( &cursor )[0] )
		G_Error( "Weapon rejected: definition limit" );
}
void G_ClearWeaponActor( int owner ) {
	auto &actor = weaponActors[owner];
	for ( auto *entity : actor.entity )
		if ( entity )
			G_FreeEntity( entity );
	actor = {};
}
static bool WeaponWallExit( const vec3_t entry, const vec3_t direction, float limit, int owner, vec3_t exit, float *thickness ) {
	// ponytail: one-unit occupancy probes, then an exact reverse surface trace;
	// gaps smaller than one unit count as part of the same penetration thickness.
	if ( limit <= 0 )
		return false;
	for ( float depth = fminf( 1, limit );; depth = fminf( depth + 1, limit ) ) {
		vec3_t candidate;
		VectorMA( entry, depth, direction, candidate );
		if ( !( trap_PointContents( candidate, owner ) & CONTENTS_SOLID ) ) {
			vec3_t inside;
			VectorMA( entry, fminf( 0.03125f, depth * 0.5f ), direction, inside );
			trace_t reverse;
			trap_Trace( &reverse, candidate, nullptr, nullptr, inside, owner, MASK_SOLID );
			if ( reverse.startsolid || reverse.allsolid || reverse.fraction >= 1 )
				return false;
			vec3_t delta;
			VectorSubtract( reverse.endpos, entry, delta );
			*thickness = DotProduct( delta, direction );
			if ( *thickness <= 0 || *thickness > limit )
				return false;
			VectorMA( reverse.endpos, 0.03125f, direction, exit );
			return true;
		}
		if ( depth == limit )
			return false;
	}
}
static void WeaponHit( gentity_t *player, const weaponDef_t *definition, const weaponEvent_t &event, int index ) {
	const bool melee = event.kind == WEAPON_MELEE_EVENT;
	const float range = melee ? definition->melee.range : definition->range;
	vec3_t angles, direction, start, end;
	VectorCopy( player->client->ps.viewangles, angles );
	angles[PITCH] += event.spread[0];
	angles[YAW] += event.spread[1];
	AngleVectors( angles, direction, nullptr, nullptr );
	VectorCopy( player->client->ps.origin, start );
	start[2] += player->client->ps.viewheight;
	VectorMA( start, range, direction, end );
	vec3_t cursor;
	VectorCopy( start, cursor );
	float scale = 1;
	// Bound work per shot; each crossed surface spends its material's damage budget.
	for ( int impact = 0; impact < 4; ++impact ) {
		trace_t trace;
		G_TraceHitscanAtTime( &trace, cursor, end, player->s.number, player, event.time );
		if ( trace.fraction >= 1 || trace.startsolid || trace.allsolid || ( trace.surfaceFlags & SURF_NOIMPACT ) )
			return;
		const auto *material = Weapon_Material( definition, uint32_t( trace.surfaceFlags ) );
		auto *effect = G_TempEntity( trace.endpos, EV_WEAPON_IMPACT );
		effect->s.eventParm = DirToByte( trace.plane.normal );
		effect->s.modelindex = index;
		effect->s.modelindex2 = int( material - definition->materials );
		effect->s.otherEntityNum = player->s.number;
		effect->s.otherEntityNum2 = trace.entityNum;
		vec3_t delta;
		VectorSubtract( trace.endpos, start, delta );
		const float distance = DotProduct( delta, direction );
		if ( trace.entityNum < ENTITYNUM_WORLD && g_entities[trace.entityNum].takedamage ) {
			const int damage = int( melee ? definition->melee.damage : Weapon_Damage( definition, distance ) * scale );
			if ( damage ) {
				G_Damage( &g_entities[trace.entityNum], player, player, direction, trace.endpos, damage, 0, melee ? MOD_GAUNTLET : MOD_MACHINEGUN );
				if ( weaponTrace.integer )
					G_Printf( "Weapon damage: owner=%d target=%d tick=%u damage=%d\n", player->s.number, trace.entityNum, event.time, damage );
			}
			return;
		}
		if ( melee || trace.entityNum != ENTITYNUM_WORLD )
			return;
		float thickness;
		if ( !WeaponWallExit( trace.endpos, direction, fminf( material->depth, range - distance ), player->s.number, cursor, &thickness ) )
			return;
		scale = Weapon_PenetrationDamage( definition, uint32_t( trace.surfaceFlags ), thickness, scale );
		if ( scale <= 0 )
			return;
	}
}
void G_WeaponCommand( gentity_t *player, const usercmd_t *cmd, int commandStart ) {
	if ( !BG_WeaponDefinition( 0 ) )
		return;
	const int owner = player->s.number;
	auto &actor = weaponActors[owner];
	const auto &ps = player->client->ps;
	if ( !actor.active || actor.spawn != ps.persistant[PERS_SPAWN_COUNT] ) {
		G_ClearWeaponActor( owner );
		actor.active = true;
		actor.spawn = ps.persistant[PERS_SPAWN_COUNT];
		for ( int hand = 0; hand < 2; ++hand ) {
			for ( int index = 0; const auto *definition = BG_WeaponDefinition( index ); ++index )
				Weapon_Reset( definition, uint32_t( owner ) * 69069u + uint32_t( actor.spawn ) * 31u + uint32_t( hand ) + uint32_t( index ) * 997u,
					uint32_t( commandStart ), &actor.inventory[hand][index] );
			actor.entity[hand] = G_Spawn();
			actor.entity[hand]->classname = "weapon_state";
			actor.entity[hand]->r.svFlags = SVF_SINGLECLIENT;
			actor.entity[hand]->r.singleClient = owner;
			GameImport_SetEntityReplication( actor.entity[hand]->s.number, 3, 0 );
		}
	}
	trap_Cvar_Update( &weaponTrace );
	for ( int hand = 0; hand < 2; ++hand ) {
		int &selected = actor.selected[hand];
		const auto *definition = BG_WeaponDefinition( selected );
		const int requested = int( cmd->weapon ) - 1;
		if ( requested != selected && BG_WeaponDefinition( requested ) && ps.stats[STAT_HEALTH] > 0 &&
			 Weapon_Switch( definition, &actor.inventory[hand][selected], BG_WeaponDefinition( requested ),
				 &actor.inventory[hand][requested], uint32_t( cmd->serverTime ) ) ) {
			if ( weaponTrace.integer )
				G_Printf( "Weapon switch: owner=%d hand=%d from=%d to=%d magazine=%u\n", owner, hand, selected, requested, actor.inventory[hand][requested].magazine );
			selected = requested;
			definition = BG_WeaponDefinition( selected );
		}
		auto &state = actor.inventory[hand][selected];
		const uint32_t buttons = requested == selected ? BG_WeaponButtons( cmd, hand, &ps ) : 0;
		const uint32_t gap = uint32_t( cmd->serverTime ) - state.time;
		// Pause the weapon across long inactivity; never catch up missed fire.
		if ( int32_t( gap ) > 1000 ) {
			state.time += gap;
			state.nextFire += gap;
			state.nextMelee += gap;
			state.switchUntil += gap;
			state.reloadStart += gap;
		}
		weaponEvents_t events;
		if ( !Weapon_Command( definition, buttons, uint32_t( cmd->serverTime ), &state, &events ) )
			G_Error( "Weapon rejected: command" );
		for ( uint32_t i = 0; i < events.count; ++i ) {
			const auto &event = events.items[i];
			if ( event.kind == WEAPON_MELEE_EVENT || ( event.kind == WEAPON_SHOT && definition->ballistics == WEAPON_HITSCAN ) )
				WeaponHit( player, definition, event, selected );
			if ( weaponTrace.integer )
				G_Printf( "Weapon event: owner=%d hand=%d kind=%u tick=%u sequence=%u\n", owner, hand, event.kind, event.time, event.sequence );
		}
		auto *entity = actor.entity[hand];
		if ( !BG_WeaponToEntityState( &state, uint32_t( actor.spawn ), owner, hand, selected, 0, ps.origin, &entity->s ) )
			G_Error( "Weapon rejected: snapshot" );
		VectorCopy( ps.origin, entity->r.currentOrigin );
		trap_LinkEntity( entity );
		if ( weaponTrace.integer )
			G_Printf( "Weapon server state: owner=%d hand=%d tick=%u sequence=%u magazine=%u reserve=%u chamber=%u ads=%u\n", owner, hand,
				state.time, state.sequence, state.magazine, state.reserve, state.chamber, state.adsQ16 );
	}
}

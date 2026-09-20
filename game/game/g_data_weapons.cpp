#include "g_local.h"

static struct {
	bool active;
	int spawn;
	weaponState_t state[2];
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
static void WeaponHit( gentity_t *player, const weaponDef_t *definition, const weaponEvent_t &event ) {
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
	trace_t trace;
	G_TraceHitscanAtTime( &trace, start, end, player->s.number, player, event.time );
	if ( trace.entityNum >= ENTITYNUM_WORLD || !g_entities[trace.entityNum].takedamage || ( trace.surfaceFlags & SURF_NOIMPACT ) )
		return;
	const int damage = int( melee ? definition->melee.damage : Weapon_Damage( definition, trace.fraction * range ) );
	if ( !damage )
		return;
	G_Damage( &g_entities[trace.entityNum], player, player, direction, trace.endpos, damage, 0, melee ? MOD_GAUNTLET : MOD_MACHINEGUN );
	if ( weaponTrace.integer )
		G_Printf( "Weapon damage: owner=%d target=%d tick=%u damage=%d\n", player->s.number, trace.entityNum, event.time, damage );
}
void G_WeaponCommand( gentity_t *player, const usercmd_t *cmd, int commandStart ) {
	const auto *definition = BG_WeaponDefinition( 0 );
	if ( !definition )
		return;
	const int owner = player->s.number;
	auto &actor = weaponActors[owner];
	const auto &ps = player->client->ps;
	if ( !actor.active || actor.spawn != ps.persistant[PERS_SPAWN_COUNT] ) {
		G_ClearWeaponActor( owner );
		actor.active = true;
		actor.spawn = ps.persistant[PERS_SPAWN_COUNT];
		for ( int hand = 0; hand < 2; ++hand ) {
			Weapon_Reset( definition, uint32_t( owner ) * 69069u + uint32_t( actor.spawn ) * 31u + uint32_t( hand ), uint32_t( commandStart ), &actor.state[hand] );
			actor.entity[hand] = G_Spawn();
			actor.entity[hand]->classname = "weapon_state";
			actor.entity[hand]->r.svFlags = SVF_SINGLECLIENT;
			actor.entity[hand]->r.singleClient = owner;
			GameImport_SetEntityReplication( actor.entity[hand]->s.number, 3, 0 );
		}
	}
	trap_Cvar_Update( &weaponTrace );
	for ( int hand = 0; hand < 2; ++hand ) {
		auto &state = actor.state[hand];
		const uint32_t buttons = BG_WeaponButtons( cmd, hand, &ps );
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
				WeaponHit( player, definition, event );
			if ( weaponTrace.integer )
				G_Printf( "Weapon event: owner=%d hand=%d kind=%u tick=%u sequence=%u\n", owner, hand, event.kind, event.time, event.sequence );
		}
		auto *entity = actor.entity[hand];
		if ( !BG_WeaponToEntityState( &state, uint32_t( actor.spawn ), owner, hand, 0, 0, ps.origin, &entity->s ) )
			G_Error( "Weapon rejected: snapshot" );
		VectorCopy( ps.origin, entity->r.currentOrigin );
		trap_LinkEntity( entity );
		if ( weaponTrace.integer )
			G_Printf( "Weapon server state: owner=%d hand=%d tick=%u sequence=%u magazine=%u reserve=%u chamber=%u ads=%u\n", owner, hand,
				state.time, state.sequence, state.magazine, state.reserve, state.chamber, state.adsQ16 );
	}
}

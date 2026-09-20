#include "g_local.h"

static struct {
	bool active;
	int spawn;
	uint32_t epoch;
	weaponState_t inventory[2][WEAPON_MAX_DEFINITIONS];
	int selected[2];
	uint32_t attachments[2][WEAPON_MAX_DEFINITIONS];
	weaponDef_t configured[2];
	gentity_t *entity[2], *animationEntity[2];
	animState_t animation[2];
	float parameters[2][ANIM_MAX_PARAMETERS];
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
		char hash[65], graphHash[65], config[MAX_QPATH + 132];
		if ( strlen( path ) >= MAX_QPATH || !BG_LoadWeapon( index, path, hash, graphHash ) )
			G_Error( "Weapon rejected: cannot load %s", path );
		Com_sprintf( config, sizeof( config ), "%s %s %s", path, hash, graphHash );
		trap_SetConfigstring( CS_WEAPONS + index, config );
		G_Printf( "Weapon server definition: index=%d name=%s\n", index, BG_WeaponDefinition( index )->name );
	}
	if ( COM_Parse( &cursor )[0] )
		G_Error( "Weapon rejected: definition limit" );
	if ( !BG_WeaponDefinition( 0 ) )
		return;
	if ( level.num_entities > ENTITYNUM_MAX_NORMAL - 4 * level.maxclients - 8 )
		G_Error( "Weapon rejected: map needs four auxiliary slots per configured client" );
	for ( int owner = 0; owner < level.maxclients; ++owner ) {
		auto &actor = weaponActors[owner];
		gentity_t **records[] = { actor.entity, actor.animationEntity };
		for ( int kind = 0; kind < 2; ++kind )
			for ( int hand = 0; hand < 2; ++hand ) {
				auto *entity = records[kind][hand] = G_Spawn();
				entity->classname = kind ? "weapon_animation" : "weapon_state";
				entity->r.svFlags = SVF_SINGLECLIENT;
				entity->r.singleClient = owner;
				GameImport_SetEntityReplication( entity->s.number, 3, 0 );
			}
	}
	G_Printf( "Weapon auxiliary pool: clients=%d records=%d\n", level.maxclients, 4 * level.maxclients );
}
void G_ClearWeaponActor( int owner ) {
	auto &actor = weaponActors[owner];
	if ( actor.active && g_entities[owner].client && g_entities[owner].client->sess.sessionTeam == TEAM_SPECTATOR )
		G_RemoveWeaponProjectiles( owner );
	for ( auto *entity : actor.entity )
		if ( entity )
			trap_UnlinkEntity( entity );
	for ( auto *entity : actor.animationEntity )
		if ( entity )
			trap_UnlinkEntity( entity );
	gentity_t *records[4] = { actor.entity[0], actor.entity[1], actor.animationEntity[0], actor.animationEntity[1] };
	actor = {};
	for ( int hand = 0; hand < 2; ++hand ) {
		actor.entity[hand] = records[hand];
		actor.animationEntity[hand] = records[hand + 2];
	}
}
void G_WeaponAttachmentCommand( int owner ) {
	if ( owner < 0 || owner >= level.maxclients || !weaponActors[owner].active || !g_entities[owner].client || g_entities[owner].health <= 0 || trap_Argc() != 3 )
		return;
	char handText[8], maskText[8];
	trap_Argv( 1, handText, sizeof( handText ) );
	trap_Argv( 2, maskText, sizeof( maskText ) );
	if ( strlen( handText ) != 1 || handText[0] < '0' || handText[0] > '1' || !maskText[0] || strlen( maskText ) > 3 )
		return;
	for ( const char *p = maskText; *p; ++p )
		if ( *p < '0' || *p > '9' )
			return;
	const int hand = handText[0] - '0';
	const uint32_t mask = uint32_t( atoi( maskText ) );
	auto &actor = weaponActors[owner];
	const int selected = actor.selected[hand];
	auto &state = actor.inventory[hand][selected];
	weaponDef_t configured;
	if ( state.reloadStage != WEAPON_NO_STAGE || int32_t( state.time - state.switchUntil ) < 0 ||
		 !Weapon_Configure( BG_WeaponDefinition( selected ), mask, &configured ) )
		return;
	actor.configured[hand] = configured;
	actor.attachments[hand][selected] = mask;
	state.switchUntil = state.nextFire = state.nextMelee = state.time + configured.switchMs;
	state.adsQ16 = 0;
	if ( weaponTrace.integer )
		G_Printf( "Weapon attachment: owner=%d hand=%d mask=%u spread=%.6f fov=%.6f\n", owner, hand, mask,
			double( configured.spreadDegrees ), double( configured.adsFov ) );
}
// ponytail: scan the fixed entity array; cap cosmetics at 128 and retain eight
// unopened slots. At the high-water ceiling, cosmetics wait for a map restart.
static gentity_t *WeaponEffect( const vec3_t origin, int event ) {
	if ( level.num_entities >= ENTITYNUM_MAX_NORMAL - 8 )
		return nullptr;
	int count = 0;
	for ( int number = MAX_CLIENTS; number < level.num_entities; ++number )
		if ( g_entities[number].inuse && g_entities[number].classname && !strcmp( g_entities[number].classname, "weapon_effect" ) )
			++count;
	if ( count >= 128 )
		return nullptr;
	vec3_t position;
	VectorCopy( origin, position );
	auto *effect = G_TempEntity( position, event );
	effect->classname = "weapon_effect";
	return effect;
}
static void WeaponNotify( gentity_t *player, int hand, int definition, const animEvent_t &notify ) {
	auto *entity = WeaponEffect( player->client->ps.origin, EV_WEAPON_NOTIFY );
	if ( entity ) {
		entity->s.modelindex = definition;
		entity->s.otherEntityNum = player->s.number;
		entity->s.otherEntityNum2 = hand;
		entity->s.eventParm = int( notify.id );
		entity->s.time = player->client->ps.persistant[PERS_SPAWN_COUNT];
		entity->s.time2 = int32_t( notify.sequence );
		entity->s.pos.trTime = int32_t( notify.time );
		entity->s.apos.trTime = int32_t( player->rewindSpawn );
	}
	if ( weaponTrace.integer )
		G_Printf( "Weapon notify server: owner=%d hand=%d definition=%d spawn=%d sequence=%u name=%s time=%u\n",
			player->s.number, hand, definition, player->client->ps.persistant[PERS_SPAWN_COUNT], notify.sequence,
			Anim_EventName( BG_WeaponAnimation( definition ), notify.id ), notify.time );
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
		if ( auto *effect = WeaponEffect( trace.endpos, EV_WEAPON_IMPACT ) ) {
			effect->s.eventParm = DirToByte( trace.plane.normal );
			effect->s.modelindex = index;
			effect->s.modelindex2 = int( material - definition->materials );
			effect->s.otherEntityNum = player->s.number;
			effect->s.otherEntityNum2 = trace.entityNum;
		}
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
static bool WeaponProjectileAvailable( void ) {
	if ( level.num_entities >= ENTITYNUM_MAX_NORMAL - 8 )
		return false;
	int count = 0;
	for ( int number = MAX_CLIENTS; number < level.num_entities; ++number )
		if ( g_entities[number].inuse && g_entities[number].s.eType == ET_MISSILE && g_entities[number].s.generic1 == WEAPON_PROJECTILE_TAG )
			++count;
	return count < 64;
}
static void SpawnWeaponProjectile( gentity_t *player, int hand, int index, const weaponDef_t *definition, const weaponEvent_t &event ) {
	gentity_t *entity = G_Spawn();
	entity->classname = "weapon_projectile";
	entity->s.eType = ET_MISSILE;
	entity->s.generic1 = WEAPON_PROJECTILE_TAG;
	entity->s.modelindex = index;
	entity->s.otherEntityNum = player->s.number;
	entity->s.otherEntityNum2 = hand;
	entity->s.time = player->client->ps.persistant[PERS_SPAWN_COUNT];
	entity->s.time2 = int32_t( event.sequence );
	entity->s.apos.trTime = int32_t( player->rewindSpawn );
	entity->s.pos.trType = TR_LINEAR;
	entity->s.pos.trTime = int32_t( event.time );
	entity->r.ownerNum = player->s.number;
	weaponProjectile_t projectile;
	BG_LaunchWeaponProjectile( definition, &event, &player->client->ps, &projectile );
	VectorCopy( projectile.position, entity->s.pos.trBase );
	VectorCopy( projectile.velocity, entity->s.pos.trDelta );
	VectorCopy( entity->s.pos.trBase, entity->r.currentOrigin );
	GameImport_SetEntityReplication( entity->s.number, 3, 0 );
	trap_LinkEntity( entity );
}
void G_RemoveWeaponProjectiles( int owner ) {
	for ( int number = MAX_CLIENTS; number < level.num_entities; ++number ) {
		auto *entity = &g_entities[number];
		if ( entity->inuse && entity->s.eType == ET_MISSILE && entity->s.generic1 == WEAPON_PROJECTILE_TAG && entity->s.otherEntityNum == owner )
			G_FreeEntity( entity );
	}
}
qboolean G_RunWeaponProjectile( gentity_t *entity ) {
	if ( entity->s.generic1 != WEAPON_PROJECTILE_TAG )
		return qfalse;
	const auto *definition = BG_WeaponDefinition( entity->s.modelindex );
	if ( !definition )
		G_Error( "Weapon rejected: projectile definition" );
	weaponProjectile_t state;
	VectorCopy( entity->s.pos.trBase, state.position );
	VectorCopy( entity->s.pos.trDelta, state.velocity );
	state.ageMs = uint32_t( entity->s.pos.trDuration );
	uint32_t clock = uint32_t( entity->s.pos.trTime );
	for ( int step = 0; step < 50 && int32_t( uint32_t( level.time ) - clock ) >= 20; ++step ) {
		trace_t impact;
		const auto result = BG_WeaponProjectileStep( definition, &state, entity->r.ownerNum, trap_Trace, &impact );
		clock += 20;
		if ( result == WEAPON_EXPLODED ) {
			auto *owner = &g_entities[entity->s.otherEntityNum];
			gentity_t *direct = impact.fraction < 1 && impact.entityNum < ENTITYNUM_WORLD ? &g_entities[impact.entityNum] : nullptr;
			if ( direct && direct->takedamage )
				G_Damage( direct, entity, owner, state.velocity, state.position, int( definition->damage ), 0, MOD_GRENADE );
			if ( definition->projectile.radius > 0 )
				G_RadiusDamage( state.position, owner, definition->damage, definition->projectile.radius, direct, MOD_GRENADE_SPLASH );
			if ( auto *effect = WeaponEffect( state.position, EV_WEAPON_IMPACT ) ) {
				effect->s.modelindex = entity->s.modelindex;
				effect->s.otherEntityNum = entity->s.otherEntityNum;
				effect->s.otherEntityNum2 = impact.entityNum;
				effect->s.generic1 = 1; // Detonation uses the same data material with a short sprite lifetime.
				vec3_t normal = { 0, 0, 1 };
				effect->s.eventParm = DirToByte( normal );
			}
			if ( weaponTrace.integer )
				G_Printf( "Weapon projectile exploded: owner=%d sequence=%u age=%u\n", entity->s.otherEntityNum, uint32_t( entity->s.time2 ), state.ageMs );
			G_FreeEntity( entity );
			return qtrue;
		}
	}
	VectorCopy( state.position, entity->s.pos.trBase );
	VectorCopy( state.velocity, entity->s.pos.trDelta );
	VectorCopy( state.position, entity->r.currentOrigin );
	entity->s.pos.trTime = int32_t( clock );
	entity->s.pos.trDuration = int32_t( state.ageMs );
	trap_LinkEntity( entity );
	if ( weaponTrace.integer )
		G_Printf( "Weapon projectile server: owner=%d hand=%d sequence=%u age=%u\n", entity->s.otherEntityNum,
			entity->s.otherEntityNum2, uint32_t( entity->s.time2 ), state.ageMs );
	return qtrue;
}
void G_WeaponCommand( gentity_t *player, const usercmd_t *cmd, int commandStart ) {
	if ( !BG_WeaponDefinition( 0 ) )
		return;
	const int owner = player->s.number;
	auto &actor = weaponActors[owner];
	const auto &ps = player->client->ps;
	const bool reset = !actor.active || actor.spawn != ps.persistant[PERS_SPAWN_COUNT] || actor.epoch != player->rewindSpawn;
	if ( reset ) {
		if ( actor.active && actor.epoch != player->rewindSpawn )
			G_RemoveWeaponProjectiles( owner );
		G_ClearWeaponActor( owner );
		actor.active = true;
		actor.spawn = ps.persistant[PERS_SPAWN_COUNT];
		actor.epoch = player->rewindSpawn;
		for ( int hand = 0; hand < 2; ++hand ) {
			actor.configured[hand] = *BG_WeaponDefinition( 0 );
			for ( int index = 0; const auto *definition = BG_WeaponDefinition( index ); ++index )
				Weapon_Reset( definition, uint32_t( owner ) * 69069u + uint32_t( actor.spawn ) * 31u + uint32_t( hand ) + uint32_t( index ) * 997u,
					uint32_t( commandStart ), &actor.inventory[hand][index] );

			Anim_Reset( BG_WeaponAnimation( 0 ), uint32_t( commandStart ), &actor.animation[hand] );
			Anim_DefaultParameters( BG_WeaponAnimation( 0 ), actor.parameters[hand] );
		}
	}
	trap_Cvar_Update( &weaponTrace );
	if ( weaponTrace.integer && reset )
		G_Printf( "Weapon actor: owner=%d spawn=%d epoch=%u state=%d,%d animation=%d,%d\n", owner, actor.spawn, actor.epoch,
			actor.entity[0]->s.number, actor.entity[1]->s.number, actor.animationEntity[0]->s.number, actor.animationEntity[1]->s.number );
	for ( int hand = 0; hand < 2; ++hand ) {
		int &selected = actor.selected[hand];
		const auto *definition = &actor.configured[hand];
		const int requested = int( cmd->weapon ) - 1;
		weaponDef_t requestedDefinition;
		if ( requested != selected && BG_WeaponDefinition( requested ) && ps.stats[STAT_HEALTH] > 0 &&
			 Weapon_Configure( BG_WeaponDefinition( requested ), actor.attachments[hand][requested], &requestedDefinition ) &&
			 Weapon_Switch( definition, &actor.inventory[hand][selected], &requestedDefinition,
				 &actor.inventory[hand][requested], uint32_t( cmd->serverTime ) ) ) {
			if ( weaponTrace.integer )
				G_Printf( "Weapon switch: owner=%d hand=%d from=%d to=%d magazine=%u\n", owner, hand, selected, requested, actor.inventory[hand][requested].magazine );
			selected = requested;
			actor.configured[hand] = requestedDefinition;
			const uint32_t sequence = actor.animation[hand].eventSequence;
			Anim_Reset( BG_WeaponAnimation( selected ), uint32_t( cmd->serverTime ), &actor.animation[hand] );
			actor.animation[hand].eventSequence = sequence;
			Anim_DefaultParameters( BG_WeaponAnimation( selected ), actor.parameters[hand] );
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
			actor.animation[hand].entered += gap;
			actor.animation[hand].previousEntered += gap;
			actor.animation[hand].blendStarted += gap;
			actor.animation[hand].lastTime += gap;
		}
		for ( int step = 0; step < 50 && int32_t( uint32_t( cmd->serverTime ) - state.time ) >= 20; ++step ) {
			weaponEvents_t events;
			animEvents_t notifies;
			uint32_t tickButtons = buttons;
			if ( definition->ballistics == WEAPON_PROJECTILE && !WeaponProjectileAvailable() ) {
				tickButtons &= ~WEAPON_FIRE;
				state.burstRemaining = 0;
			}
			if ( !Weapon_Tick( definition, tickButtons, state.time + 20, &state, &events ) ||
				 !BG_WeaponAnimationStep( BG_WeaponAnimation( selected ), &state, &events, &actor.animation[hand], actor.parameters[hand], &notifies ) ) {
				G_Error( "Weapon rejected: command animation" );
				return;
			}
			for ( uint32_t i = 0; i < notifies.count; ++i )
				WeaponNotify( player, hand, selected, notifies.items[i] );
			for ( uint32_t i = 0; i < events.count; ++i ) {
				const auto &event = events.items[i];
				if ( event.kind == WEAPON_MELEE_EVENT || ( event.kind == WEAPON_SHOT && definition->ballistics == WEAPON_HITSCAN ) )
					WeaponHit( player, definition, event, selected );
				else if ( event.kind == WEAPON_SHOT )
					SpawnWeaponProjectile( player, hand, selected, definition, event );
				if ( weaponTrace.integer )
					G_Printf( "Weapon event: owner=%d hand=%d kind=%u tick=%u sequence=%u\n", owner, hand, event.kind, event.time, event.sequence );
			}
		}
		auto *animationEntity = actor.animationEntity[hand];
		if ( !BG_WeaponAnimationToEntityState( &actor.animation[hand], actor.parameters[hand], uint32_t( actor.spawn ), owner, hand, selected,
				 actor.attachments[hand][selected], ps.origin, ps.viewangles, &animationEntity->s ) )
			G_Error( "Weapon rejected: animation snapshot" );
		animationEntity->s.constantLight = int32_t( actor.epoch );
		VectorCopy( ps.origin, animationEntity->r.currentOrigin );
		trap_LinkEntity( animationEntity );
		if ( weaponTrace.integer )
			G_Printf( "Weapon animation server: owner=%d hand=%d state=%s tick=%u sequence=%u\n", owner, hand,
				Anim_StateName( BG_WeaponAnimation( selected ), actor.animation[hand].current ), state.time, actor.animation[hand].eventSequence );

		auto *entity = actor.entity[hand];
		if ( !BG_WeaponToEntityState( &state, uint32_t( actor.spawn ), owner, hand, selected, actor.attachments[hand][selected], ps.origin, &entity->s ) )
			G_Error( "Weapon rejected: snapshot" );
		entity->s.constantLight = int32_t( actor.epoch );
		entity->s.generic1 = definition->ballistics == WEAPON_PROJECTILE && !WeaponProjectileAvailable() ? 1 : 0;
		VectorCopy( ps.origin, entity->r.currentOrigin );
		trap_LinkEntity( entity );
		if ( weaponTrace.integer ) {
			G_Printf( "Weapon server state: owner=%d hand=%d tick=%u sequence=%u magazine=%u reserve=%u chamber=%u ads=%u\n", owner, hand,
				state.time, state.sequence, state.magazine, state.reserve, state.chamber, state.adsQ16 );
			uint8_t digest[32];
			char hash[65];
			Weapon_StateHash( &state, digest );
			Anim_HashString( digest, hash );
			G_Printf( "Weapon server digest: owner=%d hand=%d tick=%u hash=%s\n", owner, hand, state.time, hash );
		}
	}
}

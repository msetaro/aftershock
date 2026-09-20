#include "q_shared.h"
#include "../../engine/public/bg_public.h"

static weaponDef_t weaponDefinitions[WEAPON_MAX_DEFINITIONS];
static int weaponDefinitionCount;
void BG_ClearWeapons( void ) {
	memset( weaponDefinitions, 0, sizeof( weaponDefinitions ) );
	weaponDefinitionCount = 0;
}
bool BG_LoadWeapon( int index, const char *path, char hash[65] ) {
	uint8_t digest[32];
	if ( index != weaponDefinitionCount || index < 0 || index >= int( WEAPON_MAX_DEFINITIONS ) ||
		 !Weapon_LoadFile( path, &weaponDefinitions[index], digest ) )
		return false;
	Anim_HashString( digest, hash );
	++weaponDefinitionCount;
	return true;
}
const weaponDef_t *BG_WeaponDefinition( int index ) {
	return index >= 0 && index < weaponDefinitionCount ? &weaponDefinitions[index] : nullptr;
}
uint32_t BG_WeaponButtons( const usercmd_t *cmd, int hand, const playerState_t *ps ) {
	if ( ps->stats[STAT_HEALTH] <= 0 || ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ||
		 ( ps->pm_flags & PMF_RESPAWNED ) || ( cmd->buttons & BUTTON_TALK ) )
		return 0;
	return ( cmd->buttons & ( hand ? 32768 : BUTTON_ATTACK ) ? WEAPON_FIRE : 0u ) |
		   ( cmd->buttons & 4096 ? WEAPON_ADS : 0u ) |
		   ( cmd->buttons & 8192 ? WEAPON_RELOAD : 0u ) |
		   ( !hand && ( cmd->buttons & 16384 ) ? WEAPON_MELEE : 0u );
}

void BG_LaunchWeaponProjectile( const weaponDef_t *definition, const weaponEvent_t *event, const playerState_t *player, weaponProjectile_t *projectile ) {
	*projectile = {};
	vec3_t angles, direction;
	VectorCopy( player->viewangles, angles );
	angles[PITCH] += event->spread[0];
	angles[YAW] += event->spread[1];
	AngleVectors( angles, direction, nullptr, nullptr );
	VectorCopy( player->origin, projectile->position );
	projectile->position[2] += player->viewheight;
	VectorScale( direction, definition->projectile.speed, projectile->velocity );
}
weaponFlight_t BG_WeaponProjectileStep( const weaponDef_t *definition, weaponProjectile_t *projectile, int owner,
	void ( *trace )( trace_t *, const vec3_t, const vec3_t, const vec3_t, const vec3_t, int, int ), trace_t *impact ) {
	*impact = {};
	impact->fraction = 1;
	if ( projectile->ageMs >= definition->projectile.fuseMs )
		return WEAPON_EXPLODED;
	vec3_t start, mins, maxs;
	VectorCopy( projectile->position, start );
	for ( int axis = 0; axis < 3; ++axis ) {
		mins[axis] = -definition->projectile.size;
		maxs[axis] = definition->projectile.size;
	}
	const bool alive = Weapon_ProjectileStep( definition, projectile );
	trace( impact, start, mins, maxs, projectile->position, owner, MASK_SHOT );
	if ( impact->fraction < 1 || impact->startsolid || impact->allsolid ) {
		VectorCopy( impact->endpos, projectile->position );
		if ( !alive || definition->projectile.bounce == 0 || impact->startsolid || impact->allsolid )
			return WEAPON_EXPLODED;
		Weapon_ProjectileBounce( definition, impact->plane.normal, projectile );
		VectorMA( projectile->position, 0.03125f, impact->plane.normal, projectile->position );
		return WEAPON_BOUNCED;
	}
	return alive ? WEAPON_FLYING : WEAPON_EXPLODED;
}

// Six full-width integer fields plus eighteen exact 16-bit float values carry
// the state and spawn counter. No bit-punned NaNs or legacy layout changes.
bool BG_WeaponToEntityState( const weaponState_t *state, uint32_t spawn, int owner, int hand, int definition,
	uint32_t attachments, const float *origin, entityState_t *entity ) {
	if ( !Weapon_StateValid( state ) || owner < 0 || owner >= MAX_CLIENTS || hand < 0 || hand > 1 ||
		 definition < 0 || definition >= int( WEAPON_MAX_DEFINITIONS ) || attachments > 255 )
		return false;
	for ( int axis = 0; axis < 3; ++axis )
		if ( !isfinite( origin[axis] ) )
			return false;
	const int number = entity->number;
	*entity = {};
	entity->number = number;
	entity->eType = ET_WEAPON_STATE;
	entity->clientNum = 1;
	entity->otherEntityNum = owner;
	entity->otherEntityNum2 = hand;
	entity->modelindex = definition;
	entity->modelindex2 = int( attachments );
	uint32_t words[15];
	static_assert( sizeof( weaponState_t ) == 14 * sizeof( uint32_t ) );
	memcpy( words, state, sizeof( *state ) );
	words[14] = spawn;
	int32_t *integers[] = { &entity->time, &entity->time2, &entity->pos.trTime, &entity->pos.trDuration, &entity->apos.trTime, &entity->apos.trDuration };
	for ( int i = 0; i < 6; ++i )
		*integers[i] = int32_t( words[i] );
	float *groups[] = { entity->pos.trBase, entity->pos.trDelta, entity->apos.trBase, entity->apos.trDelta, entity->origin2, entity->angles2 };
	for ( int i = 0; i < 18; ++i )
		groups[i / 3][i % 3] = float( ( words[6 + i / 2] >> ( ( i % 2 ) * 16 ) ) & 65535u );
	for ( int axis = 0; axis < 3; ++axis )
		entity->origin[axis] = origin[axis] == 0 ? 0 : origin[axis];
	return true;
}
bool BG_EntityStateToWeapon( const entityState_t *entity, weaponState_t *state, uint32_t *spawn ) {
	if ( entity->eType != ET_WEAPON_STATE || entity->clientNum != 1 || entity->otherEntityNum < 0 || entity->otherEntityNum >= MAX_CLIENTS ||
		 entity->otherEntityNum2 < 0 || entity->otherEntityNum2 > 1 || entity->modelindex < 0 || entity->modelindex >= int( WEAPON_MAX_DEFINITIONS ) ||
		 entity->modelindex2 < 0 || entity->modelindex2 > 255 )
		return false;
	for ( int axis = 0; axis < 3; ++axis )
		if ( !isfinite( entity->origin[axis] ) )
			return false;
	uint32_t words[15] = {};
	const int32_t integers[] = { entity->time, entity->time2, entity->pos.trTime, entity->pos.trDuration, entity->apos.trTime, entity->apos.trDuration };
	for ( int i = 0; i < 6; ++i )
		words[i] = uint32_t( integers[i] );
	const float *groups[] = { entity->pos.trBase, entity->pos.trDelta, entity->apos.trBase, entity->apos.trDelta, entity->origin2, entity->angles2 };
	for ( int i = 0; i < 18; ++i ) {
		const float value = groups[i / 3][i % 3];
		if ( !isfinite( value ) || value < 0 || value > 65535 || value != floorf( value ) )
			return false;
		words[6 + i / 2] |= uint32_t( value ) << ( ( i % 2 ) * 16 );
	}
	weaponState_t restored;
	memcpy( &restored, words, sizeof( restored ) );
	if ( !Weapon_StateValid( &restored ) )
		return false;
	*state = restored;
	*spawn = words[14];
	return true;
}

// Reuse the animation encoding; its two spare float slots hold exact spawn halves.
bool BG_WeaponAnimationToEntityState( const animState_t *state, const float *parameters, uint32_t spawn, int owner, int hand, int definition,
	uint32_t attachments, const float *origin, const float *angles, entityState_t *entity ) {
	static_assert( ANIM_MAX_PARAMETERS == 16 );
	if ( definition < 0 || definition >= int( WEAPON_MAX_DEFINITIONS ) || attachments > 255 ||
		 !BG_AnimationToEntityState( state, parameters, owner, hand, origin, angles, entity ) )
		return false;
	entity->eType = ET_WEAPON_ANIMATION;
	entity->weapon = definition;
	entity->modelindex = int( attachments );
	entity->angles2[1] = float( spawn & 65535u );
	entity->angles2[2] = float( spawn >> 16 );
	return true;
}
bool BG_EntityStateToWeaponAnimation( const entityState_t *entity, animState_t *state, float *parameters, uint32_t *spawn ) {
	if ( entity->eType != ET_WEAPON_ANIMATION || entity->weapon < 0 || entity->weapon >= int( WEAPON_MAX_DEFINITIONS ) ||
		 entity->modelindex < 0 || entity->modelindex > 255 )
		return false;
	for ( int i = 1; i < 3; ++i )
		if ( !isfinite( entity->angles2[i] ) || entity->angles2[i] < 0 || entity->angles2[i] > 65535 || entity->angles2[i] != floorf( entity->angles2[i] ) )
			return false;
	entityState_t animation = *entity;
	animation.eType = ET_ANIMATION;
	if ( !BG_EntityStateToAnimation( &animation, state, parameters ) )
		return false;
	*spawn = uint32_t( entity->angles2[1] ) | ( uint32_t( entity->angles2[2] ) << 16 );
	return true;
}

// Advance old notifies before restarting fire: automatic shots must not lose shells.
bool BG_WeaponAnimationStep( const animAsset_t *asset, const weaponState_t *weapon, const weaponEvents_t *events,
	animState_t *state, float *parameters, animEvents_t *notifies ) {
	notifies->count = 0;
	if ( events->count > ARRAY_LEN( events->items ) )
		return false;
	animState_t next = *state;
	float inputs[ANIM_MAX_PARAMETERS];
	memcpy( inputs, parameters, sizeof( inputs ) );
	const int ads = Anim_ParameterIndex( asset, "ads" );
	if ( ads >= 0 )
		inputs[ads] = float( weapon->adsQ16 ) / 65536;
	animEvents_t pending;
	if ( !Anim_Tick( asset, inputs, weapon->time, &next, &pending ) )
		return false;
	for ( uint32_t i = 0; i < events->count; ++i ) {
		const auto &event = events->items[i];
		if ( event.time != weapon->time )
			return false;
		const char *name = event.kind == WEAPON_SHOT ? "fire" : event.kind == WEAPON_RELOAD_BEGIN	? "reload"
															: event.kind == WEAPON_RELOAD_CANCELLED ? "idle"
															: event.kind == WEAPON_MELEE_EVENT		? "melee"
																									: nullptr;
		if ( !name )
			continue;
		uint32_t index = 0;
		for ( ; index < asset->header.sections[ANIM_STATES].count; ++index )
			if ( !strcmp( Anim_StateName( asset, index ), name ) )
				break;
		if ( index == asset->header.sections[ANIM_STATES].count )
			return false;
		next.previous = next.current;
		next.previousEntered = next.entered;
		next.current = index;
		next.entered = next.blendStarted = next.lastTime = weapon->time;
		next.blendDuration = 40;
		next.initialized = 0;
		animEvents_t entered;
		if ( !Anim_Tick( asset, inputs, weapon->time, &next, &entered ) || entered.count > ANIM_MAX_EVENTS - pending.count )
			return false;
		memcpy( pending.items + pending.count, entered.items, entered.count * sizeof( animEvent_t ) );
		pending.count += entered.count;
	}
	*state = next;
	memcpy( parameters, inputs, sizeof( inputs ) );
	*notifies = pending;
	return true;
}

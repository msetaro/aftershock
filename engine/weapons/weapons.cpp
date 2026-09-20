#include "weapons_public.h"
#include "../../third_party/sha256/sha-256.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

static bool Between( float value, float low, float high ) {
	return std::isfinite( value ) && value >= low && value <= high;
}
static bool Text( const char *value, size_t capacity ) {
	if ( !value[0] || !std::memchr( value, 0, capacity ) || value[0] == '/' || std::strstr( value, ".." ) )
		return false;
	for ( const char *p = value; *p; ++p )
		if ( !( ( *p >= 'a' && *p <= 'z' ) || ( *p >= '0' && *p <= '9' ) || *p == '_' || *p == '/' || *p == '.' || *p == '-' ) )
			return false;
	return true;
}
static bool Valid( const weaponDef_t &d ) {
	if ( !Text( d.name, 64 ) || !Text( d.model, 64 ) || !Text( d.animation, 64 ) || d.fireMode > WEAPON_BURST || d.ballistics > WEAPON_PROJECTILE ||
		 d.intervalMs < 20 || d.intervalMs > 60000 || !d.burstCount || d.burstCount > 32 || d.adsMs < 20 || d.adsMs > 10000 ||
		 !d.magazine || d.magazine > 1000 || d.reserve > 65535 || !d.recoilCount || d.recoilCount > 32 || d.reloadCount != 4 ||
		 !d.materialCount || d.materialCount > 8 || d.attachmentCount > 8 || d.soundCount > 8 || d.switchMs < 20 || d.switchMs > 10000 )
		return false;
	if ( !Between( d.damage, 0, 10000 ) || !Between( d.minimumDamage, 0, d.damage ) || !Between( d.range, 1, 65536 ) ||
		 !Between( d.falloffStart, 0, d.range ) || !Between( d.falloffEnd, d.falloffStart, d.range ) || d.falloffStart == d.falloffEnd ||
		 !Between( d.spreadDegrees, 0, 90 ) || !Between( d.adsSpreadScale, 0, 4 ) || !Between( d.viewKickScale, 0, 4 ) ||
		 !Between( d.adsFov, 1, 179 ) || !Between( d.sway, 0, 10 ) || !Between( d.bob, 0, 10 ) ||
		 !Between( d.projectile.speed, 1, 8192 ) || !Between( d.projectile.gravity, 0, 4096 ) || !Between( d.projectile.bounce, 0, 1 ) ||
		 !Between( d.projectile.radius, 0, 4096 ) || d.projectile.fuseMs < 20 || d.projectile.fuseMs > 60000 ||
		 !Between( d.melee.range, 0, 256 ) || !Between( d.melee.damage, 0, 10000 ) || d.melee.intervalMs < 20 || d.melee.intervalMs > 60000 )
		return false;
	for ( uint32_t i = 0; i < d.recoilCount; ++i )
		if ( !Between( d.recoil[i][0], -90, 90 ) || !Between( d.recoil[i][1], -90, 90 ) )
			return false;
	for ( uint32_t i = 0; i < d.reloadCount; ++i ) {
		const auto &row = d.reload[i];
		if ( row.action != i || row.cancel > 1 || row.timeMs > 10000 || ( i && row.timeMs <= d.reload[i - 1].timeMs ) || !Text( row.event, 64 ) )
			return false;
	}
	if ( d.materials[0].surfaceFlags )
		return false;
	for ( uint32_t i = 0; i < d.materialCount; ++i ) {
		const auto &row = d.materials[i];
		if ( !Text( row.name, 32 ) || !Text( row.effect, 64 ) || !Between( row.depth, 0, 256 ) || !Between( row.damageScale, 0, 1 ) )
			return false;
	}
	for ( uint32_t i = 0; i < d.attachmentCount; ++i ) {
		const auto &row = d.attachments[i];
		if ( !Text( row.name, 32 ) || !Text( row.socket, 32 ) || !Text( row.model, 64 ) || !Between( row.spreadScale, 0, 4 ) ||
			 !Between( row.recoilScale, 0, 4 ) || !Between( row.adsFov, 1, 179 ) )
			return false;
	}
	for ( uint32_t i = 0; i < d.soundCount; ++i )
		if ( !Text( d.sounds[i].event, 32 ) || !Text( d.sounds[i].path, 64 ) )
			return false;
	return true;
}
bool Weapon_Open( const void *data, size_t size, weaponDef_t *definition ) {
	static_assert( std::endian::native == std::endian::little );
	if ( !data || !definition || size != 48 + sizeof( weaponDef_t ) )
		return false;
	const auto *bytes = (const uint8_t *)data;
	uint32_t header[2];
	std::memcpy( header, bytes + 8, sizeof( header ) );
	if ( std::memcmp( bytes, "ASWEAP\0\0", 8 ) || header[0] != 1 || header[1] != sizeof( weaponDef_t ) )
		return false;
	uint8_t hash[32];
	calc_sha_256( hash, bytes + 48, sizeof( weaponDef_t ) );
	if ( std::memcmp( hash, bytes + 16, sizeof( hash ) ) )
		return false;
	weaponDef_t result;
	std::memcpy( &result, bytes + 48, sizeof( result ) );
	if ( !Valid( result ) )
		return false;
	*definition = result;
	return true;
}
bool Weapon_Configure( const weaponDef_t *base, uint32_t attachments, weaponDef_t *configured ) {
	if ( !base || !configured || !Valid( *base ) || attachments >= ( 1u << base->attachmentCount ) )
		return false;
	weaponDef_t result = *base;
	for ( uint32_t i = 0; i < base->attachmentCount; ++i ) {
		if ( !( attachments & ( 1u << i ) ) )
			continue;
		result.spreadDegrees *= base->attachments[i].spreadScale;
		result.adsFov = base->attachments[i].adsFov;
		for ( uint32_t r = 0; r < result.recoilCount; ++r )
			for ( int axis = 0; axis < 2; ++axis )
				result.recoil[r][axis] *= base->attachments[i].recoilScale;
	}
	if ( !Valid( result ) )
		return false;
	*configured = result;
	return true;
}
void Weapon_Reset( const weaponDef_t *d, uint32_t seed, uint32_t time, weaponState_t *state ) {
	*state = {};
	state->time = state->nextFire = state->nextMelee = state->switchUntil = time;
	state->random = seed;
	state->magazine = d->magazine;
	state->reserve = d->reserve;
	state->chamber = 1;
	state->reloadStage = WEAPON_NO_STAGE;
}
static float RandomSigned( uint32_t *seed ) {
	// Same recurrence as Q_rand, with defined unsigned wrap for this new state.
	*seed = 69069u * *seed + 1u;
	return float( *seed & 0xffffu ) * ( 1.0f / 32768.0f ) - 1.0f;
}
bool Weapon_StateValid( const weaponState_t *state ) {
	return state && state->magazine <= 1000 && state->reserve <= 66536 && state->chamber <= 1 && state->adsQ16 <= 65536 &&
		   state->burstRemaining <= 32 && ( state->reloadStage == WEAPON_NO_STAGE || state->reloadStage < WEAPON_MAX_ROWS );
}
static bool StateForDefinition( const weaponDef_t *d, const weaponState_t *state ) {
	return d && Weapon_StateValid( state ) && state->magazine <= d->magazine &&
		   ( state->reloadStage == WEAPON_NO_STAGE || state->reloadStage < d->reloadCount );
}
bool Weapon_Tick( const weaponDef_t *d, uint32_t buttons, uint32_t time, weaponState_t *state, weaponEvents_t *events ) {
	if ( !events || !StateForDefinition( d, state ) || time - state->time != 20 )
		return false;
	*events = {};
	const uint32_t pressed = buttons & ~state->previousButtons;
	state->previousButtons = buttons;
	state->time = time;
	const uint32_t adsStep = ( 65536u * 20u + d->adsMs - 1u ) / d->adsMs;
	state->adsQ16 = buttons & WEAPON_ADS ? std::min( 65536u, state->adsQ16 + adsStep ) : state->adsQ16 - std::min( state->adsQ16, adsStep );
	if ( (int32_t)( time - state->switchUntil ) < 0 )
		return true;
	if ( ( pressed & WEAPON_RELOAD ) && state->reloadStage == WEAPON_NO_STAGE &&
		 ( state->magazine < d->magazine || !state->chamber ) && ( state->reserve || state->magazine ) ) {
		state->reloadStage = 0;
		state->reloadStart = time;
		state->burstRemaining = 0;
	}
	if ( ( buttons & WEAPON_CANCEL ) && state->reloadStage != WEAPON_NO_STAGE &&
		 ( !state->reloadStage || d->reload[state->reloadStage - 1].cancel ) )
		state->reloadStage = WEAPON_NO_STAGE;
	while ( state->reloadStage != WEAPON_NO_STAGE && time - state->reloadStart >= d->reload[state->reloadStage].timeMs ) {
		const uint32_t stage = state->reloadStage++;
		auto &event = events->items[events->count++];
		event.time = time;
		event.kind = WEAPON_RELOAD_EVENT;
		event.stage = stage;
		switch ( d->reload[stage].action ) {
		case WEAPON_EJECT:
			state->reserve += state->magazine;
			state->magazine = 0;
			break;
		case WEAPON_INSERT:
			state->magazine = std::min( state->reserve, d->magazine );
			state->reserve -= state->magazine;
			break;
		case WEAPON_CHAMBER:
			if ( !state->chamber && state->magazine ) {
				--state->magazine;
				state->chamber = 1;
			}
			break;
		case WEAPON_FINISH:
			state->reloadStage = WEAPON_NO_STAGE;
			break;
		}
	}
	if ( state->reloadStage != WEAPON_NO_STAGE )
		return true;
	if ( ( buttons & WEAPON_MELEE ) && (int32_t)( time - state->nextMelee ) >= 0 ) {
		state->nextMelee = state->nextFire = ( time - state->nextMelee < 20u ? state->nextMelee : time ) + d->melee.intervalMs;
		auto &event = events->items[events->count++];
		event.time = time;
		event.kind = WEAPON_MELEE_EVENT;
		return true;
	}
	const bool ready = (int32_t)( time - state->nextFire ) >= 0;
	if ( d->fireMode == WEAPON_BURST && ready && ( pressed & WEAPON_FIRE ) && !state->burstRemaining )
		state->burstRemaining = d->burstCount;
	const bool trigger = d->fireMode == WEAPON_AUTO ? ( buttons & WEAPON_FIRE ) != 0 : d->fireMode == WEAPON_SEMI ? ( pressed & WEAPON_FIRE ) != 0
																												  : state->burstRemaining != 0;
	if ( !ready || !trigger )
		return true;
	// Carry a sub-tick remainder, but never catch up missed shots after idle/reload.
	state->nextFire = ( time - state->nextFire < 20u ? state->nextFire : time ) + d->intervalMs;
	auto &event = events->items[events->count++];
	event.time = time;
	if ( !state->chamber ) {
		event.kind = WEAPON_DRY;
		state->burstRemaining = 0;
		return true;
	}
	event.kind = WEAPON_SHOT;
	const float spread = d->spreadDegrees * ( 1.0f + float( state->adsQ16 ) * ( 1.0f / 65536.0f ) * ( d->adsSpreadScale - 1.0f ) );
	for ( int axis = 0; axis < 2; ++axis ) {
		event.spread[axis] = RandomSigned( &state->random ) * spread;
		event.recoil[axis] = d->recoil[state->sequence % d->recoilCount][axis];
	}
	event.sequence = ++state->sequence;
	state->chamber = 0;
	if ( state->magazine ) {
		--state->magazine;
		state->chamber = 1;
	}
	if ( state->burstRemaining )
		--state->burstRemaining;
	return true;
}
bool Weapon_Command( const weaponDef_t *d, uint32_t buttons, uint32_t time, weaponState_t *state, weaponEvents_t *events ) {
	if ( !events || !StateForDefinition( d, state ) )
		return false;
	const int32_t elapsed = (int32_t)( time - state->time );
	if ( elapsed > 1000 )
		return false;
	weaponState_t result = *state;
	weaponEvents_t collected = {};
	for ( int32_t remaining = elapsed; remaining >= 20; remaining -= 20 ) {
		weaponEvents_t tick;
		if ( !Weapon_Tick( d, buttons, result.time + 20, &result, &tick ) || collected.count + tick.count > 64 )
			return false;
		for ( uint32_t i = 0; i < tick.count; ++i )
			collected.items[collected.count++] = tick.items[i];
	}
	*state = result;
	*events = collected;
	return true;
}
float Weapon_Damage( const weaponDef_t *d, float distance ) {
	if ( !std::isfinite( distance ) || distance < 0 || distance > d->range )
		return 0;
	const float fraction = std::clamp( ( distance - d->falloffStart ) / ( d->falloffEnd - d->falloffStart ), 0.0f, 1.0f );
	return d->damage + fraction * ( d->minimumDamage - d->damage );
}
float Weapon_PenetrationDamage( const weaponDef_t *d, uint32_t surfaceFlags, float thickness, float damage ) {
	const weaponMaterial_t *material = &d->materials[0];
	for ( uint32_t i = 1; i < d->materialCount; ++i )
		if ( d->materials[i].surfaceFlags && ( surfaceFlags & d->materials[i].surfaceFlags ) == d->materials[i].surfaceFlags ) {
			material = &d->materials[i];
			break;
		}
	return Between( thickness, 0, material->depth ) && Between( damage, 0, 10000 ) ? damage * material->damageScale : 0;
}
bool Weapon_ProjectileStep( const weaponDef_t *d, weaponProjectile_t *projectile ) {
	if ( projectile->ageMs >= d->projectile.fuseMs )
		return false;
	for ( int axis = 0; axis < 3; ++axis )
		projectile->position[axis] += projectile->velocity[axis] * 0.02f;
	projectile->position[2] -= 0.5f * d->projectile.gravity * 0.02f * 0.02f;
	projectile->velocity[2] -= d->projectile.gravity * 0.02f;
	projectile->ageMs += 20;
	return projectile->ageMs < d->projectile.fuseMs;
}
void Weapon_ProjectileBounce( const weaponDef_t *d, const float normal[3], weaponProjectile_t *projectile ) {
	float dot = 0;
	for ( int axis = 0; axis < 3; ++axis )
		dot += projectile->velocity[axis] * normal[axis];
	for ( int axis = 0; axis < 3; ++axis )
		projectile->velocity[axis] = ( projectile->velocity[axis] - 2.0f * dot * normal[axis] ) * d->projectile.bounce;
}

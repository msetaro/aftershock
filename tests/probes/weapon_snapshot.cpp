// Weapon state uses an auxiliary entity without changing legacy wire layouts.
#include "../../engine/qcommon/q_shared.h"
#include "../../engine/qcommon/qcommon_public.h"
#include "../../engine/weapons/weapons_public.h"
#include "../../game/bg/bg_weapons.cpp"
#include "../../game/bg/bg_animation.cpp"
#include <assert.h>

cvar_t *cl_shownet;
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}
void QDECL Com_Printf( const char *, ... ) {
}
void QDECL Com_DPrintf( const char *, ... ) {
}

static bool projectileWall;
static void ProjectileTrace( trace_t *trace, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int owner, int mask ) {
	assert( owner == 3 && mask == MASK_SHOT && mins[0] == -2 && maxs[2] == 2 );
	*trace = {};
	trace->fraction = projectileWall ? 0.5f : 1.0f;
	trace->entityNum = projectileWall ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	for ( int axis = 0; axis < 3; ++axis )
		trace->endpos[axis] = start[axis] + trace->fraction * ( end[axis] - start[axis] );
	trace->plane.normal[0] = -1;
}
static void Projectiles() {
	weaponDef_t definition = {};
	definition.projectile.speed = 800;
	definition.projectile.gravity = 0;
	definition.projectile.bounce = 0.5f;
	definition.projectile.size = 2;
	definition.projectile.fuseMs = 60;
	weaponProjectile_t state = {};
	state.velocity[0] = 800;
	playerState_t player = {};
	VectorSet( player.origin, 1, 2, 3 );
	player.viewheight = 26;
	weaponEvent_t shot = {};
	weaponProjectile_t launched;
	BG_LaunchWeaponProjectile( &definition, &shot, &player, &launched );
	assert( launched.position[0] == 1 && launched.position[1] == 2 && launched.position[2] == 29 );
	assert( launched.velocity[0] == 800 && launched.velocity[1] == 0 && launched.velocity[2] == 0 && launched.ageMs == 0 );
	trace_t impact;
	assert( BG_WeaponProjectileStep( &definition, &state, 3, ProjectileTrace, &impact ) == WEAPON_FLYING );
	assert( state.position[0] == 16 && state.ageMs == 20 );
	projectileWall = true;
	assert( BG_WeaponProjectileStep( &definition, &state, 3, ProjectileTrace, &impact ) == WEAPON_BOUNCED );
	assert( state.position[0] < 24 && state.position[0] >= 23.875f && state.velocity[0] == -400 );
	projectileWall = false;
	assert( BG_WeaponProjectileStep( &definition, &state, 3, ProjectileTrace, &impact ) == WEAPON_EXPLODED );
	assert( state.ageMs == 60 );
	state = {};
	state.velocity[0] = 800;
	projectileWall = true;
	definition.projectile.bounce = 0;
	assert( BG_WeaponProjectileStep( &definition, &state, 3, ProjectileTrace, &impact ) == WEAPON_EXPLODED );
	assert( state.position[0] == 8 && state.ageMs == 20 );
}
int main() {
	Projectiles();
	const weaponState_t state = { 0xfffffff0u, 0x7fc00000u, 0xfedcba98u, 64, 29, 90, 1, WEAPON_FIRE | WEAPON_ADS,
		WEAPON_NO_STAGE, 0x80000000u, 3, 32768, 1000, 2000 };
	const float origin[3] = { 123.5f, -0.25f, 32 };
	const uint32_t spawn = 0x89abcdefu;
	entityState_t baseline = {}, encoded = {}, decoded = {};
	encoded.number = 127;
	assert( BG_WeaponToEntityState( &state, spawn, 63, 1, 31, 255, origin, &encoded ) );
	assert( encoded.eType == ET_WEAPON_STATE && encoded.solid == 0 && encoded.loopSound == 0 && encoded.event == 0 );
	byte storage[4096] = {};
	msg_t msg;
	MSG_Init( &msg, storage, sizeof( storage ) );
	MSG_WriteDeltaEntity( &msg, &baseline, &encoded, qtrue );
	MSG_BeginReading( &msg );
	const int number = MSG_ReadEntitynum( &msg );
	MSG_ReadDeltaEntity( &msg, &baseline, &decoded, number );
	weaponState_t restored;
	uint32_t restoredSpawn;
	assert( BG_EntityStateToWeapon( &decoded, &restored, &restoredSpawn ) );
	assert( !memcmp( &state, &restored, sizeof( state ) ) && spawn == restoredSpawn );
	assert( decoded.otherEntityNum == 63 && decoded.otherEntityNum2 == 1 && decoded.modelindex == 31 && decoded.modelindex2 == 255 );
	assert( !memcmp( origin, decoded.origin, sizeof( origin ) ) );
	const int initialBytes = msg.cursize;
	baseline = decoded;
	weaponState_t changed = state;
	changed.time += 20;
	assert( BG_WeaponToEntityState( &changed, spawn, 63, 1, 31, 255, origin, &encoded ) );
	MSG_Clear( &msg );
	MSG_WriteDeltaEntity( &msg, &baseline, &encoded, qtrue );
	assert( msg.cursize < initialBytes );
	MSG_BeginReading( &msg );
	MSG_ReadDeltaEntity( &msg, &baseline, &decoded, MSG_ReadEntitynum( &msg ) );
	assert( BG_EntityStateToWeapon( &decoded, &restored, &restoredSpawn ) );
	assert( !memcmp( &changed, &restored, sizeof( changed ) ) && restoredSpawn == spawn );
	const int clockDeltaBytes = msg.cursize;
	baseline = decoded;
	encoded.generic1 = 1; // Authoritative projectile capacity blocks prediction without changing weapon layout.
	MSG_Clear( &msg );
	MSG_WriteDeltaEntity( &msg, &baseline, &encoded, qtrue );
	MSG_BeginReading( &msg );
	MSG_ReadDeltaEntity( &msg, &baseline, &decoded, MSG_ReadEntitynum( &msg ) );
	assert( decoded.generic1 == 1 && BG_EntityStateToWeapon( &decoded, &restored, &restoredSpawn ) );
	assert( !memcmp( &changed, &restored, sizeof( changed ) ) );
	const animState_t animation = { 1, 2, 0xfffffff0u, 123, 456, 100, 789, 0xabcdef01u, 1 };
	float parameters[ANIM_MAX_PARAMETERS];
	for ( uint32_t i = 0; i < ANIM_MAX_PARAMETERS; ++i )
		parameters[i] = float( i ) * 0.25f;
	const float angles[3] = { 5, 90, 0 };
	assert( BG_WeaponAnimationToEntityState( &animation, parameters, spawn, 63, 1, 31, 255, origin, angles, &encoded ) );
	assert( encoded.eType == ET_WEAPON_ANIMATION && encoded.solid == 0 && encoded.event == 0 && encoded.loopSound == 0 );
	baseline = {};
	MSG_Clear( &msg );
	MSG_WriteDeltaEntity( &msg, &baseline, &encoded, qtrue );
	MSG_BeginReading( &msg );
	MSG_ReadDeltaEntity( &msg, &baseline, &decoded, MSG_ReadEntitynum( &msg ) );
	animState_t restoredAnimation;
	float restoredParameters[ANIM_MAX_PARAMETERS];
	assert( BG_EntityStateToWeaponAnimation( &decoded, &restoredAnimation, restoredParameters, &restoredSpawn ) );
	assert( !memcmp( &animation, &restoredAnimation, sizeof( animation ) ) && restoredSpawn == spawn );
	assert( !memcmp( parameters, restoredParameters, sizeof( parameters ) ) );
	assert( decoded.weapon == 31 && decoded.modelindex == 255 && decoded.otherEntityNum == 63 && decoded.otherEntityNum2 == 1 );
	puts( "PASS: per-hand weapon animation, parameters and full spawn/notify counters survive entity codec" );
	printf( "PASS: full weapon state/seed/clock/spawn and both-hand metadata survive entity deltas (%d initial, %d delta bytes)\n", initialBytes, clockDeltaBytes );
}

// Exercise actual game damage and bounded world penetration with analytic planes.
#include "../../engine/animation/animation_public.h"
#include "../../engine/qcommon/net_history_public.h"
#include "../../engine/public/g_native_public.h"
#include "../../engine/public/dev_public.h"
#define COM_TRAP_GETVALUE 700
#include "../../game/game/g_data_weapons.cpp"
#include <assert.h>

level_locals_t level;
gentity_t g_entities[MAX_GENTITIES];
static gclient_t shooter;
static float thickness;
static int flags, damage, traces, effects, layers = 1;
void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t, vec3_t ) {
	assert( angles[0] == 0 && angles[1] == 0 );
	VectorSet( forward, 1, 0, 0 );
}
void G_TraceHitscanAtTime( trace_t *trace, const vec3_t start, const vec3_t end, int pass, const gentity_t *player, uint32_t time ) {
	assert( pass == 0 && player == &g_entities[0] && time == 123 );
	++traces;
	*trace = {};
	trace->fraction = 1;
	trace->entityNum = ENTITYNUM_NONE;
	VectorCopy( end, trace->endpos );
	for ( int i = 0; thickness > 0 && i < layers; ++i ) {
		const float entry = 25 + float( i ) * 10;
		if ( start[0] < entry && end[0] > entry ) {
			trace->fraction = ( entry - start[0] ) / ( end[0] - start[0] );
			VectorSet( trace->endpos, entry, 0, 0 );
			trace->entityNum = ENTITYNUM_WORLD;
			trace->surfaceFlags = flags;
			return;
		}
	}
	if ( start[0] < 50 && end[0] >= 50 ) {
		trace->fraction = ( 50 - start[0] ) / ( end[0] - start[0] );
		VectorSet( trace->endpos, 50, 0, 0 );
		trace->entityNum = 7;
	}
}
int trap_PointContents( const vec3_t point, int pass ) {
	assert( pass == 0 );
	for ( int i = 0; i < layers; ++i )
		if ( point[0] >= 25 + float( i ) * 10 && point[0] < 25 + float( i ) * 10 + thickness )
			return CONTENTS_SOLID;
	return 0;
}
void trap_Trace( trace_t *trace, const vec3_t start, const vec3_t, const vec3_t, const vec3_t end, int pass, int mask ) {
	assert( pass == 0 && mask == MASK_SOLID && start[0] > end[0] );
	*trace = {};
	trace->fraction = 1;
	for ( int i = 0; i < layers; ++i ) {
		const float exit = 25 + float( i ) * 10 + thickness;
		if ( start[0] >= exit && end[0] < exit ) {
			trace->fraction = ( start[0] - exit ) / ( start[0] - end[0] );
			VectorSet( trace->endpos, exit, 0, 0 );
			return;
		}
	}
}
int DirToByte( vec3_t ) {
	return 0;
}
gentity_t *G_TempEntity( vec3_t, int event ) {
	assert( event == EV_WEAPON_IMPACT );
	++effects;
	static gentity_t effect;
	return &effect;
}
void G_Damage( gentity_t *target, gentity_t *, gentity_t *, vec3_t, vec3_t, int amount, int, int ) {
	assert( target == &g_entities[7] );
	damage += amount;
}
void QDECL G_Printf( const char *, ... ) {
}
int main() {
	g_entities[0].client = &shooter;
	g_entities[7].takedamage = qtrue;
	weaponDef_t definition = {};
	definition.damage = 40;
	definition.minimumDamage = 20;
	definition.falloffStart = 512;
	definition.falloffEnd = 2048;
	definition.range = 4096;
	definition.melee = { 64, 50, 500 };
	definition.materialCount = 2;
	definition.materials[0].depth = 8;
	definition.materials[0].damageScale = 0.5f;
	definition.materials[1].surfaceFlags = 4096;
	definition.materials[1].depth = 2;
	definition.materials[1].damageScale = 0.25f;
	weaponEvent_t event = {};
	event.time = 123;
	WeaponHit( &g_entities[0], &definition, event, 0 );
	assert( damage == 40 && traces == 1 );
	damage = traces = 0;
	thickness = 1;
	WeaponHit( &g_entities[0], &definition, event, 0 );
	assert( damage == 20 && traces == 2 );
	damage = 0;
	flags = 4096;
	WeaponHit( &g_entities[0], &definition, event, 0 );
	assert( damage == 10 );
	damage = 0;
	thickness = 3;
	WeaponHit( &g_entities[0], &definition, event, 0 );
	assert( damage == 0 );
	flags = 0;
	thickness = 1;
	layers = 2;
	WeaponHit( &g_entities[0], &definition, event, 0 );
	assert( damage == 10 );
	damage = 0;
	flags = SURF_NOIMPACT;
	WeaponHit( &g_entities[0], &definition, event, 0 );
	assert( damage == 0 );
	flags = 0;
	event.kind = WEAPON_MELEE_EVENT;
	WeaponHit( &g_entities[0], &definition, event, 0 );
	assert( damage == 0 ); // Melee cannot penetrate a wall.
	thickness = 0;
	WeaponHit( &g_entities[0], &definition, event, 0 );
	assert( damage == 50 );
	damage = effects = 0;
	event.kind = WEAPON_SHOT;
	level.num_entities = MAX_CLIENTS + 128;
	for ( int number = MAX_CLIENTS; number < level.num_entities; ++number ) {
		g_entities[number].inuse = qtrue;
		g_entities[number].classname = "weapon_effect";
	}
	WeaponHit( &g_entities[0], &definition, event, 0 );
	assert( damage == 40 && effects == 0 ); // Cosmetic pressure never discards damage.
	level.num_entities = MAX_CLIENTS + 64;
	for ( int number = MAX_CLIENTS; number < level.num_entities; ++number ) {
		g_entities[number].s.eType = ET_MISSILE;
		g_entities[number].s.generic1 = WEAPON_PROJECTILE_TAG;
	}
	assert( !WeaponProjectileAvailable() );
	g_entities[MAX_CLIENTS].inuse = qfalse;
	assert( WeaponProjectileAvailable() );
	level.num_entities = ENTITYNUM_MAX_NORMAL - 8;
	assert( !WeaponProjectileAvailable() );
	puts( "PASS: data hitscan preserves shot time, material depth/loss, layered walls and melee limits" );
}

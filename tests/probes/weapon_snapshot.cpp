// Weapon state uses an auxiliary entity without changing legacy wire layouts.
#include "../../engine/qcommon/q_shared.h"
#include "../../engine/qcommon/qcommon_public.h"
#include "../../engine/weapons/weapons_public.h"
#include "../../game/bg/bg_weapons.cpp"
#include <assert.h>

cvar_t *cl_shownet;
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}
void QDECL Com_Printf( const char *, ... ) {
}
void QDECL Com_DPrintf( const char *, ... ) {
}

int main() {
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
	printf( "PASS: full weapon state/seed/clock/spawn and both-hand metadata survive entity deltas (%d initial, %d delta bytes)\n", initialBytes, msg.cursize );
}

// Round-trip animation state through the production entity delta codec.
#include "../../engine/qcommon/q_shared.h"
#include "../../engine/qcommon/qcommon_public.h"
#include "../../engine/animation/animation_public.h"
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

int main() {
	const animState_t state = { 63, 62, 0xfffffff0u, 0xffffffe0u, 0xfffffff8u, 150, 24, 0xfedcba98u, 1 };
	float parameters[ANIM_MAX_PARAMETERS];
	for ( uint32_t i = 0; i < ANIM_MAX_PARAMETERS; ++i )
		parameters[i] = float( i ) * 0.125f - 0.25f;
	const float origin[3] = { 123.5f, -0.25f, 32 }, angles[3] = { 12.25f, 90, -1.5f };
	entityState_t baseline = {}, encoded = {}, decoded = {};
	encoded.number = 127;
	assert( BG_AnimationToEntityState( &state, parameters, 63, 1, origin, angles, &encoded ) );
	assert( encoded.eType == ET_ANIMATION && encoded.solid == 0 && encoded.loopSound == 0 );
	byte buffer[4096] = {};
	msg_t message;
	MSG_Init( &message, buffer, sizeof( buffer ) );
	MSG_WriteDeltaEntity( &message, &baseline, &encoded, qtrue );
	MSG_BeginReading( &message );
	const int entity = MSG_ReadBits( &message, GENTITYNUM_BITS );
	assert( entity == encoded.number );
	MSG_ReadDeltaEntity( &message, &baseline, &decoded, entity );
	animState_t restored;
	float restoredParameters[ANIM_MAX_PARAMETERS];
	assert( BG_EntityStateToAnimation( &decoded, &restored, restoredParameters ) );
	assert( memcmp( &state, &restored, sizeof( state ) ) == 0 );
	assert( memcmp( parameters, restoredParameters, sizeof( parameters ) ) == 0 );
	assert( memcmp( origin, decoded.origin, sizeof( origin ) ) == 0 );
	assert( memcmp( angles, decoded.angles, sizeof( angles ) ) == 0 );
	assert( decoded.otherEntityNum == 63 && decoded.otherEntityNum2 == 1 );
	const int initialBytes = message.cursize;
	baseline = decoded;
	animState_t changed = state;
	changed.lastTime = 44;
	assert( BG_AnimationToEntityState( &changed, parameters, 63, 1, origin, angles, &encoded ) );
	MSG_Clear( &message );
	MSG_WriteDeltaEntity( &message, &baseline, &encoded, qtrue );
	assert( message.cursize < initialBytes );
	MSG_BeginReading( &message );
	MSG_ReadDeltaEntity( &message, &baseline, &decoded, MSG_ReadBits( &message, GENTITYNUM_BITS ) );
	assert( BG_EntityStateToAnimation( &decoded, &restored, restoredParameters ) );
	assert( memcmp( &changed, &restored, sizeof( changed ) ) == 0 );
	printf( "PASS: animation snapshot preserves all state/parameter bits (%d initial, %d delta bytes)\n", initialBytes, message.cursize );
}

#include "../../engine/animation/animation_public.h"
#include "../../engine/qcommon/net_history_public.h"
#include "../../engine/public/g_native_public.h"
#include "../../engine/public/dev_public.h"
#define COM_TRAP_GETVALUE 700
#include "../../game/game/g_rewind.cpp"
#include <assert.h>

level_locals_t level;
gentity_t g_entities[MAX_GENTITIES];
static gclient_t shooterClient;
static netHistory_t storage;
static bool enabled = true, wall;
static uint32_t allocations, filtered, legacy;
static byte lastIgnored[MAX_GENTITIES];

void *GameImport_AllocLevelMemory( uint32_t bytes ) {
	assert( bytes == sizeof( storage ) );
	++allocations;
	return &storage;
}
void trap_Cvar_Register( vmCvar_t *value, const char *name, const char *, int ) {
	value->integer = !strcmp( name, "g_rewind" ) ? int( enabled ) : !strcmp( name, "g_maxRewind" ) ? 200
																								   : 0;
}
void trap_Cvar_Update( vmCvar_t * ) {
}
void QDECL G_Printf( const char *, ... ) {
}
void QDECL G_Error( const char *, ... ) {
	abort();
}
const animBox_t *G_AnimationHitBoxes( int, uint32_t *count ) {
	*count = 0;
	return nullptr;
}
void trap_Trace( trace_t *trace, const vec3_t, const vec3_t, const vec3_t, const vec3_t, int, int ) {
	++legacy;
	*trace = {};
	trace->fraction = 1;
	trace->entityNum = ENTITYNUM_NONE;
}
void GameImport_TraceFiltered( void *result, const float *, const float *, int, int, const uint8_t *ignored ) {
	++filtered;
	memcpy( lastIgnored, ignored, sizeof( lastIgnored ) );
	auto *trace = (trace_t *)result;
	*trace = {};
	trace->fraction = wall ? 0.25f : 1.0f;
	trace->entityNum = wall ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
}

int main() {
	level.num_entities = 8;
	auto &shooter = g_entities[0];
	shooter.s.number = 0;
	shooter.client = &shooterClient;
	auto &target = g_entities[7];
	target.s.number = 7;
	target.inuse = target.r.linked = target.takedamage = qtrue;
	target.health = 100;
	target.r.contents = CONTENTS_BODY;
	target.r.ownerNum = ENTITYNUM_NONE;
	target.rewindSpawn = 1;
	for ( int axis = 0; axis < 3; ++axis ) {
		target.r.mins[axis] = -2;
		target.r.maxs[axis] = 2;
	}
	G_InitRewind( 0 );
	for ( int time = 0; time <= 1000; time += 20 ) {
		level.time = time;
		target.r.currentOrigin[0] = 100;
		target.r.currentOrigin[1] = float( time ) * 0.32f;
		G_RecordRewind();
	}
	assert( allocations == 1 );
	shooterClient.pers.cmd.serverTime = 900;
	const float start[3] = { 0, 288, 0 }, end[3] = { 200, 288, 0 };
	trace_t trace;
	const auto before = target.r;
	G_TraceHitscan( &trace, start, end, 0, &shooter );
	assert( trace.entityNum == 7 && trace.fraction == 0.49f && lastIgnored[7] );
	assert( !memcmp( &before, &target.r, sizeof( before ) ) );
	wall = true;
	G_TraceHitscan( &trace, start, end, 0, &shooter );
	assert( trace.entityNum == ENTITYNUM_WORLD && trace.fraction == 0.25f );
	wall = false;
	++target.rewindSpawn;
	G_TraceHitscan( &trace, start, end, 0, &shooter );
	assert( trace.entityNum == ENTITYNUM_NONE && !lastIgnored[7] );
	assert( filtered == 3 && legacy == 0 && allocations == 1 );
	G_InitRewind( 1 );
	assert( allocations == 1 ); // restart reuses level arena storage
	enabled = false;
	G_InitRewind( 0 );
	G_TraceHitscan( &trace, start, end, 0, &shooter );
	assert( legacy == 1 && allocations == 1 );
	puts( "PASS: game history uses view time, preserves world/live state, rejects reused slots and allocates only at level init" );
}

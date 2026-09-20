#include "g_local.h"
#include "../../engine/qcommon/net_history_public.h"
#include "../../engine/public/g_native_public.h"

static netHistory_t *rewindHistory;
static vmCvar_t rewindEnabled, maximumRewind, rewindTrace;
static bool useRewind;
static struct {
	uint32_t spawn;
	int playerSpawn, teleport;
	uint64_t generation;
} rewindEntities[MAX_GENTITIES];
static uint64_t nextGeneration;
static netHistoryFrame_t rewindFrame;

void G_InitRewind( int restart ) {
	trap_Cvar_Register( &rewindEnabled, "g_rewind", "0", CVAR_LATCH | CVAR_SERVERINFO );
	trap_Cvar_Register( &maximumRewind, "g_maxRewind", "200", CVAR_SERVERINFO );
	trap_Cvar_Register( &rewindTrace, "g_rewindTrace", "0", 0 );
	if ( !restart )
		rewindHistory = nullptr;
	useRewind = rewindEnabled.integer != 0;
	if ( useRewind && !rewindHistory )
		rewindHistory = (netHistory_t *)GameImport_AllocLevelMemory( (uint32_t)sizeof( netHistory_t ) );
	if ( rewindHistory )
		NET_HistoryReset( rewindHistory );
	memset( rewindEntities, 0, sizeof( rewindEntities ) );
	nextGeneration = 0;
}

static bool RewindEligible( const gentity_t &entity ) {
	return entity.inuse && entity.r.linked && entity.takedamage && entity.health > 0 && !entity.r.bmodel && ( entity.r.contents & MASK_SHOT );
}

static uint64_t RewindGeneration( int number ) {
	const auto &entity = g_entities[number];
	const auto &saved = rewindEntities[number];
	const int spawn = entity.client ? entity.client->ps.persistant[PERS_SPAWN_COUNT] : 0;
	const int teleport = entity.client ? entity.client->ps.eFlags & EF_TELEPORT_BIT : entity.s.eFlags & EF_TELEPORT_BIT;
	return saved.spawn == entity.rewindSpawn && saved.playerSpawn == spawn && saved.teleport == teleport ? saved.generation : 0;
}

void G_RecordRewind( void ) {
	if ( !useRewind )
		return;
	rewindFrame = {};
	rewindFrame.time = (uint32_t)level.time;
	for ( int number = 0; number < level.num_entities; ++number ) {
		const auto &entity = g_entities[number];
		auto &saved = rewindEntities[number];
		if ( !RewindEligible( entity ) ) {
			saved.generation = 0;
			continue;
		}
		if ( !RewindGeneration( number ) ) {
			saved.spawn = entity.rewindSpawn;
			saved.playerSpawn = entity.client ? entity.client->ps.persistant[PERS_SPAWN_COUNT] : 0;
			saved.teleport = entity.client ? entity.client->ps.eFlags & EF_TELEPORT_BIT : entity.s.eFlags & EF_TELEPORT_BIT;
			saved.generation = ++nextGeneration;
		}
		uint32_t count = 0;
		const animBox_t *animation = G_AnimationHitBoxes( number, &count );
		if ( !count )
			count = 1;
		if ( count > NET_HISTORY_BOXES - rewindFrame.boxCount )
			G_Error( "Rewind history box capacity exceeded" );
		auto &record = rewindFrame.entities[number];
		record = { saved.generation, rewindFrame.boxCount, count };
		for ( uint32_t i = 0; i < count; ++i ) {
			auto &box = rewindFrame.boxes[rewindFrame.boxCount++];
			for ( int axis = 0; axis < 3; ++axis ) {
				box.mins[axis] = animation ? animation[i].mins[axis] : entity.r.currentOrigin[axis] + entity.r.mins[axis];
				box.maxs[axis] = animation ? animation[i].maxs[axis] : entity.r.currentOrigin[axis] + entity.r.maxs[axis];
			}
		}
	}
	if ( !NET_HistoryStore( rewindHistory, &rewindFrame ) )
		G_Error( "Rewind history frame rejected" );
}

static bool RewindPassEntity( int number, int pass ) {
	if ( pass < 0 || pass >= ENTITYNUM_WORLD )
		return false;
	const int owner = g_entities[pass].r.ownerNum;
	return number == pass || g_entities[number].r.ownerNum == pass || ( owner != ENTITYNUM_NONE && g_entities[number].r.ownerNum == owner );
}

void G_TraceHitscan( trace_t *trace, const vec3_t start, const vec3_t end, int pass, const gentity_t *shooter ) {
	netHistoryQuery_t query;
	trap_Cvar_Update( &maximumRewind );
	if ( !useRewind || !shooter->client || !NET_HistoryQuery( rewindHistory, (uint32_t)level.time, (uint32_t)shooter->client->pers.cmd.serverTime, (uint32_t)( maximumRewind.integer > 0 ? maximumRewind.integer : 0 ), &query ) ) {
		trap_Trace( trace, start, nullptr, nullptr, end, pass, MASK_SHOT );
		return;
	}
	static_assert( MAX_GENTITIES == NET_HISTORY_ENTITIES );
	byte ignored[MAX_GENTITIES] = {};
	netBox_t boxes[ANIM_MAX_BOXES];
	// ponytail: bounded 1024-entity scan; add a history spatial index only if profiling needs it.
	for ( int number = 0; number < level.num_entities; ++number ) {
		if ( !RewindEligible( g_entities[number] ) || number == shooter->s.number || RewindPassEntity( number, pass ) )
			continue;
		ignored[number] = (byte)( NET_HistoryEntity( &query, (uint32_t)number, RewindGeneration( number ), boxes, ANIM_MAX_BOXES ) != 0 );
	}
	// Historical actors are tested separately; world/brush/live fallback collision stays authoritative.
	GameImport_TraceFiltered( trace, start, end, pass, MASK_SHOT, ignored );
	for ( int number = 0; number < level.num_entities; ++number ) {
		if ( !ignored[number] )
			continue;
		const uint32_t count = NET_HistoryEntity( &query, (uint32_t)number, RewindGeneration( number ), boxes, ANIM_MAX_BOXES );
		netBoxHit_t hit;
		if ( !NET_TraceBoxes( start, end, boxes, count, trace->fraction, &hit ) )
			continue;
		trace->fraction = hit.fraction;
		trace->entityNum = number;
		trace->contents = g_entities[number].r.contents;
		trace->surfaceFlags = 0;
		trace->startsolid = (qboolean)( trace->startsolid || hit.startSolid );
		trace->allsolid = (qboolean)hit.allSolid;
		trace->plane = {};
		trace->plane.type = 3;
		for ( int axis = 0; axis < 3; ++axis ) {
			trace->endpos[axis] = start[axis] + hit.fraction * ( end[axis] - start[axis] );
			trace->plane.normal[axis] = hit.normal[axis];
			trace->plane.dist += hit.normal[axis] * trace->endpos[axis];
			if ( hit.normal[axis] )
				trace->plane.type = (byte)axis;
			if ( hit.normal[axis] < 0 )
				trace->plane.signbits |= (byte)( 1u << axis );
		}
	}
	trap_Cvar_Update( &rewindTrace );
	if ( rewindTrace.integer )
		G_Printf( "Rewind trace: shooter=%d view=%u age=%u clamped=%d hit=%d fraction=%.6f\n", shooter->s.number,
			query.time, query.rewindMs, int( query.clamped ), trace->entityNum, double( trace->fraction ) );
}

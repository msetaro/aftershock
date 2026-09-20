#include "server.h"
#include <algorithm>
#include <cmath>

bool SV_SetEntityReplication( int number, int priority, float radius ) {
	if ( number < 0 || number >= ENTITYNUM_WORLD || priority < 0 || priority > 3 || !std::isfinite( radius ) || radius < 0 || radius > 65536 )
		return false;
	sv.svEntities[number].replicationPriority = priority;
	sv.svEntities[number].interestRadius = radius;
	return true;
}

bool SV_EntityRelevant( const sharedEntity_t *entity, const vec3_t view ) {
	const float radius = sv.svEntities[entity->s.number].interestRadius;
	if ( radius == 0 )
		return true;
	double distance = 0;
	for ( int axis = 0; axis < 3; ++axis ) {
		const double delta = double( entity->r.currentOrigin[axis] ) - double( view[axis] );
		distance += delta * delta;
	}
	return distance <= double( radius ) * double( radius );
}

static int EntityBits( const entityState_t *from, const entityState_t *to, qboolean force ) {
	// A complete 208-byte entity needs fewer than 512 bytes even with Huffman expansion.
	byte data[512] = {};
	msg_t measure;
	MSG_Init( &measure, data, sizeof( data ) );
	MSG_WriteDeltaEntity( &measure, from, to, force );
	return measure.bit;
}

void SV_ApplyReplicationPolicy( client_t *client, const clientSnapshot_t *oldframe, clientSnapshot_t *frame,
	int prefixBits, entityState_t *const *candidates, int count ) {
	if ( sv_snapshotBudget->integer <= 0 )
		return;
	Q_ASSERT( count >= 0 && count <= MAX_GENTITIES );
	entityState_t *chosen[MAX_GENTITIES];
	std::copy_n( candidates, count, chosen );
	// Capacity selection is stable; bandwidth aging only schedules this selected set.
	std::sort( chosen, chosen + count, []( const entityState_t *a, const entityState_t *b ) {
		const int ap = sv.svEntities[a->number].replicationPriority, bp = sv.svEntities[b->number].replicationPriority;
		return ap != bp ? ap > bp : a->number < b->number;
	} );
	count = std::min( count, MAX_SNAPSHOT_ENTITIES );
	bool visible[MAX_GENTITIES] = {};
	for ( int i = 0; i < count; ++i )
		visible[chosen[i]->number] = true;
	entityState_t *previous[MAX_GENTITIES] = {};
	byte data[16] = {};
	msg_t marker;
	MSG_Init( &marker, data, sizeof( data ) );
	MSG_WriteBits( &marker, MAX_GENTITIES - 1, GENTITYNUM_BITS );
	int used = prefixBits + marker.bit;
	if ( oldframe ) {
		for ( int i = 0; i < oldframe->num_entities; ++i ) {
			entityState_t *entity = oldframe->ents[i];
			previous[entity->number] = entity;
			if ( !visible[entity->number] )
				used += EntityBits( entity, nullptr, qtrue );
		}
	}
	int64_t bytes = std::min( sv_snapshotBudget->integer, MAX_MSGLEN );
	if ( client->rate > 0 && client->snapshotMsec > 0 )
		bytes = std::min( bytes, int64_t( client->rate ) * client->snapshotMsec / 1000 );
	const int limit = (int)( bytes * 8 ) - 8; // MSG cursize includes the final partial byte.
	struct update_t {
		entityState_t *entity, *previous;
		uint32_t score;
		int bits;
	} updates[MAX_SNAPSHOT_ENTITIES];
	for ( int i = 0; i < count; ++i ) {
		entityState_t *entity = chosen[i], *before = previous[entity->number];
		const uint32_t age = std::min( (uint32_t)sv.time - client->lastEntityUpdate[entity->number], 60000u );
		updates[i] = { entity, before, age + (uint32_t)sv.svEntities[entity->number].replicationPriority * 250u,
			EntityBits( before ? before : &sv.svEntities[entity->number].baseline, entity, before ? qfalse : qtrue ) };
	}
	std::sort( updates, updates + count, []( const update_t &a, const update_t &b ) {
		return a.score != b.score ? a.score > b.score : a.entity->number < b.entity->number;
	} );
	frame->num_entities = 0;
	client->deferredEntities = 0;
	client->replicationOverBudget = std::max( 0, used - limit );
	if ( client->replicationOverBudget )
		++client->replicationBudgetOverruns;
	for ( int i = 0; i < count; ++i ) {
		const auto &update = updates[i];
		if ( update.bits == 0 || used + update.bits <= limit ) {
			used += update.bits;
			frame->ents[frame->num_entities++] = update.entity;
			if ( update.bits )
				client->lastEntityUpdate[update.entity->number] = (uint32_t)sv.time;
		} else {
			++client->deferredEntities;
			++client->deferredEntityUpdates;
			if ( update.previous ) {
				frame->ents[frame->num_entities++] = update.previous;
				// Tag borrowed pointers with the oldest source generation so stale deltas are rejected.
				frame->frameNum = std::min( frame->frameNum, oldframe->frameNum );
			}
		}
	}
	std::sort( frame->ents, frame->ents + frame->num_entities, []( const entityState_t *a, const entityState_t *b ) {
		return a->number < b->number;
	} );
}

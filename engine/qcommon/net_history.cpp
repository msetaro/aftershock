#include "net_history_public.h"
#include <algorithm>
#include <cmath>
#include <cstring>

static int32_t TimeDifference( uint32_t newer, uint32_t older ) {
	return (int32_t)( newer - older );
}

static bool ValidBox( const netBox_t &box ) {
	for ( uint32_t axis = 0; axis < 3; ++axis )
		if ( !std::isfinite( box.mins[axis] ) || !std::isfinite( box.maxs[axis] ) || box.mins[axis] > box.maxs[axis] )
			return false;
	return true;
}

void NET_HistoryReset( netHistory_t *history ) {
	// Old storage is unreachable once count is zero; no multi-megabyte clear needed.
	history->next = history->count = 0;
}

bool NET_HistoryStore( netHistory_t *history, const netHistoryFrame_t *frame ) {
	if ( frame->boxCount > NET_HISTORY_BOXES )
		return false;
	if ( history->count && TimeDifference( frame->time, history->frames[( history->next + NET_HISTORY_FRAMES - 1 ) % NET_HISTORY_FRAMES].time ) <= 0 )
		return false;
	for ( const auto &entity : frame->entities ) {
		if ( entity.boxCount && ( !entity.generation || entity.firstBox > frame->boxCount || entity.boxCount > frame->boxCount - entity.firstBox ) )
			return false;
	}
	for ( uint32_t box = 0; box < frame->boxCount; ++box )
		if ( !ValidBox( frame->boxes[box] ) )
			return false;
	history->frames[history->next] = *frame;
	history->next = ( history->next + 1 ) % NET_HISTORY_FRAMES;
	history->count = std::min( history->count + 1, NET_HISTORY_FRAMES );
	return true;
}

bool NET_HistoryQuery( const netHistory_t *history, uint32_t now, uint32_t viewTime, uint32_t maximumRewind, netHistoryQuery_t *query ) {
	if ( !history->count )
		return false;
	const auto *newest = &history->frames[( history->next + NET_HISTORY_FRAMES - 1 ) % NET_HISTORY_FRAMES];
	const auto *oldest = &history->frames[( history->next + NET_HISTORY_FRAMES - history->count ) % NET_HISTORY_FRAMES];
	const int32_t newestAge = TimeDifference( now, newest->time ), oldestAge = TimeDifference( now, oldest->time );
	if ( newestAge < 0 || oldestAge < newestAge )
		return false;
	const int32_t limit = (int32_t)std::min( maximumRewind, NET_MAX_REWIND_MS );
	// Stale history must never extend the configured rewind budget.
	if ( newestAge > limit )
		return false;
	const int32_t requested = TimeDifference( now, viewTime );
	const int32_t age = std::clamp( requested, newestAge, std::min( limit, oldestAge ) );
	netHistoryQuery_t result = {};
	result.time = now - (uint32_t)age;
	result.rewindMs = (uint32_t)age;
	result.clamped = age != requested;
	result.newer = newest;
	for ( uint32_t i = 0; i < history->count; ++i ) {
		const auto *frame = &history->frames[( history->next + NET_HISTORY_FRAMES - 1 - i ) % NET_HISTORY_FRAMES];
		if ( TimeDifference( result.time, frame->time ) >= 0 ) {
			result.older = frame;
			if ( result.time == frame->time )
				result.newer = frame;
			if ( result.newer != frame )
				result.fraction = float( result.time - frame->time ) / float( result.newer->time - frame->time );
			*query = result;
			return true;
		}
		result.newer = frame;
	}
	return false;
}

uint32_t NET_HistoryEntity( const netHistoryQuery_t *query, uint32_t entity, uint64_t generation, netBox_t *boxes, uint32_t capacity ) {
	if ( entity >= NET_HISTORY_ENTITIES || !generation || !query->older || !query->newer )
		return 0;
	const auto &older = query->older->entities[entity], &newer = query->newer->entities[entity];
	if ( older.generation != generation || newer.generation != generation || older.boxCount != newer.boxCount || older.boxCount > capacity )
		return 0;
	for ( uint32_t i = 0; i < older.boxCount; ++i ) {
		const auto &a = query->older->boxes[older.firstBox + i], &b = query->newer->boxes[newer.firstBox + i];
		for ( uint32_t axis = 0; axis < 3; ++axis ) {
			boxes[i].mins[axis] = a.mins[axis] + query->fraction * ( b.mins[axis] - a.mins[axis] );
			boxes[i].maxs[axis] = a.maxs[axis] + query->fraction * ( b.maxs[axis] - a.maxs[axis] );
		}
	}
	return older.boxCount;
}

bool NET_TraceBoxes( const float start[3], const float end[3], const netBox_t *boxes, uint32_t count, float maximumFraction, netBoxHit_t *hit ) {
	if ( !std::isfinite( maximumFraction ) || maximumFraction <= 0 || maximumFraction > 1 )
		return false;
	float direction[3];
	for ( uint32_t axis = 0; axis < 3; ++axis ) {
		direction[axis] = end[axis] - start[axis];
		if ( !std::isfinite( start[axis] ) || !std::isfinite( direction[axis] ) )
			return false;
	}
	netBoxHit_t best = {};
	best.fraction = maximumFraction;
	bool found = false;
	for ( uint32_t i = 0; i < count; ++i ) {
		if ( !ValidBox( boxes[i] ) )
			continue;
		float enter = 0, leave = 1, normal[3] = {};
		bool intersects = true, inside = true;
		for ( uint32_t axis = 0; axis < 3; ++axis ) {
			const float minimum = boxes[i].mins[axis], maximum = boxes[i].maxs[axis];
			inside = inside && start[axis] >= minimum && start[axis] <= maximum;
			if ( direction[axis] == 0 ) {
				if ( start[axis] < minimum || start[axis] > maximum )
					intersects = false;
				continue;
			}
			float near = ( minimum - start[axis] ) / direction[axis], far = ( maximum - start[axis] ) / direction[axis];
			float sign = -1;
			if ( near > far ) {
				std::swap( near, far );
				sign = 1;
			}
			if ( near > enter ) {
				enter = near;
				std::memset( normal, 0, sizeof( normal ) );
				normal[axis] = sign;
			}
			leave = std::min( leave, far );
		}
		if ( intersects && enter <= leave && enter < best.fraction ) {
			best.fraction = enter;
			std::memcpy( best.normal, normal, sizeof( normal ) );
			best.box = i;
			best.startSolid = inside;
			best.allSolid = inside && leave >= 1;
			found = true;
		}
	}
	if ( found )
		*hit = best;
	return found;
}

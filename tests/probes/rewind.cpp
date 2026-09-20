// Portable moving-target validation with simulated delayed/lost shot times.
#include "../../engine/qcommon/net_history_public.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static netHistory_t history;
static netHistoryFrame_t frame;

static void Record( uint32_t time, uint64_t generation = 1 ) {
	frame = {};
	frame.time = time;
	frame.boxCount = 1;
	frame.entities[7] = { generation, 0, 1 };
	const float y = float( time ) * 0.32f;
	frame.boxes[0] = { { 98, y - 2, -2 }, { 102, y + 2, 2 } };
	assert( NET_HistoryStore( &history, &frame ) );
}

int main() {
	netHistoryQuery_t query;
	netBox_t boxes[32];
	netBoxHit_t hit;
	assert( !NET_HistoryQuery( &history, 0, 0, 200, &query ) );
	uint32_t delivered = 0, hits = 0, currentHits = 0;
	float maximumError = 0;
	for ( uint32_t now = 0; now <= 22000; now += 20 ) {
		Record( now );
		if ( now < 2000 || now % 400 == 0 ) // deterministic 5% packet loss
			continue;
		const int jitter = int( ( now / 20 ) % 31 ) - 15;
		const uint32_t view = now - uint32_t( 100 + jitter );
		assert( NET_HistoryQuery( &history, now, view, 200, &query ) );
		assert( query.time == view && !query.clamped );
		assert( NET_HistoryEntity( &query, 7, 1, boxes, 32 ) == 1 );
		const float aim = float( view ) * 0.32f;
		const float start[3] = { 0, aim, 0 }, end[3] = { 200, aim, 0 };
		const float error = fabsf( ( boxes[0].mins[1] + boxes[0].maxs[1] ) * 0.5f - aim );
		maximumError = fmaxf( maximumError, error );
		++delivered;
		hits += NET_TraceBoxes( start, end, boxes, 1, 1, &hit );
		assert( hit.box == 0 && hit.normal[0] == -1 );
		currentHits += NET_TraceBoxes( start, end, frame.boxes, 1, 1, &hit );
		// A world wall halfway to the target must remain authoritative.
		assert( !NET_TraceBoxes( start, end, boxes, 1, 0.25f, &hit ) );
	}
	assert( delivered >= 950 && hits == delivered && currentHits == 0 && maximumError < 0.001f );
	assert( NET_HistoryQuery( &history, 22000, 1000, 200, &query ) && query.time == 21800 && query.clamped );
	assert( NET_HistoryQuery( &history, 22000, 23000, 200, &query ) && query.time == 22000 && query.clamped );
	assert( NET_HistoryEntity( &query, 7, 2, boxes, 32 ) == 0 ); // reused entity slot
	assert( NET_HistoryEntity( &query, 7, 1, boxes, 0 ) == 0 );
	Record( 22020, 2 );
	assert( NET_HistoryQuery( &history, 22020, 22010, 200, &query ) );
	assert( NET_HistoryEntity( &query, 7, 2, boxes, 32 ) == 0 ); // never blend across respawn/teleport
	assert( !NET_HistoryStore( &history, &frame ) ); // one record per server frame
	NET_HistoryReset( &history );
	frame = {};
	frame.time = 0xfffffff0u;
	assert( NET_HistoryStore( &history, &frame ) );
	frame.time = 4;
	assert( NET_HistoryStore( &history, &frame ) );
	assert( NET_HistoryQuery( &history, 4, 0xfffffffau, 200, &query ) );
	assert( query.time == 0xfffffffau && query.rewindMs == 10 );
	assert( NET_HistoryQuery( &history, 4, 0xffffff00u, 200, &query ) && query.clamped && query.time == 0xfffffff0u );
	assert( !NET_HistoryQuery( &history, 5000, 4900, 200, &query ) ); // stale data cannot extend the budget
	const netBox_t box = { { -1, -1, -1 }, { 1, 1, 1 } };
	const float center[3] = {}, outside[3] = { 2, 0, 0 }, miss[3] = { 2, 2, 0 };
	assert( NET_TraceBoxes( center, center, &box, 1, 1, &hit ) && hit.startSolid && hit.allSolid );
	assert( NET_TraceBoxes( center, outside, &box, 1, 1, &hit ) && hit.startSolid && !hit.allSolid );
	assert( !NET_TraceBoxes( outside, miss, &box, 1, 1, &hit ) );
	assert( NET_TraceBoxes( outside, center, &box, 1, 1, &hit ) && hit.normal[0] == 1 && hit.fraction == 0.5f );
	frame.time = 24;
	frame.boxCount = NET_HISTORY_BOXES + 1;
	assert( !NET_HistoryStore( &history, &frame ) );
	printf( "PASS: %u/%u delayed hits, %u unrewound; 100 ms +/-15 ms, 5%% loss, max error %.6f units\n", hits, delivered, currentHits, double( maximumError ) );
}

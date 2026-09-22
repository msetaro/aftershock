#include "devtools_public.h"
#include <cmath>
#include "../qcommon/qcommon_public.h"

struct cpuScope_t {
	devCpuTiming_t timing;
	int64_t begin;
	bool complete;
};
static cpuScope_t scopes[128];
static devCpuTiming_t completed[128];
static uint32_t count, completedCount, generation, parent = UINT32_MAX, dropped;
static devCpuFrame_t history[240], peak;
static uint32_t historyCount, historyCursor;
static int64_t frameStart, frameEnd;
static bool havePeak;
static bool active;
static devNetwork_t network;
static devNetworkPacket_t packets[256];
static uint32_t packetCursor, packetCount;
static devNetworkField_t networkFields[128];

void DevTools_BeginFrame( bool enabled ) {
	DevTools_BeginDebugFrame( enabled, (uint32_t)com_frameTime );
	const int64_t now = active || enabled ? Sys_Microseconds() : 0;
	completedCount = 0;
	uint32_t mapping[128];
	for ( uint32_t i = 0; i < count; ++i ) {
		mapping[i] = scopes[i].complete ? completedCount++ : UINT32_MAX;
	}
	for ( uint32_t i = 0; i < count; ++i ) {
		if ( !scopes[i].complete )
			continue;
		auto &timing = completed[mapping[i]];
		timing = scopes[i].timing;
		uint32_t ancestor = timing.parent;
		while ( ancestor != UINT32_MAX && !scopes[ancestor].complete )
			ancestor = scopes[ancestor].timing.parent;
		timing.parent = ancestor == UINT32_MAX ? UINT32_MAX : mapping[ancestor];
		timing.selfMicroseconds = timing.microseconds;
	}
	for ( uint32_t i = 0; i < completedCount; ++i ) {
		const uint32_t ancestor = completed[i].parent;
		if ( ancestor != UINT32_MAX )
			completed[ancestor].selfMicroseconds = MAX( INT64_C( 0 ), completed[ancestor].selfMicroseconds - completed[i].microseconds );
	}
	if ( active ) {
		auto &frame = history[historyCursor];
		frame.serial = generation;
		frame.count = completedCount;
		frame.dropped = dropped;
		frame.microseconds = MAX( INT64_C( 0 ), frameEnd - frameStart );
		memcpy( frame.scopes, completed, completedCount * sizeof( completed[0] ) );
		if ( !havePeak || frame.microseconds > peak.microseconds ) {
			peak = frame;
			havePeak = true;
		}
		historyCursor = ( historyCursor + 1 ) % ARRAY_LEN( history );
		historyCount = MIN( historyCount + 1, (uint32_t)ARRAY_LEN( history ) );
	}
	count = dropped = 0;
	parent = UINT32_MAX;
	frameStart = frameEnd = now;
	++generation;
	active = enabled;
}

const devCpuFrame_t *DevTools_CpuFrame( uint32_t age ) {
	return age < historyCount ? &history[( historyCursor + ARRAY_LEN( history ) - 1 - age ) % ARRAY_LEN( history )] : nullptr;
}
const devCpuFrame_t *DevTools_CpuPeak() {
	return havePeak ? &peak : nullptr;
}
void DevTools_ClearCpuHistory() {
	historyCount = historyCursor = 0;
	havePeak = false;
}

uint64_t Dev_BeginScope( const char *name ) {
	if ( !active )
		return UINT64_MAX;
	if ( count == ARRAY_LEN( scopes ) ) {
		if ( dropped != UINT32_MAX )
			++dropped;
		return UINT64_MAX;
	}
	const uint32_t slot = count++;
	scopes[slot] = {};
	scopes[slot].timing.parent = parent;
	parent = slot;
	Q_strncpyz( scopes[slot].timing.name, name, sizeof( scopes[slot].timing.name ) );
	scopes[slot].begin = Sys_Microseconds();
	return ( (uint64_t)generation << 32 ) | slot;
}

void Dev_EndScope( uint64_t token ) {
	const uint32_t slot = (uint32_t)token;
	if ( token == UINT64_MAX || (uint32_t)( token >> 32 ) != generation || slot >= count || scopes[slot].complete )
		return;
	// The existing platform clock can move backwards; never report negative time.
	const int64_t now = Sys_Microseconds();
	scopes[slot].timing.microseconds = MAX( INT64_C( 0 ), now - scopes[slot].begin );
	frameEnd = MAX( frameEnd, now );
	scopes[slot].complete = true;
	while ( parent != UINT32_MAX && scopes[parent].complete )
		parent = scopes[parent].timing.parent;
}

uint32_t DevTools_CpuTimings( const devCpuTiming_t **timings ) {
	*timings = completed;
	return completedCount;
}

void DevTools_Packet( bool outgoing, uint32_t bytes ) {
	if ( !active )
		return;
	const int direction = outgoing ? 1 : 0;
	network.bytes[direction] += bytes;
	++network.packets[direction];
	network.lastPacket[direction] = bytes;
	packets[packetCursor] = { (uint32_t)com_frameTime, bytes, outgoing };
	packetCursor = ( packetCursor + 1 ) % ARRAY_LEN( packets );
	packetCount = MIN( packetCount + 1, (uint32_t)ARRAY_LEN( packets ) );
}

void DevTools_Snapshot( uint32_t bits, bool delta ) {
	if ( active ) {
		network.snapshotBits = bits;
		network.delta = delta;
		++network.snapshots;
	}
}

void Dev_PredictionError( float distance ) {
	if ( active && std::isfinite( distance ) && distance >= 0 ) {
		network.predictionError = distance;
		network.predictionPeak = MAX( network.predictionPeak, distance );
		network.predictionSum += double( distance );
		++network.predictions;
	}
}

void Dev_RewindReport( uint32_t age, uint32_t limit, int clamped, int hit ) {
	if ( !active || age > limit || limit > 1000 || clamped < 0 || clamped > 1 || hit < 0 || hit > 1 )
		return;
	network.rewindAge = age;
	network.rewindLimit = limit;
	++network.rewindReports;
	network.rewindHits += (uint32_t)hit;
	network.rewindClamped += (uint32_t)clamped;
}

const devNetwork_t *DevTools_Network( void ) {
	return &network;
}

const devNetworkPacket_t *DevTools_NetworkPacket( uint32_t age ) {
	return age < packetCount ? &packets[( packetCursor + ARRAY_LEN( packets ) - 1 - age ) % ARRAY_LEN( packets )] : nullptr;
}
uint32_t DevTools_NetworkFields( const devNetworkField_t **fields ) {
	*fields = networkFields;
	return ARRAY_LEN( networkFields );
}
void DevTools_NetworkField( bool outgoing, bool player, uint32_t index, const char *name, int bits ) {
	if ( !active || index >= 64 || bits < 0 )
		return;
	auto &field = networkFields[( player ? 64 : 0 ) + index];
	if ( !field.name[0] )
		snprintf( field.name, sizeof( field.name ), "%s.%s", player ? "player" : "entity", name );
	const uint32_t direction = outgoing ? 1 : 0;
	field.bits[direction] += (uint32_t)bits;
	++field.samples[direction];
}
void DevTools_ClearNetwork() {
	network = {};
	packetCursor = packetCount = 0;
	memset( networkFields, 0, sizeof( networkFields ) );
}

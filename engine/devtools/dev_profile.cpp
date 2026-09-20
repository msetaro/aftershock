#include "devtools_public.h"
#include "../qcommon/qcommon_public.h"

struct cpuScope_t {
	devCpuTiming_t timing;
	int64_t begin;
	bool complete;
};
static cpuScope_t scopes[128];
static devCpuTiming_t completed[128];
static uint32_t count, completedCount, generation;
static bool active;
static devNetwork_t network;

void DevTools_BeginFrame( bool enabled ) {
	completedCount = 0;
	for ( uint32_t i = 0; i < count; ++i ) {
		if ( scopes[i].complete )
			completed[completedCount++] = scopes[i].timing;
	}
	count = 0;
	++generation;
	active = enabled;
}

uint64_t Dev_BeginScope( const char *name ) {
	if ( !active || count == ARRAY_LEN( scopes ) )
		return UINT64_MAX;
	const uint32_t slot = count++;
	scopes[slot] = {};
	Q_strncpyz( scopes[slot].timing.name, name, sizeof( scopes[slot].timing.name ) );
	scopes[slot].begin = Sys_Microseconds();
	return ( (uint64_t)generation << 32 ) | slot;
}

void Dev_EndScope( uint64_t token ) {
	const uint32_t slot = (uint32_t)token;
	if ( token == UINT64_MAX || (uint32_t)( token >> 32 ) != generation || slot >= count || scopes[slot].complete )
		return;
	// The existing platform clock can move backwards; never report negative time.
	scopes[slot].timing.microseconds = MAX( INT64_C( 0 ), Sys_Microseconds() - scopes[slot].begin );
	scopes[slot].complete = true;
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
}

void DevTools_Snapshot( uint32_t bits, bool delta ) {
	if ( active ) {
		network.snapshotBits = bits;
		network.delta = delta;
		++network.snapshots;
	}
}

void Dev_PredictionError( float distance ) {
	if ( active ) {
		network.predictionError = distance;
		++network.predictions;
	}
}

const devNetwork_t *DevTools_Network( void ) {
	return &network;
}

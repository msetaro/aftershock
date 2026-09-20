#include "../../engine/devtools/dev_profile.cpp"
#include <assert.h>

int com_frameTime;
void DevTools_BeginDebugFrame( bool, uint32_t ) {
}
static int64_t clockValue;
int64_t Sys_Microseconds( void ) {
	return clockValue;
}
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}

int main() {
	const devCpuTiming_t *timings;
	assert( Dev_BeginScope( "disabled" ) == UINT64_MAX );
	DevTools_BeginFrame( true );
	const uint64_t outer = Dev_BeginScope( "outer" );
	clockValue = 10;
	const uint64_t inner = Dev_BeginScope( "inner" );
	clockValue = 30;
	Dev_EndScope( inner );
	clockValue = 50;
	Dev_EndScope( outer );
	Dev_EndScope( inner );
	const uint64_t abandoned = Dev_BeginScope( "abandoned" );
	DevTools_BeginFrame( true );
	assert( DevTools_CpuTimings( &timings ) == 2 );
	assert( timings[0].microseconds == 50 && timings[1].microseconds == 20 );
	for ( int i = 0; i < 128; ++i )
		assert( Dev_BeginScope( "capacity" ) != UINT64_MAX );
	assert( Dev_BeginScope( "overflow" ) == UINT64_MAX );
	Dev_EndScope( abandoned );
	DevTools_BeginFrame( true );
	assert( DevTools_CpuTimings( &timings ) == 0 );
	const uint64_t backwards = Dev_BeginScope( "clock adjustment" );
	clockValue = 0;
	Dev_EndScope( backwards );
	DevTools_BeginFrame( true );
	assert( DevTools_CpuTimings( &timings ) == 1 && timings[0].microseconds == 0 );
	DevTools_Packet( false, 120 );
	DevTools_Packet( true, 40 );
	DevTools_Packet( true, 50 );
	DevTools_Snapshot( 123, true );
	Dev_PredictionError( 2.5f );
	const devNetwork_t *network = DevTools_Network();
	assert( network->bytes[0] == 120 && network->bytes[1] == 90 );
	assert( network->packets[0] == 1 && network->packets[1] == 2 && network->lastPacket[1] == 50 );
	assert( network->snapshots == 1 && network->snapshotBits == 123 && network->delta );
	assert( network->predictionError == 2.5f && network->predictions == 1 );
	DevTools_BeginFrame( false );
	DevTools_Packet( false, 99 );
	DevTools_Snapshot( 999, false );
	Dev_PredictionError( 100 );
	assert( network->bytes[0] == 120 && network->snapshots == 1 && network->predictions == 1 );
}

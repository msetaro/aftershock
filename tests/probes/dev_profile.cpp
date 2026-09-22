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

cvar_t *cl_shownet;
void QDECL Com_Printf( const char *, ... ) {
}
void QDECL Com_DPrintf( const char *, ... ) {
}

static void checkNetworkFields() {
	DevTools_BeginFrame( true );
	DevTools_ClearNetwork();
	for ( uint32_t i = 0; i < 300; ++i ) {
		com_frameTime = (int)i;
		DevTools_Packet( ( i & 1 ) != 0, i + 20 );
	}
	const auto *packet = DevTools_NetworkPacket( 0 );
	assert( packet && packet->bytes == 319 && packet->outgoing && packet->milliseconds == 299 );
	assert( DevTools_NetworkPacket( 255 )->bytes == 64 && !DevTools_NetworkPacket( 256 ) );
	byte data[4096] = {}, disabled[4096] = {};
	msg_t msg;
	entityState_t baseline = {}, entity = {}, decodedEntity = {};
	playerState_t playerBase = {}, player = {}, decodedPlayer = {};
	entity.number = 3;
	entity.pos.trTime = 123;
	entity.pos.trBase[0] = 12.5f;
	entity.weapon = 2;
	player.commandTime = 55;
	player.origin[0] = -1.5f;
	player.stats[0] = 100;
	player.ammo[2] = 19;
	MSG_Init( &msg, data, sizeof( data ) );
	MSG_WriteDeltaEntity( &msg, &baseline, &entity, qtrue );
	MSG_WriteDeltaPlayerstate( &msg, &playerBase, &player );
	const int size = msg.cursize, bits = msg.bit;
	MSG_BeginReading( &msg );
	assert( MSG_ReadEntitynum( &msg ) == 3 );
	MSG_ReadDeltaEntity( &msg, &baseline, &decodedEntity, 3 );
	MSG_ReadDeltaPlayerstate( &msg, &playerBase, &decodedPlayer );
	assert( !memcmp( &entity, &decodedEntity, sizeof( entity ) ) );
	assert( !memcmp( &player, &decodedPlayer, sizeof( player ) ) );
	const devNetworkField_t *fields;
	const uint32_t count = DevTools_NetworkFields( &fields );
	uint64_t total = 0;
	bool position = false, arrays = false;
	for ( uint32_t i = 0; i < count; ++i ) {
		if ( !fields[i].samples[0] && !fields[i].samples[1] )
			continue;
		assert( fields[i].bits[0] == fields[i].bits[1] && fields[i].samples[0] == fields[i].samples[1] );
		total += fields[i].bits[0];
		position |= !strcmp( fields[i].name, "entity.pos.trBase[0]" );
		arrays |= !strcmp( fields[i].name, "player.ammo" );
	}
	assert( total > 0 && total < (uint32_t)bits && position && arrays );
	DevTools_BeginFrame( false );
	MSG_Init( &msg, disabled, sizeof( disabled ) );
	MSG_WriteDeltaEntity( &msg, &baseline, &entity, qtrue );
	MSG_WriteDeltaPlayerstate( &msg, &playerBase, &player );
	assert( msg.cursize == size && msg.bit == bits && !memcmp( data, disabled, size ) );
	uint64_t after = 0;
	for ( uint32_t i = 0; i < count; ++i )
		after += fields[i].bits[1];
	assert( after == total );
	DevTools_ClearNetwork();
	assert( !DevTools_NetworkPacket( 0 ) && !DevTools_Network()->packets[0] );
	DevTools_ClearCpuHistory();
}

int main() {
	checkNetworkFields();
	clockValue = 0;
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
	clockValue = 1000; // Waiting between frames is not an executed CPU scope.
	DevTools_BeginFrame( true );
	assert( DevTools_CpuTimings( &timings ) == 2 );
	assert( timings[0].microseconds == 50 && timings[1].microseconds == 20 );
	assert( timings[0].parent == UINT32_MAX && timings[1].parent == 0 );
	assert( timings[0].selfMicroseconds == 30 && timings[1].selfMicroseconds == 20 );
	const devCpuFrame_t *frame = DevTools_CpuFrame( 0 );
	assert( frame && frame->count == 2 && frame->microseconds == 50 && !frame->dropped );
	assert( !DevTools_CpuFrame( 1 ) );
	assert( DevTools_CpuPeak() && DevTools_CpuPeak()->serial == frame->serial );
	for ( int i = 0; i < 128; ++i )
		assert( Dev_BeginScope( "capacity" ) != UINT64_MAX );
	assert( Dev_BeginScope( "overflow" ) == UINT64_MAX );
	Dev_EndScope( abandoned );
	DevTools_BeginFrame( true );
	assert( DevTools_CpuTimings( &timings ) == 0 );
	assert( DevTools_CpuFrame( 0 )->dropped == 1 );
	assert( DevTools_CpuFrame( 1 )->count == 2 );
	assert( DevTools_CpuPeak()->microseconds == 50 );
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
	Dev_PredictionError( 0.5f );
	assert( network->predictionPeak == 2.5f && network->predictionSum == 3.0 && network->predictions == 2 );
	Dev_PredictionError( -1 );
	Dev_PredictionError( NAN );
	assert( network->predictions == 2 );
	Dev_RewindReport( 100, 200, 0, 1 );
	Dev_RewindReport( 200, 200, 1, 0 );
	assert( network->rewindReports == 2 && network->rewindHits == 1 && network->rewindClamped == 1 );
	assert( network->rewindAge == 200 && network->rewindLimit == 200 );
	Dev_RewindReport( 300, 200, 0, 0 );
	assert( network->rewindReports == 2 );
	DevTools_BeginFrame( false );
	DevTools_Packet( false, 99 );
	DevTools_Snapshot( 999, false );
	Dev_PredictionError( 100 );
	Dev_RewindReport( 100, 200, 0, 1 );
	assert( network->rewindReports == 2 );
	assert( network->bytes[0] == 120 && network->snapshots == 1 && network->predictions == 2 );
	DevTools_ClearCpuHistory();
	assert( !DevTools_CpuFrame( 0 ) && !DevTools_CpuPeak() );
	DevTools_BeginFrame( true );
	const uint64_t spike = Dev_BeginScope( "retained spike" );
	clockValue += 100;
	Dev_EndScope( spike );
	DevTools_BeginFrame( true );
	const uint32_t peakSerial = DevTools_CpuPeak()->serial;
	for ( int i = 0; i < 300; ++i ) {
		const uint64_t steady = Dev_BeginScope( "steady" );
		++clockValue;
		Dev_EndScope( steady );
		DevTools_BeginFrame( true );
	}
	assert( DevTools_CpuFrame( 239 ) && !DevTools_CpuFrame( 240 ) );
	assert( DevTools_CpuFrame( 0 )->microseconds == 1 );
	assert( DevTools_CpuPeak()->serial == peakSerial && DevTools_CpuPeak()->microseconds == 100 );
	Dev_BeginScope( "abandoned parent" );
	const uint64_t survivor = Dev_BeginScope( "finished child" );
	clockValue += 5;
	Dev_EndScope( survivor );
	DevTools_BeginFrame( true );
	assert( DevTools_CpuTimings( &timings ) == 1 );
	assert( timings[0].parent == UINT32_MAX && timings[0].selfMicroseconds == 5 );
}

#include "../../engine/sound/snd_voice.h"
#include "../../engine/sound/snd_public.h"
#include "../../engine/qcommon/qcommon_public.h"
#include "../../engine/qcommon/voice_public.h"
#include <assert.h>
#include <cmath>
#include <stdlib.h>
#include <initializer_list>

cvar_t *cl_shownet;
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}
void QDECL Com_Printf( const char *, ... ) {
}
void QDECL Com_DPrintf( const char *, ... ) {
}

static bool playing;
static int allocations, frees;
extern "C" void *__real_malloc( size_t );
extern "C" void *__real_calloc( size_t, size_t );
extern "C" void *__real_realloc( void *, size_t );
extern "C" void *__wrap_malloc( size_t size ) {
	assert( !playing );
	return __real_malloc( size );
}
extern "C" void *__wrap_calloc( size_t count, size_t size ) {
	assert( !playing );
	return __real_calloc( count, size );
}
extern "C" void *__wrap_realloc( void *ptr, size_t size ) {
	assert( !playing );
	return __real_realloc( ptr, size );
}
void *Z_Malloc( size_t size ) {
	assert( !playing );
	++allocations;
	return calloc( 1, size );
}
void Z_Free( void *ptr ) {
	++frees;
	free( ptr );
}

int main() {
	byte storage[16384];
	msg_t message;
	voicePacket_t sent = {}, received;
	sent.sender = 7;
	sent.generation = 3;
	sent.sequence = 99;
	sent.frames = 1;
	sent.size = 3;
	sent.flags = 2;
	sent.targets[7] = 128;
	sent.data[0] = 17;
	for ( bool client : { false, true } ) {
		MSG_Init( &message, storage, sizeof( storage ) );
		assert( MSG_WriteVoice( &message, sent, client ) );
		MSG_BeginReading( &message );
		assert( MSG_ReadByte( &message ) == ( client ? int( clc_voipOpus ) : int( svc_voipOpus ) ) );
		assert( MSG_ReadVoice( &message, &received, client ) );
		assert( received.generation == 3 && received.sequence == 99 && received.frames == 1 && received.size == 3 && received.flags == 2 );
		assert( received.data[0] == 17 && ( client ? received.targets[7] == 128 : received.sender == 7 ) );
		message.cursize -= 2;
		MSG_BeginReading( &message );
		MSG_ReadByte( &message );
		assert( !MSG_ReadVoice( &message, &received, client ) );
	}
	MSG_Init( &message, storage, 16 );
	assert( !MSG_WriteVoice( &message, sent, true ) && message.cursize == 0 );
	assert( S_VoiceInit() );
	playing = true;
	int16_t tone[960];
	byte packet[256];
	float output[960][2];
	double energy = 0;
	for ( uint32_t sequence = 0; sequence < 200; ++sequence ) {
		for ( uint32_t i = 0; i < 960; ++i )
			tone[i] = int16_t( 8000 * std::sin( ( sequence * 960 + i ) * 6.283185307 * 440 / 48000 ) );
		const int size = S_VoiceEncode( tone, packet, sizeof( packet ) );
		assert( size > 0 && size <= 256 );
		assert( S_VoiceReceive( 1, 2, sequence, 1, packet, size ) );
		assert( !S_VoiceReceive( 1, 2, sequence, 1, packet, size ) );
		memset( output, 0, sizeof( output ) );
		S_VoiceMix( output, 960, 48000 );
		for ( const auto &sample : output ) {
			assert( sample[0] == sample[1] && std::isfinite( sample[0] ) );
			energy += sample[0] * sample[0];
		}
	}
	assert( energy > 1e12 );
	const int size = S_VoiceEncode( tone, packet, sizeof( packet ) );
	assert( !S_VoiceReceive( 64, 2, 200, 1, packet, size ) );
	assert( !S_VoiceReceive( 1, 2, 200, 0, packet, size ) );
	assert( !S_VoiceReceive( 1, 2, 200, 2, packet, size ) ); // Duration must match the wire header.
	assert( !S_VoiceReceive( 1, 2, 200, 1, packet, 0 ) );
	assert( S_VoiceReceive( 1, 2, 201, 1, packet, size ) ); // One missing 20 ms frame: PLC.
	sVoiceStats_t stats;
	S_VoiceStats( &stats );
	assert( stats.decoded == 201 && stats.concealed == 1 && stats.rejected >= 204 );
	// A fast sender cannot grow a queue; every speaker has a fixed 120 ms ring.
	for ( uint32_t sequence = 202; sequence < 230; ++sequence )
		assert( S_VoiceReceive( 1, 2, sequence, 1, packet, size ) );
	S_VoiceStats( &stats );
	assert( stats.queued <= 5760 && stats.overruns > 0 );
	int16_t threeFrames[2880];
	for ( int i = 0; i < 2880; ++i )
		threeFrames[i] = tone[i % 960];
	const int threeSize = S_VoiceEncode( threeFrames, packet, sizeof( packet ), 3 );
	assert( threeSize > 0 && S_VoiceReceive( 2, 1, 0, 3, packet, threeSize ) );
	S_VoiceReset();
	memset( output, 0, sizeof( output ) );
	S_VoiceMix( output, 960, 44100 );
	assert( output[100][0] == 0 );
	playing = false;
	S_VoiceShutdown();
	assert( allocations == frees && allocations == 1 );
	puts( "PASS: Opus voice roundtrip, duplicate rejection, PLC, bounded queue and allocation-free codec/mix" );
}

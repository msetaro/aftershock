#include "snd_local.h"
#include "snd_codec.h"
#include "snd_event.h"
#include <algorithm>
#include <cmath>
#include <inttypes.h>

// Prepared samples belong to the sound-registration lifetime, never the frame loop.
// ponytail: 128 events/512 samples/64 MiB; increase measured content budgets before adding eviction.
static struct {
	char name[MAX_QPATH];
	sSoundEvent_t definition;
	sEventPCM_t pcm[4];
} events[128];
static struct {
	char name[MAX_QPATH];
	sEventPCM_t pcm;
	uint32_t bytes;
} samples[512];
static uint32_t eventCount, sampleCount, sampleBytes, started;
static uint64_t mixed;
static float peak;
static sEventMixer_t mixer;
static cvar_t *hrtf, *headRadius, *busVolumes[S_BUS_COUNT];
static vec3_t listener, right = { 0, -1, 0 };
static int listenerEntity;
static struct {
	vec3_t origin, velocity;
	uint32_t time;
	bool valid;
} entities[MAX_GENTITIES];
static struct {
	vec3_t origin;
	int entity;
	bool fixed;
} sources[96];

bool S_AuthoredHandle( sfxHandle_t handle ) {
	return handle >= 65536 && handle < 65536 + 128;
}

static void Info() {
	Com_Printf( "Audio events: events=%u samples=%u bytes=%u active=%u started=%u mixed=%" PRIu64 " peak=%.3f\n",
		eventCount, sampleCount, sampleBytes, mixer.active, started, mixed, double( peak ) );
}

void S_AuthoredInit() {
	hrtf = Cvar_Get( "s_hrtf", "0", CVAR_ARCHIVE );
	Cvar_CheckRange( hrtf, "0", "1", CV_INTEGER );
	Cvar_SetDescription( hrtf, "Use approximate spherical-head headphone spatialization for authored sound events." );
	headRadius = Cvar_Get( "s_headRadius", "0.0875", CVAR_ARCHIVE );
	Cvar_CheckRange( headRadius, "0.05", "0.15", CV_FLOAT );
	Cvar_SetDescription( headRadius, "Head radius in meters for the spherical-head audio model." );
	static const char *names[S_BUS_COUNT] = { "s_busWeapons", "s_busAmbient", "s_busMusic", "s_busVoice", "s_busUI" };
	for ( uint32_t bus = 0; bus < S_BUS_COUNT; ++bus ) {
		busVolumes[bus] = Cvar_Get( names[bus], "1", CVAR_ARCHIVE );
		Cvar_CheckRange( busVolumes[bus], "0", "1", CV_FLOAT );
	}
	Cmd_AddCommand( "s_audioInfo", Info );
}

void S_AuthoredClear() {
	mixer = {};
	memset( sources, 0, sizeof( sources ) );
	memset( entities, 0, sizeof( entities ) );
	started = 0;
	mixed = 0;
	peak = 0.0f;
}

void S_AuthoredShutdown() {
	S_AuthoredClear();
	for ( uint32_t i = 0; i < sampleCount; ++i )
		Z_Free( (void *)samples[i].pcm.samples );
	memset( samples, 0, sizeof( samples ) );
	memset( events, 0, sizeof( events ) );
	eventCount = sampleCount = sampleBytes = 0;
	Cmd_RemoveCommand( "s_audioInfo" );
}

static int PrepareSample( const char *name ) {
	for ( uint32_t i = 0; i < sampleCount; ++i )
		if ( !strcmp( samples[i].name, name ) )
			return int( i );
	if ( sampleCount == 512 )
		return -1;
	auto *stream = S_CodecOpenStream( name );
	if ( !stream )
		return -1;
	const auto info = stream->info;
	if ( info.width != 2 || ( info.channels != 1 && info.channels != 2 ) || info.rate < 8000 || info.rate > 192000 ||
		 info.samples <= 0 || info.size <= 0 || info.size > 16 * 1024 * 1024 ||
		 uint64_t( info.samples ) * info.channels * 2 != uint32_t( info.size ) ||
		 uint32_t( info.size ) > 64u * 1024u * 1024u - sampleBytes ) {
		S_CodecCloseStream( stream );
		return -1;
	}
	auto *pcm = (int16_t *)Z_Malloc( size_t( info.size ) );
	int offset = 0;
	while ( offset < info.size ) {
		const int count = S_CodecReadStream( stream, info.size - offset, (byte *)pcm + offset );
		if ( count <= 0 )
			break;
		offset += count;
	}
	S_CodecCloseStream( stream );
	if ( offset != info.size ) {
		Z_Free( pcm );
		return -1;
	}
	auto &sample = samples[sampleCount];
	Q_strncpyz( sample.name, name, sizeof( sample.name ) );
	sample.pcm = { pcm, uint32_t( info.samples ), uint32_t( info.rate ), uint32_t( info.channels ) };
	sample.bytes = uint32_t( info.size );
	sampleBytes += sample.bytes;
	return int( sampleCount++ );
}

sfxHandle_t S_AuthoredRegister( const char *name ) {
	for ( uint32_t i = 0; i < eventCount; ++i )
		if ( !strcmp( events[i].name, name ) )
			return 65536 + int( i );
	if ( eventCount == 128 ) {
		Com_Printf( "Audio event rejected: event cache capacity: %s\n", name );
		return 0;
	}
	fileHandle_t file;
	const int length = FS_FOpenFileRead( name, &file, qtrue );
	if ( file == FS_INVALID_HANDLE ) {
		Com_Printf( "Audio event rejected: missing %s\n", name );
		return 0;
	}
	byte data[448];
	const bool read = length == sizeof( data ) && FS_Read( data, sizeof( data ), file ) == sizeof( data );
	FS_FCloseFile( file );
	auto &event = events[eventCount];
	if ( !read || !S_ReadSoundEvent( data, sizeof( data ), &event.definition ) ) {
		Com_Printf( "Audio event rejected: cooked record %s\n", name );
		return 0;
	}
	const uint32_t first = sampleCount;
	for ( uint32_t layer = 0; layer < event.definition.layerCount; ++layer ) {
		const int index = PrepareSample( event.definition.layers[layer].sample );
		if ( index < 0 ) {
			// Roll back newly prepared samples; existing registered events keep their storage.
			while ( sampleCount > first ) {
				auto &sample = samples[--sampleCount];
				Z_Free( (void *)sample.pcm.samples );
				sampleBytes -= sample.bytes;
				sample = {};
			}
			event = {};
			Com_Printf( "Audio event rejected: PCM resource or cache capacity: %s\n", name );
			return 0;
		}
		event.pcm[layer] = samples[index].pcm;
	}
	Q_strncpyz( event.name, name, sizeof( event.name ) );
	return 65536 + int( eventCount++ );
}

void S_AuthoredEntityPosition( int entity, const vec3_t origin ) {
	if ( entity < 0 || entity >= MAX_GENTITIES )
		return;
	auto &state = entities[entity];
	const uint32_t now = uint32_t( s_soundtime ), elapsed = now - state.time;
	if ( state.valid && elapsed && elapsed < uint32_t( dma.speed ) ) {
		for ( int axis = 0; axis < 3; ++axis )
			state.velocity[axis] = ( origin[axis] - state.origin[axis] ) * dma.speed / elapsed;
	} else if ( !state.valid || elapsed >= uint32_t( dma.speed ) ) {
		VectorClear( state.velocity );
	}
	VectorCopy( origin, state.origin );
	state.time = now;
	state.valid = true;
}

static sSpatialInput_t Spatial( const vec3_t origin, int entity, bool fixed ) {
	sSpatialInput_t input = {};
	input.speedOfSound = 13504.0f;
	VectorCopy( right, input.right );
	if ( entity != listenerEntity )
		VectorSubtract( origin, listener, input.offset );
	if ( !fixed && entity >= 0 && entity < MAX_GENTITIES )
		VectorCopy( entities[entity].velocity, input.sourceVelocity );
	if ( listenerEntity >= 0 && listenerEntity < MAX_GENTITIES )
		VectorCopy( entities[listenerEntity].velocity, input.listenerVelocity );
	return input;
}

void S_AuthoredStart( const vec3_t origin, int entity, sfxHandle_t handle ) {
	if ( !S_AuthoredHandle( handle ) || uint32_t( handle - 65536 ) >= eventCount )
		return;
	const auto &event = events[handle - 65536];
	vec3_t position;
	if ( origin )
		VectorCopy( origin, position );
	else if ( entity >= 0 && entity < MAX_GENTITIES )
		VectorCopy( entities[entity].origin, position );
	else
		return;
	const auto spatial = Spatial( position, entity, origin != nullptr );
	const int slot = S_StartEventVoice( &mixer, &event.definition, event.pcm, spatial, hrtf->integer != 0, dma.speed, headRadius->value );
	if ( slot < 0 )
		return;
	VectorCopy( position, sources[slot].origin );
	sources[slot].entity = entity;
	sources[slot].fixed = origin != nullptr;
	++started;
}

void S_AuthoredRespatialize( int entity, const vec3_t head, vec3_t axis[3] ) {
	listenerEntity = entity;
	VectorCopy( head, listener );
	VectorNegate( axis[1], right );
	for ( uint32_t i = 0; i < 96; ++i ) {
		auto &voice = mixer.voices[i];
		if ( !voice.event )
			continue;
		auto &source = sources[i];
		if ( !source.fixed && source.entity >= 0 && source.entity < MAX_GENTITIES )
			VectorCopy( entities[source.entity].origin, source.origin );
		if ( !S_UpdateEventVoice( &voice, Spatial( source.origin, source.entity, source.fixed ), dma.speed, headRadius->value ) ) {
			voice.event = nullptr;
			--mixer.active;
		}
	}
}

void S_AuthoredPaint( portable_samplepair_t *paint, int frames, float volume ) {
	if ( !mixer.active || frames <= 0 || frames > PAINTBUFFER_SIZE )
		return;
	volume = std::isfinite( volume ) ? std::clamp( volume, 0.0f, 127.0f ) : 0.0f;
	static float output[PAINTBUFFER_SIZE][2];
	memset( output, 0, size_t( frames ) * sizeof( output[0] ) );
	for ( uint32_t bus = 0; bus < S_BUS_COUNT; ++bus )
		mixer.busGain[bus] = busVolumes[bus]->value;
	S_MixEvents( &mixer, output, uint32_t( frames ), dma.speed );
	mixed += uint32_t( frames );
	for ( int frame = 0; frame < frames; ++frame ) {
		peak = std::max( peak, std::max( std::fabs( output[frame][0] ), std::fabs( output[frame][1] ) ) );
		paint[frame].left = int( std::clamp( double( paint[frame].left ) + output[frame][0] * volume, -2147483647.0, 2147483647.0 ) );
		paint[frame].right = int( std::clamp( double( paint[frame].right ) + output[frame][1] * volume, -2147483647.0, 2147483647.0 ) );
	}
}

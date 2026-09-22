#include "snd_local.h"
#include "snd_codec.h"
#include "snd_event.h"
#include "snd_voice.h"
#include "../qcommon/cm_public.h"
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
static float peak, wetPeak;
static sEventMixer_t mixer;
static sReverb_t reverb;
static struct {
	vec3_t mins, maxs;
	float wet, decay, damping;
} zones[32];
static uint32_t zoneCount, traceCursor, traced, blocked;
static int zoneIndex = -2;

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

// Decode only at preparation. Loops read a retained, uncompressed temporary file.
// ponytail: four 256 MiB streams and synchronous buffered reads; add I/O prefetch if measured stalls require it.
static struct audioStream_t {
	char name[MAX_QPATH], ioBuffer[4096];
	int16_t pcm[4096 * 2];
	fileHandle_t file;
	uint32_t frames, rate, channels, first, count;
	double cursor;
	sBus_t bus;
	bool active, loop;
} streams[4];
static uint32_t streamPrepared, streamLoops, streamReads, streamFailures;

static void ClearStreams() {
	for ( auto &stream : streams ) {
		if ( stream.file )
			FS_FCloseFile( stream.file );
		stream = {};
	}
	streamPrepared = streamLoops = streamReads = streamFailures = 0;
}

static bool PrepareStream( audioStream_t &stream, const char *name ) {
	if ( stream.file && !strcmp( stream.name, name ) )
		return true;
	if ( stream.file )
		FS_FCloseFile( stream.file );
	stream = {};
	if ( strlen( name ) >= MAX_QPATH || Q_stricmp( COM_GetExtension( name ), "wav" ) )
		return false;
	stream.file = FS_OpenTemporaryFile( stream.ioBuffer, sizeof( stream.ioBuffer ) );
	if ( !stream.file )
		return false;
	auto *source = S_CodecOpenStream( name );
	bool ready = false;
	if ( source ) {
		const auto info = source->info;
		if ( info.width == 2 && ( info.channels == 1 || info.channels == 2 ) &&
			 info.rate >= 8000 && info.rate <= 192000 && info.samples > 0 && info.size > 0 &&
			 info.size <= 256 * 1024 * 1024 && uint64_t( info.samples ) * info.channels * 2 == uint32_t( info.size ) ) {
			int remaining = info.size;
			while ( remaining > 0 ) {
				const int count = std::min( remaining, int( sizeof( stream.pcm ) ) );
				if ( S_CodecReadStream( source, count, stream.pcm ) != count || FS_Write( stream.pcm, count, stream.file ) != count )
					break;
				remaining -= count;
			}
			ready = remaining == 0 && FS_Seek( stream.file, 0, FS_SEEK_SET ) == 0;
			stream.frames = uint32_t( info.samples );
			stream.rate = uint32_t( info.rate );
			stream.channels = uint32_t( info.channels );
		}
		S_CodecCloseStream( source );
	}
	if ( !ready ) {
		FS_FCloseFile( stream.file );
		stream = {};
		return false;
	}
	Q_strncpyz( stream.name, name, sizeof( stream.name ) );
	++streamPrepared;
	return true;
}

static bool StreamSample( audioStream_t &stream, uint32_t index, float out[2] ) {
	if ( index < stream.first || index >= stream.first + stream.count ) {
		stream.first = index / 4096 * 4096;
		stream.count = std::min( 4096u, stream.frames - stream.first );
		const int bytes = int( stream.count * stream.channels * 2 );
		++streamReads;
		if ( FS_Seek( stream.file, stream.first * stream.channels * 2, FS_SEEK_SET ) != 0 ||
			 FS_Read( stream.pcm, bytes, stream.file ) != bytes ) {
			stream.active = false;
			stream.count = 0;
			++streamFailures;
			return false;
		}
	}
	const uint32_t offset = ( index - stream.first ) * stream.channels;
	out[0] = stream.pcm[offset];
	out[1] = stream.pcm[offset + stream.channels - 1];
	return true;
}

static bool StreamsActive() {
	for ( const auto &stream : streams )
		if ( stream.active )
			return true;
	return false;
}

static void MixStreams( sBusFrame_t *input, int frames ) {
	for ( auto &stream : streams ) {
		for ( int frame = 0; frame < frames && stream.active; ++frame ) {
			if ( stream.cursor >= stream.frames ) {
				if ( !stream.loop ) {
					stream.active = false;
					break;
				}
				streamLoops += uint32_t( stream.cursor / stream.frames );
				stream.cursor = std::fmod( stream.cursor, double( stream.frames ) );
			}
			const uint32_t index = uint32_t( stream.cursor );
			const uint32_t next = index + 1 < stream.frames ? index + 1 : ( stream.loop ? 0 : index );
			const float fraction = float( stream.cursor - index );
			float a[2], b[2];
			if ( !StreamSample( stream, index, a ) || !StreamSample( stream, next, b ) )
				break;
			for ( int ear = 0; ear < 2; ++ear )
				input[frame].samples[stream.bus][ear] += a[ear] + fraction * ( b[ear] - a[ear] );
			stream.cursor += double( stream.rate ) / dma.speed;
		}
	}
}

static int StreamSlot() {
	const char *text = Cmd_Argv( 1 );
	return text[0] >= '0' && text[0] <= '3' && !text[1] ? text[0] - '0' : -1;
}

static void PlayStream() {
	const int slot = StreamSlot();
	const char *bus = Cmd_Argv( 2 ), *loop = Cmd_Argv( 4 );
	if ( Cmd_Argc() != 5 || slot < 0 || ( strcmp( bus, "music" ) && strcmp( bus, "ambient" ) ) ||
		 ( strcmp( loop, "0" ) && strcmp( loop, "1" ) ) ) {
		Com_Printf( "Usage: s_stream <0..3> <music|ambient> <sample.wav> <0|1 loop>\n" );
		return;
	}
	auto &stream = streams[slot];
	if ( !PrepareStream( stream, Cmd_Argv( 3 ) ) ) {
		++streamFailures;
		Com_Printf( "Audio stream rejected: %s\n", Cmd_Argv( 3 ) );
		return;
	}
	stream.cursor = 0;
	stream.bus = !strcmp( bus, "music" ) ? S_BUS_MUSIC : S_BUS_AMBIENT;
	stream.loop = loop[0] == '1';
	stream.active = true;
}

static void StopStream() {
	const int slot = StreamSlot();
	if ( Cmd_Argc() == 2 && slot >= 0 )
		streams[slot].active = false;
	else
		Com_Printf( "Usage: s_streamStop <0..3>\n" );
}

static void StreamInfo() {
	uint32_t active = 0, bytes = 0, buffers = 0;
	for ( const auto &stream : streams ) {
		active += stream.active;
		if ( stream.file ) {
			bytes += stream.frames * stream.channels * 2;
			buffers += sizeof( stream.pcm ) + sizeof( stream.ioBuffer );
		}
	}
	Com_Printf( "Audio streams: prepared=%u active=%u bytes=%u buffers=%u loops=%u reads=%u failures=%u\n",
		streamPrepared, active, bytes, buffers, streamLoops, streamReads, streamFailures );
}

static void VoiceInfo() {
	sVoiceStats_t stats;
	S_VoiceStats( &stats );
	Com_Printf( "Audio voice: encoded=%u decoded=%u rejected=%u concealed=%u overruns=%u queued=%u\n",
		stats.encoded, stats.decoded, stats.rejected, stats.concealed, stats.overruns, stats.queued );
}

static void Info() {
	Com_Printf( "Audio events: events=%u samples=%u bytes=%u active=%u started=%u mixed=%" PRIu64 " peak=%.3f zones=%u zone=%d wet=%.3f traced=%u blocked=%u wetPeak=%.3f layers=%" PRIu64 ",%" PRIu64 ",%" PRIu64 "\n",
		eventCount, sampleCount, sampleBytes, mixer.active, started, mixed, double( peak ), zoneCount, zoneIndex, double( reverb.wet ), traced, blocked, double( wetPeak ), mixer.layerFrames[0], mixer.layerFrames[1], mixer.layerFrames[2] );
}

static void PlayAt() {
	if ( Cmd_Argc() != 5 ) {
		Com_Printf( "Usage: s_event <event.asevt> <x> <y> <z>\n" );
		return;
	}
	vec3_t origin;
	for ( int i = 0; i < 3; ++i ) {
		char *end;
		const char *text = Cmd_Argv( i + 2 );
		origin[i] = strtof( text, &end );
		if ( end == text || *end || !std::isfinite( origin[i] ) || std::fabs( origin[i] ) > 32000 ) {
			Com_Printf( "Audio event rejected: position\n" );
			return;
		}
	}
	const auto handle = S_AuthoredRegister( Cmd_Argv( 1 ) );
	if ( handle )
		S_AuthoredStart( origin, ENTITYNUM_WORLD, handle );
}

void S_AuthoredInit() {
	if ( !S_VoiceInit() )
		Com_Printf( "Opus voice initialization failed\n" );
	Cmd_AddCommand( "s_voiceInfo", VoiceInfo );
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
	Cmd_AddCommand( "s_event", PlayAt );
	Cmd_AddCommand( "s_stream", PlayStream );
	Cmd_AddCommand( "s_streamStop", StopStream );
	Cmd_AddCommand( "s_streamInfo", StreamInfo );
}

void S_AuthoredClear() {
	S_VoiceReset();
	ClearStreams();
	mixer = {};
	memset( sources, 0, sizeof( sources ) );
	memset( entities, 0, sizeof( entities ) );
	started = 0;
	mixed = 0;
	peak = wetPeak = 0.0f;
	memset( &reverb, 0, sizeof( reverb ) );
	S_ConfigureReverb( &reverb, dma.speed, 0.0f, 1.0f, 0.5f );
	zoneIndex = -2;
	traceCursor = traced = blocked = 0;
}

void S_AuthoredShutdown() {
	S_AuthoredClear();
	S_VoiceShutdown();
	Cmd_RemoveCommand( "s_voiceInfo" );
	for ( uint32_t i = 0; i < sampleCount; ++i )
		Z_Free( (void *)samples[i].pcm.samples );
	memset( samples, 0, sizeof( samples ) );
	memset( events, 0, sizeof( events ) );
	eventCount = sampleCount = sampleBytes = 0;
	Cmd_RemoveCommand( "s_audioInfo" );
	Cmd_RemoveCommand( "s_event" );
	Cmd_RemoveCommand( "s_stream" );
	Cmd_RemoveCommand( "s_streamStop" );
	Cmd_RemoveCommand( "s_streamInfo" );
	hrtf = nullptr;
	zoneCount = 0;
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

void S_LoadWorldAudio() {
	if ( !hrtf )
		return;
	zoneCount = 0;
	zoneIndex = -2;
	const char *cursor = CM_EntityString();
	if ( !cursor || strcmp( COM_Parse( &cursor ), "{" ) )
		return;
	for ( ;; ) {
		char key[64];
		Q_strncpyz( key, COM_Parse( &cursor ), sizeof( key ) );
		if ( !key[0] || !strcmp( key, "}" ) )
			break;
		const char *value = COM_Parse( &cursor );
		if ( strncmp( key, "audio_zone_", 11 ) )
			continue;
		if ( zoneCount == 32 ) {
			Com_Printf( "Audio zone rejected: capacity\n" );
			continue;
		}
		auto &zone = zones[zoneCount];
		char extra;
		bool valid = sscanf( value, "%f %f %f %f %f %f %f %f %f %c",
						 &zone.mins[0], &zone.mins[1], &zone.mins[2], &zone.maxs[0], &zone.maxs[1], &zone.maxs[2],
						 &zone.wet, &zone.decay, &zone.damping, &extra ) == 9;
		for ( int axis = 0; axis < 3; ++axis )
			valid &= std::isfinite( zone.mins[axis] ) && std::isfinite( zone.maxs[axis] ) &&
					 zone.mins[axis] >= -32000 && zone.maxs[axis] <= 32000 && zone.mins[axis] < zone.maxs[axis];
		valid &= std::isfinite( zone.wet ) && zone.wet >= 0 && zone.wet <= 1 &&
				 std::isfinite( zone.decay ) && zone.decay >= .1f && zone.decay <= 10 &&
				 std::isfinite( zone.damping ) && zone.damping >= 0 && zone.damping <= .95f;
		if ( valid )
			++zoneCount;
		else
			Com_Printf( "Audio zone rejected: %s\n", key );
	}
	Com_Printf( "Audio world: zones=%u\n", zoneCount );
}

void S_AuthoredRespatialize( int entity, const vec3_t head, vec3_t axis[3] ) {
	listenerEntity = entity;
	VectorCopy( head, listener );
	VectorNegate( axis[1], right );
	int selected = -1;
	for ( uint32_t i = 0; i < zoneCount; ++i ) {
		bool inside = true;
		for ( int component = 0; component < 3; ++component )
			inside &= head[component] >= zones[i].mins[component] && head[component] < zones[i].maxs[component];
		if ( inside ) {
			selected = int( i );
			break;
		}
	}
	if ( selected != zoneIndex ) {
		if ( selected >= 0 ) {
			const auto &zone = zones[selected];
			S_ConfigureReverb( &reverb, dma.speed, zone.wet, zone.decay, zone.damping );
		} else
			S_ConfigureReverb( &reverb, dma.speed, 0.0f, 1.0f, 0.5f );
		zoneIndex = selected;
	}

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
	// ponytail: eight round-robin static-world traces per frame; dynamic occluders need a game trace service.
	const uint32_t first = traceCursor;
	uint32_t budget = 8;
	for ( uint32_t step = 0; budget && step < 96 && CM_NumInlineModels(); ++step ) {
		const uint32_t index = ( first + step ) % 96;
		auto &voice = mixer.voices[index];
		if ( !voice.event || !( voice.event->flags & S_EVENT_OCCLUSION ) || sources[index].entity == listenerEntity )
			continue;
		trace_t trace;
		CM_BoxTrace( &trace, listener, sources[index].origin, vec3_origin, vec3_origin, 0, CONTENTS_SOLID, qfalse );
		voice.occlusion = trace.fraction < 1.0f ? 1.0f : 0.0f;
		++traced;
		if ( voice.occlusion > 0 )
			++blocked;
		traceCursor = ( index + 1 ) % 96;
		--budget;
	}
}

void S_AuthoredPaint( portable_samplepair_t *paint, int frames, float volume ) {
	if ( ( !mixer.active && !reverb.remaining && !StreamsActive() && !S_VoiceActive() ) || frames <= 0 || frames > PAINTBUFFER_SIZE )
		return;
	volume = std::isfinite( volume ) ? std::clamp( volume, 0.0f, 127.0f ) : 0.0f;
	static float output[PAINTBUFFER_SIZE][2], sends[PAINTBUFFER_SIZE];
	static sBusFrame_t input[PAINTBUFFER_SIZE];
	static float voice[PAINTBUFFER_SIZE][2];
	memset( voice, 0, size_t( frames ) * sizeof( voice[0] ) );
	S_VoiceMix( voice, uint32_t( frames ), dma.speed );
	memset( input, 0, size_t( frames ) * sizeof( input[0] ) );
	MixStreams( input, frames );
	for ( int frame = 0; frame < frames; ++frame ) {
		input[frame].samples[S_BUS_VOICE][0] = voice[frame][0];
		input[frame].samples[S_BUS_VOICE][1] = voice[frame][1];
	}
	memset( sends, 0, size_t( frames ) * sizeof( sends[0] ) );
	memset( output, 0, size_t( frames ) * sizeof( output[0] ) );
	for ( uint32_t bus = 0; bus < S_BUS_COUNT; ++bus )
		mixer.busGain[bus] = busVolumes[bus]->value;
	S_MixEvents( &mixer, output, uint32_t( frames ), dma.speed, sends, input );
	mixed += uint32_t( frames );
	for ( int frame = 0; frame < frames; ++frame ) {
		float wet[2];
		S_ReverbSample( &reverb, sends[frame], wet );
		wetPeak = std::max( wetPeak, std::max( std::fabs( wet[0] ), std::fabs( wet[1] ) ) );
		output[frame][0] += wet[0];
		output[frame][1] += wet[1];
		peak = std::max( peak, std::max( std::fabs( output[frame][0] ), std::fabs( output[frame][1] ) ) );
		paint[frame].left = int( std::clamp( double( paint[frame].left ) + output[frame][0] * volume, -2147483647.0, 2147483647.0 ) );
		paint[frame].right = int( std::clamp( double( paint[frame].right ) + output[frame][1] * volume, -2147483647.0, 2147483647.0 ) );
	}
}

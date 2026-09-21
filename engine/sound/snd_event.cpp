#include "snd_event.h"
#include "../../third_party/sha256/sha-256.h"
#include <cmath>
#include <algorithm>
#include <string.h>

struct soundEventHeader_t {
	char magic[8];
	uint32_t version, size;
	uint8_t hash[32];
};
static_assert( sizeof( soundEventHeader_t ) == 48 && offsetof( soundEventHeader_t, hash ) == 16 );

static bool Path( const char *path, size_t size ) {
	const char *end = (const char *)memchr( path, 0, size );
	if ( !end || end == path || path[0] == '/' || strstr( path, ".." ) )
		return false;
	for ( const char *c = path; c < end; ++c )
		if ( !( ( *c >= 'a' && *c <= 'z' ) || ( *c >= '0' && *c <= '9' ) || *c == '_' || *c == '-' || *c == '/' || *c == '.' ) )
			return false;
	return true;
}

static bool Range( float value, float low, float high ) {
	return std::isfinite( value ) && value >= low && value <= high;
}

bool S_ReadSoundEvent( const void *data, size_t size, sSoundEvent_t *event ) {
	if ( !event )
		return false;
	*event = {};
	if ( !data || size != sizeof( soundEventHeader_t ) + sizeof( *event ) )
		return false;
	soundEventHeader_t header;
	memcpy( &header, data, sizeof( header ) );
	if ( memcmp( header.magic, "ASEVENT\0", 8 ) || header.version != 1 || header.size != sizeof( *event ) )
		return false;
	const uint8_t *payload = (const uint8_t *)data + sizeof( header );
	uint8_t hash[32];
	calc_sha_256( hash, payload, sizeof( *event ) );
	if ( memcmp( hash, header.hash, sizeof( hash ) ) )
		return false;
	sSoundEvent_t candidate;
	memcpy( &candidate, payload, sizeof( candidate ) );
	if ( !Path( candidate.name, sizeof( candidate.name ) ) || candidate.bus >= S_BUS_COUNT || candidate.group > 15 ||
		 candidate.priority > 255 || !candidate.voiceLimit || candidate.voiceLimit > 96 || candidate.model > S_DISTANCE_INVERSE ||
		 ( candidate.flags & ~3u ) || !candidate.layerCount || candidate.layerCount > 4 || candidate.reserved ||
		 !Range( candidate.referenceDistance, 0.01f, 65535.0f ) || !Range( candidate.maxDistance, 1.0f, 65536.0f ) ||
		 candidate.maxDistance <= candidate.referenceDistance || !Range( candidate.rolloff, 0.0f, 16.0f ) || !Range( candidate.reverbSend, 0.0f, 1.0f ) )
		return false;
	for ( uint32_t i = 0; i < candidate.layerCount; ++i ) {
		const auto &layer = candidate.layers[i];
		if ( !Path( layer.sample, sizeof( layer.sample ) ) || layer.role > S_LAYER_DISTANT ||
			 !Range( layer.gain, 0.0f, 2.0f ) || !Range( layer.minDistance, 0.0f, candidate.maxDistance ) ||
			 !Range( layer.maxDistance, 1.0f, candidate.maxDistance ) || layer.maxDistance <= layer.minDistance )
			return false;
		const size_t length = strlen( layer.sample );
		if ( length < 5 || strcmp( layer.sample + length - 4, ".wav" ) )
			return false;
	}
	*event = candidate;
	return true;
}

float S_EventLayerGain( const sSoundLayer_t &layer, float distance ) {
	return distance >= layer.minDistance && distance < layer.maxDistance ? layer.gain : 0.0f;
}

bool S_UpdateEventVoice( sEventVoice_t *voice, const sSpatialInput_t &spatial, int rate, float headRadius ) {
	if ( !voice || !voice->event )
		return false;
	auto input = spatial;
	input.referenceDistance = voice->event->referenceDistance;
	input.maxDistance = voice->event->maxDistance;
	input.rolloff = voice->event->rolloff;
	input.model = voice->event->model;
	if ( !S_CalculateSpatial( input, &voice->spatial ) )
		return false;
	if ( !( voice->event->flags & S_EVENT_DOPPLER ) )
		voice->spatial.pitch = 1.0f;
	if ( voice->binaural ) {
		const float left = voice->spatial.left * voice->spatial.left;
		const float right = voice->spatial.right * voice->spatial.right;
		const float pan = left + right > 0.0f ? ( right - left ) / ( left + right ) : 0.0f;
		if ( !S_ConfigureHrtf( std::clamp( pan, -1.0f, 1.0f ), rate, headRadius, &voice->hrtf ) )
			return false;
	}
	return true;
}

int S_StartEventVoice( sEventMixer_t *mixer, const sSoundEvent_t *event, const sEventPCM_t pcm[4],
	const sSpatialInput_t &spatial, bool binaural, int rate, float headRadius ) {
	if ( !mixer || !event || !pcm || !event->layerCount || event->layerCount > 4 ||
		 event->bus >= S_BUS_COUNT || !event->voiceLimit || event->voiceLimit > 96 || rate < 8000 || rate > 192000 )
		return -1;
	for ( uint32_t i = 0; i < event->layerCount; ++i )
		if ( !pcm[i].samples || !pcm[i].frames || pcm[i].frames > 8u * 1024u * 1024u ||
			 pcm[i].rate < 8000 || pcm[i].rate > 192000 || ( pcm[i].channels != 1 && pcm[i].channels != 2 ) )
			return -1;
	// Validate coefficients before replacing a live voice.
	sEventVoice_t prepared = {};
	prepared.event = event;
	prepared.binaural = binaural;
	if ( !S_UpdateEventVoice( &prepared, spatial, rate, headRadius ) )
		return -1;
	uint32_t groupCount = 0;
	int freeSlot = -1;
	for ( int i = 0; i < 96; ++i ) {
		const auto *playing = mixer->voices[i].event;
		if ( !playing )
			freeSlot = i;
		else if ( playing->bus == event->bus && playing->group == event->group )
			++groupCount;
	}
	int selected = groupCount < event->voiceLimit ? freeSlot : -1;
	if ( selected < 0 ) {
		for ( int i = 0; i < 96; ++i ) {
			const auto &voice = mixer->voices[i];
			if ( !voice.event || voice.event->priority > event->priority )
				continue;
			if ( groupCount >= event->voiceLimit && ( voice.event->bus != event->bus || voice.event->group != event->group ) )
				continue;
			if ( selected < 0 || voice.event->priority < mixer->voices[selected].event->priority ||
				 ( voice.event->priority == mixer->voices[selected].event->priority && voice.sequence < mixer->voices[selected].sequence ) )
				selected = i;
		}
	}
	if ( selected < 0 )
		return -1;
	if ( mixer->voices[selected].event )
		++mixer->stolen;
	else
		++mixer->active;
	memcpy( prepared.pcm, pcm, sizeof( prepared.pcm ) );
	prepared.sequence = ++mixer->sequence;
	prepared.tail = binaural ? uint32_t( rate / 20 ) : 0;
	mixer->voices[selected] = prepared;
	return selected;
}

static float MonoSample( const sEventPCM_t &pcm, uint32_t frame ) {
	if ( frame >= pcm.frames )
		return 0.0f;
	const int16_t *samples = pcm.samples + frame * pcm.channels;
	return pcm.channels == 2 ? ( float( samples[0] ) + samples[1] ) * 0.5f : samples[0];
}

void S_MixEvents( sEventMixer_t *mixer, float ( *output )[2], uint32_t frames, int rate, float *reverbSend ) {
	if ( !mixer || !output || rate < 8000 || rate > 192000 )
		return;
	if ( !mixer->active ) {
		mixer->duck = 0.0f;
		return;
	}
	const float closedAlpha = 1.0f - std::exp( -2.0f * 3.14159265f * 1200.0f / rate );
	for ( uint32_t frame = 0; frame < frames; ++frame ) {
		float buses[S_BUS_COUNT][2] = {};
		float sends[S_BUS_COUNT] = {};
		bool voiceAudible = false;
		for ( auto &voice : mixer->voices ) {
			if ( !voice.event )
				continue;
			float sample = 0.0f;
			bool playing = false;
			for ( uint32_t layer = 0; layer < voice.event->layerCount; ++layer ) {
				const auto &pcm = voice.pcm[layer];
				if ( voice.cursor[layer] >= pcm.frames )
					continue;
				playing = true;
				const uint32_t index = uint32_t( voice.cursor[layer] );
				const float fraction = float( voice.cursor[layer] - index );
				const float a = MonoSample( pcm, index ), b = MonoSample( pcm, index + 1 );
				sample += ( a + fraction * ( b - a ) ) * S_EventLayerGain( voice.event->layers[layer], voice.spatial.distance );
				voice.cursor[layer] += double( pcm.rate ) * voice.spatial.pitch / rate;
			}
			if ( !playing && !voice.tail ) {
				voice.event = nullptr;
				--mixer->active;
				continue;
			}
			if ( !playing )
				--voice.tail;
			voice.occlusionSmooth += ( voice.occlusion - voice.occlusionSmooth ) / ( rate * 0.03f );
			const float alpha = 1.0f - voice.occlusionSmooth * ( 1.0f - closedAlpha );
			voice.filtered += alpha * ( sample - voice.filtered );
			sample = voice.filtered * ( 1.0f - 0.65f * voice.occlusionSmooth );
			float ears[2];
			if ( voice.binaural ) {
				S_HrtfSample( voice.hrtf, &voice.history, sample, ears );
				const float gain = std::sqrt( ( voice.spatial.left * voice.spatial.left + voice.spatial.right * voice.spatial.right ) * 0.5f );
				ears[0] *= gain;
				ears[1] *= gain;
			} else {
				ears[0] = sample * voice.spatial.left;
				ears[1] = sample * voice.spatial.right;
			}
			const auto bus = voice.event->bus;
			buses[bus][0] += ears[0];
			buses[bus][1] += ears[1];
			sends[bus] += ( ears[0] + ears[1] ) * 0.5f * voice.event->reverbSend;
			voiceAudible |= bus == S_BUS_VOICE && ( std::fabs( ears[0] ) + std::fabs( ears[1] ) > 1.0f );
		}
		// Fast voice attack, slower release; a zeroed mixer begins unducked.
		const float target = voiceAudible ? 1.0f : 0.0f;
		const float seconds = voiceAudible ? 0.005f : 0.25f;
		mixer->duck += ( target - mixer->duck ) / ( rate * seconds );
		for ( uint32_t bus = 0; bus < S_BUS_COUNT; ++bus ) {
			float gain = std::isfinite( mixer->busGain[bus] ) ? std::clamp( mixer->busGain[bus], 0.0f, 1.0f ) : 0.0f;
			if ( bus == S_BUS_MUSIC )
				gain *= 1.0f - mixer->duck * 0.65f;
			else if ( bus == S_BUS_AMBIENT )
				gain *= 1.0f - mixer->duck * 0.5f;
			output[frame][0] += buses[bus][0] * gain;
			output[frame][1] += buses[bus][1] * gain;
			if ( reverbSend )
				reverbSend[frame] += sends[bus] * gain;
		}
	}
}

bool S_ConfigureReverb( sReverb_t *state, int rate, float wet, float decay, float damping ) {
	if ( !state || rate < 8000 || rate > 192000 || !Range( wet, 0.0f, 1.0f ) ||
		 !Range( decay, 0.1f, 10.0f ) || !Range( damping, 0.0f, 0.95f ) )
		return false;
	if ( state->rate != uint32_t( rate ) )
		memset( state, 0, sizeof( *state ) );
	state->rate = uint32_t( rate );
	state->targetWet = wet;
	state->wetStep = 1.0f / ( rate * 0.05f );
	state->damping = damping;
	state->tail = uint32_t( ( decay + 0.1f ) * rate );
	// ponytail: four damped parallel combs; authored room coloration, not convolution acoustics.
	static const float seconds[4] = { 0.0297f, 0.0371f, 0.0411f, 0.0437f };
	for ( uint32_t i = 0; i < 4; ++i ) {
		state->length[i] = uint32_t( seconds[i] * rate );
		state->feedback[i] = std::pow( 0.001f, float( state->length[i] ) / ( rate * decay ) );
	}
	return true;
}

void S_ReverbSample( sReverb_t *state, float input, float output[2] ) {
	output[0] = output[1] = 0.0f;
	if ( !state || !state->rate )
		return;
	if ( std::fabs( input ) > 1e-10f )
		state->remaining = state->tail;
	else if ( state->remaining )
		--state->remaining;
	state->wet += ( state->targetWet - state->wet ) * state->wetStep;
	for ( uint32_t i = 0; i < 4; ++i ) {
		float &cell = state->lines[i][state->cursor[i]];
		const float delayed = cell;
		state->filtered[i] += ( delayed - state->filtered[i] ) * ( 1.0f - state->damping );
		cell = input * 0.25f + state->filtered[i] * state->feedback[i];
		if ( std::fabs( cell ) < 1e-20f )
			cell = 0;
		output[i & 1u] += delayed * state->wet * 0.5f;
		state->cursor[i] = ( state->cursor[i] + 1u ) % state->length[i];
	}
}

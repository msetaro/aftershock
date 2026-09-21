#ifndef SND_EVENT_H
#define SND_EVENT_H

#include "snd_spatial.h"
#include <stddef.h>
#include <type_traits>

enum sBus_t : uint32_t { S_BUS_WEAPONS,
	S_BUS_AMBIENT,
	S_BUS_MUSIC,
	S_BUS_VOICE,
	S_BUS_UI,
	S_BUS_COUNT };
enum sLayerRole_t : uint32_t { S_LAYER_MECHANICAL,
	S_LAYER_TAIL,
	S_LAYER_DISTANT };
enum : uint32_t { S_EVENT_DOPPLER = 1,
	S_EVENT_OCCLUSION = 2 };

struct sSoundLayer_t {
	char sample[64];
	sLayerRole_t role;
	float gain, minDistance, maxDistance;
};

struct sSoundEvent_t {
	char name[32];
	sBus_t bus;
	uint32_t group, priority, voiceLimit;
	sDistanceModel_t model;
	uint32_t flags, layerCount;
	float referenceDistance, maxDistance, rolloff, reverbSend;
	uint32_t reserved;
	sSoundLayer_t layers[4];
};

static_assert( sizeof( sSoundLayer_t ) == 80 && offsetof( sSoundLayer_t, gain ) == 68 );
static_assert( sizeof( sSoundEvent_t ) == 400 && offsetof( sSoundEvent_t, layers ) == 80 );
static_assert( std::is_trivially_copyable_v<sSoundEvent_t> );

bool S_ReadSoundEvent( const void *data, size_t size, sSoundEvent_t *event );
float S_EventLayerGain( const sSoundLayer_t &layer, float distance );

struct sEventPCM_t {
	const int16_t *samples;
	uint32_t frames, rate, channels;
};

struct sEventVoice_t {
	const sSoundEvent_t *event;
	sEventPCM_t pcm[4];
	double cursor[4];
	sSpatialOutput_t spatial;
	sHrtfParameters_t hrtf;
	sHrtfState_t history;
	uint64_t sequence;
	uint32_t tail;
	bool binaural;
};

struct sEventMixer_t {
	sEventVoice_t voices[96];
	float busGain[S_BUS_COUNT] = { 1, 1, 1, 1, 1 };
	float duck;
	uint64_t sequence;
	uint32_t active, stolen;
};
static_assert( std::is_trivially_destructible_v<sEventMixer_t> );

// Event and prepared PCM storage must remain alive until the voice retires.
int S_StartEventVoice( sEventMixer_t *mixer, const sSoundEvent_t *event, const sEventPCM_t pcm[4],
	const sSpatialInput_t &spatial, bool binaural, int rate, float headRadius );
bool S_UpdateEventVoice( sEventVoice_t *voice, const sSpatialInput_t &spatial, int rate, float headRadius );
// Add PCM-unit stereo output. No allocation, file access or device calls.
void S_MixEvents( sEventMixer_t *mixer, float ( *output )[2], uint32_t frames, int rate );

#endif

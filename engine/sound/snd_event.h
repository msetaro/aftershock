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

#endif

#ifndef PERCEPTION_PUBLIC_H
#define PERCEPTION_PUBLIC_H
#include "../sound/sound_model_public.h"
#include <stddef.h>
#include <type_traits>

struct aiSenseSettings_t {
	float sightRange, cosineHalfFov, hearingThreshold;
	uint32_t memoryMilliseconds;
};
struct aiCandidate_t {
	float position[3], loudness, referenceDistance, maxDistance, rolloff;
	int32_t entity;
	sDistanceModel_t model;
	bool alive, hostile, clearSight, blockedSound;
};
struct aiSenseState_t {
	int32_t target;
	uint32_t age, initialized;
	float position[3];
};
struct aiObservation_t {
	int32_t target;
	float position[3], gain;
	bool visible, heard;
};
struct aiCoverPoint_t {
	float position[3];
	int32_t identity;
	bool reachable, protectedFromThreat;
};
static_assert( sizeof( aiSenseState_t ) == 24 && offsetof( aiSenseState_t, position ) == 12 );
static_assert( std::is_trivially_copyable_v<aiSenseState_t> );
// At most 64 candidates. Collision facts come from the game trace service;
// loudness is zero unless a current game sound event exists. No allocation.
aiObservation_t AI_Sense( const aiSenseSettings_t &settings, const float eye[3], const float forward[3],
	const aiCandidate_t *candidates, uint32_t count, uint32_t milliseconds, aiSenseState_t *state );
// Caller supplies nav-reachable points and current threat occlusion, at most 256.
// Returns the stable point identity, or -1. Ties use the lower identity.
int32_t AI_SelectCover( const float position[3], const aiCoverPoint_t *points, uint32_t count, float range, float out[3] );
#endif

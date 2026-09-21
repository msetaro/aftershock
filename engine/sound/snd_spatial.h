// Authored audio only; legacy channel spatialization retains its original math.
#ifndef SND_SPATIAL_H
#define SND_SPATIAL_H

#include <stdint.h>

enum sDistanceModel_t : uint32_t {
	S_DISTANCE_LINEAR,
	S_DISTANCE_INVERSE
};

struct sSpatialInput_t {
	float offset[3]; // Source minus listener, in engine inches.
	float right[3]; // Unit listener right axis.
	float sourceVelocity[3]; // Engine inches per second.
	float listenerVelocity[3];
	float referenceDistance;
	float maxDistance;
	float rolloff;
	float speedOfSound;
	sDistanceModel_t model;
};

struct sSpatialOutput_t {
	float left, right; // Equal-power amplitude gains, [0,1].
	float pitch; // Bounded playback-rate multiplier, [0.5,2].
	float distance;
};

// Invalid input returns false and a silent, unit-pitch output.
bool S_CalculateSpatial( const sSpatialInput_t &input, sSpatialOutput_t *output );

#endif

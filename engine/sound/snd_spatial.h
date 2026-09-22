// Authored audio only; legacy channel spatialization retains its original math.
#ifndef SND_SPATIAL_H
#define SND_SPATIAL_H

#include "sound_model_public.h"

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

struct sHrtfParameters_t {
	float delay[2], b0[2], b1[2], a1;
};

struct sHrtfState_t {
	float history[512];
	float previousInput[2], previousOutput[2];
	uint32_t cursor;
};

// Spherical head approximation: horizontal delay/shadow, no pinna/elevation model.
// Configure outside the sample loop. Reset state to zero when recycling a voice.
// Pan is [-1,1], rate [8000,192000] Hz, radius [0.05,0.15] meters.
bool S_ConfigureHrtf( float pan, int rate, float radius, sHrtfParameters_t *parameters );
// parameters must come from a successful Configure call; input is finite PCM.
void S_HrtfSample( const sHrtfParameters_t &parameters, sHrtfState_t *state, float sample, float ears[2] );

#endif

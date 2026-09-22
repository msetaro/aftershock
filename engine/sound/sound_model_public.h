#ifndef SOUND_MODEL_PUBLIC_H
#define SOUND_MODEL_PUBLIC_H
#include <algorithm>
#include <stdint.h>

enum sDistanceModel_t : uint32_t { S_DISTANCE_LINEAR,
	S_DISTANCE_INVERSE };
// Shared by authored audio and AI hearing. Inputs are validated by callers;
// referenceDistance > 0, maxDistance > referenceDistance, rolloff >= 0, and
// distance < maxDistance. Both users apply the same hard audible-range cutoff.
inline double S_DistanceGain( double distance, float referenceDistance, float maxDistance, float rolloff, sDistanceModel_t model ) {
	const double beyond = (std::max)( 0.0, distance - referenceDistance );
	if ( model == S_DISTANCE_LINEAR )
		return (std::max)( 0.0, 1.0 - rolloff * beyond / ( double( maxDistance ) - referenceDistance ) );
	return referenceDistance / ( referenceDistance + rolloff * beyond );
}
// Occlusion is [0,1]. Audio smooths this input; hearing uses the settled gain.
inline float S_OcclusionGain( float occlusion ) {
	return 1.0f - 0.65f * occlusion;
}
#endif

#include "perception_public.h"
#include <cmath>
#include <cstring>

static bool Point( const float *point ) {
	if ( !point )
		return false;
	for ( int i = 0; i < 3; ++i )
		if ( !std::isfinite( point[i] ) || std::fabs( point[i] ) > 1048576 )
			return false;
	return true;
}
static double Squared( const float *a, const float *b ) {
	double sum = 0;
	for ( int i = 0; i < 3; ++i ) {
		const double delta = double( a[i] ) - b[i];
		sum += delta * delta;
	}
	return sum;
}
aiObservation_t AI_Sense( const aiSenseSettings_t &settings, const float eye[3], const float forward[3],
	const aiCandidate_t *candidates, uint32_t count, uint32_t milliseconds, aiSenseState_t *state ) {
	aiObservation_t result{};
	result.target = -1;
	if ( !state || !Point( eye ) || !Point( forward ) || ( !candidates && count ) || count > 64 || !milliseconds || milliseconds > 1000 ||
		 !std::isfinite( settings.sightRange ) || settings.sightRange <= 0 || settings.sightRange > 1048576 ||
		 !std::isfinite( settings.cosineHalfFov ) || settings.cosineHalfFov < -1 || settings.cosineHalfFov > 1 ||
		 !std::isfinite( settings.hearingThreshold ) || settings.hearingThreshold <= 0 || settings.hearingThreshold > 16 ||
		 settings.memoryMilliseconds > 600000 )
		return result;
	constexpr float zero[3] = {};
	if ( std::fabs( Squared( forward, zero ) - 1 ) > 0.001 )
		return result;
	if ( !state->initialized )
		*state = { -1, settings.memoryMilliseconds, 1, {} };
	state->age = state->age > UINT32_MAX - milliseconds ? UINT32_MAX : state->age + milliseconds;
	int visible = -1, heard = -1;
	double nearest = 0;
	float loudest = 0;
	for ( uint32_t i = 0; i < count; ++i ) {
		const auto &candidate = candidates[i];
		if ( candidate.entity < 0 || !candidate.alive || !candidate.hostile || !Point( candidate.position ) ) {
			if ( candidate.entity == state->target )
				state->target = -1;
			continue;
		}
		const double distance = std::sqrt( Squared( eye, candidate.position ) );
		double facing = 0;
		for ( int axis = 0; axis < 3; ++axis )
			facing += ( double( candidate.position[axis] ) - eye[axis] ) * forward[axis];
		if ( candidate.clearSight && distance <= settings.sightRange && facing >= distance * settings.cosineHalfFov &&
			 ( visible < 0 || distance < nearest || ( distance == nearest && candidate.entity < candidates[visible].entity ) ) ) {
			visible = int( i );
			nearest = distance;
		}
		if ( !std::isfinite( candidate.loudness ) || candidate.loudness <= 0 || candidate.loudness > 16 ||
			 !std::isfinite( candidate.referenceDistance ) || candidate.referenceDistance <= 0 ||
			 !std::isfinite( candidate.maxDistance ) || candidate.maxDistance <= candidate.referenceDistance ||
			 !std::isfinite( candidate.rolloff ) || candidate.rolloff < 0 ||
			 ( candidate.model != S_DISTANCE_LINEAR && candidate.model != S_DISTANCE_INVERSE ) || distance >= candidate.maxDistance )
			continue;
		const float gain = float( S_DistanceGain( distance, candidate.referenceDistance, candidate.maxDistance, candidate.rolloff, candidate.model ) ) *
						   candidate.loudness * S_OcclusionGain( candidate.blockedSound ? 1.0f : 0.0f );
		if ( gain >= settings.hearingThreshold && ( heard < 0 || gain > loudest || ( gain == loudest && candidate.entity < candidates[heard].entity ) ) ) {
			heard = int( i );
			loudest = gain;
		}
	}
	const int selected = visible >= 0 ? visible : heard;
	if ( selected >= 0 ) {
		state->target = candidates[selected].entity;
		state->age = 0;
		std::memcpy( state->position, candidates[selected].position, sizeof( state->position ) );
		result.visible = visible >= 0;
		result.heard = visible < 0;
		result.gain = result.heard ? loudest : 0;
	} else if ( state->age >= settings.memoryMilliseconds )
		state->target = -1;
	result.target = state->target;
	if ( result.target >= 0 )
		std::memcpy( result.position, state->position, sizeof( result.position ) );
	return result;
}
int32_t AI_SelectCover( const float position[3], const aiCoverPoint_t *points, uint32_t count, float range, float out[3] ) {
	if ( !Point( position ) || ( !points && count ) || count > 256 || !out || !std::isfinite( range ) || range <= 0 || range > 1048576 )
		return -1;
	int32_t identity = -1;
	double closest = double( range ) * range;
	for ( uint32_t i = 0; i < count; ++i ) {
		const auto &point = points[i];
		if ( point.identity < 0 || !point.reachable || !point.protectedFromThreat || !Point( point.position ) )
			continue;
		const double distance = Squared( position, point.position );
		if ( distance > closest || ( distance == closest && identity >= 0 && point.identity >= identity ) )
			continue;
		identity = point.identity;
		closest = distance;
		std::memcpy( out, point.position, sizeof( point.position ) );
	}
	return identity;
}

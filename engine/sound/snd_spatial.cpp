#include "snd_spatial.h"
#include <algorithm>
#include <cmath>

bool S_CalculateSpatial( const sSpatialInput_t &input, sSpatialOutput_t *output ) {
	if ( !output )
		return false;
	*output = { 0.0f, 0.0f, 1.0f, 0.0f };
	if ( !std::isfinite( input.referenceDistance ) || input.referenceDistance <= 0.0f ||
		 !std::isfinite( input.maxDistance ) || input.maxDistance <= input.referenceDistance ||
		 !std::isfinite( input.rolloff ) || input.rolloff < 0.0f ||
		 !std::isfinite( input.speedOfSound ) || input.speedOfSound <= 0.0f ||
		 ( input.model != S_DISTANCE_LINEAR && input.model != S_DISTANCE_INVERSE ) )
		return false;
	double squared = 0.0, rightSquared = 0.0;
	for ( int i = 0; i < 3; ++i ) {
		if ( !std::isfinite( input.offset[i] ) || !std::isfinite( input.right[i] ) ||
			 !std::isfinite( input.sourceVelocity[i] ) || !std::isfinite( input.listenerVelocity[i] ) )
			return false;
		squared += double( input.offset[i] ) * input.offset[i];
		rightSquared += double( input.right[i] ) * input.right[i];
	}
	if ( std::fabs( rightSquared - 1.0 ) > 0.001 )
		return false;
	const double distance = std::sqrt( squared );
	// Audible range caps both models, including inverse attenuation's infinite tail.
	output->distance = float( std::min( distance, double( input.maxDistance ) ) );
	if ( distance >= input.maxDistance )
		return true;
	double pan = 0.0, sourceRadial = 0.0, listenerRadial = 0.0;
	if ( distance > 0.0 ) {
		for ( int i = 0; i < 3; ++i ) {
			const double direction = input.offset[i] / distance;
			pan += direction * input.right[i];
			sourceRadial += direction * input.sourceVelocity[i];
			listenerRadial += direction * input.listenerVelocity[i];
		}
	}
	pan = std::clamp( pan, -1.0, 1.0 );
	const double beyond = std::max( 0.0, distance - input.referenceDistance );
	double gain;
	if ( input.model == S_DISTANCE_LINEAR )
		gain = std::max( 0.0, 1.0 - input.rolloff * beyond / ( double( input.maxDistance ) - input.referenceDistance ) );
	else
		gain = input.referenceDistance / ( input.referenceDistance + input.rolloff * beyond );
	output->left = float( gain * std::sqrt( ( 1.0 - pan ) * 0.5 ) );
	output->right = float( gain * std::sqrt( ( 1.0 + pan ) * 0.5 ) );
	// Clamp radial velocities before division so sonic crossings stay finite.
	const double limit = input.speedOfSound * 0.9;
	sourceRadial = std::clamp( sourceRadial, -limit, limit );
	listenerRadial = std::clamp( listenerRadial, -limit, limit );
	output->pitch = float( std::clamp( ( input.speedOfSound + listenerRadial ) / ( input.speedOfSound + sourceRadial ), 0.5, 2.0 ) );
	return true;
}

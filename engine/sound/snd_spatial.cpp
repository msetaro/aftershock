#include "snd_spatial.h"
#include <algorithm>
#include <cmath>

// Brown/Duda (1998), equations 2-5, spherical head component only:
// https://users.umiacs.umd.edu/~ramanid/cmsc828d_audio/BrownDuda.pdf
bool S_ConfigureHrtf( float pan, int rate, float radius, sHrtfParameters_t *parameters ) {
	if ( !parameters )
		return false;
	*parameters = {};
	if ( !std::isfinite( pan ) || pan < -1.0f || pan > 1.0f || rate < 8000 || rate > 192000 ||
		 !std::isfinite( radius ) || radius < 0.05f || radius > 0.15f )
		return false;
	constexpr double pi = 3.14159265358979323846;
	const double time = radius / 343.0;
	const double k = rate * time;
	parameters->a1 = float( ( 1.0 - k ) / ( 1.0 + k ) );
	for ( int ear = 0; ear < 2; ++ear ) {
		const double cosine = ear == 0 ? -pan : pan;
		const double theta = std::acos( cosine );
		const double alpha = 1.05 + 0.95 * std::cos( theta * 1.2 );
		parameters->b0[ear] = float( ( 1.0 + alpha * k ) / ( 1.0 + k ) );
		parameters->b1[ear] = float( ( 1.0 - alpha * k ) / ( 1.0 + k ) );
		const double relative = theta < pi * 0.5 ? -cosine : theta - pi * 0.5;
		// Common radius/c offset makes both delays causal. Maximum is <216 samples.
		parameters->delay[ear] = float( k * ( 1.0 + relative ) );
	}
	return true;
}

void S_HrtfSample( const sHrtfParameters_t &parameters, sHrtfState_t *state, float sample, float ears[2] ) {
	state->history[state->cursor] = sample;
	for ( int ear = 0; ear < 2; ++ear ) {
		const uint32_t delay = uint32_t( parameters.delay[ear] );
		const float fraction = parameters.delay[ear] - delay;
		const float recent = state->history[( state->cursor - delay ) & 511u];
		const float older = state->history[( state->cursor - delay - 1u ) & 511u];
		const float input = recent + fraction * ( older - recent );
		const float output = parameters.b0[ear] * input + parameters.b1[ear] * state->previousInput[ear] - parameters.a1 * state->previousOutput[ear];
		state->previousInput[ear] = input;
		// Silence tiny tails before they spend CPU time as denormals.
		state->previousOutput[ear] = std::fabs( output ) < 1e-20f ? 0.0f : output;
		ears[ear] = state->previousOutput[ear];
	}
	state->cursor = ( state->cursor + 1u ) & 511u;
}

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

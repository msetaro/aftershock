#include "../../engine/sound/snd_spatial.h"
#include <assert.h>
#include <cmath>
#include <limits>
#include <initializer_list>
#include <type_traits>

static_assert( std::is_trivially_destructible_v<sSpatialInput_t> );
static_assert( std::is_trivially_destructible_v<sSpatialOutput_t> );

static bool Near( float a, float b ) {
	return std::fabs( a - b ) < 0.0001f;
}

static void Binaural() {
	sHrtfParameters_t parameters;
	sHrtfState_t state = {};
	static_assert( std::is_trivially_destructible_v<sHrtfState_t> );
	assert( S_ConfigureHrtf( 1.0f, 48000, 0.0875f, &parameters ) );
	int first[2] = { -1, -1 };
	float energy[2] = {};
	for ( int i = 0; i < 512; ++i ) {
		float ears[2];
		S_HrtfSample( parameters, &state, i == 0 ? 1.0f : 0.0f, ears );
		for ( int ear = 0; ear < 2; ++ear ) {
			assert( std::isfinite( ears[ear] ) );
			energy[ear] += ears[ear] * ears[ear];
			if ( first[ear] == -1 && std::fabs( ears[ear] ) > 0.00001f )
				first[ear] = i;
		}
	}
	assert( first[1] == 0 && first[0] >= 30 && first[0] <= 32 );
	assert( energy[1] > energy[0] * 2.0f ); // Far ear has high-frequency shadow.
	sHrtfParameters_t mirror;
	sHrtfState_t a = {}, b = {};
	assert( S_ConfigureHrtf( -1.0f, 48000, 0.0875f, &mirror ) );
	for ( int i = 0; i < 2048; ++i ) {
		float left[2], right[2];
		const float sample = std::sin( float( i ) * 0.5f );
		S_HrtfSample( parameters, &a, sample, left );
		S_HrtfSample( mirror, &b, sample, right );
		assert( Near( left[0], right[1] ) && Near( left[1], right[0] ) );
	}
	for ( int rate : { 8000, 44100, 48000, 96000, 192000 } ) {
		assert( S_ConfigureHrtf( 0.0f, rate, 0.15f, &parameters ) );
		state = {};
		for ( int i = 0; i < 4096; ++i ) {
			float ears[2];
			S_HrtfSample( parameters, &state, 0.25f, ears );
			assert( Near( ears[0], ears[1] ) );
			if ( i == 4095 )
				assert( Near( ears[0], 0.25f ) ); // Unity DC gain at both ears.
		}
	}
	assert( !S_ConfigureHrtf( 0.0f, 0, 0.0875f, &parameters ) );
	assert( !S_ConfigureHrtf( 2.0f, 48000, 0.0875f, &parameters ) );
	assert( !S_ConfigureHrtf( 0.0f, 48000, 1.0f, &parameters ) );
}

int main() {
	Binaural();
	sSpatialInput_t input = {};
	input.right[1] = -1.0f; // Engine listener convention: +Y is left.
	input.referenceDistance = 100.0f;
	input.maxDistance = 1000.0f;
	input.rolloff = 1.0f;
	input.speedOfSound = 13504.0f; // Engine inches per second, approximately 343 m/s.
	input.model = S_DISTANCE_LINEAR;
	sSpatialOutput_t output;
	assert( S_CalculateSpatial( input, &output ) );
	assert( Near( output.left, std::sqrt( 0.5f ) ) && Near( output.left, output.right ) );
	assert( output.pitch == 1.0f && output.distance == 0.0f );
	input.offset[1] = -100.0f;
	assert( S_CalculateSpatial( input, &output ) );
	assert( output.left == 0.0f && output.right == 1.0f );
	input.offset[1] = 100.0f;
	assert( S_CalculateSpatial( input, &output ) );
	assert( output.left == 1.0f && output.right == 0.0f );
	input.offset[1] = 550.0f;
	assert( S_CalculateSpatial( input, &output ) );
	assert( Near( output.left, 0.5f ) );
	input.model = S_DISTANCE_INVERSE;
	input.offset[1] = 200.0f;
	assert( S_CalculateSpatial( input, &output ) );
	assert( Near( output.left, 0.5f ) );
	input.sourceVelocity[1] = -1000.0f;
	assert( S_CalculateSpatial( input, &output ) );
	assert( output.pitch > 1.0f ); // Approaching source raises pitch.
	input.sourceVelocity[1] = 1000.0f;
	assert( S_CalculateSpatial( input, &output ) );
	assert( output.pitch < 1.0f );
	input.listenerVelocity[1] = 1000.0f;
	assert( S_CalculateSpatial( input, &output ) );
	assert( Near( output.pitch, 1.0f ) ); // Equal radial velocities cancel.
	input.offset[1] = 1001.0f;
	assert( S_CalculateSpatial( input, &output ) );
	assert( output.left == 0.0f && output.right == 0.0f );
	input.offset[1] = std::numeric_limits<float>::infinity();
	assert( !S_CalculateSpatial( input, &output ) );
	assert( output.left == 0.0f && output.right == 0.0f && output.pitch == 1.0f );
	input.offset[1] = 100.0f;
	input.maxDistance = input.referenceDistance;
	assert( !S_CalculateSpatial( input, &output ) );
}

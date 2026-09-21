#include "../../engine/sound/snd_spatial.h"
#include <assert.h>
#include <cmath>
#include <limits>
#include <type_traits>

static_assert( std::is_trivially_destructible_v<sSpatialInput_t> );
static_assert( std::is_trivially_destructible_v<sSpatialOutput_t> );

static bool Near( float a, float b ) {
	return std::fabs( a - b ) < 0.0001f;
}

int main() {
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

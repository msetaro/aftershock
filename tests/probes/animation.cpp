// Numerical/runtime contract before the portable animation implementation.
#include "../../engine/animation/animation_public.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <type_traits>

static_assert( std::is_trivially_copyable_v<animState_t> );
static_assert( std::is_trivially_destructible_v<animAsset_t> );
static_assert( std::is_trivially_destructible_v<animPose_t> );

static float distance( const float a[3], const float b[3] ) {
	float sum = 0;
	for ( int i = 0; i < 3; ++i )
		sum += ( a[i] - b[i] ) * ( a[i] - b[i] );
	return sqrtf( sum );
}

int main( int argc, char **argv ) {
	assert( argc == 2 );
	const animTransform_t identity = { { 0, 0, 0 }, { 0, 0, 0, 1 }, { 1, 1, 1 } };
	animTransform_t a[2] = { identity, identity }, b[2] = { identity, identity }, blended[2];
	b[0].translate[0] = 10;
	b[1].translate[2] = 4;
	b[1].scale[0] = 2;
	const float mask[2] = { 0, 1 };
	Anim_BlendTransforms( 2, a, b, mask, 0.5f, blended );
	assert( blended[0].translate[0] == 0 && blended[1].translate[2] == 2 );
	assert( blended[1].scale[0] == 1.5f );
	Anim_AdditiveTransforms( 2, a, b, a, mask, 0.25f, blended );
	assert( blended[0].translate[0] == 0 && blended[1].translate[2] == 1 );
	assert( blended[1].scale[0] == 1.25f );

	const float root[3] = { 0, 0, 0 }, elbow[3] = { 1, 0, 0 }, hand[3] = { 2, 0, 0 };
	const float target[3] = { 1, 1, 0 }, pole[3] = { 0, 0, 1 }, farTarget[3] = { 3, 0, 0 };
	float solvedElbow[3], solvedHand[3], rotation[4];
	assert( Anim_TwoBoneIK( root, elbow, hand, target, pole, solvedElbow, solvedHand ) );
	assert( fabsf( distance( root, solvedElbow ) - 1 ) < 0.00001f );
	assert( fabsf( distance( solvedElbow, solvedHand ) - 1 ) < 0.00001f );
	assert( distance( target, solvedHand ) < 0.00001f && solvedElbow[2] > 0 );
	assert( Anim_TwoBoneIK( root, elbow, hand, farTarget, pole, solvedElbow, solvedHand ) );
	assert( fabsf( distance( root, solvedHand ) - 2 ) < 0.00001f );
	const float forward[3] = { 1, 0, 0 }, left[3] = { 0, 1, 0 };
	assert( Anim_LookAt( forward, left, rotation ) );
	assert( fabsf( rotation[2] * rotation[3] - 0.5f ) < 0.00001f );

	FILE *file = fopen( argv[1], "rb" );
	assert( file && fseek( file, 0, SEEK_END ) == 0 );
	const int64_t length = ftell( file );
	assert( length > 0 && length < 16 * 1024 * 1024 );
	rewind( file );
	void *bytes = malloc( (size_t)length );
	assert( bytes && fread( bytes, 1, (size_t)length, file ) == (size_t)length );
	fclose( file );
	animAsset_t asset = {};
	assert( Anim_Open( bytes, (size_t)length, &asset ) );
	float parameters[ANIM_MAX_PARAMETERS];
	Anim_DefaultParameters( &asset, parameters );
	const int active = Anim_ParameterIndex( &asset, "active" );
	assert( active >= 0 && parameters[active] == 0 );
	animState_t state;
	animEvents_t events;
	animPose_t pose;
	Anim_Reset( &asset, 0, &state );
	assert( Anim_Tick( &asset, parameters, 0, &state, &events ) && events.count == 0 );
	assert( Anim_Tick( &asset, parameters, 250, &state, &events ) && events.count == 0 );
	assert( Anim_Evaluate( &asset, &state, parameters, 250, &pose ) );
	assert( pose.jointCount == 2 && fabsf( pose.local[0].translate[0] - 0.03125f ) < 0.00001f );
	parameters[active] = 1;
	assert( Anim_Tick( &asset, parameters, 500, &state, &events ) );
	assert( strcmp( Anim_StateName( &asset, state.current ), "wave" ) == 0 );
	assert( Anim_Evaluate( &asset, &state, parameters, 500, &pose ) );
	assert( fabsf( pose.local[0].translate[0] - 0.0625f ) < 0.00001f );
	assert( Anim_Evaluate( &asset, &state, parameters, 650, &pose ) );
	assert( fabsf( pose.local[0].translate[0] ) < 0.00001f );
	assert( Anim_Tick( &asset, parameters, 1000, &state, &events ) && events.count == 1 );
	assert( strcmp( Anim_EventName( &asset, events.items[0].id ), "marker" ) == 0 );
	assert( events.items[0].bone == Anim_BoneIndex( &asset, "tip" ) );
	assert( Anim_Evaluate( &asset, &state, parameters, 1000, &pose ) );
	assert( fabsf( pose.world[1][0] - 0.70710678f ) < 0.0001f );
	assert( fabsf( pose.world[1][8] - 0.70710678f ) < 0.0001f );
	assert( Anim_Tick( &asset, parameters, 1000, &state, &events ) && events.count == 0 );
	parameters[active] = 0;
	assert( Anim_Tick( &asset, parameters, 1500, &state, &events ) );
	assert( strcmp( Anim_StateName( &asset, state.current ), "idle" ) == 0 );
	free( bytes );
	puts( "PASS: masked/additive blending, two-bone/look-at IK, clip sampling, transitions and exactly-once events" );
}

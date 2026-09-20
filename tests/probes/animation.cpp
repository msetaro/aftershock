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

static void CheckRig( const animAsset_t *asset, const char *kind ) {
	float parameters[ANIM_MAX_PARAMETERS];
	Anim_DefaultParameters( asset, parameters );
	animState_t state;
	animEvents_t events;
	animPose_t pose, baseline;
	Anim_Reset( asset, 0, &state );
	assert( Anim_Tick( asset, parameters, 0, &state, &events ) );
	auto input = [&]( const char *name, float value ) {
		const int index = Anim_ParameterIndex( asset, name );
		assert( index >= 0 );
		parameters[index] = value;
	};
	auto tick = [&]( uint32_t time, const char *expected ) {
		assert( Anim_Tick( asset, parameters, time, &state, &events ) );
		assert( strcmp( Anim_StateName( asset, state.current ), expected ) == 0 );
	};
	if ( strcmp( kind, "rifle" ) == 0 ) {
		assert( Anim_BoneIndex( asset, "muzzle" ) >= 0 && Anim_BoneIndex( asset, "optic" ) >= 0 && Anim_BoneIndex( asset, "magazine" ) >= 0 );
		input( "ads", 1 );
		tick( 20, "ads" );
		assert( Anim_Evaluate( asset, &state, parameters, 200, &baseline ) );
		input( "fire", 1 );
		tick( 220, "fire" );
		assert( events.count == 1 && strcmp( Anim_EventName( asset, events.items[0].id ), "shot" ) == 0 );
		assert( events.items[0].bone == Anim_BoneIndex( asset, "muzzle" ) );
		input( "fire", 0 );
		tick( 240, "fire" );
		assert( events.count == 1 && strcmp( Anim_EventName( asset, events.items[0].id ), "shell_eject" ) == 0 );
		tick( 360, "ads" );
		input( "reload", 1 );
		tick( 380, "reload" );
		input( "reload", 0 );
		tick( 630, "reload" );
		assert( events.count == 1 && strcmp( Anim_EventName( asset, events.items[0].id ), "magazine_out" ) == 0 );
		assert( Anim_Evaluate( asset, &state, parameters, 630, &pose ) );
		const int magazine = Anim_BoneIndex( asset, "magazine" );
		assert( fabsf( pose.world[magazine][11] - baseline.world[magazine][11] ) > 0.1f );
		tick( 1030, "reload" );
		assert( events.count == 1 && strcmp( Anim_EventName( asset, events.items[0].id ), "magazine_in" ) == 0 );
		tick( 1230, "reload" );
		assert( events.count == 1 && strcmp( Anim_EventName( asset, events.items[0].id ), "bolt" ) == 0 );
		tick( 1380, "ads" );
		assert( events.count == 1 && strcmp( Anim_EventName( asset, events.items[0].id ), "reload_complete" ) == 0 );
		input( "ads", 0 );
		input( "sprint", 1 );
		tick( 1400, "sprint" );
		input( "jump", 1 );
		tick( 1420, "jump" );
		input( "jump", 0 );
		input( "sprint", 0 );
		tick( 2420, "idle" );
	} else {
		input( "move", 1 );
		tick( 20, "move" );
		tick( 270, "move" );
		assert( events.count == 1 && strcmp( Anim_EventName( asset, events.items[0].id ), "footstep" ) == 0 );
		assert( events.items[0].bone == Anim_BoneIndex( asset, "foot.L" ) );
		assert( Anim_Evaluate( asset, &state, parameters, 270, &baseline ) );
		input( "aim_up", 1 );
		assert( Anim_Evaluate( asset, &state, parameters, 270, &pose ) );
		const int foot = Anim_BoneIndex( asset, "foot.L" ), spine = Anim_BoneIndex( asset, "spine" ), pelvis = Anim_BoneIndex( asset, "pelvis" );
		assert( memcmp( baseline.world[foot], pose.world[foot], sizeof( pose.world[foot] ) ) == 0 );
		float difference = 0;
		for ( int i = 0; i < 12; ++i )
			difference += fabsf( baseline.world[spine][i] - pose.world[spine][i] );
		assert( difference > 0.1f );
		input( "crouch", 1 );
		assert( Anim_Evaluate( asset, &state, parameters, 270, &pose ) );
		assert( pose.world[pelvis][11] < baseline.world[pelvis][11] - 8 );
		input( "prone", 1 );
		assert( Anim_Evaluate( asset, &state, parameters, 270, &pose ) );
		assert( pose.world[pelvis][11] < baseline.world[pelvis][11] - 18 );
		tick( 770, "move" );
		assert( events.count == 1 && events.items[0].bone == Anim_BoneIndex( asset, "foot.R" ) );
		Anim_RemoveRootTranslation( asset, &pose );
		animBox_t boxes[ANIM_MAX_BOXES], shifted[ANIM_MAX_BOXES];
		const float origin[3] = { 0, 0, 0 }, moved[3] = { 100, 0, 0 };
		const float axis[3][3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
		const uint32_t count = Anim_HitBoxes( asset, &pose, origin, axis, boxes, ANIM_MAX_BOXES );
		assert( count >= 3 && count == Anim_HitBoxes( asset, &pose, moved, axis, shifted, ANIM_MAX_BOXES ) );
		const int head = Anim_BoneIndex( asset, "head" );
		for ( uint32_t i = 0; i < 3; ++i ) {
			assert( boxes[0].mins[i] <= pose.world[head][i * 4 + 3] && boxes[0].maxs[i] >= pose.world[head][i * 4 + 3] );
			assert( fabsf( shifted[0].mins[i] - boxes[0].mins[i] - moved[i] ) < 0.00001f );
		}
		input( "move", 0 );
		tick( 800, "idle" );
		input( "turn", 1 );
		tick( 820, "turn" );
		input( "turn", 0 );
		tick( 1820, "idle" );
	}
	puts( "PASS: owned rig state machine, gameplay events, sockets and body layers" );
}

int main( int argc, char **argv ) {
	assert( argc == 2 || argc == 3 );
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
	if ( argc == 3 && ( strcmp( argv[2], "rifle" ) == 0 || strcmp( argv[2], "body" ) == 0 ) ) {
		CheckRig( &asset, argv[2] );
		free( bytes );
		return 0;
	}
	float parameters[ANIM_MAX_PARAMETERS];
	Anim_DefaultParameters( &asset, parameters );
	const int active = Anim_ParameterIndex( &asset, "active" );
	assert( active >= 0 && parameters[active] == 0 );
	animState_t state;
	animEvents_t events;
	animPose_t pose;
	animTransform_t motion;
	assert( Anim_RootMotion( &asset, Anim_ClipIndex( &asset, "idle" ), 750, 1250, true, &motion ) );
	assert( fabsf( motion.translate[0] - 0.0625f ) < 0.00001f );
	if ( argc == 3 && strcmp( argv[2], "trees" ) == 0 ) {
		const int upper = Anim_ParameterIndex( &asset, "upper" );
		assert( upper >= 0 );
		Anim_Reset( &asset, 0, &state );
		parameters[active] = 0.5f;
		parameters[upper] = 0.5f;
		assert( Anim_Evaluate( &asset, &state, parameters, 500, &pose ) );
		assert( fabsf( pose.local[0].translate[0] - 0.03125f ) < 0.00001f );
		assert( fabsf( pose.world[1][0] - 0.70710678f ) < 0.0001f );
		assert( fabsf( pose.world[1][8] - 0.70710678f ) < 0.0001f );
		parameters[active] = 0;
		parameters[upper] = 1;
		assert( Anim_Evaluate( &asset, &state, parameters, 500, &pose ) );
		assert( fabsf( pose.local[0].translate[0] - 0.0625f ) < 0.00001f );
		assert( fabsf( pose.world[1][0] - 0.70710678f ) < 0.0001f );
		free( bytes );
		puts( "PASS: data-authored blend tree with a masked additive upper layer" );
		return 0;
	}
	if ( argc == 3 ) {
		const int turn = Anim_ClipIndex( &asset, "turn" );
		assert( turn >= 0 );
		assert( Anim_RootMotion( &asset, turn, 0, 2000, true, &motion ) );
		assert( fabsf( motion.translate[0] - 0.125f ) < 0.00001f );
		assert( fabsf( motion.translate[2] - 0.125f ) < 0.00001f );
		assert( fabsf( fabsf( motion.rotate[1] ) - 1 ) < 0.00001f && fabsf( motion.rotate[3] ) < 0.00001f );
		assert( Anim_RootMotion( &asset, turn, 1000, 2000, true, &motion ) );
		assert( fabsf( motion.translate[0] - 0.125f ) < 0.00001f && fabsf( motion.translate[2] ) < 0.00001f );
		assert( Anim_RootMotion( &asset, turn, 0, 4000, true, &motion ) );
		assert( distance( motion.translate, root ) < 0.00001f && fabsf( fabsf( motion.rotate[3] ) - 1 ) < 0.00001f );
		Anim_Reset( &asset, 0, &state );
		assert( Anim_Tick( &asset, parameters, 0, &state, &events ) && events.count == 1 );
		assert( strcmp( Anim_EventName( &asset, events.items[0].id ), "entry" ) == 0 );
		assert( Anim_Tick( &asset, parameters, 0, &state, &events ) && events.count == 0 );
		const animState_t before = state;
		assert( !Anim_Tick( &asset, parameters, 100000, &state, &events ) );
		assert( events.count == 0 && memcmp( &before, &state, sizeof( state ) ) == 0 );
		Anim_Reset( &asset, UINT32_MAX - 100, &state );
		assert( Anim_Tick( &asset, parameters, UINT32_MAX - 100, &state, &events ) && events.count == 1 );
		assert( Anim_Tick( &asset, parameters, 149, &state, &events ) && events.count == 1 );
		assert( events.items[0].time == 149 && events.items[0].sequence == 2 );
		assert( Anim_Evaluate( &asset, &state, parameters, 149, &pose ) );
		assert( fabsf( pose.local[0].translate[0] - 0.03125f ) < 0.00001f );
		free( bytes );
		puts( "PASS: turning root motion, initial events, transactional overflow and clock wrap" );
		return 0;
	}
	Anim_Reset( &asset, 0, &state );
	assert( Anim_Tick( &asset, parameters, 0, &state, &events ) && events.count == 0 );
	assert( Anim_Tick( &asset, parameters, 250, &state, &events ) && events.count == 1 );
	assert( strcmp( Anim_EventName( &asset, events.items[0].id ), "step" ) == 0 );
	assert( Anim_Evaluate( &asset, &state, parameters, 250, &pose ) );
	assert( pose.jointCount == 2 && fabsf( pose.local[0].translate[0] - 0.03125f ) < 0.00001f );
	parameters[active] = 1;
	assert( Anim_Tick( &asset, parameters, 500, &state, &events ) && events.count == 1 );
	assert( strcmp( Anim_EventName( &asset, events.items[0].id ), "start" ) == 0 );
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
	assert( Anim_Tick( &asset, parameters, 1500, &state, &events ) && events.count == 1 );
	assert( strcmp( Anim_EventName( &asset, events.items[0].id ), "finish" ) == 0 );
	assert( strcmp( Anim_StateName( &asset, state.current ), "idle" ) == 0 );
	assert( Anim_Tick( &asset, parameters, 2750, &state, &events ) && events.count == 2 );
	assert( strcmp( Anim_EventName( &asset, events.items[0].id ), "step" ) == 0 );
	assert( strcmp( Anim_EventName( &asset, events.items[1].id ), "step" ) == 0 );
	assert( Anim_Tick( &asset, parameters, 2750, &state, &events ) && events.count == 0 );
	free( bytes );
	puts( "PASS: masked/additive blending, two-bone/look-at IK, clip sampling, transitions and exactly-once events" );
}

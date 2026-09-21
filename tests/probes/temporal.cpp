#include "../../engine/render/tr_local.h"
#include <assert.h>
#include <limits>

int main() {
	static_assert( std::is_trivially_copyable_v<temporalView_t> );
	static_assert( std::is_trivially_copyable_v<temporalEntity_t> );
	float baseProjection[16]{};
	baseProjection[0] = 1.5f;
	baseProjection[5] = 2;
	baseProjection[8] = .1f;
	baseProjection[9] = -.2f;
	baseProjection[10] = .001f;
	baseProjection[11] = -1;
	baseProjection[14] = 4;
	float sum[2]{};
	for ( uint32_t frame = 0; frame < 8; ++frame ) {
		float projection[16], repeated[16];
		memcpy( projection, baseProjection, sizeof( projection ) );
		memcpy( repeated, baseProjection, sizeof( repeated ) );
		assert( R_TemporalJitter( frame, 640, 480, projection ) );
		assert( R_TemporalJitter( frame + 8, 640, 480, repeated ) );
		assert( !memcmp( projection, repeated, sizeof( projection ) ) );
		for ( int i = 0; i < 16; ++i )
			if ( i != 8 && i != 9 )
				assert( projection[i] == baseProjection[i] );
		for ( int axis = 0; axis < 2; ++axis ) {
			const float pixels = ( projection[8 + axis] - baseProjection[8 + axis] ) * ( axis ? 480 : 640 ) * .5f;
			assert( fabsf( pixels ) < .5f );
			sum[axis] += pixels;
		}
	}
	assert( fabsf( sum[0] ) < .0001f && fabsf( sum[1] ) < .0001f );
	float invalid[16];
	memcpy( invalid, baseProjection, sizeof( invalid ) );
	assert( !R_TemporalJitter( 0, 0, 480, invalid ) );
	assert( !memcmp( invalid, baseProjection, sizeof( invalid ) ) );
	R_TemporalReset();
	temporalView_t view{};
	view.frame = 10;
	view.width = 640;
	view.height = 480;
	view.viewport = { { 0, 0 }, { 640, 480 } };
	view.axis[0][0] = view.axis[1][1] = view.axis[2][2] = 1;
	view.fovX = 90;
	view.fovY = 75;
	for ( int i = 0; i < 4; ++i )
		view.viewProjection[i * 5] = 1;
	trRefEntity_t entity{};
	entity.e.reType = RT_MODEL;
	entity.e.hModel = 1;
	entity.e.axis[0][0] = entity.e.axis[1][1] = entity.e.axis[2][2] = 1;
	entity.e.origin[0] = 8;
	entity.e.oldorigin[0] = -900; // Interpolation input is not last rendered origin.
	entity.e.frame = 7;
	entity.e.oldframe = 6;
	skeletalPose_t pose{};
	pose.jointCount = 1;
	pose.skin[0][0] = pose.skin[0][5] = pose.skin[0][10] = 1;
	pose.skin[0][3] = 12;
	entity.skeletalPose = &pose;
	uint8_t hash[32] = { 1 };
	assert( !R_TemporalBeginView( &view ) );
	assert( !R_TemporalEntity( 1, &entity, hash ) );
	pose.skin[0][3] = 99; // Submission owns a copy, not this caller's storage.
	R_TemporalEndView( true );
	view.frame++;
	view.origin[0] = 4;
	view.viewProjection[12] = -4;
	assert( R_TemporalBeginView( &view ) );
	assert( R_TemporalPreviousView()->origin[0] == 0 );
	assert( R_TemporalPreviousView()->viewProjection[12] == 0 );
	entity.e.origin[0] = 20;
	entity.e.oldorigin[0] = -1000;
	entity.e.frame = 9;
	const temporalEntity_t *previous = R_TemporalEntity( 1, &entity, hash );
	assert( previous && previous->entity.origin[0] == 8 && previous->entity.frame == 7 );
	assert( previous->hasPose && previous->pose.skin[0][3] == 12 );
	assert( !R_TemporalEntity( 1, &entity, hash ) ); // Duplicate identity is not silently replaced.
	assert( !R_TemporalEntity( 0, &entity, hash ) );
	R_TemporalEndView( true );
	view.frame++;
	assert( R_TemporalBeginView( &view ) );
	hash[0] = 2;
	assert( !R_TemporalEntity( 1, &entity, hash ) ); // Hot-reloaded geometry cannot borrow stale history.
	R_TemporalEndView( true );
	view.frame++;
	assert( R_TemporalBeginView( &view ) );
	entity.e.hModel = 2;
	assert( !R_TemporalEntity( 1, &entity, hash ) );
	R_TemporalEndView( true );
	view.frame++;
	assert( R_TemporalBeginView( &view ) );
	entity.e.origin[0] += 1024;
	assert( !R_TemporalEntity( 1, &entity, hash ) ); // Teleported object.
	R_TemporalEndView( true );
	view.frame++;
	assert( R_TemporalBeginView( &view ) );
	R_TemporalEndView( true ); // Entity absent in the immediately preceding view.
	view.frame++;
	assert( R_TemporalBeginView( &view ) );
	assert( !R_TemporalEntity( 1, &entity, hash ) );
	R_TemporalEndView( false ); // Failed/skipped resolve never becomes history.
	view.frame++;
	assert( !R_TemporalBeginView( &view ) && !R_TemporalPreviousView() );
	R_TemporalEndView( true );
	view.frame += 2;
	assert( !R_TemporalBeginView( &view ) );
	R_TemporalEndView( true );
	view.frame++;
	view.width++;
	assert( !R_TemporalBeginView( &view ) );
	R_TemporalEndView( true );
	view.frame++;
	view.viewport.offset.x = 1;
	view.viewport.extent.width = 639;
	assert( !R_TemporalBeginView( &view ) );
	R_TemporalEndView( true );
	view.frame++;
	view.origin[0] += 1024;
	assert( !R_TemporalBeginView( &view ) );
	R_TemporalEndView( true );
	view.frame++;
	view.axis[0][0] = -1;
	view.axis[1][1] = -1;
	assert( !R_TemporalBeginView( &view ) );
	R_TemporalEndView( true );
	view.frame++;
	view.fovX = 50;
	assert( !R_TemporalBeginView( &view ) );
	R_TemporalEndView( true );
	view.frame++;
	assert( R_TemporalBeginView( &view ) );
	entity.skeletalPose = nullptr;
	for ( uint32_t i = 0; i < MAX_TEMPORAL_ENTITIES; ++i )
		assert( !R_TemporalEntity( i + 1, &entity, hash ) );
	assert( !R_TemporalEntity( MAX_TEMPORAL_ENTITIES + 1, &entity, hash ) );
	assert( R_TemporalStats().stored == MAX_TEMPORAL_ENTITIES && R_TemporalStats().overflow == 1 );
	R_TemporalEndView( true );
	view.frame++;
	assert( R_TemporalBeginView( &view ) );
	assert( R_TemporalEntity( MAX_TEMPORAL_ENTITIES, &entity, hash ) );
	entity.e.origin[0] = std::numeric_limits<float>::quiet_NaN();
	assert( !R_TemporalEntity( 1, &entity, hash ) );
	entity.e.origin[0] = 0;
	entity.skeletalPose = &pose;
	pose.jointCount = ANIM_MAX_JOINTS + 1;
	assert( !R_TemporalEntity( 2, &entity, hash ) );
	pose.jointCount = 1;
	pose.skin[0][0] = std::numeric_limits<float>::infinity();
	assert( !R_TemporalEntity( 2, &entity, hash ) );
	R_TemporalEndView( true );
	view.frame++;
	view.viewport.extent.width = 0;
	assert( !R_TemporalBeginView( &view ) && !R_TemporalPreviousView() );
	R_TemporalEndView( true );
	view.viewport.extent.width = 639;
	R_TemporalReset();
	assert( !R_TemporalPreviousView() && R_TemporalStats().stored == 0 );
	view.frame = UINT32_MAX;
	assert( !R_TemporalBeginView( &view ) );
	R_TemporalEndView( true );
	view.frame = 0;
	assert( R_TemporalBeginView( &view ) ); // Unsigned frame wrap remains consecutive.
	R_TemporalEndView( true );
}

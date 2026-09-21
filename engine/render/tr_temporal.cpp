#include "tr_local.h"
#include <cmath>

namespace {
struct temporalFrame_t {
	temporalView_t view;
	temporalEntity_t entities[MAX_TEMPORAL_ENTITIES];
	uint32_t count;
	bool committed;
};
static temporalFrame_t frames[2];
static_assert( sizeof( frames ) < 4 * 1024 * 1024 );
static_assert( std::is_trivially_copyable_v<temporalFrame_t> );
static uint32_t current;
static bool active, history;
static temporalStats_t stats;

static bool Finite( const float *values, uint32_t count ) {
	for ( uint32_t i = 0; i < count; ++i )
		if ( !std::isfinite( values[i] ) )
			return false;
	return true;
}
static bool Near( const vec3_t a, const vec3_t b ) {
	float distance = 0;
	for ( int i = 0; i < 3; ++i ) {
		const float delta = a[i] - b[i];
		distance += delta * delta;
	}
	return distance <= 256 * 256;
}
static bool ValidView( const temporalView_t *view ) {
	if ( !view || !view->width || !view->height || !Finite( view->viewProjection, 16 ) || !Finite( view->origin, 3 ) ||
		 !std::isfinite( view->fovX ) || !std::isfinite( view->fovY ) || view->fovX <= 0 || view->fovX >= 180 || view->fovY <= 0 || view->fovY >= 180 )
		return false;
	if ( view->viewport.offset.x < 0 || view->viewport.offset.y < 0 || !view->viewport.extent.width || !view->viewport.extent.height ||
		 (uint64_t)view->viewport.offset.x + view->viewport.extent.width > view->width ||
		 (uint64_t)view->viewport.offset.y + view->viewport.extent.height > view->height )
		return false;
	for ( int i = 0; i < 3; ++i ) {
		const float *axis = view->axis[i];
		const float length = axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2];
		if ( !Finite( axis, 3 ) || length < .99f || length > 1.01f )
			return false;
	}
	return true;
}
static bool Continuous( const temporalView_t &previous, const temporalView_t &view ) {
	if ( view.frame - previous.frame != 1 || view.width != previous.width || view.height != previous.height ||
		 view.viewport.offset.x != previous.viewport.offset.x || view.viewport.offset.y != previous.viewport.offset.y ||
		 view.viewport.extent.width != previous.viewport.extent.width || view.viewport.extent.height != previous.viewport.extent.height ||
		 !Near( previous.origin, view.origin ) || fabsf( previous.fovX - view.fovX ) > 10 || fabsf( previous.fovY - view.fovY ) > 10 )
		return false;
	for ( int i = 0; i < 3; ++i ) {
		float alignment = 0;
		for ( int j = 0; j < 3; ++j )
			alignment += previous.axis[i][j] * view.axis[i][j];
		if ( alignment < .70710677f ) // A 45-degree view cut must not smear history.
			return false;
	}
	return true;
}
static bool ValidEntity( const trRefEntity_t *entity ) {
	if ( !entity || entity->e.reType != RT_MODEL || !entity->e.hModel || !Finite( entity->e.origin, 3 ) || !std::isfinite( entity->e.backlerp ) )
		return false;
	for ( int i = 0; i < 3; ++i )
		if ( !Finite( entity->e.axis[i], 3 ) )
			return false;
	if ( entity->skeletalPose ) {
		const skeletalPose_t &pose = *entity->skeletalPose;
		if ( !pose.jointCount || pose.jointCount > ANIM_MAX_JOINTS )
			return false;
		for ( uint32_t joint = 0; joint < pose.jointCount; ++joint )
			if ( !Finite( pose.skin[joint], 12 ) )
				return false;
	}
	return true;
}
} // namespace

void R_TemporalReset() {
	memset( frames, 0, sizeof( frames ) );
	current = 0;
	active = history = false;
	stats = {};
}
bool R_TemporalBeginView( const temporalView_t *view ) {
	current ^= 1;
	temporalFrame_t &frame = frames[current];
	frame.count = 0;
	frame.committed = false;
	stats = {};
	active = ValidView( view );
	history = active && frames[current ^ 1].committed && Continuous( frames[current ^ 1].view, *view );
	if ( active )
		frame.view = *view;
	return history;
}
const temporalView_t *R_TemporalPreviousView() {
	return active && history ? &frames[current ^ 1].view : nullptr;
}
const temporalEntity_t *R_TemporalEntity( uint64_t identity, const trRefEntity_t *entity, const uint8_t modelHash[32] ) {
	if ( !active || !identity || !modelHash || !ValidEntity( entity ) ) {
		++stats.rejected;
		return nullptr;
	}
	temporalFrame_t &frame = frames[current];
	// ponytail: linear scans are bounded by 256 entities; hash only if measured costly.
	for ( uint32_t i = 0; i < frame.count; ++i )
		if ( frame.entities[i].identity == identity ) {
			++stats.rejected;
			return nullptr;
		}
	if ( frame.count == MAX_TEMPORAL_ENTITIES ) {
		++stats.overflow;
		return nullptr;
	}
	temporalEntity_t &saved = frame.entities[frame.count++];
	saved.identity = identity;
	saved.entity = entity->e;
	saved.hasPose = entity->skeletalPose != nullptr;
	if ( saved.hasPose )
		saved.pose = *entity->skeletalPose;
	memcpy( saved.modelHash, modelHash, sizeof( saved.modelHash ) );
	stats.stored = frame.count;
	if ( history ) {
		const temporalFrame_t &previous = frames[current ^ 1];
		for ( uint32_t i = 0; i < previous.count; ++i ) {
			const temporalEntity_t &candidate = previous.entities[i];
			if ( candidate.identity == identity && candidate.entity.hModel == saved.entity.hModel &&
				 candidate.entity.renderfx == saved.entity.renderfx && candidate.hasPose == saved.hasPose &&
				 ( !saved.hasPose || candidate.pose.jointCount == saved.pose.jointCount ) &&
				 !memcmp( candidate.modelHash, saved.modelHash, sizeof( saved.modelHash ) ) && Near( candidate.entity.origin, saved.entity.origin ) ) {
				++stats.matched;
				return &candidate;
			}
		}
	}
	return nullptr;
}
void R_TemporalEndView( bool rendered ) {
	frames[current].committed = active && rendered;
	active = history = false;
}
temporalStats_t R_TemporalStats() {
	return stats;
}

#include "physics_public.h"
#include <Jolt/Jolt.h>
#include <Jolt/Core/Memory.h>
#include <joltc.h>
#include <cmath>
#include <cstdlib>
#include <cstring>

static struct {
	unsigned char *memory;
	size_t capacity;
	physStats_t stats;
	void ( *fatal )();
	JPH_PhysicsSystem *system;
	JPH_BodyInterface *bodies;
	JPH_JobSystem *jobs;
	JPH_TempAllocator *temporary;
	JPH_ObjectLayerFilter *queryFilter;
	JPH_BodyFilter *bodyFilter;
	JPH_BodyID queryIgnore;
	JPH_BodyID ids[PHYS_MAX_BODIES];
	bool dynamic[PHYS_MAX_BODIES], active[PHYS_MAX_BODIES];
	uint32_t count;
	JPH_Constraint *joints[PHYS_MAX_BODIES];
	uint32_t jointA[PHYS_MAX_BODIES], jointB[PHYS_MAX_BODIES], jointCount;
	bool started;
} physics;

static void *Aligned( size_t size, size_t alignment ) {
	const uintptr_t address = reinterpret_cast<uintptr_t>( physics.memory ) + physics.stats.used;
	const size_t padding = ( alignment - ( address & ( alignment - 1 ) ) ) & ( alignment - 1 );
	if ( !size )
		size = 1;
	if ( padding > physics.capacity - physics.stats.used || size > physics.capacity - physics.stats.used - padding ) {
		physics.fatal();
		std::abort(); // A returning callback must not let Jolt continue with null.
	}
	void *result = physics.memory + physics.stats.used + padding;
	physics.stats.used += padding + size;
	++physics.stats.allocations;
	++physics.stats.liveBlocks;
	return result;
}
static void *Allocate( size_t size ) {
	return Aligned( size, 16 );
}
static void Free( void *block ) {
	if ( block )
		--physics.stats.liveBlocks;
}
static void *Reallocate( void *block, size_t oldSize, size_t newSize ) {
	if ( !newSize ) {
		Free( block );
		return nullptr;
	}
	void *result = Allocate( newSize );
	if ( block )
		std::memcpy( result, block, oldSize < newSize ? oldSize : newSize );
	Free( block );
	return result;
}
static void One( void *, JPH_JobFunction *job, void *argument ) {
	job( argument );
}
static void Many( void *, JPH_JobFunction *job, void **arguments, uint32_t count ) {
	for ( uint32_t i = 0; i < count; ++i )
		job( arguments[i] );
}
static bool Finite( const float *values, uint32_t count ) {
	for ( uint32_t i = 0; i < count; ++i )
		if ( !std::isfinite( values[i] ) )
			return false;
	return true;
}
static bool Pose( const physTransform_t *pose ) {
	if ( !pose || !Finite( pose->position, 3 ) || !Finite( pose->rotation, 4 ) )
		return false;
	float length = 0;
	for ( float value : pose->rotation )
		length += value * value;
	return std::fabs( length - 1 ) < .001f;
}

static bool JPH_API_CALL QueryLayer( void *, JPH_ObjectLayer layer ) {
	return layer != 2;
}

static bool JPH_API_CALL QueryBody( void *, JPH_BodyID body ) {
	return body != physics.queryIgnore;
}

bool Phys_Init( void *storage, size_t bytes, void ( *fatal )() ) {
	if ( physics.memory || !storage || bytes < 32 * 1024 * 1024 || !fatal )
		return false;
	physics = {};
	physics.memory = static_cast<unsigned char *>( storage );
	physics.capacity = bytes;
	physics.fatal = fatal;
	// ponytail: load-time monotonic arena; reclaim at map teardown. Add reusable
	// blocks only if measured map preparation exceeds the caller's fixed budget.
	JPH::Allocate = Allocate;
	JPH::AlignedAllocate = Aligned;
	JPH::Reallocate = Reallocate;
	JPH::Free = JPH::AlignedFree = Free;
	if ( !JPH_Init() ) {
		physics = {};
		return false;
	}
	const JPH_JobSystemConfig jobs = { nullptr, One, Many, 1, 8 };
	physics.jobs = JPH_JobSystemCallback_Create( &jobs );
	physics.temporary = JPH_TempAllocator_Create( 16 * 1024 * 1024 );
	auto *pairs = JPH_ObjectLayerPairFilterTable_Create( 3 );
	JPH_ObjectLayerPairFilterTable_EnableCollision( pairs, 0, 1 );
	JPH_ObjectLayerPairFilterTable_EnableCollision( pairs, 1, 1 );
	auto *broad = JPH_BroadPhaseLayerInterfaceTable_Create( 3, 2 );
	JPH_BroadPhaseLayerInterfaceTable_MapObjectToBroadPhaseLayer( broad, 0, 0 );
	JPH_BroadPhaseLayerInterfaceTable_MapObjectToBroadPhaseLayer( broad, 1, 1 );
	JPH_BroadPhaseLayerInterfaceTable_MapObjectToBroadPhaseLayer( broad, 2, 1 );
	static const JPH_ObjectLayerFilter_Procs queryProcs = { QueryLayer };
	JPH_ObjectLayerFilter_SetProcs( &queryProcs );
	physics.queryFilter = JPH_ObjectLayerFilter_Create( nullptr );
	static const JPH_BodyFilter_Procs bodyProcs = { QueryBody, nullptr };
	JPH_BodyFilter_SetProcs( &bodyProcs );
	physics.bodyFilter = JPH_BodyFilter_Create( nullptr );
	auto *filter = JPH_ObjectVsBroadPhaseLayerFilterTable_Create( broad, 2, pairs, 3 );
	const JPH_PhysicsSystemSettings settings = { PHYS_MAX_BODIES, 0, 4096, 4096, 0, broad, pairs, filter };
	physics.system = JPH_PhysicsSystem_Create( &settings );
	physics.bodies = JPH_PhysicsSystem_GetBodyInterface( physics.system );
	const JPH_Vec3 gravity = { 0, 0, -9.81f };
	JPH_PhysicsSystem_SetGravity( physics.system, &gravity );
	return true;
}
uint32_t Phys_Prepare( const physBodyDesc_t *desc ) {
	if ( !physics.memory || physics.started || physics.count == PHYS_MAX_BODIES || !desc || !Pose( &desc->transform ) || !std::isfinite( desc->radius ) || desc->radius < 0 )
		return PHYS_INVALID_BODY;
	if ( !desc->radius && ( !Finite( desc->halfExtent, 3 ) || desc->halfExtent[0] <= 0 || desc->halfExtent[1] <= 0 || desc->halfExtent[2] <= 0 ) )
		return PHYS_INVALID_BODY;
	const JPH_Vec3 extent = { desc->halfExtent[0], desc->halfExtent[1], desc->halfExtent[2] };
	auto *shape = desc->radius ? (JPH_Shape *)JPH_SphereShape_Create( desc->radius ) : (JPH_Shape *)JPH_BoxShape_Create( &extent, 0 );
	const auto &pose = desc->transform;
	const JPH_RVec3 position = { pose.position[0], pose.position[1], pose.position[2] };
	const JPH_Quat rotation = { pose.rotation[0], pose.rotation[1], pose.rotation[2], pose.rotation[3] };
	auto *settings = JPH_BodyCreationSettings_Create3( shape, &position, &rotation, desc->dynamic ? JPH_MotionType_Dynamic : JPH_MotionType_Static, desc->dynamic ? 1 : 0 );
	const JPH_BodyID id = JPH_BodyInterface_CreateAndAddBody( physics.bodies, settings, JPH_Activation_DontActivate );
	JPH_BodyCreationSettings_Destroy( settings );
	JPH_Shape_Destroy( shape );
	if ( id == UINT32_MAX )
		return PHYS_INVALID_BODY;
	const uint32_t slot = physics.count++;
	physics.ids[slot] = id;
	physics.dynamic[slot] = desc->dynamic;
	physics.active[slot] = true;
	return slot;
}
bool Phys_PrepareJoint( const physJointDesc_t *desc ) {
	if ( !physics.memory || physics.started || physics.jointCount == PHYS_MAX_BODIES || !desc ||
		 desc->a >= physics.count || desc->b >= physics.count || desc->a == desc->b ||
		 !physics.dynamic[desc->a] || !physics.dynamic[desc->b] ||
		 !Finite( desc->anchorA, 3 ) || !Finite( desc->anchorB, 3 ) ||
		 !std::isfinite( desc->swing ) || desc->swing < 0 || desc->swing > 3.1415926f ||
		 !std::isfinite( desc->twist ) || desc->twist < 0 || desc->twist > 3.1415926f )
		return false;
	const auto *locks = JPH_PhysicsSystem_GetBodyLockInterfaceNoLock( physics.system );
	JPH_BodyLockWrite a{}, b{};
	JPH_BodyLockInterface_LockWrite( locks, physics.ids[desc->a], &a );
	JPH_BodyLockInterface_LockWrite( locks, physics.ids[desc->b], &b );
	JPH_SwingTwistConstraintSettings settings;
	JPH_SwingTwistConstraintSettings_Init( &settings );
	settings.space = JPH_ConstraintSpace_LocalToBodyCOM;
	settings.position1 = { desc->anchorA[0], desc->anchorA[1], desc->anchorA[2] };
	settings.position2 = { desc->anchorB[0], desc->anchorB[1], desc->anchorB[2] };
	settings.twistAxis1 = settings.twistAxis2 = { 0, 0, 1 };
	settings.planeAxis1 = settings.planeAxis2 = { 1, 0, 0 };
	settings.normalHalfConeAngle = settings.planeHalfConeAngle = desc->swing;
	settings.twistMinAngle = -desc->twist;
	settings.twistMaxAngle = desc->twist;
	auto *joint = (JPH_Constraint *)JPH_SwingTwistConstraint_Create( &settings, a.body, b.body );
	JPH_BodyLockInterface_UnlockWrite( locks, &b );
	JPH_BodyLockInterface_UnlockWrite( locks, &a );
	if ( !joint )
		return false;
	const uint32_t index = physics.jointCount++;
	physics.joints[index] = joint;
	physics.jointA[index] = desc->a;
	physics.jointB[index] = desc->b;
	JPH_PhysicsSystem_AddConstraint( physics.system, joint );
	return true;
}
static void EnableJoints( uint32_t slot ) {
	for ( uint32_t i = 0; i < physics.jointCount; ++i )
		if ( physics.jointA[i] == slot || physics.jointB[i] == slot )
			JPH_Constraint_SetEnabled( physics.joints[i], physics.active[physics.jointA[i]] && physics.active[physics.jointB[i]] );
}
bool Phys_Start() {
	if ( !physics.memory || physics.started || !physics.count )
		return false;
	JPH_PhysicsSystem_OptimizeBroadPhase( physics.system );
	physics.started = true;
	for ( uint32_t i = 0; i < physics.count; ++i )
		if ( physics.dynamic[i] )
			Phys_Despawn( i );
	return true;
}
bool Phys_Spawn( uint32_t slot, const physTransform_t *pose, const float velocity[3] ) {
	if ( !physics.started || slot >= physics.count || !physics.dynamic[slot] || !Pose( pose ) || !velocity || !Finite( velocity, 3 ) )
		return false;
	const JPH_RVec3 position = { pose->position[0], pose->position[1], pose->position[2] };
	const JPH_Quat rotation = { pose->rotation[0], pose->rotation[1], pose->rotation[2], pose->rotation[3] };
	JPH_BodyInterface_SetPositionAndRotation( physics.bodies, physics.ids[slot], &position, &rotation, JPH_Activation_DontActivate );
	JPH_Vec3 linear = { velocity[0], velocity[1], velocity[2] }, angular = {};
	JPH_BodyInterface_SetLinearAndAngularVelocity( physics.bodies, physics.ids[slot], &linear, &angular );
	JPH_BodyInterface_SetObjectLayer( physics.bodies, physics.ids[slot], 1 );
	JPH_BodyInterface_ActivateBody( physics.bodies, physics.ids[slot] );
	physics.active[slot] = true;
	EnableJoints( slot );
	return true;
}
bool Phys_Despawn( uint32_t slot ) {
	if ( !physics.started || slot >= physics.count || !physics.dynamic[slot] || !physics.active[slot] )
		return false;
	// Retain prepared broadphase nodes. Layer 2 has no collision pairs and is
	// excluded by queries, so a sleeping unused slot is invisible to the world.
	JPH_BodyInterface_SetObjectLayer( physics.bodies, physics.ids[slot], 2 );
	JPH_BodyInterface_DeactivateBody( physics.bodies, physics.ids[slot] );
	physics.active[slot] = false;
	EnableJoints( slot );
	return true;
}
bool Phys_Step() {
	return physics.started && !JPH_PhysicsSystem_Update2( physics.system, 1.f / 60.f, 1, physics.temporary, physics.jobs );
}
bool Phys_Transform( uint32_t slot, physTransform_t *pose ) {
	if ( !pose || !physics.started || slot >= physics.count || !physics.active[slot] )
		return false;
	JPH_RVec3 position;
	JPH_Quat rotation;
	JPH_BodyInterface_GetPosition( physics.bodies, physics.ids[slot], &position );
	JPH_BodyInterface_GetRotation( physics.bodies, physics.ids[slot], &rotation );
	*pose = { { position.x, position.y, position.z }, { rotation.x, rotation.y, rotation.z, rotation.w } };
	return true;
}
bool Phys_Ray( const float origin[3], const float displacement[3], float *fraction ) {
	if ( !physics.started || !origin || !displacement || !fraction || !Finite( origin, 3 ) || !Finite( displacement, 3 ) )
		return false;
	const JPH_RVec3 start = { origin[0], origin[1], origin[2] };
	const JPH_Vec3 direction = { displacement[0], displacement[1], displacement[2] };
	JPH_RayCastResult result{};
	if ( !JPH_NarrowPhaseQuery_CastRay( JPH_PhysicsSystem_GetNarrowPhaseQuery( physics.system ), &start, &direction, &result, nullptr, physics.queryFilter, nullptr ) )
		return false;
	*fraction = result.fraction;
	return true;
}
static float SweepHit( void *context, const JPH_ShapeCastResult *result ) {
	auto *fraction = static_cast<float *>( context );
	if ( result->fraction < *fraction )
		*fraction = result->fraction;
	return *fraction;
}
bool Phys_Sweep( uint32_t slot, const physTransform_t *pose, const float displacement[3], float *fraction ) {
	if ( !physics.started || slot >= physics.count || !Pose( pose ) || !displacement || !fraction || !Finite( displacement, 3 ) )
		return false;
	const JPH_RVec3 origin = { pose->position[0], pose->position[1], pose->position[2] };
	const JPH_Quat rotation = { pose->rotation[0], pose->rotation[1], pose->rotation[2], pose->rotation[3] };
	const JPH_Vec3 direction = { displacement[0], displacement[1], displacement[2] };
	JPH_RMat4 transform;
	JPH_Mat4_RotationTranslation( &transform, &rotation, &origin );
	JPH_ShapeCastSettings settings;
	JPH_ShapeCastSettings_Init( &settings );
	JPH_RVec3 offset = {};
	physics.queryIgnore = physics.ids[slot];
	*fraction = 1;
	return JPH_NarrowPhaseQuery_CastShape( JPH_PhysicsSystem_GetNarrowPhaseQuery( physics.system ),
		JPH_BodyInterface_GetShape( physics.bodies, physics.ids[slot] ), &transform, &direction,
		&settings, &offset, SweepHit, fraction, nullptr, physics.queryFilter, physics.bodyFilter, nullptr );
}
physStats_t Phys_Stats() {
	return physics.stats;
}
void Phys_Shutdown() {
	if ( !physics.memory )
		return;
	for ( uint32_t i = 0; i < physics.jointCount; ++i ) {
		JPH_PhysicsSystem_RemoveConstraint( physics.system, physics.joints[i] );
		JPH_Constraint_Destroy( physics.joints[i] );
	}
	for ( uint32_t i = 0; i < physics.count; ++i ) {
		JPH_BodyInterface_RemoveBody( physics.bodies, physics.ids[i] );
		JPH_BodyInterface_DestroyBody( physics.bodies, physics.ids[i] );
	}
	JPH_BodyFilter_Destroy( physics.bodyFilter );
	JPH_ObjectLayerFilter_Destroy( physics.queryFilter );
	JPH_PhysicsSystem_Destroy( physics.system );
	JPH_TempAllocator_Destroy( physics.temporary );
	JPH_JobSystem_Destroy( physics.jobs );
	JPH_Shutdown();
	const auto stats = physics.stats;
	physics = {};
	physics.stats = stats;
	JPH::RegisterDefaultAllocator();
}

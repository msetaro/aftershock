#include "physics_public.h"
#include "../animation/animation_public.h"
#include <Jolt/Jolt.h>
#include <Jolt/Core/Memory.h>
#include <joltc.h>
#include <cmath>
#include <cstdlib>
#include <cstring>

struct physicsRagdoll_t {
	JPH_Ragdoll *handle;
	uint32_t first, count;
	bool active;
	animPose_t pose;
};
static constexpr float unitsToMetres = .0254f;
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
	bool dynamic[PHYS_MAX_BODIES], active[PHYS_MAX_BODIES], mesh[PHYS_MAX_BODIES];
	uint32_t count;
	JPH_Constraint *joints[PHYS_MAX_BODIES];
	uint32_t jointA[PHYS_MAX_BODIES], jointB[PHYS_MAX_BODIES], jointCount;
	bool started;
	physicsRagdoll_t ragdolls[4];
	uint32_t ragdollCount;
	bool ragdollBody[PHYS_MAX_BODIES];
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
	if ( !physics.memory || physics.started || physics.count == PHYS_MAX_BODIES || !desc || !Pose( &desc->transform ) || !std::isfinite( desc->radius ) || desc->radius < 0 || !std::isfinite( desc->restitution ) || desc->restitution < 0 || desc->restitution > 1 )
		return PHYS_INVALID_BODY;
	if ( !desc->radius && ( !Finite( desc->halfExtent, 3 ) || desc->halfExtent[0] <= 0 || desc->halfExtent[1] <= 0 || desc->halfExtent[2] <= 0 ) )
		return PHYS_INVALID_BODY;
	const JPH_Vec3 extent = { desc->halfExtent[0], desc->halfExtent[1], desc->halfExtent[2] };
	auto *shape = desc->radius ? (JPH_Shape *)JPH_SphereShape_Create( desc->radius ) : (JPH_Shape *)JPH_BoxShape_Create( &extent, 0 );
	const auto &pose = desc->transform;
	const JPH_RVec3 position = { pose.position[0], pose.position[1], pose.position[2] };
	const JPH_Quat rotation = { pose.rotation[0], pose.rotation[1], pose.rotation[2], pose.rotation[3] };
	auto *settings = JPH_BodyCreationSettings_Create3( shape, &position, &rotation, desc->dynamic ? JPH_MotionType_Dynamic : JPH_MotionType_Static, desc->dynamic ? 1 : 0 );
	JPH_BodyCreationSettings_SetRestitution( settings, desc->restitution );
	if ( desc->dynamic )
		JPH_BodyCreationSettings_SetMotionQuality( settings, JPH_MotionQuality_LinearCast );
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
uint32_t Phys_PrepareMesh( const physTriangle_t *triangles, uint32_t count ) {
	if ( !physics.memory || physics.started || physics.count == PHYS_MAX_BODIES || !triangles || !count || count > PHYS_MAX_TRIANGLES )
		return PHYS_INVALID_BODY;
	for ( uint32_t i = 0; i < count; ++i )
		for ( uint32_t j = 0; j < 3; ++j )
			if ( !Finite( triangles[i].vertex[j], 3 ) )
				return PHYS_INVALID_BODY;
	auto *input = static_cast<JPH_Triangle *>( Allocate( sizeof( JPH_Triangle ) * count ) );
	for ( uint32_t i = 0; i < count; ++i ) {
		const auto &v = triangles[i].vertex;
		input[i] = { { v[0][0], v[0][1], v[0][2] }, { v[1][0], v[1][1], v[1][2] }, { v[2][0], v[2][1], v[2][2] }, 0 };
	}
	auto *meshSettings = JPH_MeshShapeSettings_Create( input, count );
	Free( input );
	auto *shape = (JPH_Shape *)JPH_MeshShapeSettings_CreateShape( meshSettings );
	JPH_ShapeSettings_Destroy( (JPH_ShapeSettings *)meshSettings );
	if ( !shape )
		return PHYS_INVALID_BODY;
	const JPH_RVec3 position = {};
	auto *settings = JPH_BodyCreationSettings_Create3( shape, &position, nullptr, JPH_MotionType_Static, 0 );
	const JPH_BodyID id = JPH_BodyInterface_CreateAndAddBody( physics.bodies, settings, JPH_Activation_DontActivate );
	JPH_BodyCreationSettings_Destroy( settings );
	JPH_Shape_Destroy( shape );
	if ( id == UINT32_MAX )
		return PHYS_INVALID_BODY;
	const uint32_t slot = physics.count++;
	physics.ids[slot] = id;
	physics.mesh[slot] = physics.active[slot] = true;
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
	for ( uint32_t i = 0; i < physics.ragdollCount; ++i )
		for ( int j = 0; j < JPH_Ragdoll_GetConstraintCount( physics.ragdolls[i].handle ); ++j )
			JPH_Constraint_SetEnabled( (JPH_Constraint *)JPH_Ragdoll_GetConstraint( physics.ragdolls[i].handle, j ), false );
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
bool Phys_Velocity( uint32_t slot, float velocity[3] ) {
	if ( !physics.started || slot >= physics.count || !physics.active[slot] || !velocity )
		return false;
	JPH_Vec3 value;
	JPH_BodyInterface_GetLinearVelocity( physics.bodies, physics.ids[slot], &value );
	velocity[0] = value.x;
	velocity[1] = value.y;
	velocity[2] = value.z;
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
	if ( !physics.started || slot >= physics.count || physics.mesh[slot] || !Pose( pose ) || !displacement || !fraction || !Finite( displacement, 3 ) )
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
	for ( uint32_t i = 0; i < physics.ragdollCount; ++i ) {
		JPH_Ragdoll_RemoveFromPhysicsSystem( physics.ragdolls[i].handle, true );
		JPH_Ragdoll_Destroy( physics.ragdolls[i].handle );
	}
	for ( uint32_t i = 0; i < physics.count; ++i ) {
		if ( physics.ragdollBody[i] )
			continue;
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

static bool RigidMatrix( const float *matrix ) {
	if ( !Finite( matrix, 12 ) )
		return false;
	for ( int a = 0; a < 3; ++a )
		for ( int b = a; b < 3; ++b ) {
			float dot = 0;
			for ( int row = 0; row < 3; ++row )
				dot += matrix[row * 4 + a] * matrix[row * 4 + b];
			if ( std::fabs( dot - ( a == b ? 1.f : 0.f ) ) > .001f )
				return false;
		}
	const float determinant = matrix[0] * ( matrix[5] * matrix[10] - matrix[6] * matrix[9] ) - matrix[1] * ( matrix[4] * matrix[10] - matrix[6] * matrix[8] ) + matrix[2] * ( matrix[4] * matrix[9] - matrix[5] * matrix[8] );
	return determinant > 0;
}
static JPH_Mat4 BoneMatrix( const float *matrix ) {
	return { { { matrix[0], matrix[4], matrix[8], 0 }, { matrix[1], matrix[5], matrix[9], 0 },
		{ matrix[2], matrix[6], matrix[10], 0 }, { matrix[3] * unitsToMetres, matrix[7] * unitsToMetres, matrix[11] * unitsToMetres, 1 } } };
}
uint32_t Phys_PrepareRagdoll( const animAsset_t *asset ) {
	if ( !physics.memory || physics.started || physics.ragdollCount == 4 || !asset || !asset->data )
		return PHYS_INVALID_BODY;
	const uint32_t count = asset->header.sections[ANIM_JOINTS].count;
	if ( !count || count > ANIM_MAX_JOINTS || count > PHYS_MAX_BODIES - physics.count )
		return PHYS_INVALID_BODY;
	animFileJoint_t bones[ANIM_MAX_JOINTS];
	animPose_t bind{};
	bind.jointCount = count;
	for ( uint32_t i = 0; i < count; ++i ) {
		std::memcpy( &bones[i], asset->data + asset->header.sections[ANIM_JOINTS].offset + i * sizeof( bones[i] ), sizeof( bones[i] ) );
		bind.local[i] = bones[i].bind;
	}
	if ( !Anim_UpdateWorld( asset, &bind ) )
		return PHYS_INVALID_BODY;
	alignas( 16 ) JPH_Mat4 matrices[ANIM_MAX_JOINTS];
	for ( uint32_t i = 0; i < count; ++i ) {
		if ( !RigidMatrix( bind.world[i] ) )
			return PHYS_INVALID_BODY;
		matrices[i] = BoneMatrix( bind.world[i] );
	}
	auto *skeleton = JPH_Skeleton_Create();
	for ( uint32_t i = 0; i < count; ++i )
		JPH_Skeleton_AddJoint2( skeleton, bones[i].name, bones[i].parent );
	auto *settings = JPH_RagdollSettings_Create();
	JPH_RagdollSettings_SetSkeleton( settings, skeleton );
	JPH_Skeleton_Destroy( skeleton );
	JPH_RagdollSettings_ResizeParts( settings, int( count ) );
	for ( uint32_t i = 0; i < count; ++i ) {
		// Authored bone boxes define collision volume; unboxed connector joints
		// retain a small sphere so the full skeleton remains articulated.
		JPH_Shape *shape = nullptr;
		for ( uint32_t j = 0; j < asset->header.sections[ANIM_BOXES].count; ++j ) {
			animFileBox_t box;
			std::memcpy( &box, asset->data + asset->header.sections[ANIM_BOXES].offset + j * sizeof( box ), sizeof( box ) );
			if ( box.bone != i )
				continue;
			const JPH_Vec3 half = { box.extent[0] * unitsToMetres, box.extent[1] * unitsToMetres, box.extent[2] * unitsToMetres };
			const JPH_Vec3 offset = { box.offset[0] * unitsToMetres, box.offset[1] * unitsToMetres, box.offset[2] * unitsToMetres };
			const JPH_Quat identity = { 0, 0, 0, 1 };
			auto *boxShape = (JPH_Shape *)JPH_BoxShape_Create( &half, 0 );
			shape = (JPH_Shape *)JPH_RotatedTranslatedShape_Create( &offset, &identity, boxShape );
			JPH_Shape_Destroy( boxShape );
			break;
		}
		if ( !shape )
			shape = (JPH_Shape *)JPH_SphereShape_Create( .04f );
		JPH_RagdollSettings_SetPartShape( settings, int( i ), shape );
		JPH_Shape_Destroy( shape );
		JPH_Vec3 position;
		JPH_Quat rotation;
		JPH_Mat4_GetTranslation( &matrices[i], &position );
		JPH_Mat4_GetQuaternion( &matrices[i], &rotation );
		JPH_RagdollSettings_SetPartPosition( settings, int( i ), &position );
		JPH_RagdollSettings_SetPartRotation( settings, int( i ), &rotation );
		JPH_RagdollSettings_SetPartMotionType( settings, int( i ), JPH_MotionType_Dynamic );
		JPH_RagdollSettings_SetPartObjectLayer( settings, int( i ), 1 );
		JPH_RagdollSettings_SetPartMassProperties( settings, int( i ), 1 );
		if ( bones[i].parent >= 0 ) {
			JPH_SwingTwistConstraintSettings joint;
			JPH_SwingTwistConstraintSettings_Init( &joint );
			joint.space = JPH_ConstraintSpace_WorldSpace;
			joint.position1 = joint.position2 = position;
			joint.twistAxis1 = joint.twistAxis2 = { 0, 0, 1 };
			joint.planeAxis1 = joint.planeAxis2 = { 1, 0, 0 };
			joint.normalHalfConeAngle = joint.planeHalfConeAngle = 1.f;
			joint.twistMinAngle = -.5f;
			joint.twistMaxAngle = .5f;
			JPH_RagdollSettings_SetPartToParent( settings, int( i ), &joint );
		}
	}
	JPH_RagdollSettings_DisableParentChildCollisions( settings, matrices, 0 );
	if ( !JPH_RagdollSettings_Stabilize( settings ) ) {
		JPH_RagdollSettings_Destroy( settings );
		return PHYS_INVALID_BODY;
	}
	auto *handle = JPH_RagdollSettings_CreateRagdoll( settings, physics.system, physics.ragdollCount + 1, 0 );
	JPH_RagdollSettings_Destroy( settings );
	if ( !handle )
		return PHYS_INVALID_BODY;
	JPH_Ragdoll_AddToPhysicsSystem( handle, JPH_Activation_DontActivate, true );
	const uint32_t index = physics.ragdollCount++;
	auto &ragdoll = physics.ragdolls[index];
	ragdoll.handle = handle;
	ragdoll.first = physics.count;
	ragdoll.count = count;
	ragdoll.pose = bind;
	for ( uint32_t i = 0; i < count; ++i ) {
		const uint32_t slot = physics.count++;
		physics.ids[slot] = JPH_Ragdoll_GetBodyID( handle, int( i ) );
		physics.dynamic[slot] = physics.active[slot] = physics.ragdollBody[slot] = true;
		JPH_BodyInterface_SetMotionQuality( physics.bodies, physics.ids[slot], JPH_MotionQuality_LinearCast );
	}
	return index;
}
bool Phys_SpawnRagdoll( uint32_t index, const animPose_t *pose, const float origin[3], const float axis[3][3], const float velocity[3] ) {
	if ( !physics.started || index >= physics.ragdollCount || !pose || !origin || !axis || !velocity || !Finite( origin, 3 ) || !Finite( velocity, 3 ) )
		return false;
	auto &ragdoll = physics.ragdolls[index];
	if ( pose->jointCount != ragdoll.count )
		return false;
	alignas( 16 ) JPH_Mat4 matrices[ANIM_MAX_JOINTS];
	for ( uint32_t i = 0; i < ragdoll.count; ++i ) {
		float world[12] = {};
		for ( int row = 0; row < 3; ++row )
			for ( int column = 0; column < 4; ++column ) {
				for ( int k = 0; k < 3; ++k )
					world[row * 4 + column] += axis[k][row] * pose->world[i][k * 4 + column];
				if ( column == 3 )
					world[row * 4 + column] += origin[row];
			}
		if ( !RigidMatrix( world ) )
			return false;
		matrices[i] = BoneMatrix( world );
	}
	const JPH_RVec3 zero = {};
	JPH_Ragdoll_ResetWarmStart( ragdoll.handle );
	JPH_Ragdoll_SetPose2( ragdoll.handle, &zero, matrices, true );
	JPH_Vec3 linear = { velocity[0] * unitsToMetres, velocity[1] * unitsToMetres, velocity[2] * unitsToMetres }, angular = {};
	for ( uint32_t i = 0; i < ragdoll.count; ++i ) {
		const uint32_t slot = ragdoll.first + i;
		JPH_BodyInterface_SetLinearAndAngularVelocity( physics.bodies, physics.ids[slot], &linear, &angular );
		JPH_BodyInterface_SetObjectLayer( physics.bodies, physics.ids[slot], 1 );
		JPH_BodyInterface_ActivateBody( physics.bodies, physics.ids[slot] );
		physics.active[slot] = true;
	}
	for ( int i = 0; i < JPH_Ragdoll_GetConstraintCount( ragdoll.handle ); ++i )
		JPH_Constraint_SetEnabled( (JPH_Constraint *)JPH_Ragdoll_GetConstraint( ragdoll.handle, i ), true );
	ragdoll.pose = *pose;
	ragdoll.active = true;
	return true;
}
bool Phys_DespawnRagdoll( uint32_t index ) {
	if ( !physics.started || index >= physics.ragdollCount || !physics.ragdolls[index].active )
		return false;
	auto &ragdoll = physics.ragdolls[index];
	for ( int i = 0; i < JPH_Ragdoll_GetConstraintCount( ragdoll.handle ); ++i )
		JPH_Constraint_SetEnabled( (JPH_Constraint *)JPH_Ragdoll_GetConstraint( ragdoll.handle, i ), false );
	for ( uint32_t i = 0; i < ragdoll.count; ++i )
		Phys_Despawn( ragdoll.first + i );
	ragdoll.active = false;
	return true;
}
bool Phys_RagdollPose( uint32_t index, animPose_t *pose, float origin[3] ) {
	if ( !physics.started || index >= physics.ragdollCount || !physics.ragdolls[index].active || !pose || !origin )
		return false;
	auto &ragdoll = physics.ragdolls[index];
	alignas( 16 ) JPH_Mat4 matrices[ANIM_MAX_JOINTS];
	JPH_RVec3 root;
	JPH_Ragdoll_GetPose2( ragdoll.handle, &root, matrices, true );
	origin[0] = root.x / unitsToMetres;
	origin[1] = root.y / unitsToMetres;
	origin[2] = root.z / unitsToMetres;
	*pose = ragdoll.pose;
	for ( uint32_t i = 0; i < ragdoll.count; ++i ) {
		const auto &m = matrices[i];
		const float rows[12] = { m.column[0].x, m.column[1].x, m.column[2].x, m.column[3].x / unitsToMetres,
			m.column[0].y, m.column[1].y, m.column[2].y, m.column[3].y / unitsToMetres,
			m.column[0].z, m.column[1].z, m.column[2].z, m.column[3].z / unitsToMetres };
		std::memcpy( pose->world[i], rows, sizeof( rows ) );
	}
	return true;
}

#include <Jolt/Jolt.h>
#include <Jolt/Core/Memory.h>
#include <joltc.h>

#include <cassert>
#include <cfenv>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

#if defined( __SSE__ )
#include <xmmintrin.h>
#endif

static JPH::AllocateFunction allocateOriginal;
static JPH::AlignedAllocateFunction alignedOriginal;
static JPH::ReallocateFunction reallocateOriginal;
static JPH::FreeFunction freeOriginal;
static JPH::AlignedFreeFunction alignedFreeOriginal;
static uint32_t allocations;
static uint32_t liveBlocks;
static uint32_t cppAllocations;
static void *CPPAllocate( size_t size, size_t alignment = 0 ) {
	++cppAllocations;
	if ( !size )
		size = 1;
	if ( alignment && size > SIZE_MAX - ( alignment - 1 ) )
		std::abort();
	void *block = alignment ? std::aligned_alloc( alignment, ( size + alignment - 1 ) & ~( alignment - 1 ) ) : std::malloc( size );
	if ( !block )
		std::abort();
	return block;
}
void *operator new( size_t size ) {
	return CPPAllocate( size );
}
void *operator new[]( size_t size ) {
	return CPPAllocate( size );
}
void *operator new( size_t size, std::align_val_t alignment ) {
	return CPPAllocate( size, size_t( alignment ) );
}
void *operator new[]( size_t size, std::align_val_t alignment ) {
	return CPPAllocate( size, size_t( alignment ) );
}
void operator delete( void *block ) noexcept {
	std::free( block );
}
void operator delete[]( void *block ) noexcept {
	std::free( block );
}
void operator delete( void *block, size_t ) noexcept {
	std::free( block );
}
void operator delete[]( void *block, size_t ) noexcept {
	std::free( block );
}
void operator delete( void *block, std::align_val_t ) noexcept {
	std::free( block );
}
void operator delete[]( void *block, std::align_val_t ) noexcept {
	std::free( block );
}
void operator delete( void *block, size_t, std::align_val_t ) noexcept {
	std::free( block );
}
void operator delete[]( void *block, size_t, std::align_val_t ) noexcept {
	std::free( block );
}
static void *Allocate( size_t size ) {
	++allocations;
	void *block = allocateOriginal( size );
	if ( block )
		++liveBlocks;
	return block;
}
static void *Aligned( size_t size, size_t alignment ) {
	++allocations;
	void *block = alignedOriginal( size, alignment );
	if ( block )
		++liveBlocks;
	return block;
}
static void *Reallocate( void *block, size_t oldSize, size_t newSize ) {
	++allocations;
	void *result = reallocateOriginal( block, oldSize, newSize );
	if ( !block && result )
		++liveBlocks;
	if ( block && !result && !newSize )
		--liveBlocks;
	return result;
}
static void Free( void *block ) {
	if ( block ) {
		assert(liveBlocks);
		--liveBlocks;
	}
	freeOriginal( block );
}
static void AlignedFree( void *block ) {
	if ( block ) {
		assert(liveBlocks);
		--liveBlocks;
	}
	alignedFreeOriginal( block );
}
static void One( void *, JPH_JobFunction *job, void *argument ) {
	job( argument );
}
static void Many( void *, JPH_JobFunction *job, void **arguments, uint32_t count ) {
	for ( uint32_t i = 0; i < count; ++i )
		job( arguments[i] );
}
static uint64_t FPControl() {
#if defined( __SSE__ )
	return _mm_getcsr() & ~uint32_t( 0x3f ); // Arithmetic status flags may change.
#elif defined( __aarch64__ )
	uint64_t value;
	asm volatile( "mrs %0, fpcr" : "=r"( value ) );
	return value;
#else
	return uint64_t( std::fegetround() );
#endif
}

static void Replay( int argc, char **argv ) {
	assert(argc == 3 || argc == 4);
	struct Command {
		uint32_t step, body;
		JPH_Vec3 impulse;
	} commands[64];
	uint32_t commandCount = 0;
	FILE *file = std::fopen( argv[1], "r" );
	assert(file);
	for ( ;; ) {
		Command command{};
		const int fields = std::fscanf( file, "%u %u %f %f %f", &command.step, &command.body,
			&command.impulse.x, &command.impulse.y, &command.impulse.z );
		if ( fields == EOF )
			break;
		assert(fields == 5 && commandCount < 64 && command.step < 600 && command.body < 32);
		assert(!commandCount || command.step >= commands[commandCount - 1].step);
		assert(std::isfinite(command.impulse.x) && std::isfinite(command.impulse.y) && std::isfinite(command.impulse.z));
		commands[commandCount++] = command;
	}
	assert(!std::ferror(file) && commandCount);
	std::fclose( file );
	if ( argc == 4 )
		commands[commandCount - 1].impulse.x += 2;
	JPH::RegisterDefaultAllocator();
	allocateOriginal = JPH::Allocate;
	alignedOriginal = JPH::AlignedAllocate;
	reallocateOriginal = JPH::Reallocate;
	freeOriginal = JPH::Free;
	alignedFreeOriginal = JPH::AlignedFree;
	JPH::Allocate = Allocate;
	JPH::AlignedAllocate = Aligned;
	JPH::Reallocate = Reallocate;
	JPH::Free = Free;
	JPH::AlignedFree = AlignedFree;
	cppAllocations = 0;
	assert(JPH_Init());
	assert(JPH::Allocate == Allocate && JPH::Reallocate == Reallocate && JPH::Free == Free);
	assert(JPH::AlignedAllocate == Aligned && JPH::AlignedFree == AlignedFree);
	const JPH_JobSystemConfig jobsConfig = { nullptr, One, Many, 1, 8 };
	auto *jobs = JPH_JobSystemCallback_Create( &jobsConfig );
	auto *temporary = JPH_TempAllocator_Create( 16 * 1024 * 1024 );
	auto *pairs = JPH_ObjectLayerPairFilterTable_Create( 2 );
	JPH_ObjectLayerPairFilterTable_EnableCollision( pairs, 0, 1 );
	JPH_ObjectLayerPairFilterTable_EnableCollision( pairs, 1, 1 );
	auto *broad = JPH_BroadPhaseLayerInterfaceTable_Create( 2, 2 );
	JPH_BroadPhaseLayerInterfaceTable_MapObjectToBroadPhaseLayer( broad, 0, 0 );
	JPH_BroadPhaseLayerInterfaceTable_MapObjectToBroadPhaseLayer( broad, 1, 1 );
	auto *filter = JPH_ObjectVsBroadPhaseLayerFilterTable_Create( broad, 2, pairs, 2 );
	const JPH_PhysicsSystemSettings config = { 128, 0, 512, 512, 0, broad, pairs, filter };
	auto *system = JPH_PhysicsSystem_Create( &config );
	auto *bodies = JPH_PhysicsSystem_GetBodyInterface( system );
	const JPH_Vec3 half = { 10, 1, 10 };
	auto *floor = JPH_BoxShape_Create( &half, JPH_DEFAULT_CONVEX_RADIUS );
	const JPH_RVec3 floorPosition = { 0, -1, 0 };
	auto *settings = JPH_BodyCreationSettings_Create3( (JPH_Shape *)floor, &floorPosition, nullptr, JPH_MotionType_Static, 0 );
	const auto floorID = JPH_BodyInterface_CreateAndAddBody( bodies, settings, JPH_Activation_DontActivate );
	JPH_BodyCreationSettings_Destroy( settings );
	JPH_Shape_Destroy( (JPH_Shape *)floor );
	auto *sphere = JPH_SphereShape_Create( .5f );
	JPH_BodyID ids[32];
	for ( uint32_t i = 0; i < 32; ++i ) {
		const JPH_RVec3 position = { float( i % 4 ) - 1.5f, 3.f + float( i / 16 ) * 1.1f, float( ( i / 4 ) % 4 ) - 1.5f };
		settings = JPH_BodyCreationSettings_Create3( (JPH_Shape *)sphere, &position, nullptr, JPH_MotionType_Dynamic, 1 );
		ids[i] = JPH_BodyInterface_CreateAndAddBody( bodies, settings, JPH_Activation_Activate );
		assert(ids[i] != UINT32_MAX);
		JPH_BodyCreationSettings_Destroy( settings );
	}
	JPH_Shape_Destroy( (JPH_Shape *)sphere );
	JPH_Constraint *constraints[16];
	const auto *locks = JPH_PhysicsSystem_GetBodyLockInterfaceNoLock( system );
	for ( uint32_t i = 0; i < 16; ++i ) {
		JPH_BodyLockWrite a{}, b{};
		JPH_BodyLockInterface_LockWrite( locks, ids[i * 2], &a );
		JPH_BodyLockInterface_LockWrite( locks, ids[i * 2 + 1], &b );
		JPH_DistanceConstraintSettings joint{};
		JPH_DistanceConstraintSettings_Init( &joint );
		joint.space = JPH_ConstraintSpace_LocalToBodyCOM;
		joint.point1 = joint.point2 = {};
		joint.minDistance = joint.maxDistance = 1.f;
		constraints[i] = (JPH_Constraint *)JPH_DistanceConstraint_Create( &joint, a.body, b.body );
		JPH_BodyLockInterface_UnlockWrite( locks, &b );
		JPH_BodyLockInterface_UnlockWrite( locks, &a );
		JPH_PhysicsSystem_AddConstraint( system, constraints[i] );
	}
	JPH_PhysicsSystem_OptimizeBroadPhase( system );
	assert(cppAllocations == 0); // All dependency-owned storage uses registered callbacks.
	uint32_t nextCommand = 0;
	const uint64_t control = FPControl();
	for ( uint32_t step = 0; step < 600; ++step ) {
		allocations = 0;
		cppAllocations = 0;
		while ( nextCommand < commandCount && commands[nextCommand].step == step ) {
			auto &command = commands[nextCommand++];
			JPH_BodyInterface_AddImpulse( bodies, ids[command.body], &command.impulse );
		}
		assert(!JPH_PhysicsSystem_Update2(system, 1.f / 60.f, 1, temporary, jobs));
		if ( allocations )
			std::fprintf( stderr, "physics step %u: %u allocation calls\n", step, allocations );
		assert(allocations == 0);
		assert(cppAllocations == 0);
		assert(FPControl() == control);
	}
	assert(nextCommand == commandCount);
	file = std::fopen( argv[2], "wb" );
	assert(file);
	for ( auto id : ids ) {
		JPH_RVec3 position;
		JPH_Quat rotation;
		JPH_BodyInterface_GetPosition( bodies, id, &position );
		JPH_BodyInterface_GetRotation( bodies, id, &rotation );
		const float record[] = { position.x, position.y, position.z, rotation.x, rotation.y, rotation.z, rotation.w };
		for ( float value : record )
			assert(std::isfinite(value));
		assert(position.y > 0 && position.y < 10);
		assert(std::fwrite(record, sizeof(record), 1, file) == 1);
	}
	assert(std::fclose(file) == 0);
	for ( auto *constraint : constraints ) {
		JPH_PhysicsSystem_RemoveConstraint( system, constraint );
		JPH_Constraint_Destroy( constraint );
	}
	for ( auto id : ids )
		JPH_BodyInterface_RemoveAndDestroyBody( bodies, id );
	JPH_BodyInterface_RemoveAndDestroyBody( bodies, floorID );
	JPH_PhysicsSystem_Destroy( system );
	JPH_TempAllocator_Destroy( temporary );
	JPH_JobSystem_Destroy( jobs );
	JPH_Shutdown();
	if ( liveBlocks )
		std::fprintf( stderr, "physics shutdown: %u retained allocation blocks\n", liveBlocks );
	assert(liveBlocks == 0);
	std::puts( "PASS: recorded prop commands, constrained contacts, fixed steps and unchanged FP control" );
}

int main( int argc, char **argv ) {
	uint8_t results[2][32 * 7 * sizeof( float )];
	for ( uint32_t attempt = 0; attempt < 2; ++attempt ) {
		Replay( argc, argv );
		FILE *file = std::fopen( argv[2], "rb" );
		assert(file && std::fread(results[attempt], sizeof(results[attempt]), 1, file) == 1);
		assert(std::fclose(file) == 0);
	}
	assert(!std::memcmp(results[0], results[1], sizeof(results[0])));
}

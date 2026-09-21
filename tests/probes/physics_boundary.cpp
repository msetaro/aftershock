#include "engine/physics/physics_public.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <type_traits>
#include "physics_allocations.h"

static_assert( std::is_trivially_destructible_v<physBodyDesc_t> );
static_assert( std::is_trivially_destructible_v<physTransform_t> );
alignas( 64 ) static unsigned char arena[128 * 1024 * 1024];
static void Fatal() {
	std::abort();
}
int main( int argc, char ** ) {
	assert(argc == 1 || argc == 2);
	for ( int restart = 0; restart < 2; ++restart ) {
		assert(!Phys_Init(nullptr, sizeof(arena), Fatal));
		assert(Phys_Init(arena, sizeof(arena), Fatal));
		assert(!Phys_Init(arena, sizeof(arena), Fatal));
		physBodyDesc_t floor{};
		floor.halfExtent[0] = floor.halfExtent[1] = 100;
		floor.halfExtent[2] = 1;
		floor.transform.rotation[3] = 1;
		floor.transform.position[2] = -1;
		assert(Phys_Prepare(&floor) == 0);
		for ( unsigned i = 1; i < PHYS_MAX_BODIES; ++i ) {
			physBodyDesc_t body{};
			body.dynamic = true;
			body.radius = .25f;
			body.transform.rotation[3] = 1;
			body.transform.position[0] = float( i % 16 ) - 8;
			body.transform.position[1] = float( i / 16 ) - 4;
			body.transform.position[2] = 3;
			assert(Phys_Prepare(&body) == i);
		}
		assert(Phys_Prepare(&floor) == PHYS_INVALID_BODY);
		physJointDesc_t joint{};
		joint.a = 1;
		joint.b = 2;
		joint.anchorA[0] = .5f;
		joint.anchorB[0] = -.5f;
		joint.swing = .5f;
		joint.twist = .25f;
		if ( argc == 1 )
			assert(Phys_PrepareJoint(&joint));
		joint.b = PHYS_INVALID_BODY;
		assert(!Phys_PrepareJoint(&joint));
		assert(Phys_Start());
		assert(cppAllocations == 0);
		const float unusedOrigin[3] = { -20, 0, 3 }, across[3] = { 40, 0, 0 };
		float unusedFraction = 1;
		assert(!Phys_Ray(unusedOrigin, across, &unusedFraction));
		const auto baseline = Phys_Stats();
		for ( unsigned cycle = 0; cycle < 4; ++cycle ) {
			for ( unsigned i = 1; i < PHYS_MAX_BODIES; ++i ) {
				physTransform_t pose{};
				pose.rotation[3] = 1;
				pose.position[0] = float( i % 16 ) - 8;
				pose.position[1] = float( i / 16 ) - 4;
				pose.position[2] = 3 + float( cycle );
				const float velocity[3] = { i == 1 ? -10.f : i == 2 ? 10.f
																	: 0.f,
					0, 1 };
				assert(Phys_Spawn(i, &pose, velocity));
			}
			for ( unsigned step = 0; step < 180; ++step )
				assert(Phys_Step());
			physTransform_t a, b;
			assert(Phys_Transform(1, &a) && Phys_Transform(2, &b));
			float separation = 0;
			for ( unsigned axis = 0; axis < 3; ++axis ) {
				float d = a.position[axis] - b.position[axis];
				separation += d * d;
			}
			assert(separation < 1.05f);
			const float origin[3] = { 30, 30, 5 }, direction[3] = { 0, 0, -10 };
			float fraction = 1;
			assert(Phys_Ray(origin, direction, &fraction));
			assert(std::fabs(fraction - .5f) < .001f);
			physTransform_t sweep = { { 30, 30, 5 }, { 0, 0, 0, 1 } };
			fraction = 1;
			assert(Phys_Sweep(1, &sweep, direction, &fraction));
			assert(std::fabs(fraction - .475f) < .001f);
			assert(!Phys_Sweep(PHYS_INVALID_BODY, &sweep, direction, &fraction));
			for ( unsigned i = 1; i < PHYS_MAX_BODIES; ++i ) {
				physTransform_t pose;
				assert(Phys_Transform(i, &pose));
				assert(pose.position[2] > 0 && pose.position[2] < 1);
				assert(Phys_Despawn(i));
				assert(!Phys_Transform(i, &pose));
			}
			const float sleepingOrigin[3] = { -20, 0, .245f };
			assert(!Phys_Ray(sleepingOrigin, across, &unusedFraction));
			const auto current = Phys_Stats();
			assert(current.allocations == baseline.allocations);
			assert(current.used == baseline.used);
			assert(cppAllocations == 0);
		}
		assert(!Phys_Spawn(PHYS_INVALID_BODY, nullptr, nullptr));
		Phys_Shutdown();
		assert(Phys_Stats().liveBlocks == 0);
	}
	std::puts( "PASS: owned arena, full body capacity, allocation-free recycle/query and restart" );
}

#include "engine/physics/physics_public.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <type_traits>

static_assert( std::is_trivially_destructible_v<physBodyDesc_t> );
static_assert( std::is_trivially_destructible_v<physTransform_t> );
alignas( 64 ) static unsigned char arena[128 * 1024 * 1024];
static void Fatal() {
	std::abort();
}
int main() {
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
		assert(Phys_Start());
		const auto baseline = Phys_Stats();
		for ( unsigned cycle = 0; cycle < 4; ++cycle ) {
			for ( unsigned i = 1; i < PHYS_MAX_BODIES; ++i ) {
				physTransform_t pose{};
				pose.rotation[3] = 1;
				pose.position[0] = float( i % 16 ) - 8;
				pose.position[1] = float( i / 16 ) - 4;
				pose.position[2] = 3 + float( cycle );
				const float velocity[3] = { 0, 0, 1 };
				assert(Phys_Spawn(i, &pose, velocity));
			}
			for ( unsigned step = 0; step < 180; ++step )
				assert(Phys_Step());
			const float origin[3] = { 30, 30, 5 }, direction[3] = { 0, 0, -10 };
			float fraction = 1;
			assert(Phys_Ray(origin, direction, &fraction));
			assert(std::fabs(fraction - .5f) < .001f);
			for ( unsigned i = 1; i < PHYS_MAX_BODIES; ++i ) {
				physTransform_t pose;
				assert(Phys_Transform(i, &pose));
				assert(pose.position[2] > 0 && pose.position[2] < 1);
				assert(Phys_Despawn(i));
				assert(!Phys_Transform(i, &pose));
			}
			const auto current = Phys_Stats();
			assert(current.allocations == baseline.allocations);
			assert(current.used == baseline.used);
		}
		assert(!Phys_Spawn(PHYS_INVALID_BODY, nullptr, nullptr));
		Phys_Shutdown();
		assert(Phys_Stats().liveBlocks == 0);
	}
	std::puts( "PASS: owned arena, full body capacity, allocation-free recycle/query and restart" );
}

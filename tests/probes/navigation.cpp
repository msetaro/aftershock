#include "../../engine/navigation/navigation_public.h"
#include "../../engine/qcommon/q_shared.h"
#include "../../engine/qcommon/qcommon_public.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>
#include <initializer_list>

static uint32_t allocations, liveBlocks;
void *Z_Malloc( size_t size ) {
	++allocations;
	++liveBlocks;
	void *data = std::calloc( 1, size );
	assert(data);
	return data;
}
void Z_Free( void *data ) {
	if ( data ) {
		assert(liveBlocks);
		--liveBlocks;
		std::free( data );
	}
}
int main( int argc, char **argv ) {
	assert(argc == 2 || (argc == 3 && !std::strcmp(argv[2],"launch")));
	FILE *file = std::fopen( argv[1], "rb" );
	assert(file);
	assert(std::fseek(file, 0, SEEK_END) == 0);
	const size_t size = size_t( std::ftell( file ) );
	assert(size > 112 && size < 16 * 1024 * 1024);
	std::rewind( file );
	void *data = std::malloc( size );
	assert(data && std::fread(data, 1, size, file) == size);
	std::fclose( file );
	uint32_t checksum;
	std::memcpy( &checksum, static_cast<const uint8_t *>( data ) + 108, sizeof( checksum ) );
	navWorld_t *world = Nav_Open( data, size, checksum, 4 );
	navWorld_t *fresh = Nav_Open( data, size, checksum, 4 );
	assert(world && fresh);
	std::free( data ); // The owner must retain its own mutable Detour storage.
	const uint32_t startupAllocations = allocations;
	static_assert( std::is_trivially_copyable_v<navPath_t> );
	static_assert( std::is_trivially_copyable_v<navAgent_t> );
	const float start[3] = { -160, -160, 0 }, end[3] = { 1696, 160, 32 };
	navPath_t path{};
	assert(Nav_Path(world, start, end, false, &path));
	assert(path.complete && path.count >= 3);
	assert(Distance(path.points[0].position, start) < 8);
	assert(Distance(path.points[path.count-1].position, end) < 8);
	for ( uint32_t i = 0; i < path.count; ++i )
		assert(path.points[i].link == 0);
	// The authored diagonal link crosses the first room's cover. Filtering links
	// off must retain a ground route; enabling them must expose the authored ID.
	const float linkEnd[3] = { 160, 160, 0 };
	assert(Nav_Path(world, start, linkEnd, true, &path));
	bool sawLink = false;
	for ( uint32_t i = 0; i < path.count; ++i )
		if ( path.points[i].link == 1 ) {
			sawLink = true;
			assert(path.points[i].kind == (argc == 3 ? NAV_LINK_LAUNCH : NAV_LINK_JUMP));
		}
	if ( !sawLink ) {
		for ( uint32_t i = 0; i < path.count; ++i )
			std::fprintf( stderr, "corner %u: %g %g %g link %u\n", i, path.points[i].position[0], path.points[i].position[1], path.points[i].position[2], path.points[i].link );
	}
	assert(path.complete && sawLink);
	// Follow the fixed route from authoritative feet positions. An existing pad
	// must launch naturally; reaching its source is not permission to skip it.
	navPath_t route{};
	route.count = 3;
	route.complete = true;
	route.points[0] = { { 0, 0, 0 }, 0, NAV_LINK_NONE };
	route.points[1] = { { 100, 0, 0 }, 5, NAV_LINK_LAUNCH };
	route.points[2] = { { 200, 0, 100 }, 0, NAV_LINK_NONE };
	navFollowState_t cursor{};
	navFollowOutput_t follow{};
	const float origin[3] = { 0, 0, 0 }, pad[3] = { 100, 0, 0 },
				flight[3] = { 150, 0, 150 }, landed[3] = { 200, 0, 100 };
	assert(Nav_Follow(route,origin,true,12,&cursor,&follow));
	assert(cursor.point == 1 && follow.position[0] == 100 && !follow.arrived);
	assert(Nav_Follow(route,pad,true,12,&cursor,&follow));
	assert(follow.kind == NAV_LINK_LAUNCH && follow.link == 5 && follow.position[0] == 100);
	assert(Nav_Follow(route,pad,true,12,&cursor,&follow) && cursor.point == 1);
	assert(Nav_Follow(route,flight,false,12,&cursor,&follow) && follow.position[0] == 200);
	auto restored = cursor;
	assert(Nav_Follow(route,landed,false,12,&cursor,&follow) && !follow.arrived);
	assert(Nav_Follow(route,landed,true,12,&cursor,&follow) && follow.arrived);
	navFollowOutput_t continued{};
	assert(Nav_Follow(route,landed,true,12,&restored,&continued));
	assert(continued.arrived && restored.point == cursor.point && restored.phase == cursor.phase);
	// Jump requests stop once airborne. Doors and drops steer through their
	// endpoint without waiting for a launch impulse. Partial routes never arrive.
	for ( auto kind : { NAV_LINK_JUMP, NAV_LINK_DROP, NAV_LINK_DOOR } ) {
		route.points[1].kind = kind;
		cursor = {};
		cursor.point = 1;
		assert(Nav_Follow(route,pad,true,12,&cursor,&follow));
		assert(follow.kind == kind && follow.position[0] == 200);
		if ( kind != NAV_LINK_DOOR )
			assert(Nav_Follow(route,flight,false,12,&cursor,&follow) && follow.kind == NAV_LINK_NONE);
		assert(Nav_Follow(route,landed,true,12,&cursor,&follow) && follow.arrived);
	}
	route.complete = false;
	cursor = {};
	cursor.point = 2;
	assert(Nav_Follow(route,landed,true,12,&cursor,&follow) && !follow.arrived);
	assert(!Nav_Follow(route,landed,true,0,&cursor,&follow));
	static_assert( std::is_trivially_copyable_v<navFollowState_t> );
	// Boundary candidates come from the navmesh's collision-derived polygons.
	// The owned low cover spans x [-32,32], y [-64,-32], z [0,48].
	navCoverQuery_t covers{};
	assert(Nav_CoverPoints(world,start,400,&covers));
	assert(covers.complete && covers.count > 0);
	bool behindCover = false;
	for ( uint32_t i = 0; i < covers.count; ++i ) {
		const auto &point = covers.points[i];
		assert(Distance(point.position,start) <= 400);
		assert(Nav_Path(world,start,point.position,false,&path));
		if ( path.complete && std::fabs( point.position[0] ) < 32 && point.position[1] > -32 && point.position[1] < 0 && point.position[2] < 4 )
			behindCover = true;
	}
	assert(behindCover);
	navCoverQuery_t repeated{};
	assert(Nav_CoverPoints(world,start,400,&repeated) && repeated.count == covers.count);
	for ( uint32_t i = 0; i < covers.count; ++i ) {
		assert(covers.points[i].identity == repeated.points[i].identity);
		assert(Distance(covers.points[i].position,repeated.points[i].position) == 0);
	}
	// Gameplay steering must depend only on authoritative positions/velocities,
	// so checkpoint reload need not serialize a third-party asynchronous path queue.
	const float position[3] = { -80, -180, 2 }, velocity[3] = { 160, 0, 0 };
	const navObstacle_t obstacle = { { 0, -180, 2 }, { -160, 0, 0 }, 15 };
	float avoided[3], again[3];
	assert(Nav_Avoid(world,position,velocity,velocity,160,&obstacle,1,avoided));
	assert(std::fabs(avoided[1]) > 1 || avoided[0] < 159);
	assert(Nav_Avoid(fresh,position,velocity,velocity,160,&obstacle,1,again));
	assert(!std::memcmp(avoided,again,sizeof(again)));
	assert(Nav_Avoid(world,position,velocity,velocity,160,nullptr,0,again));
	assert(Nav_Avoid(world,position,velocity,velocity,160,&obstacle,1,again));
	assert(!std::memcmp(avoided,again,sizeof(again)));
	// Two opposing agents in open floor must reach their goals without passing
	// through each other. Positions are steering results, not Pmove authority.
	const float left[3] = { -180, -180, 0 }, right[3] = { 180, -180, 0 };
	const int a = Nav_AddAgent( world, left, 160 ), b = Nav_AddAgent( world, right, 160 );
	assert(a >= 0 && b >= 0 && a != b);
	assert(Nav_AgentTarget(world, a, right) && Nav_AgentTarget(world, b, left));
	navAgent_t one{}, two{};
	float minimumSeparation = 1000;
	for ( int step = 0; step < 600; ++step ) {
		Nav_Update( world, 0.02f );
		assert(Nav_Agent(world, a, &one) && Nav_Agent(world, b, &two));
		minimumSeparation = std::fmin( minimumSeparation, Distance( one.position, two.position ) );
	}
	assert(minimumSeparation >= 24);
	assert(Distance(one.position, right) < 20 && Distance(two.position, left) < 20);
	Nav_RemoveAgent( world, a );
	assert(!Nav_Agent(world, a, &one));
	assert(Nav_AddAgent(world, left, 160) == a);
	assert(allocations == startupAllocations);
	Nav_Close( fresh );
	Nav_Close( world );
	assert(liveBlocks == 0);
	std::puts( "PASS: collision navmesh routes, authored off-mesh links, crowd avoidance and allocation-free ticks" );
}

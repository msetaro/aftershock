#include "../../engine/navigation/navigation_public.h"
#include "../../engine/qcommon/q_shared.h"
#include "../../engine/qcommon/qcommon_public.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>

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
static float Distance( const float *a, const float *b ) {
	float sum = 0;
	for ( int i = 0; i < 3; ++i )
		sum += ( a[i] - b[i] ) * ( a[i] - b[i] );
	return std::sqrt( sum );
}
int main( int argc, char **argv ) {
	assert(argc == 2);
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
	assert(world);
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
		sawLink |= path.points[i].link == 1;
	assert(path.complete && sawLink);
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
	Nav_Close( world );
	assert(liveBlocks == 0);
	std::puts( "PASS: collision navmesh routes, authored off-mesh links, crowd avoidance and allocation-free ticks" );
}

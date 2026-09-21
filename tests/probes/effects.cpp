#include "../../engine/effects/effects_public.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <type_traits>

static fxSystem_t state, other;
static uint8_t bytes[48 + sizeof( fxFileHeader_t ) + FX_MAX_EMITTERS * sizeof( fxFileEmitter_t )];

static bool Wall( const float start[3], const float end[3], fxTrace_t *trace, void * ) {
	if ( start[0] <= 5 && end[0] > 5 ) {
		trace->fraction = ( 5 - start[0] ) / ( end[0] - start[0] );
		trace->normal[0] = -1;
		trace->normal[1] = trace->normal[2] = 0;
		return true;
	}
	return false;
}

int main( int argc, char **argv ) {
	static_assert( std::is_trivially_destructible_v<fxSystem_t> );
	static_assert( std::is_trivially_copyable_v<fxAsset_t> );
	static_assert( sizeof( fxFileHeader_t ) == 36 );
	static_assert( sizeof( fxFileEmitter_t ) == 244 );
	assert(argc == 2);
	FILE *file = fopen( argv[1], "rb" );
	assert(file);
	const size_t size = fread( bytes, 1, sizeof( bytes ), file );
	assert(!ferror(file));
	fclose( file );
	fxAsset_t asset{};
	assert(FX_Open(bytes, size, &asset));
	assert(asset.header.emitterCount == 1);
	assert(asset.emitters[0].burst == 8 && asset.emitters[0].capacity == 16);
	const float origin[3] = { 0, 0, 0 };
	const float axis[3][3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
	auto &emitter = asset.emitters[0];
	emitter.flags = 0;
	emitter.drag = 0;
	emitter.gravity[0] = emitter.gravity[1] = emitter.gravity[2] = 0;
	emitter.velocity[0] = 100;
	emitter.velocity[1] = emitter.velocity[2] = 0;
	FX_Reset( &state );
	const uint32_t handle = FX_Start( &state, &asset, origin, axis, 164 );
	assert(handle && state.stats.particles == 8);
	FX_Update( &state, 100, nullptr, nullptr );
	uint32_t count = 0;
	for ( const auto &particle : state.particles ) {
		if ( !particle.active )
			continue;
		++count;
		assert(std::fabs(particle.origin[0] - 10) < .001f);
		assert(particle.ageMs == 100 && particle.frame == 1);
	}
	assert(count == 8);
	FX_Update( &state, 900, nullptr, nullptr );
	assert(state.stats.particles == 0 && state.stats.instances == 0);

	// Reusing an expired instance slot must not revive its old handle.
	const uint32_t replacement = FX_Start( &state, &asset, origin, axis, 165 );
	assert(replacement && replacement != handle);
	assert(!FX_Stop(&state, handle));
	assert(FX_Stop(&state, replacement));
	FX_Update( &state, 1000, nullptr, nullptr );
	assert(state.stats.particles == 0 && state.stats.instances == 0);

	// Emitters use local axes; gravity and drag affect presentation particles.
	const float turned[3][3] = { { 0, 1, 0 }, { -1, 0, 0 }, { 0, 0, 1 } };
	emitter.gravity[2] = -80;
	emitter.drag = .5f;
	FX_Reset( &state );
	assert(FX_Start(&state, &asset, origin, turned, 164));
	FX_Update( &state, 100, nullptr, nullptr );
	for ( const auto &particle : state.particles ) {
		if ( !particle.active )
			continue;
		assert(std::fabs(particle.origin[0]) < .001f);
		assert(particle.origin[1] > 9 && particle.origin[1] < 10);
		assert(particle.origin[2] < 0 && particle.velocity[2] < 0);
		assert(particle.velocity[1] > 90 && particle.velocity[1] < 100);
	}
	emitter.gravity[2] = emitter.drag = 0;
	emitter.capacity = 3;
	FX_Reset( &state );
	assert(FX_Start(&state, &asset, origin, axis, 164));
	assert(state.stats.particles == 3 && state.stats.dropped == 5);
	emitter.capacity = 16;

	// A continuous emitter carries fractional particles without heap allocation.
	emitter.burst = 0;
	emitter.rate = 5;
	emitter.lifetimeMs = 10000;
	FX_Reset( &state );
	const uint32_t continuous = FX_Start( &state, &asset, origin, axis, 164 );
	FX_Update( &state, 100, nullptr, nullptr );
	assert(state.stats.particles == 0);
	FX_Update( &state, 100, nullptr, nullptr );
	assert(state.stats.particles == 1);
	assert(FX_Stop(&state, continuous));
	FX_Update( &state, 1000, nullptr, nullptr );
	assert(state.stats.particles == 1);
	FX_Update( &state, 10000, nullptr, nullptr );
	assert(state.stats.particles == 0 && state.stats.instances == 0);
	assert(!FX_Stop(&state, continuous));

	// Independent worlds using the same seed have identical presentation state.
	emitter.rate = 0;
	emitter.burst = 8;
	emitter.flags = FX_COLLISION;
	emitter.lifetimeMs = 1000;
	FX_Reset( &state );
	FX_Reset( &other );
	assert(FX_Start(&state, &asset, origin, axis, 164));
	assert(FX_Start(&other, &asset, origin, axis, 164));
	FX_Update( &state, 100, Wall, nullptr );
	FX_Update( &other, 100, Wall, nullptr );
	assert(state.stats.collisions == 8 && other.stats.collisions == 8);
	for ( uint32_t i = 0; i < FX_MAX_PARTICLES; ++i ) {
		const auto &a = state.particles[i], &b = other.particles[i];
		assert(a.active == b.active);
		if ( a.active ) {
			assert(a.origin[0] <= 5 && a.velocity[0] < 0);
			for ( uint32_t k = 0; k < 3; ++k )
				assert(a.origin[k] == b.origin[k] && a.velocity[k] == b.velocity[k]);
		}
	}
	emitter.flags = 0;
	emitter.burst = emitter.capacity = FX_MAX_PARTICLES;
	FX_Reset( &state );
	assert(FX_Start(&state, &asset, origin, axis, 1));
	assert(FX_Start(&state, &asset, origin, axis, 2));
	assert(state.stats.particles == FX_MAX_PARTICLES && state.stats.dropped == FX_MAX_PARTICLES);
	FX_Update( &state, 1000, nullptr, nullptr );
	assert(state.stats.particles == 0);
	puts( "PASS: fixed effect pools, rate carry, expiry, stale handles, native motion, collision and overflow counters" );
}

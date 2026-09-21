#include "effects_public.h"
#include "../../third_party/sha256/sha-256.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

static bool Text( const char *text, size_t size ) {
	if ( !text[0] || text[0] == '/' || !std::memchr( text, 0, size ) || std::strstr( text, ".." ) )
		return false;
	for ( const char *p = text; *p; ++p )
		if ( !( ( *p >= 'a' && *p <= 'z' ) || ( *p >= '0' && *p <= '9' ) || *p == '_' || *p == '/' || *p == '.' || *p == '-' ) )
			return false;
	return true;
}
static bool Between( float value, float low, float high ) {
	return std::isfinite( value ) && value >= low && value <= high;
}
bool FX_Open( const void *data, size_t size, fxAsset_t *asset ) {
	static_assert( std::endian::native == std::endian::little );
	if ( !data || !asset || size < 48 + sizeof( fxFileHeader_t ) || size > 48 + sizeof( fxAsset_t ) )
		return false;
	const auto *bytes = (const uint8_t *)data;
	uint32_t envelope[2];
	std::memcpy( envelope, bytes + 8, sizeof( envelope ) );
	if ( std::memcmp( bytes, "ASEFFECT", 8 ) || envelope[0] != 2 || envelope[1] != size - 48 )
		return false;
	uint8_t hash[32];
	calc_sha_256( hash, bytes + 48, size - 48 );
	if ( std::memcmp( hash, bytes + 16, sizeof( hash ) ) )
		return false;
	fxAsset_t result{};
	std::memcpy( &result, bytes + 48, size - 48 );
	const auto &header = result.header;
	if ( !Text( header.name, sizeof( header.name ) ) || !header.emitterCount || header.emitterCount > FX_MAX_EMITTERS ||
		 size != 48 + sizeof( header ) + header.emitterCount * sizeof( fxFileEmitter_t ) )
		return false;
	for ( uint32_t i = 0; i < header.emitterCount; ++i ) {
		const auto &e = result.emitters[i];
		if ( !Text( e.name, sizeof( e.name ) ) || !Text( e.material, sizeof( e.material ) ) ||
			 ( ( e.kind == FX_MESH || e.model[0] ) && !Text( e.model, sizeof( e.model ) ) ) ||
			 e.kind > FX_TRAIL || !e.capacity || e.capacity > FX_MAX_PARTICLES || e.burst > FX_MAX_PARTICLES ||
			 !e.lifetimeMs || e.lifetimeMs > 60000 || ( e.flags & ~7u ) || !e.columns || e.columns > 64 || !e.rows || e.rows > 64 ||
			 !Between( e.rate, 0, 10000 ) || !Between( e.size, .001f, 4096 ) || !Between( e.drag, 0, 100 ) || !Between( e.fps, .01f, 1000 ) || !Between( e.endSize, .001f, 4096 ) || !Between( e.rotation, -360, 360 ) || !Between( e.rotationSpread, 0, 360 ) || !Between( e.angularVelocity, -3600, 3600 ) || !Between( e.lightRadius, 0, 4096 ) || !Between( e.lightIntensity, 0, 128 ) )
			return false;
		for ( uint32_t k = 0; k < 3; ++k )
			if ( !Between( e.velocity[k], -65536, 65536 ) || !Between( e.gravity[k], -65536, 65536 ) || !Between( e.velocitySpread[k], 0, 65536 ) || !Between( e.originSpread[k], 0, 4096 ) || !Between( e.lightColor[k], 0, 1 ) )
				return false;
		for ( float color : e.color )
			if ( !Between( color, 0, 1 ) )
				return false;
		for ( uint32_t j = 0; j < i; ++j )
			if ( !std::strcmp( e.name, result.emitters[j].name ) )
				return false;
	}
	*asset = result;
	return true;
}
void FX_Reset( fxSystem_t *system ) {
	*system = {};
	for ( uint32_t i = 0; i < FX_MAX_PARTICLES; ++i )
		system->particles[i].nextFree = i + 1;
}
static float Spread( uint32_t &seed, float extent ) {
	if ( !extent )
		return 0;
	// Presentation-only uniform samples; unsigned overflow is defined.
	seed = seed * 1664525u + 1013904223u;
	return ( float( seed >> 8 ) * ( 1.f / 8388608.f ) - 1 ) * extent;
}
static void Emit( fxSystem_t *system, uint32_t instanceIndex, uint32_t emitterIndex, uint32_t count ) {
	auto &instance = system->instances[instanceIndex];
	const auto &e = instance.asset.emitters[emitterIndex];
	const uint32_t available = std::min( e.capacity - instance.living[emitterIndex], FX_MAX_PARTICLES - system->stats.particles );
	const uint32_t emitted = std::min( count, available );
	system->stats.dropped += count - emitted;
	for ( uint32_t i = 0; i < emitted; ++i ) {
		auto &particle = system->particles[system->freeParticle];
		system->freeParticle = particle.nextFree;
		particle = {};
		particle.active = true;
		particle.instance = instanceIndex;
		particle.emitter = emitterIndex;
		float offset[3], velocity[3];
		for ( uint32_t k = 0; k < 3; ++k ) {
			offset[k] = Spread( instance.seed, e.originSpread[k] );
			velocity[k] = e.velocity[k] + Spread( instance.seed, e.velocitySpread[k] );
		}
		particle.rotation = e.rotation + Spread( instance.seed, e.rotationSpread );
		for ( uint32_t k = 0; k < 3; ++k ) {
			particle.origin[k] = instance.origin[k];
			for ( uint32_t j = 0; j < 3; ++j ) {
				particle.origin[k] += offset[j] * instance.axis[j][k];
				particle.velocity[k] += velocity[j] * instance.axis[j][k];
			}
			particle.previous[k] = particle.origin[k];
		}
	}
	instance.living[emitterIndex] += emitted;
	system->stats.particles += emitted;
}
uint32_t FX_Start( fxSystem_t *system, const fxAsset_t *asset, const float origin[3], const float axis[3][3], uint32_t seed ) {
	if ( !system || !asset || !origin || !axis || !asset->header.emitterCount || asset->header.emitterCount > FX_MAX_EMITTERS )
		return 0;
	for ( uint32_t index = 0; index < FX_MAX_INSTANCES; ++index ) {
		auto &instance = system->instances[index];
		if ( instance.handle )
			continue;
		instance = {};
		instance.asset = *asset;
		instance.seed = seed;
		std::memcpy( instance.origin, origin, sizeof( instance.origin ) );
		std::memcpy( instance.axis, axis, sizeof( instance.axis ) );
		if ( !++system->nextHandle )
			++system->nextHandle;
		instance.handle = system->nextHandle;
		++system->stats.instances;
		for ( uint32_t e = 0; e < asset->header.emitterCount; ++e )
			Emit( system, index, e, asset->emitters[e].burst );
		return instance.handle;
	}
	for ( uint32_t i = 0; i < asset->header.emitterCount; ++i )
		system->stats.dropped += asset->emitters[i].burst;
	return 0;
}
bool FX_Stop( fxSystem_t *system, uint32_t handle ) {
	if ( !system || !handle )
		return false;
	for ( auto &instance : system->instances ) {
		if ( instance.handle != handle )
			continue;
		instance.stopped = true;
		return true;
	}
	return false;
}
void FX_Update( fxSystem_t *system, uint32_t elapsedMs, fxTraceCallback_t trace, void *context ) {
	if ( !system || !elapsedMs )
		return;
	// A minute expires every supported particle; bound stalled-client emission work.
	elapsedMs = std::min( elapsedMs, 60000u );
	const float dt = float( elapsedMs ) * .001f;
	for ( uint32_t index = 0; index < FX_MAX_PARTICLES; ++index ) {
		auto &particle = system->particles[index];
		if ( !particle.active )
			continue;
		auto &instance = system->instances[particle.instance];
		const auto &e = instance.asset.emitters[particle.emitter];
		particle.ageMs += elapsedMs;
		if ( particle.ageMs >= e.lifetimeMs ) {
			particle.active = false;
			particle.nextFree = system->freeParticle;
			system->freeParticle = index;
			--instance.living[particle.emitter];
			--system->stats.particles;
			continue;
		}
		particle.rotation += e.angularVelocity * dt;
		float end[3];
		for ( uint32_t k = 0; k < 3; ++k ) {
			particle.previous[k] = particle.origin[k];
			particle.velocity[k] = ( particle.velocity[k] + e.gravity[k] * dt ) * std::max( 0.f, 1 - e.drag * dt );
			end[k] = particle.origin[k] + particle.velocity[k] * dt;
		}
		fxTrace_t hit{};
		if ( ( e.flags & FX_COLLISION ) && trace && trace( particle.origin, end, &hit, context ) && Between( hit.fraction, 0, 1 ) ) {
			float normalLength = 0, projection = 0;
			for ( uint32_t k = 0; k < 3; ++k ) {
				normalLength += hit.normal[k] * hit.normal[k];
				projection += particle.velocity[k] * hit.normal[k];
			}
			if ( Between( normalLength, .99f, 1.01f ) ) {
				for ( uint32_t k = 0; k < 3; ++k ) {
					end[k] = particle.origin[k] + ( end[k] - particle.origin[k] ) * hit.fraction;
					if ( projection < 0 )
						particle.velocity[k] -= 2 * projection * hit.normal[k] / normalLength;
				}
				++system->stats.collisions;
			}
		}
		std::memcpy( particle.origin, end, sizeof( end ) );
		particle.frame = uint32_t( float( particle.ageMs ) * .001f * e.fps ) % ( e.columns * e.rows );
	}
	for ( uint32_t index = 0; index < FX_MAX_INSTANCES; ++index ) {
		auto &instance = system->instances[index];
		if ( !instance.handle )
			continue;
		bool live = false;
		for ( uint32_t i = 0; i < instance.asset.header.emitterCount; ++i ) {
			const auto &e = instance.asset.emitters[i];
			if ( !instance.stopped && e.rate > 0 ) {
				instance.carry[i] += e.rate * dt;
				const uint32_t count = uint32_t( instance.carry[i] );
				instance.carry[i] -= float( count );
				Emit( system, index, i, count );
				live = true;
			}
			live = live || instance.living[i] != 0;
		}
		if ( !live ) {
			instance.handle = 0;
			--system->stats.instances;
		}
	}
}


bool DCL_Open( const void *data, size_t size, decalAsset_t *asset ) {
	if ( !data || !asset || size != 48 + sizeof( decalAsset_t ) )
		return false;
	const auto *bytes = (const uint8_t *)data;
	uint32_t envelope[2];
	std::memcpy( envelope, bytes + 8, sizeof( envelope ) );
	if ( std::memcmp( bytes, "ASDECAL", 8 ) || envelope[0] != 1 || envelope[1] != sizeof( decalAsset_t ) )
		return false;
	uint8_t hash[32];
	calc_sha_256( hash, bytes + 48, size - 48 );
	if ( std::memcmp( hash, bytes + 16, sizeof( hash ) ) )
		return false;
	decalAsset_t result;
	std::memcpy( &result, bytes + 48, sizeof( result ) );
	if ( !Text( result.name, sizeof( result.name ) ) || !Text( result.colorMap, sizeof( result.colorMap ) ) ||
		 !Text( result.normalMap, sizeof( result.normalMap ) ) || !result.lifetimeMs || result.lifetimeMs > 600000 ||
		 !result.fadeMs || result.fadeMs > result.lifetimeMs || !Between( result.normalStrength, 0, 4 ) )
		return false;
	for ( float size : result.halfSize )
		if ( !Between( size, .005f, 2048 ) )
			return false;
	for ( float color : result.color )
		if ( !Between( color, 0, 1 ) )
			return false;
	*asset = result;
	return true;
}
void DCL_Reset( decalSystem_t *system ) {
	*system = {};
}
uint32_t DCL_Add( decalSystem_t *system, const decalAsset_t *asset, const float origin[3], const float axis[3][3] ) {
	if ( !system || !asset || !origin || !axis )
		return 0;
	for ( uint32_t i = 0; i < 3; ++i ) {
		if ( !std::isfinite( origin[i] ) )
			return 0;
		for ( uint32_t j = 0; j < 3; ++j ) {
			float product = 0;
			for ( uint32_t k = 0; k < 3; ++k )
				product += axis[i][k] * axis[j][k];
			if ( !Between( product, i == j ? .999f : -.001f, i == j ? 1.001f : .001f ) )
				return 0;
		}
	}
	auto &instance = system->items[system->next];
	if ( instance.handle )
		++system->replaced;
	else
		++system->active;
	instance = {};
	if ( !++system->nextHandle )
		++system->nextHandle;
	instance.handle = system->nextHandle;
	instance.asset = *asset;
	std::memcpy( instance.origin, origin, sizeof( instance.origin ) );
	std::memcpy( instance.axis, axis, sizeof( instance.axis ) );
	system->next = ( system->next + 1 ) % DCL_MAX_DECALS;
	return instance.handle;
}
void DCL_Update( decalSystem_t *system, uint32_t elapsedMs ) {
	for ( auto &instance : system->items ) {
		if ( !instance.handle )
			continue;
		if ( elapsedMs >= instance.asset.lifetimeMs - instance.ageMs ) {
			instance.handle = 0;
			--system->active;
		} else
			instance.ageMs += elapsedMs;
	}
}
float DCL_Opacity( const decalInstance_t *instance ) {
	if ( !instance->handle )
		return 0;
	return std::min( 1.f, float( instance->asset.lifetimeMs - instance->ageMs ) / float( instance->asset.fadeMs ) );
}

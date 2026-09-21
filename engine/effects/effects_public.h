#ifndef EFFECTS_PUBLIC_H
#define EFFECTS_PUBLIC_H
#include <stddef.h>
#include <stdint.h>
#include <type_traits>

inline constexpr uint32_t FX_MAX_EMITTERS = 32, FX_MAX_PARTICLES = 4096, FX_MAX_INSTANCES = 128;
enum : uint32_t { FX_SPRITE,
	FX_MESH,
	FX_TRAIL };
enum : uint32_t { FX_COLLISION = 1,
	FX_SOFT = 2,
	FX_LIT = 4 };
struct fxFileHeader_t {
	char name[32];
	uint32_t emitterCount;
	char decal[64];
};
struct fxFileEmitter_t {
	char name[32], material[64], model[64];
	uint32_t kind, capacity, burst, lifetimeMs, flags, columns, rows;
	float rate, size, velocity[3], gravity[3], drag, color[4], fps;
	float velocitySpread[3], originSpread[3], endSize, rotation, rotationSpread, angularVelocity;
	float lightRadius, lightIntensity, lightColor[3];
};
struct fxAsset_t {
	fxFileHeader_t header;
	fxFileEmitter_t emitters[FX_MAX_EMITTERS];
};
struct fxParticle_t {
	bool active;
	uint32_t instance, emitter, ageMs, frame, nextFree;
	float origin[3], previous[3], velocity[3], rotation;
};
struct fxInstance_t {
	uint32_t handle, seed;
	bool stopped;
	float origin[3], axis[3][3], carry[FX_MAX_EMITTERS];
	uint32_t living[FX_MAX_EMITTERS];
	// An active burst retains its definition across a later hot reload.
	fxAsset_t asset;
};
struct fxStats_t {
	uint32_t particles, instances;
	uint64_t dropped, collisions;
};
struct fxSystem_t {
	fxStats_t stats;
	uint32_t nextHandle, freeParticle;
	fxInstance_t instances[FX_MAX_INSTANCES];
	fxParticle_t particles[FX_MAX_PARTICLES];
};
struct fxTrace_t {
	float fraction, normal[3];
};
using fxTraceCallback_t = bool ( * )( const float start[3], const float end[3], fxTrace_t *trace, void *context );
static_assert( sizeof( fxFileHeader_t ) == 100 && sizeof( fxFileEmitter_t ) == 304 );
static_assert( offsetof( fxFileEmitter_t, kind ) == 160 && offsetof( fxFileEmitter_t, rate ) == 188 );
static_assert( std::is_trivially_copyable_v<fxAsset_t> && std::is_trivially_copyable_v<fxSystem_t> );
bool FX_Open( const void *data, size_t size, fxAsset_t *asset );
void FX_Reset( fxSystem_t *system );
// Valid cooked definition; origin and orthonormal axes are caller-owned presentation data.
uint32_t FX_Start( fxSystem_t *system, const fxAsset_t *asset, const float origin[3], const float axis[3][3], uint32_t seed );
bool FX_Stop( fxSystem_t *system, uint32_t handle );
// Presentation time only. No engine/game clock, world state or heap allocation.
void FX_Update( fxSystem_t *system, uint32_t elapsedMs, fxTraceCallback_t trace, void *context );

inline constexpr uint32_t DCL_MAX_DECALS = 128;
struct decalAsset_t {
	char name[32], colorMap[64], normalMap[64];
	uint32_t lifetimeMs, fadeMs;
	float halfSize[3], color[4], normalStrength;
};
struct decalInstance_t {
	uint32_t handle, ageMs;
	float origin[3], axis[3][3];
	decalAsset_t asset;
};
struct decalSystem_t {
	decalInstance_t items[DCL_MAX_DECALS];
	uint32_t next, nextHandle, active;
	uint64_t replaced;
};
static_assert( sizeof( decalAsset_t ) == 200 && offsetof( decalAsset_t, halfSize ) == 168 );
static_assert( std::is_trivially_copyable_v<decalSystem_t> );
bool DCL_Open( const void *data, size_t size, decalAsset_t *asset );
void DCL_Reset( decalSystem_t *system );
// Definition comes from Open; axes are orthonormal U/V/outward normal. No heap.
uint32_t DCL_Add( decalSystem_t *system, const decalAsset_t *asset, const float origin[3], const float axis[3][3] );
void DCL_Update( decalSystem_t *system, uint32_t elapsedMs );
float DCL_Opacity( const decalInstance_t *instance );
#endif

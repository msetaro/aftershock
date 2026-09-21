#ifndef PHYSICS_PUBLIC_H
#define PHYSICS_PUBLIC_H
#include <cstddef>
#include <cstdint>

// Single client-owned cosmetic world. Metres, seconds, Z up; never used by CM
// movement or damage. Slots and all shapes are prepared before Phys_Start.
constexpr uint32_t PHYS_MAX_BODIES = 256;
constexpr uint32_t PHYS_INVALID_BODY = UINT32_MAX;
struct physTransform_t {
	float position[3];
	float rotation[4]; // xyzw
};
struct physBodyDesc_t {
	physTransform_t transform;
	float halfExtent[3]; // box; a positive radius selects a sphere instead
	float radius;
	bool dynamic;
};
struct physStats_t {
	size_t used;
	uint32_t allocations;
	uint32_t liveBlocks;
};
// Storage outlives Shutdown. Fatal must terminate, never Com_Error/longjmp.
bool Phys_Init( void *storage, size_t bytes, void ( *fatal )() );
uint32_t Phys_Prepare( const physBodyDesc_t *description );
bool Phys_Start();
bool Phys_Spawn( uint32_t slot, const physTransform_t *pose, const float velocity[3] );
bool Phys_Despawn( uint32_t slot );
bool Phys_Step(); // exactly 1/60 second
bool Phys_Transform( uint32_t slot, physTransform_t *pose );
bool Phys_Ray( const float origin[3], const float displacement[3], float *fraction );
// Sweep a prepared convex shape, excluding its own body and inactive slots.
bool Phys_Sweep( uint32_t slot, const physTransform_t *pose, const float displacement[3], float *fraction );
physStats_t Phys_Stats();
void Phys_Shutdown();
#endif

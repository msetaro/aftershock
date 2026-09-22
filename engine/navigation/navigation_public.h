#ifndef NAVIGATION_PUBLIC_H
#define NAVIGATION_PUBLIC_H
#include <stddef.h>
#include <stdint.h>

constexpr uint32_t NAV_MAX_POINTS = 256;
struct navPoint_t {
	float position[3];
	uint32_t link; // Authored off-mesh ID at the beginning of that segment, or zero.
};
struct navPath_t {
	navPoint_t points[NAV_MAX_POINTS];
	uint32_t count;
	bool complete;
};
struct navAgent_t {
	float position[3], velocity[3];
	bool offMesh, partial;
};
struct navWorld_t;
// Coordinates are engine Z-up. The world owns copied data and all query/crowd
// capacity until Close. Queries, agent changes and updates allocate no memory.
navWorld_t *Nav_Open( const void *data, size_t size, uint32_t collisionChecksum, uint32_t maxAgents );
void Nav_Close( navWorld_t *world );
bool Nav_Path( navWorld_t *world, const float start[3], const float end[3], bool links, navPath_t *out );
int Nav_AddAgent( navWorld_t *world, const float position[3], float speed );
void Nav_RemoveAgent( navWorld_t *world, int index );
bool Nav_AgentTarget( navWorld_t *world, int index, const float position[3] );
bool Nav_Agent( const navWorld_t *world, int index, navAgent_t *out );
// Steering only: authoritative gameplay still moves through native usercmd/Pmove.
void Nav_Update( navWorld_t *world, float seconds );
#endif

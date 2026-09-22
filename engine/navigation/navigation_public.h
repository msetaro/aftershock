#ifndef NAVIGATION_PUBLIC_H
#define NAVIGATION_PUBLIC_H
#include <stddef.h>
#include <stdint.h>

constexpr uint32_t NAV_MAX_POINTS = 256;
enum navLinkKind_t : uint32_t { NAV_LINK_NONE,
	NAV_LINK_JUMP,
	NAV_LINK_DROP,
	NAV_LINK_DOOR,
	NAV_LINK_LAUNCH };
struct navPoint_t {
	float position[3];
	uint32_t link; // Authored off-mesh ID at the beginning of that segment, or zero.
	navLinkKind_t kind;
};
struct navPath_t {
	navPoint_t points[NAV_MAX_POINTS];
	uint32_t count;
	bool complete;
};
struct navFollowState_t {
	uint32_t point, phase; // Approach, trigger, airborne; checkpoint-owned POD.
};
static_assert( sizeof( navFollowState_t ) == 8 && offsetof( navFollowState_t, phase ) == 4 );
struct navFollowOutput_t {
	float position[3];
	uint32_t link;
	navLinkKind_t kind;
	bool arrived;
};
// Feet position and grounded state come from authoritative movement. This only
// chooses a steering target/action; it never moves an actor. Reset on a new path.
bool Nav_Follow( const navPath_t &path, const float feet[3], bool grounded, float radius,
	navFollowState_t *state, navFollowOutput_t *out );
// Consume an actual native pad-touch notification, bounded to the current link.
bool Nav_TriggerLaunch( const navPath_t &path, const float mins[3], const float maxs[3], navFollowState_t *state );
struct navAgent_t {
	float position[3], velocity[3];
	bool offMesh, partial;
};
struct navCoverPoint_t {
	float position[3];
	uint32_t identity;
};
struct navCoverQuery_t {
	navCoverPoint_t points[NAV_MAX_POINTS];
	uint32_t count;
	bool complete;
};
struct navObstacle_t {
	float position[3], velocity[3], radius;
};
struct navWorld_t;
// Coordinates are engine Z-up. The world owns copied data and all query/crowd
// capacity until Close. Queries, agent changes and updates allocate no memory.
navWorld_t *Nav_Open( const void *data, size_t size, uint32_t collisionChecksum, uint32_t maxAgents );
void Nav_Close( navWorld_t *world );
navWorld_t *Nav_LoadFile( const char *path, uint32_t collisionChecksum, uint32_t maxAgents, uint8_t hash[32] );
bool Nav_Path( navWorld_t *world, const float start[3], const float end[3], bool links, navPath_t *out );
// Nearby collision-boundary candidates, closest first. Caller checks actual
// threat occlusion/reachability; complete is false if query capacity was exceeded.
bool Nav_CoverPoints( navWorld_t *world, const float position[3], float range, navCoverQuery_t *out );
// Stateless ground steering from authoritative actor snapshots, at most 64
// neighbors. It shares Detour crowd avoidance, without an asynchronous path queue.
bool Nav_Avoid( navWorld_t *world, const float position[3], const float velocity[3], const float desired[3],
	float speed, const navObstacle_t *obstacles, uint32_t count, float out[3] );
int Nav_AddAgent( navWorld_t *world, const float position[3], float speed );
void Nav_RemoveAgent( navWorld_t *world, int index );
bool Nav_AgentTarget( navWorld_t *world, int index, const float position[3] );
bool Nav_Agent( const navWorld_t *world, int index, navAgent_t *out );
// Steering only: authoritative gameplay still moves through native usercmd/Pmove.
void Nav_Update( navWorld_t *world, float seconds );
#endif

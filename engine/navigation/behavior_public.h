#ifndef BEHAVIOR_PUBLIC_H
#define BEHAVIOR_PUBLIC_H
#include <stddef.h>
#include <stdint.h>
#include <type_traits>

enum aiAction_t : uint32_t { AI_IDLE,
	AI_PATROL,
	AI_INVESTIGATE,
	AI_COVER,
	AI_ATTACK };
struct aiBehaviorHeader_t {
	char name[32];
	uint32_t stateCount, transitionCount, initial;
};
struct aiBehaviorState_t {
	char name[32];
	uint32_t parent;
	aiAction_t action;
	uint32_t first, count;
};
struct aiTransition_t {
	uint32_t target, field, operation;
	float value;
	uint32_t minimumTime;
};
struct aiBehavior_t {
	aiBehaviorHeader_t header;
	aiBehaviorState_t states[32];
	aiTransition_t transitions[128];
};
struct aiState_t {
	uint32_t current, elapsed, transitions, initialized;
};
struct aiInputs_t {
	bool visible, heard, covered;
	float health, distance;
};
static_assert( sizeof( aiBehaviorHeader_t ) == 44 && offsetof( aiBehaviorHeader_t, stateCount ) == 32 );
static_assert( sizeof( aiBehaviorState_t ) == 48 && offsetof( aiBehaviorState_t, parent ) == 32 );
static_assert( sizeof( aiTransition_t ) == 20 && offsetof( aiTransition_t, value ) == 12 );
static_assert( sizeof( aiState_t ) == 16 && std::is_trivially_copyable_v<aiState_t> );
static_assert( std::is_trivially_copyable_v<aiBehavior_t> );
bool AI_LoadBehavior( const char *path, aiBehavior_t *out, uint8_t hash[32] );
bool AI_ReadBehavior( const void *data, size_t size, aiBehavior_t *out );
// Requires a behavior returned by AI_ReadBehavior. One transition per fixed tick,
// ordered leaf rules before inherited parent rules.
// Zero-initialized state starts at the authored initial leaf. No allocation.
aiAction_t AI_UpdateBehavior( const aiBehavior_t &behavior, const aiInputs_t &input, uint32_t milliseconds, aiState_t *state );
const char *AI_StateName( const aiBehavior_t &behavior, const aiState_t &state );
#endif

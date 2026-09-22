#include "behavior_public.h"
#include "../../third_party/sha256/sha-256.h"
#include <bit>
#include <cmath>
#include <cstring>

static_assert( std::endian::native == std::endian::little );
static bool Name( const char *name ) {
	if ( name[0] < 'a' || name[0] > 'z' || !std::memchr( name, 0, 32 ) )
		return false;
	for ( const char *p = name; *p; ++p )
		if ( ( *p < 'a' || *p > 'z' ) && ( *p < '0' || *p > '9' ) && *p != '_' )
			return false;
	return true;
}
static bool Leaf( const aiBehavior_t &behavior, uint32_t index ) {
	for ( uint32_t i = 0; i < behavior.header.stateCount; ++i )
		if ( behavior.states[i].parent == index )
			return false;
	return index < behavior.header.stateCount;
}
bool AI_ReadBehavior( const void *bytes, size_t size, aiBehavior_t *out ) {
	if ( !bytes || !out || size < 48 + sizeof( aiBehaviorHeader_t ) || size > 48 + sizeof( aiBehavior_t ) )
		return false;
	const auto *data = static_cast<const uint8_t *>( bytes );
	uint32_t version, payload;
	std::memcpy( &version, data + 8, 4 );
	std::memcpy( &payload, data + 12, 4 );
	if ( std::memcmp( data, "ASAI\0\0\0\0", 8 ) || version != 1 || payload != size - 48 )
		return false;
	uint8_t hash[32];
	calc_sha_256( hash, data + 48, payload );
	if ( std::memcmp( hash, data + 16, 32 ) )
		return false;
	aiBehavior_t result{};
	std::memcpy( &result.header, data + 48, sizeof( result.header ) );
	const auto &h = result.header;
	if ( !Name( h.name ) || !h.stateCount || h.stateCount > 32 || h.transitionCount > 128 || h.initial >= h.stateCount ||
		 payload != sizeof( h ) + h.stateCount * sizeof( aiBehaviorState_t ) + h.transitionCount * sizeof( aiTransition_t ) )
		return false;
	std::memcpy( result.states, data + 48 + sizeof( h ), h.stateCount * sizeof( aiBehaviorState_t ) );
	std::memcpy( result.transitions, data + 48 + sizeof( h ) + h.stateCount * sizeof( aiBehaviorState_t ), h.transitionCount * sizeof( aiTransition_t ) );
	uint32_t first = 0;
	for ( uint32_t i = 0; i < h.stateCount; ++i ) {
		const auto &row = result.states[i];
		if ( !Name( row.name ) || row.action > AI_ATTACK || ( row.parent != UINT32_MAX && row.parent >= h.stateCount ) ||
			 row.first != first || row.count > 16 || row.count > h.transitionCount - first )
			return false;
		first += row.count;
		for ( uint32_t j = 0; j < i; ++j )
			if ( !std::strcmp( row.name, result.states[j].name ) )
				return false;
	}
	if ( first != h.transitionCount || !Leaf( result, h.initial ) )
		return false;
	for ( uint32_t i = 0; i < h.stateCount; ++i ) {
		uint32_t current = i, depth = 0;
		while ( current != UINT32_MAX ) {
			if ( ++depth > 8 )
				return false;
			current = result.states[current].parent;
		}
	}
	for ( uint32_t i = 0; i < h.transitionCount; ++i ) {
		const auto &rule = result.transitions[i];
		if ( !Leaf( result, rule.target ) || rule.field > 5 || rule.operation > 5 ||
			 rule.minimumTime > 600000 || !std::isfinite( rule.value ) || rule.value < 0 )
			return false;
		const bool boolean = rule.field == 0 || rule.field == 1 || rule.field == 3;
		if ( boolean && ( ( rule.value != 0 && rule.value != 1 ) || rule.operation > 1 ) )
			return false;
		const float maximum = rule.field == 4 ? 3600000.0f : rule.field == 5 ? 1048576.0f
																			 : 1.0f;
		if ( rule.value > maximum )
			return false;
	}
	*out = result;
	return true;
}
static bool Compare( float input, const aiTransition_t &rule ) {
	switch ( rule.operation ) {
	case 0:
		return input == rule.value;
	case 1:
		return input != rule.value;
	case 2:
		return input < rule.value;
	case 3:
		return input <= rule.value;
	case 4:
		return input > rule.value;
	case 5:
		return input >= rule.value;
	default:
		return false;
	}
}
aiAction_t AI_UpdateBehavior( const aiBehavior_t &behavior, const aiInputs_t &input, uint32_t milliseconds, aiState_t *state ) {
	if ( !state || !behavior.header.stateCount || behavior.header.stateCount > 32 || !milliseconds || milliseconds > 1000 ||
		 !std::isfinite( input.health ) || input.health < 0 || input.health > 1 ||
		 !std::isfinite( input.distance ) || input.distance < 0 || input.distance > 1048576 )
		return AI_IDLE;
	if ( !state->initialized )
		*state = { behavior.header.initial, 0, 0, 1 };
	if ( state->current >= behavior.header.stateCount )
		return AI_IDLE;
	state->elapsed = state->elapsed > UINT32_MAX - milliseconds ? UINT32_MAX : state->elapsed + milliseconds;
	const float values[] = { float( input.visible ), float( input.heard ), input.health, float( input.covered ), float( state->elapsed ), input.distance };
	for ( uint32_t current = state->current; current != UINT32_MAX; current = behavior.states[current].parent ) {
		const auto &row = behavior.states[current];
		for ( uint32_t i = 0; i < row.count; ++i ) {
			const auto &rule = behavior.transitions[row.first + i];
			if ( state->elapsed < rule.minimumTime || !Compare( values[rule.field], rule ) )
				continue;
			state->current = rule.target;
			state->elapsed = 0;
			if ( state->transitions != UINT32_MAX )
				++state->transitions;
			return behavior.states[state->current].action;
		}
	}
	return behavior.states[state->current].action;
}
const char *AI_StateName( const aiBehavior_t &behavior, const aiState_t &state ) {
	return state.initialized && state.current < behavior.header.stateCount ? behavior.states[state.current].name : "";
}

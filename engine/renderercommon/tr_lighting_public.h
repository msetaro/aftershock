#pragma once

#include <stddef.h>
#include <stdint.h>
#include <type_traits>

enum class sceneLightType_t : uint32_t { Point,
	Spot };
// Native presentation data, never network state. Color is linear RGB (0..64),
// intensity is a multiplier (0..64), radius is in world units (2..MAX_WORLD_COORD).
// Spot direction follows the light rays; cone angles are half angles in degrees.
struct sceneLight_t {
	float origin[3], radius;
	float color[3], intensity;
	float direction[3], innerCone, outerCone;
	sceneLightType_t type;
};
static_assert( sizeof( sceneLight_t ) == 56 && offsetof( sceneLight_t, type ) == 52 );
static_assert( std::is_trivially_copyable_v<sceneLight_t> );

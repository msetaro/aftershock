#pragma once
#include <cstdint>
#include <type_traits>

constexpr uint32_t STREAM_MAX_IMAGES = 2048;
constexpr uint32_t STREAM_NO_IMAGE = UINT32_MAX;

struct streamImage_t {
	uint64_t bytes[16]; // Device allocation requirements for each remaining mip chain.
	uint32_t tailMip, residentMip, lastUsed;
	bool used;
};
struct streamPlan_t {
	uint64_t residentBytes, floorBytes;
	uint32_t index, mip;
};
static_assert( std::is_trivially_copyable_v<streamImage_t> && std::is_trivially_copyable_v<streamPlan_t> );

// Pure decision: the caller changes residentMip only after successful GPU adoption.
// transientBytes includes uploads and retired images awaiting their last-use fence.
// False means invalid records or a budget too small even for safe coarse residency.
bool R_PlanTextureResidency( const streamImage_t *images, uint32_t count, uint32_t frame,
	uint64_t budget, uint64_t transientBytes, streamPlan_t *plan );

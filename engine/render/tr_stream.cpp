#include "tr_stream.h"
#include <algorithm>

bool R_PlanTextureResidency( const streamImage_t *images, uint32_t count, uint32_t frame,
	uint64_t budget, uint64_t transientBytes, streamPlan_t *plan ) {
	if ( !plan )
		return false;
	*plan = { transientBytes, 0, STREAM_NO_IMAGE, 0 };
	if ( ( count && !images ) || count > STREAM_MAX_IMAGES || transientBytes > budget )
		return false;
	uint32_t order[STREAM_MAX_IMAGES], desired[STREAM_MAX_IMAGES];
	uint64_t reserve = 0;
	for ( uint32_t i = 0; i < count; ++i ) {
		const auto &image = images[i];
		if ( image.tailMip >= 16 || image.residentMip > image.tailMip )
			return false;
		for ( uint32_t mip = 0; mip <= image.tailMip; ++mip )
			if ( !image.bytes[mip] || ( mip && image.bytes[mip] > image.bytes[mip - 1] ) )
				return false;
		const uint64_t resident = image.bytes[image.residentMip], tail = image.bytes[image.tailMip];
		if ( resident > budget - plan->residentBytes || tail > budget - plan->floorBytes )
			return false;
		plan->residentBytes += resident;
		plan->floorBytes += tail;
		reserve = std::max( reserve, tail );
		order[i] = i;
		desired[i] = image.tailMip;
	}
	// Keep room for a replacement tail while the old image is still in flight.
	if ( reserve > budget - plan->floorBytes )
		return false;
	// ponytail: one transition at a time; widen only if measured fly-through latency needs it.
	if ( transientBytes )
		return true;
	std::sort( order, order + count, [&]( uint32_t a, uint32_t b ) {
		const uint32_t ageA = images[a].used ? frame - images[a].lastUsed : UINT32_MAX;
		const uint32_t ageB = images[b].used ? frame - images[b].lastUsed : UINT32_MAX;
		if ( ageA != ageB )
			return ageA < ageB;
		if ( images[a].residentMip != images[b].residentMip )
			return images[a].residentMip < images[b].residentMip;
		return a < b;
	} );
	uint64_t available = budget - plan->floorBytes - reserve;
	for ( uint32_t n = 0; n < count; ++n ) {
		const uint32_t i = order[n];
		const auto &image = images[i];
		if ( !image.used || frame - image.lastUsed > 120 )
			continue;
		for ( uint32_t mip = 0; mip < image.tailMip; ++mip ) {
			const uint64_t extra = image.bytes[mip] - image.bytes[image.tailMip];
			if ( extra <= available ) {
				desired[i] = mip;
				available -= extra;
				break;
			}
		}
	}
	for ( uint32_t n = count; n > 0; --n ) {
		const uint32_t i = order[n - 1];
		if ( desired[i] <= images[i].residentMip )
			continue;
		for ( uint32_t mip = desired[i]; mip <= images[i].tailMip; ++mip ) {
			if ( images[i].bytes[mip] <= budget - plan->residentBytes ) {
				plan->index = i;
				plan->mip = mip;
				return true;
			}
		}
	}
	for ( uint32_t n = 0; n < count; ++n ) {
		const uint32_t i = order[n];
		for ( uint32_t mip = desired[i]; mip < images[i].residentMip; ++mip ) {
			if ( images[i].bytes[mip] <= budget - plan->residentBytes ) {
				plan->index = i;
				plan->mip = mip;
				return true;
			}
		}
	}
	return true;
}

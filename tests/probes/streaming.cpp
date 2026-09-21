#include "../../engine/render/tr_stream.h"
#include <cassert>
#include <cstdint>

int main() {
	streamImage_t images[3]{};
	for ( auto &image : images ) {
		// Deliberately non-geometric Vulkan allocation sizes, not file byte sizes.
		image.bytes[0] = 1024;
		image.bytes[1] = 384;
		image.bytes[2] = 128;
		image.tailMip = image.residentMip = 2;
	}
	streamPlan_t plan{};
	assert( R_PlanTextureResidency( images, 3, 100, 1536, 0, &plan ) );
	assert( plan.index == STREAM_NO_IMAGE && plan.residentBytes == 384 && plan.floorBytes == 384 );
	images[0].used = true;
	images[0].lastUsed = 100;
	assert( R_PlanTextureResidency( images, 3, 100, 1536, 0, &plan ) );
	assert( plan.index == 0 && plan.mip == 0 && plan.residentBytes + images[0].bytes[plan.mip] <= 1536 );
	images[0].residentMip = plan.mip;
	// An upload/retired image still occupies device memory. Serialize transitions.
	assert( R_PlanTextureResidency( images, 3, 101, 1536, 128, &plan ) );
	assert( plan.index == STREAM_NO_IMAGE && plan.residentBytes == 1408 );
	images[1].used = true;
	images[1].lastUsed = 101;
	assert( R_PlanTextureResidency( images, 3, 101, 1536, 0, &plan ) );
	assert( plan.index == 0 && plan.mip == 2 ); // Evict older before promoting newer.
	images[0].residentMip = plan.mip;
	assert( R_PlanTextureResidency( images, 3, 101, 1536, 0, &plan ) );
	assert( plan.index == 1 && plan.mip == 0 );
	images[1].residentMip = plan.mip;
	// Equal priority preserves current quality; a crowded view must not oscillate.
	images[0].lastUsed = images[1].lastUsed = 102;
	assert( R_PlanTextureResidency( images, 3, 102, 1536, 0, &plan ) );
	assert( plan.index == STREAM_NO_IMAGE );
	assert( R_PlanTextureResidency( images, 3, 223, 1536, 0, &plan ) );
	assert( plan.index == 1 && plan.mip == 2 ); // Cold textures keep their tail.
	images[1].residentMip = 2;
	images[0].lastUsed = UINT32_MAX - 2;
	images[1].used = false;
	assert( R_PlanTextureResidency( images, 3, 1, 1536, 0, &plan ) );
	assert( plan.index == 0 && plan.mip == 0 );
	// Never overcommit even when only a smaller promotion fits alongside the old tail.
	assert( R_PlanTextureResidency( images, 3, 1, 900, 0, &plan ) );
	assert( plan.index == 0 && plan.mip == 1 );
	// Downgrade through a smaller tail when the preferred replacement cannot coexist.
	images[0].residentMip = 0;
	images[1].residentMip = images[2].residentMip = 1;
	images[0].lastUsed = 99;
	images[1].lastUsed = 100;
	images[2].lastUsed = 98;
	images[0].used = images[1].used = images[2].used = true;
	assert( R_PlanTextureResidency( images, 3, 100, 2048, 0, &plan ) );
	assert( plan.index == 0 && plan.mip == 2 );
	// Repeated decisions must settle, without exceeding the budget during any swap.
	for ( uint32_t step = 0; step < 16; ++step ) {
		assert( R_PlanTextureResidency( images, 3, 100, 2048, 0, &plan ) );
		if ( plan.index == STREAM_NO_IMAGE )
			break;
		assert( step < 15 && plan.residentBytes + images[plan.index].bytes[plan.mip] <= 2048 );
		images[plan.index].residentMip = plan.mip;
	}
	assert( !R_PlanTextureResidency( images, 3, 1, 383, 0, &plan ) );
	assert( !R_PlanTextureResidency( images, 3, 1, 1536, UINT64_MAX, &plan ) );
	assert( !R_PlanTextureResidency( nullptr, 1, 1, 1536, 0, &plan ) );
	assert( !R_PlanTextureResidency( images, STREAM_MAX_IMAGES + 1, 1, 1536, 0, &plan ) );
	assert( !R_PlanTextureResidency( images, 3, 1, 1536, 0, nullptr ) );
	images[0].tailMip = 16;
	assert( !R_PlanTextureResidency( images, 3, 1, 1536, 0, &plan ) );
	images[0].tailMip = 2;
	images[0].bytes[1] = 2048;
	assert( !R_PlanTextureResidency( images, 3, 1, 1536, 0, &plan ) );
}

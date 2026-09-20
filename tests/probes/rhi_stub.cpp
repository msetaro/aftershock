// Compile-only alternative backend. Never linked into a rendering client.
#include "../../engine/rhi/rhi_public.h"
#include <stdlib.h>

bool RHI_Available( void ) {
	return false;
}

rhiStats_t RHI_GetStats( void ) {
	return {};
}

uint32_t RHI_UploadUniform( const void *, uint32_t ) {
	return RHI_INVALID_OFFSET;
}

void RHI_CreateTexture( rhiTexture_t *, int32_t, int32_t, int32_t, rhiFormat_t, rhiAddress_t, const char * ) {
	abort();
}

void RHI_UploadTexture( const rhiTexture_t *, rhiFormat_t, int32_t, int32_t, int32_t, int32_t, int32_t, uint8_t *, int32_t, bool ) {
	abort();
}

void RHI_UpdateTextureSampler( const rhiTexture_t *, rhiAddress_t, bool ) {
	abort();
}

void RHI_DestroyTexture( rhiTexture_t * ) {
	abort();
}

void RHI_BindTexture( uint32_t, const rhiTexture_t * ) {
	abort();
}

#ifdef RHI_STUB_CHECK
int main( void ) {
	const rhiStats_t stats = RHI_GetStats();
	const float uniform[32] = {};
	return RHI_Available() || stats.frameSlots != 0 || stats.geometryBytes != 0 ||
		   RHI_UploadUniform( uniform, sizeof( uniform ) ) != RHI_INVALID_OFFSET;
}
#endif

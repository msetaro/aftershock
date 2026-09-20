// Compile-only alternative backend. Never linked into a rendering client.
#include "../../engine/rhi/rhi_public.h"

bool RHI_Available( void ) {
	return false;
}

rhiStats_t RHI_GetStats( void ) {
	return {};
}

uint32_t RHI_UploadUniform( const void *, uint32_t ) {
	return RHI_INVALID_OFFSET;
}

#ifdef RHI_STUB_CHECK
int main( void ) {
	const rhiStats_t stats = RHI_GetStats();
	const float uniform[32] = {};
	return RHI_Available() || stats.frameSlots != 0 || stats.geometryBytes != 0 ||
		   RHI_UploadUniform( uniform, sizeof( uniform ) ) != RHI_INVALID_OFFSET;
}
#endif

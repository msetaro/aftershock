// Compile-only alternative backend. Never linked into a rendering client.
#include "../../engine/rhi/rhi_public.h"
#include <stdlib.h>

bool RHI_Available( void ) {
	return false;
}

rhiStats_t RHI_GetStats( void ) {
	return {};
}

rhiCapabilities_t RHI_GetCapabilities( void ) {
	return {};
}

void RHI_MarkWorldPipelines( void ) {
	abort();
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

rhiStatus_t RHI_WaitIdle( void ) {
	return rhiStatus_t::Unavailable;
}

rhiStatus_t RHI_WaitQueue( void ) {
	return rhiStatus_t::Unavailable;
}

void RHI_DrawIndexed( uint32_t, uint32_t ) {
	abort();
}

void RHI_EndPass( void ) {
	abort();
}

uint32_t RHI_BeginScope( const char * ) {
	return RHI_INVALID_OFFSET;
}

void RHI_EndScope( uint32_t ) {
}

uint32_t RHI_GetTimings( const rhiTiming_t **timings ) {
	*timings = nullptr;
	return 0;
}

uint32_t RHI_FindPipeline( uint32_t, const rhiPipelineDesc_t *, bool ) {
	return UINT32_MAX;
}

void RHI_GetPipelineDesc( uint32_t, rhiPipelineDesc_t *desc ) {
	*desc = {};
}

void RHI_BindPipeline( uint32_t ) {
	abort();
}

rhiFrameState_t RHI_GetFrameState( void ) {
	return {};
}
void RHI_InvalidateViewport( void ) {
	abort();
}
rhiDeviceDescription_t RHI_GetDeviceDescription( void ) {
	return { "", "unavailable", "", "", "unavailable" };
}
rhiStatus_t RHI_SetTextureFilter( rhiFilter_t, rhiFilter_t, bool *changed ) {
	*changed = false;
	return rhiStatus_t::Unavailable;
}
void RHI_BindIndices( rhiGeometryBuffer_t, uint32_t ) {
	abort();
}
uint32_t RHI_UploadIndices( uint32_t, const void * ) {
	return RHI_INVALID_OFFSET;
}
void RHI_BindScreenMap( uint32_t ) {
	abort();
}
void RHI_ResetBinding( int32_t ) {
	abort();
}

#ifdef RHI_STUB_CHECK
int main( void ) {
	const rhiStats_t stats = RHI_GetStats();
	const float uniform[32] = {};
	return RHI_Available() || RHI_WaitIdle() != rhiStatus_t::Unavailable || RHI_WaitQueue() != rhiStatus_t::Unavailable || stats.frameSlots != 0 || stats.geometryBytes != 0 ||
		   RHI_UploadUniform( uniform, sizeof( uniform ) ) != RHI_INVALID_OFFSET;
}
#endif

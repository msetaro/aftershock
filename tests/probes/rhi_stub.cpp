// Compile-only alternative backend. Never linked into a rendering client.
#include "../../engine/rhi/rhi_public.h"
#include <stdlib.h>

const char *RHI_GetShaderPackageHash( void ) {
	return "";
}

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

rhiStatus_t RHI_CreateTexture( rhiTexture_t *, int32_t, int32_t, int32_t, rhiFormat_t, rhiAddress_t, const char * ) {
	return rhiStatus_t::Unavailable;
}

rhiStatus_t RHI_ReplaceCompressedTexture( rhiTexture_t *, int32_t, int32_t, int32_t, const uint8_t *, uint32_t, rhiFormat_t, rhiAddress_t, const char * ) {
	return rhiStatus_t::Unavailable;
}
rhiStatus_t RHI_UploadCompressedTexture( const rhiTexture_t *, int32_t, int32_t, int32_t, const uint8_t *, uint32_t, rhiFormat_t, bool ) {
	return rhiStatus_t::Unavailable;
}
rhiStatus_t RHI_UploadTexture( const rhiTexture_t *, int32_t, int32_t, int32_t, int32_t, int32_t, const uint8_t *, int32_t, bool ) {
	return rhiStatus_t::Unavailable;
}

rhiStatus_t RHI_UpdateTextureSampler( const rhiTexture_t *, rhiAddress_t, bool ) {
	return rhiStatus_t::Unavailable;
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

rhiStatus_t RHI_FindPipeline( uint32_t, const rhiPipelineDesc_t *, bool, uint32_t *pipeline ) {
	*pipeline = {};
	return rhiStatus_t::Unavailable;
}

void RHI_GetPipelineDesc( uint32_t, rhiPipelineDesc_t *desc ) {
	*desc = {};
}

rhiStatus_t RHI_BindPipeline( uint32_t ) {
	return rhiStatus_t::Unavailable;
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

void RHI_PushTransform( const float * ) {
	abort();
}
void RHI_Bloom( const float * ) {
	abort();
}
bool RHI_ReadVisibility( uint32_t ) {
	return false;
}
void RHI_DrawVisibility( uint32_t, uint32_t, const rhiRasterState_t * ) {
	abort();
}

void RHI_BindVertexStreams( rhiGeometryBuffer_t, uint32_t, const rhiVertexStream_t * ) {
	abort();
}

rhiRenderArea_t RHI_GetRenderArea( void ) {
	return {};
}
bool RHI_PrepareDraw( const rhiRasterState_t *, const rhiTexture_t * ) {
	return false;
}
void RHI_Draw( uint32_t ) {
	abort();
}
void RHI_DrawBoundIndices( void ) {
	abort();
}
void RHI_BindIndexData( uint32_t, const uint32_t * ) {
	abort();
}
void RHI_ClearColor( const float *, const rhiRect_t * ) {
	abort();
}
void RHI_ClearDepth( bool, const rhiRect_t * ) {
	abort();
}

rhiStatus_t RHI_BeginFrame( bool, bool *started ) {
	*started = {};
	return rhiStatus_t::Unavailable;
}
rhiStatus_t RHI_EndFrame( bool, bool, rhiFrameEnd_t *result ) {
	*result = {};
	return rhiStatus_t::Unavailable;
}
void RHI_BeginMainPass( void ) {
	abort();
}
rhiStatus_t RHI_PresentFrame( void ) {
	return rhiStatus_t::Unavailable;
}

rhiPipelineCacheKey_t RHI_GetPipelineCacheKey( void ) {
	return {};
}
rhiStatus_t RHI_RestorePipelineCache( const void *, uint32_t ) {
	return rhiStatus_t::Unavailable;
}
rhiStatus_t RHI_ReadPipelineCache( void *, uint32_t * ) {
	return rhiStatus_t::Unavailable;
}

rhiStatus_t RHI_Initialize( const rhiDeviceConfig_t *, const rhiHost_t *, rhiDeviceInfo_t * ) {
	return rhiStatus_t::Unavailable;
}
rhiStatus_t RHI_InitDescriptors( void ) {
	return rhiStatus_t::Unavailable;
}
rhiStatus_t RHI_ReleaseResources( void ) {
	return rhiStatus_t::Unavailable;
}
rhiStatus_t RHI_Shutdown( void ) {
	return rhiStatus_t::Unavailable;
}
rhiStatus_t RHI_ReadPixels( uint8_t *, uint32_t, uint32_t ) {
	return rhiStatus_t::Unavailable;
}
rhiStatus_t RHI_UploadWorldGeometry( const uint8_t *, int32_t ) {
	return rhiStatus_t::Unavailable;
}
rhiStatus_t RHI_UpdatePostProcess( const rhiPostProcess_t * ) {
	return rhiStatus_t::Unavailable;
}

const rhiError_t *RHI_GetError( void ) {
	static const rhiError_t error = { false, "" };
	return &error;
}

#ifdef RHI_STUB_CHECK
int main( void ) {
	const rhiStats_t stats = RHI_GetStats();
	const float uniform[32] = {};
	return RHI_Initialize( nullptr, nullptr, nullptr ) != rhiStatus_t::Unavailable || RHI_Available() || RHI_WaitIdle() != rhiStatus_t::Unavailable || RHI_WaitQueue() != rhiStatus_t::Unavailable || stats.frameSlots != 0 || stats.geometryBytes != 0 ||
		   RHI_UploadUniform( uniform, sizeof( uniform ) ) != RHI_INVALID_OFFSET;
}
#endif

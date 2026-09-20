// Exercise the production upload allocator without creating a GPU device.
#include "../../engine/renderervk/vk.cpp"
#include <assert.h>


void QDECL Com_Printf( const char *, ... ) {
	abort();
}

void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}

static int destroyed;
static VkResult wait_result;
static int commands;
static uint8_t transform_bytes[64];
static void VKAPI_CALL push_transform( VkCommandBuffer command, VkPipelineLayout layout, VkShaderStageFlags stages, uint32_t offset, uint32_t size, const void *data ) {
	assert( (uintptr_t)command == 22 && (uintptr_t)layout == 34 );
	assert( stages == VK_SHADER_STAGE_VERTEX_BIT && offset == 0 && size == sizeof( transform_bytes ) );
	assert( memcmp( data, transform_bytes, size ) == 0 );
}
static int vertex_binds;
static void VKAPI_CALL bind_vertices( VkCommandBuffer command, uint32_t first, uint32_t count, const VkBuffer *buffers, const VkDeviceSize *offsets ) {
	assert( (uintptr_t)command == 22 );
	const uint32_t expectedFirst[] = { 1, 0, 0 };
	const uint32_t expectedCount[] = { 3, 3, 1 };
	const VkDeviceSize expectedOffsets[3][3] = { { 101, 202, 303 }, { 32, 90, 64 }, { 32, 0, 0 } };
	assert( vertex_binds < 3 && first == expectedFirst[vertex_binds] && count == expectedCount[vertex_binds] );
	for ( uint32_t i = 0; i < count; i++ ) {
		assert( (uintptr_t)buffers[i] == ( vertex_binds == 0 ? 30 : 31 ) );
		assert( offsets[i] == expectedOffsets[vertex_binds][i] );
	}
	vertex_binds++;
}
static int scissors;
static int viewports;
static void VKAPI_CALL set_scissor( VkCommandBuffer command, uint32_t first, uint32_t count, const VkRect2D *rect ) {
	assert( (uintptr_t)command == 22 && first == 0 && count == 1 );
	assert( rect->offset.x == 7 && rect->offset.y == 9 && rect->extent.width == 320 && rect->extent.height == 200 );
	scissors++;
}
static void VKAPI_CALL set_viewport( VkCommandBuffer command, uint32_t first, uint32_t count, const VkViewport *viewport ) {
	assert( (uintptr_t)command == 22 && first == 0 && count == 1 );
	assert( viewport->x == 3 && viewport->y == 4 && viewport->width == 600 && viewport->height == 400 );
	assert( viewport->minDepth == 0.6f && viewport->maxDepth == 1.0f );
	viewports++;
}
static VkResult VKAPI_CALL reject_descriptors( VkDevice device, const VkDescriptorSetAllocateInfo *info, VkDescriptorSet * ) {
	assert( (uintptr_t)device == 20 && info->descriptorSetCount == 1 );
	return VK_ERROR_OUT_OF_DEVICE_MEMORY;
}
static int waits;
static int index_binds;
static int sampler_destroys;
static void VKAPI_CALL bind_indices( VkCommandBuffer command, VkBuffer buffer, VkDeviceSize offset, VkIndexType type ) {
	assert( (uintptr_t)command == 22 && type == VK_INDEX_TYPE_UINT32 );
	assert( (uintptr_t)buffer == ( index_binds == 0 ? 30 : 31 ) && offset == 12 );
	index_binds++;
}
static void VKAPI_CALL destroy_sampler( VkDevice, VkSampler sampler, const VkAllocationCallbacks * ) {
	assert( (uintptr_t)sampler == 32 && waits == 2 );
	sampler_destroys++;
}
static uint32_t timestamp_writes;
static bool query_ready = true;
static void VKAPI_CALL write_timestamp( VkCommandBuffer, VkPipelineStageFlagBits stage, VkQueryPool, uint32_t query ) {
	assert( query == RHI_MAX_TIMINGS * 2 + timestamp_writes );
	assert( stage == ( timestamp_writes == 0 ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT ) );
	timestamp_writes++;
}
static VkResult VKAPI_CALL read_timestamps( VkDevice, VkQueryPool, uint32_t first, uint32_t count, size_t size, void *data, VkDeviceSize stride, VkQueryResultFlags flags ) {
	assert( first == RHI_MAX_TIMINGS * 2 && count == 2 && size >= 32 && stride == 16 );
	assert( flags == ( VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT ) );
	uint64_t *values = (uint64_t *)data;
	values[0] = 250;
	values[1] = 1;
	values[2] = 5;
	values[3] = query_ready ? 1 : 0;
	return query_ready ? VK_SUCCESS : VK_NOT_READY;
}
static VkResult VKAPI_CALL wait_device( VkDevice device ) {
	assert( (uintptr_t)device == 20 );
	waits++;
	return wait_result;
}
static VkResult VKAPI_CALL wait_queue( VkQueue queue ) {
	assert( (uintptr_t)queue == 21 );
	return wait_result;
}
static void VKAPI_CALL draw_indexed( VkCommandBuffer command, uint32_t count, uint32_t instances, uint32_t first, int32_t base, uint32_t firstInstance ) {
	assert( (uintptr_t)command == 22 && commands == 0 );
	assert( count == 9 && first == 3 && instances == 1 && base == 0 && firstInstance == 0 );
	commands = 1;
}
static void VKAPI_CALL end_pass( VkCommandBuffer command ) {
	assert( (uintptr_t)command == 22 && commands == 1 );
	commands = 2;
}
static void VKAPI_CALL destroy_image( VkDevice, VkImage image, const VkAllocationCallbacks * ) {
	assert( (uintptr_t)image == 11 && destroyed == 0 );
	destroyed = 1;
}
static void VKAPI_CALL destroy_view( VkDevice, VkImageView view, const VkAllocationCallbacks * ) {
	assert( (uintptr_t)view == 12 && destroyed == 1 );
	destroyed = 2;
}

static VkResult VKAPI_CALL cache_data( VkDevice device, VkPipelineCache cache, size_t *size, void *data ) {
	assert( (uintptr_t)device == 20 && (uintptr_t)cache == 90 );
	if ( data ) {
		assert( *size >= 4 );
		memcpy( data, "data", 4 );
	}
	*size = 4;
	return VK_SUCCESS;
}
static VkResult VKAPI_CALL restore_cache( VkDevice device, const VkPipelineCacheCreateInfo *info, const VkAllocationCallbacks *, VkPipelineCache *cache ) {
	assert( (uintptr_t)device == 20 && info->initialDataSize == sizeof( VkPipelineCacheHeaderVersionOne ) );
	assert( info->pInitialData != nullptr );
	*cache = (VkPipelineCache)(uintptr_t)91;
	return VK_SUCCESS;
}
static void VKAPI_CALL destroy_cache( VkDevice device, VkPipelineCache cache, const VkAllocationCallbacks * ) {
	assert( (uintptr_t)device == 20 && (uintptr_t)cache == 90 );
}

static void *missing_loader_entry( uint64_t instance, const char *name ) {
	assert( instance == 0 && strcmp( name, "vkCreateInstance" ) == 0 );
	return nullptr;
}

int main( void ) {
	vk_config.uniformBytes = 128;
	assert( !RHI_GetCapabilities().active );
	vk.active = vk.wideLines = vk.fragmentStores = vk.clearAttachment = vk.fboActive = vk.offscreenRender = qtrue;
	vk.maxBoundDescriptorSets = 8;
	const rhiCapabilities_t caps = RHI_GetCapabilities();
	assert( caps.active && caps.wideLines && caps.fragmentStores && caps.clearAttachment && caps.fboActive && caps.offscreenRender && caps.maxBoundDescriptorSets == 8 );
	vk.pipelines_count = 92;
	RHI_MarkWorldPipelines();
	assert( vk.pipelines_world_base == 92 );
	byte storage[512];
	byte uniform[128];
	memset( storage, 0xa5, sizeof( storage ) );
	memset( uniform, 0x3c, sizeof( uniform ) );
	assert( !RHI_Available() );
	assert( RHI_UploadUniform( uniform, sizeof( uniform ) ) == RHI_INVALID_OFFSET );
	vk.cmd = &vk.tess[0];
	vk.cmd->vertex_buffer_ptr = storage;
	vk.cmd->vertex_buffer_offset = 1;
	vk.cmd->uniform_descriptor = (VkDescriptorSet)(uintptr_t)1;
	vk.uniform_alignment = 256;
	vk.uniform_item_size = 128;
	vk.geometry_buffer_size = sizeof( storage );
	assert( RHI_UploadUniform( uniform, sizeof( uniform ) ) == 256 );
	assert( memcmp( storage + 256, uniform, sizeof( uniform ) ) == 0 );
	assert( storage[255] == 0xa5 && storage[384] == 0xa5 );
	assert( vk.cmd->vertex_buffer_offset == 384 );
	assert( vk.cmd->descriptor_set.offset[0] == 256 );
	assert( vk.cmd->descriptor_set.current[0] == vk.cmd->uniform_descriptor );
	assert( RHI_UploadUniform( uniform, sizeof( uniform ) ) == RHI_INVALID_OFFSET );
	assert( vk.cmd->vertex_buffer_offset == 384 );
	assert( RHI_UploadUniform( uniform, sizeof( uniform ) + 1 ) == RHI_INVALID_OFFSET );
	assert( RHI_UploadUniform( nullptr, sizeof( uniform ) ) == RHI_INVALID_OFFSET );
	// A different frame slot has independent upload space and binding offsets.
	vk.cmd = &vk.tess[1];
	vk.cmd->vertex_buffer_ptr = storage;
	assert( RHI_UploadUniform( uniform, sizeof( uniform ) ) == 0 );
	assert( vk.tess[0].vertex_buffer_offset == 384 );
	assert( vk.tess[1].vertex_buffer_offset == 128 );
	const rhiStats_t stats = RHI_GetStats();
	assert( stats.geometryBytes == sizeof( storage ) && stats.frameSlots == 2 );
	assert( vk_texture_format( rhiFormat_t::RGBA8 ) == VK_FORMAT_R8G8B8A8_UNORM );
	assert( vk_texture_format( rhiFormat_t::BGRA8 ) == VK_FORMAT_B8G8R8A8_UNORM );
	assert( vk_texture_format( rhiFormat_t::RGB8 ) == VK_FORMAT_R8G8B8_UNORM );
	assert( vk_texture_format( rhiFormat_t::BGRA4 ) == VK_FORMAT_B4G4R4A4_UNORM_PACK16 );
	assert( vk_texture_format( rhiFormat_t::A1RGB5 ) == VK_FORMAT_A1R5G5B5_UNORM_PACK16 );
	assert( vk_texture_address( rhiAddress_t::Repeat ) == VK_SAMPLER_ADDRESS_MODE_REPEAT );
	assert( vk_texture_address( rhiAddress_t::ClampToEdge ) == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE );
	assert( vk_texture_address( rhiAddress_t::ClampToBorder ) == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER );
	rhiTexture_t texture = { 11, 12, 13 };
	RHI_BindTexture( 3, &texture );
	assert( (uintptr_t)vk.cmd->descriptor_set.current[3] == 13 );
	qvkDestroyImage = destroy_image;
	qvkDestroyImageView = destroy_view;
	RHI_DestroyTexture( &texture );
	assert( destroyed == 2 && texture.image == 0 && texture.view == 0 && texture.binding == 13 );
	RHI_DestroyTexture( &texture );
	assert( destroyed == 2 );
	assert( RHI_WaitIdle() == rhiStatus_t::Unavailable && RHI_WaitQueue() == rhiStatus_t::Unavailable );
	vk.device = (VkDevice)(uintptr_t)20;
	vk.queue = (VkQueue)(uintptr_t)21;
	qvkDeviceWaitIdle = wait_device;
	qvkQueueWaitIdle = wait_queue;
	const struct {
		VkResult native;
		rhiStatus_t portable;
	} results[] = {
		{ VK_SUCCESS, rhiStatus_t::Success },
		{ VK_ERROR_OUT_OF_HOST_MEMORY, rhiStatus_t::OutOfMemory },
		{ VK_ERROR_OUT_OF_DEVICE_MEMORY, rhiStatus_t::OutOfMemory },
		{ VK_ERROR_DEVICE_LOST, rhiStatus_t::DeviceLost },
		{ VK_ERROR_INITIALIZATION_FAILED, rhiStatus_t::Error }
	};
	for ( const auto &result : results ) {
		wait_result = result.native;
		assert( RHI_WaitIdle() == result.portable && RHI_WaitQueue() == result.portable );
	}
	vk.cmd->command_buffer = (VkCommandBuffer)(uintptr_t)22;
	qvkCmdDrawIndexed = draw_indexed;
	qvkCmdEndRenderPass = end_pass;
	RHI_DrawIndexed( 9, 3 );
	RHI_EndPass();
	assert( commands == 2 );
	assert( RHI_BeginScope( "unsupported" ) == RHI_INVALID_OFFSET );
	vk.timestampPool = (VkQueryPool)(uintptr_t)23;
	vk.timestampBits = 8;
	vk.timestampPeriod = 2.0f;
	qvkCmdWriteTimestamp = write_timestamp;
	qvkGetQueryPoolResults = read_timestamps;
	const uint32_t scope = RHI_BeginScope( "test scope" );
	assert( scope == 0 && timestamp_writes == 1 );
	RHI_EndScope( scope );
	RHI_EndScope( scope );
	RHI_EndScope( RHI_INVALID_OFFSET );
	assert( timestamp_writes == 2 );
	vk_read_timings();
	const rhiTiming_t *timings;
	assert( RHI_GetTimings( &timings ) == 1 );
	assert( strcmp( timings[0].name, "test scope" ) == 0 && timings[0].microseconds == 22.0 / 1000.0 );
	query_ready = false;
	vk_read_timings();
	assert( RHI_GetTimings( &timings ) == 0 );
	vk.cmd->profile.count = RHI_MAX_TIMINGS;
	assert( RHI_BeginScope( "full" ) == RHI_INVALID_OFFSET && timestamp_writes == 2 );
	float transform[16];
	for ( uint32_t i = 0; i < sizeof( transform_bytes ); i++ )
		transform_bytes[i] = (uint8_t)( i * 7 );
	memcpy( transform, transform_bytes, sizeof( transform ) );
	vk.pipeline_layout = (VkPipelineLayout)(uintptr_t)34;
	qvkCmdPushConstants = push_transform;
	vk.stats.push_size = 0;
	RHI_PushTransform( transform );
	assert( vk.stats.push_size == sizeof( transform ) );
	uint32_t visibility[16] = {};
	visibility[8] = 7;
	vk.storage.buffer_ptr = (byte *)visibility;
	vk.storage_alignment = 16;
	assert( !RHI_ReadVisibility( 0 ) && !RHI_ReadVisibility( 1 ) && RHI_ReadVisibility( 2 ) );
	// Frame state and binding cache expose no SDK objects to the frontend.
	assert( !RHI_GetFrameState().submitted && !RHI_GetFrameState().screenMapPass );
	vk.cmd->waitForFence = qtrue;
	vk.renderPassIndex = RENDER_PASS_SCREENMAP;
	vk.capture.image = (VkImage)(uintptr_t)29;
	const rhiFrameState_t frame = RHI_GetFrameState();
	assert( frame.submitted && frame.screenMapPass && frame.captureImage );
	RHI_InvalidateViewport();
	assert( vk.cmd->depth_range == DEPTH_RANGE_COUNT );
	vk.vbo.vertex_buffer = (VkBuffer)(uintptr_t)30;
	vk.cmd->vertex_buffer = (VkBuffer)(uintptr_t)31;
	qvkCmdBindIndexBuffer = bind_indices;
	RHI_BindIndices( rhiGeometryBuffer_t::World, 12 );
	RHI_BindIndices( rhiGeometryBuffer_t::World, 12 );
	RHI_BindIndices( rhiGeometryBuffer_t::Frame, 12 );
	assert( index_binds == 2 );
	const uint32_t indices[] = { 3, 2, 1 };
	vk.cmd->vertex_buffer_offset = 0;
	assert( RHI_UploadIndices( 3, indices ) == 0 );
	assert( memcmp( storage, indices, sizeof( indices ) ) == 0 );
	vk.cmd->vertex_buffer_offset = 504;
	assert( RHI_UploadIndices( 3, indices ) == RHI_INVALID_OFFSET );
	assert( vk.cmd->vertex_buffer_offset == 504 && vk.geometry_buffer_size_new == 1024 );
	vk.screenMap.color_descriptor = (VkDescriptorSet)(uintptr_t)33;
	RHI_BindScreenMap( RHI_BINDING_TEXTURE1 );
	assert( (uintptr_t)vk.cmd->descriptor_set.current[RHI_BINDING_TEXTURE1] == 33 );
	RHI_ResetBinding( RHI_BINDING_TEXTURE1 );
	assert( vk.cmd->descriptor_set.current[RHI_BINDING_TEXTURE1] == VK_NULL_HANDLE );

	rhiVertexStream_t streams[RHI_MAX_VERTEX_STREAMS] = {};
	qvkCmdBindVertexBuffers = bind_vertices;
	streams[1].offset = 101;
	streams[3].offset = 303;
	vk.cmd->vbo_offset[2] = 202;
	RHI_BindVertexStreams( rhiGeometryBuffer_t::World, ( 1U << 1 ) | ( 1U << 3 ), streams );
	const uint8_t vertices[] = { 1, 3, 5, 7, 9 };
	streams[0].data = streams[2].data = vertices;
	streams[0].size = 3;
	streams[2].size = 5;
	vk.cmd->vertex_buffer_offset = 1;
	vk.cmd->buf_offset[1] = 90;
	RHI_BindVertexStreams( rhiGeometryBuffer_t::Frame, ( 1U << 0 ) | ( 1U << 2 ), streams );
	assert( vk.cmd->vertex_buffer_offset == 69 );
	assert( memcmp( storage + 32, vertices, 3 ) == 0 && memcmp( storage + 64, vertices, 5 ) == 0 );
	vk.cmd->vertex_buffer_offset = 500;
	vk.geometry_buffer_size_new = 0;
	RHI_BindVertexStreams( rhiGeometryBuffer_t::Frame, 1, streams );
	assert( vk.cmd->vertex_buffer_offset == 500 && vk.geometry_buffer_size_new == 1024 );
	RHI_BindVertexStreams( rhiGeometryBuffer_t::Frame, 0, streams );
	assert( vertex_binds == 3 );

	const rhiRasterState_t raster = { DEPTH_RANGE_WEAPON, { { 7, 9 }, { 320, 200 } }, { 3, 4, 600, 400, 0.6f, 1.0f } };
	qvkCmdSetScissor = set_scissor;
	qvkCmdSetViewport = set_viewport;
	vk.cmd->descriptor_set.start = ~0U;
	assert( !RHI_PrepareDraw( &raster, &texture ) );
	assert( scissors == 0 && viewports == 0 );
	vk.geometry_buffer_size_new = 0;
	assert( RHI_PrepareDraw( &raster, &texture ) );
	assert( scissors == 1 && viewports == 1 );
	assert( RHI_PrepareDraw( &raster, &texture ) );
	assert( scissors == 1 && viewports == 1 );
	RHI_InvalidateViewport();
	assert( RHI_PrepareDraw( &raster, &texture ) );
	assert( scissors == 1 && viewports == 2 );
	RHI_BindIndexData( 0, nullptr );
	assert( vk.cmd->num_indexes == 0 );

	// A failed wait leaves live sampler objects and filter policy intact.
	waits = 0;
	vk.samplers.filter_min = FILTER_NEAREST;
	vk.samplers.filter_max = FILTER_NEAREST;
	vk.samplers.count = 1;
	vk.samplers.handle[0] = (VkSampler)(uintptr_t)32;
	qvkDestroySampler = destroy_sampler;
	bool changed;
	assert( RHI_SetTextureFilter( rhiFilter_t::Nearest, rhiFilter_t::Nearest, &changed ) == rhiStatus_t::Success );
	assert( !changed && waits == 0 );
	wait_result = VK_ERROR_DEVICE_LOST;
	assert( RHI_SetTextureFilter( rhiFilter_t::LinearMipmapLinear, rhiFilter_t::Linear, &changed ) == rhiStatus_t::DeviceLost );
	assert( !changed && sampler_destroys == 0 && vk.samplers.count == 1 && vk.samplers.filter_min == FILTER_NEAREST );
	wait_result = VK_SUCCESS;
	assert( RHI_SetTextureFilter( rhiFilter_t::LinearMipmapLinear, rhiFilter_t::Linear, &changed ) == rhiStatus_t::Success );
	assert( changed && sampler_destroys == 1 && vk.samplers.count == 0 && vk.samplers.filter_min == FILTER_LINEAR_MIPMAP_LINEAR );
	assert( vk_texture_filter( rhiFilter_t::NearestMipmapNearest ) == FILTER_NEAREST_MIPMAP_NEAREST );
	assert( vk_texture_filter( rhiFilter_t::NearestMipmapLinear ) == FILTER_NEAREST_MIPMAP_LINEAR );
	assert( vk_texture_filter( rhiFilter_t::LinearMipmapNearest ) == FILTER_LINEAR_MIPMAP_NEAREST );

	// Unknown format labels must not alias the formatter's scratch buffer.
	vk.present_format.format = (VkFormat)1000;
	vk.color_format = (VkFormat)1001;
	vk.capture_format = (VkFormat)1002;
	vk.depth_format = (VkFormat)1003;
	const rhiDeviceDescription_t info = RHI_GetDeviceDescription();
	assert( strcmp( info.presentFormat, "#1000" ) == 0 && strcmp( info.colorFormat, "#1001" ) == 0 );
	assert( strcmp( info.captureFormat, "#1002" ) == 0 && strcmp( info.depthFormat, "#1003" ) == 0 );
	// Native errors return from the backend before any engine error callback.
	qvkAllocateDescriptorSets = reject_descriptors;
	assert( RHI_InitDescriptors() == rhiStatus_t::OutOfMemory );
	assert( strstr( RHI_GetError()->message, "VK_ERROR_OUT_OF_DEVICE_MEMORY" ) && !RHI_GetError()->drop );
	assert( vk_error_environment == nullptr );
	vk.pipelines_count = MAX_VK_PIPELINES;
	rhiPipelineDesc_t definition = {};
	uint32_t pipeline;
	assert( RHI_FindPipeline( MAX_VK_PIPELINES, &definition, false, &pipeline ) == rhiStatus_t::Error );
	assert( RHI_GetError()->drop && strstr( RHI_GetError()->message, "MAX_VK_PIPELINES" ) );
	assert( vk_error_environment == nullptr );
	assert( RHI_WaitIdle() == rhiStatus_t::Success );
	assert( RHI_GetError()->message[0] == '\0' && !RHI_GetError()->drop );
	vk.pipelineCache = (VkPipelineCache)(uintptr_t)90;
	qvkGetPipelineCacheData = cache_data;
	uint32_t cacheSize = 0;
	assert( RHI_ReadPipelineCache( nullptr, &cacheSize ) == rhiStatus_t::Success && cacheSize == 4 );
	byte cacheBytes[4];
	assert( RHI_ReadPipelineCache( cacheBytes, &cacheSize ) == rhiStatus_t::Success && memcmp( cacheBytes, "data", 4 ) == 0 );
	vk.pipelineCacheKey.vendor = 12;
	vk.pipelineCacheKey.device = 34;
	vk.pipelineCacheKey.driver = 56;
	vk.pipelineCacheKey.uuid[0] = 78;
	const rhiPipelineCacheKey_t cacheKey = RHI_GetPipelineCacheKey();
	assert( cacheKey.vendor == 12 && cacheKey.device == 34 && cacheKey.driver == 56 && cacheKey.uuid[0] == 78 );
	VkPipelineCacheHeaderVersionOne nativeHeader = {};
	nativeHeader.headerSize = sizeof( nativeHeader );
	nativeHeader.headerVersion = VK_PIPELINE_CACHE_HEADER_VERSION_ONE;
	nativeHeader.vendorID = 12;
	nativeHeader.deviceID = 34;
	nativeHeader.pipelineCacheUUID[0] = 78;
	qvkCreatePipelineCache = restore_cache;
	qvkDestroyPipelineCache = destroy_cache;
	assert( RHI_RestorePipelineCache( &nativeHeader, sizeof( nativeHeader ) ) == rhiStatus_t::Success );
	assert( (uintptr_t)vk.pipelineCache == 91 );
	rhiDeviceConfig_t config = {};
	config.renderWidth = 640;
	config.renderHeight = 480;
	config.uniformBytes = 128;
	rhiHost_t host = {};
	host.GetInstanceProcAddr = missing_loader_entry;
	rhiDeviceInfo_t deviceInfo;
	memset( &deviceInfo, 0xff, sizeof( deviceInfo ) );
	assert( RHI_Initialize( &config, &host, &deviceInfo ) == rhiStatus_t::Error );
	assert( strstr( RHI_GetError()->message, "vkCreateInstance" ) && !RHI_GetError()->drop );
	assert( !RHI_Available() && vk_device_info == nullptr && vk_error_environment == nullptr );
	assert( vk_config.renderWidth == 640 && vk_config.renderHeight == 480 && vk_config.uniformBytes == 128 );
	assert( deviceInfo.renderer[0] == 0 && deviceInfo.maxTextureSize == 0 );
	puts( "PASS: RHI uploads, textures, wait statuses, commands and bounded asynchronous timestamp readback" );
	return 0;
}

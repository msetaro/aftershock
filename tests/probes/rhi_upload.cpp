// Exercise the production upload allocator without creating a GPU device.
#include "../../engine/renderervk/vk.cpp"
#include <assert.h>

Vk_Instance vk;
Vk_World vk_world;

void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}

static int destroyed;
static VkResult wait_result;
static int commands;
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

int main( void ) {
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
	puts( "PASS: RHI uploads, textures, wait statuses, commands and bounded asynchronous timestamp readback" );
	return 0;
}

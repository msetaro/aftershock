#include "vk.h"
#include "../qcommon/qcommon_public.h"
#include <setjmp.h>

constexpr int FILTER_NEAREST = (int)rhiFilter_t::Nearest;
constexpr int FILTER_LINEAR = (int)rhiFilter_t::Linear;
constexpr int FILTER_NEAREST_MIPMAP_NEAREST = (int)rhiFilter_t::NearestMipmapNearest;
constexpr int FILTER_LINEAR_MIPMAP_NEAREST = (int)rhiFilter_t::LinearMipmapNearest;
constexpr int FILTER_NEAREST_MIPMAP_LINEAR = (int)rhiFilter_t::NearestMipmapLinear;
constexpr int FILTER_LINEAR_MIPMAP_LINEAR = (int)rhiFilter_t::LinearMipmapLinear;

static Vk_Instance vk;
static Vk_World vk_world;
static rhiDeviceConfig_t vk_config;
static rhiHost_t vk_host;
static rhiPostProcess_t vk_post;
static rhiDeviceInfo_t *vk_device_info;

#if defined( _DEBUG )
#if defined( _WIN32 )
#define USE_VK_VALIDATION
#include "../platform/debug_public.h"
#endif
#endif


// Only trivial automatic objects may be crossed by this backend-local abort.
// The frontend invokes the engine error path after the RHI call has returned.
static jmp_buf *vk_error_environment;
static char vk_error_message[MAXPRINTMSG];
static rhiError_t vk_error_info = { false, vk_error_message };
static rhiStatus_t vk_error_status;

static void vk_clear_error( void ) {
	vk_error_message[0] = '\0';
	vk_error_info.drop = false;
	vk_error_status = rhiStatus_t::Success;
}

const rhiError_t *RHI_GetError( void ) {
	return &vk_error_info;
}

static NORETURN void QDECL vk_fail( errorParm_t severity, rhiStatus_t status, const char *format, ... ) {
	va_list args;
	va_start( args, format );
	Q_vsnprintf( vk_error_message, sizeof( vk_error_message ), format, args );
	va_end( args );
	vk_error_message[sizeof( vk_error_message ) - 1] = '\0';
	vk_error_info.drop = severity == ERR_DROP;
	vk_error_status = status;
	Q_ASSERT( vk_error_environment != nullptr );
	longjmp( *vk_error_environment, 1 );
}

#if defined( _MSC_VER )
#pragma warning( push )
#pragma warning( disable : 4611 ) // Trivial lifetimes are enforced below and in CI.
#endif
template <typename Call>
static rhiStatus_t vk_call( Call call ) {
	static_assert( std::is_trivially_destructible_v<Call> );
	jmp_buf environment;
	jmp_buf *const previous = vk_error_environment;
	vk_error_environment = &environment;
	vk_clear_error();
	if ( setjmp( environment ) ) {
		vk_error_environment = previous;
		return vk_error_status;
	}
	const rhiStatus_t status = call();
	vk_error_environment = previous;
	return status;
}
#if defined( _MSC_VER )
#pragma warning( pop )
#endif

static rhiStatus_t vk_status( VkResult result );

static rhiStatus_t vk_impl_Initialize( void );
static void vk_impl_InitDescriptors( void );
static void vk_impl_ReleaseResources( void );
static void vk_impl_Shutdown( void );
static void vk_impl_UploadWorldGeometry( const uint8_t *data, int32_t size );
static void vk_impl_UpdatePostProcess( int32_t overbrightBits );
static void vk_impl_ReadPixels( uint8_t *buffer, uint32_t width, uint32_t height );
static void vk_impl_CreateTexture( rhiTexture_t *texture, int32_t width, int32_t height, int32_t mipLevels, rhiFormat_t format, rhiAddress_t address, const char *label, bool owned = false, bool deferSampler = false );
static void vk_impl_UploadTexture( const rhiTexture_t *texture, int32_t x, int32_t y, int32_t width, int32_t height, int32_t mipLevels, const uint8_t *pixels, int32_t bytesPerPixel, bool update, int32_t blockExtent = 1 );
static void vk_impl_UpdateTextureSampler( const rhiTexture_t *texture, rhiAddress_t address, bool mipmap );
static rhiStatus_t vk_impl_SetTextureFilter( rhiFilter_t minimize, rhiFilter_t magnify, bool *changed );
static uint32_t vk_impl_FindPipeline( uint32_t base, const rhiPipelineDesc_t *desc, bool eager );
static void vk_impl_BindPipeline( uint32_t pipeline );
static bool vk_impl_BeginFrame( bool screenMap );
static rhiFrameEnd_t vk_impl_EndFrame( bool bloom, bool capture );
static rhiStatus_t vk_impl_PresentFrame( void );

static int vkSamples = VK_SAMPLE_COUNT_1_BIT;
static int vkMaxSamples = VK_SAMPLE_COUNT_1_BIT;

static VkInstance vk_instance = VK_NULL_HANDLE;
static VkSurfaceKHR vk_surface = VK_NULL_HANDLE;

#ifndef NDEBUG
VkDebugReportCallbackEXT vk_debug_callback = VK_NULL_HANDLE;
#endif

//
// Vulkan API functions used by the renderer.
//
static PFN_vkCreateInstance qvkCreateInstance;
static PFN_vkEnumerateInstanceExtensionProperties qvkEnumerateInstanceExtensionProperties;

static PFN_vkCreateDevice qvkCreateDevice;
static PFN_vkDestroyInstance qvkDestroyInstance;
static PFN_vkEnumerateDeviceExtensionProperties qvkEnumerateDeviceExtensionProperties;
static PFN_vkEnumeratePhysicalDevices qvkEnumeratePhysicalDevices;
static PFN_vkGetDeviceProcAddr qvkGetDeviceProcAddr;
static PFN_vkGetPhysicalDeviceFeatures qvkGetPhysicalDeviceFeatures;
static PFN_vkGetPhysicalDeviceFormatProperties qvkGetPhysicalDeviceFormatProperties;
static PFN_vkGetPhysicalDeviceMemoryProperties qvkGetPhysicalDeviceMemoryProperties;
static PFN_vkGetPhysicalDeviceProperties qvkGetPhysicalDeviceProperties;
static PFN_vkGetPhysicalDeviceQueueFamilyProperties qvkGetPhysicalDeviceQueueFamilyProperties;
static PFN_vkDestroySurfaceKHR qvkDestroySurfaceKHR;
static PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR qvkGetPhysicalDeviceSurfaceCapabilitiesKHR;
static PFN_vkGetPhysicalDeviceSurfaceFormatsKHR qvkGetPhysicalDeviceSurfaceFormatsKHR;
static PFN_vkGetPhysicalDeviceSurfacePresentModesKHR qvkGetPhysicalDeviceSurfacePresentModesKHR;
static PFN_vkGetPhysicalDeviceSurfaceSupportKHR qvkGetPhysicalDeviceSurfaceSupportKHR;
#ifdef USE_VK_VALIDATION
static PFN_vkCreateDebugReportCallbackEXT qvkCreateDebugReportCallbackEXT;
static PFN_vkDestroyDebugReportCallbackEXT qvkDestroyDebugReportCallbackEXT;
#endif
static PFN_vkAllocateCommandBuffers qvkAllocateCommandBuffers;
static PFN_vkAllocateDescriptorSets qvkAllocateDescriptorSets;
static PFN_vkAllocateMemory qvkAllocateMemory;
static PFN_vkBeginCommandBuffer qvkBeginCommandBuffer;
static PFN_vkBindBufferMemory qvkBindBufferMemory;
static PFN_vkBindImageMemory qvkBindImageMemory;
static PFN_vkCmdBeginRenderPass qvkCmdBeginRenderPass;
static PFN_vkCmdBindDescriptorSets qvkCmdBindDescriptorSets;
static PFN_vkCmdBindIndexBuffer qvkCmdBindIndexBuffer;
static PFN_vkCmdBindPipeline qvkCmdBindPipeline;
static PFN_vkCmdBindVertexBuffers qvkCmdBindVertexBuffers;
static PFN_vkCmdBlitImage qvkCmdBlitImage;
static PFN_vkCmdClearAttachments qvkCmdClearAttachments;
static PFN_vkCmdCopyBuffer qvkCmdCopyBuffer;
static PFN_vkCmdCopyBufferToImage qvkCmdCopyBufferToImage;
static PFN_vkCmdCopyImage qvkCmdCopyImage;
static PFN_vkCmdDraw qvkCmdDraw;
static PFN_vkCmdDrawIndexed qvkCmdDrawIndexed;
static PFN_vkCmdEndRenderPass qvkCmdEndRenderPass;
static PFN_vkCmdNextSubpass qvkCmdNextSubpass;
static PFN_vkCmdPipelineBarrier qvkCmdPipelineBarrier;
static PFN_vkCmdPushConstants qvkCmdPushConstants;
static PFN_vkCmdSetDepthBias qvkCmdSetDepthBias;
static PFN_vkCmdSetScissor qvkCmdSetScissor;
static PFN_vkCmdSetViewport qvkCmdSetViewport;
static PFN_vkCreateQueryPool qvkCreateQueryPool;
static PFN_vkDestroyQueryPool qvkDestroyQueryPool;
static PFN_vkCmdResetQueryPool qvkCmdResetQueryPool;
static PFN_vkCmdWriteTimestamp qvkCmdWriteTimestamp;
static PFN_vkGetQueryPoolResults qvkGetQueryPoolResults;
static PFN_vkCreateBuffer qvkCreateBuffer;
static PFN_vkCreateCommandPool qvkCreateCommandPool;
static PFN_vkCreateDescriptorPool qvkCreateDescriptorPool;
static PFN_vkCreateDescriptorSetLayout qvkCreateDescriptorSetLayout;
static PFN_vkCreateFence qvkCreateFence;
static PFN_vkCreateFramebuffer qvkCreateFramebuffer;
static PFN_vkCreateGraphicsPipelines qvkCreateGraphicsPipelines;
static PFN_vkCreateImage qvkCreateImage;
static PFN_vkCreateImageView qvkCreateImageView;
static PFN_vkCreatePipelineLayout qvkCreatePipelineLayout;
static PFN_vkCreatePipelineCache qvkCreatePipelineCache;
static PFN_vkGetPipelineCacheData qvkGetPipelineCacheData;
static PFN_vkCreateRenderPass qvkCreateRenderPass;
static PFN_vkCreateSampler qvkCreateSampler;
static PFN_vkCreateSemaphore qvkCreateSemaphore;
static PFN_vkCreateShaderModule qvkCreateShaderModule;
static PFN_vkDestroyBuffer qvkDestroyBuffer;
static PFN_vkDestroyCommandPool qvkDestroyCommandPool;
static PFN_vkDestroyDescriptorPool qvkDestroyDescriptorPool;
static PFN_vkDestroyDescriptorSetLayout qvkDestroyDescriptorSetLayout;
static PFN_vkDestroyDevice qvkDestroyDevice;
static PFN_vkDestroyFence qvkDestroyFence;
static PFN_vkDestroyFramebuffer qvkDestroyFramebuffer;
static PFN_vkDestroyImage qvkDestroyImage;
static PFN_vkDestroyImageView qvkDestroyImageView;
static PFN_vkDestroyPipeline qvkDestroyPipeline;
static PFN_vkDestroyPipelineCache qvkDestroyPipelineCache;
static PFN_vkDestroyPipelineLayout qvkDestroyPipelineLayout;
static PFN_vkDestroyRenderPass qvkDestroyRenderPass;
static PFN_vkDestroySampler qvkDestroySampler;
static PFN_vkDestroySemaphore qvkDestroySemaphore;
static PFN_vkDestroyShaderModule qvkDestroyShaderModule;
static PFN_vkDeviceWaitIdle qvkDeviceWaitIdle;
static PFN_vkEndCommandBuffer qvkEndCommandBuffer;
static PFN_vkFlushMappedMemoryRanges qvkFlushMappedMemoryRanges;
static PFN_vkFreeCommandBuffers qvkFreeCommandBuffers;
static PFN_vkFreeDescriptorSets qvkFreeDescriptorSets;
static PFN_vkFreeMemory qvkFreeMemory;
static PFN_vkGetBufferMemoryRequirements qvkGetBufferMemoryRequirements;
static PFN_vkGetDeviceQueue qvkGetDeviceQueue;
static PFN_vkGetImageMemoryRequirements qvkGetImageMemoryRequirements;
static PFN_vkGetImageSubresourceLayout qvkGetImageSubresourceLayout;
static PFN_vkInvalidateMappedMemoryRanges qvkInvalidateMappedMemoryRanges;
static PFN_vkMapMemory qvkMapMemory;
static PFN_vkQueueSubmit qvkQueueSubmit;
static PFN_vkQueueWaitIdle qvkQueueWaitIdle;
static PFN_vkResetCommandBuffer qvkResetCommandBuffer;
static PFN_vkResetDescriptorPool qvkResetDescriptorPool;
static PFN_vkResetFences qvkResetFences;
static PFN_vkUnmapMemory qvkUnmapMemory;
static PFN_vkUpdateDescriptorSets qvkUpdateDescriptorSets;
static PFN_vkWaitForFences qvkWaitForFences;
static PFN_vkAcquireNextImageKHR qvkAcquireNextImageKHR;
static PFN_vkCreateSwapchainKHR qvkCreateSwapchainKHR;
static PFN_vkDestroySwapchainKHR qvkDestroySwapchainKHR;
static PFN_vkGetSwapchainImagesKHR qvkGetSwapchainImagesKHR;
static PFN_vkQueuePresentKHR qvkQueuePresentKHR;

static PFN_vkGetBufferMemoryRequirements2KHR qvkGetBufferMemoryRequirements2KHR;
static PFN_vkGetImageMemoryRequirements2KHR qvkGetImageMemoryRequirements2KHR;

static PFN_vkDebugMarkerSetObjectNameEXT qvkDebugMarkerSetObjectNameEXT;

////////////////////////////////////////////////////////////////////////////

// forward declaration
VkPipeline create_pipeline( const rhiPipelineDesc_t *def, renderPass_t renderPassIndex, uint32_t def_index );

static uint32_t find_memory_type( uint32_t memory_type_bits, VkMemoryPropertyFlags properties ) {
	VkPhysicalDeviceMemoryProperties memory_properties;
	uint32_t i;

	qvkGetPhysicalDeviceMemoryProperties( vk.physical_device, &memory_properties );

	for ( i = 0; i < memory_properties.memoryTypeCount; i++ ) {
		if ( ( memory_type_bits & ( 1 << i ) ) != 0 &&
			 ( memory_properties.memoryTypes[i].propertyFlags & properties ) == properties ) {
			return i;
		}
	}
	vk_fail( ERR_FATAL, rhiStatus_t::Error, "Vulkan: failed to find matching memory type with requested properties" );
}


static uint32_t find_memory_type2( uint32_t memory_type_bits, VkMemoryPropertyFlags properties, VkMemoryPropertyFlags *outprops ) {
	VkPhysicalDeviceMemoryProperties memory_properties;
	uint32_t i;

	qvkGetPhysicalDeviceMemoryProperties( vk.physical_device, &memory_properties );

	for ( i = 0; i < memory_properties.memoryTypeCount; i++ ) {
		if ( ( memory_type_bits & ( 1 << i ) ) != 0 && ( memory_properties.memoryTypes[i].propertyFlags & properties ) == properties ) {
			if ( outprops ) {
				*outprops = memory_properties.memoryTypes[i].propertyFlags;
			}
			return i;
		}
	}

	return ~0U;
}


static const char *pmode_to_str( VkPresentModeKHR mode ) {
	static char buf[32];

	switch ( mode ) {
	case VK_PRESENT_MODE_IMMEDIATE_KHR:
		return "IMMEDIATE";
	case VK_PRESENT_MODE_MAILBOX_KHR:
		return "MAILBOX";
	case VK_PRESENT_MODE_FIFO_KHR:
		return "FIFO";
	case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
		return "FIFO_RELAXED";
	case VK_PRESENT_MODE_FIFO_LATEST_READY_EXT:
		return "FIFO_LATEST_READY";
	default:
		snprintf( buf, sizeof( buf ), "mode#%x", mode );
		return buf;
	};
}


#define CASE_STR( x ) case (x): return #x

const char *vk_format_string( VkFormat format ) {
	static char buf[16];

	switch ( format ) {
		// color formats
		CASE_STR( VK_FORMAT_R5G5B5A1_UNORM_PACK16 );
		CASE_STR( VK_FORMAT_B5G5R5A1_UNORM_PACK16 );
		CASE_STR( VK_FORMAT_R5G6B5_UNORM_PACK16 );
		CASE_STR( VK_FORMAT_B5G6R5_UNORM_PACK16 );
		CASE_STR( VK_FORMAT_B8G8R8A8_SRGB );
		CASE_STR( VK_FORMAT_R8G8B8A8_SRGB );
		CASE_STR( VK_FORMAT_B8G8R8A8_SNORM );
		CASE_STR( VK_FORMAT_R8G8B8A8_SNORM );
		CASE_STR( VK_FORMAT_B8G8R8A8_UNORM );
		CASE_STR( VK_FORMAT_R8G8B8A8_UNORM );
		CASE_STR( VK_FORMAT_B4G4R4A4_UNORM_PACK16 );
		CASE_STR( VK_FORMAT_R4G4B4A4_UNORM_PACK16 );
		CASE_STR( VK_FORMAT_R16G16B16A16_UNORM );
		CASE_STR( VK_FORMAT_A2B10G10R10_UNORM_PACK32 );
		CASE_STR( VK_FORMAT_A2R10G10B10_UNORM_PACK32 );
		CASE_STR( VK_FORMAT_B10G11R11_UFLOAT_PACK32 );
		// depth formats
		CASE_STR( VK_FORMAT_D16_UNORM );
		CASE_STR( VK_FORMAT_D16_UNORM_S8_UINT );
		CASE_STR( VK_FORMAT_X8_D24_UNORM_PACK32 );
		CASE_STR( VK_FORMAT_D24_UNORM_S8_UINT );
		CASE_STR( VK_FORMAT_D32_SFLOAT );
		CASE_STR( VK_FORMAT_D32_SFLOAT_S8_UINT );
	default:
		Com_sprintf( buf, sizeof( buf ), "#%i", format );
		return buf;
	}
}


static const char *vk_result_string( VkResult code ) {
	static char buffer[32];

	switch ( code ) {
		CASE_STR( VK_SUCCESS );
		CASE_STR( VK_NOT_READY );
		CASE_STR( VK_TIMEOUT );
		CASE_STR( VK_EVENT_SET );
		CASE_STR( VK_EVENT_RESET );
		CASE_STR( VK_INCOMPLETE );
		CASE_STR( VK_ERROR_OUT_OF_HOST_MEMORY );
		CASE_STR( VK_ERROR_OUT_OF_DEVICE_MEMORY );
		CASE_STR( VK_ERROR_INITIALIZATION_FAILED );
		CASE_STR( VK_ERROR_DEVICE_LOST );
		CASE_STR( VK_ERROR_MEMORY_MAP_FAILED );
		CASE_STR( VK_ERROR_LAYER_NOT_PRESENT );
		CASE_STR( VK_ERROR_EXTENSION_NOT_PRESENT );
		CASE_STR( VK_ERROR_FEATURE_NOT_PRESENT );
		CASE_STR( VK_ERROR_INCOMPATIBLE_DRIVER );
		CASE_STR( VK_ERROR_TOO_MANY_OBJECTS );
		CASE_STR( VK_ERROR_FORMAT_NOT_SUPPORTED );
		CASE_STR( VK_ERROR_FRAGMENTED_POOL );
		CASE_STR( VK_ERROR_UNKNOWN );
		CASE_STR( VK_ERROR_OUT_OF_POOL_MEMORY );
		CASE_STR( VK_ERROR_INVALID_EXTERNAL_HANDLE );
		CASE_STR( VK_ERROR_FRAGMENTATION );
		CASE_STR( VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS );
		CASE_STR( VK_ERROR_SURFACE_LOST_KHR );
		CASE_STR( VK_ERROR_NATIVE_WINDOW_IN_USE_KHR );
		CASE_STR( VK_SUBOPTIMAL_KHR );
		CASE_STR( VK_ERROR_OUT_OF_DATE_KHR );
		CASE_STR( VK_ERROR_INCOMPATIBLE_DISPLAY_KHR );
		CASE_STR( VK_ERROR_VALIDATION_FAILED_EXT );
		CASE_STR( VK_ERROR_INVALID_SHADER_NV );
		CASE_STR( VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT );
		CASE_STR( VK_ERROR_NOT_PERMITTED_EXT );
		CASE_STR( VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT );
		CASE_STR( VK_THREAD_IDLE_KHR );
		CASE_STR( VK_THREAD_DONE_KHR );
		CASE_STR( VK_OPERATION_DEFERRED_KHR );
		CASE_STR( VK_OPERATION_NOT_DEFERRED_KHR );
		CASE_STR( VK_PIPELINE_COMPILE_REQUIRED_EXT );
	default:
		snprintf( buffer, sizeof( buffer ), "code %i", code );
		return buffer;
	}
}
#undef CASE_STR

#define VK_CHECK( function_call ) { \
	VkResult vkCheckResult = function_call; \
	if ( vkCheckResult < 0 ) { \
		vk_fail( ERR_FATAL, vk_status( vkCheckResult ), "Vulkan: %s returned %s", #function_call, vk_result_string( vkCheckResult ) ); \
	} \
}


/*
static VkFlags get_composite_alpha( VkCompositeAlphaFlagsKHR flags )
{
	const VkCompositeAlphaFlagBitsKHR compositeFlags[] = {
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
	};
	int i;

	for ( i = 1; i < ARRAY_LEN( compositeFlags ); i++ ) {
		if ( flags & compositeFlags[i] ) {
			return compositeFlags[i];
		}
	}

	return compositeFlags[0];
}
*/


static VkCommandBuffer begin_command_buffer( void ) {
	VkCommandBufferBeginInfo begin_info;
	VkCommandBufferAllocateInfo alloc_info;
	VkCommandBuffer command_buffer;

	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.pNext = NULL;
	alloc_info.commandPool = vk.command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = 1;
	VK_CHECK( qvkAllocateCommandBuffers( vk.device, &alloc_info, &command_buffer ) );

	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = NULL;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	begin_info.pInheritanceInfo = NULL;

	VK_CHECK( qvkBeginCommandBuffer( command_buffer, &begin_info ) );

	return command_buffer;
}


static void end_command_buffer( VkCommandBuffer command_buffer, const char *location [[maybe_unused]] ) {
#ifdef USE_UPLOAD_QUEUE
	const VkPipelineStageFlags wait_dst_stage_mask = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore waits;
#endif
	VkSubmitInfo submit_info;
	VkCommandBuffer cmdbuf[1];

	cmdbuf[0] = command_buffer;

	VK_CHECK( qvkEndCommandBuffer( command_buffer ) );

	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
#ifdef USE_UPLOAD_QUEUE
	if ( vk.rendering_finished != VK_NULL_HANDLE ) {
		waits = vk.rendering_finished;
		vk.rendering_finished = VK_NULL_HANDLE;
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = &waits;
		submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
	} else
#endif
	{
		submit_info.waitSemaphoreCount = 0;
		submit_info.pWaitSemaphores = NULL;
		submit_info.pWaitDstStageMask = NULL;
	}

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = cmdbuf;
	submit_info.signalSemaphoreCount = 0;
	submit_info.pSignalSemaphores = NULL;

	VK_CHECK( qvkQueueSubmit( vk.queue, 1, &submit_info, VK_NULL_HANDLE ) );

	vk_queue_wait_idle();

	qvkFreeCommandBuffers( vk.device, vk.command_pool, 1, cmdbuf );
}


static void record_image_layout_transition( VkCommandBuffer command_buffer, VkImage image, VkImageAspectFlags image_aspect_flags,
	VkImageLayout old_layout, VkImageLayout new_layout, uint32_t src_stage_override, uint32_t dst_stage_override [[maybe_unused]] ) {
	VkImageMemoryBarrier barrier;
	uint32_t src_stage, dst_stage;

	switch ( old_layout ) {
	case VK_IMAGE_LAYOUT_UNDEFINED:
		if ( src_stage_override != 0 )
			src_stage = src_stage_override;
		else
			src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		barrier.srcAccessMask = VK_ACCESS_NONE;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		src_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
		src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		barrier.srcAccessMask = VK_ACCESS_NONE;
		break;
	default:
		vk_fail( ERR_DROP, rhiStatus_t::Error, "unsupported old layout %i", old_layout );
	}

	switch ( new_layout ) {
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
		dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		barrier.dstAccessMask = VK_ACCESS_NONE;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		break;
	default:
		vk_fail( ERR_DROP, rhiStatus_t::Error, "unsupported new layout %i", new_layout );
	}


	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = NULL;
	//barrier.srcAccessMask = src_access_flags;
	//barrier.dstAccessMask = dst_access_flags;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = image_aspect_flags;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

	qvkCmdPipelineBarrier( command_buffer, src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &barrier );
}


// debug markers
#define SET_OBJECT_NAME( obj, objName, objType ) vk_set_object_name( (uint64_t)(obj), (objName), (objType) )

static void vk_set_object_name( uint64_t obj, const char *objName, VkDebugReportObjectTypeEXT objType ) {
	if ( qvkDebugMarkerSetObjectNameEXT && obj ) {
		VkDebugMarkerObjectNameInfoEXT info;
		info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
		info.pNext = NULL;
		info.objectType = objType;
		info.object = obj;
		info.pObjectName = objName;
		qvkDebugMarkerSetObjectNameEXT( vk.device, &info );
	}
}


static void vk_create_swapchain( VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface, VkSurfaceFormatKHR surface_format, VkSwapchainKHR *swapchain, qboolean verbose ) {
	VkImageViewCreateInfo view;
	VkSurfaceCapabilitiesKHR surface_caps;
	VkExtent2D image_extent;
	uint32_t present_mode_count, i;
	VkPresentModeKHR present_mode;
	VkPresentModeKHR *present_modes;
	uint32_t image_count;
	VkSwapchainCreateInfoKHR desc;
	qboolean mailbox_supported = qfalse;
	qboolean immediate_supported = qfalse;
	qboolean fifo_relaxed_supported = qfalse;
	int v;

	VK_CHECK( qvkGetPhysicalDeviceSurfaceCapabilitiesKHR( physical_device, surface, &surface_caps ) );

	image_extent = surface_caps.currentExtent;
	if ( image_extent.width == 0xffffffff && image_extent.height == 0xffffffff ) {
		image_extent.width = MIN( surface_caps.maxImageExtent.width, MAX( surface_caps.minImageExtent.width, (uint32_t)vk_config.renderWidth ) );
		image_extent.height = MIN( surface_caps.maxImageExtent.height, MAX( surface_caps.minImageExtent.height, (uint32_t)vk_config.renderHeight ) );
	}

	vk.clearAttachment = qtrue;

	if ( !vk.fboActive ) {
		// VK_IMAGE_USAGE_TRANSFER_DST_BIT is required by image clear operations.
		if ( ( surface_caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT ) == 0 ) {
			vk.clearAttachment = qfalse;
			vk_host.Print( rhiLog_t::Warning, "VK_IMAGE_USAGE_TRANSFER_DST_BIT is not supported by the swapchain, \\r_clear might not work\n" );
		}
		// VK_IMAGE_USAGE_TRANSFER_SRC_BIT is required in order to take screenshots.
		if ( ( surface_caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT ) == 0 ) {
			vk_fail( ERR_FATAL, rhiStatus_t::Error, "create_swapchain: VK_IMAGE_USAGE_TRANSFER_SRC_BIT is not supported by the swapchain" );
		}
	}

	// determine present mode and swapchain image count
	VK_CHECK(qvkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, NULL));

	present_modes = (VkPresentModeKHR *)vk_host.Allocate( present_mode_count * sizeof( VkPresentModeKHR ) );
	VK_CHECK(qvkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes));

	if ( verbose ) {
		vk_host.Print( rhiLog_t::Info, "...presentation modes:" );
	}
	for ( i = 0; i < present_mode_count; i++ ) {
		if ( verbose ) {
			vk_host.Print( rhiLog_t::Info, " %s", pmode_to_str( present_modes[i] ) );
		}
		if ( present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR )
			mailbox_supported = qtrue;
		else if ( present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR )
			immediate_supported = qtrue;
		else if ( present_modes[i] == VK_PRESENT_MODE_FIFO_RELAXED_KHR )
			fifo_relaxed_supported = qtrue;
	}
	if ( verbose ) {
		vk_host.Print( rhiLog_t::Info, "\n" );
	}

	vk_host.Free( present_modes );

	if ( ( v = vk_host.SwapInterval() ) != 0 ) {
		if ( v == 2 && mailbox_supported )
			present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
		else if ( fifo_relaxed_supported )
			present_mode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
		else
			present_mode = VK_PRESENT_MODE_FIFO_KHR;
		image_count = MAX( MIN_SWAPCHAIN_IMAGES_FIFO, surface_caps.minImageCount );
	} else {
		if ( immediate_supported ) {
			present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			image_count = MAX( MIN_SWAPCHAIN_IMAGES_IMM, surface_caps.minImageCount );
		} else if ( mailbox_supported ) {
			present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
			image_count = MAX( MIN_SWAPCHAIN_IMAGES_MAILBOX, surface_caps.minImageCount );
		} else if ( fifo_relaxed_supported ) {
			present_mode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
			image_count = MAX( MIN_SWAPCHAIN_IMAGES_FIFO, surface_caps.minImageCount );
		} else {
			present_mode = VK_PRESENT_MODE_FIFO_KHR;
			image_count = MAX( MIN_SWAPCHAIN_IMAGES_FIFO, surface_caps.minImageCount );
		}
	}

	if ( image_count < 2 ) {
		image_count = 2;
	}

	if ( surface_caps.maxImageCount == 0 && present_mode == VK_PRESENT_MODE_FIFO_KHR ) {
		image_count = MAX( MIN_SWAPCHAIN_IMAGES_FIFO_0, surface_caps.minImageCount );
	} else if ( surface_caps.maxImageCount > 0 ) {
		image_count = MIN( MIN( image_count, surface_caps.maxImageCount ), MAX_SWAPCHAIN_IMAGES );
	}

	if ( verbose ) {
		vk_host.Print( rhiLog_t::Info, "...selected presentation mode: %s, image count: %i\n", pmode_to_str( present_mode ), image_count );
	}

	// create swap chain
	desc.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	desc.pNext = NULL;
	desc.flags = 0;
	desc.surface = surface;
	desc.minImageCount = image_count;
	desc.imageFormat = surface_format.format;
	desc.imageColorSpace = surface_format.colorSpace;
	desc.imageExtent = image_extent;
	desc.imageArrayLayers = 1;
	desc.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if ( !vk.fboActive ) {
		desc.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}
	desc.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	desc.queueFamilyIndexCount = 0;
	desc.pQueueFamilyIndices = NULL;
	desc.preTransform = surface_caps.currentTransform;
	//desc.compositeAlpha = get_composite_alpha( surface_caps.supportedCompositeAlpha );
	desc.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	desc.presentMode = present_mode;
	desc.clipped = VK_TRUE;
	desc.oldSwapchain = VK_NULL_HANDLE;

	VK_CHECK( qvkCreateSwapchainKHR( device, &desc, NULL, swapchain ) );

	VK_CHECK( qvkGetSwapchainImagesKHR( vk.device, vk.swapchain, &vk.swapchain_image_count, NULL ) );
	vk.swapchain_image_count = MIN( vk.swapchain_image_count, MAX_SWAPCHAIN_IMAGES );
	VK_CHECK( qvkGetSwapchainImagesKHR( vk.device, vk.swapchain, &vk.swapchain_image_count, vk.swapchain_images ) );

	for ( i = 0; i < vk.swapchain_image_count; i++ ) {
		view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view.pNext = NULL;
		view.flags = 0;
		view.image = vk.swapchain_images[i];
		view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view.format = vk.present_format.format;
		view.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		view.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		view.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		view.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view.subresourceRange.baseMipLevel = 0;
		view.subresourceRange.levelCount = 1;
		view.subresourceRange.baseArrayLayer = 0;
		view.subresourceRange.layerCount = 1;

		VK_CHECK( qvkCreateImageView( vk.device, &view, NULL, &vk.swapchain_image_views[i] ) );

		SET_OBJECT_NAME( vk.swapchain_images[i], va( "swapchain image %i", i ), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT );
		SET_OBJECT_NAME( vk.swapchain_image_views[i], va( "swapchain image %i", i ), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT );
	}

	for ( i = 0; i < vk.swapchain_image_count; i++ ) {
		VkSemaphoreCreateInfo s;
		s.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		s.pNext = NULL;
		s.flags = 0;
		VK_CHECK( qvkCreateSemaphore( vk.device, &s, NULL, &vk.swapchain_rendering_finished[i] ) );
		SET_OBJECT_NAME( vk.swapchain_rendering_finished[i], va( "swapchain_rendering_finished semaphore %i", i ), VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT );
	}

#if 0
	if (vk.initSwapchainLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
		VkCommandBuffer command_buffer = begin_command_buffer();

		for (i = 0; i < vk.swapchain_image_count; i++) {
			record_image_layout_transition(command_buffer, vk.swapchain_images[i],
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, vk.initSwapchainLayout, 0, 0);
		}

		end_command_buffer(command_buffer, __func__);
	}
#endif

	for ( i = 0; i < vk.swapchain_image_count; i++ ) {
		if ( vk.initSwapchainLayout != VK_IMAGE_LAYOUT_UNDEFINED ) {
			// The Vulkan spec states : Use of a presentable image must occur only after the image is returned by vkAcquireNextImageKHR,
			// and before it is released by vkQueuePresentKHR.
			// This includes transitioning the image layout and rendering commands(https ://docs.vulkan.org/refpages/latest/refpages/source/VkSwapchainKHR.html#_description)
			vk.swapchain_images_inited[i] = qfalse;
		} else {
			vk.swapchain_images_inited[i] = qtrue; // assume undefined layout
		}
	}
}


static rhiGraph_t vk_graph;
static_assert( RHI_GRAPH_BLOOM_PASSES == VK_NUM_BLOOM_PASSES );
static_assert( ARRAY_LEN( vk_graph.targetOrder ) == MAX_ATTACHMENTS_IN_POOL );
static const char *const vk_graph_names[] = {
	"screenmap", "main", "bloom_extract", "blur 0", "blur 1", "blur 2", "blur 3",
	"blur 4", "blur 5", "blur 6", "blur 7", "post_bloom", "capture", "gamma"
};
static_assert( ARRAY_LEN( vk_graph_names ) == (uint32_t)rhiGraphPass_t::Count );

static VkFormat vk_graph_format( rhiGraphFormat_t format ) {
	switch ( format ) {
	case rhiGraphFormat_t::Color:
		return vk.color_format;
	case rhiGraphFormat_t::Depth:
		return vk.depth_format;
	case rhiGraphFormat_t::Bloom:
		return vk.bloom_format;
	case rhiGraphFormat_t::Capture:
		return vk.capture_format;
	case rhiGraphFormat_t::Present:
		return vk.present_format.format;
	}
	return VK_FORMAT_UNDEFINED;
}

static VkImageLayout vk_graph_layout( rhiGraphLayout_t layout ) {
	switch ( layout ) {
	case rhiGraphLayout_t::Undefined:
		return VK_IMAGE_LAYOUT_UNDEFINED;
	case rhiGraphLayout_t::Sampled:
		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	case rhiGraphLayout_t::Color:
		return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	case rhiGraphLayout_t::Depth:
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	case rhiGraphLayout_t::TransferSource:
		return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	case rhiGraphLayout_t::Present:
		return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}
	return VK_IMAGE_LAYOUT_UNDEFINED;
}

static VkRenderPass *vk_graph_pass( rhiGraphPass_t pass ) {
	using P = rhiGraphPass_t;
	switch ( pass ) {
	case P::Main:
		return &vk.render_pass.main;
	case P::PostBloom:
		return &vk.render_pass.post_bloom;
	case P::BloomExtract:
		return &vk.render_pass.bloom_extract;
	case P::Capture:
		return &vk.render_pass.capture;
	case P::Gamma:
		return &vk.render_pass.gamma;
	case P::ScreenMap:
		return &vk.render_pass.screenmap;
	default:
		return &vk.render_pass.blur[(uint32_t)pass - (uint32_t)P::Blur0];
	}
}

static VkAttachmentLoadOp vk_graph_load( rhiGraphLoad_t load ) {
	switch ( load ) {
	case rhiGraphLoad_t::Clear:
		return VK_ATTACHMENT_LOAD_OP_CLEAR;
	case rhiGraphLoad_t::Load:
		return VK_ATTACHMENT_LOAD_OP_LOAD;
	case rhiGraphLoad_t::Discard:
		return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	}
	return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
}

static VkAccessFlags vk_graph_access( uint32_t access ) {
	return ( access & RHI_GRAPH_COLOR_READ ? VK_ACCESS_COLOR_ATTACHMENT_READ_BIT : 0 ) |
		   ( access & RHI_GRAPH_COLOR_WRITE ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : 0 ) |
		   ( access & RHI_GRAPH_SHADER_READ ? VK_ACCESS_SHADER_READ_BIT : 0 );
}

static void vk_create_render_passes( void ) {
	for ( uint32_t p = 0; p < vk_graph.passCount; ++p ) {
		const rhiGraphPass_t id = vk_graph.passOrder[p];
		const rhiGraphPassDesc_t &pass = vk_graph.passes[(uint32_t)id];
		VkAttachmentDescription attachments[3] = {};
		for ( uint32_t i = 0; i < pass.attachmentCount; ++i ) {
			const rhiGraphAttachment_t &a = pass.attachments[i];
			const rhiGraphTargetDesc_t &target = vk_graph.targets[(uint32_t)a.target];
			attachments[i].format = vk_graph_format( target.format );
			attachments[i].samples = (VkSampleCountFlagBits)target.samples;
			attachments[i].loadOp = vk_graph_load( a.load );
			attachments[i].storeOp = a.store == rhiGraphStore_t::Store ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[i].stencilLoadOp = vk_graph_load( a.stencilLoad );
			attachments[i].stencilStoreOp = a.stencilStore == rhiGraphStore_t::Store ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[i].initialLayout = a.initialLayout == rhiGraphLayout_t::Present ? vk.initSwapchainLayout : vk_graph_layout( a.initialLayout );
			attachments[i].finalLayout = vk_graph_layout( a.finalLayout );
		}
		const VkAttachmentReference color = { pass.color, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		const VkAttachmentReference depth = { pass.depth, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
		const VkAttachmentReference resolve = { pass.resolve, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color;
		subpass.pDepthStencilAttachment = pass.depth == RHI_INVALID_OFFSET ? NULL : &depth;
		subpass.pResolveAttachments = pass.resolve == RHI_INVALID_OFFSET ? NULL : &resolve;
		VkSubpassDependency dependencies[2] = {};
		for ( uint32_t i = 0; i < pass.dependencyCount; ++i ) {
			const rhiGraphDependency_t &d = pass.dependencies[i];
			dependencies[i].srcSubpass = d.incoming ? VK_SUBPASS_EXTERNAL : 0;
			dependencies[i].dstSubpass = d.incoming ? 0 : VK_SUBPASS_EXTERNAL;
			dependencies[i].srcStageMask = d.sourceStage == rhiGraphStage_t::Fragment ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[i].dstStageMask = d.destinationStage == rhiGraphStage_t::Fragment ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[i].srcAccessMask = vk_graph_access( d.sourceAccess );
			dependencies[i].dstAccessMask = vk_graph_access( d.destinationAccess );
			dependencies[i].dependencyFlags = d.byRegion ? VK_DEPENDENCY_BY_REGION_BIT : 0;
		}
		VkRenderPassCreateInfo desc = {};
		desc.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		desc.attachmentCount = pass.attachmentCount;
		desc.pAttachments = attachments;
		desc.subpassCount = 1;
		desc.pSubpasses = &subpass;
		desc.dependencyCount = pass.dependencyCount;
		desc.pDependencies = dependencies;
		VkRenderPass *handle = vk_graph_pass( id );
		VK_CHECK( qvkCreateRenderPass( vk.device, &desc, NULL, handle ) );
		SET_OBJECT_NAME( *handle, va( "render pass - %s", vk_graph_names[(uint32_t)id] ), VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT );
	}
}


static void allocate_and_bind_image_memory( VkImage image ) {
	VkMemoryRequirements memory_requirements;
	VkDeviceSize alignment;
	ImageChunk *chunk;
	int i;

	qvkGetImageMemoryRequirements( vk.device, image, &memory_requirements );

	if ( memory_requirements.size > vk.image_chunk_size ) {
		vk_fail( ERR_FATAL, rhiStatus_t::Error, "Vulkan: could not allocate memory, image is too large (%ikbytes).",
			(int)( memory_requirements.size / 1024 ) );
	}

	chunk = NULL;

	// Try to find an existing chunk of sufficient capacity.
	alignment = memory_requirements.alignment;
	for ( i = 0; i < vk_world.num_image_chunks; i++ ) {
		// ensure that memory region has proper alignment
		VkDeviceSize offset = PAD( vk_world.image_chunks[i].used, alignment );

		if ( offset + memory_requirements.size <= vk.image_chunk_size ) {
			chunk = &vk_world.image_chunks[i];
			chunk->used = offset + memory_requirements.size;
			break;
		}
	}

	// Allocate a new chunk in case we couldn't find suitable existing chunk.
	if ( chunk == NULL ) {
		VkMemoryAllocateInfo alloc_info;
		VkDeviceMemory memory;

		if ( vk_world.num_image_chunks >= MAX_IMAGE_CHUNKS ) {
			vk_fail( ERR_FATAL, rhiStatus_t::Error, "Vulkan: image chunk limit has been reached" );
		}

		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.pNext = NULL;
		alloc_info.allocationSize = vk.image_chunk_size;
		alloc_info.memoryTypeIndex = find_memory_type( memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

		VK_CHECK( qvkAllocateMemory( vk.device, &alloc_info, NULL, &memory ) );

		chunk = &vk_world.image_chunks[vk_world.num_image_chunks];
		chunk->memory = memory;
		chunk->used = memory_requirements.size;

		SET_OBJECT_NAME( memory, va( "image memory chunk %i", vk_world.num_image_chunks ), VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT );

		vk_world.num_image_chunks++;
	}

	VK_CHECK(qvkBindImageMemory(vk.device, image, chunk->memory, chunk->used - memory_requirements.size));
}


static void vk_clean_staging_buffer( void ) {
	if ( vk.staging_buffer.handle != VK_NULL_HANDLE ) {
		qvkDestroyBuffer( vk.device, vk.staging_buffer.handle, NULL );
		vk.staging_buffer.handle = VK_NULL_HANDLE;
	}

	//if ( vk.staging_buffer.ptr != NULL )
	//	qvkUnmapMemory( vk.device, vk.staging_buffer.memory ) {
	//	vk.staging_buffer.ptr = NULL;
	//}

	if ( vk.staging_buffer.memory != VK_NULL_HANDLE ) {
		qvkFreeMemory( vk.device, vk.staging_buffer.memory, NULL );
		vk.staging_buffer.memory = VK_NULL_HANDLE;
	}

	vk.staging_buffer.ptr = NULL;
	vk.staging_buffer.size = 0;
#ifdef USE_UPLOAD_QUEUE
	vk.staging_buffer.offset = 0;
#endif
}


#ifdef USE_UPLOAD_QUEUE
static qboolean vk_wait_staging_buffer( void ) {
	if ( vk.aux_fence_wait ) {
		VkResult res = qvkWaitForFences( vk.device, 1, &vk.aux_fence, VK_TRUE, 5 * 1000000000ULL );
		if ( res != VK_SUCCESS ) {
			vk_fail( ERR_FATAL, rhiStatus_t::Error, "vkWaitForFences() failed with %s at %s", vk_result_string( res ), __func__ );
		}
		qvkResetFences( vk.device, 1, &vk.aux_fence );
		VK_CHECK( qvkResetCommandBuffer( vk.staging_command_buffer, 0 ) );
		vk.staging_buffer.offset = 0; // FIXME: is this correct?
		vk.aux_fence_wait = qfalse;
		return qtrue;
	} else {
		return qfalse;
	}
}


static void vk_flush_staging_buffer( qboolean final ) {
	const VkPipelineStageFlags wait_dst_stage_mask = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore waits;
	VkSubmitInfo submit_info;
	VkResult res;

	if ( vk.staging_buffer.offset == 0 ) {
		return;
	}

	//vk_host.Print( rhiLog_t::Warning, S_COLOR_CYAN ">>> flush %i bytes (final=%i)<<<\n", (int)vk_world.staging_buffer_offset, final );

	vk.staging_buffer.offset = 0;

	VK_CHECK( qvkEndCommandBuffer( vk.staging_command_buffer ) );

	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;

	if ( vk.rendering_finished != VK_NULL_HANDLE ) {
		// first call after previous queue submission?
		waits = vk.rendering_finished;
		vk.rendering_finished = VK_NULL_HANDLE;
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = &waits;
		submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
	} else {
		submit_info.waitSemaphoreCount = 0;
		submit_info.pWaitSemaphores = NULL;
		submit_info.pWaitDstStageMask = NULL;
	}

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &vk.staging_command_buffer;

	if ( vk.image_uploaded != VK_NULL_HANDLE ) {
		vk_fail( ERR_FATAL, rhiStatus_t::Error, "Vulkan: incorrect state during image upload" );
	}
	if ( final ) {
		// final submission before recording
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = &vk.image_uploaded2;
		vk.image_uploaded = vk.image_uploaded2;
		VK_CHECK( qvkQueueSubmit( vk.queue, 1, &submit_info, vk.aux_fence ) );
		vk.aux_fence_wait = qtrue;
	} else {
		// if submission before another upload then do explicit wait
		submit_info.signalSemaphoreCount = 0;
		submit_info.pSignalSemaphores = NULL;
		VK_CHECK( qvkQueueSubmit( vk.queue, 1, &submit_info, vk.aux_fence ) );
		res = qvkWaitForFences( vk.device, 1, &vk.aux_fence, VK_TRUE, 5 * 1000000000ULL );
		if ( res != VK_SUCCESS ) {
			vk_fail( ERR_FATAL, rhiStatus_t::Error, "vkWaitForFences() failed with %s at %s", vk_result_string( res ), __func__ );
		}
		qvkResetFences( vk.device, 1, &vk.aux_fence );
		VK_CHECK( qvkResetCommandBuffer( vk.staging_command_buffer, 0 ) );
	}
}
#endif // USE_UPLOAD_QUEUE


static void vk_alloc_staging_buffer( VkDeviceSize size ) {
	VkBufferCreateInfo buffer_desc;
	VkMemoryRequirements memory_requirements;
	VkMemoryAllocateInfo alloc_info;
	uint32_t memory_type;
	void *data;

	vk_clean_staging_buffer();

	vk.staging_buffer.size = MAX( size, STAGING_BUFFER_SIZE );
	vk.staging_buffer.size = PAD( vk.staging_buffer.size, 1024 * 1024 );

	buffer_desc.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_desc.pNext = NULL;
	buffer_desc.flags = 0;
	buffer_desc.size = vk.staging_buffer.size;
	buffer_desc.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	buffer_desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_desc.queueFamilyIndexCount = 0;
	buffer_desc.pQueueFamilyIndices = NULL;
	VK_CHECK(qvkCreateBuffer(vk.device, &buffer_desc, NULL, &vk.staging_buffer.handle));

	qvkGetBufferMemoryRequirements( vk.device, vk.staging_buffer.handle, &memory_requirements );

	memory_type = find_memory_type( memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = NULL;
	alloc_info.allocationSize = memory_requirements.size;
	alloc_info.memoryTypeIndex = memory_type;

	VK_CHECK(qvkAllocateMemory(vk.device, &alloc_info, NULL, &vk.staging_buffer.memory));
	VK_CHECK(qvkBindBufferMemory(vk.device, vk.staging_buffer.handle, vk.staging_buffer.memory, 0));

	VK_CHECK(qvkMapMemory(vk.device, vk.staging_buffer.memory, 0, VK_WHOLE_SIZE, 0, &data));
	vk.staging_buffer.ptr = (byte *)data;
#ifdef USE_UPLOAD_QUEUE
	vk.staging_buffer.offset = 0;
#endif
	SET_OBJECT_NAME( vk.staging_buffer.handle, "staging buffer", VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT );
	SET_OBJECT_NAME( vk.staging_buffer.memory, "staging buffer memory", VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT );
}


#ifdef USE_VK_VALIDATION
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback( VkDebugReportFlagsEXT flags [[maybe_unused]], VkDebugReportObjectTypeEXT object_type [[maybe_unused]], uint64_t object [[maybe_unused]], size_t location [[maybe_unused]],
	int32_t message_code [[maybe_unused]], const char *layer_prefix, const char *message, void *user_data [[maybe_unused]] ) {
#ifdef _WIN32
	Sys_GraphicsDebugMessage( message, layer_prefix );
#endif
	return VK_FALSE;
}
#endif


static qboolean used_instance_extension( const char *ext ) {
	const char *u;

	// allow all VK_*_surface extensions
	u = strrchr( ext, '_' );
	if ( u && Q_stricmp( u + 1, "surface" ) == 0 )
		return qtrue;

	if ( Q_stricmp( ext, VK_KHR_DISPLAY_EXTENSION_NAME ) == 0 )
		return qtrue; // needed for KMSDRM instances/devices?

	if ( Q_stricmp( ext, VK_KHR_SWAPCHAIN_EXTENSION_NAME ) == 0 )
		return qtrue;

#ifdef USE_VK_VALIDATION
	if ( Q_stricmp( ext, VK_EXT_DEBUG_REPORT_EXTENSION_NAME ) == 0 )
		return qtrue;
#endif

	if ( Q_stricmp( ext, VK_EXT_DEBUG_UTILS_EXTENSION_NAME ) == 0 )
		return qtrue;

	if ( Q_stricmp( ext, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME ) == 0 )
		return qtrue;

	if ( Q_stricmp( ext, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME ) == 0 )
		return qtrue;

	return qfalse;
}


static void create_instance( void ) {
#ifdef USE_VK_VALIDATION
	const char *validation_layer_name = "VK_LAYER_LUNARG_standard_validation";
	const char *validation_layer_name2 = "VK_LAYER_KHRONOS_validation";
#endif
	VkInstanceCreateInfo desc;
	VkInstanceCreateFlags flags;
	VkExtensionProperties *extension_properties;
	VkResult res;
	const char **extension_names;
	uint32_t i, n, count, extension_count;
	VkApplicationInfo appInfo;

	flags = 0;
	count = 0;
	extension_count = 0;
	VK_CHECK(qvkEnumerateInstanceExtensionProperties(NULL, &count, NULL));

	extension_properties = (VkExtensionProperties *)vk_host.Allocate( sizeof( VkExtensionProperties ) * count );
	extension_names = (const char **)vk_host.Allocate( sizeof( char * ) * count );

	VK_CHECK( qvkEnumerateInstanceExtensionProperties( NULL, &count, extension_properties ) );
	for ( i = 0; i < count; i++ ) {
		const char *ext = extension_properties[i].extensionName;

		if ( !used_instance_extension( ext ) ) {
			continue;
		}

		// search for duplicates
		for ( n = 0; n < extension_count; n++ ) {
			if ( Q_stricmp( ext, extension_names[n] ) == 0 ) {
				break;
			}
		}
		if ( n != extension_count ) {
			continue;
		}

		extension_names[extension_count++] = ext;

		if ( Q_stricmp( ext, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME ) == 0 ) {
			flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
		}

		vk_host.Print( rhiLog_t::Developer, "instance extension: %s\n", ext );
	}

	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = NULL;
	appInfo.pApplicationName = NULL; // Q3_VERSION;
	appInfo.applicationVersion = 0x0;
	appInfo.pEngineName = NULL;
	appInfo.engineVersion = 0x0;
#ifdef _DEBUG
	appInfo.apiVersion = VK_API_VERSION_1_1;
#else
	appInfo.apiVersion = VK_API_VERSION_1_0;
#endif

	// create instance
	desc.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	desc.pNext = NULL;
	desc.flags = flags;
	desc.pApplicationInfo = &appInfo;
	desc.enabledExtensionCount = extension_count;
	desc.ppEnabledExtensionNames = extension_names;

#ifdef USE_VK_VALIDATION
	desc.enabledLayerCount = 1;
	desc.ppEnabledLayerNames = &validation_layer_name;

	res = qvkCreateInstance( &desc, NULL, &vk_instance );

	if ( res == VK_ERROR_LAYER_NOT_PRESENT ) {

		desc.enabledLayerCount = 1;
		desc.ppEnabledLayerNames = &validation_layer_name2;

		res = qvkCreateInstance( &desc, NULL, &vk_instance );

		if ( res == VK_ERROR_LAYER_NOT_PRESENT ) {

			vk_host.Print( rhiLog_t::Warning, "...validation layer is not available\n" );

			// try without validation layer
			desc.enabledLayerCount = 0;
			desc.ppEnabledLayerNames = NULL;

			res = qvkCreateInstance( &desc, NULL, &vk_instance );
		}
	}
#else
	desc.enabledLayerCount = 0;
	desc.ppEnabledLayerNames = NULL;

	res = qvkCreateInstance( &desc, NULL, &vk_instance );
#endif

	vk_host.Free( (void *)extension_names );
	vk_host.Free( extension_properties );

	if ( res != VK_SUCCESS ) {
		vk_fail( ERR_FATAL, rhiStatus_t::Error, "Vulkan: instance creation failed with %s", vk_result_string( res ) );
	}
}


static VkFormat get_depth_format( VkPhysicalDevice physical_device ) {
	VkFormatProperties props;
	VkFormat formats[2];
	int i;

	if ( vk_config.stencilBits > 0 ) {
		formats[0] = vk_config.depthBits == 16 ? VK_FORMAT_D16_UNORM_S8_UINT : VK_FORMAT_D24_UNORM_S8_UINT;
		formats[1] = VK_FORMAT_D32_SFLOAT_S8_UINT;
	} else {
		formats[0] = vk_config.depthBits == 16 ? VK_FORMAT_D16_UNORM : VK_FORMAT_X8_D24_UNORM_PACK32;
		formats[1] = VK_FORMAT_D32_SFLOAT;
	}

	for ( i = 0; (size_t)i < ARRAY_LEN( formats ); i++ ) {
		qvkGetPhysicalDeviceFormatProperties( physical_device, formats[i], &props );
		if ( ( props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT ) != 0 ) {
			return formats[i];
		}
	}

	vk_fail( ERR_FATAL, rhiStatus_t::Error, "get_depth_format: failed to find depth attachment format" );
}


// Check if we can use vkCmdBlitImage for the given source and destination image formats.
static qboolean vk_blit_enabled( VkPhysicalDevice physical_device, const VkFormat srcFormat, const VkFormat dstFormat ) {
	VkFormatProperties formatProps;

	qvkGetPhysicalDeviceFormatProperties( physical_device, srcFormat, &formatProps );
	if ( ( formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT ) == 0 ) {
		return qfalse;
	}

	qvkGetPhysicalDeviceFormatProperties( physical_device, dstFormat, &formatProps );
	if ( ( formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT ) == 0 ) {
		return qfalse;
	}

	return qtrue;
}


static VkFormat get_hdr_format( VkFormat base_format ) {
	if ( vk_config.fbo == 0 ) {
		return base_format;
	}

	switch ( vk_config.hdr ) {
	case -1:
		return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
	case 1:
		return VK_FORMAT_R16G16B16A16_UNORM;
	default:
		return base_format;
	}
}

typedef struct {
	int bits;
	VkFormat rgb;
	VkFormat bgr;
} present_format_t;

static const present_format_t present_formats[] = {
	//{12, VK_FORMAT_B4G4R4A4_UNORM_PACK16, VK_FORMAT_R4G4B4A4_UNORM_PACK16},
	//{15, VK_FORMAT_B5G5R5A1_UNORM_PACK16, VK_FORMAT_R5G5B5A1_UNORM_PACK16},
	{ 16, VK_FORMAT_B5G6R5_UNORM_PACK16, VK_FORMAT_R5G6B5_UNORM_PACK16 },
	{ 24, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM },
	{ 30, VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_FORMAT_A2R10G10B10_UNORM_PACK32 },
	//{32, VK_FORMAT_B10G11R11_UFLOAT_PACK32, VK_FORMAT_B10G11R11_UFLOAT_PACK32}
};

static void get_present_format( int present_bits, VkFormat *bgr, VkFormat *rgb ) {
	const present_format_t *pf, *sel;
	int i;

	sel = NULL;
	pf = present_formats;
	for ( i = 0; (size_t)i < ARRAY_LEN( present_formats ); i++, pf++ ) {
		if ( pf->bits <= present_bits ) {
			sel = pf;
		}
	}
	if ( !sel ) {
		*bgr = VK_FORMAT_B8G8R8A8_UNORM;
		*rgb = VK_FORMAT_R8G8B8A8_UNORM;
	} else {
		*bgr = sel->bgr;
		*rgb = sel->rgb;
	}
}


static qboolean vk_select_surface_format( VkPhysicalDevice physical_device, VkSurfaceKHR surface ) {
	VkFormat base_bgr, base_rgb;
	VkFormat ext_bgr, ext_rgb;
	VkSurfaceFormatKHR *candidates;
	uint32_t format_count;
	VkResult res;

	res = qvkGetPhysicalDeviceSurfaceFormatsKHR( physical_device, surface, &format_count, NULL );
	if ( res < 0 ) {
		vk_host.Print( rhiLog_t::Error, "vkGetPhysicalDeviceSurfaceFormatsKHR returned %s\n", vk_result_string( res ) );
		return qfalse;
	}

	if ( format_count == 0 ) {
		vk_host.Print( rhiLog_t::Error, "...no surface formats found\n" );
		return qfalse;
	}

	candidates = (VkSurfaceFormatKHR *)vk_host.Allocate( format_count * sizeof( VkSurfaceFormatKHR ) );

	VK_CHECK( qvkGetPhysicalDeviceSurfaceFormatsKHR( physical_device, surface, &format_count, candidates ) );

	get_present_format( 24, &base_bgr, &base_rgb );

	if ( vk_config.fbo ) {
		get_present_format( vk_config.presentBits, &ext_bgr, &ext_rgb );
	} else {
		ext_bgr = base_bgr;
		ext_rgb = base_rgb;
	}

	if ( format_count == 1 && candidates[0].format == VK_FORMAT_UNDEFINED ) {
		// special case that means we can choose any format
		vk.base_format.format = base_bgr;
		vk.base_format.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		vk.present_format.format = ext_bgr;
		vk.present_format.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	} else {
		uint32_t i;
		for ( i = 0; i < format_count; i++ ) {
			if ( ( candidates[i].format == base_bgr || candidates[i].format == base_rgb ) && candidates[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR ) {
				vk.base_format = candidates[i];
				break;
			}
		}
		if ( i == format_count ) {
			vk.base_format = candidates[0];
		}
		for ( i = 0; i < format_count; i++ ) {
			if ( ( candidates[i].format == ext_bgr || candidates[i].format == ext_rgb ) && candidates[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR ) {
				vk.present_format = candidates[i];
				break;
			}
		}
		if ( i == format_count ) {
			vk.present_format = vk.base_format;
		}
	}

	if ( !vk_config.fbo ) {
		vk.present_format = vk.base_format;
	}

	vk_host.Free( candidates );

	return qtrue;
}


static void setup_surface_formats( VkPhysicalDevice physical_device ) {
	vk.depth_format = get_depth_format( physical_device );

	vk.color_format = get_hdr_format( vk.base_format.format );

	vk.capture_format = VK_FORMAT_R8G8B8A8_UNORM;

	vk.bloom_format = vk.base_format.format;

	vk.blitEnabled = vk_blit_enabled( physical_device, vk.color_format, vk.capture_format );

	if ( !vk.blitEnabled ) {
		vk.capture_format = vk.color_format;
	}
}


static const char *renderer_name( const VkPhysicalDeviceProperties *props ) {
	static char buf[sizeof( props->deviceName ) + 64];
	const char *device_type;

	switch ( props->deviceType ) {
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		device_type = "Integrated";
		break;
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		device_type = "Discrete";
		break;
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
		device_type = "Virtual";
		break;
	case VK_PHYSICAL_DEVICE_TYPE_CPU:
		device_type = "CPU";
		break;
	default:
		device_type = "OTHER";
		break;
	}

	Com_sprintf( buf, sizeof( buf ), "%s %s, 0x%04x",
		device_type, props->deviceName, props->deviceID );

	return buf;
}


static qboolean vk_create_device( VkPhysicalDevice physical_device, int device_index ) {

#ifdef _DEBUG
	VkPhysicalDeviceTimelineSemaphoreFeatures timeline_semaphore;
	VkPhysicalDeviceVulkanMemoryModelFeatures memory_model;
	VkPhysicalDeviceBufferDeviceAddressFeatures devaddr_features;
	VkPhysicalDevice8BitStorageFeatures storage_8bit_features;
#endif

	vk_host.Print( rhiLog_t::Info, "...selected physical device: %i\n", device_index );

	// select surface format
	if ( !vk_select_surface_format( physical_device, vk_surface ) ) {
		return qfalse;
	}

	setup_surface_formats( physical_device );

	// select queue family
	{
		VkQueueFamilyProperties *queue_families;
		uint32_t queue_family_count;
		uint32_t i;

		qvkGetPhysicalDeviceQueueFamilyProperties( physical_device, &queue_family_count, NULL );
		queue_families = (VkQueueFamilyProperties *)vk_host.Allocate( queue_family_count * sizeof( VkQueueFamilyProperties ) );
		qvkGetPhysicalDeviceQueueFamilyProperties( physical_device, &queue_family_count, queue_families );

		// select queue family with presentation and graphics support
		vk.queue_family_index = ~0U;
		for ( i = 0; i < queue_family_count; i++ ) {
			VkBool32 presentation_supported;
			VK_CHECK( qvkGetPhysicalDeviceSurfaceSupportKHR( physical_device, i, vk_surface, &presentation_supported ) );

			if ( presentation_supported && ( queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ) != 0 ) {
				vk.queue_family_index = i;
				vk.timestampBits = queue_families[i].timestampValidBits;
				break;
			}
		}

		vk_host.Free( queue_families );

		if ( vk.queue_family_index == ~0U ) {
			vk_host.Print( rhiLog_t::Error, "...failed to find graphics queue family\n" );

			return qfalse;
		}
	}

	// create VkDevice
	{
		const char *device_extension_list[8];
		uint32_t device_extension_count;
		const char *ext, *end;
		char *str;
		const float priority = 1.0;
		VkExtensionProperties *extension_properties;
		VkDeviceQueueCreateInfo queue_desc;
		VkPhysicalDeviceFeatures device_features;
		VkPhysicalDeviceFeatures features;
		VkDeviceCreateInfo device_desc;
		VkResult res;
		qboolean swapchainSupported = qfalse;
		qboolean dedicatedAllocation = qfalse;
		qboolean memoryRequirements2 = qfalse;
		qboolean debugMarker = qfalse;
#ifdef _DEBUG
		qboolean timelineSemaphore = qfalse;
		qboolean memoryModel = qfalse;
		qboolean devAddrFeat = qfalse;
		qboolean storage8bit = qfalse;
		const void **pNextPtr;
#endif
		uint32_t i, len, count = 0;

		VK_CHECK( qvkEnumerateDeviceExtensionProperties( physical_device, NULL, &count, NULL ) );
		extension_properties = (VkExtensionProperties *)vk_host.Allocate( count * sizeof( VkExtensionProperties ) );
		VK_CHECK( qvkEnumerateDeviceExtensionProperties( physical_device, NULL, &count, extension_properties ) );

		// fill vk_device_info->extensions
		str = vk_device_info->extensions;
		*str = '\0';
		end = &vk_device_info->extensions[sizeof( vk_device_info->extensions ) - 1];

		for ( i = 0; i < count; i++ ) {
			ext = extension_properties[i].extensionName;
			if ( strcmp( ext, VK_KHR_SWAPCHAIN_EXTENSION_NAME ) == 0 ) {
				swapchainSupported = qtrue;
			} else if ( strcmp( ext, VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME ) == 0 ) {
				dedicatedAllocation = qtrue;
			} else if ( strcmp( ext, VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME ) == 0 ) {
				memoryRequirements2 = qtrue;
			} else if ( strcmp( ext, VK_EXT_DEBUG_MARKER_EXTENSION_NAME ) == 0 ) {
				debugMarker = qtrue;
#ifdef _DEBUG
			} else if ( strcmp( ext, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME ) == 0 ) {
				timelineSemaphore = qtrue;
			} else if ( strcmp( ext, VK_KHR_VULKAN_MEMORY_MODEL_EXTENSION_NAME ) == 0 ) {
				memoryModel = qtrue;
			} else if ( strcmp( ext, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME ) == 0 ) {
				devAddrFeat = qtrue;
			} else if ( strcmp( ext, VK_KHR_8BIT_STORAGE_EXTENSION_NAME ) == 0 ) {
				storage8bit = qtrue;
#endif
			}
			// add this device extension to glConfig
			if ( i != 0 ) {
				if ( str + 1 >= end )
					continue;
				str = Q_stradd( str, " " );
			}
			len = (uint32_t)strlen( ext );
			if ( str + len >= end )
				continue;
			str = Q_stradd( str, ext );
		}

		vk_host.Free( extension_properties );

		device_extension_count = 0;

		if ( !swapchainSupported ) {
			vk_host.Print( rhiLog_t::Error, "...required device extension is not available: %s\n", VK_KHR_SWAPCHAIN_EXTENSION_NAME );
			return qfalse;
		}

		if ( !memoryRequirements2 )
			dedicatedAllocation = qfalse;
		else
			vk.dedicatedAllocation = dedicatedAllocation;

#ifndef USE_DEDICATED_ALLOCATION
		vk.dedicatedAllocation = qfalse;
#endif

		device_extension_list[device_extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

		if ( vk.dedicatedAllocation ) {
			device_extension_list[device_extension_count++] = VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME;
			device_extension_list[device_extension_count++] = VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME;
		}

		if ( debugMarker ) {
			device_extension_list[device_extension_count++] = VK_EXT_DEBUG_MARKER_EXTENSION_NAME;
			vk.debugMarkers = qtrue;
		}
#ifdef _DEBUG
		if ( timelineSemaphore ) {
			device_extension_list[device_extension_count++] = VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME;
		}

		if ( memoryModel ) {
			device_extension_list[device_extension_count++] = VK_KHR_VULKAN_MEMORY_MODEL_EXTENSION_NAME;
		}

		if ( devAddrFeat ) {
			device_extension_list[device_extension_count++] = VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME;
		}

		if ( storage8bit ) {
			device_extension_list[device_extension_count++] = VK_KHR_8BIT_STORAGE_EXTENSION_NAME;
		}
#endif // _DEBUG
		qvkGetPhysicalDeviceFeatures( physical_device, &device_features );

		if ( device_features.fillModeNonSolid == VK_FALSE ) {
			vk_host.Print( rhiLog_t::Error, "...fillModeNonSolid feature is not supported\n" );
			return qfalse;
		}

		queue_desc.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_desc.pNext = NULL;
		queue_desc.flags = 0;
		queue_desc.queueFamilyIndex = vk.queue_family_index;
		queue_desc.queueCount = 1;
		queue_desc.pQueuePriorities = &priority;

		Com_Memset( &features, 0, sizeof( features ) );
		features.fillModeNonSolid = VK_TRUE;
		features.textureCompressionBC = device_features.textureCompressionBC;
		vk.compressionBC = device_features.textureCompressionBC ? qtrue : qfalse;

#ifdef _DEBUG
		if ( device_features.shaderInt64 ) {
			features.shaderInt64 = VK_TRUE;
		}
#endif
		if ( device_features.wideLines ) { // needed for RB_SurfaceAxis
			features.wideLines = VK_TRUE;
			vk.wideLines = qtrue;
		}

		if ( device_features.fragmentStoresAndAtomics && device_features.vertexPipelineStoresAndAtomics ) {
			features.vertexPipelineStoresAndAtomics = VK_TRUE;
			features.fragmentStoresAndAtomics = VK_TRUE;
			vk.fragmentStores = qtrue;
		}

		if ( vk_config.anisotropy && device_features.samplerAnisotropy ) {
			features.samplerAnisotropy = VK_TRUE;
			vk.samplerAnisotropy = qtrue;
		}

		device_desc.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_desc.pNext = NULL;
		device_desc.flags = 0;
		device_desc.queueCreateInfoCount = 1;
		device_desc.pQueueCreateInfos = &queue_desc;
		device_desc.enabledLayerCount = 0;
		device_desc.ppEnabledLayerNames = NULL;
		device_desc.enabledExtensionCount = device_extension_count;
		device_desc.ppEnabledExtensionNames = device_extension_list;
		device_desc.pEnabledFeatures = &features;

#ifdef _DEBUG
		pNextPtr = (const void **)&device_desc.pNext;

		if ( timelineSemaphore ) {
			*pNextPtr = &timeline_semaphore;
			timeline_semaphore.pNext = NULL;
			timeline_semaphore.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
			timeline_semaphore.timelineSemaphore = VK_TRUE;
			pNextPtr = (const void **)&timeline_semaphore.pNext;
		}

		if ( memoryModel ) {
			*pNextPtr = &memory_model;
			memory_model.pNext = NULL;
			memory_model.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES;
			memory_model.vulkanMemoryModel = VK_TRUE;
			memory_model.vulkanMemoryModelAvailabilityVisibilityChains = VK_FALSE;
			memory_model.vulkanMemoryModelDeviceScope = VK_TRUE;
			pNextPtr = (const void **)&memory_model.pNext;
		}

		if ( devAddrFeat ) {
			*pNextPtr = &devaddr_features;
			devaddr_features.pNext = NULL;
			devaddr_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
			devaddr_features.bufferDeviceAddress = VK_TRUE;
			devaddr_features.bufferDeviceAddressCaptureReplay = VK_FALSE;
			devaddr_features.bufferDeviceAddressMultiDevice = VK_FALSE;
			pNextPtr = (const void **)&devaddr_features.pNext;
		}

		if ( storage8bit ) {
			*pNextPtr = &storage_8bit_features;
			storage_8bit_features.pNext = NULL;
			storage_8bit_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES;
			storage_8bit_features.storageBuffer8BitAccess = VK_TRUE;
			storage_8bit_features.storagePushConstant8 = VK_FALSE;
			storage_8bit_features.uniformAndStorageBuffer8BitAccess = VK_TRUE;
			pNextPtr = (const void **)&storage_8bit_features.pNext;
		}
#endif
		res = qvkCreateDevice( physical_device, &device_desc, NULL, &vk.device );
		if ( res < 0 ) {
			vk_host.Print( rhiLog_t::Error, "vkCreateDevice returned %s\n", vk_result_string( res ) );
			return qfalse;
		}
	}

	return qtrue;
}


#define INIT_INSTANCE_FUNCTION( func ) \
	q##func = /*(PFN_ ## func)*/ (PFN_ ## func)vk_host.GetInstanceProcAddr((uint64_t)(uintptr_t)vk_instance, #func); \
	if (q##func == NULL) {											\
		vk_fail( ERR_FATAL, rhiStatus_t::Error, "Failed to find entrypoint %s", #func);	\
	}

#define INIT_INSTANCE_FUNCTION_EXT( func ) \
	q##func = /*(PFN_ ## func)*/ (PFN_ ## func)vk_host.GetInstanceProcAddr((uint64_t)(uintptr_t)vk_instance, #func);


#define INIT_DEVICE_FUNCTION( func ) \
	q##func = (PFN_ ## func) qvkGetDeviceProcAddr(vk.device, #func);\
	if (q##func == NULL) {											\
		vk_fail( ERR_FATAL, rhiStatus_t::Error, "Failed to find entrypoint %s", #func);	\
	}

#define INIT_DEVICE_FUNCTION_EXT( func ) \
	q##func = (PFN_ ## func) qvkGetDeviceProcAddr(vk.device, #func);


static void vk_destroy_instance( void ) {
	if ( vk_surface != VK_NULL_HANDLE ) {
		if ( qvkDestroySurfaceKHR != NULL ) {
			qvkDestroySurfaceKHR( vk_instance, vk_surface, NULL );
		}
		vk_surface = VK_NULL_HANDLE;
	}

#ifdef USE_VK_VALIDATION
	if ( vk_debug_callback ) {
		if ( qvkDestroyDebugReportCallbackEXT != NULL ) {
			qvkDestroyDebugReportCallbackEXT( vk_instance, vk_debug_callback, NULL );
		}
		vk_debug_callback = VK_NULL_HANDLE;
	}
#endif

	if ( vk_instance != VK_NULL_HANDLE ) {
		if ( qvkDestroyInstance ) {
			qvkDestroyInstance( vk_instance, NULL );
		}
		vk_instance = VK_NULL_HANDLE;
	}
}


static void init_vulkan_library( void ) {
	VkPhysicalDeviceProperties props;
	VkPhysicalDevice *physical_devices;
	uint32_t device_count;
	int device_index, i;
	VkResult res;

	Com_Memset( &vk, 0, sizeof( vk ) );

	if ( vk_instance == VK_NULL_HANDLE ) {

		// force cleanup
		vk_destroy_instance();

		// Get functions that do not depend on VkInstance (vk_instance == nullptr at this point).
		INIT_INSTANCE_FUNCTION( vkCreateInstance )
		INIT_INSTANCE_FUNCTION( vkEnumerateInstanceExtensionProperties )

		// Get instance level functions.
		create_instance();

		INIT_INSTANCE_FUNCTION( vkCreateDevice )
		INIT_INSTANCE_FUNCTION( vkDestroyInstance )
		INIT_INSTANCE_FUNCTION( vkEnumerateDeviceExtensionProperties )
		INIT_INSTANCE_FUNCTION( vkEnumeratePhysicalDevices )
		INIT_INSTANCE_FUNCTION( vkGetDeviceProcAddr )
		INIT_INSTANCE_FUNCTION( vkGetPhysicalDeviceFeatures )
		INIT_INSTANCE_FUNCTION( vkGetPhysicalDeviceFormatProperties )
		INIT_INSTANCE_FUNCTION( vkGetPhysicalDeviceMemoryProperties )
		INIT_INSTANCE_FUNCTION( vkGetPhysicalDeviceProperties )
		INIT_INSTANCE_FUNCTION( vkGetPhysicalDeviceQueueFamilyProperties )
		INIT_INSTANCE_FUNCTION( vkDestroySurfaceKHR )
		INIT_INSTANCE_FUNCTION( vkGetPhysicalDeviceSurfaceCapabilitiesKHR )
		INIT_INSTANCE_FUNCTION( vkGetPhysicalDeviceSurfaceFormatsKHR )
		INIT_INSTANCE_FUNCTION( vkGetPhysicalDeviceSurfacePresentModesKHR )
		INIT_INSTANCE_FUNCTION( vkGetPhysicalDeviceSurfaceSupportKHR )

#ifdef USE_VK_VALIDATION
		INIT_INSTANCE_FUNCTION_EXT( vkCreateDebugReportCallbackEXT )
		INIT_INSTANCE_FUNCTION_EXT( vkDestroyDebugReportCallbackEXT )

		// Create debug callback.
		if ( qvkCreateDebugReportCallbackEXT && qvkDestroyDebugReportCallbackEXT ) {
			VkDebugReportCallbackCreateInfoEXT desc;
			desc.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
			desc.pNext = NULL;
			desc.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT |
						 VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
						 VK_DEBUG_REPORT_ERROR_BIT_EXT;
			desc.pfnCallback = &debug_callback;
			desc.pUserData = NULL;

			VK_CHECK( qvkCreateDebugReportCallbackEXT( vk_instance, &desc, NULL, &vk_debug_callback ) );
		}
#endif

		// create surface through the SDK-independent platform import
		uint64_t surface;
		if ( !vk_host.CreateSurface( (uint64_t)(uintptr_t)vk_instance, &surface ) ) {
			vk_fail( ERR_FATAL, rhiStatus_t::Error, "Error creating Vulkan surface" );
		}
		vk_surface = (VkSurfaceKHR)(uintptr_t)surface;
	} // vk_instance == VK_NULL_HANDLE

	res = qvkEnumeratePhysicalDevices( vk_instance, &device_count, NULL );
	if ( device_count == 0 ) {
		vk_fail( ERR_FATAL, rhiStatus_t::Error, "Vulkan: no physical devices found" );
	} else if ( res < 0 ) {
		vk_fail( ERR_FATAL, rhiStatus_t::Error, "vkEnumeratePhysicalDevices returned %s", vk_result_string( res ) );
	}

	physical_devices = (VkPhysicalDevice *)vk_host.Allocate( device_count * sizeof( VkPhysicalDevice ) );
	VK_CHECK( qvkEnumeratePhysicalDevices( vk_instance, &device_count, physical_devices ) );

	// initial physical device index
	device_index = vk_config.device;

	vk_host.Print( rhiLog_t::Info, ".......................\nAvailable physical devices:\n" );
	for ( i = 0; (uint32_t)i < device_count; i++ ) {
		qvkGetPhysicalDeviceProperties( physical_devices[i], &props );
		vk_host.Print( rhiLog_t::Info, " %i: %s\n", i, renderer_name( &props ) );
		if ( device_index == -1 && props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ) {
			device_index = i;
		} else if ( device_index == -2 && props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ) {
			device_index = i;
		}
	}
	vk_host.Print( rhiLog_t::Info, ".......................\n" );

	vk.physical_device = VK_NULL_HANDLE;
	for ( i = 0; (uint32_t)i < device_count; i++, device_index++ ) {
		if ( (uint32_t)device_index >= device_count || device_index < 0 ) {
			device_index = 0;
		}
		if ( vk_create_device( physical_devices[device_index], device_index ) ) {
			vk.physical_device = physical_devices[device_index];
			break;
		}
	}

	vk_host.Free( physical_devices );

	if ( vk.physical_device == VK_NULL_HANDLE ) {
		vk_fail( ERR_FATAL, rhiStatus_t::Error, "Vulkan: unable to find any suitable physical device" );
	}

	//
	// Get device level functions.
	//
	INIT_DEVICE_FUNCTION(vkAllocateCommandBuffers)
	INIT_DEVICE_FUNCTION(vkAllocateDescriptorSets)
	INIT_DEVICE_FUNCTION(vkAllocateMemory)
	INIT_DEVICE_FUNCTION(vkBeginCommandBuffer)
	INIT_DEVICE_FUNCTION(vkBindBufferMemory)
	INIT_DEVICE_FUNCTION(vkBindImageMemory)
	INIT_DEVICE_FUNCTION(vkCmdBeginRenderPass)
	INIT_DEVICE_FUNCTION(vkCmdBindDescriptorSets)
	INIT_DEVICE_FUNCTION(vkCmdBindIndexBuffer)
	INIT_DEVICE_FUNCTION(vkCmdBindPipeline)
	INIT_DEVICE_FUNCTION(vkCmdBindVertexBuffers)
	INIT_DEVICE_FUNCTION(vkCmdBlitImage)
	INIT_DEVICE_FUNCTION(vkCmdClearAttachments)
	INIT_DEVICE_FUNCTION(vkCmdCopyBuffer)
	INIT_DEVICE_FUNCTION(vkCmdCopyBufferToImage)
	INIT_DEVICE_FUNCTION(vkCmdCopyImage)
	INIT_DEVICE_FUNCTION(vkCmdDraw)
	INIT_DEVICE_FUNCTION(vkCmdDrawIndexed)
	INIT_DEVICE_FUNCTION(vkCmdEndRenderPass)
	INIT_DEVICE_FUNCTION(vkCmdNextSubpass)
	INIT_DEVICE_FUNCTION(vkCmdPipelineBarrier)
	INIT_DEVICE_FUNCTION(vkCmdPushConstants)
	INIT_DEVICE_FUNCTION(vkCmdSetDepthBias)
	INIT_DEVICE_FUNCTION(vkCmdSetScissor)
	INIT_DEVICE_FUNCTION(vkCmdSetViewport)
	INIT_DEVICE_FUNCTION(vkCreateQueryPool)
	INIT_DEVICE_FUNCTION(vkDestroyQueryPool)
	INIT_DEVICE_FUNCTION(vkCmdResetQueryPool)
	INIT_DEVICE_FUNCTION(vkCmdWriteTimestamp)
	INIT_DEVICE_FUNCTION(vkGetQueryPoolResults)
	INIT_DEVICE_FUNCTION(vkCreateBuffer)
	INIT_DEVICE_FUNCTION(vkCreateCommandPool)
	INIT_DEVICE_FUNCTION(vkCreateDescriptorPool)
	INIT_DEVICE_FUNCTION(vkCreateDescriptorSetLayout)
	INIT_DEVICE_FUNCTION(vkCreateFence)
	INIT_DEVICE_FUNCTION(vkCreateFramebuffer)
	INIT_DEVICE_FUNCTION(vkCreateGraphicsPipelines)
	INIT_DEVICE_FUNCTION(vkCreateImage)
	INIT_DEVICE_FUNCTION(vkCreateImageView)
	INIT_DEVICE_FUNCTION(vkCreatePipelineCache)
	INIT_DEVICE_FUNCTION(vkGetPipelineCacheData)
	INIT_DEVICE_FUNCTION(vkCreatePipelineLayout)
	INIT_DEVICE_FUNCTION(vkCreateRenderPass)
	INIT_DEVICE_FUNCTION(vkCreateSampler)
	INIT_DEVICE_FUNCTION(vkCreateSemaphore)
	INIT_DEVICE_FUNCTION(vkCreateShaderModule)
	INIT_DEVICE_FUNCTION(vkDestroyBuffer)
	INIT_DEVICE_FUNCTION(vkDestroyCommandPool)
	INIT_DEVICE_FUNCTION(vkDestroyDescriptorPool)
	INIT_DEVICE_FUNCTION(vkDestroyDescriptorSetLayout)
	INIT_DEVICE_FUNCTION(vkDestroyDevice)
	INIT_DEVICE_FUNCTION(vkDestroyFence)
	INIT_DEVICE_FUNCTION(vkDestroyFramebuffer)
	INIT_DEVICE_FUNCTION(vkDestroyImage)
	INIT_DEVICE_FUNCTION(vkDestroyImageView)
	INIT_DEVICE_FUNCTION(vkDestroyPipeline)
	INIT_DEVICE_FUNCTION(vkDestroyPipelineCache)
	INIT_DEVICE_FUNCTION(vkDestroyPipelineLayout)
	INIT_DEVICE_FUNCTION(vkDestroyRenderPass)
	INIT_DEVICE_FUNCTION(vkDestroySampler)
	INIT_DEVICE_FUNCTION(vkDestroySemaphore)
	INIT_DEVICE_FUNCTION(vkDestroyShaderModule)
	INIT_DEVICE_FUNCTION(vkDeviceWaitIdle)
	INIT_DEVICE_FUNCTION(vkEndCommandBuffer)
	INIT_DEVICE_FUNCTION(vkFlushMappedMemoryRanges)
	INIT_DEVICE_FUNCTION(vkFreeCommandBuffers)
	INIT_DEVICE_FUNCTION(vkFreeDescriptorSets)
	INIT_DEVICE_FUNCTION(vkFreeMemory)
	INIT_DEVICE_FUNCTION(vkGetBufferMemoryRequirements)
	INIT_DEVICE_FUNCTION(vkGetDeviceQueue)
	INIT_DEVICE_FUNCTION(vkGetImageMemoryRequirements)
	INIT_DEVICE_FUNCTION(vkGetImageSubresourceLayout)
	INIT_DEVICE_FUNCTION(vkInvalidateMappedMemoryRanges)
	INIT_DEVICE_FUNCTION(vkMapMemory)
	INIT_DEVICE_FUNCTION(vkQueueSubmit)
	INIT_DEVICE_FUNCTION(vkQueueWaitIdle)
	INIT_DEVICE_FUNCTION(vkResetCommandBuffer)
	INIT_DEVICE_FUNCTION(vkResetDescriptorPool)
	INIT_DEVICE_FUNCTION(vkResetFences)
	INIT_DEVICE_FUNCTION(vkUnmapMemory)
	INIT_DEVICE_FUNCTION(vkUpdateDescriptorSets)
	INIT_DEVICE_FUNCTION(vkWaitForFences)
	INIT_DEVICE_FUNCTION(vkAcquireNextImageKHR)
	INIT_DEVICE_FUNCTION(vkCreateSwapchainKHR)
	INIT_DEVICE_FUNCTION(vkDestroySwapchainKHR)
	INIT_DEVICE_FUNCTION(vkGetSwapchainImagesKHR)
	INIT_DEVICE_FUNCTION(vkQueuePresentKHR)

	if ( vk.dedicatedAllocation ) {
		INIT_DEVICE_FUNCTION_EXT(vkGetBufferMemoryRequirements2KHR);
		INIT_DEVICE_FUNCTION_EXT(vkGetImageMemoryRequirements2KHR);
		if ( !qvkGetBufferMemoryRequirements2KHR || !qvkGetImageMemoryRequirements2KHR ) {
			vk.dedicatedAllocation = qfalse;
		}
	}

	if ( vk.debugMarkers ) {
		INIT_DEVICE_FUNCTION_EXT(vkDebugMarkerSetObjectNameEXT)
	}
}

#undef INIT_INSTANCE_FUNCTION
#undef INIT_DEVICE_FUNCTION
#undef INIT_DEVICE_FUNCTION_EXT

static void deinit_instance_functions( void ) {
	qvkCreateInstance = NULL;
	qvkEnumerateInstanceExtensionProperties = NULL;

	// instance functions:
	qvkCreateDevice = NULL;
	qvkDestroyInstance = NULL;
	qvkEnumerateDeviceExtensionProperties = NULL;
	qvkEnumeratePhysicalDevices = NULL;
	qvkGetDeviceProcAddr = NULL;
	qvkGetPhysicalDeviceFeatures = NULL;
	qvkGetPhysicalDeviceFormatProperties = NULL;
	qvkGetPhysicalDeviceMemoryProperties = NULL;
	qvkGetPhysicalDeviceProperties = NULL;
	qvkGetPhysicalDeviceQueueFamilyProperties = NULL;
	qvkDestroySurfaceKHR = NULL;
	qvkGetPhysicalDeviceSurfaceCapabilitiesKHR = NULL;
	qvkGetPhysicalDeviceSurfaceFormatsKHR = NULL;
	qvkGetPhysicalDeviceSurfacePresentModesKHR = NULL;
	qvkGetPhysicalDeviceSurfaceSupportKHR = NULL;
#ifdef USE_VK_VALIDATION
	qvkCreateDebugReportCallbackEXT = NULL;
	qvkDestroyDebugReportCallbackEXT = NULL;
#endif
}


static void deinit_device_functions( void ) {
	// device functions:
	qvkAllocateCommandBuffers = NULL;
	qvkAllocateDescriptorSets = NULL;
	qvkAllocateMemory = NULL;
	qvkBeginCommandBuffer = NULL;
	qvkBindBufferMemory = NULL;
	qvkBindImageMemory = NULL;
	qvkCmdBeginRenderPass = NULL;
	qvkCmdBindDescriptorSets = NULL;
	qvkCmdBindIndexBuffer = NULL;
	qvkCmdBindPipeline = NULL;
	qvkCmdBindVertexBuffers = NULL;
	qvkCmdBlitImage = NULL;
	qvkCmdClearAttachments = NULL;
	qvkCmdCopyBuffer = NULL;
	qvkCmdCopyBufferToImage = NULL;
	qvkCmdCopyImage = NULL;
	qvkCmdDraw = NULL;
	qvkCmdDrawIndexed = NULL;
	qvkCmdEndRenderPass = NULL;
	qvkCmdNextSubpass = NULL;
	qvkCmdPipelineBarrier = NULL;
	qvkCmdPushConstants = NULL;
	qvkCmdSetDepthBias = NULL;
	qvkCmdSetScissor = NULL;
	qvkCmdSetViewport = NULL;
	qvkCreateQueryPool = NULL;
	qvkDestroyQueryPool = NULL;
	qvkCmdResetQueryPool = NULL;
	qvkCmdWriteTimestamp = NULL;
	qvkGetQueryPoolResults = NULL;
	qvkCreateBuffer = NULL;
	qvkCreateCommandPool = NULL;
	qvkCreateDescriptorPool = NULL;
	qvkCreateDescriptorSetLayout = NULL;
	qvkCreateFence = NULL;
	qvkCreateFramebuffer = NULL;
	qvkCreateGraphicsPipelines = NULL;
	qvkCreateImage = NULL;
	qvkCreateImageView = NULL;
	qvkCreatePipelineCache = NULL;
	qvkGetPipelineCacheData = NULL;
	qvkCreatePipelineLayout = NULL;
	qvkCreateRenderPass = NULL;
	qvkCreateSampler = NULL;
	qvkCreateSemaphore = NULL;
	qvkCreateShaderModule = NULL;
	qvkDestroyBuffer = NULL;
	qvkDestroyCommandPool = NULL;
	qvkDestroyDescriptorPool = NULL;
	qvkDestroyDescriptorSetLayout = NULL;
	qvkDestroyDevice = NULL;
	qvkDestroyFence = NULL;
	qvkDestroyFramebuffer = NULL;
	qvkDestroyImage = NULL;
	qvkDestroyImageView = NULL;
	qvkDestroyPipeline = NULL;
	qvkDestroyPipelineCache = NULL;
	qvkDestroyPipelineLayout = NULL;
	qvkDestroyRenderPass = NULL;
	qvkDestroySampler = NULL;
	qvkDestroySemaphore = NULL;
	qvkDestroyShaderModule = NULL;
	qvkDeviceWaitIdle = NULL;
	qvkEndCommandBuffer = NULL;
	qvkFlushMappedMemoryRanges = NULL;
	qvkFreeCommandBuffers = NULL;
	qvkFreeDescriptorSets = NULL;
	qvkFreeMemory = NULL;
	qvkGetBufferMemoryRequirements = NULL;
	qvkGetDeviceQueue = NULL;
	qvkGetImageMemoryRequirements = NULL;
	qvkGetImageSubresourceLayout = NULL;
	qvkInvalidateMappedMemoryRanges = NULL;
	qvkMapMemory = NULL;
	qvkQueueSubmit = NULL;
	qvkQueueWaitIdle = NULL;
	qvkResetCommandBuffer = NULL;
	qvkResetDescriptorPool = NULL;
	qvkResetFences = NULL;
	qvkUnmapMemory = NULL;
	qvkUpdateDescriptorSets = NULL;
	qvkWaitForFences = NULL;
	qvkAcquireNextImageKHR = NULL;
	qvkCreateSwapchainKHR = NULL;
	qvkDestroySwapchainKHR = NULL;
	qvkGetSwapchainImagesKHR = NULL;
	qvkQueuePresentKHR = NULL;

	qvkGetBufferMemoryRequirements2KHR = NULL;
	qvkGetImageMemoryRequirements2KHR = NULL;

	qvkDebugMarkerSetObjectNameEXT = NULL;
}


static VkShaderModule SHADER_MODULE( const uint8_t *bytes, const int count ) {
	VkShaderModuleCreateInfo desc;
	VkShaderModule module;

	if ( count % 4 != 0 ) {
		vk_fail( ERR_FATAL, rhiStatus_t::Error, "Vulkan: SPIR-V binary buffer size is not a multiple of 4" );
	}

	desc.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	desc.pNext = NULL;
	desc.flags = 0;
	desc.codeSize = count;
	desc.pCode = (const uint32_t *)bytes;

	VK_CHECK(qvkCreateShaderModule(vk.device, &desc, NULL, &module));

	return module;
}


static void vk_create_layout_binding( int binding, VkDescriptorType type, VkShaderStageFlags flags, VkDescriptorSetLayout *layout ) {
	VkDescriptorSetLayoutBinding bind;
	VkDescriptorSetLayoutCreateInfo desc;

	bind.binding = binding;
	bind.descriptorType = type;
	bind.descriptorCount = 1;
	bind.stageFlags = flags;
	bind.pImmutableSamplers = NULL;

	desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	desc.pNext = NULL;
	desc.flags = 0;
	desc.bindingCount = 1;
	desc.pBindings = &bind;

	VK_CHECK( qvkCreateDescriptorSetLayout(vk.device, &desc, NULL, layout ) );
}


void vk_update_uniform_descriptor( VkDescriptorSet descriptor, VkBuffer buffer ) {
	VkDescriptorBufferInfo info;
	VkWriteDescriptorSet desc;

	info.buffer = buffer;
	info.offset = 0;
	info.range = vk_config.uniformBytes;

	desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc.dstSet = descriptor;
	desc.dstBinding = 0;
	desc.dstArrayElement = 0;
	desc.descriptorCount = 1;
	desc.pNext = NULL;
	desc.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	desc.pImageInfo = NULL;
	desc.pBufferInfo = &info;
	desc.pTexelBufferView = NULL;

	qvkUpdateDescriptorSets( vk.device, 1, &desc, 0, NULL );
}


static VkSampler vk_find_sampler( const Vk_Sampler_Def *def ) {
	VkSamplerAddressMode address_mode;
	VkSamplerCreateInfo desc;
	VkSampler sampler;
	VkFilter mag_filter;
	VkFilter min_filter;
	VkSamplerMipmapMode mipmap_mode;
	float maxLod;
	int i;

	// Look for sampler among existing samplers.
	for ( i = 0; i < vk.samplers.count; i++ ) {
		const Vk_Sampler_Def *cur_def = &vk.samplers.def[i];
		if ( memcmp( cur_def, def, sizeof( *def ) ) == 0 ) {
			return vk.samplers.handle[i];
		}
	}

	// Create new sampler.
	if ( vk.samplers.count >= MAX_VK_SAMPLERS ) {
		vk_fail( ERR_DROP, rhiStatus_t::Error, "vk_find_sampler: MAX_VK_SAMPLERS hit\n" );
		// return VK_NULL_HANDLE;
	}

	address_mode = def->address_mode;

	if ( def->gl_mag_filter == FILTER_NEAREST ) {
		mag_filter = VK_FILTER_NEAREST;
	} else if ( def->gl_mag_filter == FILTER_LINEAR ) {
		mag_filter = VK_FILTER_LINEAR;
	} else {
		vk_fail( ERR_FATAL, rhiStatus_t::Error, "vk_find_sampler: invalid gl_mag_filter" );
	}

	maxLod = vk.maxLod;

	if ( def->gl_min_filter == FILTER_NEAREST ) {
		min_filter = VK_FILTER_NEAREST;
		mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		maxLod = 0.25f; // used to emulate OpenGL's FILTER_LINEAR/FILTER_NEAREST minification filter
	} else if ( def->gl_min_filter == FILTER_LINEAR ) {
		min_filter = VK_FILTER_LINEAR;
		mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		maxLod = 0.25f; // used to emulate OpenGL's FILTER_LINEAR/FILTER_NEAREST minification filter
	} else if ( def->gl_min_filter == FILTER_NEAREST_MIPMAP_NEAREST ) {
		min_filter = VK_FILTER_NEAREST;
		mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	} else if ( def->gl_min_filter == FILTER_LINEAR_MIPMAP_NEAREST ) {
		min_filter = VK_FILTER_LINEAR;
		mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	} else if ( def->gl_min_filter == FILTER_NEAREST_MIPMAP_LINEAR ) {
		min_filter = VK_FILTER_NEAREST;
		mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	} else if ( def->gl_min_filter == FILTER_LINEAR_MIPMAP_LINEAR ) {
		min_filter = VK_FILTER_LINEAR;
		mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	} else {
		vk_fail( ERR_FATAL, rhiStatus_t::Error, "vk_find_sampler: invalid gl_min_filter" );
	}

	if ( def->max_lod_1_0 ) {
		maxLod = 1.0f;
	}

	desc.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	desc.pNext = NULL;
	desc.flags = 0;
	desc.magFilter = mag_filter;
	desc.minFilter = min_filter;
	desc.mipmapMode = mipmap_mode;
	desc.addressModeU = address_mode;
	desc.addressModeV = address_mode;
	desc.addressModeW = address_mode;
	desc.mipLodBias = 0.0f;

	if ( def->noAnisotropy || mipmap_mode == VK_SAMPLER_MIPMAP_MODE_NEAREST || mag_filter == VK_FILTER_NEAREST ) {
		desc.anisotropyEnable = VK_FALSE;
		desc.maxAnisotropy = 1.0f;
	} else {
		desc.anisotropyEnable = ( vk_config.anisotropy && vk.samplerAnisotropy ) ? VK_TRUE : VK_FALSE;
		if ( desc.anisotropyEnable ) {
			desc.maxAnisotropy = MIN( vk_config.maxAnisotropy, vk.maxAnisotropy );
		}
	}

	desc.compareEnable = VK_FALSE;
	desc.compareOp = VK_COMPARE_OP_ALWAYS;
	desc.minLod = 0.0f;
	desc.maxLod = ( maxLod == vk.maxLod ) ? VK_LOD_CLAMP_NONE : maxLod;
	desc.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	desc.unnormalizedCoordinates = VK_FALSE;

	VK_CHECK( qvkCreateSampler( vk.device, &desc, NULL, &sampler ) );

	SET_OBJECT_NAME( sampler, va( "image sampler %i", vk.samplers.count ), VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT );

	vk.samplers.def[vk.samplers.count] = *def;
	vk.samplers.handle[vk.samplers.count] = sampler;
	vk.samplers.count++;

	return sampler;
}


void vk_destroy_samplers( void ) {
	int i;

	for ( i = 0; i < vk.samplers.count; i++ ) {
		qvkDestroySampler( vk.device, vk.samplers.handle[i], NULL );
		memset( &vk.samplers.def[i], 0x0, sizeof( vk.samplers.def[i] ) );
		vk.samplers.handle[i] = VK_NULL_HANDLE;
	}

	vk.samplers.count = 0;
}


void vk_update_attachment_descriptors( void ) {

	if ( vk.color_image_view ) {
		VkDescriptorImageInfo info;
		VkWriteDescriptorSet desc;
		Vk_Sampler_Def sd;

		Com_Memset( &sd, 0, sizeof( sd ) );
		sd.gl_mag_filter = sd.gl_min_filter = vk.blitFilter;
		sd.address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sd.max_lod_1_0 = qtrue;
		sd.noAnisotropy = qtrue;

		info.sampler = vk_find_sampler( &sd );
		info.imageView = vk.color_image_view;
		info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		desc.dstSet = vk.color_descriptor;
		desc.dstBinding = 0;
		desc.dstArrayElement = 0;
		desc.descriptorCount = 1;
		desc.pNext = NULL;
		desc.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		desc.pImageInfo = &info;
		desc.pBufferInfo = NULL;
		desc.pTexelBufferView = NULL;

		qvkUpdateDescriptorSets( vk.device, 1, &desc, 0, NULL );

		// screenmap
		sd.gl_mag_filter = sd.gl_min_filter = FILTER_LINEAR;
		sd.max_lod_1_0 = qfalse;
		sd.noAnisotropy = qtrue;

		info.sampler = vk_find_sampler( &sd );

		info.imageView = vk.screenMap.color_image_view;
		desc.dstSet = vk.screenMap.color_descriptor;

		qvkUpdateDescriptorSets( vk.device, 1, &desc, 0, NULL );

		// bloom images
		if ( vk_config.bloom ) {
			uint32_t i;
			for ( i = 0; i < ARRAY_LEN( vk.bloom_image_descriptor ); i++ ) {
				info.imageView = vk.bloom_image_view[i];
				desc.dstSet = vk.bloom_image_descriptor[i];

				qvkUpdateDescriptorSets( vk.device, 1, &desc, 0, NULL );
			}
		}
	}
}


static int vk_texture_filter( rhiFilter_t filter ) {
	switch ( filter ) {
	case rhiFilter_t::Nearest:
		return FILTER_NEAREST;
	case rhiFilter_t::Linear:
		return FILTER_LINEAR;
	case rhiFilter_t::NearestMipmapNearest:
		return FILTER_NEAREST_MIPMAP_NEAREST;
	case rhiFilter_t::LinearMipmapNearest:
		return FILTER_LINEAR_MIPMAP_NEAREST;
	case rhiFilter_t::NearestMipmapLinear:
		return FILTER_NEAREST_MIPMAP_LINEAR;
	case rhiFilter_t::LinearMipmapLinear:
		return FILTER_LINEAR_MIPMAP_LINEAR;
	}
	return -1;
}

rhiStatus_t vk_impl_SetTextureFilter( rhiFilter_t minimize, rhiFilter_t magnify, bool *changed ) {
	const int minFilter = vk_texture_filter( minimize );
	const int magFilter = vk_texture_filter( magnify );
	*changed = false;
	if ( minFilter < 0 || magFilter < 0 )
		return rhiStatus_t::Error;
	if ( minFilter == vk.samplers.filter_min && magFilter == vk.samplers.filter_max )
		return rhiStatus_t::Success;
	const rhiStatus_t status = RHI_WaitIdle();
	if ( status != rhiStatus_t::Success )
		return status;
	vk_destroy_samplers();
	vk_config.textureMin = minimize;
	vk_config.textureMag = magnify;
	vk.samplers.filter_min = minFilter;
	vk.samplers.filter_max = magFilter;
	vk_update_attachment_descriptors();
	*changed = true;
	return rhiStatus_t::Success;
}

void vk_impl_InitDescriptors( void ) {
	VkDescriptorSetAllocateInfo alloc;
	VkDescriptorBufferInfo info;
	VkWriteDescriptorSet desc;
	uint32_t i;

	alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc.pNext = NULL;
	alloc.descriptorPool = vk.descriptor_pool;
	alloc.descriptorSetCount = 1;
	alloc.pSetLayouts = &vk.set_layout_storage;

	VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, &vk.storage.descriptor ) );

	info.buffer = vk.storage.buffer;
	info.offset = 0;
	info.range = sizeof( uint32_t );

	desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc.dstSet = vk.storage.descriptor;
	desc.dstBinding = 0;
	desc.dstArrayElement = 0;
	desc.descriptorCount = 1;
	desc.pNext = NULL;
	desc.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
	desc.pImageInfo = NULL;
	desc.pBufferInfo = &info;
	desc.pTexelBufferView = NULL;

	qvkUpdateDescriptorSets( vk.device, 1, &desc, 0, NULL );

	// allocated and update descriptor set
	for ( i = 0; i < NUM_COMMAND_BUFFERS; i++ ) {
		alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc.pNext = NULL;
		alloc.descriptorPool = vk.descriptor_pool;
		alloc.descriptorSetCount = 1;
		alloc.pSetLayouts = &vk.set_layout_uniform;

		VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, &vk.tess[i].uniform_descriptor ) );

		vk_update_uniform_descriptor( vk.tess[i].uniform_descriptor, vk.tess[i].vertex_buffer );

		SET_OBJECT_NAME( vk.tess[i].uniform_descriptor, va( "uniform descriptor %i", i ), VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT );
	}

	if ( vk.color_image_view ) {
		alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc.pNext = NULL;
		alloc.descriptorPool = vk.descriptor_pool;
		alloc.descriptorSetCount = 1;
		alloc.pSetLayouts = &vk.set_layout_sampler;

		VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, &vk.color_descriptor ) );

		if ( vk_config.bloom ) {
			for ( i = 0; i < ARRAY_LEN( vk.bloom_image_descriptor ); i++ ) {
				VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, &vk.bloom_image_descriptor[i] ) );
			}
		}

		alloc.descriptorSetCount = 1;
		VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, &vk.screenMap.color_descriptor ) ); // screenmap

		vk_update_attachment_descriptors();
	}
}


static void vk_release_geometry_buffers( void ) {
	int i;

	for ( i = 0; i < NUM_COMMAND_BUFFERS; i++ ) {
		qvkDestroyBuffer( vk.device, vk.tess[i].vertex_buffer, NULL );
		vk.tess[i].vertex_buffer = VK_NULL_HANDLE;
	}

	qvkFreeMemory( vk.device, vk.geometry_buffer_memory, NULL );
	vk.geometry_buffer_memory = VK_NULL_HANDLE;
}


static void vk_create_geometry_buffers( VkDeviceSize size ) {
	VkMemoryRequirements vb_memory_requirements;
	VkMemoryAllocateInfo alloc_info;
	VkBufferCreateInfo desc;
	VkDeviceSize vertex_buffer_offset;
	uint32_t memory_type_bits;
	uint32_t memory_type;
	void *data;
	int i;

	desc.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	desc.pNext = NULL;
	desc.flags = 0;
	desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	desc.queueFamilyIndexCount = 0;
	desc.pQueueFamilyIndices = NULL;

	Com_Memset( &vb_memory_requirements, 0, sizeof( vb_memory_requirements ) );

	for ( i = 0; i < NUM_COMMAND_BUFFERS; i++ ) {
		desc.size = size;
		desc.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		VK_CHECK( qvkCreateBuffer( vk.device, &desc, NULL, &vk.tess[i].vertex_buffer ) );

		qvkGetBufferMemoryRequirements( vk.device, vk.tess[i].vertex_buffer, &vb_memory_requirements );
	}

	memory_type_bits = vb_memory_requirements.memoryTypeBits;
	memory_type = find_memory_type( memory_type_bits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = NULL;
	alloc_info.allocationSize = vb_memory_requirements.size * NUM_COMMAND_BUFFERS;
	alloc_info.memoryTypeIndex = memory_type;

	VK_CHECK( qvkAllocateMemory( vk.device, &alloc_info, NULL, &vk.geometry_buffer_memory ) );
	VK_CHECK( qvkMapMemory( vk.device, vk.geometry_buffer_memory, 0, VK_WHOLE_SIZE, 0, &data ) );

	vertex_buffer_offset = 0;

	for ( i = 0; i < NUM_COMMAND_BUFFERS; i++ ) {
		qvkBindBufferMemory( vk.device, vk.tess[i].vertex_buffer, vk.geometry_buffer_memory, vertex_buffer_offset );
		vk.tess[i].vertex_buffer_ptr = (byte *)data + vertex_buffer_offset;
		vk.tess[i].vertex_buffer_offset = 0;
		vertex_buffer_offset += vb_memory_requirements.size;

		SET_OBJECT_NAME( vk.tess[i].vertex_buffer, va( "geometry buffer %i", i ), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT );
	}

	SET_OBJECT_NAME( vk.geometry_buffer_memory, "geometry buffer memory", VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT );

	vk.geometry_buffer_size = vb_memory_requirements.size;

	Com_Memset( &vk.stats, 0, sizeof( vk.stats ) );
}


static void vk_create_storage_buffer( uint32_t size ) {
	VkMemoryRequirements memory_requirements;
	VkMemoryAllocateInfo alloc_info;
	VkBufferCreateInfo desc;
	uint32_t memory_type_bits;
	uint32_t memory_type;

	desc.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	desc.pNext = NULL;
	desc.flags = 0;
	desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	desc.queueFamilyIndexCount = 0;
	desc.pQueueFamilyIndices = NULL;

	Com_Memset( &memory_requirements, 0, sizeof( memory_requirements ) );

	desc.size = size;
	desc.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	VK_CHECK( qvkCreateBuffer( vk.device, &desc, NULL, &vk.storage.buffer ) );

	qvkGetBufferMemoryRequirements( vk.device, vk.storage.buffer, &memory_requirements );

	memory_type_bits = memory_requirements.memoryTypeBits;
	memory_type = find_memory_type( memory_type_bits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = NULL;
	alloc_info.allocationSize = memory_requirements.size;
	alloc_info.memoryTypeIndex = memory_type;

	VK_CHECK( qvkAllocateMemory( vk.device, &alloc_info, NULL, &vk.storage.memory ) );
	VK_CHECK( qvkMapMemory( vk.device, vk.storage.memory, 0, VK_WHOLE_SIZE, 0, (void**)&vk.storage.buffer_ptr ) );

	Com_Memset( vk.storage.buffer_ptr, 0, memory_requirements.size );

	qvkBindBufferMemory( vk.device, vk.storage.buffer, vk.storage.memory, 0 );

	SET_OBJECT_NAME( vk.storage.buffer, "storage buffer", VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT );
	SET_OBJECT_NAME( vk.storage.descriptor, "storage buffer", VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT );
	SET_OBJECT_NAME( vk.storage.memory, "storage buffer memory", VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT );
}


#ifdef USE_VBO
void vk_release_vbo( void ) {
	if ( vk.vbo.vertex_buffer )
		qvkDestroyBuffer( vk.device, vk.vbo.vertex_buffer, NULL );
	vk.vbo.vertex_buffer = VK_NULL_HANDLE;

	if ( vk.vbo.buffer_memory )
		qvkFreeMemory( vk.device, vk.vbo.buffer_memory, NULL );
	vk.vbo.buffer_memory = VK_NULL_HANDLE;
}


void vk_impl_UploadWorldGeometry( const uint8_t *vbo_data, int32_t vbo_size ) {
	VkMemoryRequirements vb_mem_reqs;
	VkMemoryAllocateInfo alloc_info;
	VkBufferCreateInfo desc;
	VkDeviceSize vertex_buffer_offset;
	VkDeviceSize allocationSize;
	uint32_t memory_type_bits;
	VkCommandBuffer command_buffer;
	VkBufferCopy copyRegion[1];
	VkDeviceSize uploadDone;

	vk_release_vbo();

	desc.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	desc.pNext = NULL;
	desc.flags = 0;
	desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	desc.queueFamilyIndexCount = 0;
	desc.pQueueFamilyIndices = NULL;

	// device-local buffer
	desc.size = vbo_size;
	desc.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	VK_CHECK( qvkCreateBuffer( vk.device, &desc, NULL, &vk.vbo.vertex_buffer ) );

	// memory requirements
	qvkGetBufferMemoryRequirements( vk.device, vk.vbo.vertex_buffer, &vb_mem_reqs );
	vertex_buffer_offset = 0;
	allocationSize = vertex_buffer_offset + vb_mem_reqs.size;
	memory_type_bits = vb_mem_reqs.memoryTypeBits;

	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = NULL;
	alloc_info.allocationSize = allocationSize;
	alloc_info.memoryTypeIndex = find_memory_type( memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	VK_CHECK( qvkAllocateMemory( vk.device, &alloc_info, NULL, &vk.vbo.buffer_memory ) );
	qvkBindBufferMemory( vk.device, vk.vbo.vertex_buffer, vk.vbo.buffer_memory, vertex_buffer_offset );

	// staging buffers

#ifdef USE_UPLOAD_QUEUE
	vk_flush_staging_buffer( qfalse );
#endif
	// utilize existing staging buffer
	uploadDone = 0;
	while ( uploadDone < (VkDeviceSize)vbo_size ) {
		VkDeviceSize uploadSize = vk.staging_buffer.size;
		if ( uploadDone + uploadSize > (VkDeviceSize)vbo_size ) {
			uploadSize = vbo_size - uploadDone;
		}
		memcpy( vk.staging_buffer.ptr + 0, vbo_data + uploadDone, uploadSize );
		command_buffer = begin_command_buffer();
		copyRegion[0].srcOffset = 0;
		copyRegion[0].dstOffset = uploadDone;
		copyRegion[0].size = uploadSize;
		qvkCmdCopyBuffer( command_buffer, vk.staging_buffer.handle, vk.vbo.vertex_buffer, 1, &copyRegion[0] );
		end_command_buffer( command_buffer, __func__ );
		uploadDone += uploadSize;
	}

	SET_OBJECT_NAME( vk.vbo.vertex_buffer, "static VBO", VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT );
	SET_OBJECT_NAME( vk.vbo.buffer_memory, "static VBO memory", VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT );
}
#endif

#include <shader_package.h>

const char *RHI_GetShaderPackageHash( void ) {
	return rhi_shader_package_hash;
}
#define SHADER_MODULE( name ) SHADER_MODULE(name,sizeof(name))

static void vk_create_shader_modules( void ) {
	int i, j, k, l;
	vk.modules.pbr_vs = SHADER_MODULE( pbr_vert_spv );
	vk.modules.pbr_fs = SHADER_MODULE( pbr_frag_spv );
	vk.modules.pbr_baked_vs = SHADER_MODULE( pbr_baked_vert_spv );
	vk.modules.pbr_baked_fs = SHADER_MODULE( pbr_baked_frag_spv );

	vk.modules.vert.gen[0][0][0][0] = SHADER_MODULE( vert_tx0 );
	vk.modules.vert.gen[0][0][0][1] = SHADER_MODULE( vert_tx0_fog );
	vk.modules.vert.gen[0][0][1][0] = SHADER_MODULE( vert_tx0_env );
	vk.modules.vert.gen[0][0][1][1] = SHADER_MODULE( vert_tx0_env_fog );

	vk.modules.vert.gen[1][0][0][0] = SHADER_MODULE( vert_tx1 );
	vk.modules.vert.gen[1][0][0][1] = SHADER_MODULE( vert_tx1_fog );
	vk.modules.vert.gen[1][0][1][0] = SHADER_MODULE( vert_tx1_env );
	vk.modules.vert.gen[1][0][1][1] = SHADER_MODULE( vert_tx1_env_fog );

	vk.modules.vert.gen[1][1][0][0] = SHADER_MODULE( vert_tx1_cl );
	vk.modules.vert.gen[1][1][0][1] = SHADER_MODULE( vert_tx1_cl_fog );
	vk.modules.vert.gen[1][1][1][0] = SHADER_MODULE( vert_tx1_cl_env );
	vk.modules.vert.gen[1][1][1][1] = SHADER_MODULE( vert_tx1_cl_env_fog );

	vk.modules.vert.gen[2][0][0][0] = SHADER_MODULE( vert_tx2 );
	vk.modules.vert.gen[2][0][0][1] = SHADER_MODULE( vert_tx2_fog );
	vk.modules.vert.gen[2][0][1][0] = SHADER_MODULE( vert_tx2_env );
	vk.modules.vert.gen[2][0][1][1] = SHADER_MODULE( vert_tx2_env_fog );

	vk.modules.vert.gen[2][1][0][0] = SHADER_MODULE( vert_tx2_cl );
	vk.modules.vert.gen[2][1][0][1] = SHADER_MODULE( vert_tx2_cl_fog );
	vk.modules.vert.gen[2][1][1][0] = SHADER_MODULE( vert_tx2_cl_env );
	vk.modules.vert.gen[2][1][1][1] = SHADER_MODULE( vert_tx2_cl_env_fog );

	for ( i = 0; i < 3; i++ ) {
		const char *tx[] = { "single", "double", "triple" };
		const char *cl[] = { "", "+cl" };
		const char *env[] = { "", "+env" };
		const char *fog[] = { "", "+fog" };
		for ( j = 0; j < 2; j++ ) {
			for ( k = 0; k < 2; k++ ) {
				for ( l = 0; l < 2; l++ ) {
					const char *s = va( "%s-texture%s%s%s vertex module", tx[i], cl[j], env[k], fog[l] );
					SET_OBJECT_NAME( vk.modules.vert.gen[i][j][k][l], s, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );
				}
			}
		}
	}

	// specialized depth-fragment shader
	vk.modules.frag.gen0_df = SHADER_MODULE( frag_tx0_df );
	SET_OBJECT_NAME( vk.modules.frag.gen0_df, "single-texture df fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );

	// fixed-color (1.0) shader modules
	vk.modules.vert.ident1[0][0][0] = SHADER_MODULE( vert_tx0_ident1 );
	vk.modules.vert.ident1[0][0][1] = SHADER_MODULE( vert_tx0_ident1_fog );
	vk.modules.vert.ident1[0][1][0] = SHADER_MODULE( vert_tx0_ident1_env );
	vk.modules.vert.ident1[0][1][1] = SHADER_MODULE( vert_tx0_ident1_env_fog );
	vk.modules.vert.ident1[1][0][0] = SHADER_MODULE( vert_tx1_ident1 );
	vk.modules.vert.ident1[1][0][1] = SHADER_MODULE( vert_tx1_ident1_fog );
	vk.modules.vert.ident1[1][1][0] = SHADER_MODULE( vert_tx1_ident1_env );
	vk.modules.vert.ident1[1][1][1] = SHADER_MODULE( vert_tx1_ident1_env_fog );
	for ( i = 0; i < 2; i++ ) {
		const char *tx[] = { "single", "double" };
		const char *env[] = { "", "+env" };
		const char *fog[] = { "", "+fog" };
		for ( j = 0; j < 2; j++ ) {
			for ( k = 0; k < 2; k++ ) {
				const char *s = va( "%s-texture identity%s%s vertex module", tx[i], env[j], fog[k] );
				SET_OBJECT_NAME( vk.modules.vert.ident1[i][j][k], s, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );
			}
		}
	}

	vk.modules.frag.ident1[0][0] = SHADER_MODULE( frag_tx0_ident1 );
	vk.modules.frag.ident1[0][1] = SHADER_MODULE( frag_tx0_ident1_fog );
	vk.modules.frag.ident1[1][0] = SHADER_MODULE( frag_tx1_ident1 );
	vk.modules.frag.ident1[1][1] = SHADER_MODULE( frag_tx1_ident1_fog );
	for ( i = 0; i < 2; i++ ) {
		const char *tx[] = { "single", "double" };
		const char *fog[] = { "", "+fog" };
		for ( j = 0; j < 2; j++ ) {
			const char *s = va( "%s-texture identity%s fragment module", tx[i], fog[j] );
			SET_OBJECT_NAME( vk.modules.frag.ident1[i][j], s, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );
		}
	}

	vk.modules.vert.fixed[0][0][0] = SHADER_MODULE( vert_tx0_fixed );
	vk.modules.vert.fixed[0][0][1] = SHADER_MODULE( vert_tx0_fixed_fog );
	vk.modules.vert.fixed[0][1][0] = SHADER_MODULE( vert_tx0_fixed_env );
	vk.modules.vert.fixed[0][1][1] = SHADER_MODULE( vert_tx0_fixed_env_fog );
	vk.modules.vert.fixed[1][0][0] = SHADER_MODULE( vert_tx1_fixed );
	vk.modules.vert.fixed[1][0][1] = SHADER_MODULE( vert_tx1_fixed_fog );
	vk.modules.vert.fixed[1][1][0] = SHADER_MODULE( vert_tx1_fixed_env );
	vk.modules.vert.fixed[1][1][1] = SHADER_MODULE( vert_tx1_fixed_env_fog );
	for ( i = 0; i < 2; i++ ) {
		const char *tx[] = { "single", "double" };
		const char *env[] = { "", "+env" };
		const char *fog[] = { "", "+fog" };
		for ( j = 0; j < 2; j++ ) {
			for ( k = 0; k < 2; k++ ) {
				const char *s = va( "%s-texture fixed-color%s%s vertex module", tx[i], env[j], fog[k] );
				SET_OBJECT_NAME( vk.modules.vert.fixed[i][j][k], s, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );
			}
		}
	}

	vk.modules.frag.fixed[0][0] = SHADER_MODULE( frag_tx0_fixed );
	vk.modules.frag.fixed[0][1] = SHADER_MODULE( frag_tx0_fixed_fog );
	vk.modules.frag.fixed[1][0] = SHADER_MODULE( frag_tx1_fixed );
	vk.modules.frag.fixed[1][1] = SHADER_MODULE( frag_tx1_fixed_fog );
	for ( i = 0; i < 2; i++ ) {
		const char *tx[] = { "single", "double" };
		const char *fog[] = { "", "+fog" };
		for ( j = 0; j < 2; j++ ) {
			const char *s = va( "%s-texture fixed-color%s fragment module", tx[i], fog[j] );
			SET_OBJECT_NAME( vk.modules.frag.fixed[i][j], s, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );
		}
	}

	vk.modules.frag.ent[0][0] = SHADER_MODULE( frag_tx0_ent );
	vk.modules.frag.ent[0][1] = SHADER_MODULE( frag_tx0_ent_fog );
	//vk.modules.frag.ent[1][0] = SHADER_MODULE( frag_tx1_ent );
	//vk.modules.frag.ent[1][1] = SHADER_MODULE( frag_tx1_ent_fog );
	for ( i = 0; i < 1; i++ ) {
		const char *tx[] = { "single" /*, "double" */ };
		const char *fog[] = { "", "+fog" };
		for ( j = 0; j < 2; j++ ) {
			const char *s = va( "%s-texture entity-color%s fragment module", tx[i], fog[j] );
			SET_OBJECT_NAME( vk.modules.frag.ent[i][j], s, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );
		}
	}

	vk.modules.frag.gen[0][0][0] = SHADER_MODULE( frag_tx0 );
	vk.modules.frag.gen[0][0][1] = SHADER_MODULE( frag_tx0_fog );

	vk.modules.frag.gen[1][0][0] = SHADER_MODULE( frag_tx1 );
	vk.modules.frag.gen[1][0][1] = SHADER_MODULE( frag_tx1_fog );

	vk.modules.frag.gen[1][1][0] = SHADER_MODULE( frag_tx1_cl );
	vk.modules.frag.gen[1][1][1] = SHADER_MODULE( frag_tx1_cl_fog );

	vk.modules.frag.gen[2][0][0] = SHADER_MODULE( frag_tx2 );
	vk.modules.frag.gen[2][0][1] = SHADER_MODULE( frag_tx2_fog );

	vk.modules.frag.gen[2][1][0] = SHADER_MODULE( frag_tx2_cl );
	vk.modules.frag.gen[2][1][1] = SHADER_MODULE( frag_tx2_cl_fog );

	for ( i = 0; i < 3; i++ ) {
		const char *tx[] = { "single", "double", "triple" };
		const char *cl[] = { "", "+cl" };
		const char *fog[] = { "", "+fog" };
		for ( j = 0; j < 2; j++ ) {
			for ( k = 0; k < 2; k++ ) {
				const char *s = va( "%s-texture%s%s fragment module", tx[i], cl[j], fog[k] );
				SET_OBJECT_NAME( vk.modules.frag.gen[i][j][k], s, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );
			}
		}
	}


	vk.modules.vert.light[0] = SHADER_MODULE( vert_light );
	vk.modules.vert.light[1] = SHADER_MODULE( vert_light_fog );
	SET_OBJECT_NAME( vk.modules.vert.light[0], "light vertex module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );
	SET_OBJECT_NAME( vk.modules.vert.light[1], "light fog vertex module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );

	vk.modules.frag.light[0][0] = SHADER_MODULE( frag_light );
	vk.modules.frag.light[0][1] = SHADER_MODULE( frag_light_fog );
	vk.modules.frag.light[1][0] = SHADER_MODULE( frag_light_line );
	vk.modules.frag.light[1][1] = SHADER_MODULE( frag_light_line_fog );
	SET_OBJECT_NAME( vk.modules.frag.light[0][0], "light fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );
	SET_OBJECT_NAME( vk.modules.frag.light[0][1], "light fog fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );
	SET_OBJECT_NAME( vk.modules.frag.light[1][0], "linear light fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );
	SET_OBJECT_NAME( vk.modules.frag.light[1][1], "linear light fog fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );

	vk.modules.color_fs = SHADER_MODULE( color_frag_spv );
	vk.modules.color_vs = SHADER_MODULE( color_vert_spv );

	SET_OBJECT_NAME( vk.modules.color_vs, "single-color vertex module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );
	SET_OBJECT_NAME( vk.modules.color_fs, "single-color fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );

	vk.modules.fog_vs = SHADER_MODULE( fog_vert_spv );
	vk.modules.fog_fs = SHADER_MODULE( fog_frag_spv );

	SET_OBJECT_NAME( vk.modules.fog_vs, "fog-only vertex module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );
	SET_OBJECT_NAME( vk.modules.fog_fs, "fog-only fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );

	vk.modules.dot_vs = SHADER_MODULE( dot_vert_spv );
	vk.modules.dot_fs = SHADER_MODULE( dot_frag_spv );

	SET_OBJECT_NAME( vk.modules.dot_vs, "dot vertex module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );
	SET_OBJECT_NAME( vk.modules.dot_fs, "dot fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );

	vk.modules.bloom_fs = SHADER_MODULE( bloom_frag_spv );
	vk.modules.blur_fs = SHADER_MODULE( blur_frag_spv );
	vk.modules.blend_fs = SHADER_MODULE( blend_frag_spv );

	SET_OBJECT_NAME( vk.modules.bloom_fs, "bloom extraction fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );
	SET_OBJECT_NAME( vk.modules.blur_fs, "gaussian blur fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );
	SET_OBJECT_NAME( vk.modules.blend_fs, "final bloom blend fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );

	vk.modules.gamma_fs = SHADER_MODULE( gamma_frag_spv );
	vk.modules.gamma_vs = SHADER_MODULE( gamma_vert_spv );

	SET_OBJECT_NAME( vk.modules.gamma_fs, "gamma post-processing fragment module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );
	SET_OBJECT_NAME( vk.modules.gamma_vs, "gamma post-processing vertex module", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );
}


void vk_create_blur_pipeline( uint32_t index, uint32_t width, uint32_t height, qboolean horizontal_pass );

void vk_impl_UpdatePostProcess( int32_t overbrightBits ) {
	vk.overbrightBits = overbrightBits;
	vk_update_post_process_pipelines();
}

void vk_update_post_process_pipelines( void ) {
	if ( vk.fboActive ) {
		// update gamma shader
		vk_create_post_process_pipeline( 0, 0, 0 );
		if ( vk.capture.image ) {
			// update capture pipeline
			vk_create_post_process_pipeline( 3, vk_config.captureWidth, vk_config.captureHeight );
		}
		if ( vk_config.bloom ) {
			// update bloom shaders
			uint32_t width = vk_config.captureWidth;
			uint32_t height = vk_config.captureHeight;
			uint32_t i;

			vk_create_post_process_pipeline( 1, width, height ); // bloom extraction

			for ( i = 0; i < ARRAY_LEN( vk.blur_pipeline ); i += 2 ) {
				width /= 2;
				height /= 2;
				vk_create_blur_pipeline( i + 0, width, height, qtrue ); // horizontal
				vk_create_blur_pipeline( i + 1, width, height, qfalse ); // vertical
			}

			vk_create_post_process_pipeline( 2, vk_config.renderWidth, vk_config.renderHeight ); // bloom blending
		}
	}
}


typedef struct vk_attach_desc_s {
	VkImage descriptor;
	VkImageView *image_view;
	VkImageUsageFlags usage;
	VkMemoryRequirements reqs;
	uint32_t memoryTypeIndex;
	VkDeviceSize memory_offset;
	// for layout transition:
	VkImageAspectFlags aspect_flags;
	VkImageLayout image_layout;
	VkFormat image_format;
} vk_attach_desc_t;

static vk_attach_desc_t attachments[MAX_ATTACHMENTS_IN_POOL];
static uint32_t num_attachments = 0;


static void vk_clear_attachment_pool( void ) {
	num_attachments = 0;
}


static void vk_alloc_attachments( void ) {
	VkImageViewCreateInfo view_desc;
	VkMemoryDedicatedAllocateInfoKHR alloc_info2;
	VkMemoryAllocateInfo alloc_info;
	VkCommandBuffer command_buffer;
	VkDeviceMemory memory;
	VkDeviceSize offset;
	uint32_t memoryTypeBits;
	uint32_t memoryTypeIndex;
	uint32_t i;

	if ( num_attachments == 0 ) {
		return;
	}

	if ( vk.image_memory_count >= ARRAY_LEN( vk.image_memory ) ) {
		vk_fail( ERR_DROP, rhiStatus_t::Error, "vk.image_memory_count == %i", (int)ARRAY_LEN( vk.image_memory ) );
	}

	memoryTypeBits = ~0U;
	offset = 0;

	for ( i = 0; i < num_attachments; i++ ) {
#ifdef MIN_IMAGE_ALIGN
		VkDeviceSize alignment = MAX( attachments[i].reqs.alignment, MIN_IMAGE_ALIGN );
#else
		VkDeviceSize alignment = attachments[i].reqs.alignment;
#endif
		memoryTypeBits &= attachments[i].reqs.memoryTypeBits;
		offset = PAD( offset, alignment );
		attachments[i].memory_offset = offset;
		offset += attachments[i].reqs.size;
#ifdef _DEBUG
		vk_host.Print( rhiLog_t::Info, S_COLOR_CYAN "[%i] type %i, size %i, align %i\n", i,
			attachments[i].reqs.memoryTypeBits,
			(int)attachments[i].reqs.size,
			(int)attachments[i].reqs.alignment );
#endif
	}

	if ( num_attachments == 1 && attachments[0].usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT ) {
		// try lazy memory
		memoryTypeIndex = find_memory_type2( memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, NULL );
		if ( memoryTypeIndex == ~0U ) {
			memoryTypeIndex = find_memory_type( memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
		}
	} else {
		memoryTypeIndex = find_memory_type( memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	}

#ifdef _DEBUG
	vk_host.Print( rhiLog_t::Info, "memory type bits: %04x\n", memoryTypeBits );
	vk_host.Print( rhiLog_t::Info, "memory type index: %04x\n", memoryTypeIndex );
	vk_host.Print( rhiLog_t::Info, "total size: %i\n", (int)offset );
#endif

	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = NULL;
	alloc_info.allocationSize = offset;
	alloc_info.memoryTypeIndex = memoryTypeIndex;

	if ( num_attachments == 1 ) {
		if ( vk.dedicatedAllocation ) {
			Com_Memset( &alloc_info2, 0, sizeof( alloc_info2 ) );
			alloc_info2.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR;
			alloc_info2.image = attachments[0].descriptor;
			alloc_info.pNext = &alloc_info2;
		}
	}

	// allocate and bind memory
	VK_CHECK( qvkAllocateMemory( vk.device, &alloc_info, NULL, &memory ) );

	vk.image_memory[vk.image_memory_count++] = memory;

	for ( i = 0; i < num_attachments; i++ ) {

		VK_CHECK( qvkBindImageMemory( vk.device, attachments[i].descriptor, memory, attachments[i].memory_offset ) );

		// create color image view
		view_desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_desc.pNext = NULL;
		view_desc.flags = 0;
		view_desc.image = attachments[i].descriptor;
		view_desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view_desc.format = attachments[i].image_format;
		view_desc.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_desc.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_desc.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_desc.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_desc.subresourceRange.aspectMask = attachments[i].aspect_flags;
		view_desc.subresourceRange.baseMipLevel = 0;
		view_desc.subresourceRange.levelCount = 1;
		view_desc.subresourceRange.baseArrayLayer = 0;
		view_desc.subresourceRange.layerCount = 1;

		VK_CHECK( qvkCreateImageView( vk.device, &view_desc, NULL, attachments[ i ].image_view ) );
	}

	// perform layout transition
	command_buffer = begin_command_buffer();
	for ( i = 0; i < num_attachments; i++ ) {
		record_image_layout_transition( command_buffer,
			attachments[i].descriptor,
			attachments[i].aspect_flags,
			VK_IMAGE_LAYOUT_UNDEFINED, // old_layout
			attachments[i].image_layout,
			0, 0 );
	}
	end_command_buffer( command_buffer, __func__ );

	num_attachments = 0;
}


static void vk_add_attachment_desc( VkImage desc, VkImageView *image_view, VkImageUsageFlags usage, VkMemoryRequirements *reqs, VkFormat image_format, VkImageAspectFlags aspect_flags, VkImageLayout image_layout ) {
	if ( num_attachments >= ARRAY_LEN( attachments ) ) {
		vk_fail( ERR_FATAL, rhiStatus_t::Error, "Attachments array overflow" );
	} else {
		attachments[num_attachments].descriptor = desc;
		attachments[num_attachments].image_view = image_view;
		attachments[num_attachments].usage = usage;
		attachments[num_attachments].reqs = *reqs;
		attachments[num_attachments].aspect_flags = aspect_flags;
		attachments[num_attachments].image_layout = image_layout;
		attachments[num_attachments].image_format = image_format;
		attachments[num_attachments].memory_offset = 0;
		num_attachments++;
	}
}


static void vk_get_image_memory_erquirements( VkImage image, VkMemoryRequirements *memory_requirements ) {
	if ( vk.dedicatedAllocation ) {
		VkMemoryRequirements2KHR memory_requirements2;
		VkImageMemoryRequirementsInfo2KHR image_requirements2;
		VkMemoryDedicatedRequirementsKHR mem_req2;

		Com_Memset( &mem_req2, 0, sizeof( mem_req2 ) );
		mem_req2.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR;

		image_requirements2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2_KHR;
		image_requirements2.image = image;
		image_requirements2.pNext = NULL;

		Com_Memset( &memory_requirements2, 0, sizeof( memory_requirements2 ) );
		memory_requirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR;
		memory_requirements2.pNext = &mem_req2;

		qvkGetImageMemoryRequirements2KHR( vk.device, &image_requirements2, &memory_requirements2 );

		*memory_requirements = memory_requirements2.memoryRequirements;
	} else {
		qvkGetImageMemoryRequirements( vk.device, image, memory_requirements );
	}
}


static void create_color_attachment( uint32_t width, uint32_t height, VkSampleCountFlagBits samples, VkFormat format,
	VkImageUsageFlags usage, VkImage *image, VkImageView *image_view, VkImageLayout image_layout, qboolean multisample ) {
	VkImageCreateInfo create_desc;
	VkMemoryRequirements memory_requirements;

	if ( multisample && !( usage & VK_IMAGE_USAGE_SAMPLED_BIT ) )
		usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;

	// create color image
	create_desc.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	create_desc.pNext = NULL;
	create_desc.flags = 0;
	create_desc.imageType = VK_IMAGE_TYPE_2D;
	create_desc.format = format;
	create_desc.extent.width = width;
	create_desc.extent.height = height;
	create_desc.extent.depth = 1;
	create_desc.mipLevels = 1;
	create_desc.arrayLayers = 1;
	create_desc.samples = samples;
	create_desc.tiling = VK_IMAGE_TILING_OPTIMAL;
	create_desc.usage = usage;
	create_desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_desc.queueFamilyIndexCount = 0;
	create_desc.pQueueFamilyIndices = NULL;
	create_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VK_CHECK( qvkCreateImage( vk.device, &create_desc, NULL, image ) );

	vk_get_image_memory_erquirements( *image, &memory_requirements );

	vk_add_attachment_desc( *image, image_view, usage, &memory_requirements, format, VK_IMAGE_ASPECT_COLOR_BIT, image_layout );
}


static void create_depth_attachment( uint32_t width, uint32_t height, VkSampleCountFlagBits samples, VkImage *image, VkImageView *image_view, qboolean allowTransient ) {
	VkImageCreateInfo create_desc;
	VkMemoryRequirements memory_requirements;
	VkImageAspectFlags image_aspect_flags;

	// create depth image
	create_desc.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	create_desc.pNext = NULL;
	create_desc.flags = 0;
	create_desc.imageType = VK_IMAGE_TYPE_2D;
	create_desc.format = vk.depth_format;
	create_desc.extent.width = width;
	create_desc.extent.height = height;
	create_desc.extent.depth = 1;
	create_desc.mipLevels = 1;
	create_desc.arrayLayers = 1;
	create_desc.samples = samples;
	create_desc.tiling = VK_IMAGE_TILING_OPTIMAL;
	create_desc.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if ( allowTransient ) {
		create_desc.usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	}
	create_desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_desc.queueFamilyIndexCount = 0;
	create_desc.pQueueFamilyIndices = NULL;
	create_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	image_aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
	if ( vk_config.stencilBits > 0 )
		image_aspect_flags |= VK_IMAGE_ASPECT_STENCIL_BIT;

	VK_CHECK( qvkCreateImage( vk.device, &create_desc, NULL, image ) );

	vk_get_image_memory_erquirements( *image, &memory_requirements );

	vk_add_attachment_desc( *image, image_view, create_desc.usage, &memory_requirements, vk.depth_format, image_aspect_flags, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
}


// Native handles retain the existing allocator and teardown ownership.
static void vk_graph_target( rhiGraphTarget_t target, VkImage **image, VkImageView **view ) {
	using T = rhiGraphTarget_t;
	switch ( target ) {
	case T::MainColor:
		*image = &vk.color_image;
		*view = &vk.color_image_view;
		break;
	case T::MainDepth:
		*image = &vk.depth_image;
		*view = &vk.depth_image_view;
		break;
	case T::MainMsaa:
		*image = &vk.msaa_image;
		*view = &vk.msaa_image_view;
		break;
	case T::ScreenColor:
		*image = &vk.screenMap.color_image;
		*view = &vk.screenMap.color_image_view;
		break;
	case T::ScreenDepth:
		*image = &vk.screenMap.depth_image;
		*view = &vk.screenMap.depth_image_view;
		break;
	case T::ScreenMsaa:
		*image = &vk.screenMap.color_image_msaa;
		*view = &vk.screenMap.color_image_view_msaa;
		break;
	case T::Capture:
		*image = &vk.capture.image;
		*view = &vk.capture.image_view;
		break;
	default:
		*image = &vk.bloom_image[(uint32_t)target];
		*view = &vk.bloom_image_view[(uint32_t)target];
		break;
	}
}

static void vk_create_attachments( void ) {
	uint32_t i;
	const rhiGraphConfig_t config = {
		(uint32_t)vk_config.renderWidth, (uint32_t)vk_config.renderHeight,
		(uint32_t)vk_config.windowWidth, (uint32_t)vk_config.windowHeight,
		(uint32_t)vk_config.captureWidth, (uint32_t)vk_config.captureHeight,
		(uint32_t)vk.screenMapWidth, (uint32_t)vk.screenMapHeight,
		(uint32_t)vkSamples, (uint32_t)vk.screenMapSamples,
		vk.fboActive != qfalse, vk_config.bloom != 0, vk_config.supersample != 0, vk_config.stencilBits != 0
	};
	if ( !RHI_CompileGraph( &config, &vk_graph ) )
		vk_fail( ERR_FATAL, rhiStatus_t::Error, "Vulkan: invalid render graph dimensions" );
	vk_clear_attachment_pool();
	// Preserve native allocation order: grouping similar images affects memory packing.
	for ( i = 0; i < vk_graph.targetCount; ++i ) {
		const rhiGraphTarget_t id = vk_graph.targetOrder[i];
		const rhiGraphTargetDesc_t &target = vk_graph.targets[(uint32_t)id];
		VkImage *image;
		VkImageView *view;
		vk_graph_target( id, &image, &view );
		if ( target.usage & RHI_GRAPH_DEPTH ) {
			create_depth_attachment( target.width, target.height, (VkSampleCountFlagBits)target.samples,
				image, view, target.transient ? qtrue : qfalse );
		} else {
			const VkImageUsageFlags usage = ( target.usage & RHI_GRAPH_COLOR ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 0 ) |
											( target.usage & RHI_GRAPH_SAMPLED ? VK_IMAGE_USAGE_SAMPLED_BIT : 0 ) |
											( target.usage & RHI_GRAPH_TRANSFER_SOURCE ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0 );
			create_color_attachment( target.width, target.height, (VkSampleCountFlagBits)target.samples,
				vk_graph_format( target.format ), usage, image, view, vk_graph_layout( target.initialLayout ), target.transient ? qtrue : qfalse );
		}
	}

	vk_alloc_attachments();

	for ( i = 0; i < vk.image_memory_count; i++ ) {
		SET_OBJECT_NAME( vk.image_memory[i], va( "framebuffer memory chunk %i", i ), VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT );
	}

	SET_OBJECT_NAME( vk.depth_image, "depth attachment", VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT );
	SET_OBJECT_NAME( vk.depth_image_view, "depth attachment", VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT );

	SET_OBJECT_NAME( vk.color_image, "color attachment", VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT );
	SET_OBJECT_NAME( vk.color_image_view, "color attachment", VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT );

	SET_OBJECT_NAME( vk.capture.image, "capture image", VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT );
	SET_OBJECT_NAME( vk.capture.image_view, "capture image view", VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT );

	for ( i = 0; i < ARRAY_LEN( vk.bloom_image ); i++ ) {
		SET_OBJECT_NAME( vk.bloom_image[i], va( "bloom attachment %i", i ), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT );
		SET_OBJECT_NAME( vk.bloom_image_view[i], va( "bloom attachment %i", i ), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT );
	}
}


static void vk_graph_framebuffer( rhiGraphPass_t id, uint32_t swapchainIndex, VkFramebuffer *framebuffer ) {
	const rhiGraphPassDesc_t &pass = vk_graph.passes[(uint32_t)id];
	VkImageView views[3];
	for ( uint32_t i = 0; i < pass.attachmentCount; ++i ) {
		const rhiGraphTarget_t target = pass.attachments[i].target;
		if ( target == rhiGraphTarget_t::Present ) {
			views[i] = vk.swapchain_image_views[swapchainIndex];
		} else {
			VkImage *image;
			VkImageView *view;
			vk_graph_target( target, &image, &view );
			views[i] = *view;
		}
	}
	VkFramebufferCreateInfo desc = {};
	desc.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	// Each legacy blur pair shares the first (compatible) native pass at creation.
	const uint32_t index = (uint32_t)id - (uint32_t)rhiGraphPass_t::Blur0;
	const rhiGraphPass_t compatible = index < RHI_GRAPH_BLOOM_PASSES * 2 ? (rhiGraphPass_t)( (uint32_t)id - index % 2 ) : id;
	desc.renderPass = *vk_graph_pass( compatible );
	desc.attachmentCount = pass.attachmentCount;
	desc.pAttachments = views;
	desc.width = pass.width;
	desc.height = pass.height;
	desc.layers = 1;
	VK_CHECK( qvkCreateFramebuffer( vk.device, &desc, NULL, framebuffer ) );
}

static void vk_create_framebuffers( void ) {
	using P = rhiGraphPass_t;
	// Keep native creation order and the shared offscreen main/post-bloom framebuffer.
	for ( uint32_t n = 0; n < vk.swapchain_image_count; ++n ) {
		if ( !vk_graph.passes[(uint32_t)P::Gamma].enabled || n == 0 ) {
			vk_graph_framebuffer( P::Main, n, &vk.framebuffers.main[n] );
			SET_OBJECT_NAME( vk.framebuffers.main[n], va( "framebuffer - main %u", n ), VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT );
		} else {
			vk.framebuffers.main[n] = vk.framebuffers.main[0];
		}
		if ( vk_graph.passes[(uint32_t)P::Gamma].enabled ) {
			vk_graph_framebuffer( P::Gamma, n, &vk.framebuffers.gamma[n] );
			SET_OBJECT_NAME( vk.framebuffers.gamma[n], "framebuffer - gamma-correction", VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT );
		}
	}
	if ( vk_graph.passes[(uint32_t)P::ScreenMap].enabled ) {
		vk_graph_framebuffer( P::ScreenMap, 0, &vk.framebuffers.screenmap );
		SET_OBJECT_NAME( vk.framebuffers.screenmap, "framebuffer - screenmap", VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT );
	}
	if ( vk_graph.passes[(uint32_t)P::Capture].enabled ) {
		vk_graph_framebuffer( P::Capture, 0, &vk.framebuffers.capture );
		SET_OBJECT_NAME( vk.framebuffers.capture, "framebuffer - capture", VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT );
	}
	if ( vk_graph.passes[(uint32_t)P::BloomExtract].enabled ) {
		vk_graph_framebuffer( P::BloomExtract, 0, &vk.framebuffers.bloom_extract );
		SET_OBJECT_NAME( vk.framebuffers.bloom_extract, "framebuffer - bloom extraction", VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT );
		for ( uint32_t n = 0; n < RHI_GRAPH_BLOOM_PASSES * 2; ++n ) {
			vk_graph_framebuffer( (P)( (uint32_t)P::Blur0 + n ), 0, &vk.framebuffers.blur[n] );
			SET_OBJECT_NAME( vk.framebuffers.blur[n], va( "framebuffer - blur %u", n ), VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT );
		}
	}
}


static uint32_t vk_timestamp_base( void ) {
	return (uint32_t)( vk.cmd - vk.tess ) * RHI_MAX_TIMINGS * 2;
}

uint32_t RHI_BeginScope( const char *name ) {
	if ( !vk.timestampPool || !vk.cmd || !name || vk.cmd->profile.count >= RHI_MAX_TIMINGS )
		return RHI_INVALID_OFFSET;
	const uint32_t scope = vk.cmd->profile.count++;
	Q_strncpyz( vk.cmd->profile.names[scope], name, sizeof( vk.cmd->profile.names[scope] ) );
	vk.cmd->profile.ended[scope] = false;
	qvkCmdWriteTimestamp( vk.cmd->command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, vk.timestampPool, vk_timestamp_base() + scope * 2 );
	return scope;
}

void RHI_EndScope( uint32_t scope ) {
	if ( !vk.timestampPool || !vk.cmd || scope >= vk.cmd->profile.count || vk.cmd->profile.ended[scope] )
		return;
	qvkCmdWriteTimestamp( vk.cmd->command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vk.timestampPool, vk_timestamp_base() + scope * 2 + 1 );
	vk.cmd->profile.ended[scope] = true;
}

uint32_t RHI_GetTimings( const rhiTiming_t **timings ) {
	*timings = vk.timings;
	return vk.timingCount;
}

// Called only after the existing frame fence succeeds, before recording its reset.
static void vk_read_timings( void ) {
	uint64_t results[RHI_MAX_TIMINGS * 2][2]; // timestamp, availability
	vk.timingCount = 0;
	if ( !vk.timestampPool || !vk.cmd->profile.count )
		return;
	const VkResult result = qvkGetQueryPoolResults( vk.device, vk.timestampPool,
		vk_timestamp_base(), vk.cmd->profile.count * 2, sizeof( results ), results,
		sizeof( results[0] ), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT );
	if ( result != VK_SUCCESS && result != VK_NOT_READY )
		return;
	for ( uint32_t i = 0; i < vk.cmd->profile.count; i++ ) {
		if ( !vk.cmd->profile.ended[i] || !results[i * 2][1] || !results[i * 2 + 1][1] )
			continue;
		uint64_t ticks = results[i * 2 + 1][0] - results[i * 2][0];
		if ( vk.timestampBits < 64 )
			ticks &= ( UINT64_C( 1 ) << vk.timestampBits ) - 1;
		rhiTiming_t *timing = &vk.timings[vk.timingCount++];
		Com_Memcpy( timing->name, vk.cmd->profile.names[i], sizeof( timing->name ) );
		timing->microseconds = (double)ticks * vk.timestampPeriod / 1000.0;
	}
}

static void vk_create_sync_primitives( void ) {
	VkSemaphoreCreateInfo desc;
	VkFenceCreateInfo fence_desc;
	uint32_t i;

	desc.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	desc.pNext = NULL;
	desc.flags = 0;

#ifdef USE_UPLOAD_QUEUE
	VK_CHECK( qvkCreateSemaphore( vk.device, &desc, NULL, &vk.image_uploaded2 ) );
#endif

	vk.timingCount = 0;
	if ( vk.timestampBits && vk.timestampPeriod > 0.0f ) {
		VkQueryPoolCreateInfo queries = {};
		queries.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		queries.queryType = VK_QUERY_TYPE_TIMESTAMP;
		queries.queryCount = NUM_COMMAND_BUFFERS * RHI_MAX_TIMINGS * 2;
		VK_CHECK( qvkCreateQueryPool( vk.device, &queries, NULL, &vk.timestampPool ) );
		SET_OBJECT_NAME( vk.timestampPool, "frame timestamps", VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT );
	}

	// all commands submitted
	for ( i = 0; i < NUM_COMMAND_BUFFERS; i++ ) {
		desc.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		desc.pNext = NULL;
		desc.flags = 0;

		// swapchain image acquired
		VK_CHECK( qvkCreateSemaphore( vk.device, &desc, NULL, &vk.tess[i].image_acquired ) );

#ifdef USE_UPLOAD_QUEUE
		// second semaphore to synchronize additional tasks (e.g. image upload)
		VK_CHECK( qvkCreateSemaphore( vk.device, &desc, NULL, &vk.tess[i].rendering_finished2 ) );
#endif
		fence_desc.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_desc.pNext = NULL;
		//fence_desc.flags = VK_FENCE_CREATE_SIGNALED_BIT; // so it can be used to start rendering
		fence_desc.flags = 0; // non-signalled state

		VK_CHECK( qvkCreateFence( vk.device, &fence_desc, NULL, &vk.tess[i].rendering_finished_fence ) );
		vk.tess[i].waitForFence = qfalse;
		vk.tess[i].profile.count = 0;
		vk.tess[i].profile.passScope = RHI_INVALID_OFFSET;

		SET_OBJECT_NAME( vk.tess[i].image_acquired, va( "image_acquired semaphore %i", i ), VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT );
#ifdef USE_UPLOAD_QUEUE
		SET_OBJECT_NAME( vk.tess[i].rendering_finished2, va( "rendering_finished2 semaphore %i", i ), VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT );
#endif
		SET_OBJECT_NAME( vk.tess[i].rendering_finished_fence, va( "rendering_finished fence %i", i ), VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT );
	}

	fence_desc.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_desc.pNext = NULL;
	fence_desc.flags = 0;

#ifdef USE_UPLOAD_QUEUE
	VK_CHECK( qvkCreateFence( vk.device, &fence_desc, NULL, &vk.aux_fence ) );
	SET_OBJECT_NAME( vk.aux_fence, "aux fence", VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT );

	vk.rendering_finished = VK_NULL_HANDLE;
	vk.image_uploaded = VK_NULL_HANDLE;
	vk.aux_fence_wait = qfalse;
#endif
}


static void vk_destroy_sync_primitives( void ) {
	uint32_t i;
	if ( vk.timestampPool ) {
		qvkDestroyQueryPool( vk.device, vk.timestampPool, NULL );
		vk.timestampPool = VK_NULL_HANDLE;
	}
	vk.timingCount = 0;

#ifdef USE_UPLOAD_QUEUE
	qvkDestroySemaphore( vk.device, vk.image_uploaded2, NULL );
#endif

	for ( i = 0; i < NUM_COMMAND_BUFFERS; i++ ) {
		qvkDestroySemaphore( vk.device, vk.tess[i].image_acquired, NULL );
#ifdef USE_UPLOAD_QUEUE
		qvkDestroySemaphore( vk.device, vk.tess[i].rendering_finished2, NULL );
#endif
		qvkDestroyFence( vk.device, vk.tess[i].rendering_finished_fence, NULL );
		vk.tess[i].waitForFence = qfalse;
		vk.tess[i].swapchain_image_acquired = qfalse;
	}

#ifdef USE_UPLOAD_QUEUE
	qvkDestroyFence( vk.device, vk.aux_fence, NULL );

	vk.rendering_finished = VK_NULL_HANDLE;
	vk.image_uploaded = VK_NULL_HANDLE;
#endif
}


static void vk_destroy_framebuffers( void ) {
	uint32_t n;

	for ( n = 0; n < vk.swapchain_image_count; n++ ) {
		if ( vk.framebuffers.main[n] != VK_NULL_HANDLE ) {
			if ( !vk.fboActive || n == 0 ) {
				qvkDestroyFramebuffer( vk.device, vk.framebuffers.main[n], NULL );
			}
			vk.framebuffers.main[n] = VK_NULL_HANDLE;
		}
		if ( vk.framebuffers.gamma[n] != VK_NULL_HANDLE ) {
			qvkDestroyFramebuffer( vk.device, vk.framebuffers.gamma[n], NULL );
			vk.framebuffers.gamma[n] = VK_NULL_HANDLE;
		}
	}

	if ( vk.framebuffers.bloom_extract != VK_NULL_HANDLE ) {
		qvkDestroyFramebuffer( vk.device, vk.framebuffers.bloom_extract, NULL );
		vk.framebuffers.bloom_extract = VK_NULL_HANDLE;
	}

	if ( vk.framebuffers.screenmap != VK_NULL_HANDLE ) {
		qvkDestroyFramebuffer( vk.device, vk.framebuffers.screenmap, NULL );
		vk.framebuffers.screenmap = VK_NULL_HANDLE;
	}

	if ( vk.framebuffers.capture != VK_NULL_HANDLE ) {
		qvkDestroyFramebuffer( vk.device, vk.framebuffers.capture, NULL );
		vk.framebuffers.capture = VK_NULL_HANDLE;
	}

	for ( n = 0; n < ARRAY_LEN( vk.framebuffers.blur ); n++ ) {
		if ( vk.framebuffers.blur[n] != VK_NULL_HANDLE ) {
			qvkDestroyFramebuffer( vk.device, vk.framebuffers.blur[n], NULL );
			vk.framebuffers.blur[n] = VK_NULL_HANDLE;
		}
	}
}


static void vk_destroy_swapchain( void ) {
	uint32_t i;

	for ( i = 0; i < vk.swapchain_image_count; i++ ) {
		if ( vk.swapchain_image_views[i] != VK_NULL_HANDLE ) {
			qvkDestroyImageView( vk.device, vk.swapchain_image_views[i], NULL );
			vk.swapchain_image_views[i] = VK_NULL_HANDLE;
		}
		if ( vk.swapchain_rendering_finished[i] != VK_NULL_HANDLE ) {
			qvkDestroySemaphore( vk.device, vk.swapchain_rendering_finished[i], NULL );
			vk.swapchain_rendering_finished[i] = VK_NULL_HANDLE;
		}
	}

	qvkDestroySwapchainKHR( vk.device, vk.swapchain, NULL );
}

static void vk_destroy_attachments( void );
static void vk_destroy_render_passes( void );
static void vk_destroy_pipelines( qboolean resetCount );

static void vk_restart_swapchain( const char *funcname, VkResult res [[maybe_unused]] ) {
	uint32_t i;

#ifdef _DEBUG
	vk_host.Print( rhiLog_t::Warning, "%s(%s): restarting swapchain...\n", funcname, vk_result_string( res ) );
#else
	vk_host.Print( rhiLog_t::Warning, "%s(): restarting swapchain...\n", funcname );
#endif

	vk_wait_idle();

	for ( i = 0; i < NUM_COMMAND_BUFFERS; i++ ) {
		qvkResetCommandBuffer( vk.tess[i].command_buffer, 0 );
	}

#ifdef USE_UPLOAD_QUEUE
	qvkResetCommandBuffer( vk.staging_command_buffer, 0 );
#endif

	vk_destroy_pipelines( qfalse );
	vk_destroy_framebuffers();
	vk_destroy_render_passes();
	vk_destroy_attachments();
	vk_destroy_swapchain();
	vk_destroy_sync_primitives();

	vk_select_surface_format( vk.physical_device, vk_surface );
	setup_surface_formats( vk.physical_device );

	vk_create_sync_primitives();
	vk_create_swapchain( vk.physical_device, vk.device, vk_surface, vk.present_format, &vk.swapchain, qfalse );
	vk_create_attachments();
	vk_create_render_passes();
	vk_create_framebuffers();

	vk_update_attachment_descriptors();

	vk_update_post_process_pipelines();
}


static void vk_set_render_scale( void ) {
	if ( vk_config.windowWidth != vk_config.renderWidth || vk_config.windowHeight != vk_config.renderHeight ) {
		if ( vk_config.renderScale > 0 ) {
			int scaleMode = vk_config.renderScale - 1;
			if ( scaleMode & 1 ) {
				// preserve aspect ratio (black bars on sides)
				float windowAspect = (float)vk_config.windowWidth / (float)vk_config.windowHeight;
				float renderAspect = (float)vk_config.renderWidth / (float)vk_config.renderHeight;
				if ( windowAspect >= renderAspect ) {
					float scale = (float)vk_config.windowHeight / (float)vk_config.renderHeight;
					int bias = (int)( ( vk_config.windowWidth - scale * (float)vk_config.renderWidth ) / 2 );
					vk.blitX0 += bias;
				} else {
					float scale = (float)vk_config.windowWidth / (float)vk_config.renderWidth;
					int bias = (int)( ( vk_config.windowHeight - scale * (float)vk_config.renderHeight ) / 2 );
					vk.blitY0 += bias;
				}
			}
			// linear filtering
			if ( scaleMode & 2 )
				vk.blitFilter = FILTER_LINEAR;
			else
				vk.blitFilter = FILTER_NEAREST;
		}

		vk.windowAdjusted = qtrue;
	}

	if ( vk_config.fbo && vk_config.supersample && !vk_config.renderScale ) {
		vk.blitFilter = FILTER_LINEAR;
	}
}


rhiStatus_t vk_impl_Initialize( void ) {
	char buf[64], driver_version[64];
	const char *vendor_name;
	VkPhysicalDeviceProperties props;
	uint32_t major;
	uint32_t minor;
	uint32_t patch;
	uint32_t maxSize;
	uint32_t i;

	init_vulkan_library();

	qvkGetDeviceQueue( vk.device, vk.queue_family_index, 0, &vk.queue );

	qvkGetPhysicalDeviceProperties( vk.physical_device, &props );
	vk.pipelineCacheKey.vendor = props.vendorID;
	vk.pipelineCacheKey.device = props.deviceID;
	vk.pipelineCacheKey.driver = props.driverVersion;
	memcpy( vk.pipelineCacheKey.uuid, props.pipelineCacheUUID, sizeof( vk.pipelineCacheKey.uuid ) );
	memcpy( vk.pipelineCacheKey.shaderPackage, RHI_GetShaderPackageHash(), sizeof( vk.pipelineCacheKey.shaderPackage ) );

	vk.cmd = vk.tess + 0;
	vk.timestampPeriod = props.limits.timestampPeriod;
	vk.uniform_alignment = (uint32_t)( props.limits.minUniformBufferOffsetAlignment );
	vk.uniform_item_size = PAD( (uint32_t)vk_config.uniformBytes, vk.uniform_alignment );

	// for flare visibility tests
	vk.storage_alignment = (uint32_t)( MAX( props.limits.minStorageBufferOffsetAlignment, sizeof( uint32_t ) ) );

	vk.maxAnisotropy = props.limits.maxSamplerAnisotropy;

	vk.blitFilter = FILTER_NEAREST;
	vk.windowAdjusted = qfalse;
	vk.blitX0 = vk.blitY0 = 0;

	vk_set_render_scale();

	if ( vk_config.fbo ) {
		vk.fboActive = qtrue;
		if ( vk_config.multisample ) {
			vk.msaaActive = qtrue;
		}
	} else {
		vk.fboActive = qfalse;
	}

	// multisampling

	vkMaxSamples = MIN( props.limits.sampledImageColorSampleCounts, props.limits.sampledImageDepthSampleCounts );

	if ( /*vk.fboActive &&*/ vk.msaaActive ) {
		VkSampleCountFlags mask = vkMaxSamples;
		vkSamples = MAX( (VkSampleCountFlagBits)log2pad( vk_config.multisample, 1 ), VK_SAMPLE_COUNT_2_BIT );
		while ( (VkSampleCountFlags)vkSamples > mask )
			vkSamples >>= 1;
		vk_host.Print( rhiLog_t::Info, "...using %ix MSAA\n", vkSamples );
	} else {
		vkSamples = VK_SAMPLE_COUNT_1_BIT;
	}

	vk.screenMapSamples = MIN( vkMaxSamples, VK_SAMPLE_COUNT_4_BIT );

	vk.screenMapWidth = (uint32_t)( (float)vk_config.renderWidth / 16.0 );
	if ( vk.screenMapWidth < 4 )
		vk.screenMapWidth = 4;

	vk.screenMapHeight = (uint32_t)( (float)vk_config.renderHeight / 16.0 );
	if ( vk.screenMapHeight < 4 )
		vk.screenMapHeight = 4;

	vk.defaults.geometry_size = VERTEX_BUFFER_SIZE;
	vk.defaults.staging_size = STAGING_BUFFER_SIZE;

	// get memory size & defaults
	{
		VkPhysicalDeviceMemoryProperties memoryProps;
		VkDeviceSize maxDedicatedSize = 0;
		VkDeviceSize maxBARSize = 0;
		qvkGetPhysicalDeviceMemoryProperties( vk.physical_device, &memoryProps );
		for ( i = 0; i < memoryProps.memoryTypeCount; i++ ) {
			if ( memoryProps.memoryTypes[i].propertyFlags == VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ) {
				maxDedicatedSize = memoryProps.memoryHeaps[memoryProps.memoryTypes[i].heapIndex].size;
			} else if ( memoryProps.memoryTypes[i].propertyFlags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ) {
				if ( maxDedicatedSize == 0 || memoryProps.memoryHeaps[memoryProps.memoryTypes[i].heapIndex].size > maxDedicatedSize ) {
					maxDedicatedSize = memoryProps.memoryHeaps[memoryProps.memoryTypes[i].heapIndex].size;
				}
			}
			if ( memoryProps.memoryTypes[i].propertyFlags == ( VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) ) {
				maxBARSize = memoryProps.memoryHeaps[memoryProps.memoryTypes[i].heapIndex].size;
			} else if ( ( memoryProps.memoryTypes[i].propertyFlags & ( VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) ) == ( VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) ) {
				if ( maxBARSize == 0 ) {
					maxBARSize = memoryProps.memoryHeaps[memoryProps.memoryTypes[i].heapIndex].size;
				}
			}
		}

		if ( maxDedicatedSize != 0 ) {
			vk_host.Print( rhiLog_t::Info, "...device memory size: %iMB\n", (int)( ( maxDedicatedSize + ( 1024 * 1024 ) - 1 ) / ( 1024 * 1024 ) ) );
		}
		if ( maxBARSize != 0 ) {
			if ( maxBARSize >= 128 * 1024 * 1024 ) {
				// user larger buffers to avoid potential reallocations
				vk.defaults.geometry_size = VERTEX_BUFFER_SIZE_HI;
				vk.defaults.staging_size = STAGING_BUFFER_SIZE_HI;
			}
#ifdef _DEBUG
			vk_host.Print( rhiLog_t::Info, "...BAR memory size: %iMB\n", (int)( ( maxBARSize + ( 1024 * 1024 ) - 1 ) / ( 1024 * 1024 ) ) );
#endif
		}
	}

	// fill glConfig information

	// maxTextureSize must not exceed IMAGE_CHUNK_SIZE
	maxSize = (uint32_t)( sqrtf( IMAGE_CHUNK_SIZE / 4 ) );
	// round down to next power of 2
	vk_device_info->maxTextureSize = MIN( props.limits.maxImageDimension2D, log2pad( maxSize, 0 ) );

	if ( vk_device_info->maxTextureSize > vk_config.maxTextureSize )
		vk_device_info->maxTextureSize = vk_config.maxTextureSize; // ResampleTexture() relies on that maximum

	// default chunk size, may be doubled on demand
	vk.image_chunk_size = IMAGE_CHUNK_SIZE;

	vk.maxLod = (float)( 1 + Q_log2( vk_device_info->maxTextureSize ) );

	if ( props.limits.maxPerStageDescriptorSamplers != 0xFFFFFFFF )
		vk_device_info->textureUnits = props.limits.maxPerStageDescriptorSamplers;
	else
		vk_device_info->textureUnits = props.limits.maxBoundDescriptorSets;
	if ( vk_device_info->textureUnits > vk_config.maxTextureUnits )
		vk_device_info->textureUnits = vk_config.maxTextureUnits;

	vk.maxBoundDescriptorSets = props.limits.maxBoundDescriptorSets;


	major = VK_VERSION_MAJOR( props.apiVersion );
	minor = VK_VERSION_MINOR( props.apiVersion );
	patch = VK_VERSION_PATCH( props.apiVersion );

	// decode driver version
	switch ( props.vendorID ) {
	case 0x10DE: // NVidia
		Com_sprintf( driver_version, sizeof( driver_version ), "%i.%i.%i.%i",
			( props.driverVersion >> 22 ) & 0x3FF,
			( props.driverVersion >> 14 ) & 0x0FF,
			( props.driverVersion >> 6 ) & 0x0FF,
			( props.driverVersion >> 0 ) & 0x03F );
		break;
#ifdef _WIN32
	case 0x8086: // Intel
		Com_sprintf( driver_version, sizeof( driver_version ), "%i.%i",
			( props.driverVersion >> 14 ),
			( props.driverVersion >> 0 ) & 0x3FFF );
		break;
#endif
	default:
		Com_sprintf( driver_version, sizeof( driver_version ), "%i.%i.%i",
			( props.driverVersion >> 22 ),
			( props.driverVersion >> 12 ) & 0x3FF,
			( props.driverVersion >> 0 ) & 0xFFF );
	}

	Com_sprintf( vk_device_info->version, sizeof( vk_device_info->version ), "API: %i.%i.%i, Driver: %s",
		major, minor, patch, driver_version );

#ifdef _WIN32
	// Intel iGPU drivers from 101.5333 to 101.6737 have a known bug that causes
	// VK_ERROR_DEVICE_LOST during vkQueueSubmit, see https://github.com/ec-/Quake3e/issues/312
	if ( props.vendorID == 0x8086 ) {
		uint32_t drvMajor = props.driverVersion >> 14;
		uint32_t drvMinor = props.driverVersion & 0x3FFF;
		if ( drvMajor == 101 && drvMinor >= 5333 && drvMinor <= 6737 ) {
			Com_sprintf( vk.driverNote, sizeof( vk.driverNote ), S_COLOR_WARNING "\nWARNING: Intel driver %i.%i is known to cause Vulkan crashes.\n"
																				 "Consider updating to driver >= 101.6790 or downgrading to <= 101.5186.\n",
				drvMajor, drvMinor );
		}
	}
#endif

	vk.offscreenRender = qtrue;

	if ( props.vendorID == 0x1002 ) {
		vendor_name = "Advanced Micro Devices, Inc.";
	} else if ( props.vendorID == 0x106B ) {
		vendor_name = "Apple Inc.";
	} else if ( props.vendorID == 0x10DE ) {
		// https://github.com/SaschaWillems/Vulkan/issues/493
		// we can't render to offscreen presentation surfaces on nvidia
		vk.offscreenRender = qfalse;
		vendor_name = "NVIDIA";
	} else if ( props.vendorID == 0x14E4 ) {
		vendor_name = "Broadcom Inc.";
	} else if ( props.vendorID == 0x1AE0 ) {
		vendor_name = "Google Inc.";
	} else if ( props.vendorID == 0x8086 ) {
		vendor_name = "Intel Corporation";
	} else if ( props.vendorID == VK_VENDOR_ID_MESA ) {
		vendor_name = "MESA";
	} else {
		Com_sprintf( buf, sizeof( buf ), "VendorID: %04x", props.vendorID );
		vendor_name = buf;
	}

	Q_strncpyz( vk_device_info->vendor, vendor_name, sizeof( vk_device_info->vendor ) );
	Q_strncpyz( vk_device_info->renderer, renderer_name( &props ), sizeof( vk_device_info->renderer ) );

	SET_OBJECT_NAME( (intptr_t)vk.device, vk_device_info->renderer, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT );

	// do early texture mode setup to avoid redundant descriptor updates in GL_SetDefaultState()
	vk.samplers.filter_min = -1;
	vk.samplers.filter_max = -1;
	if ( vk_config.textureFilterValid ) {
		bool changed;
		const rhiStatus_t status = vk_impl_SetTextureFilter( vk_config.textureMin, vk_config.textureMag, &changed );
		if ( status != rhiStatus_t::Success )
			return status;
	}

	//
	// Sync primitives.
	//
	vk_create_sync_primitives();

	//
	// Command pool.
	//
	{
		VkCommandPoolCreateInfo desc;

		desc.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		desc.pNext = NULL;
		desc.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		desc.queueFamilyIndex = vk.queue_family_index;

		VK_CHECK( qvkCreateCommandPool( vk.device, &desc, NULL, &vk.command_pool ) );

		SET_OBJECT_NAME( vk.command_pool, "command pool", VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT );
	}

#ifdef USE_UPLOAD_QUEUE
	{
		VkCommandBufferAllocateInfo alloc_info;

		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.pNext = NULL;
		alloc_info.commandPool = vk.command_pool;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandBufferCount = 1;

		VK_CHECK( qvkAllocateCommandBuffers( vk.device, &alloc_info, &vk.staging_command_buffer ) );
	}
#endif

	//
	// Command buffers and color attachments.
	//
	for ( i = 0; i < NUM_COMMAND_BUFFERS; i++ ) {
		VkCommandBufferAllocateInfo alloc_info;

		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.pNext = NULL;
		alloc_info.commandPool = vk.command_pool;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandBufferCount = 1;

		VK_CHECK( qvkAllocateCommandBuffers( vk.device, &alloc_info, &vk.tess[i].command_buffer ) );

		//SET_OBJECT_NAME( vk.tess[i].command_buffer, va( "command buffer %i", i ), VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT );
	}

	//
	// Descriptor pool.
	//
	{
		VkDescriptorPoolSize pool_size[3];
		VkDescriptorPoolCreateInfo desc;
		uint32_t poolIndex, maxSets;

		pool_size[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		pool_size[0].descriptorCount = vk_config.maxImages + 1 + 1 + 1 + VK_NUM_BLOOM_PASSES * 2; // color, screenmap, bloom descriptors

		pool_size[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		pool_size[1].descriptorCount = NUM_COMMAND_BUFFERS;

		//pool_size[2].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		//pool_size[2].descriptorCount = NUM_COMMAND_BUFFERS;

		pool_size[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		pool_size[2].descriptorCount = 1;

		for ( poolIndex = 0, maxSets = 0; poolIndex < ARRAY_LEN( pool_size ); poolIndex++ ) {
			maxSets += pool_size[poolIndex].descriptorCount;
		}

		desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		desc.pNext = NULL;
		desc.flags = 0;
		desc.maxSets = maxSets;
		desc.poolSizeCount = ARRAY_LEN( pool_size );
		desc.pPoolSizes = pool_size;

		VK_CHECK( qvkCreateDescriptorPool( vk.device, &desc, NULL, &vk.descriptor_pool ) );
	}

	//
	// Descriptor set layout.
	//
	vk_create_layout_binding( 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, &vk.set_layout_sampler );
	vk_create_layout_binding( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, &vk.set_layout_uniform );
	vk_create_layout_binding( 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, &vk.set_layout_storage );
	//vk_create_layout_binding( 0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, &vk.set_layout_input );

	//
	// Pipeline layouts.
	//
	{
		VkDescriptorSetLayout set_layouts[6];
		VkPipelineLayoutCreateInfo desc;
		VkPushConstantRange push_range;

		push_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		push_range.offset = 0;
		push_range.size = 64; // 16 floats

		// standard pipelines
		set_layouts[0] = vk.set_layout_uniform; // fog/dlight parameters
		set_layouts[1] = vk.set_layout_sampler; // diffuse
		set_layouts[2] = vk.set_layout_sampler; // lightmap / fog-only
		set_layouts[3] = vk.set_layout_sampler; // blend
		set_layouts[4] = vk.set_layout_sampler; // collapsed fog texture
		desc.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		desc.pNext = NULL;
		desc.flags = 0;
		desc.setLayoutCount = ( vk.maxBoundDescriptorSets >= RHI_BINDING_COUNT ) ? RHI_BINDING_COUNT : 4;
		desc.pSetLayouts = set_layouts;
		desc.pushConstantRangeCount = 1;
		desc.pPushConstantRanges = &push_range;

		VK_CHECK(qvkCreatePipelineLayout(vk.device, &desc, NULL, &vk.pipeline_layout));

		// flare test pipeline
		set_layouts[0] = vk.set_layout_storage; // dynamic storage buffer

		desc.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		desc.pNext = NULL;
		desc.flags = 0;
		desc.setLayoutCount = 1;
		desc.pSetLayouts = set_layouts;
		desc.pushConstantRangeCount = 1;
		desc.pPushConstantRanges = &push_range;

		VK_CHECK( qvkCreatePipelineLayout( vk.device, &desc, NULL, &vk.pipeline_layout_storage ) );

		// post-processing pipeline
		set_layouts[0] = vk.set_layout_sampler; // sampler
		set_layouts[1] = vk.set_layout_sampler; // sampler
		set_layouts[2] = vk.set_layout_sampler; // sampler
		set_layouts[3] = vk.set_layout_sampler; // sampler

		desc.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		desc.pNext = NULL;
		desc.flags = 0;
		desc.setLayoutCount = 1;
		desc.pSetLayouts = set_layouts;
		desc.pushConstantRangeCount = 0;
		desc.pPushConstantRanges = NULL;

		VK_CHECK( qvkCreatePipelineLayout( vk.device, &desc, NULL, &vk.pipeline_layout_post_process ) );

		desc.setLayoutCount = VK_NUM_BLOOM_PASSES;

		VK_CHECK( qvkCreatePipelineLayout( vk.device, &desc, NULL, &vk.pipeline_layout_blend ) );

		SET_OBJECT_NAME( vk.pipeline_layout, "pipeline layout - main", VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT );
		SET_OBJECT_NAME( vk.pipeline_layout_post_process, "pipeline layout - post-processing", VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT );
		SET_OBJECT_NAME( vk.pipeline_layout_blend, "pipeline layout - blend", VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT );
	}

	vk.geometry_buffer_size_new = vk.defaults.geometry_size;
	vk_create_geometry_buffers( vk.geometry_buffer_size_new );
	vk.geometry_buffer_size_new = 0;

	vk_create_storage_buffer( vk_config.maxVisibilityTests * vk.storage_alignment );

	vk_create_shader_modules();

	{
		VkPipelineCacheCreateInfo ci;
		Com_Memset( &ci, 0, sizeof( ci ) );
		ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VK_CHECK( qvkCreatePipelineCache( vk.device, &ci, NULL, &vk.pipelineCache ) );
	}

	vk.renderPassIndex = RENDER_PASS_MAIN; // default render pass

	// swapchain
	vk.initSwapchainLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	//vk.initSwapchainLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	vk_create_swapchain( vk.physical_device, vk.device, vk_surface, vk.present_format, &vk.swapchain, qtrue );

	// color/depth attachments
	vk_create_attachments();

	// renderpasses
	vk_create_render_passes();

	// framebuffers for each swapchain image
	vk_create_framebuffers();

	// preallocate staging buffer
	if ( vk.defaults.staging_size == STAGING_BUFFER_SIZE_HI ) {
		vk_alloc_staging_buffer( vk.defaults.staging_size );
	}

	vk.active = qtrue;
	return rhiStatus_t::Success;
}


void RHI_MarkWorldPipelines( void ) {
	vk.pipelines_world_base = vk.pipelines_count;
}


static void vk_destroy_attachments( void ) {
	uint32_t i;

	if ( vk.bloom_image[0] ) {
		for ( i = 0; i < ARRAY_LEN( vk.bloom_image ); i++ ) {
			qvkDestroyImage( vk.device, vk.bloom_image[i], NULL );
			qvkDestroyImageView( vk.device, vk.bloom_image_view[i], NULL );
			vk.bloom_image[i] = VK_NULL_HANDLE;
			vk.bloom_image_view[i] = VK_NULL_HANDLE;
		}
	}

	if ( vk.color_image ) {
		qvkDestroyImage( vk.device, vk.color_image, NULL );
		qvkDestroyImageView( vk.device, vk.color_image_view, NULL );
		vk.color_image = VK_NULL_HANDLE;
		vk.color_image_view = VK_NULL_HANDLE;
	}

	if ( vk.msaa_image ) {
		qvkDestroyImage( vk.device, vk.msaa_image, NULL );
		qvkDestroyImageView( vk.device, vk.msaa_image_view, NULL );
		vk.msaa_image = VK_NULL_HANDLE;
		vk.msaa_image_view = VK_NULL_HANDLE;
	}

	qvkDestroyImage( vk.device, vk.depth_image, NULL );
	qvkDestroyImageView( vk.device, vk.depth_image_view, NULL );
	vk.depth_image = VK_NULL_HANDLE;
	vk.depth_image_view = VK_NULL_HANDLE;

	if ( vk.screenMap.color_image ) {
		qvkDestroyImage( vk.device, vk.screenMap.color_image, NULL );
		qvkDestroyImageView( vk.device, vk.screenMap.color_image_view, NULL );
		vk.screenMap.color_image = VK_NULL_HANDLE;
		vk.screenMap.color_image_view = VK_NULL_HANDLE;
	}

	if ( vk.screenMap.color_image_msaa ) {
		qvkDestroyImage( vk.device, vk.screenMap.color_image_msaa, NULL );
		qvkDestroyImageView( vk.device, vk.screenMap.color_image_view_msaa, NULL );
		vk.screenMap.color_image_msaa = VK_NULL_HANDLE;
		vk.screenMap.color_image_view_msaa = VK_NULL_HANDLE;
	}

	if ( vk.screenMap.depth_image ) {
		qvkDestroyImage( vk.device, vk.screenMap.depth_image, NULL );
		qvkDestroyImageView( vk.device, vk.screenMap.depth_image_view, NULL );
		vk.screenMap.depth_image = VK_NULL_HANDLE;
		vk.screenMap.depth_image_view = VK_NULL_HANDLE;
	}

	if ( vk.capture.image ) {
		qvkDestroyImage( vk.device, vk.capture.image, NULL );
		qvkDestroyImageView( vk.device, vk.capture.image_view, NULL );
		vk.capture.image = VK_NULL_HANDLE;
		vk.capture.image_view = VK_NULL_HANDLE;
	}

	for ( i = 0; i < vk.image_memory_count; i++ ) {
		qvkFreeMemory( vk.device, vk.image_memory[i], NULL );
	}

	vk.image_memory_count = 0;
}


static void vk_destroy_render_passes( void ) {
	uint32_t i;

	if ( vk.render_pass.main != VK_NULL_HANDLE ) {
		qvkDestroyRenderPass( vk.device, vk.render_pass.main, NULL );
		vk.render_pass.main = VK_NULL_HANDLE;
	}

	if ( vk.render_pass.bloom_extract != VK_NULL_HANDLE ) {
		qvkDestroyRenderPass( vk.device, vk.render_pass.bloom_extract, NULL );
		vk.render_pass.bloom_extract = VK_NULL_HANDLE;
	}

	for ( i = 0; i < ARRAY_LEN( vk.render_pass.blur ); i++ ) {
		if ( vk.render_pass.blur[i] != VK_NULL_HANDLE ) {
			qvkDestroyRenderPass( vk.device, vk.render_pass.blur[i], NULL );
			vk.render_pass.blur[i] = VK_NULL_HANDLE;
		}
	}

	if ( vk.render_pass.post_bloom != VK_NULL_HANDLE ) {
		qvkDestroyRenderPass( vk.device, vk.render_pass.post_bloom, NULL );
		vk.render_pass.post_bloom = VK_NULL_HANDLE;
	}

	if ( vk.render_pass.screenmap != VK_NULL_HANDLE ) {
		qvkDestroyRenderPass( vk.device, vk.render_pass.screenmap, NULL );
		vk.render_pass.screenmap = VK_NULL_HANDLE;
	}

	if ( vk.render_pass.gamma != VK_NULL_HANDLE ) {
		qvkDestroyRenderPass( vk.device, vk.render_pass.gamma, NULL );
		vk.render_pass.gamma = VK_NULL_HANDLE;
	}

	if ( vk.render_pass.capture != VK_NULL_HANDLE ) {
		qvkDestroyRenderPass( vk.device, vk.render_pass.capture, NULL );
		vk.render_pass.capture = VK_NULL_HANDLE;
	}
}


static void vk_destroy_pipelines( qboolean resetCounter ) {
	uint32_t i, j;

	for ( i = 0; i < vk.pipelines_count; i++ ) {
		for ( j = 0; j < RENDER_PASS_COUNT; j++ ) {
			if ( vk.pipelines[i].handle[j] != VK_NULL_HANDLE ) {
				qvkDestroyPipeline( vk.device, vk.pipelines[i].handle[j], NULL );
				vk.pipelines[i].handle[j] = VK_NULL_HANDLE;
				vk.pipeline_create_count--;
			}
		}
	}

	if ( resetCounter ) {
		Com_Memset( &vk.pipelines, 0, sizeof( vk.pipelines ) );
		vk.pipelines_count = 0;
	}

	if ( vk.gamma_pipeline ) {
		qvkDestroyPipeline( vk.device, vk.gamma_pipeline, NULL );
		vk.gamma_pipeline = VK_NULL_HANDLE;
	}

	if ( vk.capture_pipeline ) {
		qvkDestroyPipeline( vk.device, vk.capture_pipeline, NULL );
		vk.capture_pipeline = VK_NULL_HANDLE;
	}

	if ( vk.bloom_extract_pipeline != VK_NULL_HANDLE ) {
		qvkDestroyPipeline( vk.device, vk.bloom_extract_pipeline, NULL );
		vk.bloom_extract_pipeline = VK_NULL_HANDLE;
	}

	if ( vk.bloom_blend_pipeline != VK_NULL_HANDLE ) {
		qvkDestroyPipeline( vk.device, vk.bloom_blend_pipeline, NULL );
		vk.bloom_blend_pipeline = VK_NULL_HANDLE;
	}

	for ( i = 0; i < ARRAY_LEN( vk.blur_pipeline ); i++ ) {
		if ( vk.blur_pipeline[i] != VK_NULL_HANDLE ) {
			qvkDestroyPipeline( vk.device, vk.blur_pipeline[i], NULL );
			vk.blur_pipeline[i] = VK_NULL_HANDLE;
		}
	}
}


void vk_impl_Shutdown( void ) {
	int i, j, k, l;

	if ( qvkQueuePresentKHR == NULL ) { // not fully initialized
		goto __cleanup;
	}

	vk_destroy_framebuffers();

	vk_destroy_pipelines( qtrue ); // reset counter

	vk_destroy_render_passes();

	vk_destroy_attachments();

	vk_destroy_swapchain();

	if ( vk.pipelineCache != VK_NULL_HANDLE ) {
		qvkDestroyPipelineCache( vk.device, vk.pipelineCache, NULL );
		vk.pipelineCache = VK_NULL_HANDLE;
	}

	qvkDestroyCommandPool( vk.device, vk.command_pool, NULL );

	qvkDestroyDescriptorPool( vk.device, vk.descriptor_pool, NULL );

	qvkDestroyDescriptorSetLayout( vk.device, vk.set_layout_sampler, NULL );
	qvkDestroyDescriptorSetLayout( vk.device, vk.set_layout_uniform, NULL );
	qvkDestroyDescriptorSetLayout( vk.device, vk.set_layout_storage, NULL );

	qvkDestroyPipelineLayout( vk.device, vk.pipeline_layout, NULL );
	qvkDestroyPipelineLayout( vk.device, vk.pipeline_layout_storage, NULL );
	qvkDestroyPipelineLayout( vk.device, vk.pipeline_layout_post_process, NULL );
	qvkDestroyPipelineLayout( vk.device, vk.pipeline_layout_blend, NULL );

#ifdef USE_VBO
	vk_release_vbo();
#endif

	vk_clean_staging_buffer();

	vk_release_geometry_buffers();

	vk_destroy_samplers();

	vk_destroy_sync_primitives();

	qvkDestroyBuffer( vk.device, vk.storage.buffer, NULL );
	qvkFreeMemory( vk.device, vk.storage.memory, NULL );

	for ( i = 0; i < 3; i++ ) {
		for ( j = 0; j < 2; j++ ) {
			for ( k = 0; k < 2; k++ ) {
				for ( l = 0; l < 2; l++ ) {
					if ( vk.modules.vert.gen[i][j][k][l] != VK_NULL_HANDLE ) {
						qvkDestroyShaderModule( vk.device, vk.modules.vert.gen[i][j][k][l], NULL );
						vk.modules.vert.gen[i][j][k][l] = VK_NULL_HANDLE;
					}
				}
			}
		}
	}
	for ( i = 0; i < 3; i++ ) {
		for ( j = 0; j < 2; j++ ) {
			for ( k = 0; k < 2; k++ ) {
				if ( vk.modules.frag.gen[i][j][k] != VK_NULL_HANDLE ) {
					qvkDestroyShaderModule( vk.device, vk.modules.frag.gen[i][j][k], NULL );
					vk.modules.frag.gen[i][j][k] = VK_NULL_HANDLE;
				}
			}
		}
	}
	for ( i = 0; i < 2; i++ ) {
		if ( vk.modules.vert.light[i] != VK_NULL_HANDLE ) {
			qvkDestroyShaderModule( vk.device, vk.modules.vert.light[i], NULL );
			vk.modules.vert.light[i] = VK_NULL_HANDLE;
		}
		for ( j = 0; j < 2; j++ ) {
			if ( vk.modules.frag.light[i][j] != VK_NULL_HANDLE ) {
				qvkDestroyShaderModule( vk.device, vk.modules.frag.light[i][j], NULL );
				vk.modules.frag.light[i][j] = VK_NULL_HANDLE;
			}
		}
	}

	for ( i = 0; i < 2; i++ ) {
		for ( j = 0; j < 2; j++ ) {
			for ( k = 0; k < 2; k++ ) {
				qvkDestroyShaderModule( vk.device, vk.modules.vert.ident1[i][j][k], NULL );
				vk.modules.vert.ident1[i][j][k] = VK_NULL_HANDLE;
			}
			qvkDestroyShaderModule( vk.device, vk.modules.frag.ident1[i][j], NULL );
			vk.modules.frag.ident1[i][j] = VK_NULL_HANDLE;
		}
	}

	for ( i = 0; i < 2; i++ ) {
		for ( j = 0; j < 2; j++ ) {
			for ( k = 0; k < 2; k++ ) {
				qvkDestroyShaderModule( vk.device, vk.modules.vert.fixed[i][j][k], NULL );
				vk.modules.vert.fixed[i][j][k] = VK_NULL_HANDLE;
			}
			qvkDestroyShaderModule( vk.device, vk.modules.frag.fixed[i][j], NULL );
			vk.modules.frag.fixed[i][j] = VK_NULL_HANDLE;
		}
	}

	for ( i = 0; i < 1; i++ ) {
		for ( j = 0; j < 2; j++ ) {
			qvkDestroyShaderModule( vk.device, vk.modules.frag.ent[i][j], NULL );
			vk.modules.frag.ent[i][j] = VK_NULL_HANDLE;
		}
	}

	qvkDestroyShaderModule( vk.device, vk.modules.frag.gen0_df, NULL );

	qvkDestroyShaderModule( vk.device, vk.modules.color_fs, NULL );
	qvkDestroyShaderModule( vk.device, vk.modules.color_vs, NULL );
	qvkDestroyShaderModule( vk.device, vk.modules.pbr_vs, NULL );
	qvkDestroyShaderModule( vk.device, vk.modules.pbr_fs, NULL );
	qvkDestroyShaderModule( vk.device, vk.modules.pbr_baked_vs, NULL );
	qvkDestroyShaderModule( vk.device, vk.modules.pbr_baked_fs, NULL );

	qvkDestroyShaderModule( vk.device, vk.modules.fog_vs, NULL );
	qvkDestroyShaderModule( vk.device, vk.modules.fog_fs, NULL );

	qvkDestroyShaderModule( vk.device, vk.modules.dot_vs, NULL );
	qvkDestroyShaderModule( vk.device, vk.modules.dot_fs, NULL );

	qvkDestroyShaderModule( vk.device, vk.modules.bloom_fs, NULL );
	qvkDestroyShaderModule( vk.device, vk.modules.blur_fs, NULL );
	qvkDestroyShaderModule( vk.device, vk.modules.blend_fs, NULL );

	qvkDestroyShaderModule( vk.device, vk.modules.gamma_vs, NULL );
	qvkDestroyShaderModule( vk.device, vk.modules.gamma_fs, NULL );

__cleanup:
	if ( vk.device != VK_NULL_HANDLE ) {
		qvkDestroyDevice( vk.device, NULL );
	}

	deinit_device_functions();

	Com_Memset( &vk, 0, sizeof( vk ) );
	Com_Memset( &vk_world, 0, sizeof( vk_world ) );

	vk_destroy_instance();
	deinit_instance_functions();
}


bool RHI_Available( void ) {
	return vk.device != VK_NULL_HANDLE;
}

rhiCapabilities_t RHI_GetCapabilities( void ) {
	return { vk.active != qfalse, vk.wideLines != qfalse, vk.fragmentStores != qfalse,
		vk.clearAttachment != qfalse, vk.fboActive != qfalse, vk.offscreenRender != qfalse,
		vk.maxBoundDescriptorSets };
}

rhiFrameState_t RHI_GetFrameState( void ) {
	return { vk.renderPassIndex == RENDER_PASS_SCREENMAP,
		vk.cmd && vk.cmd->waitForFence != qfalse, vk.capture.image != VK_NULL_HANDLE };
}

void RHI_InvalidateViewport( void ) {
	vk.cmd->depth_range = DEPTH_RANGE_COUNT;
}

rhiDeviceDescription_t RHI_GetDeviceDescription( void ) {
	rhiDeviceDescription_t info = {};
	info.driverNote = vk.driverNote;
	Q_strncpyz( info.presentFormat, vk_format_string( vk.present_format.format ), sizeof( info.presentFormat ) );
	if ( vk.color_format != vk.present_format.format )
		Q_strncpyz( info.colorFormat, vk_format_string( vk.color_format ), sizeof( info.colorFormat ) );
	if ( vk.capture_format != vk.present_format.format || vk.capture_format != vk.color_format )
		Q_strncpyz( info.captureFormat, vk_format_string( vk.capture_format ), sizeof( info.captureFormat ) );
	Q_strncpyz( info.depthFormat, vk_format_string( vk.depth_format ), sizeof( info.depthFormat ) );
	return info;
}


void RHI_BindScreenMap( uint32_t slot ) {
	vk_update_descriptor( slot, vk.screenMap.color_descriptor );
}

void RHI_BindIndices( rhiGeometryBuffer_t buffer, uint32_t offset ) {
	vk_bind_index_buffer( buffer == rhiGeometryBuffer_t::World ? vk.vbo.vertex_buffer : vk.cmd->vertex_buffer, offset );
}

rhiStats_t RHI_GetStats( void ) {
	return { vk.stats.vertex_buffer_max, vk.geometry_buffer_size, vk.staging_buffer.size,
		vk.stats.push_size_max, vk.pipeline_create_count, vk.pipelines_count,
		vk.pipelines_world_base, vk_world.num_image_chunks, vk.samplers.count, NUM_COMMAND_BUFFERS, vk.stats.frame_draw_calls };
}

static rhiStatus_t vk_status( VkResult result ) {
	if ( result >= VK_SUCCESS )
		return rhiStatus_t::Success;
	switch ( result ) {
	case VK_ERROR_OUT_OF_HOST_MEMORY:
	case VK_ERROR_OUT_OF_DEVICE_MEMORY:
		return rhiStatus_t::OutOfMemory;
	case VK_ERROR_DEVICE_LOST:
		return rhiStatus_t::DeviceLost;
	default:
		return rhiStatus_t::Error;
	}
}

rhiStatus_t RHI_WaitIdle( void ) {
	vk_clear_error();
	if ( !vk.device || !qvkDeviceWaitIdle )
		return rhiStatus_t::Unavailable;
	return vk_status( qvkDeviceWaitIdle( vk.device ) );
}

rhiStatus_t RHI_WaitQueue( void ) {
	vk_clear_error();
	if ( !vk.queue || !qvkQueueWaitIdle )
		return rhiStatus_t::Unavailable;
	return vk_status( qvkQueueWaitIdle( vk.queue ) );
}

void vk_wait_idle( void ) {
	VK_CHECK( qvkDeviceWaitIdle( vk.device ) );
}


void vk_queue_wait_idle( void ) {
	VK_CHECK( qvkQueueWaitIdle( vk.queue ) );
}


void vk_impl_ReleaseResources( void ) {
	int i, j;

	vk_wait_idle();

	for ( i = 0; i < vk_world.num_image_chunks; i++ )
		qvkFreeMemory( vk.device, vk_world.image_chunks[i].memory, NULL );

	vk_clean_staging_buffer();

	// vk_destroy_samplers();

	for ( i = vk.pipelines_world_base; (uint32_t)i < vk.pipelines_count; i++ ) {
		for ( j = 0; j < RENDER_PASS_COUNT; j++ ) {
			if ( vk.pipelines[i].handle[j] != VK_NULL_HANDLE ) {
				qvkDestroyPipeline( vk.device, vk.pipelines[i].handle[j], NULL );
				vk.pipelines[i].handle[j] = VK_NULL_HANDLE;
				vk.pipeline_create_count--;
			}
		}
		Com_Memset( &vk.pipelines[i], 0, sizeof( vk.pipelines[0] ) );
	}
	vk.pipelines_count = vk.pipelines_world_base;

	VK_CHECK( qvkResetDescriptorPool( vk.device, vk.descriptor_pool, 0 ) );

	if ( vk_world.num_image_chunks > 1 ) {
		// if we allocated more than 2 image chunks - use doubled default size
		vk.image_chunk_size = ( IMAGE_CHUNK_SIZE * 2 );
	}
#if 0 // do not reduce chunk size
	else if ( vk_world.num_image_chunks == 1 ) {
		// otherwise set to default if used less than a half
		if ( vk_world.image_chunks[0].used < ( IMAGE_CHUNK_SIZE - (IMAGE_CHUNK_SIZE / 10) ) ) {
			vk.image_chunk_size = IMAGE_CHUNK_SIZE;
		}
	}
#endif

	Com_Memset( &vk_world, 0, sizeof( vk_world ) );

	// Reset geometry buffers offsets
	for ( i = 0; i < NUM_COMMAND_BUFFERS; i++ ) {
		vk.tess[i].uniform_read_offset = 0;
		vk.tess[i].vertex_buffer_offset = 0;
	}

	Com_Memset( vk.cmd->buf_offset, 0, sizeof( vk.cmd->buf_offset ) );
	Com_Memset( vk.cmd->vbo_offset, 0, sizeof( vk.cmd->vbo_offset ) );

	Com_Memset( &vk.stats, 0, sizeof( vk.stats ) );
}

#if 0
static void record_buffer_memory_barrier(VkCommandBuffer cb, VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset,
		VkPipelineStageFlags src_stages, VkPipelineStageFlags dst_stages,
		VkAccessFlags src_access, VkAccessFlags dst_access) {

	VkBufferMemoryBarrier barrier;
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.pNext = NULL;
	barrier.srcAccessMask = src_access;
	barrier.dstAccessMask = dst_access;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.buffer = buffer;
	barrier.offset = offset;
	barrier.size = size;

	qvkCmdPipelineBarrier( cb, src_stages, dst_stages, 0, 0, NULL, 1, &barrier, 0, NULL );
}
#endif

static VkFormat vk_texture_format( rhiFormat_t format ) {
	switch ( format ) {
	case rhiFormat_t::RGBA8:
		return VK_FORMAT_R8G8B8A8_UNORM;
	case rhiFormat_t::BGRA8:
		return VK_FORMAT_B8G8R8A8_UNORM;
	case rhiFormat_t::RGB8:
		return VK_FORMAT_R8G8B8_UNORM;
	case rhiFormat_t::BGRA4:
		return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
	case rhiFormat_t::A1RGB5:
		return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
	case rhiFormat_t::BC4:
		return VK_FORMAT_BC4_UNORM_BLOCK;
	case rhiFormat_t::BC5:
		return VK_FORMAT_BC5_UNORM_BLOCK;
	case rhiFormat_t::BC7:
		return VK_FORMAT_BC7_UNORM_BLOCK;
	case rhiFormat_t::BC7_SRGB:
		return VK_FORMAT_BC7_SRGB_BLOCK;
	}
	return VK_FORMAT_UNDEFINED;
}

static_assert( sizeof( VkImage ) == sizeof( uint64_t ) && sizeof( VkImageView ) == sizeof( uint64_t ) && sizeof( VkDescriptorSet ) == sizeof( uint64_t ) );

static VkSamplerAddressMode vk_texture_address( rhiAddress_t address ) {
	switch ( address ) {
	case rhiAddress_t::Repeat:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case rhiAddress_t::ClampToEdge:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case rhiAddress_t::ClampToBorder:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	}
	return VK_SAMPLER_ADDRESS_MODE_REPEAT;
}

void vk_impl_CreateTexture( rhiTexture_t *texture, int32_t width, int32_t height, int32_t mip_levels, rhiFormat_t imageFormat, rhiAddress_t address, const char *label, bool owned, bool deferSampler ) {

	const VkFormat format = vk_texture_format( imageFormat );

	if ( texture->image ) {
		qvkDestroyImage( vk.device, (VkImage)(uintptr_t)texture->image, NULL );
		texture->image = 0;
	}

	if ( texture->view ) {
		qvkDestroyImageView( vk.device, (VkImageView)(uintptr_t)texture->view, NULL );
		texture->view = 0;
	}

	if ( texture->memory ) {
		qvkFreeMemory( vk.device, (VkDeviceMemory)(uintptr_t)texture->memory, NULL );
		texture->memory = 0;
	}

	// create image
	{
		VkImageCreateInfo desc;

		desc.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		desc.pNext = NULL;
		desc.flags = 0;
		desc.imageType = VK_IMAGE_TYPE_2D;
		desc.format = format;
		desc.extent.width = width;
		desc.extent.height = height;
		desc.extent.depth = 1;
		desc.mipLevels = mip_levels;
		desc.arrayLayers = 1;
		desc.samples = VK_SAMPLE_COUNT_1_BIT;
		desc.tiling = VK_IMAGE_TILING_OPTIMAL;
		desc.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		desc.queueFamilyIndexCount = 0;
		desc.pQueueFamilyIndices = NULL;
		desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VkImage handle;
		VK_CHECK( qvkCreateImage( vk.device, &desc, NULL, &handle ) );
		texture->image = (uint64_t)(uintptr_t)handle;

		if ( owned ) {
			VkMemoryRequirements requirements;
			qvkGetImageMemoryRequirements( vk.device, handle, &requirements );
			VkMemoryAllocateInfo allocation = {};
			allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocation.allocationSize = requirements.size;
			allocation.memoryTypeIndex = find_memory_type( requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
			VkDeviceMemory memory;
			VK_CHECK( qvkAllocateMemory( vk.device, &allocation, NULL, &memory ) );
			texture->memory = (uint64_t)(uintptr_t)memory;
			VK_CHECK( qvkBindImageMemory( vk.device, handle, memory, 0 ) );
		} else {
			allocate_and_bind_image_memory( (VkImage)(uintptr_t)texture->image );
		}
	}

	// create image view
	{
		VkImageViewCreateInfo desc;

		desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		desc.pNext = NULL;
		desc.flags = 0;
		desc.image = (VkImage)(uintptr_t)texture->image;
		desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
		desc.format = format;
		desc.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		desc.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		desc.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		desc.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		desc.subresourceRange.baseMipLevel = 0;
		desc.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		desc.subresourceRange.baseArrayLayer = 0;
		desc.subresourceRange.layerCount = 1;

		VkImageView view;
		VK_CHECK( qvkCreateImageView( vk.device, &desc, NULL, &view ) );
		texture->view = (uint64_t)(uintptr_t)view;
	}

	// create associated descriptor set
	if ( texture->binding == 0 ) {
		VkDescriptorSetAllocateInfo desc;

		desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		desc.pNext = NULL;
		desc.descriptorPool = vk.descriptor_pool;
		desc.descriptorSetCount = 1;
		desc.pSetLayouts = &vk.set_layout_sampler;

		VkDescriptorSet binding;
		VK_CHECK( qvkAllocateDescriptorSets( vk.device, &desc, &binding ) );
		texture->binding = (uint64_t)(uintptr_t)binding;
	}

	if ( !deferSampler )
		vk_impl_UpdateTextureSampler( texture, address, mip_levels > 1 );

	SET_OBJECT_NAME( texture->image, label, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT );
	SET_OBJECT_NAME( texture->view, label, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT );
	SET_OBJECT_NAME( texture->binding, label, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT );
}


void vk_impl_UploadTexture( const rhiTexture_t *texture, int32_t x, int32_t y, int32_t width, int32_t height, int32_t mipmaps, const uint8_t *pixels, int32_t bytesPerPixel, bool update, int32_t blockExtent ) {

	VkCommandBuffer command_buffer;
	VkBufferImageCopy regions[16];
	VkBufferImageCopy region;

	int num_regions = 0;
	int buffer_size = 0;

	while ( true ) {
		Com_Memset( &region, 0, sizeof( region ) );
		region.bufferOffset = buffer_size;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = num_regions;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset.x = x;
		region.imageOffset.y = y;
		region.imageOffset.z = 0;
		region.imageExtent.width = width;
		region.imageExtent.height = height;
		region.imageExtent.depth = 1;

		regions[num_regions] = region;
		num_regions++;

		buffer_size += ( ( width + blockExtent - 1 ) / blockExtent ) * ( ( height + blockExtent - 1 ) / blockExtent ) * bytesPerPixel;

		if ( num_regions >= mipmaps || ( width == 1 && height == 1 ) || (size_t)num_regions >= ARRAY_LEN( regions ) )
			break;

		x >>= 1;
		y >>= 1;

		width >>= 1;
		if ( width < 1 )
			width = 1;

		height >>= 1;
		if ( height < 1 )
			height = 1;
	}

#ifdef USE_UPLOAD_QUEUE
	if ( vk_wait_staging_buffer() ) {
		// wait for vkQueueSubmit() completion before new upload
	}

	if ( blockExtent > 1 )
		vk.staging_buffer.offset = PAD( vk.staging_buffer.offset, bytesPerPixel );

	if ( vk.staging_buffer.size - vk.staging_buffer.offset < buffer_size ) {
		// try to flush staging buffer and reset offset
		vk_flush_staging_buffer( qfalse );
	}

	if ( vk.staging_buffer.size /* - vk_world.staging_buffer_offset */ < buffer_size ) {
		// if still not enough - reallocate staging buffer
		vk_alloc_staging_buffer( buffer_size );
	}

	for ( int n = 0; n < num_regions; n++ ) {
		regions[n].bufferOffset += vk.staging_buffer.offset;
	}

	Com_Memcpy( vk.staging_buffer.ptr + vk.staging_buffer.offset, pixels, buffer_size );

	if ( vk.staging_buffer.offset == 0 ) {
		VkCommandBufferBeginInfo begin_info;
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.pNext = NULL;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		begin_info.pInheritanceInfo = NULL;
		VK_CHECK( qvkBeginCommandBuffer( vk.staging_command_buffer, &begin_info ) );
	}

	//vk_host.Print( rhiLog_t::Warning, "batch @%6i + %i %s \n", (int)vk_world.staging_buffer_offset, (int)buffer_size, "texture" );
	vk.staging_buffer.offset += buffer_size;

	command_buffer = vk.staging_command_buffer;

	if ( update ) {
		record_image_layout_transition( command_buffer, (VkImage)(uintptr_t)texture->image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 0 );
	} else {
		record_image_layout_transition( command_buffer, (VkImage)(uintptr_t)texture->image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_HOST_BIT, 0 );
	}

	qvkCmdCopyBufferToImage( command_buffer, vk.staging_buffer.handle, (VkImage)(uintptr_t)texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, num_regions, regions );

	// final transition after upload comleted
	record_image_layout_transition( command_buffer, (VkImage)(uintptr_t)texture->image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0 );
#else
	if ( vk.staging_buffer.size < (VkDeviceSize)buffer_size ) {
		vk_alloc_staging_buffer( buffer_size );
	}

	Com_Memcpy( vk.staging_buffer.ptr, pixels, buffer_size );

	command_buffer = begin_command_buffer();
	// record_buffer_memory_barrier( command_buffer, vk_world.staging_buffer, VK_WHOLE_SIZE, 0, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT );
	if ( update ) {
		record_image_layout_transition( command_buffer, (VkImage)(uintptr_t)texture->image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 0 );
	} else {
		record_image_layout_transition( command_buffer, (VkImage)(uintptr_t)texture->image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_HOST_BIT, 0 );
	}
	qvkCmdCopyBufferToImage( command_buffer, vk.staging_buffer.handle, (VkImage)(uintptr_t)texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, num_regions, regions );
	record_image_layout_transition( command_buffer, (VkImage)(uintptr_t)texture->image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0 );
	end_command_buffer( command_buffer, __func__ );
#endif
}


void vk_impl_UpdateTextureSampler( const rhiTexture_t *texture, rhiAddress_t address, bool mipmap ) {
	Vk_Sampler_Def sampler_def;
	VkDescriptorImageInfo image_info;
	VkWriteDescriptorSet descriptor_write;

	Com_Memset( &sampler_def, 0, sizeof( sampler_def ) );

	sampler_def.address_mode = vk_texture_address( address );

	if ( mipmap ) {
		sampler_def.gl_mag_filter = vk_texture_filter( vk_config.textureMag );
		sampler_def.gl_min_filter = vk_texture_filter( vk_config.textureMin );
	} else {
		sampler_def.gl_mag_filter = FILTER_LINEAR;
		sampler_def.gl_min_filter = FILTER_LINEAR;
		// no anisotropy without mipmaps
		sampler_def.noAnisotropy = qtrue;
	}

	image_info.sampler = vk_find_sampler( &sampler_def );
	image_info.imageView = (VkImageView)(uintptr_t)texture->view;
	image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_write.dstSet = (VkDescriptorSet)(uintptr_t)texture->binding;
	descriptor_write.dstBinding = 0;
	descriptor_write.dstArrayElement = 0;
	descriptor_write.descriptorCount = 1;
	descriptor_write.pNext = NULL;
	descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_write.pImageInfo = &image_info;
	descriptor_write.pBufferInfo = NULL;
	descriptor_write.pTexelBufferView = NULL;

	qvkUpdateDescriptorSets( vk.device, 1, &descriptor_write, 0, NULL );
}


void RHI_DestroyTexture( rhiTexture_t *texture ) {
	if ( texture->image ) {
		qvkDestroyImage( vk.device, (VkImage)(uintptr_t)texture->image, NULL );
		texture->image = 0;
	}
	if ( texture->view ) {
		qvkDestroyImageView( vk.device, (VkImageView)(uintptr_t)texture->view, NULL );
		texture->view = 0;
	}
	if ( texture->memory ) {
		qvkFreeMemory( vk.device, (VkDeviceMemory)(uintptr_t)texture->memory, NULL );
		texture->memory = 0;
	}
}

void RHI_BindTexture( uint32_t slot, const rhiTexture_t *texture ) {
	vk_update_descriptor( slot, (VkDescriptorSet)(uintptr_t)texture->binding );
}


static void set_shader_stage_desc( VkPipelineShaderStageCreateInfo *desc, VkShaderStageFlagBits stage, VkShaderModule shader_module, const char *entry ) {
	desc->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	desc->pNext = NULL;
	desc->flags = 0;
	desc->stage = stage;
	desc->module = shader_module;
	desc->pName = entry;
	desc->pSpecializationInfo = NULL;
}


#define FORMAT_DEPTH( format, r_bits, g_bits, b_bits ) case(VK_FORMAT_##format): *r = r_bits; *b = b_bits; *g = g_bits; return qtrue;
static qboolean vk_surface_format_color_depth( VkFormat format, int *r, int *g, int *b ) {
	switch ( format ) {
		// Common formats from https://vulkan.gpuinfo.org/listsurfaceformats.php
		FORMAT_DEPTH( B8G8R8A8_UNORM, 255, 255, 255 )
		FORMAT_DEPTH( B8G8R8A8_SRGB, 255, 255, 255 )
		FORMAT_DEPTH( A2B10G10R10_UNORM_PACK32, 1023, 1023, 1023 )
		FORMAT_DEPTH( R8G8B8A8_UNORM, 255, 255, 255 )
		FORMAT_DEPTH( R8G8B8A8_SRGB, 255, 255, 255 )
		FORMAT_DEPTH( A2R10G10B10_UNORM_PACK32, 1023, 1023, 1023 )
		FORMAT_DEPTH( R5G6B5_UNORM_PACK16, 31, 63, 31 )
		FORMAT_DEPTH( R8G8B8A8_SNORM, 255, 255, 255 )
		FORMAT_DEPTH( A8B8G8R8_UNORM_PACK32, 255, 255, 255 )
		FORMAT_DEPTH( A8B8G8R8_SNORM_PACK32, 255, 255, 255 )
		FORMAT_DEPTH( A8B8G8R8_SRGB_PACK32, 255, 255, 255 )
		FORMAT_DEPTH( R16G16B16A16_UNORM, 65535, 65535, 65535 )
		FORMAT_DEPTH( R16G16B16A16_SNORM, 65535, 65535, 65535 )
		FORMAT_DEPTH( B5G6R5_UNORM_PACK16, 31, 63, 31 )
		FORMAT_DEPTH( B8G8R8A8_SNORM, 255, 255, 255 )
		FORMAT_DEPTH( R4G4B4A4_UNORM_PACK16, 15, 15, 15 )
		FORMAT_DEPTH( B4G4R4A4_UNORM_PACK16, 15, 15, 15 )
		FORMAT_DEPTH( A1R5G5B5_UNORM_PACK16, 31, 31, 31 )
		FORMAT_DEPTH( R5G5B5A1_UNORM_PACK16, 31, 31, 31 )
		FORMAT_DEPTH( B5G5R5A1_UNORM_PACK16, 31, 31, 31 )
	default:
		*r = 255;
		*g = 255;
		*b = 255;
		return qfalse;
	}
}


void vk_create_post_process_pipeline( int program_index, uint32_t width, uint32_t height ) {
	VkPipelineShaderStageCreateInfo shader_stages[2];
	VkPipelineVertexInputStateCreateInfo vertex_input_state;
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state;
	VkPipelineRasterizationStateCreateInfo rasterization_state;
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state;
	VkPipelineViewportStateCreateInfo viewport_state;
	VkPipelineMultisampleStateCreateInfo multisample_state;
	VkPipelineColorBlendStateCreateInfo blend_state;
	VkPipelineColorBlendAttachmentState attachment_blend_state;
	VkGraphicsPipelineCreateInfo create_info;
	VkViewport viewport;
	VkRect2D scissor;
	VkSpecializationMapEntry spec_entries[11];
	VkSpecializationInfo frag_spec_info;
	VkPipeline *pipeline;
	VkShaderModule fsmodule;
	VkRenderPass renderpass;
	VkPipelineLayout layout;
	VkSampleCountFlagBits samples;
	const char *pipeline_name;
	qboolean blend;

	struct FragSpecData {
		float gamma;
		float overbright;
		float greyscale;
		float bloom_threshold;
		float bloom_intensity;
		int bloom_threshold_mode;
		int bloom_modulate;
		int dither;
		int depth_r;
		int depth_g;
		int depth_b;
	} frag_spec_data;

	switch ( program_index ) {
	case 1: // bloom extraction
		pipeline = &vk.bloom_extract_pipeline;
		fsmodule = vk.modules.bloom_fs;
		renderpass = vk.render_pass.bloom_extract;
		layout = vk.pipeline_layout_post_process;
		samples = VK_SAMPLE_COUNT_1_BIT;
		pipeline_name = "bloom extraction pipeline";
		blend = qfalse;
		break;
	case 2: // final bloom blend
		pipeline = &vk.bloom_blend_pipeline;
		fsmodule = vk.modules.blend_fs;
		renderpass = vk.render_pass.post_bloom;
		layout = vk.pipeline_layout_blend;
		samples = (VkSampleCountFlagBits)( vkSamples );
		pipeline_name = "bloom blend pipeline";
		blend = qtrue;
		break;
	case 3: // capture buffer extraction
		pipeline = &vk.capture_pipeline;
		fsmodule = vk.modules.gamma_fs;
		renderpass = vk.render_pass.capture;
		layout = vk.pipeline_layout_post_process;
		samples = VK_SAMPLE_COUNT_1_BIT;
		pipeline_name = "capture buffer pipeline";
		blend = qfalse;
		break;
	default: // gamma correction
		pipeline = &vk.gamma_pipeline;
		fsmodule = vk.modules.gamma_fs;
		renderpass = vk.render_pass.gamma;
		layout = vk.pipeline_layout_post_process;
		samples = VK_SAMPLE_COUNT_1_BIT;
		pipeline_name = "gamma-correction pipeline";
		blend = qfalse;
		break;
	}

	if ( *pipeline != VK_NULL_HANDLE ) {
		vk_wait_idle();
		qvkDestroyPipeline( vk.device, *pipeline, NULL );
		*pipeline = VK_NULL_HANDLE;
	}

	vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state.pNext = NULL;
	vertex_input_state.flags = 0;
	vertex_input_state.vertexBindingDescriptionCount = 0;
	vertex_input_state.pVertexBindingDescriptions = NULL;
	vertex_input_state.vertexAttributeDescriptionCount = 0;
	vertex_input_state.pVertexBindingDescriptions = NULL;

	// shaders
	set_shader_stage_desc( shader_stages + 0, VK_SHADER_STAGE_VERTEX_BIT, vk.modules.gamma_vs, "main" );
	set_shader_stage_desc( shader_stages + 1, VK_SHADER_STAGE_FRAGMENT_BIT, fsmodule, "main" );

	frag_spec_data.gamma = (float)( 1.0 / ( vk_post.gamma ) );
	frag_spec_data.overbright = (float)( 1 << vk.overbrightBits );
	frag_spec_data.greyscale = vk_post.greyscale;
	frag_spec_data.bloom_threshold = vk_post.bloomThreshold;
	frag_spec_data.bloom_intensity = vk_post.bloomIntensity;
	frag_spec_data.bloom_threshold_mode = vk_post.bloomThresholdMode;
	frag_spec_data.bloom_modulate = vk_post.bloomModulate;
	frag_spec_data.dither = vk_post.dither;

	if ( !vk_surface_format_color_depth( vk.present_format.format, &frag_spec_data.depth_r, &frag_spec_data.depth_g, &frag_spec_data.depth_b ) )
		vk_host.Print( rhiLog_t::Info, "Format %s not recognized, dither to assume 8bpc\n", vk_format_string( vk.base_format.format ) );

	spec_entries[0].constantID = 0;
	spec_entries[0].offset = offsetof( struct FragSpecData, gamma );
	spec_entries[0].size = sizeof( frag_spec_data.gamma );

	spec_entries[1].constantID = 1;
	spec_entries[1].offset = offsetof( struct FragSpecData, overbright );
	spec_entries[1].size = sizeof( frag_spec_data.overbright );

	spec_entries[2].constantID = 2;
	spec_entries[2].offset = offsetof( struct FragSpecData, greyscale );
	spec_entries[2].size = sizeof( frag_spec_data.greyscale );

	spec_entries[3].constantID = 3;
	spec_entries[3].offset = offsetof( struct FragSpecData, bloom_threshold );
	spec_entries[3].size = sizeof( frag_spec_data.bloom_threshold );

	spec_entries[4].constantID = 4;
	spec_entries[4].offset = offsetof( struct FragSpecData, bloom_intensity );
	spec_entries[4].size = sizeof( frag_spec_data.bloom_intensity );

	spec_entries[5].constantID = 5;
	spec_entries[5].offset = offsetof( struct FragSpecData, bloom_threshold_mode );
	spec_entries[5].size = sizeof( frag_spec_data.bloom_threshold_mode );

	spec_entries[6].constantID = 6;
	spec_entries[6].offset = offsetof( struct FragSpecData, bloom_modulate );
	spec_entries[6].size = sizeof( frag_spec_data.bloom_modulate );

	spec_entries[7].constantID = 7;
	spec_entries[7].offset = offsetof( struct FragSpecData, dither );
	spec_entries[7].size = sizeof( frag_spec_data.dither );

	spec_entries[8].constantID = 8;
	spec_entries[8].offset = offsetof( struct FragSpecData, depth_r );
	spec_entries[8].size = sizeof( frag_spec_data.depth_r );

	spec_entries[9].constantID = 9;
	spec_entries[9].offset = offsetof( struct FragSpecData, depth_g );
	spec_entries[9].size = sizeof( frag_spec_data.depth_g );

	spec_entries[10].constantID = 10;
	spec_entries[10].offset = offsetof( struct FragSpecData, depth_b );
	spec_entries[10].size = sizeof( frag_spec_data.depth_b );

	frag_spec_info.mapEntryCount = 11;
	frag_spec_info.pMapEntries = spec_entries;
	frag_spec_info.dataSize = sizeof( frag_spec_data );
	frag_spec_info.pData = &frag_spec_data;

	shader_stages[1].pSpecializationInfo = &frag_spec_info;

	//
	// Primitive assembly.
	//
	input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state.pNext = NULL;
	input_assembly_state.flags = 0;
	input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	input_assembly_state.primitiveRestartEnable = VK_FALSE;

	//
	// Viewport.
	//
	if ( program_index == 0 ) {
		// gamma correction
		viewport.x = (float)( 0.0 + vk.blitX0 );
		viewport.y = (float)( 0.0 + vk.blitY0 );
		viewport.width = (float)( vk_config.windowWidth - vk.blitX0 * 2 );
		viewport.height = (float)( vk_config.windowHeight - vk.blitY0 * 2 );
	} else {
		// other post-processing
		viewport.x = 0.0;
		viewport.y = 0.0;
		viewport.width = (float)( width );
		viewport.height = (float)( height );
	}

	viewport.minDepth = 0.0;
	viewport.maxDepth = 1.0;

	scissor.offset.x = (int32_t)( viewport.x );
	scissor.offset.y = (int32_t)( viewport.y );
	scissor.extent.width = (uint32_t)( viewport.width );
	scissor.extent.height = (uint32_t)( viewport.height );

	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.pNext = NULL;
	viewport_state.flags = 0;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor;

	//
	// Rasterization.
	//
	rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state.pNext = NULL;
	rasterization_state.flags = 0;
	rasterization_state.depthClampEnable = VK_FALSE;
	rasterization_state.rasterizerDiscardEnable = VK_FALSE;
	rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
	//rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT; // VK_CULL_MODE_NONE;
	rasterization_state.cullMode = VK_CULL_MODE_NONE;
	rasterization_state.frontFace = VK_FRONT_FACE_CLOCKWISE; // Q3 defaults to clockwise vertex order
	rasterization_state.depthBiasEnable = VK_FALSE;
	rasterization_state.depthBiasConstantFactor = 0.0f;
	rasterization_state.depthBiasClamp = 0.0f;
	rasterization_state.depthBiasSlopeFactor = 0.0f;
	rasterization_state.lineWidth = 1.0f;

	multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state.pNext = NULL;
	multisample_state.flags = 0;
	multisample_state.rasterizationSamples = samples;
	multisample_state.sampleShadingEnable = VK_FALSE;
	multisample_state.minSampleShading = 1.0f;
	multisample_state.pSampleMask = NULL;
	multisample_state.alphaToCoverageEnable = VK_FALSE;
	multisample_state.alphaToOneEnable = VK_FALSE;

	Com_Memset( &attachment_blend_state, 0, sizeof( attachment_blend_state ) );
	attachment_blend_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	if ( blend ) {
		attachment_blend_state.blendEnable = VK_TRUE;
		attachment_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		attachment_blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	} else {
		attachment_blend_state.blendEnable = VK_FALSE;
	}

	blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend_state.pNext = NULL;
	blend_state.flags = 0;
	blend_state.logicOpEnable = VK_FALSE;
	blend_state.logicOp = VK_LOGIC_OP_COPY;
	blend_state.attachmentCount = 1;
	blend_state.pAttachments = &attachment_blend_state;
	blend_state.blendConstants[0] = 0.0f;
	blend_state.blendConstants[1] = 0.0f;
	blend_state.blendConstants[2] = 0.0f;
	blend_state.blendConstants[3] = 0.0f;

	Com_Memset( &depth_stencil_state, 0, sizeof( depth_stencil_state ) );

	depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_state.pNext = NULL;
	depth_stencil_state.flags = 0;
	depth_stencil_state.depthTestEnable = VK_FALSE;
	depth_stencil_state.depthWriteEnable = VK_FALSE;
	depth_stencil_state.depthCompareOp = VK_COMPARE_OP_NEVER;
	depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
	depth_stencil_state.stencilTestEnable = VK_FALSE;
	depth_stencil_state.minDepthBounds = 0.0f;
	depth_stencil_state.maxDepthBounds = 1.0f;

	create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.stageCount = 2;
	create_info.pStages = shader_stages;
	create_info.pVertexInputState = &vertex_input_state;
	create_info.pInputAssemblyState = &input_assembly_state;
	create_info.pTessellationState = NULL;
	create_info.pViewportState = &viewport_state;
	create_info.pRasterizationState = &rasterization_state;
	create_info.pMultisampleState = &multisample_state;
	create_info.pDepthStencilState = ( program_index == 2 ) ? &depth_stencil_state : NULL;
	create_info.pDepthStencilState = &depth_stencil_state;
	create_info.pColorBlendState = &blend_state;
	create_info.pDynamicState = NULL;
	create_info.layout = layout;
	create_info.renderPass = renderpass;
	create_info.subpass = 0;
	create_info.basePipelineHandle = VK_NULL_HANDLE;
	create_info.basePipelineIndex = -1;

	VK_CHECK( qvkCreateGraphicsPipelines( vk.device, VK_NULL_HANDLE, 1, &create_info, NULL, pipeline ) );

	SET_OBJECT_NAME( *pipeline, pipeline_name, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT );
}


void vk_create_blur_pipeline( uint32_t index, uint32_t width, uint32_t height, qboolean horizontal_pass ) {
	VkPipelineShaderStageCreateInfo shader_stages[2];
	VkPipelineVertexInputStateCreateInfo vertex_input_state;
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state;
	VkPipelineRasterizationStateCreateInfo rasterization_state;
	VkPipelineViewportStateCreateInfo viewport_state;
	VkPipelineMultisampleStateCreateInfo multisample_state;
	VkPipelineColorBlendStateCreateInfo blend_state;
	VkPipelineColorBlendAttachmentState attachment_blend_state;
	VkGraphicsPipelineCreateInfo create_info;
	VkViewport viewport;
	VkRect2D scissor;
	float frag_spec_data[3]; // x-offset, y-offset, correction
	VkSpecializationMapEntry spec_entries[3];
	VkSpecializationInfo frag_spec_info;
	VkPipeline *pipeline;

	pipeline = &vk.blur_pipeline[index];

	if ( *pipeline != VK_NULL_HANDLE ) {
		vk_wait_idle();
		qvkDestroyPipeline( vk.device, *pipeline, NULL );
		*pipeline = VK_NULL_HANDLE;
	}

	vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state.pNext = NULL;
	vertex_input_state.flags = 0;
	vertex_input_state.vertexBindingDescriptionCount = 0;
	vertex_input_state.pVertexBindingDescriptions = NULL;
	vertex_input_state.vertexAttributeDescriptionCount = 0;
	vertex_input_state.pVertexBindingDescriptions = NULL;

	// shaders
	set_shader_stage_desc( shader_stages + 0, VK_SHADER_STAGE_VERTEX_BIT, vk.modules.gamma_vs, "main" );
	set_shader_stage_desc( shader_stages + 1, VK_SHADER_STAGE_FRAGMENT_BIT, vk.modules.blur_fs, "main" );

	frag_spec_data[0] = (float)( 1.2 / (float)width ); // x offset
	frag_spec_data[1] = (float)( 1.2 / (float)height ); // y offset
	frag_spec_data[2] = 1.0; // intensity?

	if ( horizontal_pass ) {
		frag_spec_data[1] = 0.0;
	} else {
		frag_spec_data[0] = 0.0;
	}

	spec_entries[0].constantID = 0;
	spec_entries[0].offset = 0 * sizeof( float );
	spec_entries[0].size = sizeof( float );

	spec_entries[1].constantID = 1;
	spec_entries[1].offset = 1 * sizeof( float );
	spec_entries[1].size = sizeof( float );

	spec_entries[2].constantID = 2;
	spec_entries[2].offset = 2 * sizeof( float );
	spec_entries[2].size = sizeof( float );

	frag_spec_info.mapEntryCount = 3;
	frag_spec_info.pMapEntries = spec_entries;
	frag_spec_info.dataSize = 3 * sizeof( float );
	frag_spec_info.pData = &frag_spec_data[0];

	shader_stages[1].pSpecializationInfo = &frag_spec_info;

	//
	// Primitive assembly.
	//
	input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state.pNext = NULL;
	input_assembly_state.flags = 0;
	input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	input_assembly_state.primitiveRestartEnable = VK_FALSE;

	//
	// Viewport.
	//
	viewport.x = 0.0;
	viewport.y = 0.0;
	viewport.width = (float)( width );
	viewport.height = (float)( height );
	viewport.minDepth = 0.0;
	viewport.maxDepth = 1.0;

	scissor.offset.x = (int32_t)( viewport.x );
	scissor.offset.y = (int32_t)( viewport.y );
	scissor.extent.width = (uint32_t)( viewport.width );
	scissor.extent.height = (uint32_t)( viewport.height );

	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.pNext = NULL;
	viewport_state.flags = 0;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor;

	//
	// Rasterization.
	//
	rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state.pNext = NULL;
	rasterization_state.flags = 0;
	rasterization_state.depthClampEnable = VK_FALSE;
	rasterization_state.rasterizerDiscardEnable = VK_FALSE;
	rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
	//rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT; // VK_CULL_MODE_NONE;
	rasterization_state.cullMode = VK_CULL_MODE_NONE;
	rasterization_state.frontFace = VK_FRONT_FACE_CLOCKWISE; // Q3 defaults to clockwise vertex order
	rasterization_state.depthBiasEnable = VK_FALSE;
	rasterization_state.depthBiasConstantFactor = 0.0f;
	rasterization_state.depthBiasClamp = 0.0f;
	rasterization_state.depthBiasSlopeFactor = 0.0f;
	rasterization_state.lineWidth = 1.0f;

	multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state.pNext = NULL;
	multisample_state.flags = 0;
	multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisample_state.sampleShadingEnable = VK_FALSE;
	multisample_state.minSampleShading = 1.0f;
	multisample_state.pSampleMask = NULL;
	multisample_state.alphaToCoverageEnable = VK_FALSE;
	multisample_state.alphaToOneEnable = VK_FALSE;

	Com_Memset( &attachment_blend_state, 0, sizeof( attachment_blend_state ) );
	attachment_blend_state.blendEnable = VK_FALSE;
	attachment_blend_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend_state.pNext = NULL;
	blend_state.flags = 0;
	blend_state.logicOpEnable = VK_FALSE;
	blend_state.logicOp = VK_LOGIC_OP_COPY;
	blend_state.attachmentCount = 1;
	blend_state.pAttachments = &attachment_blend_state;
	blend_state.blendConstants[0] = 0.0f;
	blend_state.blendConstants[1] = 0.0f;
	blend_state.blendConstants[2] = 0.0f;
	blend_state.blendConstants[3] = 0.0f;

	create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.stageCount = 2;
	create_info.pStages = shader_stages;
	create_info.pVertexInputState = &vertex_input_state;
	create_info.pInputAssemblyState = &input_assembly_state;
	create_info.pTessellationState = NULL;
	create_info.pViewportState = &viewport_state;
	create_info.pRasterizationState = &rasterization_state;
	create_info.pMultisampleState = &multisample_state;
	create_info.pDepthStencilState = NULL;
	create_info.pColorBlendState = &blend_state;
	create_info.pDynamicState = NULL;
	create_info.layout = vk.pipeline_layout_post_process; // one input attachment
	create_info.renderPass = vk.render_pass.blur[index];
	create_info.subpass = 0;
	create_info.basePipelineHandle = VK_NULL_HANDLE;
	create_info.basePipelineIndex = -1;

	VK_CHECK( qvkCreateGraphicsPipelines( vk.device, VK_NULL_HANDLE, 1, &create_info, NULL, pipeline ) );

	SET_OBJECT_NAME( *pipeline, va( "%s blur pipeline %i", horizontal_pass ? "horizontal" : "vertical", index / 2 + 1 ), VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT );
}


static VkVertexInputBindingDescription bindings[8];
static VkVertexInputAttributeDescription attribs[8];
static uint32_t num_binds;
static uint32_t num_attrs;

static void push_bind( uint32_t binding, uint32_t stride ) {
	bindings[num_binds].binding = binding;
	bindings[num_binds].stride = stride;
	bindings[num_binds].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	num_binds++;
}

static void push_attr( uint32_t location, uint32_t binding, VkFormat format ) {
	attribs[num_attrs].location = location;
	attribs[num_attrs].binding = binding;
	attribs[num_attrs].format = format;
	attribs[num_attrs].offset = 0;
	num_attrs++;
}


VkPipeline create_pipeline( const rhiPipelineDesc_t *def, renderPass_t renderPassIndex, uint32_t def_index ) {
	VkShaderModule *vs_module = NULL;
	VkShaderModule *fs_module = NULL;
	//int32_t vert_spec_data[1]; // clippping
	floatint_t frag_spec_data[11]; // 0:alpha-test-func, 1:alpha-test-value, 2:depth-fragment, 3:alpha-to-coverage, 4:color_mode, 5:abs_light, 6:multitexture mode, 7:discard mode, 8: ident.color, 9 - ident.alpha, 10 - acff
	VkSpecializationMapEntry spec_entries[12];
	//VkSpecializationInfo vert_spec_info;
	VkSpecializationInfo frag_spec_info;
	VkPipelineVertexInputStateCreateInfo vertex_input_state;
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state;
	VkPipelineRasterizationStateCreateInfo rasterization_state;
	VkPipelineViewportStateCreateInfo viewport_state;
	VkPipelineMultisampleStateCreateInfo multisample_state;
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state;
	VkPipelineColorBlendStateCreateInfo blend_state;
	VkPipelineColorBlendAttachmentState attachment_blend_state;
	VkPipelineDynamicStateCreateInfo dynamic_state;
	VkDynamicState dynamic_state_array[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkGraphicsPipelineCreateInfo create_info;
	VkPipeline pipeline;
	VkPipelineShaderStageCreateInfo shader_stages[2];
	VkBool32 alphaToCoverage = VK_FALSE;
	unsigned int atest_bits;
	unsigned int state_bits = def->state_bits;

	switch ( def->shader_type ) {

	case TYPE_PBR_BAKED:
		vs_module = &vk.modules.pbr_baked_vs;
		fs_module = &vk.modules.pbr_baked_fs;
		break;

	case TYPE_PBR:
		vs_module = &vk.modules.pbr_vs;
		fs_module = &vk.modules.pbr_fs;
		break;

	case TYPE_SIGNLE_TEXTURE_LIGHTING:
		vs_module = &vk.modules.vert.light[0];
		fs_module = &vk.modules.frag.light[0][0];
		break;

	case TYPE_SIGNLE_TEXTURE_LIGHTING_LINEAR:
		vs_module = &vk.modules.vert.light[0];
		fs_module = &vk.modules.frag.light[1][0];
		break;

	case TYPE_SIGNLE_TEXTURE_DF:
		state_bits |= GLS_DEPTHMASK_TRUE;
		vs_module = &vk.modules.vert.ident1[0][0][0];
		fs_module = &vk.modules.frag.gen0_df;
		break;

	case TYPE_SIGNLE_TEXTURE_FIXED_COLOR:
		vs_module = &vk.modules.vert.fixed[0][0][0];
		fs_module = &vk.modules.frag.fixed[0][0];
		break;

	case TYPE_SIGNLE_TEXTURE_FIXED_COLOR_ENV:
		vs_module = &vk.modules.vert.fixed[0][1][0];
		fs_module = &vk.modules.frag.fixed[0][0];
		break;

	case TYPE_SIGNLE_TEXTURE_ENT_COLOR:
		vs_module = &vk.modules.vert.fixed[0][0][0];
		fs_module = &vk.modules.frag.ent[0][0];
		break;

	case TYPE_SIGNLE_TEXTURE_ENT_COLOR_ENV:
		vs_module = &vk.modules.vert.fixed[0][1][0];
		fs_module = &vk.modules.frag.ent[0][0];
		break;

	case TYPE_SIGNLE_TEXTURE:
		vs_module = &vk.modules.vert.gen[0][0][0][0];
		fs_module = &vk.modules.frag.gen[0][0][0];
		break;

	case TYPE_SIGNLE_TEXTURE_ENV:
		vs_module = &vk.modules.vert.gen[0][0][1][0];
		fs_module = &vk.modules.frag.gen[0][0][0];
		break;

	case TYPE_SIGNLE_TEXTURE_IDENTITY:
		vs_module = &vk.modules.vert.ident1[0][0][0];
		fs_module = &vk.modules.frag.ident1[0][0];
		break;

	case TYPE_SIGNLE_TEXTURE_IDENTITY_ENV:
		vs_module = &vk.modules.vert.ident1[0][1][0];
		fs_module = &vk.modules.frag.ident1[0][0];
		break;

	case TYPE_MULTI_TEXTURE_ADD2_IDENTITY:
	case TYPE_MULTI_TEXTURE_MUL2_IDENTITY:
		vs_module = &vk.modules.vert.ident1[1][0][0];
		fs_module = &vk.modules.frag.ident1[1][0];
		break;

	case TYPE_MULTI_TEXTURE_ADD2_IDENTITY_ENV:
	case TYPE_MULTI_TEXTURE_MUL2_IDENTITY_ENV:
		vs_module = &vk.modules.vert.ident1[1][1][0];
		fs_module = &vk.modules.frag.ident1[1][0];
		break;

	case TYPE_MULTI_TEXTURE_ADD2_FIXED_COLOR:
	case TYPE_MULTI_TEXTURE_MUL2_FIXED_COLOR:
		vs_module = &vk.modules.vert.fixed[1][0][0];
		fs_module = &vk.modules.frag.fixed[1][0];
		break;

	case TYPE_MULTI_TEXTURE_ADD2_FIXED_COLOR_ENV:
	case TYPE_MULTI_TEXTURE_MUL2_FIXED_COLOR_ENV:
		vs_module = &vk.modules.vert.fixed[1][1][0];
		fs_module = &vk.modules.frag.fixed[1][0];
		break;

	case TYPE_MULTI_TEXTURE_MUL2:
	case TYPE_MULTI_TEXTURE_ADD2_1_1:
	case TYPE_MULTI_TEXTURE_ADD2:
		vs_module = &vk.modules.vert.gen[1][0][0][0];
		fs_module = &vk.modules.frag.gen[1][0][0];
		break;

	case TYPE_MULTI_TEXTURE_MUL2_ENV:
	case TYPE_MULTI_TEXTURE_ADD2_1_1_ENV:
	case TYPE_MULTI_TEXTURE_ADD2_ENV:
		vs_module = &vk.modules.vert.gen[1][0][1][0];
		fs_module = &vk.modules.frag.gen[1][0][0];
		break;

	case TYPE_MULTI_TEXTURE_MUL3:
	case TYPE_MULTI_TEXTURE_ADD3_1_1:
	case TYPE_MULTI_TEXTURE_ADD3:
		vs_module = &vk.modules.vert.gen[2][0][0][0];
		fs_module = &vk.modules.frag.gen[2][0][0];
		break;

	case TYPE_MULTI_TEXTURE_MUL3_ENV:
	case TYPE_MULTI_TEXTURE_ADD3_1_1_ENV:
	case TYPE_MULTI_TEXTURE_ADD3_ENV:
		vs_module = &vk.modules.vert.gen[2][0][1][0];
		fs_module = &vk.modules.frag.gen[2][0][0];
		break;

	case TYPE_BLEND2_ADD:
	case TYPE_BLEND2_MUL:
	case TYPE_BLEND2_ALPHA:
	case TYPE_BLEND2_ONE_MINUS_ALPHA:
	case TYPE_BLEND2_MIX_ALPHA:
	case TYPE_BLEND2_MIX_ONE_MINUS_ALPHA:
	case TYPE_BLEND2_DST_COLOR_SRC_ALPHA:
		vs_module = &vk.modules.vert.gen[1][1][0][0];
		fs_module = &vk.modules.frag.gen[1][1][0];
		break;

	case TYPE_BLEND2_ADD_ENV:
	case TYPE_BLEND2_MUL_ENV:
	case TYPE_BLEND2_ALPHA_ENV:
	case TYPE_BLEND2_ONE_MINUS_ALPHA_ENV:
	case TYPE_BLEND2_MIX_ALPHA_ENV:
	case TYPE_BLEND2_MIX_ONE_MINUS_ALPHA_ENV:
	case TYPE_BLEND2_DST_COLOR_SRC_ALPHA_ENV:
		vs_module = &vk.modules.vert.gen[1][1][1][0];
		fs_module = &vk.modules.frag.gen[1][1][0];
		break;

	case TYPE_BLEND3_ADD:
	case TYPE_BLEND3_MUL:
	case TYPE_BLEND3_ALPHA:
	case TYPE_BLEND3_ONE_MINUS_ALPHA:
	case TYPE_BLEND3_MIX_ALPHA:
	case TYPE_BLEND3_MIX_ONE_MINUS_ALPHA:
	case TYPE_BLEND3_DST_COLOR_SRC_ALPHA:
		vs_module = &vk.modules.vert.gen[2][1][0][0];
		fs_module = &vk.modules.frag.gen[2][1][0];
		break;

	case TYPE_BLEND3_ADD_ENV:
	case TYPE_BLEND3_MUL_ENV:
	case TYPE_BLEND3_ALPHA_ENV:
	case TYPE_BLEND3_ONE_MINUS_ALPHA_ENV:
	case TYPE_BLEND3_MIX_ALPHA_ENV:
	case TYPE_BLEND3_MIX_ONE_MINUS_ALPHA_ENV:
	case TYPE_BLEND3_DST_COLOR_SRC_ALPHA_ENV:
		vs_module = &vk.modules.vert.gen[2][1][1][0];
		fs_module = &vk.modules.frag.gen[2][1][0];
		break;

	case TYPE_COLOR_BLACK:
	case TYPE_COLOR_WHITE:
	case TYPE_COLOR_GREEN:
	case TYPE_COLOR_RED:
		vs_module = &vk.modules.color_vs;
		fs_module = &vk.modules.color_fs;
		break;

	case TYPE_FOG_ONLY:
		vs_module = &vk.modules.fog_vs;
		fs_module = &vk.modules.fog_fs;
		break;

	case TYPE_DOT:
		vs_module = &vk.modules.dot_vs;
		fs_module = &vk.modules.dot_fs;
		break;

	default:
		vk_fail( ERR_DROP, rhiStatus_t::Error, "create_pipeline: unknown shader type %i\n", def->shader_type );
	}

	if ( def->fog_stage ) {
		switch ( def->shader_type ) {
		case TYPE_FOG_ONLY:
		case TYPE_DOT:
		case TYPE_SIGNLE_TEXTURE_DF:
		case TYPE_COLOR_BLACK:
		case TYPE_COLOR_WHITE:
		case TYPE_COLOR_GREEN:
		case TYPE_COLOR_RED:
			break;
		default:
			// switch to fogged modules
			vs_module++;
			fs_module++;
			break;
		}
	}

	set_shader_stage_desc( shader_stages + 0, VK_SHADER_STAGE_VERTEX_BIT, *vs_module, "main" );
	set_shader_stage_desc( shader_stages + 1, VK_SHADER_STAGE_FRAGMENT_BIT, *fs_module, "main" );

	//Com_Memset( vert_spec_data, 0, sizeof( vert_spec_data ) );
	Com_Memset( frag_spec_data, 0, sizeof( frag_spec_data ) );

	//vert_spec_data[0] = def->clipping_plane ? 1 : 0;

	// fragment shader specialization data
	atest_bits = state_bits & GLS_ATEST_BITS;
	switch ( atest_bits ) {
	case GLS_ATEST_GT_0:
		frag_spec_data[0].i = 1; // not equal
		frag_spec_data[1].f = 0.0f;
		break;
	case GLS_ATEST_LT_80:
		frag_spec_data[0].i = 2; // less than
		frag_spec_data[1].f = 0.5f;
		break;
	case GLS_ATEST_GE_80:
		frag_spec_data[0].i = 3; // greater or equal
		frag_spec_data[1].f = 0.5f;
		break;
	default:
		frag_spec_data[0].i = 0;
		frag_spec_data[1].f = 0.0f;
		break;
	};

	// depth fragment threshold
	frag_spec_data[2].f = 0.85f;

	// constant color
	switch ( def->shader_type ) {
	default:
		frag_spec_data[4].i = 0;
		break;
	case TYPE_COLOR_WHITE:
		frag_spec_data[4].i = 1;
		break;
	case TYPE_COLOR_GREEN:
		frag_spec_data[4].i = 2;
		break;
	case TYPE_COLOR_RED:
		frag_spec_data[4].i = 3;
		break;
	}

	// abs lighting
	switch ( def->shader_type ) {
	case TYPE_SIGNLE_TEXTURE_LIGHTING:
	case TYPE_SIGNLE_TEXTURE_LIGHTING_LINEAR:
		frag_spec_data[5].i = def->abs_light ? 1 : 0;
	default:
		break;
	}

	// multutexture mode
	switch ( def->shader_type ) {
	case TYPE_MULTI_TEXTURE_MUL2_IDENTITY:
	case TYPE_MULTI_TEXTURE_MUL2_IDENTITY_ENV:
	case TYPE_MULTI_TEXTURE_MUL2_FIXED_COLOR:
	case TYPE_MULTI_TEXTURE_MUL2_FIXED_COLOR_ENV:
	case TYPE_MULTI_TEXTURE_MUL2:
	case TYPE_MULTI_TEXTURE_MUL2_ENV:
	case TYPE_MULTI_TEXTURE_MUL3:
	case TYPE_MULTI_TEXTURE_MUL3_ENV:
	case TYPE_BLEND2_MUL:
	case TYPE_BLEND2_MUL_ENV:
	case TYPE_BLEND3_MUL:
	case TYPE_BLEND3_MUL_ENV:
		frag_spec_data[6].i = 0;
		break;

	case TYPE_MULTI_TEXTURE_ADD2_IDENTITY:
	case TYPE_MULTI_TEXTURE_ADD2_IDENTITY_ENV:
	case TYPE_MULTI_TEXTURE_ADD2_FIXED_COLOR:
	case TYPE_MULTI_TEXTURE_ADD2_FIXED_COLOR_ENV:
	case TYPE_MULTI_TEXTURE_ADD2_1_1:
	case TYPE_MULTI_TEXTURE_ADD2_1_1_ENV:
	case TYPE_MULTI_TEXTURE_ADD3_1_1:
	case TYPE_MULTI_TEXTURE_ADD3_1_1_ENV:
		frag_spec_data[6].i = 1;
		break;

	case TYPE_MULTI_TEXTURE_ADD2:
	case TYPE_MULTI_TEXTURE_ADD2_ENV:
	case TYPE_MULTI_TEXTURE_ADD3:
	case TYPE_MULTI_TEXTURE_ADD3_ENV:
	case TYPE_BLEND2_ADD:
	case TYPE_BLEND2_ADD_ENV:
	case TYPE_BLEND3_ADD:
	case TYPE_BLEND3_ADD_ENV:
		frag_spec_data[6].i = 2;
		break;

	case TYPE_BLEND2_ALPHA:
	case TYPE_BLEND2_ALPHA_ENV:
	case TYPE_BLEND3_ALPHA:
	case TYPE_BLEND3_ALPHA_ENV:
		frag_spec_data[6].i = 3;
		break;

	case TYPE_BLEND2_ONE_MINUS_ALPHA:
	case TYPE_BLEND2_ONE_MINUS_ALPHA_ENV:
	case TYPE_BLEND3_ONE_MINUS_ALPHA:
	case TYPE_BLEND3_ONE_MINUS_ALPHA_ENV:
		frag_spec_data[6].i = 4;
		break;

	case TYPE_BLEND2_MIX_ALPHA:
	case TYPE_BLEND2_MIX_ALPHA_ENV:
	case TYPE_BLEND3_MIX_ALPHA:
	case TYPE_BLEND3_MIX_ALPHA_ENV:
		frag_spec_data[6].i = 5;
		break;

	case TYPE_BLEND2_MIX_ONE_MINUS_ALPHA:
	case TYPE_BLEND2_MIX_ONE_MINUS_ALPHA_ENV:
	case TYPE_BLEND3_MIX_ONE_MINUS_ALPHA:
	case TYPE_BLEND3_MIX_ONE_MINUS_ALPHA_ENV:
		frag_spec_data[6].i = 6;
		break;

	case TYPE_BLEND2_DST_COLOR_SRC_ALPHA:
	case TYPE_BLEND2_DST_COLOR_SRC_ALPHA_ENV:
	case TYPE_BLEND3_DST_COLOR_SRC_ALPHA:
	case TYPE_BLEND3_DST_COLOR_SRC_ALPHA_ENV:
		frag_spec_data[6].i = 7;
		break;

	default:
		break;
	}

	frag_spec_data[8].f = (float)( ( (float)def->color.rgb ) / 255.0 );
	frag_spec_data[9].f = (float)( ( (float)def->color.alpha ) / 255.0 );

	if ( def->fog_stage ) {
		frag_spec_data[10].i = def->acff;
	} else {
		frag_spec_data[10].i = 0;
	}

	//
	// vertex module specialization data
	//
#if 0
	spec_entries[0].constantID = 0; // clip_plane
	spec_entries[0].offset = 0 * sizeof( int32_t );
	spec_entries[0].size = sizeof( int32_t );

	vert_spec_info.mapEntryCount = 1;
	vert_spec_info.pMapEntries = spec_entries + 0;
	vert_spec_info.dataSize = 1 * sizeof( int32_t );
	vert_spec_info.pData = &vert_spec_data[0];
	shader_stages[0].pSpecializationInfo = &vert_spec_info;
#endif
	shader_stages[0].pSpecializationInfo = NULL;

	//
	// fragment module specialization data
	//

	spec_entries[1].constantID = 0; // alpha-test-function
	spec_entries[1].offset = 0 * sizeof( int32_t );
	spec_entries[1].size = sizeof( int32_t );

	spec_entries[2].constantID = 1; // alpha-test-value
	spec_entries[2].offset = 1 * sizeof( int32_t );
	spec_entries[2].size = sizeof( float );

	spec_entries[3].constantID = 2; // depth-fragment
	spec_entries[3].offset = 2 * sizeof( int32_t );
	spec_entries[3].size = sizeof( float );

	spec_entries[4].constantID = 3; // alpha-to-coverage
	spec_entries[4].offset = 3 * sizeof( int32_t );
	spec_entries[4].size = sizeof( int32_t );

	spec_entries[5].constantID = 4; // color_mode
	spec_entries[5].offset = 4 * sizeof( int32_t );
	spec_entries[5].size = sizeof( int32_t );

	spec_entries[6].constantID = 5; // abs_light
	spec_entries[6].offset = 5 * sizeof( int32_t );
	spec_entries[6].size = sizeof( int32_t );

	spec_entries[7].constantID = 6; // multitexture mode
	spec_entries[7].offset = 6 * sizeof( int32_t );
	spec_entries[7].size = sizeof( int32_t );

	spec_entries[8].constantID = 7; // discard mode
	spec_entries[8].offset = 7 * sizeof( int32_t );
	spec_entries[8].size = sizeof( int32_t );

	spec_entries[9].constantID = 8; // fixed color
	spec_entries[9].offset = 8 * sizeof( int32_t );
	spec_entries[9].size = sizeof( float );

	spec_entries[10].constantID = 9; // fixed alpha
	spec_entries[10].offset = 9 * sizeof( int32_t );
	spec_entries[10].size = sizeof( float );

	spec_entries[11].constantID = 10; // acff
	spec_entries[11].offset = 10 * sizeof( int32_t );
	spec_entries[11].size = sizeof( int32_t );

	frag_spec_info.mapEntryCount = 11;
	frag_spec_info.pMapEntries = spec_entries + 1;
	frag_spec_info.dataSize = sizeof( int32_t ) * 11;
	frag_spec_info.pData = &frag_spec_data[0];
	shader_stages[1].pSpecializationInfo = &frag_spec_info;

	//
	// Vertex input
	//
	num_binds = num_attrs = 0;
	switch ( def->shader_type ) {

	case TYPE_FOG_ONLY:
	case TYPE_DOT:
		push_bind( 0, sizeof( vec4_t ) ); // xyz array
		push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
		break;

	case TYPE_COLOR_BLACK:
	case TYPE_COLOR_WHITE:
	case TYPE_COLOR_GREEN:
	case TYPE_COLOR_RED:
		push_bind( 0, sizeof( vec4_t ) ); // xyz array
		push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
		break;

	case TYPE_SIGNLE_TEXTURE_DF:
	case TYPE_SIGNLE_TEXTURE_IDENTITY:
	case TYPE_SIGNLE_TEXTURE_FIXED_COLOR:
	case TYPE_SIGNLE_TEXTURE_ENT_COLOR:
		push_bind( 0, sizeof( vec4_t ) ); // xyz array
		push_bind( 2, sizeof( vec2_t ) ); // st0 array
		push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
		push_attr( 2, 2, VK_FORMAT_R32G32_SFLOAT );
		break;

	case TYPE_SIGNLE_TEXTURE:
		push_bind( 0, sizeof( vec4_t ) ); // xyz array
		push_bind( 1, sizeof( color4ub_t ) ); // color array
		push_bind( 2, sizeof( vec2_t ) ); // st0 array
		push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
		push_attr( 1, 1, VK_FORMAT_R8G8B8A8_UNORM );
		push_attr( 2, 2, VK_FORMAT_R32G32_SFLOAT );
		break;

	case TYPE_SIGNLE_TEXTURE_ENV:
		push_bind( 0, sizeof( vec4_t ) ); // xyz array
		push_bind( 1, sizeof( color4ub_t ) ); // color array
		//push_bind( 2, sizeof( vec2_t ) );					// st0 array
		push_bind( 5, sizeof( vec4_t ) ); // normals
		push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
		push_attr( 1, 1, VK_FORMAT_R8G8B8A8_UNORM );
		//push_attr( 2, 2, VK_FORMAT_R8G8B8A8_UNORM );
		push_attr( 5, 5, VK_FORMAT_R32G32B32A32_SFLOAT );
		break;

	case TYPE_SIGNLE_TEXTURE_IDENTITY_ENV:
	case TYPE_SIGNLE_TEXTURE_FIXED_COLOR_ENV:
	case TYPE_SIGNLE_TEXTURE_ENT_COLOR_ENV:
		push_bind( 0, sizeof( vec4_t ) ); // xyz array
		push_bind( 5, sizeof( vec4_t ) ); // normals
		push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
		push_attr( 5, 5, VK_FORMAT_R32G32B32A32_SFLOAT );
		break;

	case TYPE_PBR_BAKED:
	case TYPE_PBR:
		if ( def->shader_type == TYPE_PBR_BAKED ) {
			push_bind( 3, sizeof( vec2_t ) );
			push_attr( 4, 3, VK_FORMAT_R32G32_SFLOAT );
		}
		push_bind( 0, sizeof( vec4_t ) );
		push_bind( 2, sizeof( vec2_t ) );
		push_bind( 5, sizeof( vec4_t ) );
		push_bind( 6, sizeof( vec4_t ) );
		push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
		push_attr( 1, 2, VK_FORMAT_R32G32_SFLOAT );
		push_attr( 2, 5, VK_FORMAT_R32G32B32A32_SFLOAT );
		push_attr( 3, 6, VK_FORMAT_R32G32B32A32_SFLOAT );
		break;

	case TYPE_SIGNLE_TEXTURE_LIGHTING:
	case TYPE_SIGNLE_TEXTURE_LIGHTING_LINEAR:
		push_bind( 0, sizeof( vec4_t ) ); // xyz array
		push_bind( 1, sizeof( vec2_t ) ); // st0 array
		push_bind( 2, sizeof( vec4_t ) ); // normals array
		push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
		push_attr( 1, 1, VK_FORMAT_R32G32_SFLOAT );
		push_attr( 2, 2, VK_FORMAT_R32G32B32A32_SFLOAT );
		break;

	case TYPE_MULTI_TEXTURE_MUL2_IDENTITY:
	case TYPE_MULTI_TEXTURE_ADD2_IDENTITY:
	case TYPE_MULTI_TEXTURE_MUL2_FIXED_COLOR:
	case TYPE_MULTI_TEXTURE_ADD2_FIXED_COLOR:
		push_bind( 0, sizeof( vec4_t ) ); // xyz array
		push_bind( 2, sizeof( vec2_t ) ); // st0 array
		push_bind( 3, sizeof( vec2_t ) ); // st1 array
		push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
		push_attr( 2, 2, VK_FORMAT_R32G32_SFLOAT );
		push_attr( 3, 3, VK_FORMAT_R32G32_SFLOAT );
		break;

	case TYPE_MULTI_TEXTURE_MUL2_IDENTITY_ENV:
	case TYPE_MULTI_TEXTURE_ADD2_IDENTITY_ENV:
	case TYPE_MULTI_TEXTURE_MUL2_FIXED_COLOR_ENV:
	case TYPE_MULTI_TEXTURE_ADD2_FIXED_COLOR_ENV:
		push_bind( 0, sizeof( vec4_t ) ); // xyz array
		push_bind( 3, sizeof( vec2_t ) ); // st1 array
		push_bind( 5, sizeof( vec4_t ) ); // normals
		push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
		push_attr( 3, 3, VK_FORMAT_R32G32_SFLOAT );
		push_attr( 5, 5, VK_FORMAT_R32G32B32A32_SFLOAT );
		break;

	case TYPE_MULTI_TEXTURE_MUL2:
	case TYPE_MULTI_TEXTURE_ADD2_1_1:
	case TYPE_MULTI_TEXTURE_ADD2:
		push_bind( 0, sizeof( vec4_t ) ); // xyz array
		push_bind( 1, sizeof( color4ub_t ) ); // color array
		push_bind( 2, sizeof( vec2_t ) ); // st0 array
		push_bind( 3, sizeof( vec2_t ) ); // st1 array
		push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
		push_attr( 1, 1, VK_FORMAT_R8G8B8A8_UNORM );
		push_attr( 2, 2, VK_FORMAT_R32G32_SFLOAT );
		push_attr( 3, 3, VK_FORMAT_R32G32_SFLOAT );
		break;

	case TYPE_MULTI_TEXTURE_MUL2_ENV:
	case TYPE_MULTI_TEXTURE_ADD2_1_1_ENV:
	case TYPE_MULTI_TEXTURE_ADD2_ENV:
		push_bind( 0, sizeof( vec4_t ) ); // xyz array
		push_bind( 1, sizeof( color4ub_t ) ); // color array
		//push_bind( 2, sizeof( vec2_t ) );					// st0 array
		push_bind( 3, sizeof( vec2_t ) ); // st1 array
		push_bind( 5, sizeof( vec4_t ) ); // normals
		push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
		push_attr( 1, 1, VK_FORMAT_R8G8B8A8_UNORM );
		//push_attr( 2, 2, VK_FORMAT_R32G32_SFLOAT );
		push_attr( 3, 3, VK_FORMAT_R32G32_SFLOAT );
		push_attr( 5, 5, VK_FORMAT_R32G32B32A32_SFLOAT );
		break;

	case TYPE_MULTI_TEXTURE_MUL3:
	case TYPE_MULTI_TEXTURE_ADD3_1_1:
	case TYPE_MULTI_TEXTURE_ADD3:
		push_bind( 0, sizeof( vec4_t ) ); // xyz array
		push_bind( 1, sizeof( color4ub_t ) ); // color array
		push_bind( 2, sizeof( vec2_t ) ); // st0 array
		push_bind( 3, sizeof( vec2_t ) ); // st1 array
		push_bind( 4, sizeof( vec2_t ) ); // st2 array
		push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
		push_attr( 1, 1, VK_FORMAT_R8G8B8A8_UNORM );
		push_attr( 2, 2, VK_FORMAT_R32G32_SFLOAT );
		push_attr( 3, 3, VK_FORMAT_R32G32_SFLOAT );
		push_attr( 4, 4, VK_FORMAT_R32G32_SFLOAT );
		break;

	case TYPE_MULTI_TEXTURE_MUL3_ENV:
	case TYPE_MULTI_TEXTURE_ADD3_1_1_ENV:
	case TYPE_MULTI_TEXTURE_ADD3_ENV:
		push_bind( 0, sizeof( vec4_t ) ); // xyz array
		push_bind( 1, sizeof( color4ub_t ) ); // color array
		//push_bind( 2, sizeof( vec2_t ) );					// st0 array
		push_bind( 3, sizeof( vec2_t ) ); // st1 array
		push_bind( 4, sizeof( vec2_t ) ); // st2 array
		push_bind( 5, sizeof( vec4_t ) ); // normals
		push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
		push_attr( 1, 1, VK_FORMAT_R8G8B8A8_UNORM );
		//push_attr( 2, 2, VK_FORMAT_R32G32_SFLOAT );
		push_attr( 3, 3, VK_FORMAT_R32G32_SFLOAT );
		push_attr( 4, 4, VK_FORMAT_R32G32_SFLOAT );
		push_attr( 5, 5, VK_FORMAT_R32G32B32A32_SFLOAT );
		break;

	case TYPE_BLEND2_ADD:
	case TYPE_BLEND2_MUL:
	case TYPE_BLEND2_ALPHA:
	case TYPE_BLEND2_ONE_MINUS_ALPHA:
	case TYPE_BLEND2_MIX_ALPHA:
	case TYPE_BLEND2_MIX_ONE_MINUS_ALPHA:
	case TYPE_BLEND2_DST_COLOR_SRC_ALPHA:
		push_bind( 0, sizeof( vec4_t ) ); // xyz array
		push_bind( 1, sizeof( color4ub_t ) ); // color0 array
		push_bind( 2, sizeof( vec2_t ) ); // st0 array
		push_bind( 3, sizeof( vec2_t ) ); // st1 array
		push_bind( 6, sizeof( color4ub_t ) ); // color1 array
		push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
		push_attr( 1, 1, VK_FORMAT_R8G8B8A8_UNORM );
		push_attr( 2, 2, VK_FORMAT_R32G32_SFLOAT );
		push_attr( 3, 3, VK_FORMAT_R32G32_SFLOAT );
		push_attr( 6, 6, VK_FORMAT_R8G8B8A8_UNORM );
		break;

	case TYPE_BLEND2_ADD_ENV:
	case TYPE_BLEND2_MUL_ENV:
	case TYPE_BLEND2_ALPHA_ENV:
	case TYPE_BLEND2_ONE_MINUS_ALPHA_ENV:
	case TYPE_BLEND2_MIX_ALPHA_ENV:
	case TYPE_BLEND2_MIX_ONE_MINUS_ALPHA_ENV:
	case TYPE_BLEND2_DST_COLOR_SRC_ALPHA_ENV:
		push_bind( 0, sizeof( vec4_t ) ); // xyz array
		push_bind( 1, sizeof( color4ub_t ) ); // color0 array
		//push_bind( 2, sizeof( vec2_t ) );					// st0 array
		push_bind( 3, sizeof( vec2_t ) ); // st1 array
		push_bind( 5, sizeof( vec4_t ) ); // normals
		push_bind( 6, sizeof( color4ub_t ) ); // color1 array
		push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
		push_attr( 1, 1, VK_FORMAT_R8G8B8A8_UNORM );
		//push_attr( 2, 2, VK_FORMAT_R32G32_SFLOAT );
		push_attr( 3, 3, VK_FORMAT_R32G32_SFLOAT );
		push_attr( 5, 5, VK_FORMAT_R32G32B32A32_SFLOAT );
		push_attr( 6, 6, VK_FORMAT_R8G8B8A8_UNORM );
		break;

	case TYPE_BLEND3_ADD:
	case TYPE_BLEND3_MUL:
	case TYPE_BLEND3_ALPHA:
	case TYPE_BLEND3_ONE_MINUS_ALPHA:
	case TYPE_BLEND3_MIX_ALPHA:
	case TYPE_BLEND3_MIX_ONE_MINUS_ALPHA:
	case TYPE_BLEND3_DST_COLOR_SRC_ALPHA:
		push_bind( 0, sizeof( vec4_t ) ); // xyz array
		push_bind( 1, sizeof( color4ub_t ) ); // color0 array
		push_bind( 2, sizeof( vec2_t ) ); // st0 array
		push_bind( 3, sizeof( vec2_t ) ); // st1 array
		push_bind( 4, sizeof( vec2_t ) ); // st2 array
		push_bind( 6, sizeof( color4ub_t ) ); // color1 array
		push_bind( 7, sizeof( color4ub_t ) ); // color2 array
		push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
		push_attr( 1, 1, VK_FORMAT_R8G8B8A8_UNORM );
		push_attr( 2, 2, VK_FORMAT_R32G32_SFLOAT );
		push_attr( 3, 3, VK_FORMAT_R32G32_SFLOAT );
		push_attr( 4, 4, VK_FORMAT_R32G32_SFLOAT );
		push_attr( 6, 6, VK_FORMAT_R8G8B8A8_UNORM );
		push_attr( 7, 7, VK_FORMAT_R8G8B8A8_UNORM );
		break;

	case TYPE_BLEND3_ADD_ENV:
	case TYPE_BLEND3_MUL_ENV:
	case TYPE_BLEND3_ALPHA_ENV:
	case TYPE_BLEND3_ONE_MINUS_ALPHA_ENV:
	case TYPE_BLEND3_MIX_ALPHA_ENV:
	case TYPE_BLEND3_MIX_ONE_MINUS_ALPHA_ENV:
	case TYPE_BLEND3_DST_COLOR_SRC_ALPHA_ENV:
		push_bind( 0, sizeof( vec4_t ) ); // xyz array
		push_bind( 1, sizeof( color4ub_t ) ); // color0 array
		//push_bind( 2, sizeof( vec2_t ) );					// st0 array
		push_bind( 3, sizeof( vec2_t ) ); // st1 array
		push_bind( 4, sizeof( vec2_t ) ); // st2 array
		push_bind( 5, sizeof( vec4_t ) ); // normals
		push_bind( 6, sizeof( color4ub_t ) ); // color1 array
		push_bind( 7, sizeof( color4ub_t ) ); // color2 array
		push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
		push_attr( 1, 1, VK_FORMAT_R8G8B8A8_UNORM );
		//push_attr( 2, 2, VK_FORMAT_R32G32_SFLOAT );
		push_attr( 3, 3, VK_FORMAT_R32G32_SFLOAT );
		push_attr( 4, 4, VK_FORMAT_R32G32_SFLOAT );
		push_attr( 5, 5, VK_FORMAT_R32G32B32A32_SFLOAT );
		push_attr( 6, 6, VK_FORMAT_R8G8B8A8_UNORM );
		push_attr( 7, 7, VK_FORMAT_R8G8B8A8_UNORM );
		break;

	default:
		vk_fail( ERR_DROP, rhiStatus_t::Error, "%s: invalid shader type - %i", __func__, def->shader_type );
		break;
	}

	vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state.pNext = NULL;
	vertex_input_state.flags = 0;
	vertex_input_state.pVertexBindingDescriptions = bindings;
	vertex_input_state.pVertexAttributeDescriptions = attribs;
	vertex_input_state.vertexBindingDescriptionCount = num_binds;
	vertex_input_state.vertexAttributeDescriptionCount = num_attrs;

	//
	// Primitive assembly.
	//
	input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state.pNext = NULL;
	input_assembly_state.flags = 0;
	input_assembly_state.primitiveRestartEnable = VK_FALSE;

	switch ( def->primitives ) {
	case LINE_LIST:
		input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		break;
	case POINT_LIST:
		input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		break;
	case TRIANGLE_STRIP:
		input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		break;
	default:
		input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		break;
	}

	//
	// Viewport.
	//
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.pNext = NULL;
	viewport_state.flags = 0;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = NULL; // dynamic viewport state
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = NULL; // dynamic scissor state

	//
	// Rasterization.
	//
	rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state.pNext = NULL;
	rasterization_state.flags = 0;
	rasterization_state.depthClampEnable = VK_FALSE;
	rasterization_state.rasterizerDiscardEnable = VK_FALSE;
	if ( def->shader_type == TYPE_DOT ) {
		rasterization_state.polygonMode = VK_POLYGON_MODE_POINT;
	} else {
		rasterization_state.polygonMode = ( state_bits & GLS_POLYMODE_LINE ) ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
	}

	switch ( def->face_culling ) {
	case CT_TWO_SIDED:
		rasterization_state.cullMode = VK_CULL_MODE_NONE;
		break;
	case CT_FRONT_SIDED:
		rasterization_state.cullMode = ( def->mirror ? VK_CULL_MODE_FRONT_BIT : VK_CULL_MODE_BACK_BIT );
		break;
	case CT_BACK_SIDED:
		rasterization_state.cullMode = ( def->mirror ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_FRONT_BIT );
		break;
	default:
		vk_fail( ERR_DROP, rhiStatus_t::Error, "create_pipeline: invalid face culling mode %i\n", def->face_culling );
		break;
	}

	rasterization_state.frontFace = VK_FRONT_FACE_CLOCKWISE; // Q3 defaults to clockwise vertex order
	if ( ( def->shader_type == TYPE_PBR || def->shader_type == TYPE_PBR_BAKED ) && def->mirror ) {
		// Keep gl_FrontFacing meaningful for the PBR double-sided normal rule.
		rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		if ( def->face_culling == CT_FRONT_SIDED )
			rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
		else if ( def->face_culling == CT_BACK_SIDED )
			rasterization_state.cullMode = VK_CULL_MODE_FRONT_BIT;
	}

	// depth bias state
	if ( def->polygon_offset ) {
		rasterization_state.depthBiasEnable = VK_TRUE;
		rasterization_state.depthBiasClamp = 0.0f;
#ifdef USE_REVERSED_DEPTH
		rasterization_state.depthBiasConstantFactor = -vk_config.offsetUnits;
		rasterization_state.depthBiasSlopeFactor = -vk_config.offsetFactor;
#else
		rasterization_state.depthBiasConstantFactor = vk_config.offsetUnits;
		rasterization_state.depthBiasSlopeFactor = vk_config.offsetFactor;
#endif
	} else {
		rasterization_state.depthBiasEnable = VK_FALSE;
		rasterization_state.depthBiasClamp = 0.0f;
		rasterization_state.depthBiasConstantFactor = 0.0f;
		rasterization_state.depthBiasSlopeFactor = 0.0f;
	}

	if ( def->line_width )
		rasterization_state.lineWidth = (float)def->line_width;
	else
		rasterization_state.lineWidth = 1.0f;

	multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state.pNext = NULL;
	multisample_state.flags = 0;

	multisample_state.rasterizationSamples = (VkSampleCountFlagBits)( ( renderPassIndex == RENDER_PASS_SCREENMAP ) ? vk.screenMapSamples : vkSamples );

	multisample_state.sampleShadingEnable = VK_FALSE;
	multisample_state.minSampleShading = 1.0f;
	multisample_state.pSampleMask = NULL;
	multisample_state.alphaToCoverageEnable = alphaToCoverage;
	multisample_state.alphaToOneEnable = VK_FALSE;

	Com_Memset( &depth_stencil_state, 0, sizeof( depth_stencil_state ) );

	depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_state.pNext = NULL;
	depth_stencil_state.flags = 0;
	depth_stencil_state.depthTestEnable = ( state_bits & GLS_DEPTHTEST_DISABLE ) ? VK_FALSE : VK_TRUE;
	depth_stencil_state.depthWriteEnable = ( state_bits & GLS_DEPTHMASK_TRUE ) ? VK_TRUE : VK_FALSE;
#ifdef USE_REVERSED_DEPTH
	depth_stencil_state.depthCompareOp = ( state_bits & GLS_DEPTHFUNC_EQUAL ) ? VK_COMPARE_OP_EQUAL : VK_COMPARE_OP_GREATER_OR_EQUAL;
#else
	depth_stencil_state.depthCompareOp = ( state_bits & GLS_DEPTHFUNC_EQUAL ) ? VK_COMPARE_OP_EQUAL : VK_COMPARE_OP_LESS_OR_EQUAL;
#endif
	depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
	depth_stencil_state.stencilTestEnable = ( def->shadow_phase != SHADOW_DISABLED ) ? VK_TRUE : VK_FALSE;

	if ( def->shadow_phase == SHADOW_EDGES ) {
		depth_stencil_state.front.failOp = VK_STENCIL_OP_KEEP;
		depth_stencil_state.front.passOp = ( def->face_culling == CT_FRONT_SIDED ) ? VK_STENCIL_OP_INCREMENT_AND_CLAMP : VK_STENCIL_OP_DECREMENT_AND_CLAMP;
		depth_stencil_state.front.depthFailOp = VK_STENCIL_OP_KEEP;
		depth_stencil_state.front.compareOp = VK_COMPARE_OP_ALWAYS;
		depth_stencil_state.front.compareMask = 255;
		depth_stencil_state.front.writeMask = 255;
		depth_stencil_state.front.reference = 0;

		depth_stencil_state.back = depth_stencil_state.front;

	} else if ( def->shadow_phase == SHADOW_FS_QUAD ) {
		depth_stencil_state.front.failOp = VK_STENCIL_OP_KEEP;
		depth_stencil_state.front.passOp = VK_STENCIL_OP_KEEP;
		depth_stencil_state.front.depthFailOp = VK_STENCIL_OP_KEEP;
		depth_stencil_state.front.compareOp = VK_COMPARE_OP_NOT_EQUAL;
		depth_stencil_state.front.compareMask = 255;
		depth_stencil_state.front.writeMask = 255;
		depth_stencil_state.front.reference = 0;

		depth_stencil_state.back = depth_stencil_state.front;
	}

	depth_stencil_state.minDepthBounds = 0.0f;
	depth_stencil_state.maxDepthBounds = 1.0f;

	Com_Memset( &attachment_blend_state, 0, sizeof( attachment_blend_state ) );
	attachment_blend_state.blendEnable = ( state_bits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) ? VK_TRUE : VK_FALSE;

	if ( def->shadow_phase == SHADOW_EDGES || def->shader_type == TYPE_SIGNLE_TEXTURE_DF || def->shader_type == TYPE_DOT ) {
		attachment_blend_state.colorWriteMask = 0;
	} else {
		attachment_blend_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	}

	if ( attachment_blend_state.blendEnable ) {
		switch ( state_bits & GLS_SRCBLEND_BITS ) {
		case GLS_SRCBLEND_ZERO:
			attachment_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			break;
		case GLS_SRCBLEND_ONE:
			attachment_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
			break;
		case GLS_SRCBLEND_DST_COLOR:
			attachment_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
			break;
		case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
			attachment_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
			break;
		case GLS_SRCBLEND_SRC_ALPHA:
			attachment_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			break;
		case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
			attachment_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			break;
		case GLS_SRCBLEND_DST_ALPHA:
			attachment_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
			break;
		case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
			attachment_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			break;
		case GLS_SRCBLEND_ALPHA_SATURATE:
			attachment_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
			break;
		default:
			vk_fail( ERR_DROP, rhiStatus_t::Error, "create_pipeline: invalid src blend state bits\n" );
			break;
		}
		switch ( state_bits & GLS_DSTBLEND_BITS ) {
		case GLS_DSTBLEND_ZERO:
			attachment_blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			break;
		case GLS_DSTBLEND_ONE:
			attachment_blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
			break;
		case GLS_DSTBLEND_SRC_COLOR:
			attachment_blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
			break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
			attachment_blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
			break;
		case GLS_DSTBLEND_SRC_ALPHA:
			attachment_blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
			attachment_blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			break;
		case GLS_DSTBLEND_DST_ALPHA:
			attachment_blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
			break;
		case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
			attachment_blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			break;
		default:
			vk_fail( ERR_DROP, rhiStatus_t::Error, "create_pipeline: invalid dst blend state bits\n" );
			break;
		}

		attachment_blend_state.srcAlphaBlendFactor = attachment_blend_state.srcColorBlendFactor;
		attachment_blend_state.dstAlphaBlendFactor = attachment_blend_state.dstColorBlendFactor;
		attachment_blend_state.colorBlendOp = VK_BLEND_OP_ADD;
		attachment_blend_state.alphaBlendOp = VK_BLEND_OP_ADD;

		if ( def->allow_discard && vkSamples != VK_SAMPLE_COUNT_1_BIT && depth_stencil_state.depthWriteEnable == VK_FALSE ) {
			// try to reduce pixel fillrate for transparent surfaces, this yields 1..10% fps increase when multisampling in enabled
			if ( attachment_blend_state.srcColorBlendFactor == VK_BLEND_FACTOR_SRC_ALPHA && attachment_blend_state.dstColorBlendFactor == VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA ) {
				frag_spec_data[7].i = 1;
			} else if ( attachment_blend_state.srcColorBlendFactor == VK_BLEND_FACTOR_ONE && attachment_blend_state.dstColorBlendFactor == VK_BLEND_FACTOR_ONE ) {
				frag_spec_data[7].i = 2;
			}
		}
	}

	blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend_state.pNext = NULL;
	blend_state.flags = 0;
	blend_state.logicOpEnable = VK_FALSE;
	blend_state.logicOp = VK_LOGIC_OP_COPY;
	blend_state.attachmentCount = 1;
	blend_state.pAttachments = &attachment_blend_state;
	blend_state.blendConstants[0] = 0.0f;
	blend_state.blendConstants[1] = 0.0f;
	blend_state.blendConstants[2] = 0.0f;
	blend_state.blendConstants[3] = 0.0f;

	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.pNext = NULL;
	dynamic_state.flags = 0;
	dynamic_state.dynamicStateCount = ARRAY_LEN( dynamic_state_array );
	dynamic_state.pDynamicStates = dynamic_state_array;

	create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.stageCount = ARRAY_LEN( shader_stages );
	create_info.pStages = shader_stages;
	create_info.pVertexInputState = &vertex_input_state;
	create_info.pInputAssemblyState = &input_assembly_state;
	create_info.pTessellationState = NULL;
	create_info.pViewportState = &viewport_state;
	create_info.pRasterizationState = &rasterization_state;
	create_info.pMultisampleState = &multisample_state;
	create_info.pDepthStencilState = &depth_stencil_state;
	create_info.pColorBlendState = &blend_state;
	create_info.pDynamicState = &dynamic_state;

	if ( def->shader_type == TYPE_DOT )
		create_info.layout = vk.pipeline_layout_storage;
	else
		create_info.layout = vk.pipeline_layout;

	if ( renderPassIndex == RENDER_PASS_SCREENMAP )
		create_info.renderPass = vk.render_pass.screenmap;
	else
		create_info.renderPass = vk.render_pass.main;

	create_info.subpass = 0;
	create_info.basePipelineHandle = VK_NULL_HANDLE;
	create_info.basePipelineIndex = -1;

	VK_CHECK( qvkCreateGraphicsPipelines( vk.device, vk.pipelineCache, 1, &create_info, NULL, &pipeline ) );

	SET_OBJECT_NAME( pipeline, va( "pipeline def#%i, pass#%i", def_index, renderPassIndex ), VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT );

	vk.pipeline_create_count++;

	return pipeline;
}


static uint32_t vk_alloc_pipeline( const rhiPipelineDesc_t *def ) {
	VK_Pipeline_t *pipeline;
	if ( vk.pipelines_count >= MAX_VK_PIPELINES ) {
		vk_fail( ERR_DROP, rhiStatus_t::Error, "alloc_pipeline: MAX_VK_PIPELINES reached" );
	} else {
		int j;
		pipeline = &vk.pipelines[vk.pipelines_count];
		pipeline->def = *def;
		for ( j = 0; j < RENDER_PASS_COUNT; j++ ) {
			pipeline->handle[j] = VK_NULL_HANDLE;
		}
		return vk.pipelines_count++;
	}
}


VkPipeline vk_gen_pipeline( uint32_t index ) {
	if ( index < vk.pipelines_count ) {
		VK_Pipeline_t *pipeline = vk.pipelines + index;
		const renderPass_t pass = vk.renderPassIndex;
		if ( pipeline->handle[pass] == VK_NULL_HANDLE ) {
			pipeline->handle[pass] = create_pipeline( &pipeline->def, pass, index );
		}
		return pipeline->handle[pass];
	} else {
		vk_fail( ERR_FATAL, rhiStatus_t::Error, "%s(%i): NULL pipeline", __func__, index );
	}
}


uint32_t vk_impl_FindPipeline( uint32_t base, const rhiPipelineDesc_t *def, bool use ) {
	const rhiPipelineDesc_t *cur_def;
	uint32_t index;

	for ( index = base; index < vk.pipelines_count; index++ ) {
		cur_def = &vk.pipelines[index].def;
		if ( memcmp( cur_def, def, sizeof( *def ) ) == 0 ) {
			goto found;
		}
	}

	index = vk_alloc_pipeline( def );
found:

	if ( use )
		vk_gen_pipeline( index );

	return index;
}


void RHI_GetPipelineDesc( uint32_t pipeline, rhiPipelineDesc_t *def ) {
	if ( pipeline >= vk.pipelines_count ) {
		Com_Memset( def, 0, sizeof( *def ) );
	} else {
		Com_Memcpy( def, &vk.pipelines[pipeline].def, sizeof( *def ) );
	}
}


rhiRenderArea_t RHI_GetRenderArea( void ) {
	return { vk.renderWidth, vk.renderHeight, vk.renderScaleX, vk.renderScaleY };
}

static VkRect2D vk_rect( const rhiRect_t *rect ) {
	return { { rect->offset.x, rect->offset.y }, { rect->extent.width, rect->extent.height } };
}


void RHI_ClearColor( const float *color, const rhiRect_t *rect ) {

	VkClearAttachment attachment;
	VkClearRect clear_rect;

	if ( !vk.active )
		return;

	attachment.colorAttachment = 0;
	attachment.clearValue.color.float32[0] = color[0];
	attachment.clearValue.color.float32[1] = color[1];
	attachment.clearValue.color.float32[2] = color[2];
	attachment.clearValue.color.float32[3] = color[3];
	attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	clear_rect.rect = vk_rect( rect );
	clear_rect.baseArrayLayer = 0;
	clear_rect.layerCount = 1;

	qvkCmdClearAttachments( vk.cmd->command_buffer, 1, &attachment, 1, &clear_rect );
}


void RHI_ClearDepth( bool clear_stencil, const rhiRect_t *rect ) {

	VkClearAttachment attachment;
	VkClearRect clear_rect[1];

	if ( !vk.active )
		return;

	if ( vk_world.dirty_depth_attachment == 0 )
		return;

	attachment.colorAttachment = 0;
#ifdef USE_REVERSED_DEPTH
	attachment.clearValue.depthStencil.depth = 0.0f;
#else
	attachment.clearValue.depthStencil.depth = 1.0f;
#endif
	attachment.clearValue.depthStencil.stencil = 0;
	if ( clear_stencil ) {
		attachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	} else {
		attachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	clear_rect[0].rect = vk_rect( rect );
	clear_rect[0].baseArrayLayer = 0;
	clear_rect[0].layerCount = 1;

	qvkCmdClearAttachments( vk.cmd->command_buffer, 1, &attachment, 1, clear_rect );
}


void RHI_PushTransform( const float *m ) {
	float push_constants[16];
	Com_Memcpy( push_constants, m, sizeof( push_constants ) );
	qvkCmdPushConstants( vk.cmd->command_buffer, vk.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( push_constants ), push_constants );
	vk.stats.push_size += sizeof( push_constants );
}


void RHI_BindVertexStreams( rhiGeometryBuffer_t pool, uint32_t mask, const rhiVertexStream_t *streams ) {
	VkBuffer buffers[RHI_MAX_VERTEX_STREAMS];
	VkDeviceSize *offsets = pool == rhiGeometryBuffer_t::World ? vk.cmd->vbo_offset : vk.cmd->buf_offset;
	const VkBuffer buffer = pool == rhiGeometryBuffer_t::World ? vk.vbo.vertex_buffer : vk.cmd->vertex_buffer;
	uint32_t first = RHI_MAX_VERTEX_STREAMS, last = 0;
	for ( uint32_t i = 0; i < RHI_MAX_VERTEX_STREAMS; i++ ) {
		buffers[i] = buffer;
		if ( !( mask & ( 1U << i ) ) )
			continue;
		if ( first == RHI_MAX_VERTEX_STREAMS )
			first = i;
		last = i;
		if ( pool == rhiGeometryBuffer_t::World ) {
			offsets[i] = streams[i].offset;
		} else {
			const uint32_t offset = PAD( vk.cmd->vertex_buffer_offset, 32 );
			const uint32_t size = streams[i].size;
			if ( offset + size > vk.geometry_buffer_size ) {
				// Preserve the existing deferred resize and stale binding on overflow.
				vk.geometry_buffer_size_new = log2pad( offset + size, 1 );
			} else {
				offsets[i] = offset;
				Com_Memcpy( vk.cmd->vertex_buffer_ptr + offset, streams[i].data, size );
				vk.cmd->vertex_buffer_offset = (VkDeviceSize)offset + size;
			}
		}
	}
	if ( first != RHI_MAX_VERTEX_STREAMS )
		qvkCmdBindVertexBuffers( vk.cmd->command_buffer, first, last - first + 1, buffers, offsets + first );
}


uint32_t RHI_UploadIndices( uint32_t numIndexes, const void *src ) {
	const uint32_t offset = vk.cmd->vertex_buffer_offset;
	const uint32_t size = numIndexes * sizeof( uint32_t );

	if ( offset + size > vk.geometry_buffer_size ) {
		// schedule geometry buffer resize
		vk.geometry_buffer_size_new = log2pad( offset + size, 1 );
		return ~0U;
	} else {
		Com_Memcpy( vk.cmd->vertex_buffer_ptr + offset, src, size );
		vk.cmd->vertex_buffer_offset = (VkDeviceSize)offset + size;
		return offset;
	}
}


void vk_bind_index_buffer( VkBuffer buffer, uint32_t offset ) {
	if ( vk.cmd->curr_index_buffer != buffer || vk.cmd->curr_index_offset != offset )
		qvkCmdBindIndexBuffer( vk.cmd->command_buffer, buffer, offset, VK_INDEX_TYPE_UINT32 );

	vk.cmd->curr_index_buffer = buffer;
	vk.cmd->curr_index_offset = offset;
}


static void vk_draw( uint32_t vertexCount ) {
	qvkCmdDraw( vk.cmd->command_buffer, vertexCount, 1, 0, 0 );
	++vk.stats.draw_calls;
}

void RHI_DrawIndexed( uint32_t indexCount, uint32_t firstIndex ) {
	qvkCmdDrawIndexed( vk.cmd->command_buffer, indexCount, 1, firstIndex, 0, 0 );
	++vk.stats.draw_calls;
}


void RHI_BindIndexData( uint32_t numIndexes, const uint32_t *indexes ) {
	if ( !indexes ) {
		vk.cmd->num_indexes = 0;
		return;
	}
	uint32_t offset = RHI_UploadIndices( numIndexes, indexes );
	if ( offset != ~0U ) {
		vk_bind_index_buffer( vk.cmd->vertex_buffer, offset );
		vk.cmd->num_indexes = numIndexes;
	} else {
		vk.cmd->num_indexes = 0;
	}
}

void RHI_DrawBoundIndices( void ) {
	RHI_DrawIndexed( vk.cmd->num_indexes, 0 );
}

void RHI_Draw( uint32_t vertexCount ) {
	vk_draw( vertexCount );
}


uint32_t RHI_UploadUniform( const void *data, uint32_t size ) {
	if ( !vk.cmd || !data || size > vk_config.uniformBytes )
		return RHI_INVALID_OFFSET;

	const uint32_t offset = vk.cmd->uniform_read_offset = PAD( vk.cmd->vertex_buffer_offset, vk.uniform_alignment );

	if ( offset + vk.uniform_item_size > vk.geometry_buffer_size )
		return ~0U;

	// push uniform
	Com_Memcpy( vk.cmd->vertex_buffer_ptr + offset, data, size );
	vk.cmd->vertex_buffer_offset = offset + vk.uniform_item_size;

	RHI_ResetBinding( RHI_BINDING_UNIFORM );
	vk_update_descriptor( RHI_BINDING_UNIFORM, vk.cmd->uniform_descriptor );
	vk_update_descriptor_offset( RHI_BINDING_UNIFORM, vk.cmd->uniform_read_offset );

	return offset;
}


void RHI_ResetBinding( int index ) {
	vk.cmd->descriptor_set.current[index] = VK_NULL_HANDLE;
}


void vk_update_descriptor( int index, VkDescriptorSet descriptor ) {
	if ( vk.cmd->descriptor_set.current[index] != descriptor ) {
		vk.cmd->descriptor_set.start = ( (uint32_t)index < vk.cmd->descriptor_set.start ) ? index : vk.cmd->descriptor_set.start;
		vk.cmd->descriptor_set.end = ( (uint32_t)index > vk.cmd->descriptor_set.end ) ? index : vk.cmd->descriptor_set.end;
	}
	vk.cmd->descriptor_set.current[index] = descriptor;
}


void vk_update_descriptor_offset( int index, uint32_t offset ) {
	vk.cmd->descriptor_set.offset[index] = offset;
}


static void vk_bind_descriptor_sets( const rhiTexture_t *fallback ) {
	uint32_t offsets[2], offset_count;
	uint32_t start, end, count, i;

	start = vk.cmd->descriptor_set.start;
	if ( start == ~0U )
		return;

	end = vk.cmd->descriptor_set.end;

	offset_count = 0;
	if ( /*start == RHI_BINDING_STORAGE || */ start == RHI_BINDING_UNIFORM ) { // uniform offset or storage offset
		offsets[offset_count++] = vk.cmd->descriptor_set.offset[start];
	}

	count = end - start + 1;

	// fill NULL descriptor gaps
	for ( i = start + 1; i < end; i++ ) {
		if ( vk.cmd->descriptor_set.current[i] == VK_NULL_HANDLE ) {
			vk.cmd->descriptor_set.current[i] = (VkDescriptorSet)(uintptr_t)fallback->binding;
		}
	}

	qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout, start, count, vk.cmd->descriptor_set.current + start, offset_count, offsets );

	vk.cmd->descriptor_set.end = 0;
	vk.cmd->descriptor_set.start = ~0U;
}


void vk_impl_BindPipeline( uint32_t pipeline ) {
	VkPipeline vkpipe;

	vkpipe = vk_gen_pipeline( pipeline );

	if ( vkpipe != vk.cmd->last_pipeline ) {
		qvkCmdBindPipeline( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkpipe );
		vk.cmd->last_pipeline = vkpipe;
	}

	vk_world.dirty_depth_attachment |= ( vk.pipelines[pipeline].def.state_bits & GLS_DEPTHMASK_TRUE );
}

static void vk_update_depth_range( const rhiRasterState_t *raster ) {
	if ( vk.cmd->depth_range != raster->depthRange ) {
		const VkRect2D scissor_rect = vk_rect( &raster->scissor );
		const rhiViewport_t *v = &raster->viewport;
		const VkViewport viewport = { v->x, v->y, v->width, v->height, v->minDepth, v->maxDepth };
		vk.cmd->depth_range = raster->depthRange;
		if ( memcmp( &vk.cmd->scissor_rect, &scissor_rect, sizeof( scissor_rect ) ) != 0 ) {
			qvkCmdSetScissor( vk.cmd->command_buffer, 0, 1, &scissor_rect );
			vk.cmd->scissor_rect = scissor_rect;
		}
		qvkCmdSetViewport( vk.cmd->command_buffer, 0, 1, &viewport );
	}
}

bool RHI_PrepareDraw( const rhiRasterState_t *raster, const rhiTexture_t *fallback ) {
	if ( vk.geometry_buffer_size_new )
		return false;
	vk_bind_descriptor_sets( fallback );
	vk_update_depth_range( raster );
	return true;
}


bool RHI_ReadVisibility( uint32_t index ) {
	const uint32_t offset = index * vk.storage_alignment;
	return *(const uint32_t *)( vk.storage.buffer_ptr + offset ) != 0;
}

void RHI_DrawVisibility( uint32_t index, uint32_t vertexCount, const rhiRasterState_t *raster ) {
	const uint32_t storage_offset = index * vk.storage_alignment;
	if ( vk.geometry_buffer_size_new ) {
		// geometry buffer overflow happened this frame
		return;
	}

	qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout_storage, RHI_BINDING_STORAGE, 1, &vk.storage.descriptor, 1, &storage_offset );

	// configure pipeline's dynamic state
	vk_update_depth_range( raster );

	vk_draw( vertexCount );
}


static void vk_begin_render_pass( VkRenderPass renderPass, VkFramebuffer frameBuffer, qboolean clearValues, uint32_t width, uint32_t height ) {
	VkRenderPassBeginInfo render_pass_begin_info;
	VkClearValue clear_values[3];

	// Begin render pass.

	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.pNext = NULL;
	render_pass_begin_info.renderPass = renderPass;
	render_pass_begin_info.framebuffer = frameBuffer;
	render_pass_begin_info.renderArea.offset.x = 0;
	render_pass_begin_info.renderArea.offset.y = 0;
	render_pass_begin_info.renderArea.extent.width = width;
	render_pass_begin_info.renderArea.extent.height = height;

	if ( clearValues ) {
		// attachments layout:
		// [0] - resolve/color/presentation
		// [1] - depth/stencil
		// [2] - multisampled color, optional
		Com_Memset( clear_values, 0, sizeof( clear_values ) );
#ifndef USE_REVERSED_DEPTH
		clear_values[1].depthStencil.depth = 1.0;
#endif
		render_pass_begin_info.clearValueCount = vk.msaaActive ? 3 : 2;
		render_pass_begin_info.pClearValues = clear_values;

		vk_world.dirty_depth_attachment = 0;
	} else {
		render_pass_begin_info.clearValueCount = 0;
		render_pass_begin_info.pClearValues = NULL;
	}

	const char *name = "blur";
	if ( renderPass == vk.render_pass.main )
		name = "main";
	else if ( renderPass == vk.render_pass.screenmap )
		name = "screenmap";
	else if ( renderPass == vk.render_pass.gamma )
		name = "gamma";
	else if ( renderPass == vk.render_pass.capture )
		name = "capture";
	else if ( renderPass == vk.render_pass.bloom_extract )
		name = "bloom extract";
	else if ( renderPass == vk.render_pass.post_bloom )
		name = "post bloom";
	vk.cmd->profile.passScope = RHI_BeginScope( name );
	qvkCmdBeginRenderPass( vk.cmd->command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE );

	vk.cmd->last_pipeline = VK_NULL_HANDLE;
	vk.cmd->depth_range = DEPTH_RANGE_COUNT;
}


void RHI_BeginMainPass( void ) {
	VkFramebuffer frameBuffer = vk.framebuffers.main[vk.cmd->swapchain_image_index];

	vk.renderPassIndex = RENDER_PASS_MAIN;

	vk.renderWidth = vk_config.renderWidth;
	vk.renderHeight = vk_config.renderHeight;

	//vk.renderScaleX = (float)vk.renderWidth / (float)vk_config.renderWidth;
	//vk.renderScaleY = (float)vk.renderHeight / (float)vk_config.renderHeight;
	vk.renderScaleX = vk.renderScaleY = 1.0f;

	vk_begin_render_pass( vk.render_pass.main, frameBuffer, qtrue, vk.renderWidth, vk.renderHeight );
}


void vk_begin_post_bloom_render_pass( void ) {
	VkFramebuffer frameBuffer = vk.framebuffers.main[vk.cmd->swapchain_image_index];

	vk.renderPassIndex = RENDER_PASS_POST_BLOOM;

	vk.renderWidth = vk_config.renderWidth;
	vk.renderHeight = vk_config.renderHeight;

	//vk.renderScaleX = (float)vk.renderWidth / (float)vk_config.renderWidth;
	//vk.renderScaleY = (float)vk.renderHeight / (float)vk_config.renderHeight;
	vk.renderScaleX = vk.renderScaleY = 1.0f;

	vk_begin_render_pass( vk.render_pass.post_bloom, frameBuffer, qfalse, vk.renderWidth, vk.renderHeight );
}


void vk_begin_bloom_extract_render_pass( void ) {
	VkFramebuffer frameBuffer = vk.framebuffers.bloom_extract;

	//vk.renderPassIndex = RENDER_PASS_BLOOM_EXTRACT; // doesn't matter, we will use dedicated pipelines

	vk.renderWidth = vk_config.captureWidth;
	vk.renderHeight = vk_config.captureHeight;

	//vk.renderScaleX = (float)vk.renderWidth / (float)vk_config.renderWidth;
	//vk.renderScaleY = (float)vk.renderHeight / (float)vk_config.renderHeight;
	vk.renderScaleX = vk.renderScaleY = 1.0f;

	vk_begin_render_pass( vk.render_pass.bloom_extract, frameBuffer, qfalse, vk.renderWidth, vk.renderHeight );
}


void vk_begin_blur_render_pass( uint32_t index ) {
	VkFramebuffer frameBuffer = vk.framebuffers.blur[index];

	//vk.renderPassIndex = RENDER_PASS_BLOOM_EXTRACT; // doesn't matter, we will use dedicated pipelines

	vk.renderWidth = vk_config.captureWidth / ( 2 << ( index / 2 ) );
	vk.renderHeight = vk_config.captureHeight / ( 2 << ( index / 2 ) );

	//vk.renderScaleX = (float)vk.renderWidth / (float)vk_config.renderWidth;
	//vk.renderScaleY = (float)vk.renderHeight / (float)vk_config.renderHeight;
	vk.renderScaleX = vk.renderScaleY = 1.0f;

	vk_begin_render_pass( vk.render_pass.blur[index], frameBuffer, qfalse, vk.renderWidth, vk.renderHeight );
}


static void vk_begin_screenmap_render_pass( void ) {
	VkFramebuffer frameBuffer = vk.framebuffers.screenmap;

	vk.renderPassIndex = RENDER_PASS_SCREENMAP;

	vk.renderWidth = vk.screenMapWidth;
	vk.renderHeight = vk.screenMapHeight;

	vk.renderScaleX = (float)vk.renderWidth / (float)vk_config.renderWidth;
	vk.renderScaleY = (float)vk.renderHeight / (float)vk_config.renderHeight;

	vk_begin_render_pass( vk.render_pass.screenmap, frameBuffer, qtrue, vk.renderWidth, vk.renderHeight );
}


void RHI_EndPass( void ) {
	qvkCmdEndRenderPass( vk.cmd->command_buffer );
	RHI_EndScope( vk.cmd->profile.passScope );
	vk.cmd->profile.passScope = RHI_INVALID_OFFSET;

	//	vk.renderPassIndex = RENDER_PASS_MAIN;
}


#ifndef UINT64_MAX
#define UINT64_MAX 0xFFFFFFFFFFFFFFFFULL
#endif

bool vk_impl_BeginFrame( bool screenMap ) {
	VkCommandBufferBeginInfo begin_info;
	VkResult res;

	if ( vk.frame_count++ ) // might happen during stereo rendering
		return false;

#ifdef USE_UPLOAD_QUEUE
	vk_flush_staging_buffer( qtrue );
#endif

	vk.cmd = &vk.tess[vk.cmd_index];

	if ( vk.cmd->waitForFence ) {
		vk.cmd->waitForFence = qfalse;
		res = qvkWaitForFences( vk.device, 1, &vk.cmd->rendering_finished_fence, VK_FALSE, (uint64_t)1e10 );
		if ( res != VK_SUCCESS ) {
			if ( res == VK_ERROR_DEVICE_LOST ) {
				// silently discard previous command buffer
				vk_host.Print( rhiLog_t::Warning, "Vulkan: %s returned %s", "vkWaitForFences", vk_result_string( res ) );
			} else {
				vk_fail( ERR_FATAL, rhiStatus_t::Error, "Vulkan: %s returned %s", "vkWaitForFences", vk_result_string( res ) );
			}
		}
		if ( res == VK_SUCCESS )
			vk_read_timings();
		else
			vk.timingCount = 0;
		VK_CHECK( qvkResetFences( vk.device, 1, &vk.cmd->rendering_finished_fence ) );
	}

	if ( !vk_host.IsMinimized() && !vk.cmd->swapchain_image_acquired ) {
		qboolean retry = qfalse;
	_retry:
		res = qvkAcquireNextImageKHR( vk.device, vk.swapchain, 1 * 1000000000ULL, vk.cmd->image_acquired, VK_NULL_HANDLE, &vk.cmd->swapchain_image_index );
		// when running via RDP: "Application has already acquired the maximum number of images (0x2)"
		// probably caused by "device lost" errors
		if ( res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR ) {
			if ( res == VK_ERROR_OUT_OF_DATE_KHR && retry == qfalse ) {
				// swapchain re-creation needed
				retry = qtrue;
				vk_restart_swapchain( __func__, res );
				goto _retry;
			} else {
				vk_fail( ERR_FATAL, rhiStatus_t::Error, "vkAcquireNextImageKHR returned %s", vk_result_string( res ) );
			}
		}
		vk.cmd->swapchain_image_acquired = qtrue;
	}

	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = NULL;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	begin_info.pInheritanceInfo = NULL;

	VK_CHECK( qvkBeginCommandBuffer( vk.cmd->command_buffer, &begin_info ) );
	vk.cmd->profile.count = 0;
	vk.cmd->profile.passScope = RHI_INVALID_OFFSET;
	if ( vk.timestampPool )
		qvkCmdResetQueryPool( vk.cmd->command_buffer, vk.timestampPool, vk_timestamp_base(), RHI_MAX_TIMINGS * 2 );


	if ( vk.swapchain_images_inited[vk.cmd->swapchain_image_index] == qfalse ) {
		// perform initial swapchain image layout transition
		vk.swapchain_images_inited[vk.cmd->swapchain_image_index] = qtrue;
		record_image_layout_transition( vk.cmd->command_buffer, vk.swapchain_images[vk.cmd->swapchain_image_index],
			VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, vk.initSwapchainLayout,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0 );
	}

	// Ensure visibility of geometry buffers writes.
	//record_buffer_memory_barrier( vk.cmd->command_buffer, vk.cmd->vertex_buffer, vk.geometry_buffer_size, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT );

#if 0
	// add explicit layout transition dependency
	if ( vk.fboActive ) {
		record_image_layout_transition( vk.cmd->command_buffer, vk.color_image, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0 );
	} else {
		record_image_layout_transition( vk.cmd->command_buffer, vk.swapchain_images[ vk.swapchain_image_index ], VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, 0 );
	}
#endif

	if ( vk.cmd->vertex_buffer_offset > vk.stats.vertex_buffer_max ) {
		vk.stats.vertex_buffer_max = vk.cmd->vertex_buffer_offset;
	}

	if ( vk.stats.push_size > vk.stats.push_size_max ) {
		vk.stats.push_size_max = vk.stats.push_size;
	}

	vk.cmd->last_pipeline = VK_NULL_HANDLE;

	if ( screenMap ) {
		vk_begin_screenmap_render_pass();
	} else {
		RHI_BeginMainPass();
	}

	// dynamic vertex buffer layout
	vk.cmd->uniform_read_offset = 0;
	vk.cmd->vertex_buffer_offset = 0;
	Com_Memset( vk.cmd->buf_offset, 0, sizeof( vk.cmd->buf_offset ) );
	Com_Memset( vk.cmd->vbo_offset, 0, sizeof( vk.cmd->vbo_offset ) );
	vk.cmd->curr_index_buffer = VK_NULL_HANDLE;
	vk.cmd->curr_index_offset = 0;
	vk.cmd->num_indexes = 0;

	Com_Memset( &vk.cmd->descriptor_set, 0, sizeof( vk.cmd->descriptor_set ) );
	vk.cmd->descriptor_set.start = ~0U;
	//vk.cmd->descriptor_set.end = 0;

	Com_Memset( &vk.cmd->scissor_rect, 0, sizeof( vk.cmd->scissor_rect ) );

	// other stats
	vk.stats.push_size = 0;
	return true;
}

static void vk_resize_geometry_buffer( void ) {
	int i;

	RHI_EndPass();

	VK_CHECK( qvkEndCommandBuffer( vk.cmd->command_buffer ) );

	qvkResetCommandBuffer( vk.cmd->command_buffer, 0 );

	vk_wait_idle();

	vk_release_geometry_buffers();

	vk_create_geometry_buffers( vk.geometry_buffer_size_new );
	vk.geometry_buffer_size_new = 0;

	for ( i = 0; i < NUM_COMMAND_BUFFERS; i++ )
		vk_update_uniform_descriptor( vk.tess[i].uniform_descriptor, vk.tess[i].vertex_buffer );

	vk_host.Print( rhiLog_t::Developer, "...geometry buffer resized to %iK\n", (int)( vk.geometry_buffer_size / 1024 ) );
}


rhiFrameEnd_t vk_impl_EndFrame( bool bloom, bool capture ) {
#ifdef USE_UPLOAD_QUEUE
	VkSemaphore waits[2], signals[2];
	const VkPipelineStageFlags wait_dst_stage_mask[2] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
#else
	const VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
#endif
	VkSubmitInfo submit_info;
	rhiFrameEnd_t result = {};

	if ( vk.frame_count == 0 )
		return result;

	vk.frame_count = 0;

	if ( vk.geometry_buffer_size_new ) {
		vk_resize_geometry_buffer();
		// issue: one frame may be lost during video recording
		// solution: re-record all commands again? (might be complicated though)
		return result;
	}

	if ( vk.fboActive ) {
		vk.cmd->last_pipeline = VK_NULL_HANDLE; // do not restore clobbered descriptors in RHI_Bloom()

		if ( bloom && vk.renderPassIndex != RENDER_PASS_SCREENMAP ) {
			RHI_Bloom( nullptr );
			result.bloomApplied = true;
		}

		if ( capture && vk.capture.image ) {
			RHI_EndPass();

			// render to capture FBO
			vk_begin_render_pass( vk.render_pass.capture, vk.framebuffers.capture, qfalse, vk_config.captureWidth, vk_config.captureHeight );
			qvkCmdBindPipeline( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.capture_pipeline );
			qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout_post_process, 0, 1, &vk.color_descriptor, 0, NULL );

			vk_draw( 4 );
		}

		if ( !vk_host.IsMinimized() ) {
			RHI_EndPass();

			vk.renderWidth = vk_config.windowWidth;
			vk.renderHeight = vk_config.windowHeight;

			vk.renderScaleX = 1.0;
			vk.renderScaleY = 1.0;

			vk_begin_render_pass( vk.render_pass.gamma, vk.framebuffers.gamma[vk.cmd->swapchain_image_index], qfalse, vk.renderWidth, vk.renderHeight );
			qvkCmdBindPipeline( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.gamma_pipeline );
			qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout_post_process, 0, 1, &vk.color_descriptor, 0, NULL );

			vk_draw( 4 );
		}
	}

	RHI_EndPass();

	VK_CHECK( qvkEndCommandBuffer( vk.cmd->command_buffer ) );

	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &vk.cmd->command_buffer;
	if ( !vk_host.IsMinimized() ) {
#ifdef USE_UPLOAD_QUEUE
		if ( vk.image_uploaded != VK_NULL_HANDLE ) {
			waits[0] = vk.cmd->image_acquired;
			waits[1] = vk.image_uploaded;
			submit_info.waitSemaphoreCount = 2;
			submit_info.pWaitSemaphores = &waits[0];
			submit_info.pWaitDstStageMask = &wait_dst_stage_mask[0];
			signals[0] = vk.swapchain_rendering_finished[vk.cmd->swapchain_image_index];
			signals[1] = vk.cmd->rendering_finished2;
			submit_info.signalSemaphoreCount = 2;
			submit_info.pSignalSemaphores = &signals[0];

			vk.rendering_finished = vk.cmd->rendering_finished2;
			vk.image_uploaded = VK_NULL_HANDLE;
		} else if ( vk.rendering_finished != VK_NULL_HANDLE ) {
			waits[0] = vk.cmd->image_acquired;
			waits[1] = vk.rendering_finished;
			submit_info.waitSemaphoreCount = 2;
			submit_info.pWaitSemaphores = &waits[0];
			submit_info.pWaitDstStageMask = &wait_dst_stage_mask[0];
			signals[0] = vk.swapchain_rendering_finished[vk.cmd->swapchain_image_index];
			signals[1] = vk.cmd->rendering_finished2;
			submit_info.signalSemaphoreCount = 2;
			submit_info.pSignalSemaphores = &signals[0];

			vk.rendering_finished = vk.cmd->rendering_finished2;
		} else {
			submit_info.waitSemaphoreCount = 1;
			submit_info.pWaitSemaphores = &vk.cmd->image_acquired;
			submit_info.pWaitDstStageMask = &wait_dst_stage_mask[0];
			submit_info.signalSemaphoreCount = 1;
			submit_info.pSignalSemaphores = &vk.swapchain_rendering_finished[vk.cmd->swapchain_image_index];
		}
#else
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = &vk.cmd->image_acquired;
		submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = &vk.swapchain_rendering_finished[vk.cmd->swapchain_image_index];
#endif
	} else {
		submit_info.waitSemaphoreCount = 0;
		submit_info.pWaitSemaphores = NULL;
		submit_info.pWaitDstStageMask = NULL;
		submit_info.signalSemaphoreCount = 0;
		submit_info.pSignalSemaphores = NULL;
	}

	VK_CHECK( qvkQueueSubmit( vk.queue, 1, &submit_info, vk.cmd->rendering_finished_fence ) );
	vk.cmd->waitForFence = qtrue;

	vk.renderPassIndex = RENDER_PASS_MAIN;
	result.submitted = true;
	return result;
}


rhiStatus_t vk_impl_PresentFrame( void ) {
	VkPresentInfoKHR present_info;
	VkResult res;

	if ( vk_host.IsMinimized() || !vk.cmd->swapchain_image_acquired ) {
		return rhiStatus_t::Success;
	}

	if ( !vk.cmd->waitForFence ) {
		// nothing has been submitted this frame due to geometry buffer overflow?
		return rhiStatus_t::Success;
	}

	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext = NULL;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &vk.swapchain_rendering_finished[vk.cmd->swapchain_image_index];
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &vk.swapchain;
	present_info.pImageIndices = &vk.cmd->swapchain_image_index;
	present_info.pResults = NULL;

	vk.cmd->swapchain_image_acquired = qfalse;

	res = qvkQueuePresentKHR( vk.queue, &present_info );
	switch ( res ) {
	case VK_SUCCESS:
		break;
	case VK_SUBOPTIMAL_KHR:
	case VK_ERROR_OUT_OF_DATE_KHR:
		// swapchain re-creation needed
		vk_restart_swapchain( __func__, res );
		return rhiStatus_t::Success;
	case VK_ERROR_DEVICE_LOST:
		// Let the frontend retain its existing device-loss continuation policy.
		break;
	default:
		// or we don't
		vk_fail( ERR_FATAL, rhiStatus_t::Error, "vkQueuePresentKHR returned %s", vk_result_string( res ) );
	}

	// pickup next command buffer for rendering
	vk.cmd_index++;
	vk.cmd_index %= NUM_COMMAND_BUFFERS;
	vk.cmd = &vk.tess[vk.cmd_index];
	return vk_status( res );
}


static qboolean is_bgr( VkFormat format ) {
	switch ( format ) {
	case VK_FORMAT_B8G8R8A8_UNORM:
	case VK_FORMAT_B8G8R8A8_SNORM:
	case VK_FORMAT_B8G8R8A8_UINT:
	case VK_FORMAT_B8G8R8A8_SINT:
	case VK_FORMAT_B8G8R8A8_SRGB:
	case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
		return qtrue;
	default:
		return qfalse;
	}
}


void vk_impl_ReadPixels( byte *buffer, uint32_t width, uint32_t height ) {
	VkCommandBuffer command_buffer;
	VkDeviceMemory memory;
	VkMemoryRequirements memory_requirements;
	VkMemoryPropertyFlags memory_reqs;
	VkMemoryPropertyFlags memory_flags;
	VkMemoryAllocateInfo alloc_info;
	VkImageSubresource subresource;
	VkSubresourceLayout layout;
	VkImageCreateInfo desc;
	VkImage srcImage;
	VkImageLayout srcImageLayout;
	VkImage dstImage;
	byte *buffer_ptr;
	byte *data;
	uint32_t pixel_width;
	uint32_t i, n;
	qboolean invalidate_ptr;

	VK_CHECK( qvkWaitForFences( vk.device, 1, &vk.cmd->rendering_finished_fence, VK_FALSE, (uint64_t)1e12 ) );

	if ( vk.fboActive ) {
		if ( vk.capture.image ) {
			// dedicated capture buffer
			srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			srcImage = vk.capture.image;
		} else {
			srcImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			srcImage = vk.color_image;
		}
	} else {
		srcImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		srcImage = vk.swapchain_images[vk.cmd->swapchain_image_index];
	}

	Com_Memset( &desc, 0, sizeof( desc ) );

	// Create image in host visible memory to serve as a destination for framebuffer pixels.
	desc.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	desc.pNext = NULL;
	desc.flags = 0;
	desc.imageType = VK_IMAGE_TYPE_2D;
	desc.format = vk.capture_format;
	desc.extent.width = width;
	desc.extent.height = height;
	desc.extent.depth = 1;
	desc.mipLevels = 1;
	desc.arrayLayers = 1;
	desc.samples = VK_SAMPLE_COUNT_1_BIT;
	desc.tiling = VK_IMAGE_TILING_LINEAR;
	desc.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	desc.queueFamilyIndexCount = 0;
	desc.pQueueFamilyIndices = NULL;
	desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VK_CHECK( qvkCreateImage( vk.device, &desc, NULL, &dstImage ) );

	qvkGetImageMemoryRequirements( vk.device, dstImage, &memory_requirements );

	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = NULL;
	alloc_info.allocationSize = memory_requirements.size;

	// host_cached bit is desirable for fast reads
	memory_reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
	alloc_info.memoryTypeIndex = find_memory_type2( memory_requirements.memoryTypeBits, memory_reqs, &memory_flags );
	if ( alloc_info.memoryTypeIndex == (uint32_t)( ~0 ) ) {
		// try less explicit flags, without host_coherent
		memory_reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
		alloc_info.memoryTypeIndex = find_memory_type2( memory_requirements.memoryTypeBits, memory_reqs, &memory_flags );
		if ( alloc_info.memoryTypeIndex == ~0U ) {
			// slowest case
			memory_reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			alloc_info.memoryTypeIndex = find_memory_type2( memory_requirements.memoryTypeBits, memory_reqs, &memory_flags );
			if ( alloc_info.memoryTypeIndex == ~0U ) {
				vk_fail( ERR_FATAL, rhiStatus_t::Error, "%s(): failed to find matching memory type for image capture", __func__ );
			}
		}
	}

	if ( memory_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) {
		invalidate_ptr = qfalse;
	} else {
		// according to specification - must be performed if host_coherent is not set
		invalidate_ptr = qtrue;
	}

	VK_CHECK(qvkAllocateMemory(vk.device, &alloc_info, NULL, &memory));
	VK_CHECK(qvkBindImageMemory(vk.device, dstImage, memory, 0));

	command_buffer = begin_command_buffer();

	if ( srcImageLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ) {
		record_image_layout_transition( command_buffer, srcImage,
			VK_IMAGE_ASPECT_COLOR_BIT,
			srcImageLayout,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			0, 0 );
	}

	record_image_layout_transition( command_buffer, dstImage,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 0 );

	// end_command_buffer( command_buffer );

	// command_buffer = begin_command_buffer();

	if ( vk.blitEnabled ) {
		VkImageBlit region;

		region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.srcSubresource.mipLevel = 0;
		region.srcSubresource.baseArrayLayer = 0;
		region.srcSubresource.layerCount = 1;
		region.srcOffsets[0].x = 0;
		region.srcOffsets[0].y = 0;
		region.srcOffsets[0].z = 0;
		region.srcOffsets[1].x = width;
		region.srcOffsets[1].y = height;
		region.srcOffsets[1].z = 1;
		region.dstSubresource = region.srcSubresource;
		region.dstOffsets[0] = region.srcOffsets[0];
		region.dstOffsets[1] = region.srcOffsets[1];

		qvkCmdBlitImage( command_buffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_NEAREST );

	} else {
		VkImageCopy region;

		region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.srcSubresource.mipLevel = 0;
		region.srcSubresource.baseArrayLayer = 0;
		region.srcSubresource.layerCount = 1;
		region.srcOffset.x = 0;
		region.srcOffset.y = 0;
		region.srcOffset.z = 0;
		region.dstSubresource = region.srcSubresource;
		region.dstOffset = region.srcOffset;
		region.extent.width = width;
		region.extent.height = height;
		region.extent.depth = 1;

		qvkCmdCopyImage( command_buffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );
	}

	end_command_buffer( command_buffer, __func__ );

	// Copy data from destination image to memory buffer.
	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource.mipLevel = 0;
	subresource.arrayLayer = 0;

	qvkGetImageSubresourceLayout( vk.device, dstImage, &subresource, &layout );

	VK_CHECK( qvkMapMemory( vk.device, memory, 0, VK_WHOLE_SIZE, 0, (void**)&data ) );

	if ( invalidate_ptr ) {
		VkMappedMemoryRange range;
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.pNext = NULL;
		range.memory = memory;
		range.size = VK_WHOLE_SIZE;
		range.offset = 0;
		qvkInvalidateMappedMemoryRanges( vk.device, 1, &range );
	}

	data += layout.offset;

	switch ( vk.capture_format ) {
	case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
		pixel_width = 2;
		break;
	case VK_FORMAT_R16G16B16A16_UNORM:
		pixel_width = 8;
		break;
	default:
		pixel_width = 4;
		break;
	}

	buffer_ptr = buffer + width * ( height - 1 ) * 3;
	for ( i = 0; i < height; i++ ) {
		switch ( pixel_width ) {
		case 2: {
			uint16_t *src = (uint16_t *)data;
			for ( n = 0; n < width; n++ ) {
				buffer_ptr[n * 3 + 0] = ( ( src[n] >> 12 ) & 0xF ) << 4;
				buffer_ptr[n * 3 + 1] = ( ( src[n] >> 8 ) & 0xF ) << 4;
				buffer_ptr[n * 3 + 2] = ( ( src[n] >> 4 ) & 0xF ) << 4;
			}
		} break;

		case 4: {
			for ( n = 0; n < width; n++ ) {
				Com_Memcpy( &buffer_ptr[n * 3], &data[n * 4], 3 );
				//buffer_ptr[n*3+0] = data[n*4+0];
				//buffer_ptr[n*3+1] = data[n*4+1];
				//buffer_ptr[n*3+2] = data[n*4+2];
			}
		} break;

		case 8: {
			const uint16_t *src = (uint16_t *)data;
			for ( n = 0; n < width; n++ ) {
				buffer_ptr[n * 3 + 0] = src[n * 4 + 0] >> 8;
				buffer_ptr[n * 3 + 1] = src[n * 4 + 1] >> 8;
				buffer_ptr[n * 3 + 2] = src[n * 4 + 2] >> 8;
			}
		} break;
		}
		buffer_ptr -= width * 3;
		data += layout.rowPitch;
	}

	if ( is_bgr( vk.capture_format ) ) {
		buffer_ptr = buffer;
		for ( i = 0; i < width * height; i++ ) {
			byte tmp = buffer_ptr[0];
			buffer_ptr[0] = buffer_ptr[2];
			buffer_ptr[2] = tmp;
			buffer_ptr += 3;
		}
	}

	qvkDestroyImage( vk.device, dstImage, NULL );
	qvkFreeMemory( vk.device, memory, NULL );

	// restore previous layout
	if ( srcImageLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ) {
		command_buffer = begin_command_buffer();

		record_image_layout_transition( command_buffer, srcImage,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			srcImageLayout, 0, 0 );

		end_command_buffer( command_buffer, "restore layout" );
	}
}


void RHI_Bloom( const float *restoreTransform ) {
	uint32_t i;

	RHI_EndPass(); // end main

	// bloom extraction
	vk_begin_bloom_extract_render_pass();
	qvkCmdBindPipeline( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.bloom_extract_pipeline );
	qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout_post_process, 0, 1, &vk.color_descriptor, 0, NULL );
	vk_draw( 4 );
	RHI_EndPass();

	for ( i = 0; i < VK_NUM_BLOOM_PASSES * 2; i += 2 ) {
		// horizontal blur
		vk_begin_blur_render_pass( i + 0 );
		qvkCmdBindPipeline( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.blur_pipeline[i + 0] );
		qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout_post_process, 0, 1, &vk.bloom_image_descriptor[i + 0], 0, NULL );
		vk_draw( 4 );
		RHI_EndPass();

		// vectical blur
		vk_begin_blur_render_pass( i + 1 );
		qvkCmdBindPipeline( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.blur_pipeline[i + 1] );
		qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout_post_process, 0, 1, &vk.bloom_image_descriptor[i + 1], 0, NULL );
		vk_draw( 4 );
		RHI_EndPass();
#if 0
		// horizontal blur
		vk_begin_blur_render_pass( i+0 );
		qvkCmdBindPipeline( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.blur_pipeline[i+0] );
		qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout_post_process, 0, 1, &vk.bloom_image_descriptor[i+2], 0, NULL );
		vk_draw( 4 );
		RHI_EndPass();

		// vectical blur
		vk_begin_blur_render_pass( i+1 );
		qvkCmdBindPipeline( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.blur_pipeline[i+1] );
		qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout_post_process, 0, 1, &vk.bloom_image_descriptor[i+1], 0, NULL );
		vk_draw( 4 );
		RHI_EndPass();
#endif
	}

	vk_begin_post_bloom_render_pass(); // begin post-bloom
	{
		VkDescriptorSet dset[VK_NUM_BLOOM_PASSES];

		for ( i = 0; i < VK_NUM_BLOOM_PASSES; i++ ) {
			dset[i] = vk.bloom_image_descriptor[( i + 1 ) * 2];
		}

		// blend downscaled buffers to main fbo
		qvkCmdBindPipeline( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.bloom_blend_pipeline );
		qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout_blend, 0, ARRAY_LEN( dset ), dset, 0, NULL );
		vk_draw( 4 );
	}

	// invalidate pipeline state cache
	//vk.cmd->last_pipeline = VK_NULL_HANDLE;

	if ( vk.cmd->last_pipeline != VK_NULL_HANDLE ) {
		// restore last pipeline
		qvkCmdBindPipeline( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.cmd->last_pipeline );

		RHI_PushTransform( restoreTransform );

		// force depth range and viewport/scissor updates
		vk.cmd->depth_range = DEPTH_RANGE_COUNT;

		// restore clobbered descriptor sets
		for ( i = 0; i < VK_NUM_BLOOM_PASSES; i++ ) {
			if ( vk.cmd->descriptor_set.current[i] != VK_NULL_HANDLE ) {
				if ( i == RHI_BINDING_UNIFORM /*|| i == RHI_BINDING_STORAGE*/ )
					qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout, i, 1, &vk.cmd->descriptor_set.current[i], 1, &vk.cmd->descriptor_set.offset[i] );
				else
					qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout, i, 1, &vk.cmd->descriptor_set.current[i], 0, NULL );
			}
		}
	}
}


rhiPipelineCacheKey_t RHI_GetPipelineCacheKey( void ) {
	return vk.pipelineCacheKey;
}

static_assert( sizeof( VkPipelineCacheHeaderVersionOne ) == 32 && offsetof( VkPipelineCacheHeaderVersionOne, pipelineCacheUUID ) == 16 );

rhiStatus_t RHI_RestorePipelineCache( const void *data, uint32_t size ) {
	vk_clear_error();
	if ( !vk.device || !vk.pipelineCache || !data || size < sizeof( VkPipelineCacheHeaderVersionOne ) )
		return rhiStatus_t::Unavailable;
	VkPipelineCacheHeaderVersionOne header;
	memcpy( &header, data, sizeof( header ) );
	if ( header.headerSize < sizeof( header ) || header.headerSize > size || header.headerVersion != VK_PIPELINE_CACHE_HEADER_VERSION_ONE ||
		 header.vendorID != vk.pipelineCacheKey.vendor || header.deviceID != vk.pipelineCacheKey.device ||
		 memcmp( header.pipelineCacheUUID, vk.pipelineCacheKey.uuid, sizeof( header.pipelineCacheUUID ) ) != 0 )
		return rhiStatus_t::Unavailable;
	VkPipelineCacheCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	info.initialDataSize = size;
	info.pInitialData = data;
	VkPipelineCache restored;
	const VkResult result = qvkCreatePipelineCache( vk.device, &info, NULL, &restored );
	if ( result != VK_SUCCESS )
		return vk_status( result );
	qvkDestroyPipelineCache( vk.device, vk.pipelineCache, NULL );
	vk.pipelineCache = restored;
	return rhiStatus_t::Success;
}

rhiStatus_t RHI_ReadPipelineCache( void *data, uint32_t *size ) {
	vk_clear_error();
	if ( !vk.device || !vk.pipelineCache )
		return rhiStatus_t::Unavailable;
	size_t bytes = data ? *size : 0;
	const VkResult result = qvkGetPipelineCacheData( vk.device, vk.pipelineCache, &bytes, data );
	if ( bytes > UINT32_MAX || result == VK_INCOMPLETE )
		return rhiStatus_t::Error;
	*size = (uint32_t)bytes;
	return vk_status( result );
}

rhiStatus_t RHI_Initialize( const rhiDeviceConfig_t *config, const rhiHost_t *host, rhiDeviceInfo_t *info ) {
	vk_config = *config;
	vk_host = *host;
	*info = {};
	vk_device_info = info;
	const rhiStatus_t status = vk_call( [&]() {
		return vk_impl_Initialize();
	} );
	vk_device_info = nullptr;
	return status;
}

rhiStatus_t RHI_InitDescriptors( void ) {
	return vk_call( [&]() {
		vk_impl_InitDescriptors();
		return rhiStatus_t::Success;
	} );
}

rhiStatus_t RHI_ReleaseResources( void ) {
	return vk_call( [&]() {
		vk_impl_ReleaseResources();
		return rhiStatus_t::Success;
	} );
}

rhiStatus_t RHI_Shutdown( void ) {
	return vk_call( [&]() {
		vk_impl_Shutdown();
		return rhiStatus_t::Success;
	} );
}

rhiStatus_t RHI_UploadWorldGeometry( const uint8_t *data, int32_t size ) {
	return vk_call( [&]() {
		vk_impl_UploadWorldGeometry( data, size );
		return rhiStatus_t::Success;
	} );
}

rhiStatus_t RHI_UpdatePostProcess( const rhiPostProcess_t *settings ) {
	vk_post = *settings;
	return vk_call( [&]() {
		vk_impl_UpdatePostProcess( settings->overbrightBits );
		return rhiStatus_t::Success;
	} );
}

rhiStatus_t RHI_ReadPixels( uint8_t *buffer, uint32_t width, uint32_t height ) {
	return vk_call( [&]() {
		vk_impl_ReadPixels( buffer, width, height );
		return rhiStatus_t::Success;
	} );
}

static bool vk_texture_supported( rhiFormat_t format ) {
	if ( format >= rhiFormat_t::BC4 && format <= rhiFormat_t::BC7_SRGB ) {
		if ( !vk.compressionBC )
			return false;
		VkFormatProperties properties;
		qvkGetPhysicalDeviceFormatProperties( vk.physical_device, vk_texture_format( format ), &properties );
		const VkFormatFeatureFlags required = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
		if ( ( properties.optimalTilingFeatures & required ) != required )
			return false;
	}
	return true;
}

rhiStatus_t RHI_CreateTexture( rhiTexture_t *texture, int32_t width, int32_t height, int32_t mipLevels, rhiFormat_t format, rhiAddress_t address, const char *label ) {
	return vk_call( [&]() {
		if ( !vk_texture_supported( format ) )
			return rhiStatus_t::Unavailable;
		vk_impl_CreateTexture( texture, width, height, mipLevels, format, address, label );
		return rhiStatus_t::Success;
	} );
}

rhiStatus_t RHI_UploadTexture( const rhiTexture_t *texture, int32_t x, int32_t y, int32_t width, int32_t height, int32_t mipLevels, const uint8_t *pixels, int32_t bytesPerPixel, bool update ) {
	return vk_call( [&]() {
		vk_impl_UploadTexture( texture, x, y, width, height, mipLevels, pixels, bytesPerPixel, update );
		return rhiStatus_t::Success;
	} );
}

rhiStatus_t RHI_UploadCompressedTexture( const rhiTexture_t *texture, int32_t width, int32_t height, int32_t mipLevels, const uint8_t *blocks, uint32_t size, rhiFormat_t format, bool update ) {
	return vk_call( [&]() {
		int32_t blockBytes;
		switch ( format ) {
		case rhiFormat_t::BC4:
			blockBytes = 8;
			break;
		case rhiFormat_t::BC5:
		case rhiFormat_t::BC7:
		case rhiFormat_t::BC7_SRGB:
			blockBytes = 16;
			break;
		default:
			return rhiStatus_t::Error;
		}
		if ( !texture || !texture->image || !blocks || width < 1 || height < 1 || width > 32768 || height > 32768 || mipLevels < 1 || mipLevels > 16 )
			return rhiStatus_t::Error;
		uint64_t expected = 0;
		int32_t w = width, h = height;
		for ( int32_t level = 0; level < mipLevels; level++ ) {
			expected += (uint64_t)( ( w + 3 ) / 4 ) * ( ( h + 3 ) / 4 ) * blockBytes;
			if ( w == 1 && h == 1 && level + 1 != mipLevels )
				return rhiStatus_t::Error;
			w = MAX( 1, w / 2 );
			h = MAX( 1, h / 2 );
		}
		if ( expected != size || expected > INT32_MAX )
			return rhiStatus_t::Error;
		vk_impl_UploadTexture( texture, 0, 0, width, height, mipLevels, blocks, blockBytes, update, 4 );
		return rhiStatus_t::Success;
	} );
}

rhiStatus_t RHI_ReplaceCompressedTexture( rhiTexture_t *texture, int32_t width, int32_t height, int32_t mipLevels, const uint8_t *blocks, uint32_t size, rhiFormat_t format, rhiAddress_t address, const char *label ) {
	if ( !texture || !blocks || width < 1 || height < 1 || width > 16384 || height > 16384 || mipLevels < 1 || mipLevels > 15 || format < rhiFormat_t::BC4 || format > rhiFormat_t::BC7_SRGB )
		return rhiStatus_t::Error;
	rhiStatus_t status = RHI_WaitIdle();
	if ( status != rhiStatus_t::Success )
		return status;
	rhiTexture_t replacement = {};
	replacement.binding = texture->binding;
	status = vk_call( [&]() {
		if ( !vk_texture_supported( format ) )
			return rhiStatus_t::Unavailable;
		vk_impl_CreateTexture( &replacement, width, height, mipLevels, format, address, label, true, true );
		return rhiStatus_t::Success;
	} );
	if ( status == rhiStatus_t::Success )
		status = RHI_UploadCompressedTexture( &replacement, width, height, mipLevels, blocks, size, format, false );
	if ( status == rhiStatus_t::Success )
		status = RHI_UpdateTextureSampler( &replacement, address, mipLevels > 1 );
	if ( status == rhiStatus_t::Success ) {
		RHI_DestroyTexture( texture );
		*texture = replacement;
	} else {
		// Retain a newly allocated pool binding for a retry; it is map-owned.
		if ( !texture->binding )
			texture->binding = replacement.binding;
		RHI_DestroyTexture( &replacement );
	}
	return status;
}

rhiStatus_t RHI_UpdateTextureSampler( const rhiTexture_t *texture, rhiAddress_t address, bool mipmap ) {
	return vk_call( [&]() {
		vk_impl_UpdateTextureSampler( texture, address, mipmap );
		return rhiStatus_t::Success;
	} );
}

rhiStatus_t RHI_SetTextureFilter( rhiFilter_t minimize, rhiFilter_t magnify, bool *changed ) {
	return vk_call( [&]() {
		return vk_impl_SetTextureFilter( minimize, magnify, changed );
	} );
}

rhiStatus_t RHI_FindPipeline( uint32_t base, const rhiPipelineDesc_t *desc, bool eager, uint32_t *pipeline ) {
	*pipeline = {};
	return vk_call( [&]() {
		*pipeline = vk_impl_FindPipeline( base, desc, eager );
		return rhiStatus_t::Success;
	} );
}

rhiStatus_t RHI_BindPipeline( uint32_t pipeline ) {
	if ( pipeline < vk.pipelines_count && vk.pipelines[pipeline].handle[vk.renderPassIndex] != VK_NULL_HANDLE ) {
		vk_clear_error();
		vk_impl_BindPipeline( pipeline );
		return rhiStatus_t::Success;
	}
	return vk_call( [&]() {
		vk_impl_BindPipeline( pipeline );
		return rhiStatus_t::Success;
	} );
}

rhiStatus_t RHI_BeginFrame( bool screenMap, bool *started ) {
	*started = {};
	return vk_call( [&]() {
		*started = vk_impl_BeginFrame( screenMap );
		if ( *started )
			vk.stats.draw_calls = 0;
		return rhiStatus_t::Success;
	} );
}

rhiStatus_t RHI_EndFrame( bool bloom, bool capture, rhiFrameEnd_t *result ) {
	*result = {};
	return vk_call( [&]() {
		*result = vk_impl_EndFrame( bloom, capture );
		if ( result->submitted )
			vk.stats.frame_draw_calls = vk.stats.draw_calls;
		return rhiStatus_t::Success;
	} );
}

rhiStatus_t RHI_PresentFrame( void ) {
	return vk_call( [&]() {
		return vk_impl_PresentFrame();
	} );
}

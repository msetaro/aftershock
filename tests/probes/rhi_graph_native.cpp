// Observe production target/pass/framebuffer declarations without creating a GPU.
#include "../../engine/renderervk/vk.cpp"
#include <assert.h>
void QDECL Com_Printf( const char *, ... ) {
	abort();
}
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}
template <class... T>
static void row( const char *label, T... values ) {
	printf( "%s", label );
	( ( printf( " %u", (uint32_t)values ) ), ... );
	puts( "" );
}
static uint32_t passCount;
static uint32_t shadowPassCount, shadowImageCount;
static VkResult VKAPI_CALL createPass( VkDevice, const VkRenderPassCreateInfo *p, const VkAllocationCallbacks *, VkRenderPass *handle ) {
	row( "pass", passCount++, p->flags, p->attachmentCount, p->subpassCount, p->dependencyCount );
	assert(p->subpassCount==1 && !p->pNext);
	for ( uint32_t i = 0; i < p->attachmentCount; ++i ) {
		const auto &a = p->pAttachments[i];
		row( "attachment", a.flags, a.format, a.samples, a.loadOp, a.storeOp, a.stencilLoadOp, a.stencilStoreOp, a.initialLayout, a.finalLayout );
	}
	const auto &s = p->pSubpasses[0];
	row( "subpass", s.flags, s.pipelineBindPoint, s.inputAttachmentCount, s.colorAttachmentCount, s.preserveAttachmentCount );
	assert(s.colorAttachmentCount<=1 && s.inputAttachmentCount==0 && s.preserveAttachmentCount==0);
	if ( s.colorAttachmentCount ) {
		row( "color", s.pColorAttachments[0].attachment, s.pColorAttachments[0].layout );
	} else {
		++shadowPassCount;
		assert( p->attachmentCount == 1 && p->pAttachments[0].format == VK_FORMAT_D32_SFLOAT );
		assert( p->pAttachments[0].loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		assert( p->pAttachments[0].storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		assert( p->pAttachments[0].finalLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL );
		assert( s.pDepthStencilAttachment && s.pDepthStencilAttachment->attachment == 0 );
		assert( p->dependencyCount == 2 );
		assert( p->pDependencies[1].srcStageMask == ( VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT ) );
		assert( p->pDependencies[1].srcAccessMask == VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT );
		assert( p->pDependencies[1].dstStageMask == VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT );
		assert( p->pDependencies[1].dstAccessMask == VK_ACCESS_SHADER_READ_BIT );
	}
	if ( s.pDepthStencilAttachment )
		row( "depth", s.pDepthStencilAttachment->attachment, s.pDepthStencilAttachment->layout );
	if ( s.pResolveAttachments )
		row( "resolve", s.pResolveAttachments[0].attachment, s.pResolveAttachments[0].layout );
	for ( uint32_t i = 0; i < p->dependencyCount; ++i ) {
		const auto &d = p->pDependencies[i];
		row( "dependency", d.srcSubpass, d.dstSubpass, d.srcStageMask, d.dstStageMask, d.srcAccessMask, d.dstAccessMask, d.dependencyFlags );
	}
	*handle = (VkRenderPass)(uintptr_t)passCount;
	return VK_SUCCESS;
}
static jmp_buf imageDone;
static uint32_t imageCount;
static VkResult VKAPI_CALL createImage( VkDevice, const VkImageCreateInfo *p, const VkAllocationCallbacks *, VkImage *out ) {
	row( "image", imageCount++, p->flags, p->imageType, p->format, p->extent.width, p->extent.height, p->extent.depth, p->mipLevels, p->arrayLayers, p->samples, p->tiling, p->usage, p->sharingMode, p->queueFamilyIndexCount, p->initialLayout );
	if ( ( p->usage & ( VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT ) ) ==
		 ( VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT ) ) {
		++shadowImageCount;
		assert( p->format == VK_FORMAT_D32_SFLOAT && p->extent.width == 1024 && p->extent.height == 1024 );
		assert( p->samples == VK_SAMPLE_COUNT_1_BIT && !( p->usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT ) );
	}
	*out = (VkImage)(uintptr_t)imageCount;
	return VK_SUCCESS;
}
static void VKAPI_CALL memoryRequirements( VkDevice, VkImage, VkMemoryRequirements *p ) {
	*p = { 1024, 256, 1 };
}
static void VKAPI_CALL memoryProperties( VkPhysicalDevice, VkPhysicalDeviceMemoryProperties *p ) {
	*p = {};
	p->memoryTypeCount = 1;
	p->memoryTypes[0].propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
}
static VkResult VKAPI_CALL allocationBoundary( VkDevice, const VkMemoryAllocateInfo *p, const VkAllocationCallbacks *, VkDeviceMemory * ) {
	row( "allocate", p->allocationSize, p->memoryTypeIndex, num_attachments );
	for ( uint32_t i = 0; i < num_attachments; ++i ) {
		row( "initial", attachments[i].image_layout, attachments[i].aspect_flags, attachments[i].memory_offset );
		*attachments[i].image_view = (VkImageView)(uintptr_t)( 100 + i );
	}
	longjmp( imageDone, 1 ); // All declarations captured; never allocate or create a GPU.
}
static void captureImages() {
	if ( !setjmp( imageDone ) )
		vk_create_attachments();
}
static uint32_t framebufferCount;
static VkResult VKAPI_CALL createFramebuffer( VkDevice, const VkFramebufferCreateInfo *p, const VkAllocationCallbacks *, VkFramebuffer *out ) {
	row( "framebuffer", framebufferCount++, (uintptr_t)p->renderPass, p->flags, p->attachmentCount, p->width, p->height, p->layers );
	for ( uint32_t i = 0; i < p->attachmentCount; ++i )
		row( "view", (uintptr_t)p->pAttachments[i] );
	*out = (VkFramebuffer)(uintptr_t)framebufferCount;
	return VK_SUCCESS;
}
static uint32_t begun, ended, depthBegun, resumed;
static void VKAPI_CALL beginPass( VkCommandBuffer, const VkRenderPassBeginInfo *p, VkSubpassContents ) {
	++begun;
	if ( p->renderPass == vk.render_pass.shadow[0] || p->renderPass == vk.render_pass.shadow[1] ) {
		++depthBegun;
		assert( p->clearValueCount == 1 && p->pClearValues[0].depthStencil.depth == 0 );
		assert( p->renderArea.extent.width == 1024 && p->renderArea.extent.height == 1024 );
	} else if ( p->clearValueCount == 0 ) {
		++resumed;
		assert( p->renderPass == vk.render_pass.resume[0] || p->renderPass == vk.render_pass.resume[1] );
	}
}
static void VKAPI_CALL endPass( VkCommandBuffer ) {
	++ended;
}
static void shadowCommands( bool screen ) {
	vk.cmd = &vk.tess[0];
	begun = ended = depthBegun = resumed = 0;
	if ( screen )
		vk_begin_screenmap_render_pass();
	else
		RHI_BeginMainPass();
	const auto area = RHI_GetRenderArea();
	const auto pass = vk.renderPassIndex;
	vk_world.dirty_depth_attachment = GLS_DEPTHMASK_TRUE;
	assert( !RHI_BeginShadowPass( 2 ) && begun == 1 && ended == 0 );
	assert( RHI_BeginShadowPass( 0 ) );
	assert( RHI_GetRenderArea().width == 1024 );
	assert( RHI_BeginShadowPass( 1 ) );
	assert( depthBegun == 2 && begun == 3 && ended == 2 );
	RHI_EndShadowPass();
	assert( begun == 4 && ended == 3 && resumed == 1 );
	assert( vk.renderPassIndex == pass );
	assert( vk_world.dirty_depth_attachment == GLS_DEPTHMASK_TRUE );
	const auto restored = RHI_GetRenderArea();
	assert( restored.width == area.width && restored.height == area.height );
	assert( restored.scaleX == area.scaleX && restored.scaleY == area.scaleY );
	RHI_EndShadowPass(); // No active shadow pass: do not terminate the resumed scene.
	assert( begun == 4 && ended == 3 );
	RHI_EndPass();
}
int main( int argc, char ** ) {
	qvkCreateRenderPass = createPass;
	qvkCreateFramebuffer = createFramebuffer;
	qvkCreateImage = createImage;
	qvkGetImageMemoryRequirements = memoryRequirements;
	qvkGetPhysicalDeviceMemoryProperties = memoryProperties;
	qvkAllocateMemory = allocationBoundary;
	qvkCmdBeginRenderPass = beginPass;
	qvkCmdEndRenderPass = endPass;
	for ( uint32_t mode = 0; mode < 36; ++mode ) {
		vk = {};
		vk_config = {};
		passCount = imageCount = framebufferCount = shadowPassCount = shadowImageCount = 0;
		vk_config.shadowMapSize = argc > 1 ? 1024 : 0;
		const bool offscreen = mode >= 4;
		const uint32_t options = offscreen ? mode - 4 : mode;
		vk_config.fbo = offscreen;
		vk_config.bloom = options & 1;
		vk_config.stencilBits = options & 2 ? 8 : 0;
		vk.msaaActive = offscreen && ( options & 4 ) ? qtrue : qfalse;
		vkSamples = vk.msaaActive ? VK_SAMPLE_COUNT_4_BIT : VK_SAMPLE_COUNT_1_BIT;
		vk.screenMapSamples = offscreen && ( options & 16 ) ? VK_SAMPLE_COUNT_4_BIT : VK_SAMPLE_COUNT_1_BIT;
		vk.capture.image = offscreen && ( options & 8 ) ? (VkImage)(uintptr_t)1 : VK_NULL_HANDLE;
		vk.depth_format = VK_FORMAT_D24_UNORM_S8_UINT;
		vk.color_format = VK_FORMAT_R16G16B16A16_SFLOAT;
		vk.bloom_format = VK_FORMAT_R8G8B8A8_UNORM;
		vk.capture_format = VK_FORMAT_B8G8R8A8_UNORM;
		vk.present_format.format = VK_FORMAT_B8G8R8A8_SRGB;
		vk.initSwapchainLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		vk.fboActive = vk_config.fbo ? qtrue : qfalse;
		vk_config.supersample = vk.capture.image != VK_NULL_HANDLE;
		vk_config.windowWidth = 640;
		vk_config.windowHeight = 480;
		vk.swapchain_image_count = 2;
		vk.swapchain_image_views[0] = (VkImageView)(uintptr_t)200;
		vk.swapchain_image_views[1] = (VkImageView)(uintptr_t)201;
		vk_config.renderWidth = 640;
		vk_config.renderHeight = 480;
		vk_config.captureWidth = 320;
		vk_config.captureHeight = 240;
		vk.screenMapWidth = 80;
		vk.screenMapHeight = 60;
		row( "mode", mode );
		captureImages();
		vk_create_render_passes();
		vk_create_framebuffers();
		assert(passCount==(offscreen?3u+(vk_config.bloom?10u:0u)+(vk.capture.image?1u:0u):1u)+(argc>1?(offscreen?4u:3u):0u));
		assert( shadowPassCount == ( argc > 1 ? 2u : 0u ) && shadowImageCount == shadowPassCount );
		if ( argc > 1 ) {
			shadowCommands( false );
			if ( offscreen )
				shadowCommands( true );
		}
	}
}

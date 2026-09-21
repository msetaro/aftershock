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
		if ( p->format != VK_FORMAT_D32_SFLOAT ) {
			assert( ( vk_config.occlusionScale || vk_config.depthEffects ) && p->extent.width == 640 && p->extent.height == 480 );
			assert( p->samples == vkSamples && !( p->usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT ) );
			*out = (VkImage)(uintptr_t)imageCount;
			return VK_SUCCESS;
		}
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
	} else if ( p->clearValueCount == 0 && p->renderPass != vk.render_pass.occlusion[0] && p->renderPass != vk.render_pass.occlusion[1] && p->renderPass != vk.render_pass.occlusion[2] && p->renderPass != vk.render_pass.particles && p->renderPass != vk.render_pass.particlesResume && p->renderPass != vk.render_pass.post[0] && p->renderPass != vk.render_pass.post[1] ) {
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
static uint32_t descriptorCount, depthWrites;
static VkSampler nearestSampler;
static VkResult VKAPI_CALL allocateDescriptors( VkDevice, const VkDescriptorSetAllocateInfo *p, VkDescriptorSet *out ) {
	assert( p->descriptorSetCount == 1 );
	*out = (VkDescriptorSet)(uintptr_t)++descriptorCount;
	return VK_SUCCESS;
}
static VkResult VKAPI_CALL createSampler( VkDevice, const VkSamplerCreateInfo *p, const VkAllocationCallbacks *, VkSampler *out ) {
	*out = (VkSampler)(uintptr_t)( 100 + vk.samplers.count );
	if ( p->magFilter == VK_FILTER_NEAREST && p->minFilter == VK_FILTER_NEAREST ) {
		assert( p->addressModeU == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE );
		assert( p->addressModeV == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE );
		assert( !p->anisotropyEnable && !p->compareEnable );
		nearestSampler = *out;
	}
	return VK_SUCCESS;
}
static void VKAPI_CALL updateDescriptors( VkDevice, uint32_t count, const VkWriteDescriptorSet *writes, uint32_t, const VkCopyDescriptorSet * ) {
	for ( uint32_t i = 0; i < count; ++i ) {
		const auto &write = writes[i];
		if ( write.descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER || write.pImageInfo->imageLayout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL )
			continue;
		assert( write.descriptorCount == 1 && write.pImageInfo->sampler == nearestSampler );
		if ( write.dstSet == vk.depth_descriptor ) {
			assert( write.pImageInfo->imageView == vk.depth_sample_view );
			continue;
		}
		const uint32_t index = depthWrites++ % 2;
		assert( write.pImageInfo->imageView == vk.shadow_image_view[index] );
		assert( write.dstSet == vk.shadow_descriptor[index] );
	}
}
static void shadowDescriptors() {
	descriptorCount = depthWrites = 0;
	vk.blitFilter = FILTER_LINEAR;
	vk.maxBoundDescriptorSets = 5;
	vk_impl_InitDescriptors();
	assert( depthWrites == 2 );
	assert( RHI_BindShadowAtlas( 0, 4 ) );
	assert( vk.cmd->descriptor_set.current[4] == vk.shadow_descriptor[0] );
	assert( RHI_BindShadowAtlas( 1, 4 ) );
	assert( vk.cmd->descriptor_set.current[4] == vk.shadow_descriptor[1] );
	assert( !RHI_BindShadowAtlas( 2, 4 ) && !RHI_BindShadowAtlas( 0, 0 ) && !RHI_BindShadowAtlas( 0, 5 ) );
	// Resize and texture-filter changes use this existing descriptor refresh.
	vk_update_attachment_descriptors();
	assert( depthWrites == 4 );
}
static VkResult VKAPI_CALL createDepthPipeline( VkDevice, VkPipelineCache, uint32_t count, const VkGraphicsPipelineCreateInfo *p, const VkAllocationCallbacks *, VkPipeline *out ) {
	assert( count == 1 );
	if ( p->pStages[0].module == vk.modules.direct_vs || p->pStages[1].module == vk.modules.reflection_fs ) {
		assert( p->renderPass == vk.render_pass.main );
		assert( p->stageCount == 2 && p->pStages[1].module == ( p->pStages[0].module == vk.modules.direct_vs ? vk.modules.direct_fs : vk.modules.reflection_fs ) );
		assert( p->pColorBlendState->attachmentCount == 1 );
		const auto &blend = p->pColorBlendState->pAttachments[0];
		assert( blend.blendEnable && blend.srcColorBlendFactor == VK_BLEND_FACTOR_ONE && blend.dstColorBlendFactor == VK_BLEND_FACTOR_ONE );
		assert( p->pDepthStencilState->depthTestEnable && !p->pDepthStencilState->depthWriteEnable );
		assert( p->pDepthStencilState->depthCompareOp == VK_COMPARE_OP_EQUAL );
		assert( p->pVertexInputState->vertexAttributeDescriptionCount == 4 );
		assert( p->pRasterizationState->frontFace == VK_FRONT_FACE_COUNTER_CLOCKWISE );
		*out = (VkPipeline)(uintptr_t)2;
		return VK_SUCCESS;
	}
	assert( p->renderPass == vk.render_pass.shadow[0] );
	assert( p->stageCount == 2 && p->pStages[0].module == vk.modules.shadow_vs && p->pStages[1].module == vk.modules.shadow_fs );
	assert( p->pColorBlendState->attachmentCount == 0 );
	assert( p->pMultisampleState->rasterizationSamples == VK_SAMPLE_COUNT_1_BIT && !p->pMultisampleState->alphaToCoverageEnable );
	assert( p->pDepthStencilState->depthTestEnable && p->pDepthStencilState->depthWriteEnable && !p->pDepthStencilState->stencilTestEnable );
	assert( p->pDepthStencilState->depthCompareOp == VK_COMPARE_OP_GREATER_OR_EQUAL );
	assert( p->pVertexInputState->vertexAttributeDescriptionCount == 3 );
	*out = (VkPipeline)(uintptr_t)1;
	return VK_SUCCESS;
}
static VkResult VKAPI_CALL createOcclusionPipeline( VkDevice, VkPipelineCache, uint32_t count, const VkGraphicsPipelineCreateInfo *p, const VkAllocationCallbacks *, VkPipeline *out ) {
	assert( count == 1 && p->layout == vk.pipeline_layout );
	const bool apply = p->renderPass == vk.render_pass.occlusion[2];
	assert( p->pMultisampleState->rasterizationSamples == ( apply ? vkSamples : VK_SAMPLE_COUNT_1_BIT ) );
	assert( !p->pDepthStencilState->depthTestEnable && !p->pDepthStencilState->depthWriteEnable );
	const auto &blend = p->pColorBlendState->pAttachments[0];
	assert( bool( blend.blendEnable ) == apply );
	if ( apply ) {
		assert( blend.srcColorBlendFactor == VK_BLEND_FACTOR_DST_COLOR && blend.dstColorBlendFactor == VK_BLEND_FACTOR_ZERO );
		assert( !( blend.colorWriteMask & VK_COLOR_COMPONENT_A_BIT ) );
	}
	assert( !p->pStages[1].pSpecializationInfo );
	*out = (VkPipeline)(uintptr_t)1;
	return VK_SUCCESS;
}
static uint32_t occlusionDraws;
static void VKAPI_CALL bindPostPipeline( VkCommandBuffer, VkPipelineBindPoint, VkPipeline pipeline ) {
	assert( pipeline );
}
static void VKAPI_CALL bindPostDescriptors( VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout, uint32_t first, uint32_t count, const VkDescriptorSet *sets, uint32_t offsets, const uint32_t * ) {
	assert( first == 0 && ( ( count == 1 && offsets == 0 ) || ( ( count == 2 || count == 3 || count == 4 ) && offsets == 1 ) ) );
	for ( uint32_t i = 0; i < count; ++i )
		assert( sets[i] );
}
static void VKAPI_CALL drawPost( VkCommandBuffer, uint32_t vertices, uint32_t instances, uint32_t first, uint32_t instance ) {
	assert( vertices == 4 && instances == 1 && first == 0 && instance == 0 );
	++occlusionDraws;
}
static void occlusionCommands() {
	qvkCreateGraphicsPipelines = createOcclusionPipeline;
	for ( uint32_t i = 0; i < 3; ++i ) {
		const auto &node = vk_graph.passes[(uint32_t)rhiGraphPass_t::Occlusion + i];
		vk_create_post_process_pipeline( 4 + (int)i, node.width, node.height );
	}
	qvkCmdBindPipeline = bindPostPipeline;
	qvkCmdBindDescriptorSets = bindPostDescriptors;
	qvkCmdDraw = drawPost;
	uint8_t upload[256] = {};
	vk.cmd->vertex_buffer_ptr = upload;
	vk.cmd->vertex_buffer_offset = 0;
	vk.cmd->uniform_read_offset = 32;
	vk_config.uniformBytes = 64;
	vk.uniform_alignment = vk.uniform_item_size = 64;
	vk.geometry_buffer_size = sizeof( upload );
	const auto descriptors = vk.cmd->descriptor_set;
	RHI_BeginMainPass();
	const float projection[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0.001f, -1, 0, 0, 4, 0 };
	const rhiRect_t viewport = { { 0, 0 }, { 640, 480 } };
	occlusionDraws = 0;
	RHI_Occlusion( projection, &viewport, 32, 0 );
	assert( occlusionDraws == 0 && vk.cmd->vertex_buffer_offset == 0 );
	RHI_Occlusion( projection, &viewport, 32, 1 );
	assert( occlusionDraws == 3 && vk.cmd->uniform_read_offset == 32 );
	assert( !memcmp( descriptors.current, vk.cmd->descriptor_set.current, sizeof( descriptors.current ) ) );
	assert( vk.cmd->descriptor_set.start == 0 && vk.cmd->descriptor_set.end == 4 );
	assert( vk.cmd->last_pipeline == VK_NULL_HANDLE && vk.cmd->depth_range == DEPTH_RANGE_COUNT );
	RHI_EndPass();
}
static uint32_t particlePipelines;
static VkResult VKAPI_CALL createParticlePipeline( VkDevice, VkPipelineCache, uint32_t count, const VkGraphicsPipelineCreateInfo *p, const VkAllocationCallbacks *, VkPipeline *out ) {
	assert( count == 1 && p->renderPass == vk.render_pass.particles );
	assert( p->pMultisampleState->rasterizationSamples == vkSamples );
	assert( !p->pDepthStencilState->depthTestEnable && !p->pDepthStencilState->depthWriteEnable );
	assert( p->pDynamicState && p->pDynamicState->dynamicStateCount == 2 );
	const auto &blend = p->pColorBlendState->pAttachments[0];
	assert( blend.blendEnable && blend.srcColorBlendFactor == VK_BLEND_FACTOR_SRC_ALPHA );
	assert( blend.dstColorBlendFactor == ( particlePipelines % 2 ? VK_BLEND_FACTOR_ONE : VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA ) );
	*out = (VkPipeline)(uintptr_t)++particlePipelines;
	return VK_SUCCESS;
}
static void VKAPI_CALL particleViewport( VkCommandBuffer, uint32_t first, uint32_t count, const VkViewport *view ) {
	assert( first == 0 && count == 1 && view->width == 640 && view->height == 480 );
}
static VkRect2D observedScissor;
static void VKAPI_CALL particleScissor( VkCommandBuffer, uint32_t first, uint32_t count, const VkRect2D *area ) {
	assert( first == 0 && count == 1 );
	observedScissor = *area;
}
static void particleCommands() {
	qvkCreateGraphicsPipelines = createParticlePipeline;
	particlePipelines = 0;
	for ( int i = 0; i < 3; ++i )
		vk_create_post_process_pipeline( 7 + i, 640, 480 );
	qvkCmdBindPipeline = bindPostPipeline;
	qvkCmdBindDescriptorSets = bindPostDescriptors;
	qvkCmdDraw = drawPost;
	qvkCmdSetViewport = particleViewport;
	qvkCmdSetScissor = particleScissor;
	uint8_t upload[256] = {};
	vk.cmd->vertex_buffer_ptr = upload;
	vk.cmd->vertex_buffer_offset = 0;
	vk.cmd->uniform_read_offset = 32;
	vk_config.uniformBytes = sizeof( rhiParticle_t );
	vk.uniform_alignment = 64;
	vk.uniform_item_size = 192;
	vk.geometry_buffer_size = sizeof( upload );
	const auto descriptors = vk.cmd->descriptor_set;
	const rhiRect_t viewport = { { 0, 0 }, { 640, 480 } };
	RHI_BeginMainPass();
	assert( RHI_BeginEffects( &viewport ) );
	rhiParticle_t draw{};
	rhiTexture_t image{};
	image.binding = 123;
	occlusionDraws = 0;
	assert( RHI_DrawParticle( &draw, &image, false ) );
	assert( !RHI_DrawParticle( &draw, &image, true ) ); // Upload exhaustion stays bounded.
	assert( occlusionDraws == 1 && vk.cmd->uniform_read_offset == 32 );
	rhiDecal_t decal{};
	assert( !RHI_DrawDecal( &decal, &image, &image, &viewport ) );
	vk.cmd->vertex_buffer_offset = 0;
	vk_config.uniformBytes = sizeof( decal );
	vk.uniform_item_size = sizeof( upload );
	assert( RHI_DrawDecal( &decal, &image, &image, &viewport ) );
	assert( !RHI_DrawDecal( &decal, &image, &image, &viewport ) );
	assert( occlusionDraws == 2 && vk.cmd->uniform_read_offset == 32 );
	vk.cmd->scissor_rect = { { 0, 0 }, { 640, 480 } };
	const rhiRect_t decalArea = { { 16, 24 }, { 32, 48 } };
	RHI_EffectsScissor( &decalArea );
	RHI_EndEffects();
	rhiRasterState_t raster{};
	raster.scissor = viewport;
	raster.viewport = { 0, 0, 640, 480, 0, 1 };
	raster.depthRange = DEPTH_RANGE_NORMAL;
	vk_update_depth_range( &raster );
	assert( observedScissor.offset.x == 0 && observedScissor.offset.y == 0 );
	assert( observedScissor.extent.width == 640 && observedScissor.extent.height == 480 );
	assert( !memcmp( descriptors.current, vk.cmd->descriptor_set.current, sizeof( descriptors.current ) ) );
	assert( vk.cmd->last_pipeline == VK_NULL_HANDLE && vk.cmd->depth_range == DEPTH_RANGE_NORMAL );
	RHI_EndPass();
}
static VkResult VKAPI_CALL createFilmPipeline( VkDevice, VkPipelineCache, uint32_t count, const VkGraphicsPipelineCreateInfo *p, const VkAllocationCallbacks *, VkPipeline *out ) {
	assert( count == 1 && ( p->renderPass == vk.render_pass.post[0] || p->renderPass == vk.render_pass.post[1] ) );
	const bool film = p->renderPass == vk.render_pass.post[0];
	assert( p->layout == ( film ? vk.pipeline_layout : vk.pipeline_layout_post_process ) );
	assert( p->pMultisampleState->rasterizationSamples == ( film ? VK_SAMPLE_COUNT_1_BIT : vkSamples ) );
	assert( !p->pDepthStencilState->depthTestEnable && !p->pDepthStencilState->depthWriteEnable );
	assert( !p->pColorBlendState->pAttachments[0].blendEnable );
	assert( p->pDynamicState && p->pDynamicState->dynamicStateCount == 2 );
	*out = (VkPipeline)(uintptr_t)( film ? 11 : 12 );
	return VK_SUCCESS;
}
static void filmCommands() {
	qvkCreateGraphicsPipelines = createFilmPipeline;
	vk_create_post_process_pipeline( 10, 640, 480 );
	vk_create_post_process_pipeline( 11, 640, 480 );
	uint8_t upload[128]{};
	vk.cmd->vertex_buffer_ptr = upload;
	vk.cmd->vertex_buffer_offset = 0;
	vk.cmd->uniform_read_offset = 32;
	vk_config.uniformBytes = sizeof( rhiPostDraw_t );
	vk.uniform_item_size = sizeof( upload );
	vk.geometry_buffer_size = sizeof( upload );
	const auto descriptors = vk.cmd->descriptor_set;
	RHI_BeginMainPass();
	rhiPostDraw_t post{};
	rhiTexture_t lut{};
	lut.binding = 123;
	occlusionDraws = 0;
	assert( RHI_DrawPost( &post, &lut ) );
	assert( !RHI_DrawPost( &post, &lut ) );
	assert( occlusionDraws == 2 && vk.cmd->uniform_read_offset == 32 );
	assert( !memcmp( descriptors.current, vk.cmd->descriptor_set.current, sizeof( descriptors.current ) ) );
	assert( vk.cmd->last_pipeline == VK_NULL_HANDLE && vk.cmd->depth_range == DEPTH_RANGE_COUNT );
	RHI_EndPass();
}
int main( int argc, char **argv ) {
	if ( argc == 2 && !strcmp( argv[1], "--hdr" ) ) {
		const VkFormat base = VK_FORMAT_R8G8B8A8_UNORM;
		vk_config = {};
		vk_config.hdr = 2;
		assert(get_hdr_format(base) == base);
		vk_config.fbo = 1;
		vk_config.hdr = 0;
		assert(get_hdr_format(base) == base);
		vk_config.hdr = -1;
		assert(get_hdr_format(base) == VK_FORMAT_B4G4R4A4_UNORM_PACK16);
		vk_config.hdr = 1;
		assert(get_hdr_format(base) == VK_FORMAT_R16G16B16A16_UNORM);
		vk_config.hdr = 2;
		assert(get_hdr_format(base) == VK_FORMAT_R16G16B16A16_SFLOAT);
		puts( "PASS: floating HDR target and unchanged legacy color formats" );
		return 0;
	}
	const bool post = argc > 1 && !strcmp( argv[1], "--post" );
	const bool particles = post || ( argc > 1 && !strcmp( argv[1], "--particles" ) );
	const bool occlusion = argc > 1 && !strcmp( argv[1], "--ssao" );
	qvkCreateRenderPass = createPass;
	qvkCreateFramebuffer = createFramebuffer;
	qvkCreateImage = createImage;
	qvkGetImageMemoryRequirements = memoryRequirements;
	qvkGetPhysicalDeviceMemoryProperties = memoryProperties;
	qvkAllocateMemory = allocationBoundary;
	qvkCmdBeginRenderPass = beginPass;
	qvkCmdEndRenderPass = endPass;
	qvkAllocateDescriptorSets = allocateDescriptors;
	qvkCreateSampler = createSampler;
	qvkUpdateDescriptorSets = updateDescriptors;
	qvkCreateGraphicsPipelines = createDepthPipeline;
	for ( uint32_t mode = occlusion || particles ? 4 : 0; mode < 36; ++mode ) {
		qvkCreateGraphicsPipelines = createDepthPipeline;
		vk = {};
		vk_config = {};
		passCount = imageCount = framebufferCount = shadowPassCount = shadowImageCount = 0;
		vk_config.shadowMapSize = argc > 1 ? 1024 : 0;
		const bool offscreen = mode >= 4;
		const uint32_t options = offscreen ? mode - 4 : mode;
		vk_config.occlusionScale = occlusion ? 1 + mode % 2 : ( particles ? mode % 2 : 0 );
		vk_config.depthEffects = particles;
		vk_config.postProcess = post;
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
		assert(passCount==(offscreen?3u+(vk_config.bloom?10u:0u)+(vk.capture.image?1u:0u):1u)+(argc>1?(offscreen?4u:3u):0u)+(vk_config.occlusionScale?3u:0u)+(particles?2u:0u)+(post?2u:0u));
		assert( shadowPassCount == ( argc > 1 ? 2u : 0u ) && shadowImageCount == shadowPassCount );
		if ( argc > 1 ) {
			shadowCommands( false );
			if ( offscreen )
				shadowCommands( true );
			if ( vk_config.occlusionScale || particles )
				vk.depth_sample_view = (VkImageView)(uintptr_t)500;
			if ( vk_config.occlusionScale ) {
				vk.depth_sample_view = (VkImageView)(uintptr_t)500;
				for ( uint32_t i = 0; i < 3; ++i ) {
					const auto &node = vk_graph.passes[(uint32_t)rhiGraphPass_t::Occlusion + i];
					assert( node.enabled && *vk_graph_pass( (rhiGraphPass_t)( (uint32_t)rhiGraphPass_t::Occlusion + i ) ) );
				}
				assert( vk.framebuffers.occlusion[0] && vk.framebuffers.occlusion[1] );
				const auto &depth = vk_graph.targets[(uint32_t)rhiGraphTarget_t::MainDepth];
				assert( depth.usage & RHI_GRAPH_SAMPLED && !depth.transient );
			}
			if ( post ) {
				assert( vk.post_image && vk.post_image_view );
				assert( vk.render_pass.post[0] && vk.render_pass.post[1] );
				assert( vk.framebuffers.post[0] && vk.framebuffers.post[1] );
				const auto &node = vk_graph.passes[(uint32_t)rhiGraphPass_t::PostApply];
				assert( node.depth == RHI_INVALID_OFFSET && node.resolve == ( vk.msaaActive ? 0u : RHI_INVALID_OFFSET ) );
			}
			shadowDescriptors();
			if ( post )
				assert( vk.post_descriptor );
			vk.modules.shadow_vs = (VkShaderModule)(uintptr_t)11;
			vk.modules.shadow_fs = (VkShaderModule)(uintptr_t)12;
			rhiPipelineDesc_t depth = {};
			depth.shader_type = TYPE_SHADOW;
			depth.state_bits = GLS_DEPTHMASK_TRUE;
			assert( create_pipeline( &depth, RENDER_PASS_SHADOW, 0 ) != VK_NULL_HANDLE );
			vk.renderPassIndex = RENDER_PASS_MAIN;
			const uint32_t pipeline = vk_impl_FindPipeline( 0, &depth, true );
			assert( vk.pipelines[pipeline].handle[RENDER_PASS_SHADOW] && !vk.pipelines[pipeline].handle[RENDER_PASS_MAIN] );
			const int created = vk.pipeline_create_count;
			assert( vk_gen_pipeline( pipeline ) && vk.pipeline_create_count == created );
			vk.modules.direct_vs = (VkShaderModule)(uintptr_t)13;
			vk.modules.direct_fs = (VkShaderModule)(uintptr_t)14;
			auto direct = depth;
			direct.shader_type = TYPE_DIRECT;
			direct.state_bits = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL;
			direct.mirror = qtrue;
			assert( create_pipeline( &direct, RENDER_PASS_MAIN, 0 ) != VK_NULL_HANDLE );
			vk.modules.pbr_vs = (VkShaderModule)(uintptr_t)15;
			vk.modules.reflection_fs = (VkShaderModule)(uintptr_t)16;
			direct.shader_type = TYPE_REFLECTION;
			assert( create_pipeline( &direct, RENDER_PASS_MAIN, 0 ) != VK_NULL_HANDLE );
			if ( vk_config.occlusionScale )
				occlusionCommands();
			if ( particles )
				particleCommands();
			if ( post )
				filmCommands();
		}
	}
}

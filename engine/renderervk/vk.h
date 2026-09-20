#pragma once

#include "../../third_party/vulkan/vulkan.h"
#include "tr_common.h"

#define MAX_SWAPCHAIN_IMAGES 8
#define MIN_SWAPCHAIN_IMAGES_IMM 3
#define MIN_SWAPCHAIN_IMAGES_FIFO   3
#define MIN_SWAPCHAIN_IMAGES_FIFO_0 4
#define MIN_SWAPCHAIN_IMAGES_MAILBOX 3

#define MAX_VK_SAMPLERS 32
#define MAX_VK_PIPELINES ((1024 + 128)*2)

#define VERTEX_BUFFER_SIZE     (4 * 1024 * 1024)  /* by default */
#define VERTEX_BUFFER_SIZE_HI  (8 * 1024 * 1024)

#define STAGING_BUFFER_SIZE    (2 * 1024 * 1024)  /* by default */
#define STAGING_BUFFER_SIZE_HI (24 * 1024 * 1024) /* enough for max.texture size upload with all mip levels at once */

#define IMAGE_CHUNK_SIZE (32 * 1024 * 1024)
#define MAX_IMAGE_CHUNKS 56

#define NUM_COMMAND_BUFFERS 2	// number of command buffers / render semaphores / framebuffer sets

#define USE_REVERSED_DEPTH

//#define USE_UPLOAD_QUEUE

#define VK_NUM_BLOOM_PASSES 4

#ifndef _DEBUG
#define USE_DEDICATED_ALLOCATION
#endif
//#define MIN_IMAGE_ALIGN (128*1024)
#define MAX_ATTACHMENTS_IN_POOL (8+VK_NUM_BLOOM_PASSES*2) // depth + msaa + msaa-resolve + depth-resolve + screenmap.msaa + screenmap.resolve + screenmap.depth + bloom_extract + blur pairs


typedef struct {
	VkSamplerAddressMode address_mode; // clamp/repeat texture addressing mode
	int gl_mag_filter; // GL_XXX mag filter
	int gl_min_filter; // GL_XXX min filter
	qboolean max_lod_1_0; // fixed 1.0 lod
	qboolean noAnisotropy;
} Vk_Sampler_Def;

typedef enum {
	RENDER_PASS_MAIN = 0,
	RENDER_PASS_SCREENMAP,
	RENDER_PASS_POST_BLOOM,
	RENDER_PASS_COUNT
} renderPass_t;


typedef struct VK_Pipeline {
	rhiPipelineDesc_t def;
	VkPipeline handle[RENDER_PASS_COUNT];
} VK_Pipeline_t;


#define TESS_XYZ   (1)
#define TESS_RGBA0 (2)
#define TESS_RGBA1 (4)
#define TESS_RGBA2 (8)
#define TESS_ST0   (16)
#define TESS_ST1   (32)
#define TESS_ST2   (64)
#define TESS_NNN   (128)
#define TESS_VPOS  (256)  // uniform with eyePos
#define TESS_ENV   (512)  // mark shader stage with environment mapping
#define TESS_ENT0  (1024) // uniform with ent.color[0]
#define TESS_ENT1  (2048) // uniform with ent.color[1]
#define TESS_ENT2  (4096) // uniform with ent.color[2]
//
// Initialization.
//

// Initializes VK_Instance structure.
// After calling this function we get fully functional vulkan subsystem.
void vk_initialize( void );

// Called after initialization or renderer restart
void vk_init_descriptors( void );

// Shutdown vulkan subsystem by releasing resources acquired by Vk_Instance.
void vk_shutdown( refShutdownCode_t code );

// Releases vulkan resources allocated during program execution.
// This effectively puts vulkan subsystem into initial state (the state we have after vk_initialize call).
void vk_release_resources( void );

void vk_wait_idle( void );
void vk_queue_wait_idle( void );

//
// Resources allocation.
//
void vk_update_attachment_descriptors( void );
void vk_destroy_samplers( void );


void vk_create_post_process_pipeline( int program_index, uint32_t width, uint32_t height );

//
// Rendering setup.
//

void vk_clear_color( const vec4_t color );
void vk_clear_depth( qboolean clear_stencil );
void vk_begin_frame( void );
void vk_end_frame( void );
void vk_present_frame( void );

void vk_begin_main_render_pass( void );

void vk_bind_index( void );
void vk_bind_index_ext( const int numIndexes, const uint32_t *indexes );
void vk_bind_geometry( uint32_t flags );
void vk_bind_lighting( int stage, int bundle );
void vk_draw_geometry( rhiDepthRange_t depth_range, qboolean indexed );
void vk_draw_dot( uint32_t storage_offset );

void vk_read_pixels( byte *buffer, uint32_t width, uint32_t height ); // screenshots
qboolean vk_bloom( void );

qboolean vk_alloc_vbo( const byte *vbo_data, int vbo_size );
void vk_update_mvp( const float *m );

void vk_bind_index_buffer( VkBuffer buffer, uint32_t offset );
void vk_update_descriptor( int index, VkDescriptorSet descriptor );
void vk_update_descriptor_offset( int index, uint32_t offset );

void vk_update_post_process_pipelines( void );

const char *vk_format_string( VkFormat format );

void VBO_PrepareQueues( void );
void VBO_RenderIBOItems( void );
void VBO_ClearQueue( void );

typedef struct vk_tess_s {
	VkCommandBuffer command_buffer;
	struct {
		uint32_t count;
		uint32_t passScope;
		bool ended[RHI_MAX_TIMINGS];
		char names[RHI_MAX_TIMINGS][32];
	} profile;

	VkSemaphore image_acquired;
	uint32_t swapchain_image_index;
	qboolean swapchain_image_acquired;
#ifdef USE_UPLOAD_QUEUE
	VkSemaphore rendering_finished2;
#endif
	VkFence rendering_finished_fence;
	qboolean waitForFence;

	VkBuffer vertex_buffer;
	byte *vertex_buffer_ptr; // pointer to mapped vertex buffer
	uint32_t vertex_buffer_offset; // VkDeviceSize

	VkDescriptorSet uniform_descriptor;
	uint32_t uniform_read_offset;
	VkDeviceSize buf_offset[8];
	VkDeviceSize vbo_offset[8];

	VkBuffer curr_index_buffer;
	uint32_t curr_index_offset;

	struct {
		uint32_t start, end;
		VkDescriptorSet current[5]; // 0:uniform, 1:color0, 2:color1, 3:color2, 4:fog
		uint32_t offset[1]; // 0 (uniform)
	} descriptor_set;

	rhiDepthRange_t depth_range;
	VkPipeline last_pipeline;

	uint32_t num_indexes; // value from most recent vk_bind_index() call

	VkRect2D scissor_rect;
} vk_tess_t;


// Vk_Instance contains engine-specific vulkan resources that persist entire renderer lifetime.
// This structure is initialized/deinitialized by vk_initialize/vk_shutdown functions correspondingly.
typedef struct {
	VkPhysicalDevice physical_device;
	VkSurfaceFormatKHR base_format;
	VkSurfaceFormatKHR present_format;

	uint32_t queue_family_index;
	VkDevice device;
	VkQueue queue;
	VkQueryPool timestampPool;
	uint32_t timestampBits;
	float timestampPeriod;
	rhiTiming_t timings[RHI_MAX_TIMINGS];
	uint32_t timingCount;

	VkSwapchainKHR swapchain;
	uint32_t swapchain_image_count;
	VkImage swapchain_images[MAX_SWAPCHAIN_IMAGES];
	qboolean swapchain_images_inited[MAX_SWAPCHAIN_IMAGES];
	VkImageView swapchain_image_views[MAX_SWAPCHAIN_IMAGES];
	VkSemaphore swapchain_rendering_finished[MAX_SWAPCHAIN_IMAGES];
	//uint32_t swapchain_image_index;

	VkCommandPool command_pool;
#ifdef USE_UPLOAD_QUEUE
	VkCommandBuffer staging_command_buffer;
#endif

	VkDeviceMemory image_memory[MAX_ATTACHMENTS_IN_POOL];
	uint32_t image_memory_count;

	struct {
		VkRenderPass main;
		VkRenderPass screenmap;
		VkRenderPass gamma;
		VkRenderPass capture;
		VkRenderPass bloom_extract;
		VkRenderPass blur[VK_NUM_BLOOM_PASSES * 2]; // horizontal-vertical pairs
		VkRenderPass post_bloom;
	} render_pass;

	VkDescriptorPool descriptor_pool;
	VkDescriptorSetLayout set_layout_sampler; // combined image sampler
	VkDescriptorSetLayout set_layout_uniform; // dynamic uniform buffer
	VkDescriptorSetLayout set_layout_storage; // feedback buffer

	VkPipelineLayout pipeline_layout; // default shaders
	VkPipelineLayout pipeline_layout_storage; // flare test shader layout
	VkPipelineLayout pipeline_layout_post_process; // post-processing
	VkPipelineLayout pipeline_layout_blend; // post-processing

	VkDescriptorSet color_descriptor;

	VkImage color_image;
	VkImageView color_image_view;

	VkImage bloom_image[1 + VK_NUM_BLOOM_PASSES * 2];
	VkImageView bloom_image_view[1 + VK_NUM_BLOOM_PASSES * 2];

	VkDescriptorSet bloom_image_descriptor[1 + VK_NUM_BLOOM_PASSES * 2];

	VkImage depth_image;
	VkImageView depth_image_view;

	VkImage msaa_image;
	VkImageView msaa_image_view;

	// screenMap
	struct {
		VkDescriptorSet color_descriptor;
		VkImage color_image;
		VkImageView color_image_view;

		VkImage color_image_msaa;
		VkImageView color_image_view_msaa;

		VkImage depth_image;
		VkImageView depth_image_view;

	} screenMap;

	struct {
		VkImage image;
		VkImageView image_view;
	} capture;

	struct {
		VkFramebuffer blur[VK_NUM_BLOOM_PASSES * 2];
		VkFramebuffer bloom_extract;
		VkFramebuffer main[MAX_SWAPCHAIN_IMAGES];
		VkFramebuffer gamma[MAX_SWAPCHAIN_IMAGES];
		VkFramebuffer screenmap;
		VkFramebuffer capture;
	} framebuffers;

#ifdef USE_UPLOAD_QUEUE
	VkSemaphore rendering_finished; // reference to vk.cmd->rendering_finished2
	VkSemaphore image_uploaded2;
	VkSemaphore image_uploaded; // reference to vk.image_uploaded2
#endif

	vk_tess_t tess[NUM_COMMAND_BUFFERS], *cmd;
	int cmd_index;

	struct {
		VkBuffer buffer;
		byte *buffer_ptr;
		VkDeviceMemory memory;
		VkDescriptorSet descriptor;
	} storage;

	uint32_t uniform_item_size;
	uint32_t uniform_alignment;
	uint32_t storage_alignment;

	struct {
		VkBuffer vertex_buffer;
		VkDeviceMemory buffer_memory;
	} vbo;

	// host visible memory that holds vertex, index and uniform data
	VkDeviceMemory geometry_buffer_memory;
	VkDeviceSize geometry_buffer_size;
	VkDeviceSize geometry_buffer_size_new;

	// statistics
	struct {
		VkDeviceSize vertex_buffer_max;
		uint32_t push_size;
		uint32_t push_size_max;
	} stats;

	//
	// Shader modules.
	//
	struct {
		struct {
			VkShaderModule gen[3][2][2][2]; // tx[0,1,2], cl[0,1] env0[0,1] fog[0,1]
			VkShaderModule ident1[2][2][2]; // tx[0,1], env0[0,1] fog[0,1]
			VkShaderModule fixed[2][2][2]; // tx[0,1], env0[0,1] fog[0,1]
			VkShaderModule light[2]; // fog[0,1]
		} vert;
		struct {
			VkShaderModule gen0_df;
			VkShaderModule gen[3][2][2]; // tx[0,1,2] cl[0,1] fog[0,1]
			VkShaderModule ident1[2][2]; // tx[0,1], fog[0,1]
			VkShaderModule fixed[2][2]; // tx[0,1], fog[0,1]
			VkShaderModule ent[1][2]; // tx[0], fog[0,1]
			VkShaderModule light[2][2]; // linear[0,1] fog[0,1]
		} frag;

		VkShaderModule color_fs;
		VkShaderModule color_vs;

		VkShaderModule bloom_fs;
		VkShaderModule blur_fs;
		VkShaderModule blend_fs;

		VkShaderModule gamma_fs;
		VkShaderModule gamma_vs;

		VkShaderModule fog_fs;
		VkShaderModule fog_vs;

		VkShaderModule dot_fs;
		VkShaderModule dot_vs;
	} modules;

	VkPipelineCache pipelineCache;

	VK_Pipeline_t pipelines[MAX_VK_PIPELINES];
	uint32_t pipelines_count;
	uint32_t pipelines_world_base;

	// pipeline statistics
	int32_t pipeline_create_count;


	VkPipeline gamma_pipeline;
	VkPipeline capture_pipeline;
	VkPipeline bloom_extract_pipeline;
	VkPipeline blur_pipeline[VK_NUM_BLOOM_PASSES * 2]; // horizontal & vertical pairs
	VkPipeline bloom_blend_pipeline;

	uint32_t frame_count;
	qboolean active;
	qboolean wideLines;
	qboolean samplerAnisotropy;
	qboolean fragmentStores;
	qboolean dedicatedAllocation;
	qboolean debugMarkers;

	float maxAnisotropy;
	float maxLod;

	VkFormat color_format;
	VkFormat capture_format;
	VkFormat depth_format;
	VkFormat bloom_format;

	VkImageLayout initSwapchainLayout;

	qboolean clearAttachment; // requires VK_IMAGE_USAGE_TRANSFER_DST_BIT for swapchains
	qboolean fboActive;
	qboolean blitEnabled;
	qboolean msaaActive;

	qboolean offscreenRender;

	qboolean windowAdjusted;
	int blitX0;
	int blitY0;
	int blitFilter;

	uint32_t renderWidth;
	uint32_t renderHeight;

	float renderScaleX;
	float renderScaleY;

	renderPass_t renderPassIndex;

	uint32_t screenMapWidth;
	uint32_t screenMapHeight;
	uint32_t screenMapSamples;

	uint32_t image_chunk_size;

	uint32_t maxBoundDescriptorSets;

#ifdef USE_UPLOAD_QUEUE
	VkFence aux_fence;
	qboolean aux_fence_wait;
#endif

	struct staging_buffer_s {
		VkBuffer handle;
		VkDeviceMemory memory;
		VkDeviceSize size;
		byte *ptr; // pointer to mapped staging buffer
#ifdef USE_UPLOAD_QUEUE
		VkDeviceSize offset;
#endif
	} staging_buffer;

	struct samplers_s {
		int count;
		Vk_Sampler_Def def[MAX_VK_SAMPLERS];
		VkSampler handle[MAX_VK_SAMPLERS];
		int filter_min;
		int filter_max;
	} samplers;

	struct defaults_t {
		VkDeviceSize staging_size;
		VkDeviceSize geometry_size;
	} defaults;

	char driverNote[200];

} Vk_Instance;

typedef struct {
	VkDeviceMemory memory;
	VkDeviceSize used;
} ImageChunk;

// Vk_World contains vulkan resources/state requested by the game code.
// It is reinitialized on a map change.
typedef struct {
	//
	// Memory allocations.
	//
	int num_image_chunks;
	ImageChunk image_chunks[MAX_IMAGE_CHUNKS];

	//
	// State.
	//

	// Descriptor sets corresponding to bound texture images.
	//VkDescriptorSet current_descriptor_sets[ MAX_TEXTURE_UNITS ];

	// This flag is used to decide whether framebuffer's depth attachment should be cleared
	// with vmCmdClearAttachment (dirty_depth_attachment != 0), or it have just been
	// cleared by render pass instance clear op (dirty_depth_attachment == 0).
	int dirty_depth_attachment;

	float modelview_transform[16];
} Vk_World;

extern Vk_Instance vk; // shouldn't be cleared during ref re-init
extern Vk_World vk_world; // this data is cleared during ref re-init

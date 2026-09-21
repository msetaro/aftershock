#pragma once

#include <stdint.h>
#include <stddef.h>
#include <type_traits>

// Backend-owned frame uploads remain valid until that frame's fence completes.
// No GPU SDK or scene types are part of this contract.
constexpr uint32_t RHI_INVALID_OFFSET = UINT32_MAX;

struct rhiStats_t {
	uint64_t vertexBytesPeak;
	uint64_t geometryBytes;
	uint64_t stagingBytes;
	uint32_t pushBytesPeak;
	int32_t pipelineHandles;
	uint32_t pipelineDescriptions;
	uint32_t worldPipelineBase;
	int32_t imageChunks;
	int32_t samplers;
	uint32_t frameSlots;
	uint32_t frameDrawCalls; // All indexed/non-indexed commands in the last submitted frame.
};
static_assert( std::is_trivially_copyable_v<rhiStats_t> );

// SHA-256 of offline sources/options/compiler, SPIR-V and interface metadata.
const char *RHI_GetShaderPackageHash( void );
bool RHI_Available( void );
rhiStats_t RHI_GetStats( void );

// Snapshot of device capabilities and the active presentation configuration.
struct rhiCapabilities_t {
	bool active;
	bool wideLines;
	bool fragmentStores;
	bool clearAttachment;
	bool fboActive;
	bool offscreenRender;
	uint32_t maxBoundDescriptorSets;
};
static_assert( std::is_trivially_copyable_v<rhiCapabilities_t> );
rhiCapabilities_t RHI_GetCapabilities( void );

// Pipelines created before this point survive map-resource resets.
void RHI_MarkWorldPipelines( void );

struct rhiFrameState_t {
	bool screenMapPass;
	bool submitted;
	bool captureImage;
};
rhiFrameState_t RHI_GetFrameState( void );
void RHI_InvalidateViewport( void );

// The driver note is borrowed for the device lifetime. Empty optional color/
// capture strings mean the format matches the preceding attachments.
struct rhiDeviceDescription_t {
	const char *driverNote;
	char presentFormat[64];
	char colorFormat[64];
	char captureFormat[64];
	char depthFormat[64];
};
rhiDeviceDescription_t RHI_GetDeviceDescription( void );

enum class rhiStatus_t : uint32_t {
	Success,
	Unavailable,
	OutOfMemory,
	DeviceLost,
	Error
};

struct rhiError_t {
	bool drop;
	const char *message;
};
// Borrowed diagnostic from the last fallible call; empty for status-only errors.
const rhiError_t *RHI_GetError( void );

// These return to the caller before it reports an engine error or unwinds.
[[nodiscard]] rhiStatus_t RHI_WaitIdle( void );
[[nodiscard]] rhiStatus_t RHI_WaitQueue( void );

// Record into the current frame's command list, preserving submission order.
void RHI_DrawIndexed( uint32_t indexCount, uint32_t firstIndex );
void RHI_EndPass( void );
// Suspend a scene pass, clear a sampled-depth atlas, then resume its stored data.
// Atlas 0 is local lights; atlas 1 is the sun. Invalid/disabled atlases return false.
bool RHI_BeginShadowPass( uint32_t atlas );
void RHI_EndShadowPass( void );
bool RHI_BindShadowAtlas( uint32_t atlas, uint32_t slot );

constexpr uint32_t RHI_MAX_TIMINGS = 32;
struct rhiTiming_t {
	char name[32];
	double microseconds;
};
static_assert( std::is_trivially_copyable_v<rhiTiming_t> );

// Scopes belong to the current frame. Unsupported/full pools return INVALID_OFFSET.
uint32_t RHI_BeginScope( const char *name );
void RHI_EndScope( uint32_t scope );
// Borrowed results from a completed frame; valid until the next frame begins.
uint32_t RHI_GetTimings( const rhiTiming_t **timings );

// Copies into the current frame's aligned uniform buffer and updates its binding.
// Returns RHI_INVALID_OFFSET if the upload cannot fit; does not allocate or wait.
uint32_t RHI_UploadUniform( const void *data, uint32_t size );

enum class rhiFormat_t : uint32_t {
	RGBA8,
	BGRA8,
	RGB8,
	BGRA4,
	A1RGB5,
	BC4,
	BC5,
	BC7,
	BC7_SRGB
};

enum class rhiAddress_t : uint32_t {
	Repeat,
	ClampToEdge,
	ClampToBorder
};

// Zero-initialize before first use. Only the selected backend interprets these
// handles; map-lifetime memory and descriptor pools retain their ownership.
struct rhiTexture_t {
	uint64_t image;
	uint64_t view;
	uint64_t binding;
	uint64_t memory; // Zero for pooled images; otherwise separately owned device memory.
};
static_assert( sizeof( rhiTexture_t ) == 32 && std::is_trivially_copyable_v<rhiTexture_t> );

[[nodiscard]] rhiStatus_t RHI_CreateTexture( rhiTexture_t *texture, int32_t width, int32_t height, int32_t mipLevels, rhiFormat_t format, rhiAddress_t address, const char *label );
// Input is a contiguous mip chain already converted to the texture format.
[[nodiscard]] rhiStatus_t RHI_UploadTexture( const rhiTexture_t *texture, int32_t x, int32_t y, int32_t width, int32_t height, int32_t mipLevels, const uint8_t *pixels, int32_t bytesPerPixel, bool update );
// Whole BC mip chain, largest level first, tightly packed 4x4 blocks.
[[nodiscard]] rhiStatus_t RHI_UploadCompressedTexture( const rhiTexture_t *texture, int32_t width, int32_t height, int32_t mipLevels, const uint8_t *blocks, uint32_t size, rhiFormat_t format, bool update );
// Development replacement waits for GPU use, then commits image/view/binding atomically.
// Failure retains the live image; separately owned memory is reclaimed on replacement.
[[nodiscard]] rhiStatus_t RHI_ReplaceCompressedTexture( rhiTexture_t *texture, int32_t width, int32_t height, int32_t mipLevels, const uint8_t *blocks, uint32_t size, rhiFormat_t format, rhiAddress_t address, const char *label );
[[nodiscard]] rhiStatus_t RHI_UpdateTextureSampler( const rhiTexture_t *texture, rhiAddress_t address, bool mipmap );
// Call only after GPU use completes. Binding storage is released by the map pool reset.
void RHI_DestroyTexture( rhiTexture_t *texture );
void RHI_BindTexture( uint32_t slot, const rhiTexture_t *texture );

enum class rhiFilter_t : uint32_t {
	Nearest,
	Linear,
	NearestMipmapNearest,
	LinearMipmapNearest,
	NearestMipmapLinear,
	LinearMipmapLinear
};
// On a changed filter, wait before replacing sampler objects and attachment
// bindings. The caller then updates its mipmapped texture bindings.
[[nodiscard]] rhiStatus_t RHI_SetTextureFilter( rhiFilter_t minimize, rhiFilter_t magnify, bool *changed );

// The two existing geometry pools: map-owned static data and fence-owned frame
// uploads. Offsets use bytes; indices are always uint32_t.
enum class rhiGeometryBuffer_t : uint32_t { World,
	Frame };
constexpr uint32_t RHI_MAX_VERTEX_STREAMS = 8;
struct rhiVertexStream_t {
	const void *data; // frame upload source, copied during this call
	uint32_t size; // frame upload bytes
	uint64_t offset; // world-pool byte offset
};
static_assert( std::is_trivially_copyable_v<rhiVertexStream_t> );
// Only masked streams are updated. Gaps retain their previous per-frame offsets.
void RHI_BindVertexStreams( rhiGeometryBuffer_t pool, uint32_t mask, const rhiVertexStream_t *streams );

void RHI_BindIndices( rhiGeometryBuffer_t buffer, uint32_t offset );
uint32_t RHI_UploadIndices( uint32_t count, const void *indices );

#define RHI_BINDING_STORAGE      0
#define RHI_BINDING_UNIFORM      0
#define RHI_BINDING_TEXTURE0     1
#define RHI_BINDING_TEXTURE1     2
#define RHI_BINDING_TEXTURE2     3
#define RHI_BINDING_FOG_COLLAPSE 4
#define RHI_BINDING_BAKED_LIGHT 4 // PBR uses a separate fog pass.
#define RHI_BINDING_COUNT        5

#define RHI_BINDING_TEXTURE_BASE RHI_BINDING_TEXTURE0
#define RHI_BINDING_FOG_ONLY     RHI_BINDING_TEXTURE1
#define RHI_BINDING_FOG_DLIGHT   RHI_BINDING_TEXTURE1


void RHI_BindScreenMap( uint32_t slot );
void RHI_ResetBinding( int32_t slot );

// Logical shader programs and state bits retain the existing renderer encoding.
// These are engine-owned values, not graphics SDK enums. Zero-initialize pipeline
// descriptions: the existing cache compares their complete object representation.
enum cullType_t : uint32_t {
	CT_FRONT_SIDED = 0,
	CT_BACK_SIDED,
	CT_TWO_SIDED
};

enum rhiShader_t : uint32_t {
	TYPE_COLOR_BLACK,
	TYPE_COLOR_WHITE,
	TYPE_COLOR_GREEN,
	TYPE_COLOR_RED,
	TYPE_FOG_ONLY,
	TYPE_DOT,

	TYPE_SIGNLE_TEXTURE_LIGHTING,
	TYPE_SIGNLE_TEXTURE_LIGHTING_LINEAR,

	TYPE_SIGNLE_TEXTURE_DF,

	TYPE_GENERIC_BEGIN, // start of non-env/env shader pairs
	TYPE_SIGNLE_TEXTURE = TYPE_GENERIC_BEGIN,
	TYPE_SIGNLE_TEXTURE_ENV,

	TYPE_SIGNLE_TEXTURE_IDENTITY,
	TYPE_SIGNLE_TEXTURE_IDENTITY_ENV,

	TYPE_SIGNLE_TEXTURE_FIXED_COLOR,
	TYPE_SIGNLE_TEXTURE_FIXED_COLOR_ENV,

	TYPE_SIGNLE_TEXTURE_ENT_COLOR,
	TYPE_SIGNLE_TEXTURE_ENT_COLOR_ENV,

	TYPE_MULTI_TEXTURE_ADD2_IDENTITY,
	TYPE_MULTI_TEXTURE_ADD2_IDENTITY_ENV,
	TYPE_MULTI_TEXTURE_MUL2_IDENTITY,
	TYPE_MULTI_TEXTURE_MUL2_IDENTITY_ENV,

	TYPE_MULTI_TEXTURE_ADD2_FIXED_COLOR,
	TYPE_MULTI_TEXTURE_ADD2_FIXED_COLOR_ENV,
	TYPE_MULTI_TEXTURE_MUL2_FIXED_COLOR,
	TYPE_MULTI_TEXTURE_MUL2_FIXED_COLOR_ENV,

	TYPE_MULTI_TEXTURE_MUL2,
	TYPE_MULTI_TEXTURE_MUL2_ENV,
	TYPE_MULTI_TEXTURE_ADD2_1_1,
	TYPE_MULTI_TEXTURE_ADD2_1_1_ENV,
	TYPE_MULTI_TEXTURE_ADD2,
	TYPE_MULTI_TEXTURE_ADD2_ENV,

	TYPE_MULTI_TEXTURE_MUL3,
	TYPE_MULTI_TEXTURE_MUL3_ENV,
	TYPE_MULTI_TEXTURE_ADD3_1_1,
	TYPE_MULTI_TEXTURE_ADD3_1_1_ENV,
	TYPE_MULTI_TEXTURE_ADD3,
	TYPE_MULTI_TEXTURE_ADD3_ENV,

	TYPE_BLEND2_ADD,
	TYPE_BLEND2_ADD_ENV,
	TYPE_BLEND2_MUL,
	TYPE_BLEND2_MUL_ENV,
	TYPE_BLEND2_ALPHA,
	TYPE_BLEND2_ALPHA_ENV,
	TYPE_BLEND2_ONE_MINUS_ALPHA,
	TYPE_BLEND2_ONE_MINUS_ALPHA_ENV,
	TYPE_BLEND2_MIX_ALPHA,
	TYPE_BLEND2_MIX_ALPHA_ENV,

	TYPE_BLEND2_MIX_ONE_MINUS_ALPHA,
	TYPE_BLEND2_MIX_ONE_MINUS_ALPHA_ENV,

	TYPE_BLEND2_DST_COLOR_SRC_ALPHA,
	TYPE_BLEND2_DST_COLOR_SRC_ALPHA_ENV,

	TYPE_BLEND3_ADD,
	TYPE_BLEND3_ADD_ENV,
	TYPE_BLEND3_MUL,
	TYPE_BLEND3_MUL_ENV,
	TYPE_BLEND3_ALPHA,
	TYPE_BLEND3_ALPHA_ENV,
	TYPE_BLEND3_ONE_MINUS_ALPHA,
	TYPE_BLEND3_ONE_MINUS_ALPHA_ENV,
	TYPE_BLEND3_MIX_ALPHA,
	TYPE_BLEND3_MIX_ALPHA_ENV,
	TYPE_BLEND3_MIX_ONE_MINUS_ALPHA,
	TYPE_BLEND3_MIX_ONE_MINUS_ALPHA_ENV,

	TYPE_BLEND3_DST_COLOR_SRC_ALPHA,
	TYPE_BLEND3_DST_COLOR_SRC_ALPHA_ENV,

	TYPE_GENERIC_END = TYPE_BLEND3_MIX_ONE_MINUS_ALPHA_ENV,
	TYPE_PBR = TYPE_BLEND3_DST_COLOR_SRC_ALPHA_ENV + 1,
	TYPE_PBR_BAKED,
	TYPE_SHADOW,
	TYPE_DIRECT,
	TYPE_REFLECTION

};

// used with cg_shadows == 2
enum rhiShadow_t : uint32_t {
	SHADOW_DISABLED,
	SHADOW_EDGES,
	SHADOW_FS_QUAD,
};

enum rhiTopology_t : uint32_t {
	TRIANGLE_LIST = 0,
	TRIANGLE_STRIP,
	LINE_LIST,
	POINT_LIST
};

enum rhiDepthRange_t : uint32_t {
	DEPTH_RANGE_NORMAL, // [0..1]
	DEPTH_RANGE_ZERO, // [0..0]
	DEPTH_RANGE_ONE, // [1..1]
	DEPTH_RANGE_WEAPON, // [0..0.3]
	DEPTH_RANGE_COUNT
};

#define GLS_SRCBLEND_ZERO						0x00000001
#define GLS_SRCBLEND_ONE						0x00000002
#define GLS_SRCBLEND_DST_COLOR					0x00000003
#define GLS_SRCBLEND_ONE_MINUS_DST_COLOR		0x00000004
#define GLS_SRCBLEND_SRC_ALPHA					0x00000005
#define GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA		0x00000006
#define GLS_SRCBLEND_DST_ALPHA					0x00000007
#define GLS_SRCBLEND_ONE_MINUS_DST_ALPHA		0x00000008
#define GLS_SRCBLEND_ALPHA_SATURATE				0x00000009
#define GLS_SRCBLEND_BITS						0x0000000f

#define GLS_DSTBLEND_ZERO						0x00000010
#define GLS_DSTBLEND_ONE						0x00000020
#define GLS_DSTBLEND_SRC_COLOR					0x00000030
#define GLS_DSTBLEND_ONE_MINUS_SRC_COLOR		0x00000040
#define GLS_DSTBLEND_SRC_ALPHA					0x00000050
#define GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA		0x00000060
#define GLS_DSTBLEND_DST_ALPHA					0x00000070
#define GLS_DSTBLEND_ONE_MINUS_DST_ALPHA		0x00000080
#define GLS_DSTBLEND_BITS						0x000000f0

#define GLS_BLEND_BITS							(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)

#define GLS_DEPTHMASK_TRUE						0x00000100

#define GLS_POLYMODE_LINE						0x00000200

#define GLS_DEPTHTEST_DISABLE					0x00000400
#define GLS_DEPTHFUNC_EQUAL						0x00000800

#define GLS_ATEST_GT_0							0x00001000
#define GLS_ATEST_LT_80							0x00002000
#define GLS_ATEST_GE_80							0x00003000
#define GLS_ATEST_BITS							0x00003000

#define GLS_DEFAULT								GLS_DEPTHMASK_TRUE

struct rhiPipelineDesc_t {
	rhiShader_t shader_type;
	uint32_t state_bits; // GLS_XXX flags
	cullType_t face_culling;
	uint32_t polygon_offset;
	uint32_t mirror;
	rhiShadow_t shadow_phase;
	rhiTopology_t primitives;
	int32_t line_width;
	int32_t fog_stage; // off, fog-in / fog-out
	int32_t abs_light;
	int32_t allow_discard;
	int32_t acff; // none, rgb, rgba, alpha
	struct {
		uint8_t rgb;
		uint8_t alpha;
	} color;
};
static_assert( sizeof( rhiPipelineDesc_t ) == 52 && alignof( rhiPipelineDesc_t ) == 4 );
static_assert( std::is_trivially_copyable_v<rhiPipelineDesc_t> );

[[nodiscard]] rhiStatus_t RHI_FindPipeline( uint32_t base, const rhiPipelineDesc_t *desc, bool eager, uint32_t *pipeline );
void RHI_GetPipelineDesc( uint32_t pipeline, rhiPipelineDesc_t *desc );
[[nodiscard]] rhiStatus_t RHI_BindPipeline( uint32_t pipeline );

struct rhiRect_t {
	struct {
		int32_t x, y;
	} offset;
	struct {
		uint32_t width, height;
	} extent;
};
struct rhiViewport_t {
	float x, y, width, height, minDepth, maxDepth;
};
struct rhiRasterState_t {
	rhiDepthRange_t depthRange;
	rhiRect_t scissor;
	rhiViewport_t viewport;
};
struct rhiRenderArea_t {
	uint32_t width, height;
	float scaleX, scaleY;
};
rhiRenderArea_t RHI_GetRenderArea( void );
// Return false after upload exhaustion; descriptor/depth state stays untouched.
bool RHI_PrepareDraw( const rhiRasterState_t *raster, const rhiTexture_t *fallback );
void RHI_Draw( uint32_t vertexCount );
void RHI_DrawBoundIndices( void );
// Null data clears the bound count without uploading or rebinding a buffer.
void RHI_BindIndexData( uint32_t count, const uint32_t *indices );
void RHI_ClearColor( const float *color, const rhiRect_t *rect );
void RHI_ClearDepth( bool stencil, const rhiRect_t *rect );

// Exact 4x4 shader transform bytes; matrix generation belongs to the frontend.
void RHI_PushTransform( const float *matrix );
void RHI_Bloom( const float *restoreTransform );
// Projection is the frontend perspective matrix; viewport is in render pixels.
struct rhiParticle_t {
	float clip[4][4], uv[4][4], color[4], depth[4];
};
static_assert( sizeof( rhiParticle_t ) == 160 && offsetof( rhiParticle_t, depth ) == 144 );
bool RHI_BeginParticles( const rhiRect_t *viewport );
bool RHI_DrawParticle( const rhiParticle_t *particle, const rhiTexture_t *texture, bool additive );
void RHI_EndParticles();
void RHI_Occlusion( const float *projection, const rhiRect_t *viewport, float radius, float strength );
// Existing one-frame delayed, coherent visibility storage; no additional wait.
bool RHI_ReadVisibility( uint32_t index );
void RHI_DrawVisibility( uint32_t index, uint32_t vertexCount, const rhiRasterState_t *raster );

// Duplicate stereo begin calls keep the current command list and return false.
[[nodiscard]] rhiStatus_t RHI_BeginFrame( bool screenMap, bool *started );
struct rhiFrameEnd_t {
	bool submitted, bloomApplied;
};
[[nodiscard]] rhiStatus_t RHI_EndFrame( bool bloom, bool capture, rhiFrameEnd_t *result );
void RHI_BeginMainPass( void );
[[nodiscard]] rhiStatus_t RHI_PresentFrame( void );

enum class rhiLog_t : uint32_t { Info,
	Developer,
	Warning,
	Error };
// Host services retain the existing engine allocator and platform ownership.
// Allocate/Free failures are process-fatal; GPU failures use returned statuses.
struct rhiHost_t {
	void *( *Allocate )( size_t bytes );
	void ( *Free )( void *pointer );
	void ( *Print )( rhiLog_t level, const char *format, ... );
	bool ( *IsMinimized )( void );
	int32_t ( *SwapInterval )( void );
	void *( *GetInstanceProcAddr )( uint64_t instance, const char *name );
	bool ( *CreateSurface )( uint64_t instance, uint64_t *surface );
};
static_assert( std::is_trivially_copyable_v<rhiHost_t> );

// Persisted identity: shader package plus native device/driver compatibility.
struct rhiPipelineCacheKey_t {
	uint32_t vendor, device, driver;
	uint8_t uuid[16];
	char shaderPackage[64];
};
static_assert( sizeof( rhiPipelineCacheKey_t ) == 92 && alignof( rhiPipelineCacheKey_t ) == 4 );
static_assert( offsetof( rhiPipelineCacheKey_t, uuid ) == 12 && offsetof( rhiPipelineCacheKey_t, shaderPackage ) == 28 );
static_assert( std::is_trivially_copyable_v<rhiPipelineCacheKey_t> );
rhiPipelineCacheKey_t RHI_GetPipelineCacheKey( void );
// Restore only before frontend pipelines are created. A miss keeps the empty cache.
[[nodiscard]] rhiStatus_t RHI_RestorePipelineCache( const void *data, uint32_t size );
// A null data pointer queries the size; otherwise size is in/out buffer capacity.
[[nodiscard]] rhiStatus_t RHI_ReadPipelineCache( void *data, uint32_t *size );

struct rhiDeviceConfig_t {
	int32_t renderWidth, renderHeight;
	int32_t windowWidth, windowHeight;
	int32_t captureWidth, captureHeight;
	int32_t depthBits, stencilBits;
	int32_t maxTextureSize, maxTextureUnits;
	uint32_t maxImages, maxVisibilityTests, uniformBytes;
	int32_t fbo;
	int32_t bloom;
	int32_t hdr;
	int32_t presentBits;
	int32_t device;
	int32_t anisotropy;
	int32_t maxAnisotropy;
	int32_t multisample;
	int32_t supersample;
	int32_t renderScale;
	float offsetUnits;
	float offsetFactor;
	rhiFilter_t textureMin, textureMag;
	bool textureFilterValid;
	uint32_t shadowMapSize;
	uint32_t occlusionScale;
	bool softParticles = false;
};
struct rhiDeviceInfo_t {
	char renderer[1024], vendor[1024], version[1024], extensions[8192];
	int32_t maxTextureSize, textureUnits;
};
static_assert( std::is_trivially_copyable_v<rhiDeviceConfig_t> && std::is_trivially_copyable_v<rhiDeviceInfo_t> );
// Config is copied. Info is an output borrowed only for the duration of this call.
[[nodiscard]] rhiStatus_t RHI_Initialize( const rhiDeviceConfig_t *config, const rhiHost_t *host, rhiDeviceInfo_t *info );
[[nodiscard]] rhiStatus_t RHI_InitDescriptors( void );
[[nodiscard]] rhiStatus_t RHI_ReleaseResources( void );
// Renderer context retention uses ReleaseResources alone; Shutdown destroys it.
[[nodiscard]] rhiStatus_t RHI_Shutdown( void );
[[nodiscard]] rhiStatus_t RHI_ReadPixels( uint8_t *buffer, uint32_t width, uint32_t height );
[[nodiscard]] rhiStatus_t RHI_UploadWorldGeometry( const uint8_t *data, int32_t size );
struct rhiPostProcess_t {
	int32_t overbrightBits;
	float gamma;
	float greyscale;
	float bloomThreshold;
	float bloomIntensity;
	int32_t bloomThresholdMode;
	int32_t bloomModulate;
	int32_t dither;
};
static_assert( std::is_trivially_copyable_v<rhiPostProcess_t> );
[[nodiscard]] rhiStatus_t RHI_UpdatePostProcess( const rhiPostProcess_t *settings );

// Fixed reference pass graph. Declarations are compiled at target creation/resize;
// individual frames may omit passes. No aliasing or per-frame resource allocation.
constexpr uint32_t RHI_GRAPH_BLOOM_PASSES = 4;
enum class rhiGraphTarget_t : uint32_t {
	Bloom0,
	Bloom1,
	Bloom2,
	Bloom3,
	Bloom4,
	Bloom5,
	Bloom6,
	Bloom7,
	Bloom8,
	MainColor,
	ScreenMsaa,
	ScreenColor,
	ScreenDepth,
	MainMsaa,
	Capture,
	MainDepth,
	Present,
	LocalShadow,
	SunShadow,
	Occlusion,
	OcclusionBlur,
	Count
};
enum class rhiGraphPass_t : uint32_t {
	ScreenMap,
	Main,
	BloomExtract,
	Blur0,
	Blur1,
	Blur2,
	Blur3,
	Blur4,
	Blur5,
	Blur6,
	Blur7,
	PostBloom,
	Capture,
	Gamma,
	LocalShadow,
	SunShadow,
	MainResume,
	ScreenResume,
	Occlusion,
	OcclusionBlur,
	OcclusionApply,
	Particles,
	ParticlesResume,
	Count
};
enum class rhiGraphFormat_t : uint32_t { Color,
	Depth,
	Bloom,
	Capture,
	Present,
	ShadowDepth,
	Occlusion };
enum class rhiGraphLayout_t : uint32_t { Undefined,
	Sampled,
	Color,
	Depth,
	TransferSource,
	Present,
	DepthSampled };
enum class rhiGraphLoad_t : uint32_t { Discard,
	Clear,
	Load };
enum class rhiGraphStore_t : uint32_t { Discard,
	Store };
enum class rhiGraphStage_t : uint32_t { Fragment,
	ColorOutput,
	DepthTests,
	SceneAttachments,
	FragmentColor };
enum : uint32_t {
	RHI_GRAPH_COLOR = 1,
	RHI_GRAPH_SAMPLED = 2,
	RHI_GRAPH_TRANSFER_SOURCE = 4,
	RHI_GRAPH_DEPTH = 8,
	RHI_GRAPH_COLOR_READ = 1,
	RHI_GRAPH_COLOR_WRITE = 2,
	RHI_GRAPH_SHADER_READ = 4,
	RHI_GRAPH_DEPTH_READ = 8,
	RHI_GRAPH_DEPTH_WRITE = 16
};
struct rhiGraphConfig_t {
	uint32_t renderWidth, renderHeight, windowWidth, windowHeight;
	uint32_t captureWidth, captureHeight, screenWidth, screenHeight;
	uint32_t samples, screenSamples;
	bool offscreen, bloom, capture, stencil;
	uint32_t shadowSize; // Zero disables both depth atlases.
	uint32_t occlusionScale; // 0 off, 1 full resolution, 2 half resolution; requires offscreen.
	bool softParticles = false;
};
struct rhiGraphTargetDesc_t {
	uint32_t width, height, samples, usage;
	rhiGraphFormat_t format;
	rhiGraphLayout_t initialLayout;
	uint32_t firstUse, lastUse;
	bool enabled, imported, persistent, exported, transient;
};
struct rhiGraphAttachment_t {
	rhiGraphTarget_t target;
	rhiGraphLoad_t load, stencilLoad;
	rhiGraphStore_t store, stencilStore;
	rhiGraphLayout_t initialLayout, finalLayout;
};
struct rhiGraphDependency_t {
	rhiGraphStage_t sourceStage, destinationStage;
	uint32_t sourceAccess, destinationAccess;
	bool incoming, byRegion;
};
struct rhiGraphPassDesc_t {
	uint32_t width, height, readMask, writeMask, dependencyMask;
	uint32_t attachmentCount, dependencyCount, color, depth, resolve;
	rhiGraphAttachment_t attachments[3];
	rhiGraphDependency_t dependencies[2];
	bool enabled;
};
struct rhiGraph_t {
	rhiGraphTargetDesc_t targets[(uint32_t)rhiGraphTarget_t::Count];
	rhiGraphPassDesc_t passes[(uint32_t)rhiGraphPass_t::Count];
	rhiGraphTarget_t targetOrder[(uint32_t)rhiGraphTarget_t::Count - 1];
	rhiGraphPass_t passOrder[(uint32_t)rhiGraphPass_t::Count];
	rhiGraphPass_t executionOrder[(uint32_t)rhiGraphPass_t::Count];
	uint32_t targetCount, passCount, executionCount;
};
static_assert( std::is_trivially_copyable_v<rhiGraph_t> );
static_assert( (uint32_t)rhiGraphTarget_t::Count <= 32 && (uint32_t)rhiGraphPass_t::Count <= 32 );
// First/last use describe the frame's possible pass interval. Persistent contents
// survive skipped passes; exported outputs remain available for existing readback.
// All backing storage is retained until the existing resize/restart teardown.
[[nodiscard]] bool RHI_CompileGraph( const rhiGraphConfig_t *config, rhiGraph_t *graph );

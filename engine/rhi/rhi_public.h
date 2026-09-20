#pragma once

#include <stdint.h>
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
};
static_assert( std::is_trivially_copyable_v<rhiStats_t> );

bool RHI_Available( void );
rhiStats_t RHI_GetStats( void );

enum class rhiStatus_t : uint32_t {
	Success,
	Unavailable,
	OutOfMemory,
	DeviceLost,
	Error
};

// These return to the caller before it reports an engine error or unwinds.
rhiStatus_t RHI_WaitIdle( void );
rhiStatus_t RHI_WaitQueue( void );

// Record into the current frame's command list, preserving submission order.
void RHI_DrawIndexed( uint32_t indexCount, uint32_t firstIndex );
void RHI_EndPass( void );

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
	A1RGB5
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
};
static_assert( sizeof( rhiTexture_t ) == 24 && std::is_trivially_copyable_v<rhiTexture_t> );

void RHI_CreateTexture( rhiTexture_t *texture, int32_t width, int32_t height, int32_t mipLevels,
	rhiFormat_t format, rhiAddress_t address, const char *label );
// Input is the existing contiguous RGBA8 mip chain; conversion preserves its bytes.
void RHI_UploadTexture( const rhiTexture_t *texture, rhiFormat_t format, int32_t x, int32_t y,
	int32_t width, int32_t height, int32_t mipLevels, uint8_t *pixels, int32_t size, bool update );
void RHI_UpdateTextureSampler( const rhiTexture_t *texture, rhiAddress_t address, bool mipmap );
// Call only after GPU use completes. Binding storage is released by the map pool reset.
void RHI_DestroyTexture( rhiTexture_t *texture );
void RHI_BindTexture( uint32_t slot, const rhiTexture_t *texture );

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

	TYPE_GENERIC_END = TYPE_BLEND3_MIX_ONE_MINUS_ALPHA_ENV

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

uint32_t RHI_FindPipeline( uint32_t base, const rhiPipelineDesc_t *desc, bool eager );
void RHI_GetPipelineDesc( uint32_t pipeline, rhiPipelineDesc_t *desc );
void RHI_BindPipeline( uint32_t pipeline );

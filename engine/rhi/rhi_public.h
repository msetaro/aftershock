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

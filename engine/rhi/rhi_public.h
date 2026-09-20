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

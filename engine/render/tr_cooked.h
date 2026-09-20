#pragma once

#include "../rhi/rhi_public.h"

// Borrowed views into a versioned cooked file, valid until FS_FreeFile.
struct cookedMip_t {
	const uint8_t *data;
	uint32_t size;
};
struct cookedTexture_t {
	uint32_t width, height, mipLevels, size;
	rhiFormat_t format;
	uint8_t contentHash[32];
	cookedMip_t levels[16];
};
static_assert( std::is_trivially_copyable_v<cookedTexture_t> );
bool R_ReadCookedTexture( const void *data, size_t size, cookedTexture_t *texture );

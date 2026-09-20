#pragma once

#include <stdint.h>
#include <type_traits>

struct devUiVertex_t {
	float x, y, u, v;
	uint32_t color;
};
struct devUiCommand_t {
	uint32_t texture, firstIndex, indexCount;
	float clip[4];
};
struct devUiDraw_t {
	const devUiVertex_t *vertices;
	const uint32_t *indices;
	const devUiCommand_t *commands;
	uint32_t vertexCount, indexCount, commandCount;
};
static_assert( sizeof( devUiVertex_t ) == 20 && alignof( devUiVertex_t ) == 4 );
static_assert( sizeof( devUiCommand_t ) == 28 && alignof( devUiCommand_t ) == 4 );
static_assert( std::is_trivially_copyable_v<devUiDraw_t> );

// Copies of frontend registries; no renderer-private pointers cross the ABI.
struct devImage_t {
	char name[256];
	uint32_t texture;
	int32_t width, height, uploadWidth, uploadHeight;
	uint32_t flags, format, reloads;
};
struct devMaterial_t {
	char name[64];
	float sort;
	int32_t stages, cull, surfaceFlags, contentFlags;
	uint32_t reloads;
	bool explicitDefinition, fallback;
	bool present[8];
	uint32_t stateBits[8];
	uint32_t textures[8][3];
};
struct devGpuTiming_t {
	char name[32];
	double microseconds;
};
static_assert( std::is_trivially_copyable_v<devImage_t> && std::is_trivially_copyable_v<devMaterial_t> );

struct devModel_t {
	char name[64];
	int32_t type, frames, bytes;
	uint32_t reloads;
};
static_assert( std::is_trivially_copyable_v<devModel_t> );

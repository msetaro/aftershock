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

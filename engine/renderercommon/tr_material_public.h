#pragma once

#include <stddef.h>
#include <stdint.h>
#include <type_traits>

// Linear factors, multiplied by the cooked texture samples. ASMAT v2 payload.
struct materialParams_t {
	float color[4], emissive[3];
	float metallic, roughness, normalScale, alphaCutoff;
	uint32_t flags; // 1: double sided, 2: unlit, 4: blend, 8: mask.
};
static_assert( sizeof( materialParams_t ) == 48 && offsetof( materialParams_t, flags ) == 44 );
static_assert( std::is_trivially_copyable_v<materialParams_t> );

enum : uint32_t {
	MATERIAL_OVERRIDE_COLOR = 1,
	MATERIAL_OVERRIDE_EMISSIVE = 2,
	MATERIAL_OVERRIDE_METALLIC = 4,
	MATERIAL_OVERRIDE_ROUGHNESS = 8,
	MATERIAL_OVERRIDE_NORMAL_SCALE = 16,
	MATERIAL_OVERRIDE_ALPHA_CUTOFF = 32
};
// Select absolute factor replacements. Pipeline flags stay with the material.
struct materialOverride_t {
	uint32_t mask;
	materialParams_t values;
};
static_assert( sizeof( materialOverride_t ) == 52 && std::is_trivially_copyable_v<materialOverride_t> );

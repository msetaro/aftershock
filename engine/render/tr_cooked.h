#pragma once

#include "../rhi/rhi_public.h"
#include "../renderercommon/tr_material_public.h"

// Borrowed views into a versioned cooked file, valid until FS_FreeFile.
struct cookedMip_t {
	const uint8_t *data;
	uint32_t size;
};
struct cookedTexture_t {
	uint32_t width, height, mipLevels, size;
	rhiFormat_t format;
	uint8_t contentHash[32];
	uint8_t fileHash[32];
	cookedMip_t levels[16];
};
static_assert( std::is_trivially_copyable_v<cookedTexture_t> );
bool R_ReadCookedTexture( const void *data, size_t size, cookedTexture_t *texture );

struct cookedMaterial_t {
	float color[4]; // Source factors, already baked into the cooked texture.
	float alphaCutoff;
	uint32_t flags; // 1: double sided, 2: unlit, 4: blend, 8: mask.
	char texture[64];
};
static_assert( sizeof( cookedMaterial_t ) == 88 && offsetof( cookedMaterial_t, texture ) == 24 && std::is_trivially_copyable_v<cookedMaterial_t> );
bool R_ReadCookedMaterial( const void *data, size_t size, cookedMaterial_t *material, uint8_t fileHash[32] = nullptr );

struct cookedPbrMaterial_t {
	materialParams_t params;
	char textures[3][64]; // base/opacity, normal/roughness, emissive/metallic.
};
static_assert( sizeof( cookedPbrMaterial_t ) == 240 && offsetof( cookedPbrMaterial_t, textures ) == 48 );
static_assert( std::is_trivially_copyable_v<cookedPbrMaterial_t> );
bool R_ReadPbrMaterial( const void *data, size_t size, cookedPbrMaterial_t *material, uint8_t fileHash[32] = nullptr );
bool R_ResolveMaterialParams( const materialParams_t *base, const materialOverride_t *instance, materialParams_t *result );

struct cookedLodEntry_t {
	char path[64];
	uint8_t hash[32];
};
struct cookedLods_t {
	uint8_t baseHash[32];
	uint32_t count;
	cookedLodEntry_t levels[3];
};
static_assert( sizeof( cookedLodEntry_t ) == 96 && offsetof( cookedLods_t, levels ) == 36 && std::is_trivially_copyable_v<cookedLods_t> );
bool R_ReadCookedLods( const void *data, size_t size, const uint8_t baseHash[32], cookedLods_t *lods );

struct cookedPost_t {
	char name[32], lut[64];
	float exposureEV, sharpen, vignette, grain, lutStrength, focusDistance, focusRange, dofRadius, motionBlur;
};
static_assert( sizeof( cookedPost_t ) == 132 && offsetof( cookedPost_t, exposureEV ) == 96 && std::is_trivially_copyable_v<cookedPost_t> );
bool R_ReadCookedPost( const void *data, size_t size, cookedPost_t *settings );

struct cookedEntry_t {
	char path[64];
	uint8_t hash[32];
	uint32_t size, kind;
};
static_assert( sizeof( cookedEntry_t ) == 104 && offsetof( cookedEntry_t, size ) == 96 && std::is_trivially_copyable_v<cookedEntry_t> );
struct cookedIndex_t {
	const uint8_t *entries;
	uint32_t count;
};
bool R_ReadCookedIndex( const void *data, size_t size, const uint8_t revision[32], cookedIndex_t *index );
bool R_CookedHashMatches( const void *data, size_t size, const uint8_t hash[32] );

enum class cookedModelStatus_t { Legacy,
	Valid,
	Invalid };
cookedModelStatus_t R_ReadCookedModel( const void *data, size_t size, uint8_t hash[32] );

#ifdef AFTERSHOCK_DEVTOOLS
void R_ReloadEffects( const cookedIndex_t *index );
#endif

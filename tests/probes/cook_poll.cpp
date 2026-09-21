// Unchanged development publications must not allocate or touch GPU resources.
#include "../../engine/render/tr_image.cpp"
#include <assert.h>

refimport_t ri;
trGlobals_t tr;
glconfig_t glConfig;
static int64_t now;
static uint32_t reads;
static int64_t clockTime() {
	return now;
}
static void *allocate( size_t ) {
	abort();
}
static int readRevision( const char *path, void *buffer, int capacity ) {
	assert( !strcmp( path, "cook.revision" ) && buffer && capacity == 32 );
	memset( buffer, 42, 32 );
	reads++;
	return 32;
}
void R_ReloadEffects( const cookedIndex_t * ) {
	abort();
}
void R_ReloadCookedModels( const cookedIndex_t * ) {
	abort();
}
void R_ReloadCookedMaterials( const cookedIndex_t * ) {
	abort();
}
rhiStatus_t RHI_ReplaceCompressedTexture( rhiTexture_t *, int32_t, int32_t, int32_t, const uint8_t *, uint32_t, rhiFormat_t, rhiAddress_t, const char * ) {
	abort();
}

// Any GPU access during an unchanged publication is a test failure.
rhiCapabilities_t RHI_GetCapabilities( void ) {
	abort();
}
rhiStatus_t RHI_PollTextureUpload( bool * ) {
	abort();
}
rhiStatus_t RHI_AdoptResidentTexture( rhiTexture_t *, rhiTexture_t * ) {
	abort();
}
rhiStatus_t RHI_WaitIdle( void ) {
	abort();
}
rhiTextureResidencyStats_t RHI_GetTextureResidencyStats() {
	abort();
}
rhiStatus_t RHI_PollTextureResidency() {
	abort();
}
rhiStatus_t RHI_TextureResidencyBytes( int32_t, int32_t, int32_t, rhiFormat_t, uint64_t * ) {
	abort();
}
rhiStatus_t RHI_CreateResidentTexture( rhiTexture_t *, int32_t, int32_t, int32_t, rhiFormat_t, rhiAddress_t, const char * ) {
	abort();
}
rhiStatus_t RHI_ShutdownTextureUploads() {
	abort();
}
void RHI_DestroyTexture( rhiTexture_t * ) {
	abort();
}
rhiStatus_t RHI_InitTextureUploads() {
	abort();
}
rhiStatus_t RHI_UploadCompressedTexture( const rhiTexture_t *, int32_t, int32_t, int32_t, const uint8_t *, uint32_t, rhiFormat_t, bool ) {
	abort();
}
rhiStatus_t RHI_QueueTextureUpload( const rhiTexture_t *, int32_t, int32_t, int32_t, const uint8_t *, uint32_t, rhiFormat_t ) {
	abort();
}

int main() {
	ri.Microseconds = clockTime;
	ri.Malloc = allocate;
	ri.FS_ReadDeveloperFile = readRevision;
	static cvar_t enabled;
	reloadAssets = &enabled;
	memset( cookedRevision, 42, sizeof( cookedRevision ) );
	now = 100000;
	R_PollCookedAssets();
	assert( reads == 0 );
	enabled.integer = 1;
	for ( uint32_t frame = 0; frame < 10000; frame++ ) {
		R_PollCookedAssets();
		R_PollCookedAssets();
		assert( reads == frame + 1 );
		now += 100000;
	}
	puts( "PASS: 10000 unchanged publication polls throttle reads and allocate no engine memory" );
}

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

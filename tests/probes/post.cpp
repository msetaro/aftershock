#include "../../engine/render/tr_cooked.h"
#include <cassert>
#include <cstdio>
#include <cstring>

int main( int argc, char **argv ) {
	static_assert( sizeof( cookedPost_t ) == 132 && offsetof( cookedPost_t, exposureEV ) == 96 );
	assert(argc == 2);
	FILE *file = fopen( argv[1], "rb" );
	assert(file);
	uint8_t data[180];
	assert(fread(data, 1, sizeof(data), file) == sizeof(data));
	assert(fgetc(file) == EOF);
	fclose( file );
	cookedPost_t settings;
	assert(R_ReadCookedPost(data, sizeof(data), &settings));
	assert(!strcmp(settings.name, "filmic") && !strcmp(settings.lut, "textures/grade.ktx2"));
	assert(settings.exposureEV == 1 && settings.sharpen == .25f && settings.vignette == .125f);
	assert(settings.grain == 0 && settings.lutStrength == .5f && settings.focusDistance == 256);
	assert(settings.focusRange == 128 && settings.dofRadius == 0 && settings.motionBlur == 0);
	puts( "PASS: native filmic post profile preserves authored settings" );
}

// Exercise the production registry copies without a window or GPU.
#include "../../engine/render/tr_backend.cpp"
#include <assert.h>

trGlobals_t tr;
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}
uint32_t RHI_GetTimings( const rhiTiming_t **timings ) {
	static const rhiTiming_t values[] = { { "main", 1250 }, { "ui", 75 } };
	*timings = values;
	return ARRAY_LEN( values );
}
int main() {
	devImage_t copy;
	assert( !RE_GetDeveloperImage( -1, &copy ) && !RE_GetDeveloperImage( 0, &copy ) );
	image_t image = {};
	char name[] = "test/texture";
	image.imgName = name;
	image.width = 128;
	image.height = 64;
	image.uploadWidth = 64;
	image.uploadHeight = 32;
	tr.images[0] = &image;
	tr.numImages = 1;
	assert( RE_GetDeveloperImage( 0, &copy ) );
	assert( !strcmp( copy.name, name ) && copy.texture == 1 && copy.width == 128 && copy.uploadHeight == 32 );
	assert( !RE_GetDeveloperImage( 1, &copy ) && copy.texture == 0 );
	shader_t shader = {};
	shaderStage_t stage = {};
	strcpy( shader.name, "test/material" );
	shader.numUnfoggedPasses = 2;
	shader.stages[0] = &stage;
	stage.stateBits = 123;
	stage.bundle[2].image[0] = &image;
	tr.shaders[0] = &shader;
	tr.numShaders = 1;
	devMaterial_t material;
	assert( RE_GetDeveloperMaterial( 0, &material ) );
	assert( !strcmp( material.name, shader.name ) && material.stages == 2 );
	assert( material.present[0] && !material.present[1] );
	assert( material.stateBits[0] == 123 && material.textures[0][2] == 1 && material.textures[0][1] == 0 );
	assert( !RE_GetDeveloperMaterial( -1, &material ) && !RE_GetDeveloperMaterial( 1, &material ) );
	devModel_t modelCopy;
	assert( !RE_GetDeveloperModel( -1, &modelCopy ) && !RE_GetDeveloperModel( 0, &modelCopy ) );
	model_t model = {};
	md3Header_t md3 = {};
	strcpy( model.name, "test/model.md3" );
	model.type = MOD_MESH;
	model.md3[0] = &md3;
	model.dataSize = 512;
	md3.numFrames = 24;
	tr.models[0] = &model;
	tr.numModels = 1;
	assert( RE_GetDeveloperModel( 0, &modelCopy ) && modelCopy.frames == 24 && modelCopy.bytes == 512 );
	assert( !strcmp( modelCopy.name, model.name ) && !RE_GetDeveloperModel( 1, &modelCopy ) );
	mdrHeader_t mdr = {};
	mdr.numFrames = 30;
	model.type = MOD_MDR;
	model.modelData = &mdr;
	assert( RE_GetDeveloperModel( 0, &modelCopy ) && modelCopy.frames == 30 );
	iqmData_t iqm = {};
	iqm.num_frames = 12;
	model.type = MOD_IQM;
	model.modelData = &iqm;
	assert( RE_GetDeveloperModel( 0, &modelCopy ) && modelCopy.frames == 12 );
	devGpuTiming_t timing[3];
	assert( RE_GetDeveloperTimings( timing, 0 ) == 0 );
	assert( RE_GetDeveloperTimings( timing, 1 ) == 1 && timing[0].microseconds == 1250 );
	assert( RE_GetDeveloperTimings( timing, 3 ) == 2 && !strcmp( timing[1].name, "ui" ) && timing[1].microseconds == 75 );
}

// Read the ordinary owned cooker output and resolve independent instance values.
#include "../../engine/render/tr_cooked.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main( int argc, char **argv ) {
	assert( argc == 2 );
	FILE *file = fopen( argv[1], "rb" );
	assert( file );
	unsigned char bytes[288];
	assert( fread( bytes, 1, sizeof( bytes ), file ) == sizeof( bytes ) );
	assert( fgetc( file ) == EOF );
	fclose( file );
	cookedPbrMaterial_t material;
	assert( R_ReadPbrMaterial( bytes, sizeof( bytes ), &material ) );
	assert( material.params.color[0] == 0.5f && material.params.color[3] == 0.5f );
	assert( material.params.emissive[0] == 0.25f && material.params.emissive[2] == 1 );
	assert( material.params.metallic == 0.75f && material.params.roughness == 0.5f );
	assert( material.params.normalScale == 0.75f && material.params.alphaCutoff == 0.25f );
	assert( material.params.flags == 9 );
	for ( const auto &path : material.textures )
		assert( !strncmp( path, "models/pbr_material0", 19 ) && strstr( path, ".ktx2" ) );
	const materialParams_t base = material.params;
	materialOverride_t instance = {};
	instance.mask = MATERIAL_OVERRIDE_COLOR | MATERIAL_OVERRIDE_ROUGHNESS;
	instance.values.color[0] = 1;
	instance.values.color[1] = 0.25f;
	instance.values.color[2] = 0.5f;
	instance.values.color[3] = 0.75f;
	instance.values.roughness = 0.125f;
	materialParams_t resolved;
	assert( R_ResolveMaterialParams( &base, &instance, &resolved ) );
	assert( resolved.color[0] == 1 && resolved.color[3] == 0.75f && resolved.roughness == 0.125f );
	assert( resolved.metallic == base.metallic && resolved.flags == base.flags );
	assert( !memcmp( &base, &material.params, sizeof( base ) ) );
	assert( R_ResolveMaterialParams( &base, nullptr, &resolved ) );
	assert( !memcmp( &base, &resolved, sizeof( base ) ) );
	puts( "PASS: native PBR data and independent material instance values" );
}

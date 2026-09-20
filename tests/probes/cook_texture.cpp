// Ordinary cooked assets exercise the runtime records, hashes and mip ordering.
#include "../../engine/render/tr_cooked.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main( int argc, char **argv ) {
	assert( argc == 3 );
	FILE *file = fopen( argv[1], "rb" );
	assert( file && fseek( file, 0, SEEK_END ) == 0 );
	const size_t size = (size_t)ftell( file );
	assert( fseek( file, 0, SEEK_SET ) == 0 );
	void *data = malloc( size );
	assert( data && fread( data, 1, size, file ) == size );
	fclose( file );
	cookedTexture_t texture;
	assert( R_ReadCookedTexture( data, size, &texture ) );
	assert( texture.width == 16 && texture.height == 16 && texture.mipLevels == 5 );
	assert( texture.format == rhiFormat_t::BC7_SRGB );
	const uint32_t sizes[] = { 256, 64, 16, 16, 16 };
	assert( texture.size == 368 );
	for ( uint32_t i = 0; i < 5; i++ ) {
		assert( texture.levels[i].size == sizes[i] );
		assert( i == 0 || texture.levels[i].data < texture.levels[i - 1].data );
	}
	free( data );
	file = fopen( argv[2], "rb" );
	assert( file );
	uint8_t materialBytes[136];
	assert( fread( materialBytes, 1, sizeof( materialBytes ), file ) == sizeof( materialBytes ) );
	fclose( file );
	cookedMaterial_t material;
	assert( R_ReadCookedMaterial( materialBytes, sizeof( materialBytes ), &material ) );
	assert( strcmp( material.texture, "models/character_material0.ktx2" ) == 0 );
	assert( material.alphaCutoff == 0.5f && material.flags == 0 );
	for ( uint32_t i = 0; i < 4; i++ )
		assert( material.color[i] == 1 );
	puts( "PASS: native cooked KTX2 records, SHA-256 and mip views" );
}

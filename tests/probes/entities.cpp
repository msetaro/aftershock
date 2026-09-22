#include "../../engine/entities/entities_public.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main( int argc, char **argv ) {
	assert( argc == 3 );
	FILE *file = fopen( argv[1], "rb" );
	assert( file );
	static unsigned char bytes[524288];
	const size_t size = fread( bytes, 1, sizeof( bytes ), file );
	assert( feof( file ) );
	fclose( file );
	static entityDefinitions_t definitions;
	assert( Entity_ReadDefinitions( bytes, size, &definitions ) );
	const auto *base = Entity_FindDefinition( definitions, "health_base" );
	const auto *crate = Entity_FindDefinition( definitions, "medical_crate" );
	const auto *small = Entity_FindDefinition( definitions, "medical_crate_small" );
	assert( base && crate && small );
	assert( !strcmp( crate->native, "item_health_large" ) );
	assert( !strcmp( Entity_Field( definitions, *base, "count" )->value, "50" ) );
	assert( !strcmp( Entity_Field( definitions, *crate, "count" )->value, "75" ) );
	assert( !strcmp( Entity_Field( definitions, *small, "count" )->value, "15" ) );
	const auto *origin = Entity_Field( definitions, *small, "origin" );
	assert( origin && !strcmp( origin->component, "transform" ) && !strcmp( origin->value, "16 32 48" ) );
	assert( crate->priority == 2 && small->priority == 2 && small->radius == 0 );
	assert( !Entity_FindDefinition( definitions, "missing" ) && !Entity_Field( definitions, *base, "origin" ) );
	file = fopen( argv[2], "rb" );
	assert( file );
	const size_t composedSize = fread( bytes, 1, sizeof( bytes ), file );
	assert( feof( file ) );
	fclose( file );
	assert( Entity_ReadDefinitions( bytes, composedSize, &definitions ) );
	const auto *composed = Entity_FindDefinition( definitions, "signal_crate" );
	assert( composed && composed->components == 1021 && composed->priority == 2 && composed->radius == 2048 );
	const char *properties[][2] = {
		{ "model", "models/character.iqm" }, { "anim_frames", "31" }, { "mins", "-16 -16 -16" },
		{ "solid", "1" }, { "trigger_wait", "250" }, { "dmg", "7" },
		{ "noise", "sound/entities/hum.wav" }, { "audio_loop", "1" }
	};
	for ( const auto &property : properties ) {
		const auto *field = Entity_Field( definitions, *composed, property[0] );
		assert( field && !strcmp( field->value, property[1] ) );
	}
	puts( "PASS: native prefab lookup, inherited pickup/transform and replication metadata" );
}

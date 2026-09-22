#include "../../engine/entities/entities_public.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void CheckState( const entityDefinitions_t &definitions ) {
	static unsigned char archive[1048576], cooked[524288];
	static entityDefinitions_t restored;
	stateWriter_t writer{ archive, sizeof( archive ) };
	assert( State_Append( &writer, entityDefinitionHeaderSchema, 0, &definitions.header ) );
	for ( uint32_t i = 0; i < definitions.header.count; ++i )
		assert( State_Append( &writer, entityDefinitionSchema, i, &definitions.definitions[i] ) );
	for ( uint32_t i = 0; i < definitions.header.fieldCount; ++i )
		assert( State_Append( &writer, entityDefinitionFieldSchema, i, &definitions.fields[i] ) );
	const size_t size = State_Finish( &writer );
	stateReader_t reader;
	assert( size && State_Open( archive, size, &reader ) );
	uint32_t version;
	assert( State_Find( reader, entityDefinitionHeaderSchema, 0, &restored.header, &version ) && version == 1 );
	assert( restored.header.count == definitions.header.count && restored.header.fieldCount == definitions.header.fieldCount );
	for ( uint32_t i = 0; i < restored.header.count; ++i )
		assert( State_Find( reader, entityDefinitionSchema, i, &restored.definitions[i], &version ) && version == 1 );
	for ( uint32_t i = 0; i < restored.header.fieldCount; ++i )
		assert( State_Find( reader, entityDefinitionFieldSchema, i, &restored.fields[i], &version ) && version == 1 );
	assert( !memcmp( &definitions.header, &restored.header, sizeof( definitions.header ) ) );
	assert( !memcmp( definitions.definitions, restored.definitions, definitions.header.count * sizeof( entityDefinition_t ) ) );
	assert( !memcmp( definitions.fields, restored.fields, definitions.header.fieldCount * sizeof( entityDefinitionField_t ) ) );
	const size_t cookedSize = Entity_WriteDefinitions( restored, cooked, sizeof( cooked ) );
	assert( cookedSize && Entity_ReadDefinitions( cooked, cookedSize, &restored ) );
}

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
	assert( !Entity_SetField( &definitions, "medical_crate", "count", "-1" ) );
	assert( Entity_SetField( &definitions, "medical_crate", "count", "92" ) );
	assert( Entity_SetField( &definitions, "medical_crate", "rep_priority", "3" ) );
	assert( Entity_SetField( &definitions, "medical_crate", "rep_radius", "128.5" ) );
	assert( crate->priority == 3 && crate->radius == 128.5f );
	assert( !strcmp( Entity_Field( definitions, *base, "count" )->value, "50" ) );
	static unsigned char saved[sizeof( bytes )];
	const size_t savedSize = Entity_WriteDefinitions( definitions, saved, sizeof( saved ) );
	assert( savedSize == size && Entity_ReadDefinitions( saved, savedSize, &definitions ) );
	crate = Entity_FindDefinition( definitions, "medical_crate" );
	assert( crate && !strcmp( Entity_Field( definitions, *crate, "count" )->value, "92" ) );
	assert( crate->priority == 3 && crate->radius == 128.5f );
	CheckState( definitions );
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
	CheckState( definitions );
	puts( "PASS: named state records preserve edited prefab and component definitions" );
	puts( "PASS: native prefab lookup, inherited pickup/transform and replication metadata" );
}

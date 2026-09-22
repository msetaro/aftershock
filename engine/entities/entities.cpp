#include "entities_public.h"
#include "../../third_party/sha256/sha-256.h"
#include <charconv>
#include <cmath>
#include <string.h>

static bool Identifier( const char *value, size_t capacity, bool uppercase = false ) {
	const char *end = (const char *)memchr( value, 0, capacity );
	if ( !end || end == value )
		return false;
	for ( const char *p = value; p < end; ++p )
		if ( !( *p >= 'a' && *p <= 'z' ) && !( *p >= '0' && *p <= '9' ) && *p != '_' && !( uppercase && *p >= 'A' && *p <= 'Z' ) )
			return false;
	return true;
}
static bool ValidField( const entityDefinitionField_t &field, uint32_t mask ) {
	if ( !Identifier( field.component, sizeof( field.component ) ) || !Identifier( field.key, sizeof( field.key ) ) || !memchr( field.value, 0, sizeof( field.value ) ) )
		return false;
	const char *cursor = field.value, *end = cursor + strlen( cursor );
	if ( !strcmp( field.component, "hooks" ) )
		return ( mask & 4 ) && ( !strcmp( field.key, "target" ) || !strcmp( field.key, "targetname" ) ) && Identifier( field.value, 64 );
	if ( !strcmp( field.component, "pickup" ) ) {
		int quantity;
		const auto result = std::from_chars( cursor, end, quantity );
		return ( mask & 2 ) && !strcmp( field.key, "count" ) && result.ec == std::errc() && result.ptr == end && quantity >= 1 && quantity <= 10000;
	}
	if ( strcmp( field.component, "transform" ) || !( mask & 1 ) || ( strcmp( field.key, "origin" ) && strcmp( field.key, "angles" ) ) )
		return false;
	const float bound = !strcmp( field.key, "origin" ) ? 32000.0f : 360.0f;
	for ( int i = 0; i < 3; ++i ) {
		float value;
		const auto result = std::from_chars( cursor, end, value, std::chars_format::fixed );
		if ( result.ec != std::errc() || !std::isfinite( value ) || value < -bound || value > bound )
			return false;
		cursor = result.ptr;
		if ( i != 2 ) {
			if ( cursor == end || *cursor != ' ' )
				return false;
			++cursor;
		}
	}
	return cursor == end;
}
bool Entity_ReadDefinitions( const void *data, size_t size, entityDefinitions_t *out ) {
	struct envelope_t {
		char magic[8];
		uint32_t version, size;
		uint8_t hash[32];
	} envelope;
	static_assert( sizeof( envelope ) == 48 );
	if ( !data || !out || size < sizeof( envelope ) + sizeof( entityDefinitionHeader_t ) || size > sizeof( envelope ) + sizeof( entityDefinitions_t ) )
		return false;
	memcpy( &envelope, data, sizeof( envelope ) );
	if ( memcmp( envelope.magic, "ASENT\0\0\0", 8 ) || envelope.version != 1 || envelope.size != size - sizeof( envelope ) )
		return false;
	const auto *payload = (const uint8_t *)data + sizeof( envelope );
	uint8_t hash[32];
	calc_sha_256( hash, payload, envelope.size );
	if ( memcmp( hash, envelope.hash, sizeof( hash ) ) )
		return false;
	entityDefinitionHeader_t header;
	memcpy( &header, payload, sizeof( header ) );
	if ( !Identifier( header.name, sizeof( header.name ) ) || !header.count || header.count > 256 || header.fieldCount > 2048 ||
		 envelope.size != sizeof( header ) + header.count * sizeof( entityDefinition_t ) + header.fieldCount * sizeof( entityDefinitionField_t ) )
		return false;
	const auto *records = payload + sizeof( header );
	const auto *fields = records + header.count * sizeof( entityDefinition_t );
	uint32_t first = 0;
	for ( uint32_t i = 0; i < header.count; ++i ) {
		entityDefinition_t definition;
		memcpy( &definition, records + i * sizeof( definition ), sizeof( definition ) );
		if ( !Identifier( definition.name, sizeof( definition.name ) ) || !Identifier( definition.native, sizeof( definition.native ), true ) ||
			 definition.firstField != first || definition.fieldCount > 32 || definition.fieldCount > header.fieldCount - first ||
			 definition.components > 15 || definition.priority > 3 || !std::isfinite( definition.radius ) || definition.radius < 0 || definition.radius > 32768 )
			return false;
		for ( uint32_t j = 0; j < i; ++j ) {
			entityDefinition_t previous;
			memcpy( &previous, records + j * sizeof( previous ), sizeof( previous ) );
			if ( !strcmp( definition.name, previous.name ) )
				return false;
		}
		for ( uint32_t j = first; j < first + definition.fieldCount; ++j ) {
			entityDefinitionField_t field;
			memcpy( &field, fields + j * sizeof( field ), sizeof( field ) );
			if ( !ValidField( field, definition.components ) )
				return false;
			for ( uint32_t k = first; k < j; ++k ) {
				entityDefinitionField_t previous;
				memcpy( &previous, fields + k * sizeof( previous ), sizeof( previous ) );
				if ( !strcmp( field.key, previous.key ) )
					return false;
			}
		}
		first += definition.fieldCount;
	}
	if ( first != header.fieldCount )
		return false;
	out->header = header;
	memcpy( out->definitions, records, header.count * sizeof( entityDefinition_t ) );
	memcpy( out->fields, fields, header.fieldCount * sizeof( entityDefinitionField_t ) );
	return true;
}
const entityDefinition_t *Entity_FindDefinition( const entityDefinitions_t &definitions, const char *name ) {
	for ( uint32_t i = 0; i < definitions.header.count; ++i )
		if ( !strcmp( definitions.definitions[i].name, name ) )
			return &definitions.definitions[i];
	return nullptr;
}
const entityDefinitionField_t *Entity_Field( const entityDefinitions_t &definitions, const entityDefinition_t &definition, const char *key ) {
	for ( uint32_t i = definition.firstField; i < definition.firstField + definition.fieldCount; ++i )
		if ( !strcmp( definitions.fields[i].key, key ) )
			return &definitions.fields[i];
	return nullptr;
}

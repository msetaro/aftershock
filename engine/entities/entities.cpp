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
	struct rule_t {
		const char *component, *key;
		uint32_t mask;
		int count; // 0 identifier, -1 resource path, 1 integer, 2 scalar float, 3 vector
		float low, high;
	};
	static const rule_t rules[] = {
		{ "transform", "origin", 1, 3, -32000, 32000 }, { "transform", "angles", 1, 3, -360, 360 },
		{ "pickup", "count", 2, 1, 1, 10000 },
		{ "hooks", "target", 4, 0, 0, 0 }, { "hooks", "targetname", 4, 0, 0, 0 },
		{ "model", "model", 16, -1, 0, 0 },
		{ "animation", "anim_first", 32, 1, 0, 4095 }, { "animation", "anim_frames", 32, 1, 1, 4096 },
		{ "animation", "anim_ms", 32, 1, 10, 10000 }, { "animation", "anim_loop", 32, 1, 0, 1 },
		{ "collision", "mins", 64, 3, -1024, 1024 }, { "collision", "maxs", 64, 3, -1024, 1024 }, { "collision", "solid", 64, 1, 0, 1 },
		{ "trigger", "trigger_wait", 128, 1, 0, 60000 }, { "trigger", "trigger_once", 128, 1, 0, 1 },
		{ "damage", "dmg", 256, 1, 0, 10000 }, { "damage", "health", 256, 1, 0, 10000 },
		{ "damage", "splash_damage", 256, 1, 0, 10000 }, { "damage", "splash_radius", 256, 2, 0, 4096 },
		{ "audio", "noise", 512, -1, 0, 0 }, { "audio", "audio_loop", 512, 1, 0, 1 }
	};
	const rule_t *rule = nullptr;
	for ( const auto &candidate : rules )
		if ( !strcmp( field.component, candidate.component ) && !strcmp( field.key, candidate.key ) )
			rule = &candidate;
	if ( !rule || !( mask & rule->mask ) )
		return false;
	const char *cursor = field.value, *end = cursor + strlen( cursor );
	if ( rule->count == 0 )
		return Identifier( field.value, 64 );
	if ( rule->count < 0 ) {
		if ( end == cursor || end - cursor >= 64 || *cursor == '/' || strstr( cursor, ".." ) )
			return false;
		for ( const char *p = cursor; p < end; ++p )
			if ( !( *p >= 'a' && *p <= 'z' ) && !( *p >= '0' && *p <= '9' ) && !strchr( "_./-", *p ) )
				return false;
		return true;
	}
	if ( rule->count == 1 ) {
		int value;
		const auto result = std::from_chars( cursor, end, value );
		return result.ec == std::errc() && result.ptr == end && float( value ) >= rule->low && float( value ) <= rule->high;
	}
	const int count = rule->count == 3 ? 3 : 1;
	for ( int i = 0; i < count; ++i ) {
		float value;
		const auto result = std::from_chars( cursor, end, value, std::chars_format::fixed );
		if ( result.ec != std::errc() || !std::isfinite( value ) || value < rule->low || value > rule->high )
			return false;
		cursor = result.ptr;
		if ( i != count - 1 ) {
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
			 definition.components > 1023 || definition.priority > 3 || !std::isfinite( definition.radius ) || definition.radius < 0 || definition.radius > 32768 )
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

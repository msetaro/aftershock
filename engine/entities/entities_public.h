#ifndef ENTITIES_PUBLIC_H
#define ENTITIES_PUBLIC_H

#include "../public/state_public.h"
#include <stddef.h>
#include <stdint.h>
#include <type_traits>

struct entityDefinitionHeader_t {
	char name[32];
	uint32_t count, fieldCount;
};
struct entityDefinition_t {
	char name[64], native[64];
	uint32_t firstField, fieldCount, components, priority;
	float radius;
};
struct entityDefinitionField_t {
	char component[16], key[32], value[128];
};
struct entityDefinitions_t {
	entityDefinitionHeader_t header;
	entityDefinition_t definitions[256];
	entityDefinitionField_t fields[2048];
};
static_assert( sizeof( entityDefinitionHeader_t ) == 40 );
static_assert( sizeof( entityDefinition_t ) == 148 && offsetof( entityDefinition_t, firstField ) == 128 );
static_assert( sizeof( entityDefinitionField_t ) == 176 && offsetof( entityDefinitionField_t, value ) == 48 );
static_assert( std::is_trivially_copyable_v<entityDefinitions_t> );
size_t Entity_WriteDefinitions( const entityDefinitions_t &definitions, void *data, size_t capacity );
bool Entity_SetField( entityDefinitions_t *definitions, const char *name, const char *key, const char *value );
bool Entity_ReadDefinitions( const void *data, size_t size, entityDefinitions_t *out );
const entityDefinition_t *Entity_FindDefinition( const entityDefinitions_t &definitions, const char *name );
const entityDefinitionField_t *Entity_Field( const entityDefinitions_t &definitions, const entityDefinition_t &definition, const char *key );

// Checkpoints use the same bounded definition records as the cooker and inspector.
// The accepted ASENT payload remains unchanged.
inline constexpr stateField_t entityDefinitionHeaderFields[] = {
	{ "name", offsetof( entityDefinitionHeader_t, name ), 32, stateType_t::String },
	{ "count", offsetof( entityDefinitionHeader_t, count ), 1, stateType_t::UInt32 },
	{ "fieldCount", offsetof( entityDefinitionHeader_t, fieldCount ), 1, stateType_t::UInt32 },
};
inline constexpr stateSchema_t entityDefinitionHeaderSchema = { "entityDefinitionHeader", 1, 1, sizeof( entityDefinitionHeader_t ), entityDefinitionHeaderFields, 3 };
inline constexpr stateField_t entityDefinitionFields[] = {
	{ "name", offsetof( entityDefinition_t, name ), 64, stateType_t::String },
	{ "native", offsetof( entityDefinition_t, native ), 64, stateType_t::String },
	{ "firstField", offsetof( entityDefinition_t, firstField ), 1, stateType_t::UInt32 },
	{ "fieldCount", offsetof( entityDefinition_t, fieldCount ), 1, stateType_t::UInt32 },
	{ "components", offsetof( entityDefinition_t, components ), 1, stateType_t::UInt32 },
	{ "priority", offsetof( entityDefinition_t, priority ), 1, stateType_t::UInt32 },
	{ "radius", offsetof( entityDefinition_t, radius ), 1, stateType_t::Float32 },
};
inline constexpr stateSchema_t entityDefinitionSchema = { "entityDefinition", 1, 1, sizeof( entityDefinition_t ), entityDefinitionFields, 7 };
inline constexpr stateField_t entityDefinitionFieldFields[] = {
	{ "component", offsetof( entityDefinitionField_t, component ), 16, stateType_t::String },
	{ "key", offsetof( entityDefinitionField_t, key ), 32, stateType_t::String },
	{ "value", offsetof( entityDefinitionField_t, value ), 128, stateType_t::String },
};
inline constexpr stateSchema_t entityDefinitionFieldSchema = { "entityDefinitionField", 1, 1, sizeof( entityDefinitionField_t ), entityDefinitionFieldFields, 3 };

#endif

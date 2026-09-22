#include "../../engine/public/state_public.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

struct previous_t {
	int32_t health, removedItem;
	float position[3];
	char name[32];
};
struct current_t {
	char name[32];
	float position[3];
	int32_t armor, health;
};
static constexpr stateField_t previousFields[] = {
	{ "health", offsetof( previous_t, health ), 1, stateType_t::Int32 },
	{ "removedItem", offsetof( previous_t, removedItem ), 1, stateType_t::Int32 },
	{ "position", offsetof( previous_t, position ), 3, stateType_t::Float32 },
	{ "name", offsetof( previous_t, name ), 32, stateType_t::Bytes }
};
static constexpr stateField_t currentFields[] = {
	{ "name", offsetof( current_t, name ), 32, stateType_t::Bytes },
	{ "position", offsetof( current_t, position ), 3, stateType_t::Float32 },
	{ "armor", offsetof( current_t, armor ), 1, stateType_t::Int32 },
	{ "health", offsetof( current_t, health ), 1, stateType_t::Int32 }
};
static constexpr stateSchema_t previousSchema = { "example", 1, 1, sizeof( previous_t ), previousFields, 4 };
static constexpr stateSchema_t currentSchema = { "example", 2, 1, sizeof( current_t ), currentFields, 4 };

int main() {
	const previous_t previous = { 73, 9, { 16.25f, -32.5f, 48.0f }, "checkpoint" };
	unsigned char bytes[4096];
	const size_t size = State_Write( previousSchema, &previous, bytes, sizeof( bytes ) );
	assert( size > 0 );
	previous_t same = {};
	uint32_t version = 0;
	assert( State_Read( previousSchema, bytes, size, &same, &version ) && version == 1 );
	assert( same.health == previous.health && same.removedItem == previous.removedItem );
	assert( !memcmp( same.position, previous.position, sizeof( same.position ) ) && !strcmp( same.name, previous.name ) );
	current_t current = {};
	assert( State_Read( currentSchema, bytes, size, &current, &version ) && version == 1 );
	// Explicit migration: v1 had no armor field; the new policy starts at 25.
	if ( version == 1 )
		current.armor = 25;
	assert( current.health == 73 && current.armor == 25 );
	assert( !memcmp( current.position, previous.position, sizeof( current.position ) ) && !strcmp( current.name, previous.name ) );
	const size_t currentSize = State_Write( currentSchema, &current, bytes, sizeof( bytes ) );
	assert( currentSize > 0 );
	current_t restored = {};
	assert( State_Read( currentSchema, bytes, currentSize, &restored, &version ) && version == 2 );
	assert( restored.health == 73 && restored.armor == 25 );
	assert( !memcmp( restored.position, previous.position, sizeof( restored.position ) ) && !strcmp( restored.name, previous.name ) );
	puts( "PASS: named POD fields preserve bits across reordering and explicit added/removed-field migration" );
}

#ifdef STATE_NATIVE_GAME
#include "../../game/bg/q_shared.h"
#else
#include "../../engine/qcommon/q_shared.h"
#endif
#include "../../engine/public/state_public.h"
#include "../../engine/public/state_replication.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

struct previous_t {
	int32_t health, removedItem;
	uint64_t identity;
	float position[3];
	char name[32];
};
struct current_t {
	char name[32];
	float position[3];
	int32_t armor, health;
	uint64_t identity;
};
static constexpr stateField_t previousFields[] = {
	{ "health", offsetof( previous_t, health ), 1, stateType_t::Int32 },
	{ "identity", offsetof( previous_t, identity ), 1, stateType_t::UInt64 },
	{ "removedItem", offsetof( previous_t, removedItem ), 1, stateType_t::Int32 },
	{ "position", offsetof( previous_t, position ), 3, stateType_t::Float32 },
	{ "name", offsetof( previous_t, name ), 32, stateType_t::Bytes }
};
static constexpr stateField_t currentFields[] = {
	{ "name", offsetof( current_t, name ), 32, stateType_t::Bytes },
	{ "identity", offsetof( current_t, identity ), 1, stateType_t::UInt64 },
	{ "position", offsetof( current_t, position ), 3, stateType_t::Float32 },
	{ "armor", offsetof( current_t, armor ), 1, stateType_t::Int32, 2 },
	{ "health", offsetof( current_t, health ), 1, stateType_t::Int32 }
};
static constexpr stateSchema_t previousSchema = { "example", 1, 1, sizeof( previous_t ), previousFields, 5 };
static constexpr stateSchema_t currentSchema = { "example", 2, 1, sizeof( current_t ), currentFields, 5 };

template <typename T>
static void CheckReplicationState( const stateSchema_t &schema ) {
	T source = {}, restored = {};
	// Exercise every scalar/array, including fields omitted by the wire codec.
	for ( size_t offset = 0; offset < sizeof( source ); offset += sizeof( uint32_t ) ) {
		const uint32_t bits = 0x3e800000U + uint32_t( offset );
		memcpy( (unsigned char *)&source + offset, &bits, sizeof( bits ) );
	}
	unsigned char bytes[32768];
	const size_t size = State_Write( schema, &source, bytes, sizeof( bytes ) );
	assert( size );
	uint32_t version;
	assert( State_Read( schema, bytes, size, &restored, &version ) && version == 1 );
	assert( !memcmp( &source, &restored, sizeof( source ) ) );
}

int main() {
	CheckReplicationState<entityState_t>( entityStateSchema );
	CheckReplicationState<playerState_t>( playerStateSchema );
	const previous_t previous = { 73, 9, UINT64_C( 0xfedcba9876543210 ), { 16.25f, -32.5f, 48.0f }, "checkpoint" };
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
	assert( current.health == 73 && current.armor == 25 && current.identity == previous.identity );
	assert( !memcmp( current.position, previous.position, sizeof( current.position ) ) && !strcmp( current.name, previous.name ) );
	const size_t currentSize = State_Write( currentSchema, &current, bytes, sizeof( bytes ) );
	assert( currentSize > 0 );
	current_t restored = {};
	assert( State_Read( currentSchema, bytes, currentSize, &restored, &version ) && version == 2 );
	assert( restored.health == 73 && restored.armor == 25 && restored.identity == previous.identity );
	assert( !memcmp( restored.position, previous.position, sizeof( restored.position ) ) && !strcmp( restored.name, previous.name ) );
	// A checkpoint contains many independently versioned records, keyed by slot.
	unsigned char archive[65536];
	stateWriter_t writer = { archive, sizeof( archive ) };
	assert( State_Append( &writer, previousSchema, 7, &previous ) );
	assert( State_Append( &writer, previousSchema, 19, &previous ) );
	playerState_t player = {};
	player.stats[0] = 81;
	player.ping = 27; // Local-only data is part of a checkpoint.
	assert( State_Append( &writer, playerStateSchema, 0, &player ) );
	const size_t archiveSize = State_Finish( &writer );
	assert( archiveSize );
	stateReader_t reader;
	assert( State_Open( archive, archiveSize, &reader ) );
	playerState_t savedPlayer = {};
	assert( State_Find( reader, playerStateSchema, 0, &savedPlayer, &version ) );
	assert( savedPlayer.stats[0] == 81 && savedPlayer.ping == 27 );
	current_t savedCurrent = {};
	assert( State_Find( reader, currentSchema, 19, &savedCurrent, &version ) && version == 1 );
	if ( version == 1 )
		savedCurrent.armor = 25;
	assert( savedCurrent.health == 73 && savedCurrent.armor == 25 && savedCurrent.identity == previous.identity );
	assert( !State_Find( reader, currentSchema, 20, &savedCurrent, &version ) );
	assert( !State_Append( &writer, previousSchema, 7, &previous ) && !State_Finish( &writer ) );
	archive[archiveSize - 1] ^= 1;
	assert( !State_Open( archive, archiveSize, &reader ) );
	stateWriter_t shortWriter = { archive, 64 };
	assert( !State_Append( &shortWriter, previousSchema, 0, &previous ) && !State_Finish( &shortWriter ) );
	puts( "PASS: named POD fields preserve bits across reordering and explicit added/removed-field migration" );
}

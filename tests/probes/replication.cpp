// Existing wire field order and bytes, captured before #12.
#include "../../engine/qcommon/msg.cpp"
#include <assert.h>

cvar_t *cl_shownet;
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}
void QDECL Com_Printf( const char *, ... ) {
}
void QDECL Com_DPrintf( const char *, ... ) {
}

static uint32_t seed = 42;
static uint32_t Random() {
	seed = seed * 1664525u + 1013904223u;
	return seed;
}
static void Fields( void *state, const netField_t *fields, int count, int round ) {
	for ( int i = 0; i < count; ++i ) {
		if ( round < count ? i != round : ( Random() & 3u ) == 0 )
			continue;
		byte *destination = (byte *)state + fields[i].offset;
		const int bits = fields[i].bits;
		if ( !bits ) {
			const float value = float( int( Random() % 200000u ) - 100000 ) * ( round & 1 ? 0.125f : 1.0f );
			memcpy( destination, &value, 4 );
		} else {
			const int width = bits < 0 ? -bits : bits;
			const uint32_t mask = width == 32 ? UINT32_MAX : ( 1u << width ) - 1;
			uint32_t value = Random() & mask;
			if ( bits < 0 && ( value & ( 1u << ( width - 1 ) ) ) )
				value |= ~mask;
			memcpy( destination, &value, 4 );
		}
	}
}
static void Packet( const msg_t &msg ) {
	assert( !msg.overflowed );
	// Explicit byte order makes the reference independent of host integer output.
	const byte length[4] = { byte( msg.bit ), byte( msg.bit >> 8 ), byte( msg.bit >> 16 ), byte( msg.bit >> 24 ) };
	assert( fwrite( length, 1, 4, stdout ) == 4 );
	assert( fwrite( msg.data, 1, (size_t)msg.cursize, stdout ) == (size_t)msg.cursize );
}
int main() {
	for ( const auto &field : entityStateFields )
		printf( "entity %s %d %d\n", field.name, field.offset, field.bits );
	for ( const auto &field : playerStateFields )
		printf( "player %s %d %d\n", field.name, field.offset, field.bits );
	entityState_t oldEntity = {}, entity = {}, decodedEntity = {};
	playerState_t oldPlayer = {}, player = {}, decodedPlayer = {};
	byte buffer[4096];
	msg_t msg;
	MSG_Init( &msg, buffer, sizeof( buffer ) );
	for ( int round = 0; round < 256; ++round ) {
		entity.number = 1;
		Fields( &entity, entityStateFields, ARRAY_LEN( entityStateFields ), round );
		Fields( &player, playerStateFields, ARRAY_LEN( playerStateFields ), round );
		for ( int i = 0; i < 16; ++i ) {
			player.stats[i] = int32_t( Random() % 65536u ) - 32768;
			player.persistant[i] = int32_t( Random() % 65536u ) - 32768;
			player.ammo[i] = int32_t( Random() % 65536u ) - 32768;
			player.powerups[i] = int32_t( Random() & 0x7fffffffu );
		}
		memset( buffer, 0, sizeof( buffer ) );
		MSG_Clear( &msg );
		MSG_WriteDeltaEntity( &msg, &oldEntity, &entity, qtrue );
		MSG_WriteDeltaPlayerstate( &msg, &oldPlayer, &player );
		Packet( msg );
		MSG_BeginReading( &msg );
		MSG_ReadDeltaEntity( &msg, &oldEntity, &decodedEntity, MSG_ReadEntitynum( &msg ) );
		MSG_ReadDeltaPlayerstate( &msg, &oldPlayer, &decodedPlayer );
		assert( memcmp( &entity, &decodedEntity, sizeof( entity ) ) == 0 );
		assert( memcmp( &player, &decodedPlayer, sizeof( player ) ) == 0 );
		oldEntity = entity;
		oldPlayer = player;
	}
	memset( buffer, 0, sizeof( buffer ) );
	MSG_Clear( &msg );
	MSG_WriteDeltaEntity( &msg, &entity, &entity, qfalse );
	assert( msg.cursize == 0 );
	MSG_WriteDeltaEntity( &msg, &entity, nullptr, qtrue );
	Packet( msg );
	MSG_BeginReading( &msg );
	MSG_ReadDeltaEntity( &msg, &entity, &decodedEntity, MSG_ReadEntitynum( &msg ) );
	assert( decodedEntity.number == MAX_GENTITIES - 1 );
}

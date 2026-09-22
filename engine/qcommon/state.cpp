#include "../public/state_public.h"
#include "../../third_party/sha256/sha-256.h"
#include <bit>
#include <string.h>

static_assert( std::endian::native == std::endian::little );
struct stateEnvelope_t {
	char magic[8];
	uint32_t version, size;
	uint8_t hash[32];
};
struct stateHeader_t {
	char name[32];
	uint32_t version, fieldCount;
};
struct stateRecord_t {
	char name[64];
	stateType_t type;
	uint32_t size;
};
static_assert( sizeof( stateEnvelope_t ) == 48 && offsetof( stateEnvelope_t, hash ) == 16 );
static_assert( sizeof( stateHeader_t ) == 40 && offsetof( stateHeader_t, version ) == 32 );
static_assert( sizeof( stateRecord_t ) == 72 && offsetof( stateRecord_t, size ) == 68 );

static bool Name( const char *text, size_t capacity ) {
	if ( !text )
		return false;
	for ( size_t i = 0; i < capacity; ++i ) {
		const char c = text[i];
		if ( !c )
			return i != 0;
		if ( !( c >= 'a' && c <= 'z' ) && !( c >= 'A' && c <= 'Z' ) && !( c >= '0' && c <= '9' ) && !strchr( "_.[]-", c ) )
			return false;
	}
	return false;
}
static uint32_t Width( stateType_t type ) {
	switch ( type ) {
	case stateType_t::UInt64:
		return 8;
	case stateType_t::Bytes:
		return 1;
	case stateType_t::Int32:
	case stateType_t::UInt32:
	case stateType_t::Float32:
		return 4;
	}
	return 0;
}
static bool Schema( const stateSchema_t &schema ) {
	if ( !Name( schema.name, 32 ) || !schema.minimumVersion || schema.minimumVersion > schema.version ||
		 !schema.objectSize || !schema.fields || !schema.fieldCount || schema.fieldCount > 256 )
		return false;
	for ( uint32_t i = 0; i < schema.fieldCount; ++i ) {
		const auto &field = schema.fields[i];
		const uint32_t width = Width( field.type );
		if ( !Name( field.name, 64 ) || !width || !field.count || !field.sinceVersion || field.sinceVersion > schema.version ||
			 field.offset > schema.objectSize || field.count > ( schema.objectSize - field.offset ) / width )
			return false;
		for ( uint32_t j = 0; j < i; ++j ) {
			const auto &previous = schema.fields[j];
			if ( !strcmp( previous.name, field.name ) ||
				 ( field.offset < previous.offset + previous.count * Width( previous.type ) && previous.offset < field.offset + field.count * width ) )
				return false;
		}
	}
	return true;
}
size_t State_Write( const stateSchema_t &schema, const void *object, void *data, size_t capacity ) {
	if ( !object || !data || !Schema( schema ) )
		return 0;
	size_t payloadSize = sizeof( stateHeader_t );
	for ( uint32_t i = 0; i < schema.fieldCount; ++i )
		payloadSize += sizeof( stateRecord_t ) + schema.fields[i].count * Width( schema.fields[i].type );
	if ( payloadSize > UINT32_MAX || capacity < sizeof( stateEnvelope_t ) + payloadSize )
		return 0;
	stateEnvelope_t envelope = { "ASSTATE", 1, uint32_t( payloadSize ), {} };
	stateHeader_t header = { {}, schema.version, schema.fieldCount };
	memcpy( header.name, schema.name, strlen( schema.name ) );
	auto *payload = (uint8_t *)data + sizeof( envelope );
	memcpy( payload, &header, sizeof( header ) );
	auto *cursor = payload + sizeof( header );
	for ( uint32_t i = 0; i < schema.fieldCount; ++i ) {
		const auto &field = schema.fields[i];
		stateRecord_t record = { {}, field.type, field.count * Width( field.type ) };
		memcpy( record.name, field.name, strlen( field.name ) );
		memcpy( cursor, &record, sizeof( record ) );
		cursor += sizeof( record );
		memcpy( cursor, (const uint8_t *)object + field.offset, record.size );
		cursor += record.size;
	}
	calc_sha_256( envelope.hash, payload, payloadSize );
	memcpy( data, &envelope, sizeof( envelope ) );
	return sizeof( envelope ) + payloadSize;
}
bool State_Read( const stateSchema_t &schema, const void *data, size_t size, void *object, uint32_t *sourceVersion ) {
	if ( !data || !object || !sourceVersion || !Schema( schema ) || size < sizeof( stateEnvelope_t ) + sizeof( stateHeader_t ) )
		return false;
	stateEnvelope_t envelope;
	memcpy( &envelope, data, sizeof( envelope ) );
	if ( memcmp( envelope.magic, "ASSTATE", 8 ) || envelope.version != 1 || envelope.size != size - sizeof( envelope ) )
		return false;
	const auto *payload = (const uint8_t *)data + sizeof( envelope );
	uint8_t hash[32];
	calc_sha_256( hash, payload, envelope.size );
	if ( memcmp( hash, envelope.hash, sizeof( hash ) ) )
		return false;
	stateHeader_t header;
	memcpy( &header, payload, sizeof( header ) );
	if ( !Name( header.name, sizeof( header.name ) ) || strcmp( header.name, schema.name ) ||
		 header.version < schema.minimumVersion || header.version > schema.version || !header.fieldCount || header.fieldCount > 256 )
		return false;
	const uint8_t *values[256] = {};
	const char *names[256] = {};
	const auto *cursor = payload + sizeof( header ), *end = payload + envelope.size;
	for ( uint32_t i = 0; i < header.fieldCount; ++i ) {
		if ( size_t( end - cursor ) < sizeof( stateRecord_t ) )
			return false;
		stateRecord_t record;
		memcpy( &record, cursor, sizeof( record ) );
		if ( !Name( record.name, sizeof( record.name ) ) || !Width( record.type ) || !record.size || record.size % Width( record.type ) )
			return false;
		names[i] = (const char *)cursor;
		cursor += sizeof( record );
		if ( record.size > size_t( end - cursor ) )
			return false;
		for ( uint32_t j = 0; j < i; ++j )
			if ( !strcmp( names[j], record.name ) )
				return false;
		for ( uint32_t j = 0; j < schema.fieldCount; ++j ) {
			const auto &field = schema.fields[j];
			if ( strcmp( field.name, record.name ) )
				continue;
			if ( field.sinceVersion > header.version || field.type != record.type || field.count * Width( field.type ) != record.size )
				return false;
			values[j] = cursor;
		}
		cursor += record.size;
	}
	if ( cursor != end )
		return false;
	for ( uint32_t i = 0; i < schema.fieldCount; ++i )
		if ( schema.fields[i].sinceVersion <= header.version && !values[i] )
			return false;
	for ( uint32_t i = 0; i < schema.fieldCount; ++i )
		if ( values[i] )
			memcpy( (uint8_t *)object + schema.fields[i].offset, values[i], schema.fields[i].count * Width( schema.fields[i].type ) );
	*sourceVersion = header.version;
	return true;
}

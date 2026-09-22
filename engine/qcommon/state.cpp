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
	case stateType_t::String:
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
static uint32_t FieldSize( const stateField_t &field, const void *object ) {
	if ( field.type != stateType_t::String )
		return field.count * Width( field.type );
	const auto *text = (const char *)object + field.offset;
	const auto *end = (const char *)memchr( text, 0, field.count );
	return end ? uint32_t( end - text ) + 1 : 0;
}
size_t State_Write( const stateSchema_t &schema, const void *object, void *data, size_t capacity ) {
	if ( !object || !data || !Schema( schema ) )
		return 0;
	size_t payloadSize = sizeof( stateHeader_t );
	for ( uint32_t i = 0; i < schema.fieldCount; ++i ) {
		const uint32_t size = FieldSize( schema.fields[i], object );
		if ( !size )
			return 0;
		payloadSize += sizeof( stateRecord_t ) + size;
	}
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
		stateRecord_t record = { {}, field.type, FieldSize( field, object ) };
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
	uint32_t lengths[256] = {};
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
		if ( record.type == stateType_t::String && ( cursor[record.size - 1] || memchr( cursor, 0, record.size - 1 ) ) )
			return false;
		for ( uint32_t j = 0; j < i; ++j )
			if ( !strcmp( names[j], record.name ) )
				return false;
		for ( uint32_t j = 0; j < schema.fieldCount; ++j ) {
			const auto &field = schema.fields[j];
			if ( strcmp( field.name, record.name ) )
				continue;
			if ( field.sinceVersion > header.version || field.type != record.type ||
				 ( field.type == stateType_t::String ? record.size > field.count : field.count * Width( field.type ) != record.size ) )
				return false;
			values[j] = cursor;
			lengths[j] = record.size;
		}
		cursor += record.size;
	}
	if ( cursor != end )
		return false;
	for ( uint32_t i = 0; i < schema.fieldCount; ++i )
		if ( schema.fields[i].sinceVersion <= header.version && !values[i] )
			return false;
	for ( uint32_t i = 0; i < schema.fieldCount; ++i ) {
		if ( !values[i] )
			continue;
		const auto &field = schema.fields[i];
		auto *destination = (uint8_t *)object + field.offset;
		if ( field.type == stateType_t::String )
			memset( destination, 0, field.count );
		memcpy( destination, values[i], lengths[i] );
	}
	*sourceVersion = header.version;
	return true;
}

struct stateEntry_t {
	uint32_t slot, size;
};
static_assert( sizeof( stateEntry_t ) == 8 && offsetof( stateEntry_t, size ) == 4 );
static constexpr size_t ARCHIVE_START = sizeof( stateEnvelope_t ) + sizeof( uint32_t );
static constexpr uint32_t ARCHIVE_RECORDS = 16384;

static bool ArchiveEntry( const uint8_t *data, size_t size, size_t offset, stateEntry_t *entry, stateHeader_t *header ) {
	if ( offset > size || size - offset < sizeof( *entry ) )
		return false;
	memcpy( entry, data + offset, sizeof( *entry ) );
	offset += sizeof( *entry );
	if ( entry->size < sizeof( stateEnvelope_t ) + sizeof( *header ) || entry->size > size - offset )
		return false;
	stateEnvelope_t envelope;
	memcpy( &envelope, data + offset, sizeof( envelope ) );
	memcpy( header, data + offset + sizeof( envelope ), sizeof( *header ) );
	return !memcmp( envelope.magic, "ASSTATE", 8 ) && envelope.version == 1 && envelope.size == entry->size - sizeof( envelope ) &&
		   Name( header->name, sizeof( header->name ) ) && header->version && header->fieldCount && header->fieldCount <= 256;
}
bool State_Append( stateWriter_t *writer, const stateSchema_t &schema, uint32_t slot, const void *object ) {
	if ( !writer || writer->failed )
		return false;
	writer->failed = true; // Any rejected append prevents a partial checkpoint being published.
	if ( !writer->data || writer->capacity < ARCHIVE_START || writer->records >= ARCHIVE_RECORDS || !Schema( schema ) )
		return false;
	if ( !writer->size )
		writer->size = ARCHIVE_START;
	if ( writer->size > writer->capacity || writer->capacity - writer->size < sizeof( stateEntry_t ) )
		return false;
	auto *data = (uint8_t *)writer->data;
	size_t offset = ARCHIVE_START;
	for ( uint32_t i = 0; i < writer->records; ++i ) {
		stateEntry_t previous;
		stateHeader_t header;
		if ( !ArchiveEntry( data, writer->size, offset, &previous, &header ) || ( previous.slot == slot && !strcmp( header.name, schema.name ) ) )
			return false;
		offset += sizeof( previous ) + previous.size;
	}
	if ( offset != writer->size )
		return false;
	const size_t size = State_Write( schema, object, data + offset + sizeof( stateEntry_t ), writer->capacity - offset - sizeof( stateEntry_t ) );
	if ( !size || size > UINT32_MAX || offset + sizeof( stateEntry_t ) + size - sizeof( stateEnvelope_t ) > UINT32_MAX )
		return false;
	const stateEntry_t entry = { slot, uint32_t( size ) };
	memcpy( data + offset, &entry, sizeof( entry ) );
	writer->size += sizeof( entry ) + size;
	++writer->records;
	writer->failed = false;
	return true;
}
size_t State_Finish( stateWriter_t *writer ) {
	if ( !writer || writer->failed || !writer->data || !writer->records || writer->size < ARCHIVE_START || writer->size > writer->capacity )
		return 0;
	auto *data = (uint8_t *)writer->data;
	stateEnvelope_t envelope = { "ASARCH", 1, uint32_t( writer->size - sizeof( stateEnvelope_t ) ), {} };
	memcpy( data + sizeof( envelope ), &writer->records, sizeof( writer->records ) );
	calc_sha_256( envelope.hash, data + sizeof( envelope ), envelope.size );
	memcpy( data, &envelope, sizeof( envelope ) );
	return writer->size;
}
bool State_Open( const void *data, size_t size, stateReader_t *reader ) {
	if ( !data || !reader || size < ARCHIVE_START )
		return false;
	stateEnvelope_t envelope;
	memcpy( &envelope, data, sizeof( envelope ) );
	if ( memcmp( envelope.magic, "ASARCH\0", 8 ) || envelope.version != 1 || envelope.size != size - sizeof( envelope ) )
		return false;
	const auto *bytes = (const uint8_t *)data;
	uint8_t hash[32];
	calc_sha_256( hash, bytes + sizeof( envelope ), envelope.size );
	if ( memcmp( hash, envelope.hash, sizeof( hash ) ) )
		return false;
	uint32_t records;
	memcpy( &records, bytes + sizeof( envelope ), sizeof( records ) );
	if ( !records || records > ARCHIVE_RECORDS )
		return false;
	size_t offset = ARCHIVE_START;
	for ( uint32_t i = 0; i < records; ++i ) {
		stateEntry_t entry;
		stateHeader_t header;
		if ( !ArchiveEntry( bytes, size, offset, &entry, &header ) )
			return false;
		size_t previousOffset = ARCHIVE_START;
		for ( uint32_t j = 0; j < i; ++j ) {
			stateEntry_t previous;
			stateHeader_t previousHeader;
			if ( !ArchiveEntry( bytes, size, previousOffset, &previous, &previousHeader ) ||
				 ( previous.slot == entry.slot && !strcmp( previousHeader.name, header.name ) ) )
				return false;
			previousOffset += sizeof( previous ) + previous.size;
		}
		offset += sizeof( entry ) + entry.size;
	}
	if ( offset != size )
		return false;
	*reader = { data, size, records };
	return true;
}
bool State_Find( const stateReader_t &reader, const stateSchema_t &schema, uint32_t slot, void *object, uint32_t *sourceVersion, bool *present ) {
	if ( present )
		*present = false;
	if ( !reader.data || reader.records > ARCHIVE_RECORDS || !Schema( schema ) )
		return false;
	// ponytail: linear lookup for bounded explicit saves; add a caller-owned index if load time warrants it.
	const auto *bytes = (const uint8_t *)reader.data;
	size_t offset = ARCHIVE_START;
	for ( uint32_t i = 0; i < reader.records; ++i ) {
		stateEntry_t entry;
		stateHeader_t header;
		if ( !ArchiveEntry( bytes, reader.size, offset, &entry, &header ) )
			return false;
		if ( entry.slot == slot && !strcmp( header.name, schema.name ) ) {
			if ( present )
				*present = true;
			return State_Read( schema, bytes + offset + sizeof( entry ), entry.size, object, sourceVersion );
		}
		offset += sizeof( entry ) + entry.size;
	}
	return false;
}

#include "package_public.h"
#include "qcommon_public.h"
#include "../platform/content_public.h"
#include "../../third_party/sha256/sha-256.h"
#include "../../third_party/zlib/puff.h"
#include <algorithm>
#include <bit>

static_assert( std::endian::native == std::endian::little );
static_assert( sizeof( packageHeader_t ) == 128 && offsetof( packageHeader_t, manifestOffset ) == 16 && offsetof( packageHeader_t, metadataHash ) == 96 );
static_assert( sizeof( packageEntry_t ) == 128 && offsetof( packageEntry_t, offset ) == 64 && offsetof( packageEntry_t, hash ) == 88 && offsetof( packageEntry_t, flags ) == 124 );

static bool ZeroHash( const uint8_t hash[32] ) {
	for ( int i = 0; i < 32; ++i )
		if ( hash[i] )
			return false;
	return true;
}
static bool EntryName( const char name[64] ) {
	const char *end = static_cast<const char *>( memchr( name, 0, 64 ) );
	if ( !end || end == name )
		return false;
	for ( const char *p = end; p < name + 64; ++p )
		if ( *p )
			return false;
	const char *segment = name;
	for ( const char *p = name; p <= end; ++p ) {
		if ( p == end || *p == '/' ) {
			if ( p == segment || ( p - segment == 1 && segment[0] == '.' ) || ( p - segment == 2 && segment[0] == '.' && segment[1] == '.' ) )
				return false;
			segment = p + 1;
		} else if ( !( ( *p >= 'a' && *p <= 'z' ) || ( *p >= '0' && *p <= '9' ) || *p == '_' || *p == '-' || *p == '.' ) ) {
			return false;
		}
	}
	return true;
}
static void HashText( Sha_256 *hash, const char *text ) {
	sha_256_write( hash, text, strlen( text ) );
}
static void HashHex( Sha_256 *hash, const uint8_t value[32] ) {
	static constexpr char digits[] = "0123456789abcdef";
	char text[64];
	for ( int i = 0; i < 32; ++i ) {
		text[2 * i] = digits[value[i] >> 4];
		text[2 * i + 1] = digits[value[i] & 15];
	}
	sha_256_write( hash, text, sizeof( text ) );
}
static void HashEntry( Sha_256 *hash, const packageEntry_t &entry ) {
	sha_256_write( hash, entry.name, strlen( entry.name ) + 1 );
	sha_256_write( hash, entry.hash, sizeof( entry.hash ) );
}
static bool MetadataValid( const package_t *package, const sysContentFile_t &file ) {
	const auto &header = package->header;
	uint64_t cursor = sizeof( header ) + uint64_t( header.count ) * sizeof( packageEntry_t );
	uint8_t ownIdentity[32];
	Sha_256 own;
	sha_256_init( &own, ownIdentity );
	for ( uint32_t i = 0; i < header.count; ++i ) {
		const auto &entry = package->entries[i];
		if ( !EntryName( entry.name ) || ( i && strcmp( package->entries[i - 1].name, entry.name ) >= 0 ) )
			return false;
		if ( entry.flags == 1 ) {
			if ( entry.offset || entry.size || entry.stored || entry.codec || !ZeroHash( entry.hash ) || ZeroHash( header.base ) )
				return false;
		} else {
			if ( entry.flags || entry.codec > 1 || entry.size > PACKAGE_MAX_ASSET_BYTES || entry.stored > PACKAGE_MAX_ASSET_BYTES ||
				 entry.offset != cursor || entry.stored > header.manifestOffset - cursor || ( !entry.codec && entry.size != entry.stored ) )
				return false;
			cursor += entry.stored;
			HashEntry( &own, entry );
		}
	}
	sha_256_close( &own );
	if ( cursor != header.manifestOffset || ( ZeroHash( header.base ) && memcmp( ownIdentity, header.identity, 32 ) ) )
		return false;

	// Hash the exact canonical manifest described by the index. Its fixed schema
	// needs no second JSON parser or persistent manifest allocation in the engine.
	uint8_t expected[32], actual[32];
	Sha_256 hash;
	sha_256_init( &hash, expected );
	sha_256_write( &hash, package->entries, size_t( header.count ) * sizeof( packageEntry_t ) );
	HashText( &hash, "{\"assets\":{" );
	bool comma = false;
	for ( uint32_t i = 0; i < header.count; ++i ) {
		const auto &entry = package->entries[i];
		if ( entry.flags )
			continue;
		HashText( &hash, comma ? ",\"" : "\"" );
		HashText( &hash, entry.name );
		HashText( &hash, "\":\"" );
		HashHex( &hash, entry.hash );
		HashText( &hash, "\"" );
		comma = true;
	}
	HashText( &hash, "},\"base\":" );
	if ( ZeroHash( header.base ) ) {
		HashText( &hash, "null" );
	} else {
		HashText( &hash, "\"" );
		HashHex( &hash, header.base );
		HashText( &hash, "\"" );
	}
	HashText( &hash, ",\"identity\":\"" );
	HashHex( &hash, header.identity );
	HashText( &hash, "\",\"removed\":[" );
	comma = false;
	for ( uint32_t i = 0; i < header.count; ++i ) {
		const auto &entry = package->entries[i];
		if ( !entry.flags )
			continue;
		HashText( &hash, comma ? ",\"" : "\"" );
		HashText( &hash, entry.name );
		HashText( &hash, "\"" );
		comma = true;
	}
	HashText( &hash, "],\"version\":1}" );
	sha_256_close( &hash );
	if ( memcmp( expected, header.metadataHash, 32 ) )
		return false;
	sha_256_init( &hash, actual );
	sha_256_write( &hash, package->entries, size_t( header.count ) * sizeof( packageEntry_t ) );
	uint8_t buffer[4096];
	for ( uint64_t offset = 0; offset < header.manifestSize; ) {
		const size_t size = size_t( std::min( uint64_t( sizeof( buffer ) ), header.manifestSize - offset ) );
		if ( !Sys_ReadContentFile( file, header.manifestOffset + offset, buffer, size ) )
			return false;
		sha_256_write( &hash, buffer, size );
		offset += size;
	}
	sha_256_close( &hash );
	return !memcmp( actual, expected, 32 );
}
package_t *Package_Load( const char *path ) {
	if ( !path || strlen( path ) >= MAX_OSPATH )
		return nullptr;
	sysContentFile_t file{};
	if ( !Sys_OpenContentFile( path, &file ) )
		return nullptr;
	packageHeader_t header{};
	if ( !Sys_ReadContentFile( file, 0, &header, sizeof( header ) ) || memcmp( header.magic, "ASPACK\0", 8 ) || header.version != 1 ||
		 header.count > PACKAGE_MAX_ASSETS || header.manifestSize > 16 * 1024 * 1024 || header.manifestOffset > file.size ||
		 header.manifestSize != file.size - header.manifestOffset || header.manifestOffset < sizeof( header ) + uint64_t( header.count ) * sizeof( packageEntry_t ) ) {
		Sys_CloseContentFile( &file );
		return nullptr;
	}
	const size_t indexSize = size_t( header.count ) * sizeof( packageEntry_t );
	auto *package = static_cast<package_t *>( Z_TagMalloc( sizeof( package_t ) + indexSize, TAG_PACK ) );
	package->header = header;
	package->entries = reinterpret_cast<packageEntry_t *>( package + 1 );
	memcpy( package->path, path, strlen( path ) + 1 );
	const bool valid = Sys_ReadContentFile( file, sizeof( header ), package->entries, indexSize ) && MetadataValid( package, file );
	Sys_CloseContentFile( &file );
	if ( !valid ) {
		Z_Free( package );
		return nullptr;
	}
	return package;
}
void Package_Free( package_t *package ) {
	if ( package )
		Z_Free( package );
}
int Package_Find( const package_t *package, const char *name ) {
	if ( !package || !name )
		return -1;
	uint32_t first = 0, end = package->header.count;
	while ( first < end ) {
		const uint32_t middle = first + ( end - first ) / 2;
		const int order = strcmp( package->entries[middle].name, name );
		if ( !order )
			return int( middle );
		if ( order < 0 )
			first = middle + 1;
		else
			end = middle;
	}
	return -1;
}

bool Package_ValidateMounts( package_t *const *packages, uint32_t count, uint8_t identity[32] ) {
	if ( !identity || count > 64 || ( count && !packages ) )
		return false;
	auto **storage = static_cast<const packageEntry_t **>( Z_TagMalloc( 2 * PACKAGE_MAX_ASSETS * sizeof( const packageEntry_t * ), TAG_PACK ) );
	auto **view = storage, **next = storage + PACKAGE_MAX_ASSETS;
	uint32_t entries = 0;
	uint8_t current[32];
	Sha_256 hash;
	sha_256_init( &hash, current );
	sha_256_close( &hash );
	bool valid = true;
	for ( uint32_t mount = 0; mount < count && valid; ++mount ) {
		const auto *package = packages[mount];
		if ( !package || ( !ZeroHash( package->header.base ) && memcmp( package->header.base, current, 32 ) ) ) {
			valid = false;
			break;
		}
		uint32_t old = 0, added = 0, written = 0;
		while ( old < entries || added < package->header.count ) {
			const auto *entry = added < package->header.count ? &package->entries[added] : nullptr;
			const int order = !entry ? -1 : old == entries ? 1
														   : strcmp( view[old]->name, entry->name );
			const packageEntry_t *chosen = nullptr;
			if ( order < 0 ) {
				chosen = view[old++];
			} else {
				if ( !order )
					++old;
				else if ( entry->flags ) {
					valid = false;
					break;
				}
				++added;
				if ( !entry->flags )
					chosen = entry;
			}
			if ( chosen ) {
				if ( written == PACKAGE_MAX_ASSETS ) {
					valid = false;
					break;
				}
				next[written++] = chosen;
			}
		}
		if ( !valid )
			break;
		std::swap( view, next );
		entries = written;
		sha_256_init( &hash, current );
		for ( uint32_t i = 0; i < entries; ++i )
			HashEntry( &hash, *view[i] );
		sha_256_close( &hash );
		if ( !ZeroHash( package->header.base ) && memcmp( package->header.identity, current, 32 ) )
			valid = false;
	}
	Z_Free( storage );
	if ( valid )
		memcpy( identity, current, 32 );
	return valid;
}

struct packageStream_t {
	sysContentFile_t file;
	packageEntry_t entry;
	uint8_t *decoded;
	uint64_t position;
};
void Package_CloseAsset( packageStream_t *stream ) {
	if ( stream ) {
		Sys_CloseContentFile( &stream->file );
		if ( stream->decoded )
			Z_Free( stream->decoded );
		Z_Free( stream );
	}
}
packageStream_t *Package_OpenAsset( const package_t *package, uint32_t index ) {
	if ( !package || index >= package->header.count || package->entries[index].flags )
		return nullptr;
	auto *stream = static_cast<packageStream_t *>( Z_Malloc( sizeof( packageStream_t ) ) );
	stream->entry = package->entries[index];
	if ( !Sys_OpenContentFile( package->path, &stream->file ) ) {
		Package_CloseAsset( stream );
		return nullptr;
	}
	const auto &entry = stream->entry;
	uint8_t hash[32];
	bool valid = true;
	if ( entry.codec ) {
		// ponytail: bounded per-open decoding reuses puff; add an incremental decoder
		// only if compressed stream sizes make this asset-lifetime buffer unsuitable.
		stream->decoded = static_cast<uint8_t *>( Z_TagMalloc( size_t( std::max( entry.size, UINT64_C( 1 ) ) ), TAG_PACK ) );
		auto *compressed = static_cast<uint8_t *>( Z_TagMalloc( size_t( std::max( entry.stored, UINT64_C( 1 ) ) ), TAG_PACK ) );
		uint32_t input = uint32_t( entry.stored ), output = uint32_t( entry.size );
		valid = Sys_ReadContentFile( stream->file, entry.offset, compressed, input ) &&
				puff( stream->decoded, &output, compressed, &input ) == 0 && input == entry.stored && output == entry.size;
		Z_Free( compressed );
		Sys_CloseContentFile( &stream->file );
		if ( valid )
			calc_sha_256( hash, stream->decoded, size_t( entry.size ) );
	} else {
		Sha_256 state;
		sha_256_init( &state, hash );
		uint8_t buffer[65536];
		for ( uint64_t offset = 0; offset < entry.size; ) {
			const size_t size = size_t( std::min( uint64_t( sizeof( buffer ) ), entry.size - offset ) );
			if ( !Sys_ReadContentFile( stream->file, entry.offset + offset, buffer, size ) ) {
				valid = false;
				break;
			}
			sha_256_write( &state, buffer, size );
			offset += size;
		}
		sha_256_close( &state );
	}
	if ( !valid || memcmp( hash, entry.hash, 32 ) ) {
		Package_CloseAsset( stream );
		return nullptr;
	}
	return stream;
}
int Package_ReadAsset( packageStream_t *stream, void *data, int size ) {
	if ( !stream || size < 0 || ( size && !data ) )
		return -1;
	const size_t count = size_t( std::min( uint64_t( size ), stream->entry.size - stream->position ) );
	if ( count ) {
		if ( stream->decoded )
			memcpy( data, stream->decoded + stream->position, count );
		else if ( !Sys_ReadContentFile( stream->file, stream->entry.offset + stream->position, data, count ) )
			return -1;
	}
	stream->position += count;
	return int( count );
}
bool Package_SeekAsset( packageStream_t *stream, int64_t offset, fsOrigin_t origin ) {
	if ( !stream || ( origin != FS_SEEK_SET && origin != FS_SEEK_CUR && origin != FS_SEEK_END ) )
		return false;
	const int64_t base = origin == FS_SEEK_CUR ? int64_t( stream->position ) : origin == FS_SEEK_END ? int64_t( stream->entry.size )
																									 : 0;
	// Package extents are bounded to 256 MiB. Check the signed offset before
	// addition, including INT64_MIN/MAX requests from 64-bit file interfaces.
	if ( offset < -base || offset > int64_t( stream->entry.size ) - base )
		return false;
	stream->position = uint64_t( base + offset );
	return true;
}
uint64_t Package_TellAsset( const packageStream_t *stream ) {
	return stream ? stream->position : 0;
}

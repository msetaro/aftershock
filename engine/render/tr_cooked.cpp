#include "tr_cooked.h"
#include "iqm.h"
#include "../../third_party/sha256/sha-256.h"
#include <bit>
#include <string.h>

struct ktxHeader_t {
	uint8_t magic[12];
	uint32_t format, typeSize, width, height, depth, layers, faces, levels, compression;
	uint32_t dfdOffset, dfdSize, kvdOffset, kvdSize;
	uint64_t sgdOffset, sgdSize;
};
struct ktxLevel_t {
	uint64_t offset, size, uncompressedSize;
};
static_assert( sizeof( ktxHeader_t ) == 80 && offsetof( ktxHeader_t, sgdOffset ) == 64 && std::is_trivially_copyable_v<ktxHeader_t> );
static_assert( sizeof( ktxLevel_t ) == 24 && std::is_trivially_copyable_v<ktxLevel_t> );
static_assert( std::endian::native == std::endian::little );
static_assert( std::is_trivially_destructible_v<Sha_256> );

static bool range( size_t size, uint64_t offset, uint64_t length ) {
	return offset <= size && length <= size - offset;
}

bool R_ReadCookedTexture( const void *data, size_t size, cookedTexture_t *texture ) {
	*texture = {};
	if ( !data || size < sizeof( ktxHeader_t ) || size > INT32_MAX )
		return false;
	const uint8_t *bytes = (const uint8_t *)data;
	ktxHeader_t h;
	memcpy( &h, data, sizeof( h ) );
	if ( memcmp( h.magic, "\xabKTX 20\xbb\r\n\x1a\n", 12 ) || h.typeSize != 1 || h.depth || h.layers || h.faces != 1 || h.compression || h.sgdOffset || h.sgdSize || !h.width || !h.height || h.width > 16384 || h.height > 16384 || !h.levels || h.levels > 15 )
		return false;
	uint32_t blockBytes, colorModel;
	switch ( h.format ) {
	case 139:
		texture->format = rhiFormat_t::BC4;
		blockBytes = 8;
		colorModel = 131;
		break;
	case 141:
		texture->format = rhiFormat_t::BC5;
		blockBytes = 16;
		colorModel = 132;
		break;
	case 145:
		texture->format = rhiFormat_t::BC7;
		blockBytes = 16;
		colorModel = 134;
		break;
	case 146:
		texture->format = rhiFormat_t::BC7_SRGB;
		blockBytes = 16;
		colorModel = 134;
		break;
	default:
		return false;
	}
	const uint32_t indexEnd = sizeof( h ) + h.levels * sizeof( ktxLevel_t );
	if ( !range( size, sizeof( h ), h.levels * sizeof( ktxLevel_t ) ) || h.dfdOffset < indexEnd || h.dfdSize < 44 || !range( size, h.dfdOffset, h.dfdSize ) || h.kvdOffset < h.dfdOffset + h.dfdSize || !range( size, h.kvdOffset, h.kvdSize ) )
		return false;
	uint32_t dfd[7];
	memcpy( dfd, bytes + h.dfdOffset, sizeof( dfd ) );
	const uint32_t samples = h.format == 141 ? 2 : 1;
	if ( dfd[0] != h.dfdSize || h.dfdSize != 28 + 16 * samples || dfd[1] || dfd[2] != ( 2U | ( ( h.dfdSize - 4 ) << 16 ) ) || ( dfd[3] & 255 ) != colorModel || ( ( dfd[3] >> 16 ) & 255 ) != ( h.format == 146 ? 2U : 1U ) || dfd[4] != 0x303 || dfd[5] != blockBytes || dfd[6] )
		return false;
	const uint8_t *hash = nullptr;
	bool version = false, source = false;
	for ( uint32_t pos = h.kvdOffset, end = h.kvdOffset + h.kvdSize; pos < end; ) {
		if ( end - pos < 4 )
			return false;
		uint32_t length;
		memcpy( &length, bytes + pos, 4 );
		pos += 4;
		if ( !length || length > end - pos )
			return false;
		const uint8_t *entry = bytes + pos;
		const uint8_t *nul = (const uint8_t *)memchr( entry, 0, length );
		if ( !nul )
			return false;
		const uint32_t valueSize = length - (uint32_t)( nul + 1 - entry );
		if ( !strcmp( (const char *)entry, "aftershock.contentHash" ) ) {
			if ( hash || valueSize != 32 )
				return false;
			hash = nul + 1;
		} else if ( !strcmp( (const char *)entry, "aftershock.version" ) ) {
			uint32_t value;
			if ( version || valueSize != 4 )
				return false;
			memcpy( &value, nul + 1, 4 );
			if ( value != 1 )
				return false;
			version = true;
		} else if ( !strcmp( (const char *)entry, "aftershock.sourceHash" ) ) {
			if ( source || valueSize != 32 )
				return false;
			source = true;
		}
		const uint32_t padded = ( length + 3 ) & ~3U;
		if ( padded > end - pos )
			return false;
		pos += padded;
	}
	if ( !hash || !version || !source )
		return false;
	uint32_t width = h.width, height = h.height;
	uint64_t previous = size;
	for ( uint32_t i = 0; i < h.levels; i++ ) {
		ktxLevel_t level;
		memcpy( &level, bytes + sizeof( h ) + sizeof( level ) * i, sizeof( level ) );
		const uint32_t expected = ( ( width + 3 ) / 4 ) * ( ( height + 3 ) / 4 ) * blockBytes;
		if ( level.size != expected || level.uncompressedSize != expected || level.offset % blockBytes || level.offset < h.kvdOffset + h.kvdSize || !range( previous, level.offset, level.size ) )
			return false;
		texture->levels[i] = { bytes + level.offset, expected };
		texture->size += expected;
		previous = level.offset;
		if ( width == 1 && height == 1 && i + 1 != h.levels )
			return false;
		width = width > 1 ? width / 2 : 1;
		height = height > 1 ? height / 2 : 1;
	}
	uint8_t calculated[32], zeros[32] = {};
	Sha_256 state;
	sha_256_init( &state, calculated );
	sha_256_write( &state, bytes, (size_t)( hash - bytes ) );
	sha_256_write( &state, zeros, sizeof( zeros ) );
	sha_256_write( &state, hash + 32, size - (size_t)( hash + 32 - bytes ) );
	sha_256_close( &state );
	if ( memcmp( calculated, hash, 32 ) )
		return false;
	texture->width = h.width;
	texture->height = h.height;
	texture->mipLevels = h.levels;
	memcpy( texture->contentHash, hash, 32 );
	calc_sha_256( texture->fileHash, bytes, size );
	return true;
}

struct cookedHeader_t {
	uint8_t magic[8];
	uint32_t version, size;
	uint8_t hash[32];
};
static_assert( sizeof( cookedHeader_t ) == 48 && std::is_trivially_copyable_v<cookedHeader_t> );

bool R_ReadCookedMaterial( const void *data, size_t size, cookedMaterial_t *material, uint8_t fileHash[32] ) {
	*material = {};
	if ( !data || size != sizeof( cookedHeader_t ) + sizeof( *material ) )
		return false;
	cookedHeader_t header;
	memcpy( &header, data, sizeof( header ) );
	if ( memcmp( header.magic, "ASMAT\0\0\0", 8 ) || header.version != 1 || header.size != sizeof( *material ) )
		return false;
	const uint8_t *payload = (const uint8_t *)data + sizeof( header );
	uint8_t hash[32];
	calc_sha_256( hash, payload, header.size );
	if ( memcmp( hash, header.hash, sizeof( hash ) ) )
		return false;
	memcpy( material, payload, sizeof( *material ) );
	for ( float color : material->color ) {
		if ( !( color >= 0 && color <= 1 ) )
			return false;
	}
	if ( !( material->alphaCutoff >= 0 && material->alphaCutoff <= 1 ) || material->flags > 15 || ( material->flags & 12 ) == 12 || ( ( material->flags & 8 ) && material->alphaCutoff != 0.5f ) )
		return false;
	const char *end = (const char *)memchr( material->texture, 0, sizeof( material->texture ) );
	if ( !end || end == material->texture || material->texture[0] == '/' || strstr( material->texture, ".." ) )
		return false;
	for ( const char *p = material->texture; p != end; p++ ) {
		if ( !( ( *p >= 'a' && *p <= 'z' ) || ( *p >= '0' && *p <= '9' ) || *p == '_' || *p == '/' || *p == '.' || *p == '-' ) )
			return false;
	}
	if ( fileHash )
		calc_sha_256( fileHash, data, size );
	return true;
}

bool R_CookedHashMatches( const void *data, size_t size, const uint8_t hash[32] ) {
	uint8_t calculated[32];
	calc_sha_256( calculated, data, size );
	return memcmp( calculated, hash, 32 ) == 0;
}

bool R_ReadCookedIndex( const void *data, size_t size, const uint8_t revision[32], cookedIndex_t *index ) {
	*index = {};
	if ( !data || size < sizeof( cookedHeader_t ) + 4 || size > 52 + 4096 * sizeof( cookedEntry_t ) )
		return false;
	cookedHeader_t header;
	memcpy( &header, data, sizeof( header ) );
	const uint8_t *payload = (const uint8_t *)data + sizeof( header );
	uint32_t count;
	memcpy( &count, payload, 4 );
	if ( memcmp( header.magic, "ASIDX\0\0\0", 8 ) || header.version != 1 || count > 4096 || header.size != 4 + count * sizeof( cookedEntry_t ) || size != sizeof( header ) + header.size || !R_CookedHashMatches( payload, header.size, header.hash ) || !R_CookedHashMatches( data, size, revision ) )
		return false;
	for ( uint32_t i = 0; i < count; i++ ) {
		cookedEntry_t entry;
		memcpy( &entry, payload + 4 + i * sizeof( entry ), sizeof( entry ) );
		if ( !entry.path[0] || entry.path[0] == '/' || !memchr( entry.path, 0, sizeof( entry.path ) ) || strstr( entry.path, ".." ) || entry.size > INT32_MAX || entry.kind < 1 || entry.kind > 7 )
			return false;
	}
	index->entries = payload + 4;
	index->count = count;
	return true;
}

struct iqmCookedExtension_t {
	uint32_t name, size, offset, next;
};
struct iqmCookedStamp_t {
	uint32_t version;
	uint8_t sourceHash[32], contentHash[32];
};
static_assert( sizeof( iqmCookedExtension_t ) == 16 && std::is_trivially_copyable_v<iqmCookedExtension_t> );
static_assert( sizeof( iqmCookedStamp_t ) == 68 && offsetof( iqmCookedStamp_t, contentHash ) == 36 && std::is_trivially_copyable_v<iqmCookedStamp_t> );

cookedModelStatus_t R_ReadCookedModel( const void *data, size_t size, uint8_t hash[32] ) {
	memset( hash, 0, 32 );
	if ( !data || size < sizeof( iqmHeader_t ) || size > 16 * 1024 * 1024 )
		return cookedModelStatus_t::Invalid;
	iqmHeader_t header;
	memcpy( &header, data, sizeof( header ) );
	if ( memcmp( header.magic, IQM_MAGIC, sizeof( header.magic ) ) || header.version != IQM_VERSION || header.filesize > size || header.num_extensions > 4096 )
		return cookedModelStatus_t::Invalid;
	const uint8_t *bytes = (const uint8_t *)data;
	uint32_t offset = header.ofs_extensions;
	bool found = false;
	for ( uint32_t i = 0; i < header.num_extensions; i++ ) {
		if ( !range( header.filesize, offset, sizeof( iqmCookedExtension_t ) ) )
			return cookedModelStatus_t::Invalid;
		iqmCookedExtension_t extension;
		memcpy( &extension, bytes + offset, sizeof( extension ) );
		if ( !range( header.filesize, header.ofs_text, header.num_text ) || extension.name >= header.num_text )
			return cookedModelStatus_t::Invalid;
		const char *name = (const char *)bytes + header.ofs_text + extension.name;
		if ( !memchr( name, 0, header.num_text - extension.name ) )
			return cookedModelStatus_t::Invalid;
		if ( !strcmp( name, "aftershock.cook" ) ) {
			if ( found || extension.size != sizeof( iqmCookedStamp_t ) || !range( header.filesize, extension.offset, extension.size ) )
				return cookedModelStatus_t::Invalid;
			iqmCookedStamp_t stamp;
			memcpy( &stamp, bytes + extension.offset, sizeof( stamp ) );
			if ( stamp.version != 1 )
				return cookedModelStatus_t::Invalid;
			uint8_t calculated[32], zeros[32] = {};
			const uint32_t hashOffset = extension.offset + offsetof( iqmCookedStamp_t, contentHash );
			Sha_256 state;
			sha_256_init( &state, calculated );
			sha_256_write( &state, bytes, hashOffset );
			sha_256_write( &state, zeros, sizeof( zeros ) );
			sha_256_write( &state, bytes + hashOffset + 32, size - hashOffset - 32 );
			sha_256_close( &state );
			if ( memcmp( calculated, stamp.contentHash, 32 ) )
				return cookedModelStatus_t::Invalid;
			found = true;
		}
		offset = extension.next;
		if ( ( i + 1 == header.num_extensions ) != ( offset == 0 ) )
			return cookedModelStatus_t::Invalid;
	}
	if ( !found )
		return cookedModelStatus_t::Legacy;
	calc_sha_256( hash, data, size );
	return cookedModelStatus_t::Valid;
}

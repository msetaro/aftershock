#include "snd_event.h"
#include "../../third_party/sha256/sha-256.h"
#include <cmath>
#include <string.h>

struct soundEventHeader_t {
	char magic[8];
	uint32_t version, size;
	uint8_t hash[32];
};
static_assert( sizeof( soundEventHeader_t ) == 48 && offsetof( soundEventHeader_t, hash ) == 16 );

static bool Path( const char *path, size_t size ) {
	const char *end = (const char *)memchr( path, 0, size );
	if ( !end || end == path || path[0] == '/' || strstr( path, ".." ) )
		return false;
	for ( const char *c = path; c < end; ++c )
		if ( !( ( *c >= 'a' && *c <= 'z' ) || ( *c >= '0' && *c <= '9' ) || *c == '_' || *c == '-' || *c == '/' || *c == '.' ) )
			return false;
	return true;
}

static bool Range( float value, float low, float high ) {
	return std::isfinite( value ) && value >= low && value <= high;
}

bool S_ReadSoundEvent( const void *data, size_t size, sSoundEvent_t *event ) {
	if ( !event )
		return false;
	*event = {};
	if ( !data || size != sizeof( soundEventHeader_t ) + sizeof( *event ) )
		return false;
	soundEventHeader_t header;
	memcpy( &header, data, sizeof( header ) );
	if ( memcmp( header.magic, "ASEVENT\0", 8 ) || header.version != 1 || header.size != sizeof( *event ) )
		return false;
	const uint8_t *payload = (const uint8_t *)data + sizeof( header );
	uint8_t hash[32];
	calc_sha_256( hash, payload, sizeof( *event ) );
	if ( memcmp( hash, header.hash, sizeof( hash ) ) )
		return false;
	sSoundEvent_t candidate;
	memcpy( &candidate, payload, sizeof( candidate ) );
	if ( !Path( candidate.name, sizeof( candidate.name ) ) || candidate.bus >= S_BUS_COUNT || candidate.group > 15 ||
		 candidate.priority > 255 || !candidate.voiceLimit || candidate.voiceLimit > 96 || candidate.model > S_DISTANCE_INVERSE ||
		 ( candidate.flags & ~3u ) || !candidate.layerCount || candidate.layerCount > 4 || candidate.reserved ||
		 !Range( candidate.referenceDistance, 0.01f, 65535.0f ) || !Range( candidate.maxDistance, 1.0f, 65536.0f ) ||
		 candidate.maxDistance <= candidate.referenceDistance || !Range( candidate.rolloff, 0.0f, 16.0f ) || !Range( candidate.reverbSend, 0.0f, 1.0f ) )
		return false;
	for ( uint32_t i = 0; i < candidate.layerCount; ++i ) {
		const auto &layer = candidate.layers[i];
		if ( !Path( layer.sample, sizeof( layer.sample ) ) || layer.role > S_LAYER_DISTANT ||
			 !Range( layer.gain, 0.0f, 2.0f ) || !Range( layer.minDistance, 0.0f, candidate.maxDistance ) ||
			 !Range( layer.maxDistance, 1.0f, candidate.maxDistance ) || layer.maxDistance <= layer.minDistance )
			return false;
		const size_t length = strlen( layer.sample );
		if ( length < 5 || strcmp( layer.sample + length - 4, ".wav" ) )
			return false;
	}
	*event = candidate;
	return true;
}

float S_EventLayerGain( const sSoundLayer_t &layer, float distance ) {
	return distance >= layer.minDistance && distance < layer.maxDistance ? layer.gain : 0.0f;
}

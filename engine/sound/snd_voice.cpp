#include "snd_voice.h"
#include "snd_public.h"
#include "../qcommon/qcommon_public.h"
#include "../../third_party/opus/include/opus.h"
#include <algorithm>
#include <cmath>

static OpusEncoder *encoder;
static byte *codecMemory;
static sVoiceStats_t counters;
static struct voiceSpeaker_t {
	OpusDecoder *decoder;
	int16_t pcm[5760];
	uint32_t read, count, next;
	double fraction;
	byte generation;
	bool received;
} speakers[MAX_CLIENTS];

void S_VoiceReset() {
	if ( encoder )
		opus_encoder_ctl( encoder, OPUS_RESET_STATE );
	for ( auto &speaker : speakers ) {
		OpusDecoder *decoder = speaker.decoder;
		speaker = {};
		speaker.decoder = decoder;
		if ( decoder )
			opus_decoder_ctl( decoder, OPUS_RESET_STATE );
	}
	counters = {};
}

void S_VoiceShutdown() {
	if ( codecMemory )
		Z_Free( codecMemory );
	codecMemory = nullptr;
	encoder = nullptr;
	memset( speakers, 0, sizeof( speakers ) );
	counters = {};
}

bool S_VoiceInit() {
	if ( encoder )
		return true;
	const int encBytes = opus_encoder_get_size( 1 ), decBytes = opus_decoder_get_size( 1 );
	if ( encBytes <= 0 || encBytes > 1024 * 1024 || decBytes <= 0 || decBytes > 128 * 1024 )
		return false;
	const size_t encStride = ( size_t( encBytes ) + 15 ) & ~size_t( 15 );
	const size_t decStride = ( size_t( decBytes ) + 15 ) & ~size_t( 15 );
	codecMemory = (byte *)Z_Malloc( encStride + MAX_CLIENTS * decStride );
	encoder = (OpusEncoder *)codecMemory;
	bool ready = opus_encoder_init( encoder, 48000, 1, OPUS_APPLICATION_VOIP ) == OPUS_OK;
	if ( ready )
		ready = opus_encoder_ctl( encoder, OPUS_SET_BITRATE( 24000 ) ) == OPUS_OK &&
				opus_encoder_ctl( encoder, OPUS_SET_COMPLEXITY( 5 ) ) == OPUS_OK;
	for ( uint32_t i = 0; ready && i < MAX_CLIENTS; ++i ) {
		speakers[i].decoder = (OpusDecoder *)( codecMemory + encStride + i * decStride );
		ready = opus_decoder_init( speakers[i].decoder, 48000, 1 ) == OPUS_OK;
	}
	if ( !ready ) {
		S_VoiceShutdown();
		return false;
	}
	S_VoiceReset();
	return true;
}

int S_VoiceEncode( const int16_t *pcm, byte *data, int capacity ) {
	if ( !encoder || !pcm || !data || capacity < 1 || capacity > 4000 )
		return 0;
	const int size = opus_encode( encoder, pcm, 960, data, capacity );
	if ( size <= 0 )
		return 0;
	++counters.encoded;
	return size;
}

static void Enqueue( voiceSpeaker_t &speaker, const int16_t *pcm, uint32_t count ) {
	if ( speaker.count + count > 5760 ) {
		const uint32_t dropped = speaker.count + count - 5760;
		speaker.read = ( speaker.read + dropped ) % 5760;
		speaker.count -= dropped;
		speaker.fraction = 0;
		++counters.overruns;
	}
	for ( uint32_t i = 0; i < count; ++i )
		speaker.pcm[( speaker.read + speaker.count + i ) % 5760] = pcm[i];
	speaker.count += count;
}

bool S_VoiceReceive( int sender, byte generation, uint32_t sequence, int frames, const byte *data, int size ) {
	if ( !encoder || sender < 0 || sender >= MAX_CLIENTS || frames < 1 || frames > 3 ||
		 !data || size < 1 || size > 4000 || opus_packet_get_nb_samples( data, size, 48000 ) != frames * 960 ) {
		++counters.rejected;
		return false;
	}
	auto &speaker = speakers[sender];
	int32_t gap = 0;
	if ( speaker.received && generation == speaker.generation ) {
		gap = int32_t( sequence - speaker.next );
		if ( gap < 0 ) {
			++counters.rejected;
			return false;
		}
	}
	if ( !speaker.received || generation != speaker.generation || gap > 3 ) {
		opus_decoder_ctl( speaker.decoder, OPUS_RESET_STATE );
		speaker.count = speaker.read = 0;
		speaker.fraction = 0;
		gap = 0;
	}
	int16_t pcm[2880];
	for ( int i = 0; i < gap; ++i ) {
		const int count = opus_decode( speaker.decoder, nullptr, 0, pcm, 960, 0 );
		if ( count == 960 ) {
			Enqueue( speaker, pcm, 960 );
			++counters.concealed;
		}
	}
	const int count = opus_decode( speaker.decoder, data, size, pcm, 2880, 0 );
	if ( count != frames * 960 ) {
		++counters.rejected;
		return false;
	}
	Enqueue( speaker, pcm, uint32_t( count ) );
	speaker.next = sequence + uint32_t( frames );
	speaker.generation = generation;
	speaker.received = true;
	++counters.decoded;
	return true;
}

bool S_VoiceActive() {
	for ( const auto &speaker : speakers )
		if ( speaker.count )
			return true;
	return false;
}

void S_VoiceMix( float ( *output )[2], uint32_t frames, int rate ) {
	if ( !output || rate < 8000 || rate > 192000 )
		return;
	const double step = 48000.0 / rate;
	for ( auto &speaker : speakers ) {
		for ( uint32_t frame = 0; frame < frames && speaker.count; ++frame ) {
			const float a = speaker.pcm[speaker.read];
			const float b = speaker.pcm[( speaker.read + ( speaker.count > 1 ? 1 : 0 ) ) % 5760];
			const float sample = a + float( speaker.fraction ) * ( b - a );
			output[frame][0] += sample;
			output[frame][1] += sample;
			speaker.fraction += step;
			const uint32_t advance = std::min( uint32_t( speaker.fraction ), speaker.count );
			speaker.read = ( speaker.read + advance ) % 5760;
			speaker.count -= advance;
			speaker.fraction = speaker.count ? speaker.fraction - advance : 0;
		}
	}
}

void S_VoiceStats( sVoiceStats_t *stats ) {
	*stats = counters;
	for ( const auto &speaker : speakers )
		stats->queued += speaker.count;
}

#include "../../engine/sound/snd_event.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <cmath>

static void Reverb() {
	static sReverb_t room, outdoors;
	assert( S_ConfigureReverb( &room, 48000, .5f, .5f, .3f ) );
	assert( S_ConfigureReverb( &outdoors, 48000, 0, .5f, .3f ) );
	double early = 0, late = 0;
	for ( int i = 0; i < 96000; ++i ) {
		float a[2], b[2];
		S_ReverbSample( &room, i == 0 ? 1000.0f : 0.0f, a );
		S_ReverbSample( &outdoors, i == 0 ? 1000.0f : 0.0f, b );
		assert( b[0] == 0 && b[1] == 0 );
		assert( std::isfinite( a[0] ) && std::isfinite( a[1] ) );
		if ( i < 1000 )
			assert( a[0] == 0 && a[1] == 0 );
		const double energy = double( a[0] ) * a[0] + double( a[1] ) * a[1];
		if ( i < 24000 )
			early += energy;
		if ( i > 48000 )
			late += energy;
	}
	assert( early > 100 && late < early * .0001 );
	assert( !S_ConfigureReverb( &room, 0, .5f, .5f, .3f ) );
}

int main( int argc, char **argv ) {
	Reverb();
	assert( argc == 2 );
	FILE *file = fopen( argv[1], "rb" );
	assert( file );
	unsigned char data[448];
	assert( fread( data, 1, sizeof( data ), file ) == sizeof( data ) );
	assert( fgetc( file ) == EOF );
	fclose( file );
	sSoundEvent_t event;
	assert( S_ReadSoundEvent( data, sizeof( data ), &event ) );
	assert( !strcmp( event.name, "rifle" ) && event.bus == S_BUS_WEAPONS );
	assert( event.group == 2 && event.priority == 100 && event.voiceLimit == 8 );
	assert( event.layerCount == 3 && event.flags == 3 && event.model == S_DISTANCE_INVERSE );
	assert( event.referenceDistance == 80 && event.maxDistance == 4096 && event.reverbSend == .25f );
	assert( S_EventLayerGain( event.layers[0], 0 ) == .5f );
	assert( S_EventLayerGain( event.layers[0], 255 ) == .5f );
	assert( S_EventLayerGain( event.layers[0], 256 ) == 0.0f );
	assert( S_EventLayerGain( event.layers[2], 255 ) == 0.0f );
	assert( S_EventLayerGain( event.layers[2], 256 ) == .75f );
	assert( S_EventLayerGain( event.layers[2], 4096 ) == 0.0f );
	static sEventMixer_t mixer;
	static int16_t pcm[480] = {};
	for ( auto &sample : pcm )
		sample = 1000;
	sEventPCM_t samples[4] = {};
	for ( int i = 0; i < 3; ++i )
		samples[i] = { pcm, 480, 48000, 1 };
	sSpatialInput_t spatial = {};
	spatial.right[1] = -1;
	spatial.offset[1] = -40;
	spatial.speedOfSound = 13504;
	float output[256][2] = {};
	assert( S_StartEventVoice( &mixer, &event, samples, spatial, false, 48000, .0875f ) >= 0 );
	S_MixEvents( &mixer, output, 256, 48000 );
	assert( output[100][0] == 0 && std::fabs( output[100][1] - 1500 ) < .01f );
	assert( mixer.active == 1 );
	S_MixEvents( &mixer, output, 256, 48000 );
	assert( mixer.active == 0 );
	mixer = {};
	memset( output, 0, sizeof( output ) );
	spatial.offset[1] = -2048;
	assert( S_StartEventVoice( &mixer, &event, samples, spatial, false, 48000, .0875f ) >= 0 );
	S_MixEvents( &mixer, output, 256, 48000 );
	assert( std::fabs( output[100][1] - 750 * (80.0f / 2048.0f) ) < .01f );
	mixer = {};
	event.voiceLimit = 2;
	spatial.offset[1] = -40;
	for ( int i = 0; i < 10; ++i )
		assert( S_StartEventVoice( &mixer, &event, samples, spatial, false, 48000, .0875f ) >= 0 );
	assert( mixer.active == 2 && mixer.stolen == 8 );
	auto low = event;
	low.priority = event.priority - 1;
	assert( S_StartEventVoice( &mixer, &low, samples, spatial, false, 48000, .0875f ) == -1 );
	assert( mixer.active == 2 );
	mixer = {};
	assert( S_StartEventVoice( &mixer, &event, samples, spatial, true, 48000, .0875f ) >= 0 );
	for ( int i = 0; i < 20; ++i ) {
		memset( output, 0, sizeof( output ) );
		S_MixEvents( &mixer, output, 256, 48000 );
		for ( const auto &frame : output )
			assert( std::isfinite( frame[0] ) && std::isfinite( frame[1] ) );
	}
	assert( mixer.active == 0 );
	// Bus attenuation and voice ducking operate on actual mixed samples.
	mixer = {};
	auto music = event, speech = event;
	music.bus = S_BUS_MUSIC;
	speech.bus = S_BUS_VOICE;
	music.layerCount = speech.layerCount = 1;
	music.layers[0].gain = speech.layers[0].gain = 1;
	static int16_t sustained[48000];
	for ( auto &sample : sustained )
		sample = 1000;
	samples[0] = { sustained, 48000, 48000, 1 };
	mixer.busGain[S_BUS_MUSIC] = .5f;
	assert( S_StartEventVoice( &mixer, &music, samples, spatial, false, 48000, .0875f ) >= 0 );
	memset( output, 0, sizeof( output ) );
	S_MixEvents( &mixer, output, 256, 48000 );
	assert( std::fabs( output[100][1] - 500 ) < .01f );
	const int talker = S_StartEventVoice( &mixer, &speech, samples, spatial, false, 48000, .0875f );
	assert( talker >= 0 );
	for ( int i = 0; i < 10; ++i ) {
		memset( output, 0, sizeof( output ) );
		S_MixEvents( &mixer, output, 256, 48000 );
	}
	assert( output[100][1] > 1174 && output[100][1] < 1180 );
	mixer.voices[talker].event = nullptr;
	--mixer.active;
	for ( int i = 0; i < 100; ++i ) {
		memset( output, 0, sizeof( output ) );
		S_MixEvents( &mixer, output, 256, 48000 );
	}
	assert( output[100][1] > 450 && output[100][1] < 500 );
	mixer = {};
	for ( int i = 0; i < 48000; ++i )
		sustained[i] = i % 2 ? 1000 : -1000;
	const int blocked = S_StartEventVoice( &mixer, &music, samples, spatial, false, 48000, .0875f );
	assert( blocked >= 0 );
	mixer.voices[blocked].occlusion = 1;
	for ( int i = 0; i < 40; ++i ) {
		memset( output, 0, sizeof( output ) );
		S_MixEvents( &mixer, output, 256, 48000 );
	}
	assert( std::fabs( output[100][1] ) < 40 ); // Shadow is frequency-dependent, not just quieter.
	mixer.voices[blocked].occlusion = 0;
	for ( int i = 0; i < 40; ++i ) {
		memset( output, 0, sizeof( output ) );
		S_MixEvents( &mixer, output, 256, 48000 );
	}
	assert( std::fabs( output[100][1] ) > 990 );
	// Streamed stereo music and decoded voice use the same buses, even with no event voices.
	static sBusFrame_t input[256];
	for ( auto &frame : input ) {
		frame.samples[S_BUS_MUSIC][0] = 1000;
		frame.samples[S_BUS_MUSIC][1] = -500;
		frame.samples[S_BUS_VOICE][0] = frame.samples[S_BUS_VOICE][1] = 100;
	}
	mixer = {};
	for ( int i = 0; i < 10; ++i ) {
		memset( output, 0, sizeof( output ) );
		S_MixEvents( &mixer, output, 256, 48000, nullptr, input );
	}
	assert( output[100][0] > 449 && output[100][0] < 452 );
	assert( output[100][1] < -74 && output[100][1] > -77 );
	mixer = {};
	mixer.busGain[S_BUS_VOICE] = 0;
	memset( output, 0, sizeof( output ) );
	S_MixEvents( &mixer, output, 256, 48000, nullptr, input );
	assert( output[100][0] == 1000 && output[100][1] == -500 && mixer.duck == 0 );
}

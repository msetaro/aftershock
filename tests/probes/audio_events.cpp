#include "../../engine/sound/snd_event.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <cmath>

int main( int argc, char **argv ) {
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
}

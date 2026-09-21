#include "../../engine/sound/snd_event.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

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
}

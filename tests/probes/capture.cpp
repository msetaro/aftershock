#include "../../engine/platform/sdl/sdl_snd.cpp"
#include <assert.h>

static cvar_t device;
cvar_t *Cvar_Get( const char *, const char *, int ) {
	return &device;
}
void QDECL Com_Printf( const char *, ... ) {
}

int main() {
	assert( SDL_setenv( "SDL_AUDIODRIVER", "dummy", 1 ) == 0 );
	assert( SDL_InitSubSystem( SDL_INIT_AUDIO ) == 0 );
	device.string = (char *)"";
	assert( !SNDDMA_StartVoiceCapture() );
	snd_inited = qtrue;
	assert( SNDDMA_StartVoiceCapture() );
	const auto deviceId = voiceDevice;
	assert( SNDDMA_StartVoiceCapture() && voiceDevice == deviceId );
	SDL_PauseAudioDevice( voiceDevice, 1 );
	voiceRead = voiceCount = 0;
	int16_t input[20000], output[2880];
	for ( int i = 0; i < 20000; ++i )
		input[i] = int16_t( i );
	VoiceCaptureCallback( nullptr, (Uint8 *)input, sizeof( input ) );
	assert( voiceCount == 8192 );
	assert( SNDDMA_ReadVoiceCapture( output, 2880 ) == 2880 );
	for ( int i = 0; i < 2880; ++i )
		assert( output[i] == 20000 - 8192 + i );
	assert( SNDDMA_ReadVoiceCapture( output, 0 ) == 0 );
	SNDDMA_StopVoiceCapture();
	assert( !voiceDevice && !voiceCount );
	assert( SNDDMA_ReadVoiceCapture( output, 2880 ) == 0 );
	assert( SNDDMA_StartVoiceCapture() );
	SNDDMA_StopVoiceCapture();
	SDL_QuitSubSystem( SDL_INIT_AUDIO );
	puts( "PASS: SDL dummy capture opens explicitly, caps its callback ring and closes/reopens cleanly" );
}

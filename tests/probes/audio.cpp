/* Type-check pthread callbacks and run both paths against ALSA null output. */
#include "../../engine/platform/unix/linux_snd.cpp"
#include <assert.h>

typedef void *( *thread_callback_t )( void * );
static const thread_callback_t callbacks[] = { thread_proc_mmap, thread_proc_direct };

dma_t dma;
static cvar_t device, rate;
cvar_t *Cvar_Get( const char *, const char *, int ) {
	return &device;
}
cvar_t *s_device = &device;
cvar_t *s_khz = &rate;
static unsigned int mmap_writes, direct_writes;

void QDECL Com_Printf( const char *fmt, ... ) {
	(void)fmt;
}
extern "C" snd_pcm_sframes_t __real_snd_pcm_mmap_commit( snd_pcm_t *, snd_pcm_uframes_t, snd_pcm_uframes_t );
extern "C" snd_pcm_sframes_t __wrap_snd_pcm_mmap_commit( snd_pcm_t *pcm, snd_pcm_uframes_t offset, snd_pcm_uframes_t frames ) {
	snd_pcm_sframes_t result = __real_snd_pcm_mmap_commit( pcm, offset, frames );
	if ( result > 0 )
		mmap_writes++;
	return result;
}
extern "C" snd_pcm_sframes_t __real_snd_pcm_writei( snd_pcm_t *, const void *, snd_pcm_uframes_t );
extern "C" snd_pcm_sframes_t __wrap_snd_pcm_writei( snd_pcm_t *pcm, const void *data, snd_pcm_uframes_t frames ) {
	snd_pcm_sframes_t result = __real_snd_pcm_writei( pcm, data, frames );
	if ( result > 0 )
		direct_writes++;
	return result;
}

int main( void ) {
	assert( callbacks[0] && callbacks[1] );
	device.string = (char *)"null";
	rate.integer = 22;
	assert( setup_ALSA( SND_MODE_MMAP ) );
	usleep( 50000 );
	assert( SNDDMA_GetDMAPos() >= 0 );
	SNDDMA_Shutdown();
	assert( mmap_writes > 0 );
	assert( setup_ALSA( SND_MODE_DIRECT ) );
	usleep( 50000 );
	assert( SNDDMA_GetDMAPos() >= 0 );
	SNDDMA_Shutdown();
	assert( direct_writes > 0 );
	assert( setup_ALSA( SND_MODE_DIRECT ) );
	assert( SNDDMA_StartVoiceCapture() );
	int16_t voice[960];
	assert( SNDDMA_ReadVoiceCapture( voice, 960 ) == 960 );
	SNDDMA_StopVoiceCapture();
	assert( SNDDMA_ReadVoiceCapture( voice, 960 ) == 0 );
	SNDDMA_Shutdown();
	puts( "PASS: ALSA null sink received MMAP and DIRECT samples; both threads joined; explicit null capture opened/read/closed" );
	return 0;
}

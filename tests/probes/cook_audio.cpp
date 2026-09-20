// Verify that cooked audio uses the existing production PCM loading path.
#include "../../engine/sound/snd_codec_wav.cpp"
#include <assert.h>

static FILE *input;
int FS_FOpenFileRead( const char *path, fileHandle_t *handle, qboolean ) {
	input = fopen( path, "rb" );
	assert( input );
	*handle = 1;
	return 1;
}
int FS_Read( void *buffer, int size, fileHandle_t ) {
	return (int)fread( buffer, 1, size, input );
}
int FS_Seek( fileHandle_t, fsOffset_t offset, fsOrigin_t origin ) {
	return fseek( input, offset, origin == FS_SEEK_CUR ? SEEK_CUR : SEEK_SET );
}
void FS_FCloseFile( fileHandle_t ) {
	fclose( input );
}
void *Hunk_AllocateTempMemory( size_t size ) {
	return malloc( size );
}
void Hunk_FreeTempMemory( void *pointer ) {
	free( pointer );
}
void QDECL Com_Printf( const char *, ... ) {
	abort();
}
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}
int main( int argc, char **argv ) {
	assert( argc == 3 );
	for ( int i = 1; i < argc; i++ ) {
		snd_info_t info = {};
		int16_t *samples = (int16_t *)S_WAV_CodecLoad( argv[i], &info );
		assert( samples && info.rate == 22050 && info.width == 2 && info.channels == 1 && info.samples == 1102 );
		int16_t high = 0, low = 0;
		for ( int sample = 0; sample < info.samples; sample++ ) {
			high = MAX( high, samples[sample] );
			low = MIN( low, samples[sample] );
		}
		assert( high > 7000 && low < -7000 );
		Hunk_FreeTempMemory( samples );
	}
	puts( "PASS: cooked WAV and Ogg tones load through the production PCM codec" );
}

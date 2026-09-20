// Offline Vorbis decoding uses the same vendored codec as the engine.
#include <vorbis/vorbisfile.h>
#include <stdint.h>
#include <stdio.h>

int main( int argc, char **argv ) {
	if ( argc != 3 )
		return 2;
	OggVorbis_File input;
	if ( ov_fopen( argv[1], &input ) )
		return 1;
	const vorbis_info *info = ov_info( &input, -1 );
	const ogg_int64_t frames = ov_pcm_total( &input, -1 );
	if ( !info || info->channels < 1 || info->channels > 2 || info->rate < 8000 || info->rate > 192000 || frames <= 0 || frames > ( 256 << 20 ) / ( info->channels * 2 ) ) {
		ov_clear( &input );
		return 1;
	}
	const int channels = info->channels;
	const int rate = (int)info->rate;
	FILE *output = fopen( argv[2], "wb" );
	if ( !output ) {
		ov_clear( &input );
		return 1;
	}
	uint32_t total = 0;
	bool valid = true;
	for ( ;; ) {
		char buffer[4096];
		int section = 0;
		const int count = (int)ov_read( &input, buffer, sizeof( buffer ), 0, 2, 1, &section );
		if ( !count )
			break;
		info = ov_info( &input, section );
		if ( count < 0 || !info || info->channels != channels || info->rate != rate || total + (uint32_t)count > (uint64_t)frames * channels * 2 || fwrite( buffer, 1, count, output ) != (size_t)count ) {
			valid = false;
			break;
		}
		total += count;
	}
	valid = fclose( output ) == 0 && valid && total == (uint64_t)frames * channels * 2;
	ov_clear( &input );
	if ( !valid )
		return 1;
	printf( "%d %d\n", rate, channels );
	return 0;
}

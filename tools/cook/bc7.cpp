// Offline block encoder. Input: consecutive 4x4 RGBA8 blocks; output: BC7 blocks.
#include "bc7enc.h"
#include <stdio.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

int main() {
#ifdef _WIN32
	_setmode( _fileno( stdin ), _O_BINARY );
	_setmode( _fileno( stdout ), _O_BINARY );
#endif
	bc7enc_compress_block_init();
	bc7enc_compress_block_params params;
	bc7enc_compress_block_params_init( &params );
	uint8_t pixels[64], block[16];
	for ( ;; ) {
		const size_t count = fread( pixels, 1, sizeof( pixels ), stdin );
		if ( !count )
			return ferror( stdin ) || fflush( stdout ) ? 1 : 0;
		if ( count != sizeof( pixels ) )
			return 1;
		bc7enc_compress_block( block, pixels, &params );
		if ( fwrite( block, 1, sizeof( block ), stdout ) != sizeof( block ) )
			return 1;
	}
}

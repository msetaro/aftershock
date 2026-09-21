// Offline simplifier bridge: bounded binary input/output, no engine dependency.
#include "../../third_party/meshoptimizer/src/meshoptimizer.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
int main() {
#ifdef _WIN32
	_setmode( _fileno( stdin ), _O_BINARY );
	_setmode( _fileno( stdout ), _O_BINARY );
#endif
	uint32_t header[3];
	float error;
	if ( fread( header, sizeof( header ), 1, stdin ) != 1 || fread( &error, sizeof( error ), 1, stdin ) != 1 ||
		 !header[0] || header[0] > 1000 || !header[1] || header[1] > 6000 || header[1] % 3 ||
		 !header[2] || header[2] > header[1] || header[2] % 3 || !std::isfinite( error ) || error < 0 || error > 1 )
		return 1;
	std::vector<float> vertices( size_t( header[0] ) * 12 );
	std::vector<unsigned char> locked( header[0] );
	std::vector<unsigned int> indices( header[1] ), output( header[1] );
	if ( fread( vertices.data(), sizeof( float ), vertices.size(), stdin ) != vertices.size() ||
		 fread( locked.data(), 1, locked.size(), stdin ) != locked.size() ||
		 fread( indices.data(), sizeof( unsigned int ), indices.size(), stdin ) != indices.size() || fgetc( stdin ) != EOF )
		return 1;
	for ( float value : vertices )
		if ( !std::isfinite( value ) )
			return 1;
	for ( unsigned int index : indices )
		if ( index >= header[0] )
			return 1;
	const float weights[9] = { .5f, .5f, .5f, 1, 1, 1, 1, 1, 1 };
	const size_t count = meshopt_simplifyWithAttributes( output.data(), indices.data(), indices.size(), vertices.data(), header[0], 12 * sizeof( float ),
		vertices.data() + 3, 12 * sizeof( float ), weights, 9, locked.data(), header[2], error, meshopt_SimplifyLockBorder, nullptr );
	const uint32_t size = (uint32_t)count;
	if ( !count || count > header[1] || count % 3 || fwrite( &size, sizeof( size ), 1, stdout ) != 1 ||
		 fwrite( output.data(), sizeof( unsigned int ), count, stdout ) != count || fflush( stdout ) )
		return 1;
}

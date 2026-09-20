// Consume the owned Blender export through the production IQM model/pose path.
#include "../../engine/render/tr_model_iqm.cpp"
#include <assert.h>

refimport_t ri;
trGlobals_t tr;
static void *allocation;
static uint32_t ownedAllocations, ownedPeak;
static void *allocateOwned( size_t size ) {
	ownedAllocations++;
	ownedPeak = MAX( ownedPeak, ownedAllocations );
	return calloc( 1, size );
}
static void releaseOwned( void *pointer ) {
	assert( pointer && ownedAllocations );
	ownedAllocations--;
	free( pointer );
}
static void *allocate( size_t size, ha_pref ) {
	assert( !allocation );
	allocation = calloc( 1, size );
	assert( allocation );
	return allocation;
}
static void QDECL print( printParm_t, const char *format, ... ) {
	va_list args;
	va_start( args, format );
	vfprintf( stderr, format, args );
	va_end( args );
}
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}
void QDECL Com_Printf( const char *, ... ) {
	abort();
}
shader_t *R_FindShader( const char *name, int lightmap, qboolean ) {
	static shader_t shader;
	assert( !strcmp( name, "models/character_material0" ) && lightmap == LIGHTMAP_NONE );
	return &shader;
}
int main( int argc, char **argv ) {
	assert( argc == 2 || argc == 3 );
	FILE *file = fopen( argv[1], "rb" );
	assert( file );
	assert( fseek( file, 0, SEEK_END ) == 0 );
	const fileOffset_t size = ftell( file );
	assert( size > 0 && size < 16 * 1024 * 1024 );
	rewind( file );
	void *bytes = malloc( (size_t)size );
	assert( bytes && fread( bytes, 1, (size_t)size, file ) == (size_t)size );
	fclose( file );
	ri.Hunk_Alloc = allocate;
	ri.Printf = print;
	model_t model = {};
	assert( R_LoadIQM( &model, bytes, (int)size, argv[1] ) );
	const iqmData_t *data = (iqmData_t *)model.modelData;
	assert( data == allocation && data->num_surfaces == 6 && data->num_joints == 3 && data->num_frames == 62 );
	for ( int frame = 0; frame < data->num_frames; frame++ ) {
		float matrices[IQM_MAX_JOINTS * 12];
		ComputePoseMats( (iqmData_t *)data, frame, frame, 0, matrices );
		for ( int vertex = 0; vertex < data->num_vertexes; vertex++ ) {
			const int influence = data->influences[vertex];
			float blended[12] = {};
			for ( int component = 0; component < 12; component++ ) {
				for ( int joint = 0; joint < 4; joint++ )
					blended[component] += data->influenceBlendWeights.f[influence * 4 + joint] * matrices[12 * data->influenceBlendIndexes[influence * 4 + joint] + component];
			}
			for ( int axis = 0; axis < 3; axis++ ) {
				const float value = DotProduct( blended + axis * 4, data->positions + vertex * 3 ) + blended[axis * 4 + 3];
				assert( value >= data->bounds[frame * 6 + axis] && value <= data->bounds[frame * 6 + axis + 3] );
			}
		}
	}
	if ( argc == 3 ) {
		free( allocation );
		free( bytes );
		puts( "PASS: native posed vertices stay inside cooked frame bounds at enlarged scale" );
		return 0;
	}
	assert( data->num_anims == 2 );
	assert( strcmp( data->animations[0].name, "idle" ) == 0 && strcmp( data->animations[1].name, "wave" ) == 0 );
	for ( uint32_t i = 0; i < 2; i++ ) {
		assert( data->animations[i].firstFrame == i * 31 && data->animations[i].frameCount == 31 );
		assert( data->animations[i].framesPerSecond == 30 && data->animations[i].flags == 0 );
	}
	orientation_t first, middle, waveStart, waveMiddle;
	assert( R_IQMLerpTag( &first, (iqmData_t *)model.modelData, 0, 0, 0, "root" ) );
	assert( R_IQMLerpTag( &middle, (iqmData_t *)model.modelData, 15, 15, 0, "root" ) );
	assert( fabsf( first.origin[1] ) < 0.001f && fabsf( middle.origin[1] + 1.28f ) < 0.001f );
	assert( R_IQMLerpTag( &waveStart, (iqmData_t *)model.modelData, 31, 31, 0, "arm.L" ) );
	assert( R_IQMLerpTag( &waveMiddle, (iqmData_t *)model.modelData, 46, 46, 0, "arm.L" ) );
	float difference = 0;
	for ( uint32_t r = 0; r < 3; ++r ) {
		for ( uint32_t c = 0; c < 3; ++c )
			difference += fabsf( waveStart.axis[r][c] - waveMiddle.axis[r][c] );
		assert( fabsf( waveStart.origin[r] - waveMiddle.origin[r] ) < 0.001f );
	}
	assert( difference > 0.5f );
	free( allocation );
	model.modelData = nullptr;
	ri.Malloc = allocateOwned;
	ri.Free = releaseOwned;
	for ( uint32_t i = 0; i < 12; i++ ) {
		assert( R_ReplaceIQM( &model, bytes, (int)size, argv[1] ) );
		assert( model.ownsData && ownedAllocations == 1 );
		assert( ( (iqmData_t *)model.modelData )->num_anims == 2 );
	}
	assert( ownedPeak == 2 );
	releaseOwned( model.modelData );
	assert( ownedAllocations == 0 );
	free( bytes );
	puts( "PASS: cooked Blender geometry and both clips consumed by production IQM poses" );
}

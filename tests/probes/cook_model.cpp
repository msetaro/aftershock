// Consume the owned Blender export through the production IQM model/pose path.
#include "../../engine/render/tr_model_iqm.cpp"
#include <assert.h>

refimport_t ri;
trGlobals_t tr;
static void *allocation;
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
	assert( argc == 2 );
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
	free( bytes );
	const iqmData_t *data = (iqmData_t *)model.modelData;
	assert( data == allocation && data->num_surfaces == 6 && data->num_joints == 3 && data->num_frames == 62 );
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
	puts( "PASS: cooked Blender geometry and both clips consumed by production IQM poses" );
}

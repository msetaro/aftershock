// Cooked LODs use the original skeleton and screen-size policy without frame allocations.
#include "../../engine/render/tr_model_iqm.cpp"
#include "../../engine/render/tr_mesh.cpp"
#include <assert.h>

refimport_t ri;
trGlobals_t tr;
cvar_t *r_lodscale, *r_lodbias;
static uint32_t allocations;
static int64_t clockTime() {
	static int64_t ticks;
	return ++ticks;
}
static void *allocate( size_t size, ha_pref ) {
	allocations++;
	return calloc( 1, size );
}
static void QDECL print( printParm_t, const char *, ... ) {
}
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}
void QDECL Com_Printf( const char *, ... ) {
	abort();
}
shader_t *R_FindShader( const char *, int, qboolean ) {
	static shader_t shader;
	return &shader;
}
static model_t load( const char *path ) {
	FILE *file = fopen( path, "rb" );
	assert( file && fseek( file, 0, SEEK_END ) == 0 );
	const fileOffset_t size = ftell( file );
	assert( size > 0 && size < 1024 * 1024 );
	rewind( file );
	void *bytes = malloc( (size_t)size );
	assert( bytes && fread( bytes, 1, (size_t)size, file ) == (size_t)size );
	fclose( file );
	model_t model{};
	assert( R_LoadIQM( &model, bytes, (int)size, path ) );
	free( bytes );
	return model;
}
int main( int argc, char **argv ) {
	assert( argc == 4 );
	ri.Hunk_Alloc = allocate;
	ri.Printf = print;
	ri.Microseconds = clockTime;
	model_t models[3] = { load( argv[1] ), load( argv[2] ), load( argv[3] ) };
	for ( int i = 1; i < 3; i++ ) {
		const iqmData_t *base = (iqmData_t *)models[0].modelData;
		iqmData_t *level = (iqmData_t *)models[i].modelData;
		assert( R_IQMLodCompatible( base, level ) );
		assert( level->num_triangles < base->num_triangles );
		for ( int frame = 0; frame < base->num_frames; frame++ ) {
			float original[IQM_MAX_JOINTS * 12]{}, reduced[IQM_MAX_JOINTS * 12]{};
			ComputePoseMats( (iqmData_t *)base, frame, frame, 0, original );
			ComputePoseMats( level, frame, frame, 0, reduced );
			assert( !memcmp( original, reduced, sizeof( original ) ) );
		}
		level->bindJoints[0] += 1;
		assert( !R_IQMLodCompatible( base, level ) );
		level->bindJoints[0] -= 1;
	}
	models[0].numLods = 3;
	tr.currentModel = &models[0];
	cvar_t scale{}, bias{};
	scale.value = 5;
	r_lodscale = &scale;
	r_lodbias = &bias;
	tr.viewParms.orientation.axis[0][0] = 1;
	tr.viewParms.projectionMatrix[5] = 1;
	tr.viewParms.projectionMatrix[11] = -1;
	trRefEntity_t entity{};
	AxisClear( entity.e.axis );
	entity.e.origin[0] = 1;
	assert( R_ComputeLOD( &entity ) == 0 );
	entity.e.origin[0] = 10000;
	assert( R_ComputeLOD( &entity ) == 2 );
	entity.e.nonNormalizedAxes = qtrue;
	VectorScale( entity.e.axis[0], 10000, entity.e.axis[0] );
	VectorScale( entity.e.axis[1], 10000, entity.e.axis[1] );
	VectorScale( entity.e.axis[2], 10000, entity.e.axis[2] );
	assert( R_ComputeLOD( &entity ) == 0 );
	entity.e.origin[0] = -1;
	assert( R_ComputeLOD( &entity ) == 0 );
	bias.integer = 99;
	assert( R_ComputeLOD( &entity ) == 2 );
	bias.integer = -99;
	assert( R_ComputeLOD( &entity ) == 0 );
	assert( allocations == 3 );
	assert( tr.lodCpuUsec == 6 );
	for ( model_t &model : models )
		free( model.modelData );
	puts( "PASS: native animated mesh LOD compatibility, projected size and scaled entities" );
}

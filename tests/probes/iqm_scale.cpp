// IQM joints apply local scale before rotation, followed by translation.
#include "../../engine/render/tr_model_iqm.cpp"
#include <assert.h>

refimport_t ri;
trGlobals_t tr;
static void *allocation;
static void *allocate( size_t size, ha_pref ) {
	assert( !allocation );
	allocation = calloc( 1, size );
	return allocation;
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

int main( int argc, char **argv ) {
	const vec3_t scales[] = { { 2, 3, 4 }, { -2, 3, 0.5f }, { 1, 1, 1 } };
	const vec3_t translation = { 5, 6, 7 }, vertex = { 1, 2, 3 };
	for ( const auto &scale : scales ) {
		for ( int axis = 0; axis < 3; ++axis ) {
			quat_t rotation = { 0, 0, 0, 0.7071067811865475f };
			rotation[axis] = rotation[3];
			float matrix[12], inverse[12], product[12];
			JointToMatrix( rotation, scale, translation, matrix );
			const vec3_t scaled = { scale[0] * vertex[0], scale[1] * vertex[1], scale[2] * vertex[2] };
			const vec3_t expected[] = {
				{ scaled[0], -scaled[2], scaled[1] },
				{ scaled[2], scaled[1], -scaled[0] },
				{ -scaled[1], scaled[0], scaled[2] }
			};
			for ( int row = 0; row < 3; ++row ) {
				const float actual = DotProduct( matrix + row * 4, vertex ) + matrix[row * 4 + 3];
				assert( fabsf( actual - expected[axis][row] - translation[row] ) < 0.00001f );
			}
			Matrix34Invert( matrix, inverse );
			Matrix34Multiply( matrix, inverse, product );
			for ( int component = 0; component < 12; ++component )
				assert( fabsf( product[component] - identityMatrix[component] ) < 0.00001f );
		}
	}
	puts( "PASS: IQM joint scale precedes rotation on all axes; inverse round trips" );
	if ( argc == 1 )
		return 0;
	assert( argc == 2 );
	FILE *file = fopen( argv[1], "rb" );
	assert( file && fseek( file, 0, SEEK_END ) == 0 );
	const fileOffset_t length = ftell( file );
	assert( length > 0 && length < 16 * 1024 * 1024 );
	rewind( file );
	void *bytes = malloc( (size_t)length );
	assert( bytes && fread( bytes, 1, (size_t)length, file ) == (size_t)length );
	fclose( file );
	ri.Hunk_Alloc = allocate;
	ri.Printf = print;
	model_t model = {};
	assert( R_LoadIQM( &model, bytes, (int)length, argv[1] ) );
	auto *data = (iqmData_t *)model.modelData;
	assert( data->num_joints == 2 && data->num_frames == 6 && data->num_vertexes == 3 );
	// glTF tip: T(0,1,0) Rz(angle) S(2,1,1), inverse bind T(0,-1,0).
	// Check the final wave pose after the cooker's (x,y,z)->(x,-z,y) basis.
	const vec3_t expected[] = { { 0, 0, 0 }, { -1, 0, 3 }, { -1, 0, -1 } };
	float matrices[IQM_MAX_JOINTS * 12];
	ComputePoseMats( data, 5, 5, 0, matrices );
	for ( int vertexIndex = 0; vertexIndex < 3; ++vertexIndex ) {
		const int influence = data->influences[vertexIndex];
		const int joint = data->influenceBlendIndexes[influence * 4];
		assert( data->influenceBlendWeights.f[influence * 4] == 1 );
		for ( int axis = 0; axis < 3; ++axis ) {
			const float *row = matrices + joint * 12 + axis * 4;
			const float actual = DotProduct( row, data->positions + vertexIndex * 3 ) + row[3];
			assert( fabsf( actual - expected[vertexIndex][axis] ) < 0.001f );
		}
	}
	free( allocation );
	free( bytes );
	puts( "PASS: cooked nonuniform glTF joint poses match analytical native vertex positions" );
}

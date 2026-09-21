// Previous rendered skin poses, including blended influences and legacy clips.
#include "../../engine/render/tr_model_iqm.cpp"
#include <assert.h>

int main() {
	float positions[] = { 1, 0, 0, 0, 1, 0, 1, 1, 0 };
	int influences[] = { 0, 0, 1 };
	byte joints[] = { 0, 1, 0, 0, 1, 0, 0, 0 };
	float weights[] = { .25f, .75f, 0, 0, 1, 0, 0, 0 };
	iqmData_t data{};
	data.num_joints = 2;
	data.num_vertexes = 3;
	data.positions = positions;
	data.influences = influences;
	data.influenceBlendIndexes = joints;
	data.influenceBlendWeights.f = weights;
	data.blendWeightsType = IQM_FLOAT;
	srfIQModel_t surface{};
	surface.data = &data;
	surface.num_vertexes = 3;
	surface.num_influences = 2;
	temporalEntity_t previous{};
	previous.hasPose = true;
	previous.pose.jointCount = 2;
	previous.entity.frame = previous.entity.oldframe = 999; // Saved skin is authoritative here.
	previous.entity.origin[0] = 1000; // Object/world transform is applied separately.
	float skin[] = { 1, 0, 0, 4, 0, 1, 0, 0, 0, 0, 1, 0,
		0, -1, 0, 0, 1, 0, 0, 2, 0, 0, 1, 0 };
	memcpy( previous.pose.skin, skin, sizeof( skin ) );
	vec4_t output[3]{};
	assert( !R_IQMPreviousPositions( &surface, &previous, output, 2 ) );
	assert( output[0][0] == 0 );
	assert( R_IQMPreviousPositions( &surface, &previous, output, 3 ) );
	const vec3_t expected[] = { { 1.25f, 2.25f, 0 }, { .25f, 1.75f, 0 }, { -1, 3, 0 } };
	for ( int i = 0; i < 3; ++i ) {
		for ( int axis = 0; axis < 3; ++axis )
			assert( output[i][axis] == expected[i][axis] );
		assert( output[i][3] == 1 );
	}
	byte byteWeights[] = { 64, 191, 0, 0, 255, 0, 0, 0 };
	data.blendWeightsType = IQM_UBYTE;
	data.influenceBlendWeights.b = byteWeights;
	assert( R_IQMPreviousPositions( &surface, &previous, output, 3 ) );
	assert( fabsf( output[0][0] - 5 * ( 64.0f / 255 ) ) < .000001f );
	assert( fabsf( output[0][1] - 3 * ( 191.0f / 255 ) ) < .000001f );
	previous.pose.jointCount = 1;
	assert( !R_IQMPreviousPositions( &surface, &previous, output, 3 ) );
	previous.hasPose = false;
	assert( R_IQMPreviousPositions( &surface, &previous, output, 3 ) );
	for ( int i = 0; i < 3; ++i )
		assert( !memcmp( output[i], positions + i * 3, sizeof( vec3_t ) ) );
	data.blendWeightsType = IQM_FLOAT;
	data.influenceBlendWeights.f = weights;
	iqmTransform_t poses[4]{};
	for ( auto &pose : poses ) {
		pose.rotate[3] = 1;
		pose.scale[0] = pose.scale[1] = pose.scale[2] = 1;
	}
	poses[2].translate[0] = 8;
	poses[3].translate[0] = 4;
	float inverse[] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0,
		1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0 };
	int parents[] = { -1, -1 };
	data.poses = poses;
	data.num_frames = data.num_poses = 2;
	data.jointParents = parents;
	data.invBindJoints = data.bindJoints = inverse;
	previous.entity.frame = 1;
	previous.entity.oldframe = 0;
	previous.entity.backlerp = .25f;
	assert( R_IQMPreviousPositions( &surface, &previous, output, 3 ) );
	assert( output[0][0] == 4.75f && output[1][0] == 3.75f && output[2][0] == 4 );
	assert( output[0][1] == 0 && output[1][1] == 1 && output[2][1] == 1 );
}

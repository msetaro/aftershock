// Render submission owns a bounded copy and validates the graph/model revision.
#include "../../engine/animation/animation_public.h"
#include "../../engine/render/tr_scene.cpp"
#include "../../engine/render/tr_model_iqm.cpp"
#include <assert.h>

refimport_t ri;
trGlobals_t tr;
static backEndData_t storage;
backEndData_t *backEndData = &storage;
static model_t model;
model_t *R_GetModelByHandle( qhandle_t handle ) {
	assert( handle == 1 );
	return &model;
}
static void QDECL print( printParm_t, const char *, ... ) {
}
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}
void QDECL Com_Printf( const char *, ... ) {
	abort();
}
int main() {
	ri.Printf = print;
	ri.Error = Com_Error;
	tr.registered = qtrue;
	float inverse[24] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0,
		1, 0, 0, 0, 0, 1, 0, -1, 0, 0, 1, 0 };
	iqmData_t data = {};
	data.num_joints = 2;
	data.invBindJoints = inverse;
	for ( int axis = 0; axis < 3; ++axis ) {
		data.bindBounds[0][axis] = -1;
		data.bindBounds[1][axis] = 1;
	}
	model.type = MOD_IQM;
	model.modelData = &data;
	model.cookedHash[0] = 42;
	animPose_t pose = {};
	pose.jointCount = 2;
	for ( uint32_t joint = 0; joint < 2; ++joint ) {
		pose.world[joint][0] = pose.world[joint][5] = pose.world[joint][10] = 1;
		pose.world[joint][3] = 7;
	}
	pose.world[1][7] = 2;
	refEntity_t entity = {};
	entity.reType = RT_MODEL;
	entity.hModel = 1;
	entity.axis[0][0] = entity.axis[1][1] = entity.axis[2][2] = 1;
	uint8_t hash[32] = {};
	R_InitNextFrame();
	assert( !RE_AddSkeletalEntityToScene( &entity, &pose, hash, qfalse ) );
	assert( r_numentities == 0 );
	hash[0] = 42;
	assert( RE_AddSkeletalEntityToScene( &entity, &pose, hash, qfalse ) );
	assert( r_numentities == 1 );
	const auto *copied = storage.entities[0].skeletalPose;
	assert( copied && copied->skin[0][3] == 7 && copied->skin[1][7] == 1 );
	assert( copied->bounds[0][0] <= 6 && copied->bounds[1][0] >= 8 );
	assert( copied->bounds[0][1] <= -1 && copied->bounds[1][1] >= 2 );
	pose.world[0][3] = 900;
	assert( copied->skin[0][3] == 7 );
	for ( uint32_t i = 1; i < MAX_SKELETAL_POSES; ++i )
		assert( RE_AddSkeletalEntityToScene( &entity, &pose, hash, qfalse ) );
	assert( !RE_AddSkeletalEntityToScene( &entity, &pose, hash, qfalse ) );
	assert( r_numentities == MAX_SKELETAL_POSES );
	R_InitNextFrame();
	RE_AddRefEntityToScene( &entity, qfalse );
	assert( r_numentities == 1 && storage.entities[0].skeletalPose == nullptr );
	assert( RE_AddSkeletalEntityToScene( &entity, &pose, hash, qfalse ) );
	puts( "PASS: model-bound poses are copied, bounded and reset between renderer frames" );
}

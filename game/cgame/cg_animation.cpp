#include "cg_local.h"

static struct {
	animAsset_t asset;
	void *storage;
	qhandle_t model;
} animationRigs[2];
static struct {
	bool valid;
	int entity;
	animState_t state;
	float parameters[ANIM_MAX_PARAMETERS], origin[3], angles[3];
} animationActors[MAX_CLIENTS][2];
static vmCvar_t animationTrace, animationFov;
static uint32_t bodyDraws, rifleDraws;

void CG_ShutdownAnimation( void ) {
	for ( auto &rig : animationRigs ) {
		Anim_FreeFile( rig.storage );
		rig = {};
	}
	memset( animationActors, 0, sizeof( animationActors ) );
}
void CG_InitAnimation( void ) {
	CG_ShutdownAnimation();
	bodyDraws = rifleDraws = 0;
	trap_Cvar_Register( &animationTrace, "cg_animationTrace", "0", 0 );
	trap_Cvar_Register( &animationFov, "cg_animationFov", "70", CVAR_ARCHIVE );
	for ( int i = 0; i < 2; ++i ) {
		char text[MAX_QPATH + 66];
		Q_strncpyz( text, CG_ConfigString( CS_ANIMATION_BODY + i ), sizeof( text ) );
		char *config = text;
		if ( !config[0] )
			continue;
		char path[MAX_QPATH], expected[65], actual[65];
		Q_strncpyz( path, COM_Parse( &config ), sizeof( path ) );
		Q_strncpyz( expected, COM_Parse( &config ), sizeof( expected ) );
		auto &rig = animationRigs[i];
		rig.storage = Anim_LoadFile( path, &rig.asset );
		if ( !rig.storage )
			CG_Error( "Animation rejected: cannot load %s", path );
		Anim_HashString( rig.asset.hash, actual );
		if ( strcmp( actual, expected ) )
			CG_Error( "Animation rejected: server graph revision differs for %s", path );
		rig.model = trap_R_RegisterModel( rig.asset.header.model );
		if ( !rig.model )
			CG_Error( "Animation rejected: model %s", rig.asset.header.model );
	}
}
void CG_AnimationSnapshot( const entityState_t *entity ) {
	animState_t state;
	float parameters[ANIM_MAX_PARAMETERS];
	if ( !BG_EntityStateToAnimation( entity, &state, parameters ) )
		CG_Error( "Animation rejected: snapshot record" );
	const int owner = entity->otherEntityNum, rig = entity->otherEntityNum2;
	if ( !animationRigs[rig].storage )
		CG_Error( "Animation rejected: snapshot has no matching graph" );
	auto &actor = animationActors[owner][rig];
	actor.valid = true;
	actor.entity = entity->number;
	actor.state = state;
	memcpy( actor.parameters, parameters, sizeof( parameters ) );
	VectorCopy( entity->origin, actor.origin );
	VectorCopy( entity->angles, actor.angles );
	trap_Cvar_Update( &animationTrace );
	if ( rig == 0 && animationTrace.integer ) {
		animPose_t pose;
		animBox_t boxes[ANIM_MAX_BOXES];
		vec3_t axis[3];
		AnglesToAxis( entity->angles, axis );
		const auto *asset = &animationRigs[0].asset;
		if ( !Anim_Evaluate( asset, &state, parameters, state.lastTime, &pose ) || !Anim_RemoveRootTranslation( asset, &pose ) )
			CG_Error( "Animation rejected: fixed client pose" );
		const uint32_t count = Anim_HitBoxes( asset, &pose, entity->origin, axis, boxes, ANIM_MAX_BOXES );
		if ( !count )
			CG_Error( "Animation rejected: client hit boxes" );
		char hash[65];
		Anim_BoxHash( boxes, count, hash );
		CG_Printf( "Animation client boxes: tick=%u owner=%d hash=%s\n", state.lastTime, owner, hash );
	}
}
static bool AnimationPose( int owner, int rig, animPose_t *pose ) {
	if ( owner < 0 || owner >= MAX_CLIENTS || !animationRigs[rig].storage )
		return false;
	const auto &actor = animationActors[owner][rig];
	if ( !actor.valid || !cg_entities[actor.entity].currentValid )
		return false;
	// Cosmetic sampling may move ahead inside the current state. Only snapshots
	// replace gameplay state; rendering never ticks transitions or delivers events.
	const uint32_t elapsed = uint32_t( cg.time ) - actor.state.lastTime;
	const uint32_t time = actor.state.lastTime + ( elapsed < 100 ? elapsed : 0 );
	return Anim_Evaluate( &animationRigs[rig].asset, &actor.state, actor.parameters, time, pose );
}
bool CG_AnimationPlayer( centity_t *cent ) {
	const int owner = cent->currentState.clientNum;
	if ( cent->currentState.number != owner )
		return false; // Legacy corpse presentation.
	animPose_t pose;
	if ( !AnimationPose( owner, 0, &pose ) )
		return false;
	Anim_RemoveRootTranslation( &animationRigs[0].asset, &pose );
	refEntity_t entity = {};
	entity.reType = RT_MODEL;
	entity.hModel = animationRigs[0].model;
	VectorCopy( cent->lerpOrigin, entity.origin );
	entity.origin[2] += MINS_Z;
	VectorCopy( entity.origin, entity.oldorigin );
	VectorCopy( cent->lerpOrigin, entity.lightingOrigin );
	vec3_t angles = { 0, cent->lerpAngles[YAW], 0 };
	AnglesToAxis( angles, entity.axis );
	entity.renderfx = RF_LIGHTING_ORIGIN;
	if ( owner == cg.snap->ps.clientNum && !cg.renderingThirdPerson )
		entity.renderfx |= RF_THIRD_PERSON;
	memset( entity.shaderRGBA, 255, sizeof( entity.shaderRGBA ) );
	if ( !CGameImport_R_AddSkeletalEntityToScene( &entity, &pose, animationRigs[0].asset.header.modelHash ) )
		CG_Error( "Animation rejected: body render binding" );
	++bodyDraws;
	return true;
}
bool CG_AnimationViewWeapon( const playerState_t *ps, const vec3_t origin, const vec3_t angles ) {
	animPose_t pose;
	if ( !AnimationPose( ps->clientNum, 1, &pose ) )
		return false;
	trap_Cvar_Update( &animationFov );
	refEntity_t entity = {};
	entity.reType = RT_MODEL;
	entity.hModel = animationRigs[1].model;
	AnglesToAxis( angles, entity.axis );
	VectorCopy( origin, entity.origin );
	VectorMA( entity.origin, 12, entity.axis[0], entity.origin );
	VectorMA( entity.origin, -5, entity.axis[1], entity.origin );
	VectorMA( entity.origin, -9, entity.axis[2], entity.origin );
	VectorCopy( entity.origin, entity.oldorigin );
	VectorCopy( origin, entity.lightingOrigin );
	const float fov = fminf( fmaxf( animationFov.value, 30 ), 120 );
	const float forwardScale = tanf( fov * float( M_PI ) / 360 ) / tanf( cg.refdef.fov_x * float( M_PI ) / 360 );
	VectorScale( entity.axis[0], forwardScale, entity.axis[0] );
	entity.nonNormalizedAxes = qtrue;
	entity.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;
	memset( entity.shaderRGBA, 255, sizeof( entity.shaderRGBA ) );
	if ( !CGameImport_R_AddSkeletalEntityToScene( &entity, &pose, animationRigs[1].asset.header.modelHash ) )
		CG_Error( "Animation rejected: rifle render binding" );
	++rifleDraws;
	return true;
}
void CG_AnimationStatus( void ) {
	CG_Printf( "Animation rendering: body=%u rifle=%u\n", bodyDraws, rifleDraws );
}

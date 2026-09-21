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
static vmCvar_t animationTrace, animationFov, animationSway;
static vec3_t adsOptic, lastViewAngles, swayAngles;
static uint32_t adsSamples;
static float adsError;
static bool haveViewAngles;
static uint32_t bodyDraws, rifleDraws;
static struct {
	uint32_t handle;
	int owner, expires;
	uint32_t serial;
	bool active;
} ragdolls[4];
static uint32_t ragdollCount, ragdollNext, ragdollSpawns, ragdollDraws;
static animPose_t lastBodyPose[MAX_CLIENTS];
static bool haveBodyPose[MAX_CLIENTS], deadSeen[MAX_CLIENTS];
static int deathTeleport[MAX_CLIENTS];

void CG_ShutdownAnimation( void ) {
	memset( ragdolls, 0, sizeof( ragdolls ) );
	memset( haveBodyPose, 0, sizeof( haveBodyPose ) );
	memset( deadSeen, 0, sizeof( deadSeen ) );
	memset( deathTeleport, 0, sizeof( deathTeleport ) );
	ragdollCount = ragdollNext = ragdollSpawns = ragdollDraws = 0;
	for ( auto &rig : animationRigs ) {
		Anim_FreeFile( rig.storage );
		rig = {};
	}
	memset( animationActors, 0, sizeof( animationActors ) );
}
void CG_InitAnimation( void ) {
	CG_ShutdownAnimation();
	bodyDraws = rifleDraws = adsSamples = 0;
	adsError = 0;
	haveViewAngles = false;
	VectorClear( swayAngles );
	trap_Cvar_Register( &animationTrace, "cg_animationTrace", "0", 0 );
	trap_Cvar_Register( &animationFov, "cg_animationFov", "70", CVAR_ARCHIVE );
	trap_Cvar_Register( &animationSway, "cg_animationSway", "0.15", CVAR_ARCHIVE );
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
	if ( animationRigs[1].storage ) {
		const auto *asset = &animationRigs[1].asset;
		animState_t state;
		animPose_t pose;
		float parameters[ANIM_MAX_PARAMETERS];
		Anim_Reset( asset, 0, &state );
		Anim_DefaultParameters( asset, parameters );
		bool found = false;
		for ( uint32_t i = 0; i < asset->header.sections[ANIM_STATES].count; ++i )
			if ( !strcmp( Anim_StateName( asset, i ), "ads" ) ) {
				state.current = state.previous = i;
				found = true;
			}
		const int optic = Anim_BoneIndex( asset, "optic" );
		if ( !found || optic < 0 || !Anim_Evaluate( asset, &state, parameters, 0, &pose ) )
			CG_Error( "Animation rejected: ADS optic required" );
		for ( int i = 0; i < 3; ++i )
			adsOptic[i] = pose.world[optic][i * 4 + 3];
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
	trap_Cvar_Update( &animationTrace );
	if ( animationTrace.integer && ( !actor.valid || actor.state.current != state.current ) )
		CG_Printf( "Animation client state: owner=%d rig=%d state=%s\n", owner, rig, Anim_StateName( &animationRigs[rig].asset, state.current ) );
	actor.valid = true;
	actor.entity = entity->number;
	actor.state = state;
	memcpy( actor.parameters, parameters, sizeof( parameters ) );
	VectorCopy( entity->origin, actor.origin );
	VectorCopy( entity->angles, actor.angles );
	if ( rig == 0 && animationTrace.integer ) {
		animPose_t pose;
		animBox_t boxes[ANIM_MAX_BOXES];
		vec3_t axis[3];
		AnglesToAxis( entity->angles, axis );
		const auto *asset = &animationRigs[0].asset;
		if ( !BG_AnimationPose( asset, &state, parameters, state.lastTime, 0, &pose ) )
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
	if ( !actor.valid )
		return false;
	const auto &snapshot = cg_entities[actor.entity];
	if ( !snapshot.currentValid || snapshot.currentState.eType != ET_ANIMATION || snapshot.currentState.otherEntityNum != owner || snapshot.currentState.otherEntityNum2 != rig )
		return false;
	// Cosmetic sampling may move ahead inside the current state. Only snapshots
	// replace gameplay state; rendering never ticks transitions or delivers events.
	const uint32_t elapsed = uint32_t( cg.time ) - actor.state.lastTime;
	const uint32_t time = actor.state.lastTime + ( elapsed < 100 ? elapsed : 0 );
	return BG_AnimationPose( &animationRigs[rig].asset, &actor.state, actor.parameters, time, rig, pose );
}
void CG_PrepareRagdolls() {
	if ( !animationRigs[0].storage )
		return;
	for ( auto &ragdoll : ragdolls ) {
		ragdoll.handle = Phys_PrepareRagdoll( &animationRigs[0].asset );
		if ( ragdoll.handle == PHYS_INVALID_BODY )
			break;
		++ragdollCount;
	}
	CG_Printf( "Physics ragdolls prepared: %u\n", ragdollCount );
}
void CG_ClearRagdolls() {
	for ( auto &ragdoll : ragdolls ) {
		if ( ragdoll.active )
			Phys_DespawnRagdoll( ragdoll.handle );
		ragdoll.active = false;
	}
	memset( deadSeen, 0, sizeof( deadSeen ) );
	memset( haveBodyPose, 0, sizeof( haveBodyPose ) );
}
static bool RagdollOwner( int owner ) {
	for ( const auto &ragdoll : ragdolls )
		if ( ragdoll.active && ragdoll.owner == owner )
			return true;
	return false;
}
static bool DeathRagdoll( centity_t *cent, int owner ) {
	if ( !ragdollCount || !haveBodyPose[owner] )
		return false;
	const int teleport = cent->currentState.eFlags & EF_TELEPORT_BIT;
	if ( deadSeen[owner] && deathTeleport[owner] == teleport )
		return RagdollOwner( owner );
	deadSeen[owner] = true;
	deathTeleport[owner] = teleport;
	auto &ragdoll = ragdolls[ragdollNext++ % ragdollCount];
	if ( ragdoll.active )
		Phys_DespawnRagdoll( ragdoll.handle );
	vec3_t origin, axis[3], angles = { 0, animationActors[owner][0].angles[YAW], 0 };
	VectorCopy( cent->lerpOrigin, origin );
	origin[2] += MINS_Z;
	AnglesToAxis( angles, axis );
	ragdoll.active = Phys_SpawnRagdoll( ragdoll.handle, &lastBodyPose[owner], origin, axis, cent->currentState.pos.trDelta );
	if ( !ragdoll.active )
		return false;
	ragdoll.owner = owner;
	ragdoll.expires = cg.time + 15000;
	ragdoll.serial = ++ragdollSpawns;
	CG_Printf( "Physics ragdoll death: owner=%d serial=%u joints=%u\n", owner, ragdoll.serial, lastBodyPose[owner].jointCount );
	return true;
}
void CG_RagdollStatus() {
	uint32_t active = 0;
	for ( const auto &ragdoll : ragdolls )
		active += ragdoll.active;
	CG_Printf( "Physics ragdoll status: prepared=%u spawned=%u active=%u draws=%u\n", ragdollCount, ragdollSpawns, active, ragdollDraws );
}
void CG_AddRagdolls() {
	for ( auto &ragdoll : ragdolls ) {
		if ( !ragdoll.active )
			continue;
		if ( cg.time >= ragdoll.expires ) {
			Phys_DespawnRagdoll( ragdoll.handle );
			ragdoll.active = false;
			continue;
		}
		animPose_t pose;
		refEntity_t entity{};
		if ( !Phys_RagdollPose( ragdoll.handle, &pose, entity.origin ) )
			continue;
		entity.reType = RT_MODEL;
		entity.hModel = animationRigs[0].model;
		AxisClear( entity.axis );
		VectorCopy( entity.origin, entity.oldorigin );
		VectorCopy( entity.origin, entity.lightingOrigin );
		entity.renderfx = RF_LIGHTING_ORIGIN;
		memset( entity.shaderRGBA, 255, sizeof( entity.shaderRGBA ) );
		const uint64_t identity = UINT64_C( 0xc000000000000000 ) | ( uint64_t( ragdoll.serial ) << 16 ) | ragdoll.handle;
		if ( !CGameImport_R_AddTemporalEntityToScene( &entity, identity, nullptr, &pose, animationRigs[0].asset.header.modelHash ) )
			CG_Error( "Cosmetic ragdoll render binding rejected" );
		++ragdollDraws;
#ifdef AFTERSHOCK_DEVTOOLS
		if ( CG_PhysicsDebugEnabled() ) {
			animBox_t boxes[ANIM_MAX_BOXES];
			const uint32_t count = Anim_HitBoxes( &animationRigs[0].asset, &pose, entity.origin, entity.axis, boxes, ANIM_MAX_BOXES );
			for ( uint32_t i = 0; i < count; ++i )
				Dev_DrawBox( boxes[i].mins, boxes[i].maxs, 0xff00ffffU, 0 );
			Dev_DrawText( entity.origin, "Cosmetic ragdoll", 0xff00ffffU, 0 );
		}
#endif
	}
}
bool CG_AnimationPlayer( centity_t *cent ) {
	const int owner = cent->currentState.clientNum;
	if ( owner < 0 || owner >= MAX_CLIENTS )
		return false;
	if ( cent->currentState.number != owner )
		return ( cent->currentState.eFlags & EF_DEAD ) && RagdollOwner( owner );
	if ( cent->currentState.eFlags & EF_DEAD )
		return DeathRagdoll( cent, owner );
	deadSeen[owner] = false;
	animPose_t pose;
	if ( !AnimationPose( owner, 0, &pose ) )
		return false;
	lastBodyPose[owner] = pose;
	haveBodyPose[owner] = true;
	refEntity_t entity = {};
	entity.reType = RT_MODEL;
	entity.hModel = animationRigs[0].model;
	VectorCopy( cent->lerpOrigin, entity.origin );
	entity.origin[2] += MINS_Z;
	VectorCopy( entity.origin, entity.oldorigin );
	VectorCopy( cent->lerpOrigin, entity.lightingOrigin );
	const auto &actor = animationActors[owner][0];
	const auto &snapshot = cg_entities[actor.entity];
	float yaw = actor.angles[YAW];
	if ( snapshot.interpolate && snapshot.nextState.eType == ET_ANIMATION )
		yaw = LerpAngle( yaw, snapshot.nextState.angles[YAW], cg.frameInterpolation );
	vec3_t angles = { 0, yaw, 0 };
	AnglesToAxis( angles, entity.axis );
	entity.renderfx = RF_LIGHTING_ORIGIN;
	if ( owner == cg.snap->ps.clientNum && !cg.renderingThirdPerson )
		entity.renderfx |= RF_THIRD_PERSON;
	memset( entity.shaderRGBA, 255, sizeof( entity.shaderRGBA ) );
	const uint64_t identity = ( (uint64_t)( owner + 1 ) << 32 ) | 0x80000000u | (uint32_t)( cent->currentState.eFlags & EF_TELEPORT_BIT );
	if ( !CGameImport_R_AddTemporalEntityToScene( &entity, identity, nullptr, &pose, animationRigs[0].asset.header.modelHash ) )
		CG_Error( "Animation rejected: body render binding" );
	++bodyDraws;
	return true;
}
bool CG_AnimationViewWeapon( const playerState_t *ps, const vec3_t origin, const vec3_t angles ) {
	animPose_t pose;
	if ( !AnimationPose( ps->clientNum, 1, &pose ) )
		return false;
	trap_Cvar_Update( &animationFov );
	trap_Cvar_Update( &animationSway );
	const auto *asset = &animationRigs[1].asset;
	const auto &actor = animationActors[ps->clientNum][1];
	const auto &state = actor.state;
	const int adsParameter = Anim_ParameterIndex( asset, "ads" );
	auto sightWeight = [&]( uint32_t index ) {
		const char *name = Anim_StateName( asset, index );
		return !strcmp( name, "ads" ) || ( !strcmp( name, "fire" ) && adsParameter >= 0 && actor.parameters[adsParameter] > 0.5f ) ? 1.0f : 0.0f;
	};
	const uint32_t elapsed = uint32_t( cg.time ) - state.blendStarted;
	const float blend = state.blendDuration ? fminf( float( elapsed ) / float( state.blendDuration ), 1 ) : 1;
	const float ads = sightWeight( state.previous ) * ( 1 - blend ) + sightWeight( state.current ) * blend;
	vec3_t weaponAngles;
	const float smoothing = fminf( fmaxf( float( cg.frametime ) / 80, 0 ), 1 );
	for ( int i = 0; i < 3; ++i ) {
		const float delta = haveViewAngles ? AngleSubtract( cg.refdefViewAngles[i], lastViewAngles[i] ) : 0;
		const float target = fminf( fmaxf( delta, -8 ), 8 );
		swayAngles[i] += smoothing * ( target - swayAngles[i] );
		weaponAngles[i] = LerpAngle( angles[i], cg.refdefViewAngles[i], ads ) - ( 1 - ads ) * swayAngles[i] * fminf( fmaxf( animationSway.value, 0 ), 1 );
		lastViewAngles[i] = cg.refdefViewAngles[i];
	}
	haveViewAngles = true;
	refEntity_t entity = {};
	entity.reType = RT_MODEL;
	entity.hModel = animationRigs[1].model;
	AnglesToAxis( weaponAngles, entity.axis );
	for ( int i = 0; i < 3; ++i )
		entity.origin[i] = origin[i] * ( 1 - ads ) + cg.refdef.vieworg[i] * ads;
	VectorMA( entity.origin, 12, entity.axis[0], entity.origin );
	VectorMA( entity.origin, -5 * ( 1 - ads ) - adsOptic[1] * ads, entity.axis[1], entity.origin );
	VectorMA( entity.origin, -9 * ( 1 - ads ) - adsOptic[2] * ads, entity.axis[2], entity.origin );
	VectorCopy( entity.origin, entity.oldorigin );
	VectorCopy( origin, entity.lightingOrigin );
	const float fov = fminf( fmaxf( animationFov.value, 30 ), 120 );
	const float forwardScale = tanf( fov * float( M_PI ) / 360 ) / tanf( cg.refdef.fov_x * float( M_PI ) / 360 );
	VectorScale( entity.axis[0], forwardScale, entity.axis[0] );
	entity.nonNormalizedAxes = qtrue;
	entity.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;
	memset( entity.shaderRGBA, 255, sizeof( entity.shaderRGBA ) );
	const uint64_t identity = ( (uint64_t)( ps->clientNum + 1 ) << 32 ) | 0x80010000u | (uint32_t)( ps->eFlags & EF_TELEPORT_BIT );
	if ( !CGameImport_R_AddTemporalEntityToScene( &entity, identity, nullptr, &pose, animationRigs[1].asset.header.modelHash ) )
		CG_Error( "Animation rejected: rifle render binding" );
	if ( ads == 1 && !strcmp( Anim_StateName( asset, state.current ), "ads" ) && blend == 1 ) {
		const int optic = Anim_BoneIndex( asset, "optic" );
		vec3_t position, relative;
		VectorCopy( entity.origin, position );
		for ( int i = 0; i < 3; ++i )
			VectorMA( position, pose.world[optic][i * 4 + 3], entity.axis[i], position );
		VectorSubtract( position, cg.refdef.vieworg, relative );
		const float horizontal = DotProduct( relative, cg.refdef.viewaxis[1] ), vertical = DotProduct( relative, cg.refdef.viewaxis[2] );
		adsError = fmaxf( adsError, sqrtf( horizontal * horizontal + vertical * vertical ) );
		++adsSamples;
	}
	++rifleDraws;
	return true;
}
void CG_AnimationStatus( void ) {
	CG_Printf( "Animation rendering: body=%u rifle=%u\n", bodyDraws, rifleDraws );
	CG_Printf( "Animation ADS: samples=%u max_error=%.6f\n", adsSamples, double( adsError ) );
}

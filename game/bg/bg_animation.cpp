#include "q_shared.h"
#include "../../engine/public/bg_public.h"

// Auxiliary entities have their own interpretation of existing snapshot fields.
// Keep solid/loopSound/event zero: they are state records, not world geometry.
bool BG_AnimationToEntityState( const animState_t *state, const float *parameters, int owner, int rig, const float *origin, const float *angles, entityState_t *entity ) {
	if ( owner < 0 || owner >= MAX_CLIENTS || rig < 0 || rig > 1 || state->current >= ANIM_MAX_STATES || state->previous >= ANIM_MAX_STATES || state->blendDuration > 60000 || state->initialized > 1 )
		return false;
	for ( uint32_t i = 0; i < ANIM_MAX_PARAMETERS; ++i )
		if ( !isfinite( parameters[i] ) )
			return false;
	for ( uint32_t i = 0; i < 3; ++i )
		if ( !isfinite( origin[i] ) || !isfinite( angles[i] ) )
			return false;
	const int number = entity->number;
	*entity = {};
	entity->number = number;
	entity->eType = ET_ANIMATION;
	entity->clientNum = 1; // Encoding version, unrelated to the owning player.
	entity->otherEntityNum = owner;
	entity->otherEntityNum2 = rig;
	entity->frame = int32_t( state->current | ( state->previous << 8 ) );
	entity->time = int32_t( state->entered );
	entity->time2 = int32_t( state->previousEntered );
	entity->pos.trTime = int32_t( state->blendStarted );
	entity->pos.trDuration = int32_t( state->blendDuration );
	entity->apos.trTime = int32_t( state->lastTime );
	entity->apos.trDuration = int32_t( state->eventSequence );
	entity->eFlags = int32_t( state->initialized );
	float *groups[] = { entity->pos.trBase, entity->pos.trDelta, entity->apos.trBase, entity->apos.trDelta, entity->origin2, entity->angles2 };
	for ( uint32_t i = 0; i < ANIM_MAX_PARAMETERS; ++i )
		groups[i / 3][i % 3] = parameters[i] == 0 ? 0 : parameters[i];
	for ( uint32_t i = 0; i < 3; ++i ) {
		entity->origin[i] = origin[i] == 0 ? 0 : origin[i];
		entity->angles[i] = angles[i] == 0 ? 0 : angles[i];
	}
	return true;
}

bool BG_EntityStateToAnimation( const entityState_t *entity, animState_t *state, float *parameters ) {
	if ( entity->eType != ET_ANIMATION || entity->clientNum != 1 || entity->otherEntityNum < 0 || entity->otherEntityNum >= MAX_CLIENTS || entity->otherEntityNum2 < 0 || entity->otherEntityNum2 > 1 || entity->eFlags < 0 || entity->eFlags > 1 || entity->frame < 0 || entity->frame > 65535 || entity->pos.trDuration < 0 || entity->pos.trDuration > 60000 )
		return false;
	const uint32_t current = uint32_t( entity->frame ) & 255, previous = uint32_t( entity->frame ) >> 8;
	if ( current >= ANIM_MAX_STATES || previous >= ANIM_MAX_STATES )
		return false;
	const float *groups[] = { entity->pos.trBase, entity->pos.trDelta, entity->apos.trBase, entity->apos.trDelta, entity->origin2, entity->angles2 };
	for ( uint32_t i = 0; i < ANIM_MAX_PARAMETERS; ++i )
		if ( !isfinite( groups[i / 3][i % 3] ) )
			return false;
	for ( uint32_t i = 0; i < 3; ++i )
		if ( !isfinite( entity->origin[i] ) || !isfinite( entity->angles[i] ) )
			return false;
	*state = { current, previous, uint32_t( entity->time ), uint32_t( entity->time2 ), uint32_t( entity->pos.trTime ), uint32_t( entity->pos.trDuration ), uint32_t( entity->apos.trTime ), uint32_t( entity->apos.trDuration ), uint32_t( entity->eFlags ) };
	for ( uint32_t i = 0; i < ANIM_MAX_PARAMETERS; ++i )
		parameters[i] = groups[i / 3][i % 3];
	return true;
}

static float AnimationParameter( const animAsset_t *asset, const float *parameters, const char *name ) {
	const int index = Anim_ParameterIndex( asset, name );
	return index >= 0 ? parameters[index] : 0;
}
bool BG_AnimationPose( const animAsset_t *asset, const animState_t *state, const float *parameters, uint32_t time, int rig, animPose_t *pose ) {
	if ( !Anim_Evaluate( asset, state, parameters, time, pose ) )
		return false;
	if ( rig == 0 && !Anim_RemoveRootMotion( asset, pose ) )
		return false;
	const char *roots[2] = { rig ? "upperarm.L" : "thigh.L", rig ? "upperarm.R" : "thigh.R" };
	const char *middles[2] = { rig ? "forearm.L" : "shin.L", rig ? "forearm.R" : "shin.R" };
	const char *ends[2] = { rig ? "hand.L" : "foot.L", rig ? "hand.R" : "foot.R" };
	for ( int side = 0; side < 2; ++side ) {
		const int root = Anim_BoneIndex( asset, roots[side] ), middle = Anim_BoneIndex( asset, middles[side] ), end = Anim_BoneIndex( asset, ends[side] );
		if ( root < 0 || middle < 0 || end < 0 )
			continue;
		float target[3], pole[3];
		for ( int i = 0; i < 3; ++i ) {
			target[i] = pose->world[end][i * 4 + 3];
			pole[i] = pose->world[middle][i * 4 + 3];
		}
		if ( rig ) {
			const bool reload = side == 0 && !strcmp( Anim_StateName( asset, state->current ), "reload" );
			const int socket = Anim_BoneIndex( asset, reload ? "magazine" : ( side ? "grip.R" : "grip.L" ) );
			if ( socket < 0 )
				continue;
			for ( int i = 0; i < 3; ++i )
				target[i] = pose->world[socket][i * 4 + 3];
			pole[1] += side ? -16 : 16;
			pole[2] -= 10;
		} else {
			target[2] += AnimationParameter( asset, parameters, side ? "foot_right" : "foot_left" );
			pole[0] += 16;
		}
		if ( !Anim_ApplyTwoBoneIK( asset, pose, root, middle, end, target, pole, 1 ) )
			return false;
	}
	if ( rig == 0 ) {
		const int head = Anim_BoneIndex( asset, "head" );
		if ( head >= 0 ) {
			const float pitch = 0.45f * ( AnimationParameter( asset, parameters, "aim_up" ) - AnimationParameter( asset, parameters, "aim_down" ) );
			const float yaw = AnimationParameter( asset, parameters, "aim_yaw" ) * float( M_PI ) / 180;
			const float forward[3] = { 1, 0, 0 };
			const float target[3] = { pose->world[head][3] + 100 * cosf( pitch ) * cosf( yaw ), pose->world[head][7] + 100 * cosf( pitch ) * sinf( yaw ), pose->world[head][11] + 100 * sinf( pitch ) };
			if ( !Anim_ApplyLookAt( asset, pose, head, forward, target, 1 ) )
				return false;
		}
	}
	return true;
}

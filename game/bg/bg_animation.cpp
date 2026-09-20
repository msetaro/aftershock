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

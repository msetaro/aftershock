#include "g_local.h"
#include <cmath>

static struct composedState_t {
	uint32_t started, nextTouch;
	int firstFrame, frames, frameMS, waitMS, sound;
	float splashRadius;
	bool loopAnimation, once, loopSound;
} composed[MAX_GENTITIES];

void G_ResetComposed() {
	memset( composed, 0, sizeof( composed ) );
}
static void ComposedThink( gentity_t *entity ) {
	const auto &state = composed[entity->s.number];
	const uint32_t elapsed = uint32_t( level.time ) - state.started;
	const uint32_t frame = elapsed / uint32_t( state.frameMS );
	entity->s.frame = state.firstFrame + int( state.loopAnimation ? frame % uint32_t( state.frames ) : frame < uint32_t( state.frames ) ? frame
																																		: uint32_t( state.frames - 1 ) );
	entity->nextthink = level.time + FRAMETIME;
}
static void ComposedUse( gentity_t *entity, gentity_t *other [[maybe_unused]], gentity_t *activator ) {
	const auto &state = composed[entity->s.number];
	if ( state.sound ) {
		if ( state.loopSound )
			entity->s.loopSound = entity->s.loopSound ? 0 : state.sound;
		else
			G_AddEvent( entity, EV_GENERAL_SOUND, state.sound );
	}
	G_UseTargets( entity, activator );
}
static void ComposedTouch( gentity_t *entity, gentity_t *other, trace_t *trace [[maybe_unused]] ) {
	if ( !other->client || other->health <= 0 )
		return;
	auto &state = composed[entity->s.number];
	if ( int32_t( uint32_t( level.time ) - state.nextTouch ) < 0 )
		return;
	state.nextTouch = uint32_t( level.time ) + uint32_t( state.waitMS );
	if ( state.once )
		entity->touch = nullptr;
	if ( entity->damage )
		G_Damage( other, entity, entity, nullptr, nullptr, entity->damage, DAMAGE_NO_PROTECTION, MOD_TRIGGER_HURT );
	if ( entity->inuse )
		ComposedUse( entity, other, other );
}
static void ComposedDie( gentity_t *entity, gentity_t *inflictor [[maybe_unused]], gentity_t *attacker, int damage [[maybe_unused]], int mod [[maybe_unused]] ) {
	entity->takedamage = qfalse;
	const float radius = composed[entity->s.number].splashRadius;
	if ( entity->splashDamage && radius > 0 )
		G_RadiusDamage( entity->r.currentOrigin, attacker, float( entity->splashDamage ), radius, entity, MOD_TRIGGER_HURT );
	G_UseTargets( entity, attacker );
	if ( entity->inuse )
		G_FreeEntity( entity );
}
void SP_composed( gentity_t *entity ) {
	auto &state = composed[entity->s.number];
	state = {};
	state.started = uint32_t( level.time );
	entity->s.eType = ET_GENERAL;
	entity->use = ComposedUse;
	G_SetOrigin( entity, entity->s.origin );
	VectorCopy( entity->s.angles, entity->s.apos.trBase );
	VectorCopy( entity->s.angles, entity->r.currentAngles );
	if ( entity->model && entity->model[0] )
		entity->s.modelindex = G_ModelIndex( entity->model );
	G_SpawnInt( "anim_first", "0", &state.firstFrame );
	G_SpawnInt( "anim_frames", "0", &state.frames );
	G_SpawnInt( "anim_ms", "100", &state.frameMS );
	int flag;
	G_SpawnInt( "anim_loop", "1", &flag );
	state.loopAnimation = flag != 0;
	if ( state.frames ) {
		if ( !entity->s.modelindex || state.firstFrame < 0 || state.firstFrame > 4095 || state.frames < 1 || state.frames > 4096 || state.frameMS < 10 || state.frameMS > 10000 )
			G_Error( "Composed entity %s has invalid model animation", entity->classname );
		entity->think = ComposedThink;
		ComposedThink( entity );
	}
	G_SpawnVector( "mins", "-16 -16 -16", entity->r.mins );
	G_SpawnVector( "maxs", "16 16 16", entity->r.maxs );
	for ( int i = 0; i < 3; ++i )
		if ( !std::isfinite( entity->r.mins[i] ) || !std::isfinite( entity->r.maxs[i] ) || entity->r.mins[i] < -1024 || entity->r.maxs[i] > 1024 || entity->r.mins[i] >= entity->r.maxs[i] )
			G_Error( "Composed entity %s has invalid collision bounds", entity->classname );
	G_SpawnInt( "solid", "0", &flag );
	entity->r.contents = flag ? CONTENTS_SOLID : 0;
	if ( G_SpawnInt( "trigger_wait", "250", &state.waitMS ) ) {
		if ( state.waitMS < 0 || state.waitMS > 60000 )
			G_Error( "Composed entity %s has invalid trigger interval", entity->classname );
		G_SpawnInt( "trigger_once", "0", &flag );
		state.once = flag != 0;
		entity->r.contents |= CONTENTS_TRIGGER;
		entity->touch = ComposedTouch;
	}
	G_SpawnInt( "splash_damage", "0", &entity->splashDamage );
	G_SpawnFloat( "splash_radius", "0", &state.splashRadius );
	if ( entity->health < 0 || entity->health > 10000 || entity->damage < 0 || entity->damage > 10000 || entity->splashDamage < 0 || entity->splashDamage > 10000 ||
		 !std::isfinite( state.splashRadius ) || state.splashRadius < 0 || state.splashRadius > 4096 )
		G_Error( "Composed entity %s has invalid damage", entity->classname );
	entity->takedamage = entity->health > 0 ? qtrue : qfalse;
	entity->die = ComposedDie;
	char *sound;
	G_SpawnString( "noise", "", &sound );
	G_SpawnInt( "audio_loop", "0", &flag );
	state.loopSound = flag != 0;
	if ( sound[0] ) {
		if ( state.loopSound && ( strlen( sound ) < 4 || Q_stricmp( sound + strlen( sound ) - 4, ".wav" ) ) )
			G_Error( "Composed entity %s requires WAV for looping audio", entity->classname );
		state.sound = G_SoundIndex( sound );
		if ( state.loopSound )
			entity->s.loopSound = state.sound;
	}
	trap_LinkEntity( entity );
}

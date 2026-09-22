#pragma once
#ifdef AFTERSHOCK_DEVTOOLS
#include <stdint.h>
#include "../weapons/weapons_public.h"
#include "../animation/animation_public.h"

struct devEntity_t {
	char classname[64];
	float origin[3], mins[3], maxs[3];
	int32_t source, health, model, frame, sound, contents;
	bool linked;
};
struct devWeaponState_t {
	weaponState_t state;
	animState_t animation;
	char name[64], animationName[64];
	int32_t selected;
	uint32_t attachments;
};
struct devAnimationState_t {
	animState_t state;
	char name[64];
};
// Registered by the local native game; external content modules may omit tools.
struct devGameTools_t {
	bool ( *ReadEntity )( int index, devEntity_t *entity );
	const char *( *FieldName )( int index );
	bool ( *ReadField )( int entity, const char *key, char *value, int capacity );
	bool ( *WriteField )( int entity, const char *key, const char *value );
	int ( *Spawn )( const char *classname, const float *origin );
	bool ( *Delete )( int entity );
	int ( *MapCount )( void ); // -1 if the complete source could not be retained
	const char *( *MapText )( int index ); // empty string for deliberately deleted records
	bool ( *ReadWeapon )( int owner, int hand, devWeaponState_t *state ) = nullptr;
	bool ( *ReadAnimation )( int owner, int rig, devAnimationState_t *state ) = nullptr;
};
void Dev_RegisterGameTools( const devGameTools_t *tools );
#endif

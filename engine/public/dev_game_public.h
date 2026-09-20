#pragma once
#ifdef AFTERSHOCK_DEVTOOLS
#include <stdint.h>

struct devEntity_t {
	char classname[64];
	float origin[3], mins[3], maxs[3];
	int32_t source, health;
	bool linked;
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
};
void Dev_RegisterGameTools( const devGameTools_t *tools );
#endif

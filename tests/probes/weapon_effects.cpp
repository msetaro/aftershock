// Material-hit presentation selects cooked effects without changing gameplay hits.
#include "../../engine/public/cg_native_public.h"
#define COM_TRAP_GETVALUE 700
#include "../../game/cgame/cg_data_weapons.cpp"
#include <assert.h>

static weaponDef_t definition;
static uint32_t bursts, marks, explosions;
static vec3_t emittedOrigin, emittedNormal;
const weaponDef_t *BG_WeaponDefinition( int index ) {
	assert( index == 0 );
	return &definition;
}
void QDECL CG_Error( const char *, ... ) {
	abort();
}
void QDECL CG_Printf( const char *, ... ) {
}
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}
void QDECL Com_Printf( const char *, ... ) {
}
uint32_t trap_R_StartEffect( qhandle_t handle, const vec3_t origin, const vec3_t axis[3], uint32_t ) {
	assert( handle == 17 );
	VectorCopy( origin, emittedOrigin );
	VectorCopy( axis[0], emittedNormal );
	assert( fabsf( DotProduct( axis[0], axis[1] ) ) < .001f );
	assert( fabsf( VectorLength( axis[2] ) - 1 ) < .001f );
	return ++bursts;
}
void CG_ImpactMark( qhandle_t, const vec3_t, const vec3_t, float, float, float, float, float, qboolean, float, qboolean ) {
	++marks;
}
localEntity_t *CG_MakeExplosion( vec3_t, vec3_t, qhandle_t, qhandle_t, int, qboolean ) {
	++explosions;
	return nullptr;
}
int main() {
	definition.materialCount = 1;
	strcpy( definition.materials[0].effect, "effects/metal.asfx" );
	weaponImpacts[0][0] = 17;
	entityState_t hit{};
	vec3_t position = { 10, 20, 30 }, normal = { 0, 0, 1 };
	hit.eventParm = DirToByte( normal );
	CG_WeaponImpact( &hit, position );
	assert( bursts == 1 && marks == 0 && explosions == 0 );
	assert( VectorCompare( position, emittedOrigin ) && VectorCompare( normal, emittedNormal ) );
	hit.generic1 = 1;
	CG_WeaponImpact( &hit, position );
	assert( bursts == 2 && marks == 0 && explosions == 0 );
	strcpy( definition.materials[0].effect, "effects/legacy" );
	CG_WeaponImpact( &hit, position );
	hit.generic1 = 0;
	CG_WeaponImpact( &hit, position );
	assert( bursts == 2 && marks == 1 && explosions == 1 );
	puts( "PASS: cooked material-hit bursts preserve origin/normal and legacy presentation" );
}

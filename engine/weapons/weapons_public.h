#ifndef WEAPONS_PUBLIC_H
#define WEAPONS_PUBLIC_H
#include <stddef.h>
#include <stdint.h>
#include <type_traits>

inline constexpr uint32_t WEAPON_MAX_DEFINITIONS = 32;
inline constexpr uint32_t WEAPON_MAX_RECOIL = 32, WEAPON_MAX_ROWS = 8, WEAPON_NO_STAGE = UINT32_MAX;
enum weaponFireMode_t : uint32_t { WEAPON_AUTO,
	WEAPON_SEMI,
	WEAPON_BURST };
enum weaponBallistics_t : uint32_t { WEAPON_HITSCAN,
	WEAPON_PROJECTILE };
enum weaponReloadAction_t : uint32_t { WEAPON_EJECT,
	WEAPON_INSERT,
	WEAPON_CHAMBER,
	WEAPON_FINISH };
enum weaponButtons_t : uint32_t { WEAPON_FIRE = 1,
	WEAPON_ADS = 2,
	WEAPON_RELOAD = 4,
	WEAPON_CANCEL = 8,
	WEAPON_MELEE = 16 };
enum weaponEventKind_t : uint32_t { WEAPON_SHOT,
	WEAPON_DRY,
	WEAPON_RELOAD_EVENT,
	WEAPON_MELEE_EVENT };
struct weaponReload_t {
	uint32_t timeMs, action, cancel;
	char event[64];
};
struct weaponMaterial_t {
	char name[32];
	uint32_t surfaceFlags;
	float depth, damageScale;
	char effect[64];
};
struct weaponAttachment_t {
	char name[32], socket[32], model[64];
	float spreadScale, recoilScale, adsFov;
};
struct weaponSound_t {
	char event[32], path[64];
};
struct weaponProjectileDef_t {
	float speed, gravity, bounce, radius;
	uint32_t fuseMs;
};
struct weaponMeleeDef_t {
	float range, damage;
	uint32_t intervalMs;
};
// Version-1 cooked payload. Counts bound every fixed array; unused rows are zero.
struct weaponDef_t {
	char name[64], model[64], animation[64];
	uint32_t fireMode, intervalMs, burstCount, ballistics;
	float damage, minimumDamage, falloffStart, falloffEnd, range, spreadDegrees, adsSpreadScale, viewKickScale;
	uint32_t adsMs;
	float adsFov, sway, bob;
	uint32_t magazine, reserve, recoilCount, reloadCount, materialCount, attachmentCount, soundCount, switchMs;
	weaponProjectileDef_t projectile;
	weaponMeleeDef_t melee;
	float recoil[WEAPON_MAX_RECOIL][2];
	weaponReload_t reload[WEAPON_MAX_ROWS];
	weaponMaterial_t materials[WEAPON_MAX_ROWS];
	weaponAttachment_t attachments[WEAPON_MAX_ROWS];
	weaponSound_t sounds[WEAPON_MAX_ROWS];
};
struct weaponState_t {
	uint32_t time, random, sequence, nextFire, magazine, reserve, chamber, previousButtons;
	uint32_t reloadStage, reloadStart, burstRemaining, adsQ16, nextMelee, switchUntil;
};
struct weaponEvent_t {
	uint32_t kind, stage, sequence;
	float spread[2], recoil[2];
};
struct weaponEvents_t {
	uint32_t count;
	weaponEvent_t items[16];
};
struct weaponProjectile_t {
	float position[3], velocity[3];
	uint32_t ageMs;
};
static_assert( sizeof( weaponDef_t ) == 3936 && std::is_trivially_copyable_v<weaponDef_t> );
static_assert( sizeof( weaponReload_t ) == 76 && sizeof( weaponMaterial_t ) == 108 && sizeof( weaponAttachment_t ) == 140 && sizeof( weaponSound_t ) == 96 );
static_assert( sizeof( weaponState_t ) == 56 && std::is_trivially_copyable_v<weaponState_t> );
static_assert( sizeof( weaponEvent_t ) == 28 && sizeof( weaponProjectile_t ) == 28 );

bool Weapon_StateValid( const weaponState_t *state );
bool Weapon_Open( const void *data, size_t size, weaponDef_t *definition );
bool Weapon_Configure( const weaponDef_t *base, uint32_t attachments, weaponDef_t *configured );
void Weapon_Reset( const weaponDef_t *definition, uint32_t seed, uint32_t time, weaponState_t *state );
// Definition must come from Open/Configure. Exactly one 20 ms tick; no allocation.
bool Weapon_Tick( const weaponDef_t *definition, uint32_t buttons, uint32_t time, weaponState_t *state, weaponEvents_t *events );
float Weapon_Damage( const weaponDef_t *definition, float distance );
float Weapon_PenetrationDamage( const weaponDef_t *definition, uint32_t surfaceFlags, float thickness, float damage );
bool Weapon_ProjectileStep( const weaponDef_t *definition, weaponProjectile_t *projectile );
void Weapon_ProjectileBounce( const weaponDef_t *definition, const float normal[3], weaponProjectile_t *projectile );
#endif

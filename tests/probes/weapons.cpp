#include "../../engine/weapons/weapons_public.h"
#include <assert.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <initializer_list>

static weaponDef_t Load( const char *path ) {
	uint8_t data[65536];
	FILE *file = fopen( path, "rb" );
	assert( file );
	const size_t size = fread( data, 1, sizeof( data ), file );
	assert( !ferror( file ) && feof( file ) );
	fclose( file );
	weaponDef_t definition;
	assert( Weapon_Open( data, size, &definition ) );
	return definition;
}
static weaponEvents_t Step( const weaponDef_t &def, weaponState_t *state, uint32_t buttons ) {
	weaponEvents_t events;
	assert( Weapon_Tick( &def, buttons, state->time + 20, state, &events ) );
	return events;
}
int main( int argc, char **argv ) {
	assert( argc == 3 );
	static_assert( std::is_trivially_copyable_v<weaponDef_t> && std::is_trivially_copyable_v<weaponState_t> );
	weaponDef_t def = Load( argv[1] ), second = Load( argv[2] );
	assert( !strcmp( def.name, "range_rifle" ) && second.damage == 55 && def.damage == 40 );
	assert( def.intervalMs == 80 && def.magazine == 30 && def.reloadCount == 4 && def.attachmentCount == 1 );
	assert( Weapon_Damage( &def, 512 ) == 40 && Weapon_Damage( &def, 1280 ) == 30 && Weapon_Damage( &def, 2048 ) == 20 );
	assert( Weapon_PenetrationDamage( &def, 4096, 1, 40 ) == 10 );
	assert( Weapon_PenetrationDamage( &def, 4096, 3, 40 ) == 0 );
	weaponDef_t attached;
	assert( Weapon_Configure( &def, 1, &attached ) );
	assert( attached.spreadDegrees == def.spreadDegrees * 0.5f && attached.adsFov == 45 );
	assert( attached.recoil[0][0] == 0.75f && !strcmp( attached.attachments[0].socket, "optic" ) );

	weaponState_t a, b, other;
	Weapon_Reset( &def, 12345, 0, &a );
	Weapon_Reset( &def, 12345, 0, &b );
	Weapon_Reset( &def, 12346, 0, &other );
	uint32_t shots = 0, previousShotTime = 0;
	bool seedDiffers = false;
	for ( int tick = 0; tick < 10000 && shots < 1000; ++tick ) {
		// Refill test inventory without changing timing or random state.
		a.magazine = b.magazine = other.magazine = def.magazine;
		a.chamber = b.chamber = other.chamber = 1;
		const auto ea = Step( def, &a, WEAPON_FIRE ), eb = Step( def, &b, WEAPON_FIRE ), ec = Step( def, &other, WEAPON_FIRE );
		assert( !memcmp( &a, &b, sizeof( a ) ) && !memcmp( &ea, &eb, sizeof( ea ) ) );
		for ( uint32_t i = 0; i < ea.count; ++i ) {
			if ( ea.items[i].kind != WEAPON_SHOT )
				continue;
			const auto &shot = ea.items[i];
			assert( !shots || a.time - previousShotTime == def.intervalMs );
			assert( std::fabs( shot.spread[0] ) <= def.spreadDegrees && std::fabs( shot.spread[1] ) <= def.spreadDegrees );
			assert( shot.recoil[0] == def.recoil[shots % def.recoilCount][0] );
			seedDiffers |= memcmp( &ea, &ec, sizeof( ea ) ) != 0;
			printf( "%u %.9g %.9g %.9g %.9g\n", shots, double( shot.spread[0] ), double( shot.spread[1] ), double( shot.recoil[0] ), double( shot.recoil[1] ) );
			previousShotTime = a.time;
			++shots;
		}
	}
	assert( shots == 1000 && seedDiffers );

	Weapon_Reset( &def, 1, 0, &a );
	a.magazine = 2;
	Step( def, &a, WEAPON_FIRE );
	assert( a.magazine == 1 && a.chamber == 1 );
	Step( def, &a, WEAPON_RELOAD );
	const uint32_t started = a.time;
	while ( a.time - started < 1000 )
		Step( def, &a, WEAPON_ADS );
	assert( a.magazine == 30 && a.chamber == 1 && a.reserve == 61 );
	assert( a.adsQ16 == 65536 );
	Step( def, &a, WEAPON_RELOAD );
	assert( a.reloadStage == WEAPON_NO_STAGE ); // A full magazine does not waste reserve ammo.
	Weapon_Reset( &def, 1, 0, &a );
	a.magazine = a.chamber = 0;
	Step( def, &a, WEAPON_RELOAD );
	while ( a.time < 220 )
		Step( def, &a, 0 );
	Step( def, &a, WEAPON_CANCEL );
	assert( a.reloadStage == WEAPON_NO_STAGE && a.magazine == 0 && a.chamber == 0 && a.reserve == 90 );

	for ( auto mode : { WEAPON_SEMI, WEAPON_BURST } ) {
		def.fireMode = mode;
		Weapon_Reset( &def, 1, 0, &a );
		uint32_t count = 0;
		for ( int tick = 0; tick < 50; ++tick ) {
			const auto events = Step( def, &a, WEAPON_FIRE );
			for ( uint32_t i = 0; i < events.count; ++i )
				count += events.items[i].kind == WEAPON_SHOT;
		}
		assert( count == ( mode == WEAPON_SEMI ? 1u : 3u ) );
	}
	weaponProjectile_t projectile = {};
	projectile.velocity[0] = 800;
	Weapon_ProjectileStep( &def, &projectile );
	assert( projectile.position[0] == 16 && projectile.velocity[2] == -16 );
	const float normal[3] = { -1, 0, 0 };
	Weapon_ProjectileBounce( &def, normal, &projectile );
	assert( projectile.velocity[0] == -400 );
}

#include "../../engine/weapons/weapons_public.h"
#include "../../engine/render/tr_cooked.h"
#include "../../third_party/sha256/sha-256.h"
#include <assert.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <initializer_list>

static size_t ReadFile( const char *path, uint8_t *data, size_t capacity ) {
	FILE *file = fopen( path, "rb" );
	assert( file );
	const size_t size = fread( data, 1, capacity, file );
	assert( !ferror( file ) && feof( file ) );
	fclose( file );
	return size;
}
static weaponDef_t Load( const char *path ) {
	uint8_t data[65536];
	const size_t size = ReadFile( path, data, sizeof( data ) );
	weaponDef_t definition;
	assert( Weapon_Open( data, size, &definition ) );
	return definition;
}
static weaponEvents_t Step( const weaponDef_t &def, weaponState_t *state, uint32_t buttons ) {
	weaponEvents_t events;
	assert( Weapon_Tick( &def, buttons, state->time + 20, state, &events ) );
	return events;
}
static void Commands( const weaponDef_t &definition ) {
	weaponDef_t def = definition;
	def.intervalMs = 20;
	def.magazine = 1000;
	weaponState_t server, acknowledged, predicted, offhand;
	Weapon_Reset( &def, 7, 0, &server );
	acknowledged = server;
	Weapon_Reset( &def, 9, 0, &offhand );
	struct command_t {
		uint32_t time, buttons;
	} commands[80];
	uint32_t time = 0;
	for ( uint32_t i = 0; i < 80; ++i ) {
		time += 7 + i % 17;
		commands[i] = { time, i < 20 ? WEAPON_FIRE : i < 40 ? WEAPON_ADS
												 : i < 60	? WEAPON_RELOAD
															: WEAPON_CANCEL | WEAPON_FIRE };
		weaponEvents_t events;
		assert( Weapon_Command( &def, commands[i].buttons, time, &server, &events ) );
		for ( uint32_t j = 0; j < events.count; ++j )
			assert( events.items[j].time <= time && events.items[j].time % 20 == 0 );
		// Missing snapshots: replay from an older acknowledgement on every frame.
		if ( i % 19 == 0 )
			acknowledged = server;
		predicted = acknowledged;
		for ( uint32_t j = 0; j <= i; ++j )
			assert( Weapon_Command( &def, commands[j].buttons, commands[j].time, &predicted, &events ) );
		assert( !memcmp( &server, &predicted, sizeof( server ) ) );
		assert( Weapon_Command( &def, commands[i].buttons, time, &predicted, &events ) && events.count == 0 );
		assert( Weapon_Command( &def, WEAPON_MELEE, time, &offhand, &events ) );
		assert( offhand.sequence == 0 && offhand.random == 9 );
	}
	Weapon_Reset( &def, 7, 0, &server );
	weaponEvents_t events;
	assert( Weapon_Command( &def, WEAPON_FIRE, 1000, &server, &events ) && events.count == 50 );
	for ( uint32_t i = 0; i < 50; ++i )
		assert( events.items[i].kind == WEAPON_SHOT && events.items[i].time == ( i + 1 ) * 20 && events.items[i].sequence == i + 1 );
	predicted = server;
	const auto before = events;
	assert( !Weapon_Command( &def, WEAPON_FIRE, 2001, &server, &events ) );
	assert( !memcmp( &predicted, &server, sizeof( server ) ) && !memcmp( &before, &events, sizeof( events ) ) );
	Weapon_Reset( &def, 7, UINT32_MAX - 9, &server );
	assert( Weapon_Command( &def, WEAPON_FIRE, 30, &server, &events ) && events.count == 2 );
	assert( events.items[0].time == 10 && events.items[1].time == 30 );
}
int main( int argc, char **argv ) {
	assert( argc == 3 );
	if ( !strcmp( argv[1], "index" ) ) {
		uint8_t data[65536], revision[32];
		const size_t size = ReadFile( argv[2], data, sizeof( data ) );
		calc_sha_256( revision, data, size );
		cookedIndex_t index;
		assert( R_ReadCookedIndex( data, size, revision, &index ) && index.count == 2 );
		puts( "PASS: native asset registry accepts both cooked weapon definitions" );
		return 0;
	}
	static_assert( std::is_trivially_copyable_v<weaponDef_t> && std::is_trivially_copyable_v<weaponState_t> );
	weaponDef_t def = Load( argv[1] ), second = Load( argv[2] );
	assert( !strcmp( def.name, "range_rifle" ) && second.damage == 55 && def.damage == 40 );
	Commands( def );
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
	Weapon_Reset( &def, 1, 0, &a );
	a.magazine = a.chamber = 0;
	Step( def, &a, WEAPON_RELOAD );
	while ( a.time < 1020 )
		Step( def, &a, 0 );
	assert( a.magazine == 29 && a.chamber == 1 && a.reserve == 60 );
	Weapon_Reset( &def, 1, 0, &a );
	for ( int i = 0; i < 10; ++i )
		Step( def, &a, WEAPON_ADS );
	assert( a.adsQ16 == 65536 );
	uint32_t strikes = 0;
	for ( int i = 0; i < 50; ++i ) {
		const auto events = Step( def, &a, WEAPON_MELEE );
		for ( uint32_t j = 0; j < events.count; ++j )
			strikes += events.items[j].kind == WEAPON_MELEE_EVENT;
	}
	assert( strikes == 2 );
	Weapon_Reset( &def, 1, UINT32_MAX - 9, &a );
	const auto wrap = Step( def, &a, WEAPON_FIRE );
	assert( a.time == 10 && wrap.count == 1 && wrap.items[0].kind == WEAPON_SHOT );
	b = a;
	weaponEvents_t untouched;
	assert( !Weapon_Tick( &def, 0, a.time + 40, &a, &untouched ) && !memcmp( &a, &b, sizeof( a ) ) );
	def.fireMode = WEAPON_AUTO;
	def.intervalMs = 30;
	Weapon_Reset( &def, 1, 0, &a );
	uint32_t fractionalShots = 0;
	for ( int tick = 0; tick < 20; ++tick ) {
		const auto events = Step( def, &a, WEAPON_FIRE );
		for ( uint32_t i = 0; i < events.count; ++i )
			if ( events.items[i].kind == WEAPON_SHOT ) {
				assert( a.time == ( ( 20 + fractionalShots * 30 + 19 ) / 20 ) * 20 );
				++fractionalShots;
			}
	}
	assert( fractionalShots == 13 ); // Carry remainder; 30 ms must not become a 40 ms weapon.
	weaponProjectile_t projectile = {};
	projectile.velocity[0] = 800;
	Weapon_ProjectileStep( &def, &projectile );
	assert( projectile.position[0] == 16 && projectile.velocity[2] == -16 );
	const float normal[3] = { -1, 0, 0 };
	Weapon_ProjectileBounce( &def, normal, &projectile );
	assert( projectile.velocity[0] == -400 );
	while ( Weapon_ProjectileStep( &def, &projectile ) ) {
	}
	assert( projectile.ageMs == def.projectile.fuseMs );
	assert( !Weapon_ProjectileStep( &def, &projectile ) && projectile.ageMs == def.projectile.fuseMs );
}

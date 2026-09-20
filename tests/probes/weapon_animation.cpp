#include "../../engine/qcommon/q_shared.h"
#include "../../engine/weapons/weapons_public.h"
#include "../../game/bg/bg_weapons.cpp"
#include <assert.h>
#include <fstream>
#include <iterator>
#include <vector>

int main( int argc, char **argv ) {
	weaponNotifyHistory_t history = {};
	assert( Weapon_NotifyOnce( &history, 1, 10, 2 ) );
	assert( Weapon_NotifyOnce( &history, 1, 10, 1 ) ); // Entity slot order can differ from notify order.
	assert( !Weapon_NotifyOnce( &history, 1, 10, 2 ) );
	assert( !Weapon_NotifyOnce( &history, 1, 10, 1 ) );
	assert( Weapon_NotifyOnce( &history, 2, 10, 1 ) ); // Respawn.
	assert( Weapon_NotifyOnce( &history, 2, 11, 1 ) ); // Reused remote client slot.
	history = {};
	assert( Weapon_NotifyOnce( &history, 1, 1, 0xfffffffeu ) );
	assert( Weapon_NotifyOnce( &history, 1, 1, 0 ) );
	assert( Weapon_NotifyOnce( &history, 1, 1, 0xffffffffu ) );
	assert( !Weapon_NotifyOnce( &history, 1, 1, 0xffffffffu ) );
	assert( Weapon_NotifyOnce( &history, 1, 1, 10000 ) );
	assert( !Weapon_NotifyOnce( &history, 1, 1, 0 ) );
	assert( Weapon_NotifyOnce( &history, 1, 1, 10001 ) );
	Weapon_ForgetNotifiesAfter( &history, 10000 ); // Server rejected a predicted shot at capacity.
	assert( !Weapon_NotifyOnce( &history, 1, 1, 10000 ) );
	assert( Weapon_NotifyOnce( &history, 1, 1, 10001 ) ); // The identity can play when the shot is accepted later.
	assert( argc == 3 );
	std::ifstream weaponFile( argv[1], std::ios::binary ), graphFile( argv[2], std::ios::binary );
	const std::vector<char> weaponBytes( ( std::istreambuf_iterator<char>( weaponFile ) ), {} );
	const std::vector<char> graphBytes( ( std::istreambuf_iterator<char>( graphFile ) ), {} );
	weaponDef_t definition;
	animAsset_t graph;
	assert( Weapon_Open( weaponBytes.data(), weaponBytes.size(), &definition ) );
	assert( Anim_Open( graphBytes.data(), graphBytes.size(), &graph ) );
	weaponState_t weapon;
	Weapon_Reset( &definition, 12345, 0, &weapon );
	animState_t animation;
	Anim_Reset( &graph, 0, &animation );
	float parameters[ANIM_MAX_PARAMETERS];
	Anim_DefaultParameters( &graph, parameters );
	uint32_t shots = 0, shells = 0, reloads = 0;
	auto advance = [&]( uint32_t buttons ) {
		weaponEvents_t events;
		assert( Weapon_Tick( &definition, buttons, weapon.time + 20, &weapon, &events ) );
		animEvents_t notifies;
		assert( BG_WeaponAnimationStep( &graph, &weapon, &events, &animation, parameters, &notifies ) );
		for ( uint32_t i = 0; i < notifies.count; ++i ) {
			const char *name = Anim_EventName( &graph, notifies.items[i].id );
			shots += !strcmp( name, "shot" );
			shells += !strcmp( name, "shell_eject" );
			reloads += !strcmp( name, "magazine_out" ) || !strcmp( name, "magazine_in" ) || !strcmp( name, "bolt" ) || !strcmp( name, "reload_complete" );
		}
		assert( animation.lastTime == weapon.time );
		return notifies;
	};
	for ( int tick = 0; tick < 20; ++tick )
		advance( WEAPON_FIRE );
	assert( shots == 5 && shells == 5 ); // Every automatic shot restarts fire without losing the previous shell.
	advance( WEAPON_RELOAD );
	assert( !strcmp( Anim_StateName( &graph, animation.current ), "reload" ) );
	for ( int tick = 0; tick < 50; ++tick )
		advance( 0 );
	assert( reloads == 4 && weapon.reloadStage == WEAPON_NO_STAGE );
	advance( WEAPON_FIRE );
	advance( WEAPON_RELOAD );
	advance( 0 );
	advance( WEAPON_RELOAD );
	assert( !strcmp( Anim_StateName( &graph, animation.current ), "idle" ) );
	const auto melee = advance( WEAPON_MELEE );
	assert( melee.count == 1 && !strcmp( Anim_EventName( &graph, melee.items[0].id ), "melee" ) );
	for ( int tick = 0; tick < 30; ++tick )
		advance( WEAPON_ADS );
	assert( parameters[Anim_ParameterIndex( &graph, "ads" )] == 1 );
	const auto acknowledgedWeapon = weapon;
	const auto acknowledgedAnimation = animation;
	float acknowledgedParameters[ANIM_MAX_PARAMETERS];
	memcpy( acknowledgedParameters, parameters, sizeof( parameters ) );
	advance( WEAPON_FIRE );
	const auto predictedWeapon = weapon;
	const auto predictedAnimation = animation;
	weapon = acknowledgedWeapon;
	animation = acknowledgedAnimation;
	memcpy( parameters, acknowledgedParameters, sizeof( parameters ) );
	advance( WEAPON_FIRE );
	assert( !memcmp( &predictedWeapon, &weapon, sizeof( weapon ) ) );
	assert( !memcmp( &predictedAnimation, &animation, sizeof( animation ) ) );
	puts( "PASS: automatic shot/shell notifies, staged reload/cancel, melee, ADS and animation command replay" );
}

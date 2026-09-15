#include "../../code/ui/ui_local.h"
#include <assert.h>
#include <stddef.h>
#include <type_traits>

using setInfo_t = void (*)( playerInfo_t *, int, int, vec3_t, vec3_t, int, qboolean );
static_assert( std::is_same_v<decltype(&UI_PlayerInfo_SetInfo), setInfo_t> );
static_assert( sizeof(playerInfo_t) == 1128 );
static_assert( offsetof(playerInfo_t, pendingWeapon) == 1076 );
static_assert( offsetof(playerInfo_t, weaponTimer) == 1080 );

int main( int argc, char **argv ) {
	playerInfo_t player = {};
	assert( argc == 2 );
	player.pendingWeapon = (decltype(player.pendingWeapon))atoi( argv[1] );
	assert( player.pendingWeapon == -1 );
	player.pendingWeapon = WP_MACHINEGUN;
	assert( player.pendingWeapon == WP_MACHINEGUN );
	return 0;
}

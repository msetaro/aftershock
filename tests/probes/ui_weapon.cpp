#include "../../code/ui/ui_local.h"
#include <assert.h>

int main( int argc, char **argv ) {
	playerInfo_t player = {};
	assert( argc == 2 );
	player.pendingWeapon = (decltype(player.pendingWeapon))atoi( argv[1] );
	assert( player.pendingWeapon == -1 );
	player.pendingWeapon = WP_MACHINEGUN;
	assert( player.pendingWeapon == WP_MACHINEGUN );
	return 0;
}

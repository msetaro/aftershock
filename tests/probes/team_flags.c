#include "../../code/game/g_team.cpp"
#include <assert.h>

vmCvar_t g_gametype;
static char flagString[4];
static int updates;

void trap_SetConfigstring( int index, const char *value ) {
	assert( index == CS_FLAGSTATUS );
	assert( strlen(value) < sizeof(flagString) );
	strcpy( flagString, value );
	updates++;
}

int main( void ) {
	g_gametype.integer = GT_FFA;
	Team_InitGame();
	assert( updates == 0 );
	g_gametype.integer = GT_CTF;
	Team_InitGame();
	assert( updates == 1 && !strcmp(flagString, "00") );
	assert( teamgame.redStatus == FLAG_ATBASE && teamgame.blueStatus == FLAG_ATBASE );
	Team_SetFlagStatus( TEAM_RED, FLAG_TAKEN );
	assert( updates == 2 && !strcmp(flagString, "10") );
	Team_SetFlagStatus( TEAM_RED, FLAG_TAKEN );
	assert( updates == 2 );
	Team_SetFlagStatus( TEAM_BLUE, FLAG_DROPPED );
	assert( updates == 3 && !strcmp(flagString, "12") );
	Team_InitGame();
	assert( updates == 4 && !strcmp(flagString, "00") );
#ifdef MISSIONPACK
	g_gametype.integer = GT_1FCTF;
	Team_InitGame();
	assert( updates == 5 && !strcmp(flagString, "0") );
	assert( teamgame.flagStatus == FLAG_ATBASE );
	Team_SetFlagStatus( TEAM_FREE, FLAG_TAKEN_BLUE );
	assert( updates == 6 && !strcmp(flagString, "3") );
	Team_SetFlagStatus( TEAM_FREE, FLAG_TAKEN_BLUE );
	assert( updates == 6 );
#endif
	return 0;
}

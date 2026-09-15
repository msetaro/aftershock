#include "g_local.h"
#include <assert.h>

void trap_SetConfigstring( int index, const char *value ) { (void)index; (void)value; }
char * QDECL va( char *format, ... ) { (void)format; return ""; }
void CheckExitRules( void ) {}
void SendScoreboardMessageToAllClients( void ) {}

int main( void ) {
	static gclient_t clients[MAX_CLIENTS];
	level.clients = clients;
	level.numteamVotingClients[0] = 7;
	level.numteamVotingClients[1] = 9;
	level.spawning = qtrue;
	level.numSpawnVars = 17;
	CalculateRanks();
	assert( level.numteamVotingClients[0] == 0 );
	assert( level.numteamVotingClients[1] == 0 );
	assert( level.spawning == qtrue );
	assert( level.numSpawnVars == 17 );
	level.maxclients = 3;
	clients[0].pers.connected = CON_CONNECTED;
	clients[0].sess.sessionTeam = TEAM_RED;
	clients[1].pers.connected = CON_CONNECTED;
	clients[1].sess.sessionTeam = TEAM_BLUE;
	clients[2].pers.connected = CON_CONNECTED;
	clients[2].sess.sessionTeam = TEAM_RED;
	g_entities[2].r.svFlags = SVF_BOT;
	CalculateRanks();
	assert( level.numteamVotingClients[0] == 1 );
	assert( level.numteamVotingClients[1] == 1 );
	assert( level.numVotingClients == 2 );
	assert( level.spawning == qtrue );
	assert( level.numSpawnVars == 17 );
	return 0;
}

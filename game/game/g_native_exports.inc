/* Typed exports; included after the module namespace. */

void Game_Init( int levelTime, int randomSeed, int restart ) {
	game::G_InitGame( levelTime, randomSeed, restart );
}

void Game_Shutdown( int restart ) {
	game::G_ShutdownGame( restart );
}

const char *Game_ClientConnect( int clientNum, int firstTime, int isBot ) {
	return game::ClientConnect( clientNum, (game::qboolean)firstTime, (game::qboolean)isBot );
}

void Game_RunFrame( int time ) {
	game::G_RunFrame( time );
}

int Game_ConsoleCommand( void ) {
	return game::ConsoleCommand();
}

int Game_BotStartFrame( int time ) {
	return game::BotAIStartFrame( time );
}

void Game_ClientThink( int clientNum ) {
	game::ClientThink( clientNum );
}

void Game_ClientUserinfoChanged( int clientNum ) {
	game::ClientUserinfoChanged( clientNum );
}

void Game_ClientDisconnect( int clientNum ) {
	game::ClientDisconnect( clientNum );
}

void Game_ClientBegin( int clientNum ) {
	game::ClientBegin( clientNum );
}

void Game_ClientCommand( int clientNum ) {
	game::ClientCommand( clientNum );
}

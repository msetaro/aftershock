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
#ifdef AFTERSHOCK_DEVTOOLS
	const uint64_t scope = Dev_BeginScope( "game simulation" );
#endif
	game::G_RunFrame( time );
#ifdef AFTERSHOCK_DEVTOOLS
	Dev_EndScope( scope );
#endif
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

int Game_WriteCheckpoint( void *writer ) {
	return writer && game::G_WriteCheckpoint( (stateWriter_t *)writer );
}
int Game_ReadCheckpointCvars( const void *reader, int apply ) {
	return reader && game::G_ReadCheckpointCvars( *(const stateReader_t *)reader, apply );
}
int Game_ReadCheckpoint( const void *reader, int apply ) {
	return reader && ( apply == 0 || apply == 1 ) && game::G_ReadCheckpoint( *(const stateReader_t *)reader, apply != 0 );
}
void Game_LinkCheckpointEntities( void ) {
	game::G_LinkCheckpointEntities();
}
int Game_RestoreCheckpointRandom( const void *reader ) {
	return reader && game::G_RestoreCheckpointRandom( *(const stateReader_t *)reader );
}

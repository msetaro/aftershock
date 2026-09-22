#define NATIVE_NAMESPACE game
#define NATIVE_SOURCE "game/g_main.cpp"
#include "../../game/module.cpp"
int main() {
	using namespace game;
	for ( uint32_t i = 0; i < SAVED_GAME_CVARS; ++i ) {
		auto &entry = gameCvarTable[i];
		entry.modificationCount = int( i ) + 9;
		if ( entry.vmCvar )
			*entry.vmCvar = { int( i ), int( i ) + 7, 2.5f, 2, "2.5" };
	}
	passwordLastMod = 31;
	static unsigned char bytes[131072];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(G_WriteMainCvarState(&writer));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	for ( auto &entry : gameCvarTable ) {
		entry.modificationCount = 0;
		if ( entry.vmCvar )
			*entry.vmCvar = { 700, 0, 0, 0, "" };
	}
	passwordLastMod = -1;
	assert(G_ReadMainCvarState(reader,false) && passwordLastMod==-1 && !g_gametype.value);
	assert(G_ReadMainCvarState(reader,true) && passwordLastMod==31);
	for ( uint32_t i = 0; i < SAVED_GAME_CVARS; ++i ) {
		const auto &entry = gameCvarTable[i];
		assert(entry.modificationCount==int(i)+9);
		if ( entry.vmCvar )
			assert(entry.vmCvar->handle==700 && entry.vmCvar->modificationCount==int(i)+7 && entry.vmCvar->value==2.5f);
	}
	gCachedCvar_t bindings[SAVED_GAME_CVARS];
	for ( uint32_t i = 0; i < SAVED_GAME_CVARS; ++i )
		bindings[i] = { gameCvarTable[i].cvarName, gameCvarTable[i].vmCvar };
	writer = { bytes, sizeof( bytes ) };
	assert(G_WriteCachedCvars(&writer,"game.cvars.main",bindings,SAVED_GAME_CVARS));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	g_gametype.value = 99;
	assert(!G_ReadMainCvarState(reader,true) && g_gametype.value==99 && passwordLastMod==31);
	puts( "PASS: main cvar table tracking and password change detection resume against rebound handles" );
}

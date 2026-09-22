#include "../../engine/botlib/be_interface.cpp"
#include <assert.h>
int main() {
	botlibsetup = botlibglobals.botlibsetup = 1;
	botlibglobals.maxclients = 4;
	botlibglobals.maxentities = 1024;
	botlibglobals.time = 123.5f;
	botDeveloper = 2;
	static unsigned char bytes[8192];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(BotLib_WriteGlobalState(&writer));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	botlibglobals.time = 0;
	botDeveloper = 0;
	assert(BotLib_ReadGlobalState(reader,false) && botlibglobals.time==0);
	assert(BotLib_ReadGlobalState(reader,true) && botlibglobals.time==123.5f && botDeveloper==2);
	botlibglobals.maxclients = 5;
	botlibglobals.time = 9;
	assert(!BotLib_ReadGlobalState(reader,true) && botlibglobals.time==9);
	botlibglobals.maxclients = 4;
	botlibsetup = 0;
	assert(!BotLib_ReadGlobalState(reader,true));
	botlibsetup = 1;
	botlibglobals.time = std::numeric_limits<float>::infinity();
	writer = { bytes, sizeof( bytes ) };
	assert(!BotLib_WriteGlobalState(&writer) && !State_Finish(&writer));
	puts( "PASS: botlib global clocks restore only against matching initialized pool dimensions" );
}

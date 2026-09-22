#include "../../engine/botlib/be_ea.cpp"
#include <assert.h>
botlib_globals_t botlibglobals;
int main() {
	static bot_input_t original[4], restored[4];
	botlibglobals.maxclients = 4;
	botinputs = original;
	vec3_t direction{ 1, -0.0f, 0.5f }, angles{ 10, 20, 30 };
	EA_Move( 2, direction, 321 );
	EA_View( 2, angles );
	EA_Jump( 2 );
	static unsigned char bytes[8192];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(EA_WriteState(&writer));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	const auto saved = original[2];
	EA_ResetInput( 2 );
	EA_Jump( 2 );
	const auto expected = original[2];
	botinputs = restored;
	assert(EA_ReadState(reader,false) && !restored[2].actionflags);
	assert(EA_ReadState(reader,true) && !memcmp(&saved,&restored[2],sizeof(saved)));
	EA_ResetInput( 2 );
	EA_Jump( 2 );
	assert(!memcmp(&expected,&restored[2],sizeof(expected)));
	// A valid envelope missing the last client must preserve prior restored inputs.
	writer = { bytes, sizeof( bytes ) };
	const int32_t count = 4;
	assert(State_Append(&writer,inputCountSchema,0,&count));
	for ( uint32_t i = 0; i < 3; ++i )
		assert(State_Append(&writer,inputSchema,i,&original[i]));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(!EA_ReadState(reader,true) && !memcmp(&expected,&restored[2],sizeof(expected)));
	restored[3].speed = 401;
	writer = { bytes, sizeof( bytes ) };
	assert(!EA_WriteState(&writer) && !State_Finish(&writer));
	botinputs = nullptr;
	writer = { bytes, sizeof( bytes ) };
	assert(EA_WriteState(&writer));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(EA_ReadState(reader,true));
	botinputs = restored;
	assert(!EA_ReadState(reader,true));
	puts( "PASS: bot input records preserve jump-edge continuation across relocated storage" );
}

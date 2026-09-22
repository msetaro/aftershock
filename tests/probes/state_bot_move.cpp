#include "../../engine/botlib/be_ai_move.cpp"
#include <assert.h>
botlib_import_t botimport;
static bot_movestate_t allocated[2];
static int allocations;
void *GetClearedMemory(size_t size) {
	assert(size==sizeof(bot_movestate_t) && allocations<2);
	return &allocated[allocations++];
}
int main() {
	static bot_movestate_t original[2], restored[2];
	original[0].client = original[0].entitynum = 3;
	original[1].client = original[1].entitynum = 5;
	original[0].origin[1] = -0.0f;
	original[0].reachability_time = 71.125f;
	original[0].avoidreach[0] = 17;
	original[0].avoidreachtimes[0] = 93.75f;
	original[0].avoidreachtries[0] = 3;
	original[1].lastorigin[2] = 128.5f;
	botmovestates[1] = &original[0];
	botmovestates[7] = &original[1];
	modeltypes[12] = MODELTYPE_FUNC_PLAT;
	vec3_t spot{ 10, 20, 30 };
	BotAddAvoidSpot( 1, spot, 64, AVOID_ALWAYS );
	static unsigned char bytes[65536];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(Bot_WriteMoveState(&writer));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	const auto saved = original[0];
	BotAddAvoidSpot( 1, spot, 32, AVOID_DONTBLOCK );
	const auto expected = original[0];
	botmovestates[1] = &restored[0];
	botmovestates[7] = &restored[1];
	modeltypes[12] = 0;
	assert(Bot_ReadMoveState(reader,false) && !modeltypes[12] && !restored[0].numavoidspots);
	assert(Bot_ReadMoveState(reader,true));
	assert(modeltypes[12]==MODELTYPE_FUNC_PLAT && !memcmp(&saved,&restored[0],sizeof(saved)));
	assert(!memcmp(&original[1],&restored[1],sizeof(original[1])));
	BotAddAvoidSpot( 1, spot, 32, AVOID_DONTBLOCK );
	assert(!memcmp(&expected,&restored[0],sizeof(expected)));
	movePoolSave_t pool{};
	uint32_t version;
	assert(State_Find(reader,movePoolSchema,0,&pool,&version));
	botmovestates[7] = nullptr;
	assert(!Bot_ReadMoveState(reader,true));
	botmovestates[7] = &restored[1];
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,movePoolSchema,0,&pool));
	assert(State_Append(&writer,moveStateSchema,1,&saved));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(!Bot_ReadMoveState(reader,true) && !memcmp(&expected,&restored[0],sizeof(expected)));
	restored[1].numavoidspots = MAX_AVOIDSPOTS + 1;
	writer = { bytes, sizeof( bytes ) };
	assert(!Bot_WriteMoveState(&writer) && !State_Finish(&writer));

	restored[1].numavoidspots = 0;
	writer = { bytes, sizeof( bytes ) };
	assert(Bot_WriteMoveState(&writer));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(!Bot_PrepareMoveState(reader) && allocations==0);
	for ( auto &state : botmovestates )
		state = nullptr;
	assert(Bot_PrepareMoveState(reader) && allocations==2);
	assert(botmovestates[1]==&allocated[0] && botmovestates[7]==&allocated[1] && !botmovestates[2]);
	assert(!memcmp(botmovestates[1],&expected,sizeof(expected)) && modeltypes[12]==MODELTYPE_FUNC_PLAT);
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,movePoolSchema,0,&pool));
	assert(State_Append(&writer,moveStateSchema,1,&saved));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	for ( auto &state : botmovestates )
		state = nullptr;
	assert(!Bot_PrepareMoveState(reader) && allocations==2 && !botmovestates[1]);
	puts( "PASS: movement handles reconstruct exact saved slots only after all records validate" );
	puts( "PASS: bot movement memory and avoidance continue identically after relocating handle storage" );
}

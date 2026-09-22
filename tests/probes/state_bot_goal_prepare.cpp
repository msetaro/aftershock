#include "../../engine/botlib/be_ai_goal.cpp"
#include "state_bot_weight_io.h"
int main() {
	botimport.FS_FOpenFile = Open;
	botimport.FS_Read = Read;
	botimport.FS_FCloseFile = Close;
	botimport.Print = Print;
	iteminfo_t info{};
	strcpy( info.classname, "choice" );
	itemconfig_t content{ 1, &info };
	itemconfig = &content;
	auto *shared = ReadWeightConfig( "checkpoint_w.c" );
	assert(shared && Bot_WeightCacheIndex(shared)==0);
	reload.value = 1;
	auto *owned = ReadWeightConfig( "checkpoint_w.c" );
	reload.value = 0;
	assert(owned && owned!=shared);
	owned->weights[0].firstseperator->weight = 11.5f;
	bot_goalstate_t first{}, last{};
	int indices[1] = { 0 };
	first.itemweightconfig = shared;
	first.itemweightindex = indices;
	last.itemweightconfig = owned;
	last.itemweightindex = indices;
	first.client = 3;
	last.client = 5;
	last.goalstacktop = 1;
	last.goalstack[1].areanum = 21;
	botgoalstates[3] = &first;
	botgoalstates[MAX_CLIENTS] = &last;
	static unsigned char bytes[65536];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(Bot_WriteGoalState(&writer));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	const int before = allocations;
	assert(!Bot_PrepareGoalState(reader) && allocations==before);
	botgoalstates[3] = botgoalstates[MAX_CLIENTS] = nullptr;
	Bot_FreePrivateWeightState( owned );
	const int sharedOnly = allocations;
	assert(Bot_PrepareGoalState(reader));
	assert(botgoalstates[3] && botgoalstates[MAX_CLIENTS] && !botgoalstates[1]);
	assert(botgoalstates[3]->itemweightconfig==shared);
	assert(botgoalstates[MAX_CLIENTS]->itemweightconfig!=shared);
	assert(botgoalstates[MAX_CLIENTS]->itemweightconfig->weights[0].firstseperator->weight==11.5f);
	assert(botgoalstates[MAX_CLIENTS]->itemweightindex[0]==0);
	assert(Bot_ReadGoalState(reader,false));
	assert(botgoalstates[MAX_CLIENTS]->goalstacktop==1);
	BotPopGoal( MAX_CLIENTS );
	assert(botgoalstates[MAX_CLIENTS]->goalstacktop==0);
	for ( auto *&state : botgoalstates )
		if ( state ) {
			Bot_FreePrivateWeightState( state->itemweightconfig );
			FreeMemory( state->itemweightindex );
			FreeMemory( state );
			state = nullptr;
		}
	assert(allocations==sharedOnly);
	// A later private-content mismatch must preserve the existing shared cache.
	sourceText = "weight \"other\" return balance(7.5, 1, 12);";
	assert(!Bot_PrepareGoalState(reader) && allocations==sharedOnly);
	assert(Bot_WeightCacheAt(0)==shared && reload.value==0);
	for ( const auto *state : botgoalstates )
		assert(!state);
	BotShutdownWeights();
	assert(allocations==0);
	puts( "PASS: goal handles reconstruct exact slots, shared/private weights and content indices" );
}

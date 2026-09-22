#include "../../engine/botlib/be_ai_weight.cpp"
#include "state_bot_weight_io.h"
int main() {
	botimport.FS_FOpenFile = Open;
	botimport.FS_Read = Read;
	botimport.FS_FCloseFile = Close;
	botimport.Print = Print;
	const auto originalVar = reload;
	auto *original = ReadWeightConfig( "checkpoint_w.c" );
	assert(original && weightFileList[0]==original);
	weightFileList[0] = nullptr;
	weightFileList[7] = original;
	original->weights[0].firstseperator->weight = 9.25f;
	original->weights[0].firstseperator->minweight = -3;
	static unsigned char bytes[131072];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(Bot_WriteWeightCacheState(&writer));
	assert(Bot_WriteWeightState(&writer,129,original));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(!Bot_PrepareWeightCacheState(reader));
	BotShutdownWeights();
	assert(allocations==0);
	assert(Bot_PrepareWeightCacheState(reader));
	assert(!weightFileList[0] && weightFileList[7]);
	int inventory[1] = {};
	assert(FuzzyWeight(inventory,weightFileList[7],0)==9.25f);
	weightconfig_t *privateConfig = nullptr;
	assert(Bot_CreateWeightState(reader,129,&privateConfig));
	assert(privateConfig!=weightFileList[7] && Bot_WeightCacheIndex(privateConfig)==-2);
	assert(FuzzyWeight(inventory,privateConfig,0)==9.25f);
	assert(!memcmp(&reload,&originalVar,sizeof(reload)));
	FreeWeightConfig2( privateConfig );
	BotShutdownWeights();
	assert(allocations==0);
	// Valid changed content must fail identity validation without publishing a cache.
	sourceText = "weight \"other\" return balance(7.5, 1, 12);";
	assert(!Bot_PrepareWeightCacheState(reader) && allocations==0);
	assert(!memcmp(&reload,&originalVar,sizeof(reload)));
	sourceText = "weight \"choice\" return balance(7.5, 1, 12);";
	assert(Bot_PrepareWeightCacheState(reader));
	writer = { bytes, sizeof( bytes ) };
	for ( uint32_t i = 0; i < MAX_WEIGHT_FILES - 1; ++i )
		assert(Bot_WriteWeightState(&writer,i,weightFileList[i]));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	BotShutdownWeights();
	assert(!Bot_PrepareWeightCacheState(reader) && allocations==0);
	for ( const auto *config : weightFileList )
		assert(!config);
	assert(!memcmp(&reload,&originalVar,sizeof(reload)));
	puts( "PASS: weight reconstruction preserves cache slots, private ownership, learned values and libvars" );
}

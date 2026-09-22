#include "../../engine/botlib/be_ai_weap.cpp"
#include "state_bot_weight_io.h"
int main() {
	botimport.FS_FOpenFile = Open;
	botimport.FS_Read = Read;
	botimport.FS_FCloseFile = Close;
	botimport.Print = Print;
	weaponinfo_t info{};
	strcpy( info.name, "choice" );
	weaponconfig_t content{ 1, 0, nullptr, &info };
	weaponconfig = &content;
	auto *shared = ReadWeightConfig( "checkpoint_w.c" );
	assert(shared && Bot_WeightCacheIndex(shared)==0);
	reload.value = 1;
	auto *owned = ReadWeightConfig( "checkpoint_w.c" );
	reload.value = 0;
	assert(owned && owned!=shared);
	owned->weights[0].firstseperator->weight = 11.5f;
	bot_weaponstate_t first{}, last{};
	int indices[1] = { 0 };
	first.weaponweightconfig = shared;
	first.weaponweightindex = indices;
	last.weaponweightconfig = owned;
	last.weaponweightindex = indices;
	botweaponstates[3] = &first;
	botweaponstates[MAX_CLIENTS] = &last;
	static unsigned char bytes[65536];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(Bot_WriteWeaponState(&writer));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	const int before = allocations;
	assert(!Bot_PrepareWeaponState(reader) && allocations==before);
	botweaponstates[3] = botweaponstates[MAX_CLIENTS] = nullptr;
	Bot_FreePrivateWeightState( owned );
	const int sharedOnly = allocations;
	assert(Bot_PrepareWeaponState(reader));
	assert(botweaponstates[3] && botweaponstates[MAX_CLIENTS] && !botweaponstates[1]);
	assert(botweaponstates[3]->weaponweightconfig==shared);
	assert(botweaponstates[MAX_CLIENTS]->weaponweightconfig!=shared);
	assert(botweaponstates[MAX_CLIENTS]->weaponweightconfig->weights[0].firstseperator->weight==11.5f);
	assert(botweaponstates[MAX_CLIENTS]->weaponweightindex[0]==0);
	assert(Bot_ReadWeaponState(reader,false));
	for ( auto *&state : botweaponstates )
		if ( state ) {
			Bot_FreePrivateWeightState( state->weaponweightconfig );
			FreeMemory( state->weaponweightindex );
			FreeMemory( state );
			state = nullptr;
		}
	assert(allocations==sharedOnly);
	// A later private-content mismatch must preserve the existing shared cache.
	sourceText = "weight \"other\" return balance(7.5, 1, 12);";
	assert(!Bot_PrepareWeaponState(reader) && allocations==sharedOnly);
	assert(Bot_WeightCacheAt(0)==shared && reload.value==0);
	for ( const auto *state : botweaponstates )
		assert(!state);
	BotShutdownWeights();
	assert(allocations==0);
	puts( "PASS: weapon handles reconstruct exact slots, shared/private weights and content indices" );
}

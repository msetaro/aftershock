#define NATIVE_NAMESPACE game
#define NATIVE_SOURCE "game/g_utils.cpp"
#include "../../game/module.cpp"

namespace game {
gentity_t g_entities[MAX_GENTITIES];
}
int GameImport_SetEntityReplication( int number, int priority, float radius ) {
	assert(number==7 && priority==0 && radius==0);
	return 1;
}
int main() {
	using namespace game;
	nextRewindSpawn = UINT32_MAX;
	remapCount = 2;
	remappedShaders[0] = { "textures/first", "textures/second", -0.0f };
	remappedShaders[1] = { "textures/third", "textures/fourth", 123.25f };
	const auto first = remappedShaders[0], second = remappedShaders[1];
	static unsigned char bytes[65536];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(G_WriteUtilityState(&writer));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	G_InitGentity( &g_entities[7] );
	assert(g_entities[7].rewindSpawn==1);
	const auto expected = g_entities[7];
	nextRewindSpawn = 99;
	G_ResetShaderRemaps();
	assert(G_ReadUtilityState(reader,false));
	assert(nextRewindSpawn==99 && !remapCount);
	assert(G_ReadUtilityState(reader,true));
	assert(nextRewindSpawn==UINT32_MAX && remapCount==2);
	assert(!memcmp(&first,&remappedShaders[0],sizeof(first)));
	assert(!memcmp(&second,&remappedShaders[1],sizeof(second)));
	g_entities[7] = {};
	G_InitGentity( &g_entities[7] );
	assert(!memcmp(&expected,&g_entities[7],sizeof(expected)));
	// A missing later remap cannot replace the current generation or earlier remap.
	writer = { bytes, sizeof( bytes ) };
	utilitySave_t saved{ 123, 2 };
	assert(State_Append(&writer,utilitySchema,0,&saved));
	assert(State_Append(&writer,remapSchema,0,&first));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(!G_ReadUtilityState(reader,true));
	assert(nextRewindSpawn==1 && remapCount==2);
	assert(!memcmp(&second,&remappedShaders[1],sizeof(second)));
	memset( remappedShaders[1].oldShader, 'x', MAX_QPATH );
	writer = { bytes, sizeof( bytes ) };
	assert(!G_WriteUtilityState(&writer) && !State_Finish(&writer));
	// Unreachable remaps left by reset do not prevent a checkpoint.
	G_ResetShaderRemaps();
	writer = { bytes, sizeof( bytes ) };
	assert(G_WriteUtilityState(&writer) && State_Finish(&writer));
	puts( "PASS: shader remaps and wrapped entity generation restore before identical spawn continuation" );
}

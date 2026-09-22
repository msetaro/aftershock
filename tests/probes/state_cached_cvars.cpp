#define NATIVE_NAMESPACE game
#define NATIVE_SOURCE "game/g_state.cpp"
#include "../../game/module.cpp"
#include "state_cvar_imports.h"
int main() {
	using namespace game;
	vmCvar_t first{ 3, 19, 2.5f, 2, "2.5" }, last{ 4, 11, 7, 7, "7" };
	gCachedCvar_t bindings[] = { { "first", &first }, { "engine-only", nullptr }, { "last", &last } };
	unsigned char bytes[8192];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(G_WriteCachedCvars(&writer,"game.cvars.test",bindings,3));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	first = { 101, 0, 0, 0, "" };
	last = { 102, 0, 0, 0, "" };
	assert(G_ReadCachedCvars(reader,"game.cvars.test",bindings,3,false) && !first.value);
	rejectCvarSlot = 2;
	assert(!G_ReadCachedCvars(reader,"game.cvars.test",bindings,3,true) && !first.value && !cvarApplyCalls);
	rejectCvarSlot = -1;
	assert(G_ReadCachedCvars(reader,"game.cvars.test",bindings,3,true));
	assert(cvarApplyCalls==3);
	assert(first.handle==101 && first.modificationCount==19 && first.value==2.5f && first.integer==2 && !strcmp(first.string,"2.5"));
	assert(last.handle==102 && last.modificationCount==11 && last.value==7);
	const stateSchema_t schema{ "game.cvars.test", 1, 1, sizeof( cachedCvarSave_t ), cachedCvarFields, 7 };
	cachedCvarSave_t saved[3]{};
	uint32_t version;
	for ( uint32_t i = 0; i < 3; ++i )
		assert(State_Find(reader,schema,i,&saved[i],&version));
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,schema,0,&saved[0]) && State_Append(&writer,schema,1,&saved[1]));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	first.integer = 77;
	assert(!G_ReadCachedCvars(reader,"game.cvars.test",bindings,3,true) && first.integer==77);
	writer = { bytes, sizeof( bytes ) };
	saved[2].value.value = HUGE_VALF;
	for ( uint32_t i = 0; i < 3; ++i )
		assert(State_Append(&writer,schema,i,&saved[i]));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(!G_ReadCachedCvars(reader,"game.cvars.test",bindings,3,true) && first.integer==77);
	puts( "PASS: cached cvars restore values and counters with fresh handles only after the whole group validates" );
}

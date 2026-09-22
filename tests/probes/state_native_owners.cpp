#define NATIVE_NAMESPACE game
#if defined( STATE_ITEMS )
#define NATIVE_SOURCE "game/g_items.cpp"
#elif defined( STATE_FILTERS )
#define NATIVE_SOURCE "game/g_svcmds.cpp"
#else
#define NATIVE_SOURCE "game/g_mem.cpp"
#endif
#include "../../game/module.cpp"
namespace game {
#if defined( STATE_FILTERS )
vmCvar_t g_filterBan;
#elif defined( STATE_MEMORY )
vmCvar_t g_debugAlloc;
void QDECL G_Printf( const char *, ... ) {
}
void QDECL G_Error( const char *, ... ) {
	abort();
}
#endif
} // namespace game
int main() {
	using namespace game;
	unsigned char bytes[16384];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	stateReader_t reader;
#if defined( STATE_ITEMS )
	itemRegistered[7] = itemRegistered[MAX_ITEMS - 1] = qtrue;
	assert(G_WriteRegisteredItemState(&writer));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	memset( itemRegistered, 0, sizeof( itemRegistered ) );
	assert(G_ReadRegisteredItemState(reader,false) && !itemRegistered[7]);
	assert(G_ReadRegisteredItemState(reader,true) && itemRegistered[7] && itemRegistered[MAX_ITEMS-1]);
	uint32_t bad[MAX_ITEMS] = {};
	bad[7] = 2;
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,registeredItemsSchema,0,bad));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(!G_ReadRegisteredItemState(reader,true) && itemRegistered[7]);
	puts( "PASS: complete item registration validates boolean values before restore" );
#elif defined( STATE_FILTERS )
	numIPFilters = MAX_IPFILTERS;
	for ( int i = 0; i < numIPFilters; ++i )
		ipFilters[i] = { UINT32_MAX, 0xffffffff };
	ipFilters[MAX_IPFILTERS - 1] = { UINT32_MAX, 0x04030201 };
	g_filterBan.integer = 1;
	char address[] = "1.2.3.4:27960";
	assert(G_FilterPacket(address));
	assert(G_WriteIPFilterState(&writer));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	numIPFilters = 0;
	assert(!G_FilterPacket(address) && G_ReadIPFilterState(reader,false) && !numIPFilters);
	assert(G_ReadIPFilterState(reader,true) && numIPFilters==MAX_IPFILTERS && G_FilterPacket(address));
	ipFiltersSave_t bad{};
	bad.count = MAX_IPFILTERS + 1;
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,ipFiltersSchema,0,&bad));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(!G_ReadIPFilterState(reader,true) && numIPFilters==MAX_IPFILTERS);
	puts( "PASS: all filter slots survive independently of the shorter presentation cvar" );
#else
	G_InitMemory();
	G_Alloc( 33 );
	G_Alloc( 17 );
	assert(allocPoint==96 && G_WriteMemoryState(&writer));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	const auto offset = (char *)G_Alloc( 1 ) - memoryPool;
	assert(offset==96);
	G_InitMemory();
	strcpy( (char *)G_Alloc( 33 ), "initial content" );
	assert(G_ReadMemoryState(reader,false) && allocPoint==64);
	assert(G_ReadMemoryState(reader,true) && allocPoint==96);
	assert(!strcmp(memoryPool,"initial content"));
	assert((char *)G_Alloc(1)-memoryPool==offset);
	assert(!G_ReadMemoryState(reader,true) && allocPoint==128);
	int32_t bad = POOLSIZE + 32;
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,gameMemorySchema,0,&bad));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(!G_ReadMemoryState(reader,true) && allocPoint==128);
	puts( "PASS: legacy game arena restores remaining capacity and the next allocation offset" );
#endif
}

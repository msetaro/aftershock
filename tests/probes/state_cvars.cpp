#include "../../engine/qcommon/cvar.cpp"
#include <assert.h>
#include <stdlib.h>
#ifdef STATE_NATIVE_CACHE
// This mixed engine/native probe shares the legacy q_shared include guard.
namespace game {
uint32_t Q_GetRandomSeed( void );
}
#define NATIVE_NAMESPACE game
#define NATIVE_SOURCE "game/g_state.cpp"
#include "../../game/module.cpp"
namespace game {
gitem_t bg_itemlist[MAX_ITEMS];
int bg_numItems = 3;
} // namespace game
int GameImport_WriteCvarState( void *writer, const char *group, uint32_t slot, const char *name ) {
	return Cvar_WriteState( (stateWriter_t *)writer, group, slot, name );
}
int GameImport_ReadCvarState( const void *reader, const char *group, uint32_t slot, const char *name, int apply, int removable ) {
	return Cvar_ReadState( *(const stateReader_t *)reader, group, slot, name, apply != 0, apply == 2, removable != 0 );
}
#endif
qboolean com_errorEntered;
static int allocations;
char *CopyString( const char *text ) {
	auto *copy = (char *)malloc( strlen( text ) + 1 );
	assert(copy);
	strcpy( copy, text );
	++allocations;
	return copy;
}
void Z_Free( void *memory ) {
	assert(memory && allocations>0);
	--allocations;
	free( memory );
}
void QDECL Com_Printf( const char *, ... ) {
}
void QDECL Com_DPrintf( const char *, ... ) {
}
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}
qboolean FS_InvalidGameDir( const char * ) {
	abort();
}
static bool CapacityRecords( const stateReader_t &reader ) {
	return Cvar_ReadState( reader, "engine.capacity", 0, "capacity_one", false ) &&
		   Cvar_ReadState( reader, "engine.capacity", 1, "capacity_two", false );
}
static bool DuplicateCapacityRecord( const stateReader_t &reader ) {
	return Cvar_ReadState( reader, "engine.capacity", 0, "capacity_one", false ) &&
		   Cvar_ReadState( reader, "engine.capacity", 0, "capacity_one", false );
}
static bool MutatingCapacityRecord( const stateReader_t &reader ) {
	return Cvar_ReadState( reader, "engine.capacity", 0, "capacity_one", true );
}
int main() {
	auto *var = Cvar_Get( "checkpoint_gravity", "800", CVAR_LATCH | CVAR_SERVERINFO );
	var->latchedString = CopyString( "600" );
	var->mins = CopyString( "1" );
	var->maxs = CopyString( "2000" );
	var->validator = CV_INTEGER;
	var->group = CVG_SERVER;
	var->modificationCount = 19;
	var->modified = qfalse;
	const auto handle = var - cvar_indexes;
	static unsigned char bytes[524288];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(Cvar_WriteState(&writer,"engine.cvars.test",0,var->name));
	assert(Cvar_WriteState(&writer,"engine.cvars.test",1,"checkpoint_absent"));
	assert(Cvar_WriteServerState(&writer));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	Cvar_Set( var->name, "300" );
	assert(var->integer==300 && !var->latchedString);
	assert(Cvar_ReadState(reader,"engine.cvars.test",0,var->name,false) && var->integer==300);
	assert(Cvar_ReadState(reader,"engine.cvars.test",0,var->name,true));
	assert(var==&cvar_indexes[handle] && var->integer==800 && var->value==800 && !strcmp(var->string,"800"));
	assert(var->modificationCount==19 && !var->modified && !strcmp(var->latchedString,"600"));
	assert(!strcmp(var->mins,"1") && !strcmp(var->maxs,"2000") && var->validator==CV_INTEGER && var->group==CVG_SERVER);
	assert(!Cvar_ReadState(reader,"engine.cvars.test",0,"other",true));
	assert(Cvar_ReadState(reader,"engine.cvars.test",0,var->name,true,true));
	assert(!var->latchedString && var->integer==800);
	assert(Cvar_Get(var->name,"800",CVAR_LATCH)==var && var->integer==800);
	assert(Cvar_ReadState(reader,"engine.cvars.test",0,var->name,true) && !strcmp(var->latchedString,"600"));
	cvar_modifiedFlags = CVAR_ARCHIVE | CVAR_USERINFO;
	cvar_group[CVG_SERVER] = cvar_group[CVG_RENDERER] = 1;
	assert(Cvar_ReadServerState(reader,true));
	assert(cvar_modifiedFlags==(CVAR_ARCHIVE|CVAR_USERINFO|CVAR_SERVERINFO));
	assert(cvar_group[CVG_SERVER]==0 && cvar_group[CVG_RENDERER]==1);
	Cvar_Set2( var->name, "5000", qfalse );
	assert(!strcmp(var->latchedString,"2000") && var->integer==800 && cvar_group[CVG_SERVER]==1);
	Cvar_Unset( var );
	assert(allocations==0);
	Cvar_Get( "unrelated_context", "123", 0 );
	assert(Cvar_ReadState(reader,"engine.cvars.test",0,"checkpoint_gravity",true));
	var = Cvar_FindVar( "checkpoint_gravity" );
	assert(var && var!=&cvar_indexes[handle] && var->modificationCount==19 && var->integer==800);
	assert(Cvar_ReadState(reader,"engine.cvars.test",1,"checkpoint_absent",true));
	assert(!Cvar_FindVar("checkpoint_absent"));
	Cvar_Get( "checkpoint_absent", "1", CVAR_USER_CREATED );
	assert(Cvar_ReadState(reader,"engine.cvars.test",1,"checkpoint_absent",true) && !Cvar_FindVar("checkpoint_absent"));
	Cvar_Get( "checkpoint_absent", "1", CVAR_ROM );
	assert(!Cvar_ReadState(reader,"engine.cvars.test",1,"checkpoint_absent",true));
	cvarStateSave_t bad;
	uint32_t version;
	const stateSchema_t schema{ "engine.cvars.test", 1, 1, sizeof( bad ), cvarStateFields, ARRAY_LEN( cvarStateFields ) };
	assert(State_Find(reader,schema,0,&bad,&version));
	bad.validator = CV_MAX;
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,schema,0,&bad));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	const int before = allocations;
	assert(!Cvar_ReadState(reader,"engine.cvars.test",0,var->name,true) && allocations==before && var->integer==800);
#ifdef STATE_NATIVE_CACHE
	vmCvar_t cached{ 999, 17, 750, 750, "750" };
	const game::gCachedCvar_t binding{ "checkpoint_gravity", &cached };
	writer = { bytes, sizeof( bytes ) };
	assert(game::G_WriteCachedCvars(&writer,"game.cvars.integration",&binding,1));
	assert(State_Open(bytes,State_Finish(&writer),&reader) && reader.records==2);
	Cvar_Set( var->name, "300" );
	cached = { int( var - cvar_indexes ), 0, 0, 0, "" };
	assert(game::G_ReadCachedCvars(reader,"game.cvars.integration",&binding,1,true));
	assert(var->integer==800 && var->modificationCount==19 && cached.integer==750 && cached.modificationCount==17);
	assert(cached.handle==var-cvar_indexes);
	// The actual engine update sees the restored pending revision and refreshes
	// the native cache on the same next update as before saving.
	vmCvar_t update;
	static_assert( sizeof( update ) == sizeof( cached ) );
	memcpy( &update, &cached, sizeof( update ) );
	Cvar_Update( &update, 0 );
	assert(update.integer==800 && update.modificationCount==19 && !strcmp(update.string,"800"));
	game::bg_itemlist[1].classname = "item_health";
	game::bg_itemlist[2].classname = "weapon_test";
	auto *path = Cvar_Get( "g_weapons", "weapons/original.json", CVAR_LATCH );
	path->latchedString = CopyString( "weapons/pending.json" );
	Cvar_Get( "session3", "saved session", 0 );
	Cvar_Get( "disable_item_health", "1", CVAR_USER_CREATED );
	writer = { bytes, sizeof( bytes ) };
	assert(game::G_WriteExtraCvarState(&writer));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	Cvar_Set( "session3", "changed" );
	Cvar_Set( "botsession7", "created during shutdown" );
	Cvar_Set( "disable_item_health", "0" );
	assert(game::G_ReadExtraCvarState(reader,2));
	assert(!strcmp(path->string,"weapons/original.json") && !path->latchedString);
	assert(!strcmp(Cvar_VariableString("session3"),"saved session") && !Cvar_FindVar("botsession7"));
	assert(Cvar_VariableIntegerValue("disable_item_health")==1);
	assert(game::G_ReadExtraCvarState(reader,1));
	assert(!strcmp(path->latchedString,"weapons/pending.json"));
	puts( "PASS: native and engine cvar records retain a pending update across the service boundary" );
#endif
	while ( cvar_vars )
		Cvar_Unset( cvar_vars );
	assert(allocations==0);
	Cvar_Get( "capacity_one", "1", 0 );
	Cvar_Get( "capacity_two", "2", 0 );
	writer = { bytes, sizeof( bytes ) };
	assert(Cvar_WriteState(&writer,"engine.capacity",0,"capacity_one") && Cvar_WriteState(&writer,"engine.capacity",1,"capacity_two"));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	while ( cvar_vars )
		Cvar_Unset( cvar_vars );
	for ( int i = 0; i < MAX_CVARS - 1; ++i ) {
		char name[64];
		snprintf( name, sizeof( name ), "capacity_filler_%d", i );
		assert(Cvar_Get(name,"0",0));
	}
	const int capacityAllocations = allocations;
	assert(CapacityRecords(reader)); // Each record alone sees the same remaining slot.
	assert(!Cvar_CheckStateCapacity(reader,CapacityRecords));
	assert(Cvar_CheckStateCapacity(reader,DuplicateCapacityRecord));
	assert(!Cvar_CheckStateCapacity(reader,MutatingCapacityRecord));
	assert(allocations==capacityAllocations && !Cvar_FindVar("capacity_one"));
	Cvar_Unset( cvar_vars );
	assert(Cvar_CheckStateCapacity(reader,CapacityRecords));
	assert(Cvar_ReadState(reader,"engine.capacity",0,"capacity_one",true));
	assert(Cvar_ReadState(reader,"engine.capacity",1,"capacity_two",true));
	while ( cvar_vars )
		Cvar_Unset( cvar_vars );
	assert(allocations==0);
	puts( "PASS: aggregate cvar preparation rejects overcapacity without mutation and deduplicates names" );
	puts( "PASS: selected cvar values, latch, bounds and counters restore across fresh registry handles" );
}

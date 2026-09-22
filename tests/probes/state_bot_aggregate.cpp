#include "../../engine/qcommon/q_shared.h"
#include "../../engine/botlib/botlib_public.h"
#include "../../engine/botlib/be_interface.h"
#include "../../engine/botlib/l_libvar.h"
#include "../../engine/botlib/l_log.h"
#include "../../engine/botlib/be_ai_chat.h"
#include <cassert>
#include <cstdlib>

static void *hunks[256];
static int nhunks, allocations;
static void *Allocate( size_t n ) {
	++allocations;
	auto *p = malloc( n );
	assert(p);
	return p;
}
static void Release( void *p ) {
	assert(p);
	--allocations;
	free( p );
}
static void *Hunk( size_t n ) {
	assert(nhunks<256);
	return hunks[nhunks++] = Allocate( n );
}
static void ClearHunk() {
	while ( nhunks )
		Release( hunks[--nhunks] );
}
static void QDECL Print( int, const char *fmt, ... ) {
	va_list ap;
	va_start( ap, fmt );
	vfprintf( stderr, fmt, ap );
	va_end( ap );
}
// Synchronous parsers accept empty owned configs; no installed game assets needed.
static constexpr char source[] = "// empty owned bot configuration\n";
static int Open( const char *, fileHandle_t *f, fsMode_t m ) {
	assert(m==FS_READ);
	*f = 1;
	return sizeof( source ) - 1;
}
static int Read( void *out, int size, fileHandle_t f ) {
	assert(f==1 && size==sizeof(source)-1);
	memcpy( out, source, size );
	return size;
}
static void Close( fileHandle_t f ) {
	assert(f==1);
}
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}
void QDECL Com_Printf( const char *, ... ) {
}
int QDECL FS_OSPrintf( FILE *, const char *, ... ) {
	abort();
}
void Log_Open( const char * ) {
}
void Log_Shutdown() {
}
void QDECL Log_Write( const char *, ... ) {
}
void Log_Flush() {
}
FILE *Log_FilePointer() {
	return nullptr;
}
time_t Sys_Time( time_t * ) {
	abort();
}
const char *Sys_CTime( const time_t * ) {
	abort();
}

int main() {
	botlib_import_t imports{};
	imports.GetMemory = Allocate;
	imports.FreeMemory = Release;
	imports.HunkAlloc = Hunk;
	imports.Print = Print;
	imports.FS_FOpenFile = Open;
	imports.FS_Read = Read;
	imports.FS_FCloseFile = Close;
	auto *api = GetBotLibAPI( BOTLIB_API_VERSION, &imports );
	assert(api);
	static unsigned char bytes[8 * 1024 * 1024];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	stateReader_t reader;
	assert(BotLib_WriteState(&writer,100));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(BotLib_PrepareSettings(reader) && BotLib_PrepareState(reader,200) && BotLib_ReadState(reader,200,true));
	api->BotLibVarSet( "maxclients", "4" );
	api->BotLibVarSet( "maxentities", "64" );
	assert(api->BotLibSetup()==BLERR_NOERROR);
	int move = api->ai.BotAllocMoveState(), chat = api->ai.BotAllocChatState();
	assert(move==1 && chat==1);
	api->ea.EA_Jump( 2 );
	api->ai.BotQueueConsoleMessage( chat, 7, "checkpoint message" );
	writer = { bytes, sizeof( bytes ) };
	assert(BotLib_WriteState(&writer,100));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	api->ea.EA_ResetInput( 2 );
	api->ea.EA_Attack( 2 );
	assert(BotLib_ReadState(reader,100,false));
	bot_input_t changed{};
	api->ea.EA_GetInput( 2, 0, &changed );
	assert((changed.actionflags&ACTION_ATTACK) && !(changed.actionflags&ACTION_JUMP));
	assert(BotLib_ReadState(reader,100,true));
	api->ea.EA_GetInput( 2, 0, &changed );
	assert((changed.actionflags&ACTION_JUMP) && !(changed.actionflags&ACTION_ATTACK));
	assert(api->BotLibShutdown()==BLERR_NOERROR);
	ClearHunk();
	assert(!allocations);
	assert(BotLib_PrepareSettings(reader));
	assert(api->BotLibSetup()==BLERR_NOERROR);
	assert(BotLib_PrepareState(reader,200));
	assert(BotLib_ReadState(reader,200,true));
	bot_input_t input{};
	api->ea.EA_GetInput( 2, 0, &input );
	assert(input.actionflags&ACTION_JUMP);
	bot_consolemessage_qvm_t message{};
	assert(api->ai.BotNextConsoleMessage(chat,&message));
	assert(message.type==7 && !strcmp(message.message,"checkpoint message"));
	assert(api->ai.BotAllocMoveState()==2 && api->ai.BotAllocChatState()==2);
	assert(api->BotLibShutdown()==BLERR_NOERROR);
	ClearHunk();
	assert(!allocations);
	puts( "PASS: real botlib setup/shutdown reconstructs saved owners without AAS content" );
}

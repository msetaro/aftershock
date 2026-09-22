#include "../../engine/botlib/be_ai_weight.cpp"
#include <assert.h>
#include <stdlib.h>

botlib_import_t botimport{};
static char reloadName[] = "bot_reloadcharacters", reloadText[] = "0";
static libvar_t reload{ reloadName, reloadText, 17, qtrue, 0, nullptr };
libvar_t *LibVarGet( const char *name ) {
	assert(!strcmp(name,reloadName));
	return &reload;
}
float LibVarGetValue( const char *name ) {
	return LibVarGet( name )->value;
}
static int allocations;
void *GetMemory( size_t size ) {
	void *p = malloc( size );
	assert(p);
	++allocations;
	return p;
}
void *GetClearedMemory( size_t size ) {
	void *p = GetMemory( size );
	memset( p, 0, size );
	return p;
}
void FreeMemory( void *p ) {
	if ( p ) {
		--allocations;
		free( p );
	}
}
void Q_strncpyz( char *out, const char *in, int size ) {
	assert(size>0 && strlen(in)<size_t(size));
	strcpy( out, in );
}
int QDECL Com_sprintf( char *out, int size, const char *format, ... ) {
	va_list args;
	va_start( args, format );
	const int result = vsnprintf( out, size_t( size ), format, args );
	va_end( args );
	assert(result>=0 && result<size);
	return result;
}
// The fixed weight source has no includes or clock macros.
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}
int Q_stricmp( const char *, const char * ) {
	abort();
}
void Q_strcat( char *, int, const char * ) {
	abort();
}
time_t Sys_Time( time_t * ) {
	abort();
}
const char *Sys_CTime( const time_t * ) {
	abort();
}
static const char *sourceText = "weight \"choice\" return balance(7.5, 1, 12);";
static int Open( const char *path, fileHandle_t *file, fsMode_t mode ) {
	assert(!strcmp(path,"botfiles/checkpoint_w.c") && mode==FS_READ);
	*file = 1;
	return int( strlen( sourceText ) );
}
static int Read( void *out, int size, fileHandle_t file ) {
	assert(file==1 && size==int(strlen(sourceText)));
	memcpy( out, sourceText, size_t( size ) );
	return size;
}
static void Close( fileHandle_t file ) {
	assert(file==1);
}
static void QDECL Print( int, const char *, ... ) {
}
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

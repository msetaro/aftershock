#include "../../engine/botlib/be_ai_char.cpp"
#include <assert.h>
#include <cstddef>
botlib_import_t botimport;
static uint32_t clockTime;
static void *freed;
static int Clock() {
	return int32_t( clockTime );
}
void FreeMemory( void *memory ) {
	assert(memory);
	freed = memory;
}
void *GetMemory(size_t size) {
	alignas( std::max_align_t ) static unsigned char memory[65536];
	static size_t offset;
	const size_t start = offset;
	offset += ( size + alignof( std::max_align_t ) - 1 ) / alignof( std::max_align_t ) * alignof( std::max_align_t );
	assert(offset<=sizeof(memory));
	return memory + start;
}
void *GetClearedMemory(size_t size) {
	void *memory = GetMemory(size);
	memset( memory, 0, size );
	return memory;
}
int main() {
	botimport.Sys_Milliseconds = Clock;
	bot_character_t original[3]{};
	char sourceName[] = "bots/checkpoint_i.c", loadedName[] = "bots/checkpoint_i.c";
	for ( auto &character : original ) {
		strcpy( character.filename, "bots/checkpoint_c.c" );
		character.skill = 3.5f;
		character.c[1].type = CT_INTEGER;
		character.c[1].value.integer = 24;
		character.c[2].type = CT_FLOAT;
		character.c[2].value._float = 0.5f;
		character.c[3].type = CT_STRING;
		character.c[3].value.string = sourceName;
	}
	original[0].reftime = 200;
	original[1].reftime = 900;
	original[2].refcnt = 2;
	original[2].reftime = 5000;
	botcharacters[1] = &original[0];
	botcharacters[4] = &original[1];
	botcharacters[9] = &original[2];
	clockTime = 1000;
	static unsigned char bytes[65536];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(Bot_WriteCharacterState(&writer,clockTime));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	uint32_t present[MAX_HANDLES + 1], version;
	characterSave_t saved[3];
	assert(State_Find(reader,characterPoolSchema,0,present,&version));
	assert(State_Find(reader,characterSaveSchema,1,&saved[0],&version));
	assert(State_Find(reader,characterSaveSchema,4,&saved[1],&version));
	assert(State_Find(reader,characterSaveSchema,9,&saved[2],&version));
	const int expected = BotReleaseUnreferencedHandle();
	assert(expected==1 && freed==&original[0]);
	bot_character_t restored[3];
	memcpy( restored, original, sizeof( restored ) );
	for ( auto &character : restored ) {
		character.c[3].value.string = loadedName;
		character.refcnt = 99;
		character.reftime = 0;
	}
	botcharacters[1] = &restored[0];
	botcharacters[4] = &restored[1];
	botcharacters[9] = &restored[2];
	clockTime = 100000;
	assert(Bot_ReadCharacterState(reader,clockTime,false) && restored[0].refcnt==99);
	assert(Bot_ReadCharacterState(reader,clockTime,true));
	assert(restored[0].refcnt==0 && restored[0].reftime==99200);
	assert(restored[1].refcnt==0 && restored[1].reftime==99900);
	assert(restored[2].refcnt==2 && restored[2].reftime==104000);
	assert(Characteristic_Float(9,2)==0.5f);
	assert(BotReleaseUnreferencedHandle()==expected && freed==&restored[0]);
	botcharacters[1] = &restored[0];
	restored[2].c[2].value._float = 0.75f;
	restored[0].refcnt = 77;
	assert(!Bot_ReadCharacterState(reader,clockTime,true) && restored[0].refcnt==77);
	restored[2].c[2].value._float = 0.5f;
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,characterPoolSchema,0,present));
	assert(State_Append(&writer,characterSaveSchema,1,&saved[0]));
	assert(State_Append(&writer,characterSaveSchema,4,&saved[1]));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(!Bot_ReadCharacterState(reader,clockTime,true) && restored[0].refcnt==77);
	restored[0].refcnt = 0;
	restored[0].reftime = -64;
	writer = { bytes, sizeof( bytes ) };
	assert(Bot_WriteCharacterState(&writer,32));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(Bot_ReadCharacterState(reader,5,true) && restored[0].reftime==-91);

	assert(!Bot_PrepareCharacterState(reader,5));
	for ( auto &character : botcharacters )
		character = nullptr;
	assert(Bot_PrepareCharacterState(reader,5));
	assert(botcharacters[1]!=&restored[0] && botcharacters[1]->reftime==-91 && botcharacters[1]->refcnt==0);
	assert(botcharacters[4] && botcharacters[9] && !botcharacters[2]);
	assert(botcharacters[1]->c[3].value.string!=loadedName && !strcmp(botcharacters[1]->c[3].value.string,loadedName));
	assert(Characteristic_Float(9,2)==0.5f);
	assert(Bot_ReadCharacterState(reader,5,false));
	// Truncation by missing a later resolved-attribute record cannot publish an
	// earlier character in an empty destination pool.
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,characterPoolSchema,0,present));
	assert(State_Append(&writer,characterSaveSchema,1,&saved[0]));
	assert(CharacterValuesRecord(&writer,nullptr,1,botcharacters[1],nullptr));
	assert(State_Append(&writer,characterSaveSchema,4,&saved[1]));
	for ( auto &character : botcharacters )
		character = nullptr;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(!Bot_PrepareCharacterState(reader,5) && !botcharacters[1]);
	puts( "PASS: resolved character attributes reconstruct exact empty-pool slots with owned strings before clock rebasing" );
	puts( "PASS: relocated character caches preserve attributes, reference counts and clock-rebased eviction order" );
}

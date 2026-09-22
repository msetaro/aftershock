#include "../../engine/botlib/l_libvar.cpp"
#include <assert.h>
#include <cstddef>
void *GetMemory(size_t size) {
	alignas( std::max_align_t ) static unsigned char memory[65536];
	static size_t offset;
	const size_t start = offset;
	offset += ( size + alignof( std::max_align_t ) - 1 ) / alignof( std::max_align_t ) * alignof( std::max_align_t );
	assert(offset<=sizeof(memory));
	return memory + start;
}
void FreeMemory( void * ) {
}
int Q_stricmp( const char *left, const char *right ) {
	return strcasecmp( left, right );
}
int main() {
	auto *first = LibVar( "first", "3.25" );
	first->modified = qfalse;
	first->flags = 7;
	LibVar( "second", "file/path" );
	auto *third = LibVar( "third", "8" );
	static unsigned char bytes[8192];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(LibVar_WriteState(&writer));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	LibVarSet( "first", "9" );
	assert(LibVar_ReadState(reader,false) && first->value==9);
	assert(LibVar_ReadState(reader,true));
	assert(LibVarGet("first")==first && first->value==3.25f && first->flags==7 && first->modified==qfalse);
	assert(libvarlist==third && !strcmp(libvarlist->next->name,"second") && libvarlist->next->next==first);
	// Empty fresh-process storage reconstructs names/order, preserving an already
	// cached handle if only a subset has been initialized in a different order.
	libvarlist = nullptr;
	auto *retained = LibVar( "FIRST", "0" );
	assert(LibVar_ReadState(reader,true) && LibVarGet("first")==retained && retained->value==3.25f);
	assert(!strcmp(libvarlist->name,"third") && !strcmp(libvarlist->next->name,"second"));
	const uint32_t count = 3;
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,libvarCountSchema,0,&count));
	assert(State_Append(&writer,libvarSchema,0,&savedLibVars[0]));
	assert(State_Append(&writer,libvarSchema,1,&savedLibVars[1]));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	LibVarSet( "first", "11" );
	assert(!LibVar_ReadState(reader,true) && retained->value==11);
	writer = { bytes, sizeof( bytes ) };
	assert(LibVar_WriteState(&writer));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	LibVar( "unexpected", "1" );
	assert(!LibVar_ReadState(reader,true) && retained->value==11);
	libvarlist = libvarlist->next;
	strcpy( savedLibVars[1].name, savedLibVars[0].name );
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,libvarCountSchema,0,&count));
	for ( uint32_t i = 0; i < count; ++i )
		assert(State_Append(&writer,libvarSchema,i,&savedLibVars[i]));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(!LibVar_ReadState(reader,true) && retained->value==11);
	first = libvarlist;
	first->next = first;
	writer = { bytes, sizeof( bytes ) };
	assert(!LibVar_WriteState(&writer) && !State_Finish(&writer));
	puts( "PASS: bot variables restore order and numeric/modified state while preserving cached handles and rejecting incomplete drafts" );
}

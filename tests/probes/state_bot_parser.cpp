#include "../../engine/botlib/l_precomp.cpp"
#include <assert.h>
void Q_strncpyz( char *out, const char *in, int size ) {
	assert(size>0 && strlen(in)<size_t(size));
	strcpy( out, in );
}
int main() {
	PS_SetBaseFolder( "botfiles" );
	static unsigned char bytes[4096];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(PC_WriteState(&writer));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	PS_SetBaseFolder( "" );
	assert(PC_ReadState(reader,false));
	assert(PC_ReadState(reader,true));
	static unsigned char after[4096];
	stateWriter_t restored{ after, sizeof( after ) };
	assert(PC_WriteState(&restored));
	const size_t size = State_Finish( &restored );
	assert(size==writer.size && !memcmp(bytes,after,size));
	source_t source{};
	sourceFiles[1] = &source;
	restored = { after, sizeof( after ) };
	assert(!PC_WriteState(&restored) && !State_Finish(&restored) && !PC_ReadState(reader,true));
	sourceFiles[1] = nullptr;
	numtokens = 1;
	restored = { after, sizeof( after ) };
	assert(!PC_WriteState(&restored) && !State_Finish(&restored));
	numtokens = 0;
	define_t define{};
	globaldefines = &define;
	restored = { after, sizeof( after ) };
	assert(!PC_WriteState(&restored) && !State_Finish(&restored));
	globaldefines = nullptr;
	puts( "PASS: bot parser search-folder state resumes only at a quiescent native-game boundary" );
}

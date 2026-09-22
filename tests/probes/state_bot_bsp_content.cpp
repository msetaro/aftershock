#include "../../engine/botlib/be_aas_bspq3.cpp"
#include <assert.h>
int main() {
	char text[] = "{\"message\" \"Checkpoint\"}", key[] = "message", value[] = "Checkpoint";
	bspworld.loaded = 1;
	bspworld.entdatasize = sizeof( text );
	bspworld.dentdata = text;
	bspworld.numentities = 2;
	bsp_epair_t pair{ key, value, nullptr };
	bspworld.entities[1].epairs = &pair;
	static unsigned char bytes[4096];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(AAS_WriteBSPContentState(&writer));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	char loadedText[sizeof( text )], loadedKey[sizeof( key )], loadedValue[sizeof( value )];
	memcpy( loadedText, text, sizeof( text ) );
	memcpy( loadedKey, key, sizeof( key ) );
	memcpy( loadedValue, value, sizeof( value ) );
	bsp_epair_t loadedPair{ loadedKey, loadedValue, nullptr };
	bspworld.entities[1].epairs = &loadedPair;
	bspworld.dentdata = loadedText;
	assert(AAS_ReadBSPContentState(reader));
	loadedValue[0] = 'X';
	assert(!AAS_ReadBSPContentState(reader));
	loadedValue[0] = 'C';
	loadedText[1] = ' ';
	assert(!AAS_ReadBSPContentState(reader));
	loadedText[1] = '"';
	assert(AAS_ReadBSPContentState(reader));
	puts( "PASS: bot BSP entity text and parsed epair identity survive relocation and reject different map content" );
}

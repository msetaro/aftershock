#include "../../engine/botlib/be_ai_goal.cpp"
#include <assert.h>
int main() {
	maplocation_t locations[2]{};
	locations[0].origin[0] = 128;
	locations[0].areanum = 2;
	strcpy( locations[0].name, "Bridge" );
	locations[0].next = &locations[1];
	locations[1].origin[2] = 16;
	locations[1].areanum = 3;
	strcpy( locations[1].name, "Hall" );
	maplocations = locations;
	campspot_t camp{};
	camp.origin[1] = 12;
	camp.areanum = 4;
	camp.range = 200;
	camp.weight = 1;
	camp.wait = 3;
	camp.random = 0.5f;
	strcpy( camp.name, "Camp" );
	campspots = &camp;
	static unsigned char bytes[8192];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(Bot_WriteGoalMapState(&writer));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	maplocation_t loaded[2];
	memcpy( loaded, locations, sizeof( loaded ) );
	loaded[0].next = &loaded[1];
	maplocations = loaded;
	campspot_t loadedCamp = camp;
	campspots = &loadedCamp;
	assert(Bot_ReadGoalMapState(reader));
	loadedCamp.random = 0.25f;
	assert(!Bot_ReadGoalMapState(reader));
	loadedCamp.random = 0.5f;
	loaded[1].areanum = 6;
	assert(!Bot_ReadGoalMapState(reader));
	loaded[1].areanum = 3;
	loaded[1].name[0] = 'X';
	assert(!Bot_ReadGoalMapState(reader));
	loaded[1].name[0] = 'H';
	loaded[1].next = &loaded[0];
	writer = { bytes, sizeof( bytes ) };
	assert(!Bot_WriteGoalMapState(&writer) && !State_Finish(&writer));
	puts( "PASS: immutable map locations and camp descriptors match after relocation and reject changed content/order" );
}

#define NATIVE_NAMESPACE game
#define NATIVE_SOURCE "game/g_rewind.cpp"
#include "../../game/module.cpp"

int main() {
	static netHistory_t original, restored;
	static netHistoryFrame_t frame;
	game::rewindHistory = &original;
	game::useRewind = true;
	NET_HistoryReset( &original );
	const uint64_t generation = UINT64_C( 0xfedcba9800001234 );
	for ( uint32_t i = 0; i < 70; ++i ) {
		frame = {};
		frame.time = UINT32_MAX - 999 + i * 20;
		frame.boxCount = 1;
		frame.entities[3] = { generation, 0, 1 };
		frame.boxes[0] = { { float( i ), -1, -2 }, { float( i ) + 2, 3, 4 } };
		assert(NET_HistoryStore(&original,&frame));
	}
	game::nextGeneration = generation + 1;
	for ( uint32_t i = 0; i < MAX_GENTITIES; ++i ) {
		game::rewindEntities[i] = { i, int( i + 7 ), int( i & 1 ), generation };
	}
	for ( uint32_t i = 0; i < MAX_CLIENTS; ++i )
		game::lastRewindReport[i] = i * 20;
	static unsigned char entities[sizeof( game::rewindEntities )];
	memcpy( entities, game::rewindEntities, sizeof( entities ) );
	static unsigned char archive[8388608];
	stateWriter_t writer{ archive, sizeof( archive ) };
	assert(game::G_WriteRewindState(&writer));
	const size_t size = State_Finish( &writer );
	stateReader_t reader;
	assert(size&&State_Open(archive,size,&reader));
	netHistoryQuery_t query;
	netBox_t before, after, resumed;
	assert(NET_HistoryQuery(&original,380,345,200,&query));
	assert(NET_HistoryEntity(&query,3,generation,&before,1)==1);
	frame.time = 400;
	frame.boxes[0] = { { 70, -1, -2 }, { 72, 3, 4 } };
	assert(NET_HistoryStore(&original,&frame));
	assert(NET_HistoryQuery(&original,400,385,200,&query));
	assert(NET_HistoryEntity(&query,3,generation,&after,1)==1);
	game::rewindHistory = &restored;
	NET_HistoryReset( &restored );
	memset( game::rewindEntities, 0, sizeof( game::rewindEntities ) );
	memset( game::lastRewindReport, 0, sizeof( game::lastRewindReport ) );
	game::nextGeneration = 0;
	assert(game::G_ReadRewindState(reader,false)&&!restored.count&&!game::nextGeneration);
	assert(game::G_ReadRewindState(reader,true));
	assert(game::nextGeneration==generation+1&&!memcmp(entities,game::rewindEntities,sizeof(entities)));
	for ( uint32_t i = 0; i < MAX_CLIENTS; ++i )
		assert(game::lastRewindReport[i]==i*20);
	assert(NET_HistoryQuery(&restored,380,345,200,&query));
	assert(NET_HistoryEntity(&query,3,generation,&resumed,1)==1&&!memcmp(&before,&resumed,sizeof(before)));
	assert(NET_HistoryStore(&restored,&frame));
	assert(NET_HistoryQuery(&restored,400,385,200,&query));
	assert(NET_HistoryEntity(&query,3,generation,&resumed,1)==1&&!memcmp(&after,&resumed,sizeof(after)));
	static unsigned char partial[131072];
	stateWriter_t incomplete{ partial, sizeof( partial ) };
	game::rewindSave_t saved;
	static game::rewindFrameSave_t first;
	uint32_t version;
	assert(State_Find(reader,game::rewindSchema,0,&saved,&version));
	assert(State_Append(&incomplete,game::rewindSchema,0,&saved));
	assert(State_Find(reader,game::rewindFrameSchema,0,&first,&version));
	assert(State_Append(&incomplete,game::rewindFrameSchema,0,&first));
	const size_t partialSize = State_Finish( &incomplete );
	stateReader_t missing;
	assert(partialSize&&State_Open(partial,partialSize,&missing));
	assert(!game::G_ReadRewindState(missing,true));
	assert(restored.frames[6].time==400&&restored.next==7);
	game::useRewind = false;
	assert(!game::G_ReadRewindState(reader,true)&&restored.next==7);
	game::useRewind = true;
	NET_HistoryReset( &restored );
	restored.frames[17].boxCount = NET_HISTORY_BOXES + 1; // Unreachable old storage is not a save record.
	writer = { archive, sizeof( archive ) };
	assert(game::G_WriteRewindState(&writer));
	const size_t emptySize = State_Finish( &writer );
	assert(emptySize&&emptySize<32768);
	puts( "PASS: rewind ring, spawn generations and report clocks restore across clock wrap and resume identical queries" );
}

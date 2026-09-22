#define NATIVE_NAMESPACE game
#if defined( STATE_QUEUE )
#define NATIVE_SOURCE "game/g_bot.cpp"
#elif defined( STATE_CLOCK )
#define NATIVE_SOURCE "game/ai_main.cpp"
#else
#define NATIVE_SOURCE "game/ai_team.cpp"
#endif
#include "../../game/module.cpp"
#ifdef STATE_TEAM
namespace game {
int notleader[MAX_CLIENTS];
}
#endif
int main() {
	using namespace game;
	static unsigned char bytes[32768];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	stateReader_t reader;
#ifdef STATE_QUEUE
	checkminimumplayers_time = INT32_MIN + 700;
	for ( int i = 0; i < BOT_SPAWN_QUEUE_DEPTH; ++i )
		botSpawnQueue[i] = { i, 1200 + i * 50 };
	assert(G_WriteBotQueueState(&writer));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	G_RemoveQueuedBotBegin( 7 );
	checkminimumplayers_time = 0;
	assert(G_ReadBotQueueState(reader,false) && !botSpawnQueue[7].spawnTime);
	assert(G_ReadBotQueueState(reader,true));
	assert(checkminimumplayers_time==INT32_MIN+700 && botSpawnQueue[7].spawnTime==1550);
	G_RemoveQueuedBotBegin( 7 );
	assert(!botSpawnQueue[7].spawnTime && botSpawnQueue[8].spawnTime==1600);
	botSpawnQueue[15].clientNum = MAX_CLIENTS;
	writer = { bytes, sizeof( bytes ) };
	assert(!G_WriteBotQueueState(&writer) && !State_Finish(&writer));
	botQueueSave_t invalid{};
	invalid.clients[15] = -1;
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,botQueueSchema,0,&invalid));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(!G_ReadBotQueueState(reader,true) && checkminimumplayers_time==INT32_MIN+700);
	puts( "PASS: queued bot spawns and minimum-player clock survive checkpoint continuation" );
#elif defined( STATE_CLOCK )
	numbots = 3;
	floattime = 123.25f;
	regularupdate_time = 123.5f;
	bot_interbreed = 1;
	bot_interbreedmatchcount = 7;
	local_time = INT32_MIN + 12;
	botlib_residual = 19;
	lastbotthink_time = 100;
	assert(G_WriteBotClockState(&writer));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	numbots = 0;
	floattime = 0;
	regularupdate_time = 0;
	local_time = 0;
	assert(G_ReadBotClockState(reader,false) && !numbots && !local_time);
	assert(G_ReadBotClockState(reader,true));
	assert(numbots==3 && floattime==123.25f && regularupdate_time==123.5f && bot_interbreed==1 && bot_interbreedmatchcount==7);
	assert(local_time==INT32_MIN+12 && botlib_residual==19 && lastbotthink_time==100);
	botClockSave_t invalid{};
	invalid.numbots = MAX_CLIENTS + 1;
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,botClockSchema,0,&invalid));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(!G_ReadBotClockState(reader,true) && numbots==3 && floattime==123.25f);
	puts( "PASS: bot scheduler and interbreeding clocks retain exact saved values" );
#else
	for ( int i = 0; i < MAX_CLIENTS; ++i ) {
		snprintf( ctftaskpreferences[i].name, sizeof( ctftaskpreferences[i].name ), "player_%d", i );
		ctftaskpreferences[i].preference = i % 4;
		notleader[i] = i % 2;
	}
	assert(G_WriteBotTeamState(&writer));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	BotResetTeamPreferences();
	memset( notleader, 0, sizeof( notleader ) );
	assert(G_ReadBotTeamState(reader,false) && !ctftaskpreferences[1].name[0]);
	assert(G_ReadBotTeamState(reader,true));
	for ( int i = 0; i < MAX_CLIENTS; ++i ) {
		char name[36];
		snprintf( name, sizeof( name ), "player_%d", i );
		assert(!strcmp(ctftaskpreferences[i].name,name) && ctftaskpreferences[i].preference==i%4 && notleader[i]==i%2);
	}
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,botLeaderSchema,0,notleader));
	assert(State_Append(&writer,botTeamSchema,0,&ctftaskpreferences[0]));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	ctftaskpreferences[0].preference = 3;
	assert(!G_ReadBotTeamState(reader,true) && ctftaskpreferences[0].preference==3);
	notleader[63] = 2;
	writer = { bytes, sizeof( bytes ) };
	assert(!G_WriteBotTeamState(&writer) && !State_Finish(&writer));
	puts( "PASS: all bot team preferences and leader exclusions validate before publication" );
#endif
}

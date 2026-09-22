#define NATIVE_NAMESPACE game
#if defined( STATE_COMBAT )
#define NATIVE_SOURCE "game/g_combat.cpp"
#elif defined( STATE_TEAM )
#define NATIVE_SOURCE "game/g_team.cpp"
#else
#define NATIVE_SOURCE "game/g_arenas.cpp"
#endif
#include "../../game/module.cpp"
int main() {
	using namespace game;
	unsigned char bytes[8192];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	stateReader_t reader;
#if defined( STATE_COMBAT )
	deathAnimationIndex = 2;
	assert(G_WriteCombatState(&writer));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	G_InitCombat();
	assert(G_ReadCombatState(reader,false) && deathAnimationIndex==0);
	assert(G_ReadCombatState(reader,true) && deathAnimationIndex==2);
	int32_t bad = 3;
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,combatSchema,0,&bad));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(!G_ReadCombatState(reader,true) && deathAnimationIndex==2);
	puts( "PASS: saved death animation cycle validates before restoration" );
#else
	static gentity_t original[9], restored[9];
	gStatePools_t before{ original, 9, nullptr, 0, nullptr, 0 }, after{ restored, 9, nullptr, 0, nullptr, 0 };
#if defined( STATE_TEAM )
	teamgame = { -0.0f, TEAM_BLUE, FLAG_TAKEN, FLAG_DROPPED, FLAG_TAKEN_RED, INT32_MIN, 900, 1200, 1700 };
	const auto expected = teamgame;
	neutralObelisk = &original[8];
	assert(G_WriteTeamState(&writer,before));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	teamgame = {};
	neutralObelisk = nullptr;
	assert(G_ReadTeamState(reader,after,false) && !neutralObelisk);
	assert(G_ReadTeamState(reader,after,true));
	assert(!memcmp(&teamgame,&expected,sizeof(expected)) && neutralObelisk==&restored[8]);
	teamSave_t bad{};
	bad.redStatus = UINT32_MAX;
	bad.neutral = -1;
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,teamSaveSchema,0,&bad));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(!G_ReadTeamState(reader,after,true));
	assert(!memcmp(&teamgame,&expected,sizeof(expected)) && neutralObelisk==&restored[8]);
	writer = { bytes, sizeof( bytes ) };
	assert(!G_WriteTeamState(&writer,before) && !State_Finish(&writer));
	puts( "PASS: team clocks, flag statuses and neutral entity restore into relocated pools" );
#else
	podium1 = &original[7];
	podium2 = nullptr;
	podium3 = &original[5];
	assert(G_WritePodiumState(&writer,before));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	podium1 = podium2 = podium3 = nullptr;
	assert(G_ReadPodiumState(reader,after,false) && !podium1);
	assert(G_ReadPodiumState(reader,after,true));
	assert(podium1==&restored[7] && !podium2 && podium3==&restored[5]);
	podiumSave_t bad{ { 0, -1, 9 } };
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,podiumSchema,0,&bad));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(!G_ReadPodiumState(reader,after,true));
	assert(podium1==&restored[7] && !podium2 && podium3==&restored[5]);
	writer = { bytes, sizeof( bytes ) };
	assert(!G_WritePodiumState(&writer,before) && !State_Finish(&writer));
	puts( "PASS: podium references rebind and reject unknown slots before mutation" );
#endif
#endif
}

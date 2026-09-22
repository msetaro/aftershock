#define NATIVE_NAMESPACE game
#define NATIVE_SOURCE "game/ai_dmq3.cpp"
#include "../../game/module.cpp"
namespace game {
void Q_strncpyz( char *out, const char *text, int capacity ) {
	assert(capacity>0 && strlen(text)<size_t(capacity));
	strcpy( out, text );
}
void QDECL BotAI_Print( int, char *, ... ) {
	assert(false);
}
} // namespace game
int main() {
	using namespace game;
	BotInitWaypoints();
	vec3_t origin{ 12, -8, 64 };
	auto *first = BotCreateWayPoint( (char *)"first", origin, 19 );
	auto *second = BotCreateWayPoint( (char *)"second", origin, 23 );
	assert(G_StateWaypointSlot(first)==127 && G_StateWaypointSlot(second)==126);
	first->next = second;
	second->prev = first;
	const int freeSlot = G_StateWaypointSlot( botai_freewaypoints );
	static unsigned char bytes[262144];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(G_WriteWaypointState(&writer));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	auto *continued = BotCreateWayPoint( (char *)"continued", origin, 31 );
	const int expectedSlot = G_StateWaypointSlot( continued );
	const auto expected = *continued;
	memset( botai_waypoints, 0, sizeof( botai_waypoints ) );
	botai_freewaypoints = nullptr;
	assert(G_ReadWaypointState(reader,false) && !botai_freewaypoints);
	assert(G_ReadWaypointState(reader,true));
	assert(G_StateWaypointSlot(botai_freewaypoints)==freeSlot);
	assert(botai_waypoints[127].next==&botai_waypoints[126] && botai_waypoints[126].prev==&botai_waypoints[127]);
	continued = BotCreateWayPoint( (char *)"continued", origin, 31 );
	assert(G_StateWaypointSlot(continued)==expectedSlot && !memcmp(continued,&expected,sizeof(expected)));
	waypointLinksSave_t invalid;
	invalid.free = 0;
	for ( int i = 0; i < MAX_WAYPOINTS; ++i )
		invalid.next[i] = invalid.prev[i] = -1;
	invalid.next[0] = 1;
	invalid.next[1] = 0;
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,waypointLinksSchema,0,&invalid));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(!G_ReadWaypointState(reader,true));
	assert(!memcmp(continued,&expected,sizeof(expected)));
	bot_waypoint_t external{};
	assert(G_StateWaypointSlot(&external)==-2 && G_StateWaypointSlot(nullptr)==-1);
	botai_freewaypoints = &external;
	writer = { bytes, sizeof( bytes ) };
	assert(!G_WriteWaypointState(&writer) && !State_Finish(&writer));
	puts( "PASS: waypoint chains and free-list order restore before identical next allocation" );

	gametype = GT_CTF;
	maxclients = 4;
	max_bspmodelindex = 11;
	altroutegoals_setup = 1;
	lastteleport_time = 73.25f;
	lastteleport_origin[0] = -0.0f;
	lastteleport_origin[1] = 125.5f;
	ctf_redflag.areanum = 9;
	ctf_blueflag.areanum = 19;
	ctf_redflag.entitynum = 84;
	ctf_blueflag.entitynum = 85;
	red_numaltroutegoals = 1;
	blue_numaltroutegoals = 3;
	red_altroutegoals[0] = { { 1, 2, 3 }, 17, UINT16_MAX, 123, 456 };
	for ( int i = 0; i < 3; ++i )
		blue_altroutegoals[i] = { { -4, 5, -6 }, 19 + i, uint16_t( 10 + i ), uint16_t( 20 + i ), uint16_t( 30 + i ) };
	const auto red = red_altroutegoals[0];
	aas_altroutegoal_t blue[3];
	memcpy( blue, blue_altroutegoals, sizeof( blue ) );
	writer = { bytes, sizeof( bytes ) };
	assert(G_WriteBotNavigationState(&writer));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	altroutegoals_setup = 0;
	lastteleport_time = 0;
	red_numaltroutegoals = blue_numaltroutegoals = 0;
	ctf_redflag = {};
	ctf_blueflag = {};
	assert(G_ReadBotNavigationState(reader,false) && !altroutegoals_setup);
	assert(G_ReadBotNavigationState(reader,true));
	assert(altroutegoals_setup==1 && lastteleport_time==73.25f && std::signbit(lastteleport_origin[0]));
	assert(ctf_redflag.areanum==9 && ctf_blueflag.entitynum==85);
	assert(red_numaltroutegoals==1 && blue_numaltroutegoals==3);
	assert(!memcmp(&red,&red_altroutegoals[0],sizeof(red)) && !memcmp(blue,blue_altroutegoals,sizeof(blue)));
	maxclients = 5;
	assert(!G_ReadBotNavigationState(reader,true) && maxclients==5 && red_numaltroutegoals==1);
	maxclients = 4;
	// A later missing team route record must not publish earlier teleport/goal data.
	botNavigationSave_t navigation{};
	botRoutesSave_t route{};
	uint32_t version;
	assert(State_Find(reader,botNavigationSchema,0,&navigation,&version));
	assert(State_Find(reader,botRoutesSchema,0,&route,&version));
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,botNavigationSchema,0,&navigation));
	for ( uint32_t i = 0; i < sizeof( botMapGoals ) / sizeof( botMapGoals[0] ); ++i )
		assert(State_Append(&writer,botGoalSchema,128+i,botMapGoals[i]));
	assert(State_Append(&writer,botRoutesSchema,0,&route));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	lastteleport_time = 999;
	assert(!G_ReadBotNavigationState(reader,true) && lastteleport_time==999);
	puts( "PASS: map bot goals, teleport memory and 16-bit alternative-route costs survive checkpoint restore" );
}

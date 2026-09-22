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
}

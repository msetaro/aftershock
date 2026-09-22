#define NATIVE_NAMESPACE game
#define NATIVE_SOURCE "game/ai_main.cpp"
#include "../../game/module.cpp"
namespace game {
bool G_NavigationEnabled() {
	return false;
}
static bot_waypoint_t originalWaypoints[128], restoredWaypoints[128];
static bot_waypoint_t *waypoints = originalWaypoints;
int G_StateWaypointSlot( const bot_waypoint_t *pointer ) {
	if ( !pointer )
		return -1;
	for ( int i = 0; i < 128; ++i )
		if ( pointer == &waypoints[i] )
			return i;
	return -2;
}
bot_waypoint_t *G_StateWaypoint( int slot ) {
	return slot >= 0 && slot < 128 ? &waypoints[slot] : nullptr;
}
int AINode_Intermission( bot_state_t * ) {
	return 0;
}
int AINode_Observer( bot_state_t * ) {
	return 1;
}
int AINode_Respawn( bot_state_t * ) {
	return 2;
}
int AINode_Stand( bot_state_t * ) {
	return 3;
}
int AINode_Seek_ActivateEntity( bot_state_t * ) {
	return 4;
}
int AINode_Seek_NBG( bot_state_t * ) {
	return 5;
}
int AINode_Seek_LTG( bot_state_t * ) {
	return 6;
}
int AINode_Battle_Fight( bot_state_t * ) {
	return 7;
}
int AINode_Battle_Chase( bot_state_t * ) {
	return 8;
}
int AINode_Battle_Retreat( bot_state_t * ) {
	return 9;
}
int AINode_Battle_NBG( bot_state_t * ) {
	return 10;
}
} // namespace game
static game::bot_state_t restoredActors[2];
static int levelAllocations;
static bool handlesExist = true;
int GameImport_ValidateBotHandles( int character, int move, int goal, int weapon, int chat ) {
	if ( !handlesExist || character != 17 )
		return 0;
	const int handles[] = { move, goal, weapon, chat };
	for ( int handle : handles )
		if ( handle != 17 && handle != 18 )
			return 0;
	return 1;
}
void *GameImport_AllocLevelMemory( uint32_t bytes ) {
	assert(bytes==sizeof(restoredActors));
	++levelAllocations;
	return restoredActors;
}
static int UnknownNode( game::bot_state_t * ) {
	return -1;
}
int main() {
	using namespace game;
	bot_state_t original{};
	for ( const auto &field : botActorFields ) {
		auto *destination = (uint8_t *)&original + field.offset;
		for ( uint32_t i = 0; i < field.count; ++i ) {
			if ( field.type == stateType_t::Int32 ) {
				const int32_t value = int32_t( i + 17 );
				memcpy( destination + i * 4, &value, 4 );
			}
			if ( field.type == stateType_t::Float32 ) {
				const float value = float( i ) + 0.25f;
				memcpy( destination + i * 4, &value, 4 );
			}
		}
	}
	original.inuse = 1;
	original.client = original.entitynum = 3;
	original.numproxmines = MAX_PROXMINES;
	strcpy( original.settings.characterfile, "bots/sarge_c.c" );
	strcpy( original.settings.team, "red" );
	strcpy( original.teamleader, "leader" );
	strcpy( original.subteam, "attackers" );
	strcpy( original.formation_teammate, "partner" );
	original.cur_ps.origin[1] = -123.5f;
	original.lastucmd.buttons = 0x700001;
	original.lastucmd.forwardmove = -64;
	original.checkpoints = &waypoints[9];
	original.patrolpoints = original.curpatrolpoint = &waypoints[11];
	original.activatestack = &original.activategoalheap[2];
	original.activategoalheap[2].inuse = 1;
	original.activategoalheap[2].next = &original.activategoalheap[5];
	original.activategoalheap[2].time = 123.75f;
	original.activategoalheap[2].numareas = 2;
	original.activategoalheap[2].areas[0] = 19;
	original.activategoalheap[2].areas[1] = 31;
	original.activategoalheap[5].inuse = 1;
	original.activategoalheap[5].target[2] = 200;
	static unsigned char bytes[131072];
	for ( const auto &node : botNodes ) {
		waypoints = originalWaypoints;
		original.ainode = node.node;
		stateWriter_t writer{ bytes, sizeof( bytes ) };
		assert(G_WriteBotActorState(&writer,3,original));
		stateReader_t reader;
		assert(State_Open(bytes,State_Finish(&writer),&reader));
		waypoints = restoredWaypoints;
		bot_state_t restored{};
		assert(G_ReadBotActorState(reader,3,&restored,false) && !restored.inuse);
		assert(G_ReadBotActorState(reader,3,&restored,true));
		auto expected = original;
		expected.checkpoints = &waypoints[9];
		expected.patrolpoints = expected.curpatrolpoint = &waypoints[11];
		expected.activatestack = &restored.activategoalheap[2];
		expected.activategoalheap[2].next = &restored.activategoalheap[5];
		assert(!memcmp(&expected,&restored,sizeof(expected)));
		assert(restored.ainode(&restored)==original.ainode(&original));
		assert(!G_ReadBotActorState(reader,4,&restored,true));
		assert(!memcmp(&expected,&restored,sizeof(expected)));
	}
	waypoints = originalWaypoints;
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	original.ainode = UnknownNode;
	assert(!G_WriteBotActorState(&writer,3,original) && !State_Finish(&writer));
	original.ainode = nullptr;
	original.activategoalheap[5].next = original.activatestack;
	writer = { bytes, sizeof( bytes ) };
	assert(!G_WriteBotActorState(&writer,3,original) && !State_Finish(&writer));
	original.activategoalheap[5].next = nullptr;
	writer = { bytes, sizeof( bytes ) };
	assert(G_WriteBotActorState(&writer,3,original));
	// Structurally valid archive with only scalar state lacks required companion records.
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,botActorSchema,3,&original));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	const auto expected = original;
	assert(!G_ReadBotActorState(reader,3,&original,true));
	assert(!memcmp(&expected,&original,sizeof(expected)));
	bot_state_t inactive{};
	botstates[3] = &original;
	botstates[7] = &inactive;
	writer = { bytes, sizeof( bytes ) };
	assert(G_WriteBotPoolState(&writer));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	static gentity_t entities[MAX_GENTITIES];
	static gclient_t clients[MAX_CLIENTS];
	entities[3].r.svFlags = SVF_BOT;
	clients[3].pers.connected = CON_CONNECTED;
	assert(G_ValidateBotReferences(reader,entities,clients));
	handlesExist = false;
	assert(!G_ValidateBotReferences(reader,entities,clients));
	handlesExist = true;
	entities[3].r.svFlags = 0;
	assert(!G_ValidateBotReferences(reader,entities,clients));
	entities[3].r.svFlags = SVF_BOT;
	clients[3].pers.connected = CON_DISCONNECTED;
	assert(!G_ValidateBotReferences(reader,entities,clients));
	clients[3].pers.connected = CON_CONNECTED;
	auto second = original;
	second.client = second.entitynum = 4;
	second.activatestack = &second.activategoalheap[2];
	second.activategoalheap[2].next = &second.activategoalheap[5];
	botstates[4] = &second;
	entities[4].r.svFlags = SVF_BOT;
	clients[4].pers.connected = CON_CONNECTED;
	writer = { bytes, sizeof( bytes ) };
	assert(G_WriteBotPoolState(&writer) && State_Open(bytes,State_Finish(&writer),&reader));
	assert(!G_ValidateBotReferences(reader,entities,clients)); // Actor-local handles cannot be shared.
	second.ms = second.gs = second.ws = second.cs = 18;
	writer = { bytes, sizeof( bytes ) };
	assert(G_WriteBotPoolState(&writer) && State_Open(bytes,State_Finish(&writer),&reader));
	assert(G_ValidateBotReferences(reader,entities,clients)); // Character caches can be shared.
	botstates[4] = nullptr;
	clients[4].pers.connected = CON_DISCONNECTED;
	writer = { bytes, sizeof( bytes ) };
	assert(G_WriteBotPoolState(&writer) && State_Open(bytes,State_Finish(&writer),&reader));
	assert(G_ReadBotPoolState(reader,false) && !G_ReadBotPoolState(reader,true) && !levelAllocations);
	botstates[3] = botstates[7] = nullptr;
	assert(G_ReadBotPoolState(reader,true) && levelAllocations==1);
	assert(botstates[3]==&restoredActors[0] && botstates[7]==&restoredActors[1] && !botstates[1]);
	assert(botstates[3]->activatestack==&restoredActors[0].activategoalheap[2]);
	assert(botstates[3]->activategoalheap[2].next==&restoredActors[0].activategoalheap[5]);
	assert(!botstates[7]->inuse);
	botPoolSave_t pool;
	uint32_t version;
	assert(State_Find(reader,botPoolSchema,0,&pool,&version));
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,botPoolSchema,0,&pool));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	botstates[3] = botstates[7] = nullptr;
	assert(!G_ReadBotPoolState(reader,true) && levelAllocations==1 && !botstates[3]);
	botstates[7] = &inactive;
	inactive.character = 3;
	writer = { bytes, sizeof( bytes ) };
	assert(!G_WriteBotPoolState(&writer) && !State_Finish(&writer));
	puts( "PASS: native bot pool reconstruction preserves allocated inactive slots and rebases activation links" );
	puts( "PASS: full bot actor drafts preserve every scalar, typed AI node and relocated activation/waypoint reference" );
}

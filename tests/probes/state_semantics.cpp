#define NATIVE_NAMESPACE game
#define NATIVE_SOURCE "game/g_state.cpp"
#include "../../game/module.cpp"
int main() {
	using namespace game;
	static gentity_t entities[MAX_GENTITIES];
	static gclient_t clients[MAX_CLIENTS];
	gStatePools_t pools{ entities, MAX_GENTITIES, clients, MAX_CLIENTS, nullptr, 0 };
	auto &entity = entities[3];
	entity.inuse = qtrue;
	entity.s.number = 3;
	entity.s.eType = ET_PLAYER;
	entity.client = &clients[3];
	entity.r.ownerNum = ENTITYNUM_NONE;
	entity.r.mins[2] = -24;
	entity.r.maxs[2] = 32;
	entity.s.pos.trBase[0] = -0.0f;
	const auto before = entity;
	assert(G_ValidateEntityState(3,entity,pools) && !memcmp(&before,&entity,sizeof(entity)));
	const uint32_t invalid = UINT32_MAX;
	memcpy( &entity.inuse, &invalid, sizeof( invalid ) );
	assert(!G_ValidateEntityState(3,entity,pools));
	entity = before;
	entity.r.currentOrigin[1] = HUGE_VALF;
	assert(!G_ValidateEntityState(3,entity,pools));
	entity = before;
	entity.s.pos.trType = TR_SINE;
	assert(!G_ValidateEntityState(3,entity,pools));
	entity.s.pos.trDuration = 1000;
	assert(G_ValidateEntityState(3,entity,pools));
	entity = before;
	entity.client = &clients[4];
	assert(!G_ValidateEntityState(3,entity,pools));
	entity = before;
	entity.client = nullptr;
	entity.s.eType = ET_WEAPON_STATE;
	entity.s.pos.trBase[0] = 65535;
	assert(G_ValidateEntityState(3,entity,pools));
	for ( int sequence = 0; sequence < 4; ++sequence ) {
		entity.s.eType = int( ET_EVENTS ) + int( EV_JUMP ) + ( sequence << 8 );
		assert(G_ValidateEntityState(3,entity,pools));
		entity.s.eType = int( ET_EVENTS ) + int( EV_WEAPON_NOTIFY ) + 1 + ( sequence << 8 );
		assert(!G_ValidateEntityState(3,entity,pools));
	}
	entity.s.eType = INT32_MIN;
	assert(!G_ValidateEntityState(3,entity,pools));
	auto &client = clients[3];
	client.pers.connected = CON_CONNECTED;
	client.ps.clientNum = 3;
	client.ps.weapon = WP_MACHINEGUN;
	client.ps.groundEntityNum = ENTITYNUM_NONE;
	client.sess.spectatorClient = -2;
	client.pers.cmd.weapon = 255; // Input validation belongs to the existing move code.
	assert(G_ValidateClientState(3,client,pools));
	memcpy( &client.sess.sessionTeam, &invalid, sizeof( invalid ) );
	assert(!G_ValidateClientState(3,client,pools));
	client.sess.sessionTeam = TEAM_FREE;
	client.ps.weapon = WP_NUM_WEAPONS;
	assert(!G_ValidateClientState(3,client,pools));
	client.ps.weapon = WP_MACHINEGUN;
	client.ps.clientNum = 4;
	assert(!G_ValidateClientState(3,client,pools));
	level_locals_t saved{};
	saved.maxclients = 4;
	saved.num_entities = MAX_CLIENTS;
	saved.numConnectedClients = saved.numNonSpectatorClients = saved.numPlayingClients = 2;
	saved.sortedClients[0] = 0;
	saved.sortedClients[1] = 3;
	saved.follow1 = 0;
	saved.follow2 = 3;
	assert(G_ValidateLevelState(saved,pools));
	saved.sortedClients[1] = 0;
	assert(!G_ValidateLevelState(saved,pools));
	saved.sortedClients[1] = 3;
	saved.numPlayingClients = 3;
	assert(!G_ValidateLevelState(saved,pools));
	puts( "PASS: gameplay drafts reject invalid raw enums, numeric state, slot identities and client-order metadata" );
}

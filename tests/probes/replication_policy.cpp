#include "../../engine/server/sv_replication.cpp"
#include <assert.h>

server_t sv;
serverStatic_t svs;
static cvar_t budget;
cvar_t *sv_snapshotBudget = &budget;
cvar_t *cl_shownet;
static sharedEntity_t entities[MAX_GENTITIES];
static client_t client;
static clientSnapshot_t previous, current;
static entityState_t oldStates[2], newStates[300], acknowledged;

sharedEntity_t *SV_GentityNum( int number ) {
	return &entities[number];
}
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}
void QDECL Com_Printf( const char *, ... ) {
}
void QDECL Com_DPrintf( const char *, ... ) {
}

static entityState_t State( int number, int step ) {
	entityState_t state = {};
	state.number = number;
	for ( int axis = 0; axis < 3; ++axis ) {
		state.pos.trBase[axis] = float( step * 123 + axis * 11 ) + 0.125f;
		state.origin[axis] = float( step * 234 + axis * 17 ) + 0.375f;
	}
	return state;
}
static int Bits( const entityState_t *from, const entityState_t *to ) {
	byte storage[512] = {};
	msg_t msg;
	MSG_Init( &msg, storage, sizeof( storage ) );
	MSG_WriteDeltaEntity( &msg, from, to, qfalse );
	return msg.bit;
}
static void Prepare( entityState_t **candidates, int count ) {
	current = {};
	current.frameNum = 10;
	current.num_entities = count < MAX_SNAPSHOT_ENTITIES ? count : MAX_SNAPSHOT_ENTITIES;
	for ( int i = 0; i < current.num_entities; ++i )
		current.ents[i] = candidates[i];
}

int main() {
	entityState_t *candidates[300];
	for ( int i = 0; i < 2; ++i ) {
		oldStates[i] = State( i + 1, 1 );
		newStates[i] = State( i + 1, 2 );
		previous.ents[i] = &oldStates[i];
		candidates[i] = &newStates[i];
	}
	previous.num_entities = 2;
	previous.frameNum = 5;
	Prepare( candidates, 2 );
	SV_ApplyReplicationPolicy( &client, &previous, &current, 16, candidates, 2 );
	assert( current.ents[0] == candidates[0] && current.ents[1] == candidates[1] && current.frameNum == 10 );

	byte buffer[16] = {};
	msg_t marker;
	MSG_Init( &marker, buffer, sizeof( buffer ) );
	MSG_WriteBits( &marker, MAX_GENTITIES - 1, GENTITYNUM_BITS );
	const int first = Bits( &oldStates[0], &newStates[0] ), second = Bits( &oldStates[1], &newStates[1] );
	budget.integer = ( 16 + marker.bit + ( first > second ? first : second ) + 7 ) / 8 + 1;
	assert( 16 + marker.bit + first + second > budget.integer * 8 ); // Only one update fits.
	assert( SV_SetEntityReplication( 2, 3, 0 ) );
	sv.time = 1000;
	SV_ApplyReplicationPolicy( &client, &previous, &current, 16, candidates, 2 );
	assert( current.num_entities == 2 && current.ents[0] == &oldStates[0] && current.ents[1] == &newStates[1] );
	assert( current.frameNum == previous.frameNum ); // Retained pointers keep the oldest storage generation.
	assert( 16 + marker.bit + Bits( previous.ents[1], current.ents[1] ) < budget.integer * 8 );

	acknowledged = newStates[1];
	previous.ents[1] = &acknowledged;
	newStates[1] = State( 2, 3 );
	Prepare( candidates, 2 );
	sv.time = 2000;
	SV_ApplyReplicationPolicy( &client, &previous, &current, 16, candidates, 2 );
	assert( current.ents[0] == &newStates[0] && current.ents[1] == &acknowledged ); // Age prevents starvation.

	Prepare( candidates, 1 );
	budget.integer = 1;
	SV_ApplyReplicationPolicy( &client, &previous, &current, 16, candidates, 1 );
	assert( current.num_entities == 1 && current.ents[0]->number == 1 ); // Never retain an entity leaving visibility.
	assert( current.ents[0] == previous.ents[0] ); // Mandatory traffic can exceed the optional update budget.

	budget.integer = 16000;
	client.rate = 1000;
	client.snapshotMsec = 20;
	Prepare( candidates, 2 );
	SV_ApplyReplicationPolicy( &client, &previous, &current, 16, candidates, 2 );
	assert( current.ents[0] == previous.ents[0] && current.ents[1] == previous.ents[1] );
	client.rate = 0;

	assert( SV_SetEntityReplication( 7, 1, 100 ) );
	entities[7].s.number = 7;
	entities[7].r.currentOrigin[0] = 101;
	const vec3_t view = {};
	assert( !SV_EntityRelevant( &entities[7], view ) );
	entities[7].r.currentOrigin[0] = 99;
	assert( SV_EntityRelevant( &entities[7], view ) );
	assert( !SV_SetEntityReplication( 7, 4, 100 ) );
	assert( !SV_SetEntityReplication( 7, 1, -1 ) );
	assert( SV_SetEntityReplication( 7, 0, 0 ) );
	entities[7].r.currentOrigin[0] = 100000;
	assert( SV_EntityRelevant( &entities[7], view ) );

	for ( int i = 0; i < 300; ++i ) {
		newStates[i] = State( i + 1, 2 );
		candidates[i] = &newStates[i];
	}
	assert( SV_SetEntityReplication( 300, 3, 0 ) );
	Prepare( candidates, 300 );
	SV_ApplyReplicationPolicy( &client, nullptr, &current, 16, candidates, 300 );
	assert( current.num_entities == MAX_SNAPSHOT_ENTITIES );
	assert( current.ents[current.num_entities - 1]->number == 300 );
	for ( int i = 1; i < current.num_entities; ++i )
		assert( current.ents[i - 1]->number < current.ents[i]->number );
	puts( "PASS: optional update budget, acknowledged-state retention, removal, priority/age, client rate and interest radius" );
}

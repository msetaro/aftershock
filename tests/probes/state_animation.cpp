#define NATIVE_NAMESPACE game
#define NATIVE_SOURCE "game/g_animation.cpp"
#include "../../game/module.cpp"

int main( int argc, char **argv ) {
	assert(argc==3);
	static unsigned char assets[2][1048576];
	static game::gentity_t entities[8], replacement[8];
	game::gStatePools_t pools{ entities, 8, nullptr, 0, nullptr, 0 };
	for ( int rig = 0; rig < 2; ++rig ) {
		FILE *file = fopen( argv[rig + 1], "rb" );
		assert(file);
		const size_t bytes = fread( assets[rig], 1, sizeof( assets[rig] ), file );
		assert(feof(file));
		fclose( file );
		assert(Anim_Open(assets[rig],bytes,&game::animationRigs[rig].asset));
		game::animationRigs[rig].storage = assets[rig];
		game::animationRigs[rig].footHeight[0] = 3.5f;
		game::animationRigs[rig].rootYaw = -7.25f;
	}
	game::animationEnabled = true;
	auto &actor = game::animationActors[0];
	actor.active = true;
	actor.spawn = 3;
	actor.clock = 1000;
	actor.facing = 35;
	actor.turnSign = -1;
	actor.boxCount = 1;
	actor.boxes[0] = { { -1, -2, -3 }, { 4, 5, 6 } };
	for ( int rig = 0; rig < 2; ++rig ) {
		actor.entity[rig] = &entities[rig + 3];
		const auto *asset = &game::animationRigs[rig].asset;
		Anim_DefaultParameters( asset, actor.parameters[rig] );
		Anim_Reset( asset, 1000, &actor.state[rig] );
		animEvents_t events;
		assert(Anim_Tick(asset,actor.parameters[rig],1000,&actor.state[rig],&events));
	}
	static unsigned char bytes[524288];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(game::G_WriteAnimationState(&writer,pools));
	const size_t size = State_Finish( &writer );
	stateReader_t reader;
	assert(size&&State_Open(bytes,size,&reader));
	const auto expected = actor;
	animState_t next[2];
	animEvents_t events[2];
	animPose_t poses[2];
	for ( int rig = 0; rig < 2; ++rig ) {
		next[rig] = actor.state[rig];
		const auto *asset = &game::animationRigs[rig].asset;
		assert(Anim_Tick(asset,actor.parameters[rig],1020,&next[rig],&events[rig]));
		assert(Anim_Evaluate(asset,&next[rig],actor.parameters[rig],1020,&poses[rig]));
	}
	actor = {};
	game::animationRigs[0].footHeight[0] = 0;
	game::animationRigs[0].rootYaw = 0;
	pools.entities = replacement;
	assert(game::G_ReadAnimationState(reader,pools,false)&&!actor.active);
	assert(game::G_ReadAnimationState(reader,pools,true)&&actor.active);
	assert(actor.entity[0]==&replacement[3]&&actor.entity[1]==&replacement[4]);
	assert(actor.spawn==3&&actor.clock==1000&&actor.facing==35&&actor.turnSign==-1);
	assert(!memcmp(actor.boxes,expected.boxes,sizeof(actor.boxes))&&actor.boxCount==1);
	for ( int rig = 0; rig < 2; ++rig ) {
		assert(!memcmp(&actor.state[rig],&expected.state[rig],sizeof(animState_t)));
		animEvents_t resumed{};
		animPose_t pose{};
		const auto *asset = &game::animationRigs[rig].asset;
		assert(Anim_Tick(asset,actor.parameters[rig],1020,&actor.state[rig],&resumed));
		assert(Anim_Evaluate(asset,&actor.state[rig],actor.parameters[rig],1020,&pose));
		assert(!memcmp(&next[rig],&actor.state[rig],sizeof(animState_t)));
		assert(events[rig].count==resumed.count&&!memcmp(events[rig].items,resumed.items,resumed.count*sizeof(animEvent_t)));
		assert(pose.jointCount==poses[rig].jointCount&&!memcmp(pose.world,poses[rig].world,pose.jointCount*sizeof(pose.world[0])));
	}
	static unsigned char partial[8192];
	stateWriter_t incomplete{ partial, sizeof( partial ) };
	game::animationRigSave_t rigRecord;
	game::animationActorSave_t actorRecord;
	uint32_t version;
	assert(State_Find(reader,game::animationRigSchema,0,&rigRecord,&version));
	assert(State_Append(&incomplete,game::animationRigSchema,0,&rigRecord));
	assert(State_Find(reader,game::animationActorSchema,0,&actorRecord,&version));
	assert(State_Append(&incomplete,game::animationActorSchema,0,&actorRecord));
	for ( uint32_t rig = 0; rig < 2; ++rig ) {
		animState_t state;
		assert(State_Find(reader,animationStateSchema,rig,&state,&version));
		assert(State_Append(&incomplete,animationStateSchema,rig,&state));
	}
	const size_t partialSize = State_Finish( &incomplete );
	stateReader_t missing;
	assert(partialSize&&State_Open(partial,partialSize,&missing));
	assert(!game::G_ReadAnimationState(missing,pools,true));
	assert(!memcmp(&next[0],&actor.state[0],sizeof(animState_t)));
	game::animationRigs[0].asset.hash[0] ^= 1;
	assert(!game::G_ReadAnimationState(reader,pools,true));
	assert(!memcmp(&next[0],&actor.state[0],sizeof(animState_t)));
	puts( "PASS: saved animation rigs, actors and references resume identical states, events and pose words" );
}

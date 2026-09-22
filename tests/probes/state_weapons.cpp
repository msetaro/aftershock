#define NATIVE_NAMESPACE game
#define NATIVE_SOURCE "game/g_data_weapons.cpp"
#include "../../game/module.cpp"

static weaponDef_t definition;
static animAsset_t graph;
namespace game {
int BG_WeaponCount() {
	return 1;
}
const weaponDef_t *BG_WeaponDefinition( int index ) {
	return index == 0 ? &definition : nullptr;
}
const animAsset_t *BG_WeaponAnimation( int index ) {
	return index == 0 ? &graph : nullptr;
}
} // namespace game
static size_t Read( const char *path, void *out, size_t capacity ) {
	FILE *file = fopen( path, "rb" );
	assert(file);
	const size_t size = fread( out, 1, capacity, file );
	assert(feof(file));
	fclose( file );
	return size;
}
int main( int argc, char **argv ) {
	assert(argc==3);
	static unsigned char source[1048576], animation[1048576], archive[524288];
	assert(Weapon_Open(source,Read(argv[1],source,sizeof(source)),&definition));
	uint8_t hash[32];
	Weapon_DefinitionHash( &definition, hash );
	assert(!memcmp(hash,source+16,32));
	assert(Anim_Open(animation,Read(argv[2],animation,sizeof(animation)),&graph));
	static game::gentity_t entities[8], replacement[8];
	game::gStatePools_t pools{ entities, 8, nullptr, 0, nullptr, 0 };
	auto &actor = game::weaponActors[0];
	actor.active = true;
	actor.spawn = 3;
	actor.epoch = 123;
	for ( int hand = 0; hand < 2; ++hand ) {
		actor.entity[hand] = &entities[hand + 2];
		actor.animationEntity[hand] = &entities[hand + 4];
		actor.attachments[hand][0] = 1;
		assert(Weapon_Configure(&definition,1,&actor.configured[hand]));
		Weapon_Reset( &actor.configured[hand], uint32_t( 123 + hand ), 1000, &actor.inventory[hand][0] );
		weaponEvents_t events;
		assert(Weapon_Tick(&actor.configured[hand],WEAPON_FIRE,1020,&actor.inventory[hand][0],&events));
		Anim_DefaultParameters( &graph, actor.parameters[hand] );
		Anim_Reset( &graph, 1020, &actor.animation[hand] );
	}
	const auto before = actor;
	stateWriter_t writer{ archive, sizeof( archive ) };
	assert(game::G_WriteWeaponState(&writer,pools));
	const size_t size = State_Finish( &writer );
	stateReader_t reader;
	assert(size&&State_Open(archive,size,&reader));
	weaponState_t continued[2];
	weaponEvents_t events[2];
	for ( int hand = 0; hand < 2; ++hand ) {
		continued[hand] = actor.inventory[hand][0];
		assert(Weapon_Command(&actor.configured[hand],WEAPON_FIRE|WEAPON_ADS,1220,&continued[hand],&events[hand]));
	}
	actor = {};
	pools.entities = replacement;
	assert(game::G_ReadWeaponState(reader,pools,false)&&!actor.active);
	assert(game::G_ReadWeaponState(reader,pools,true)&&actor.active);
	assert(actor.epoch==123&&actor.spawn==3);
	for ( int hand = 0; hand < 2; ++hand ) {
		assert(actor.entity[hand]==&replacement[hand+2]&&actor.animationEntity[hand]==&replacement[hand+4]);
		assert(actor.attachments[hand][0]==1&&!memcmp(&actor.configured[hand],&before.configured[hand],sizeof(weaponDef_t)));
		assert(!memcmp(actor.inventory[hand],before.inventory[hand],sizeof(actor.inventory[hand])));
		assert(!memcmp(&actor.animation[hand],&before.animation[hand],sizeof(animState_t)));
		weaponEvents_t resumed;
		assert(Weapon_Command(&actor.configured[hand],WEAPON_FIRE|WEAPON_ADS,1220,&actor.inventory[hand][0],&resumed));
		assert(!memcmp(&continued[hand],&actor.inventory[hand][0],sizeof(weaponState_t)));
		assert(resumed.count==events[hand].count&&!memcmp(resumed.items,events[hand].items,resumed.count*sizeof(weaponEvent_t)));
	}
	static unsigned char partial[16384];
	stateWriter_t incomplete{ partial, sizeof( partial ) };
	game::weaponContentSave_t content;
	game::weaponActorSave_t saved;
	uint32_t version;
	assert(State_Find(reader,game::weaponContentSchema,0,&content,&version));
	assert(State_Append(&incomplete,game::weaponContentSchema,0,&content));
	assert(State_Find(reader,game::weaponActorSchema,0,&saved,&version));
	assert(State_Append(&incomplete,game::weaponActorSchema,0,&saved));
	for ( uint32_t hand = 0; hand < 2; ++hand ) {
		animState_t state;
		weaponState_t weapon;
		const uint32_t animationSlot = game::WEAPON_ANIMATION_SLOT + hand;
		const uint32_t inventorySlot = hand * WEAPON_MAX_DEFINITIONS;
		assert(State_Find(reader,animationStateSchema,animationSlot,&state,&version));
		assert(State_Append(&incomplete,animationStateSchema,animationSlot,&state));
		assert(State_Find(reader,weaponStateSchema,inventorySlot,&weapon,&version));
		assert(State_Append(&incomplete,weaponStateSchema,inventorySlot,&weapon));
	}
	const size_t partialSize = State_Finish( &incomplete );
	stateReader_t missing;
	assert(partialSize&&State_Open(partial,partialSize,&missing));
	assert(!game::G_ReadWeaponState(missing,pools,true));
	assert(!memcmp(&continued[0],&actor.inventory[0][0],sizeof(weaponState_t)));
	game::weaponActors[1].inventory[0][0].sequence = 1;
	stateWriter_t rejected{ partial, sizeof( partial ) };
	assert(!game::G_WriteWeaponState(&rejected,pools)&&!State_Finish(&rejected));
	game::weaponActors[1].inventory[0][0] = {};
	definition.damage += 1;
	assert(!game::G_ReadWeaponState(reader,pools,true));
	assert(!memcmp(&continued[0],&actor.inventory[0][0],sizeof(weaponState_t)));
	puts( "PASS: saved weapon inventory, attachments, animation and RNG resume identical command events" );
}

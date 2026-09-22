#define NATIVE_NAMESPACE game
#define NATIVE_SOURCE "game/g_state.cpp"
#include "../../game/module.cpp"

int main() {
	static game::gentity_t entities[8], replacements[8];
	static game::gclient_t clients[2], newClients[2];
	game::gitem_t items[2]{}, newItems[2]{};
	items[0].classname = (char *)"item_health";
	items[1].classname = (char *)"weapon_rocketlauncher";
	newItems[0] = items[1];
	newItems[1] = items[0];
	const game::gStatePools_t original{ entities, 8, clients, 2, items, 2 };
	const game::gStatePools_t replacement{ replacements, 8, newClients, 2, newItems, 2 };
	game::gentity_t entity{};
	entity.client = &clients[1];
	entity.parent = &entities[3];
	entity.nextTrain = &entities[4];
	entity.prevTrain = &entities[5];
	entity.target_ent = &entities[6];
	entity.chain = &entities[7];
	entity.enemy = &entities[0];
	entity.activator = &entities[1];
	entity.teamchain = &entities[2];
	entity.teammaster = &entities[3];
	entity.item = &items[1];
	game::gEntityRefs_t saved{};
	assert( game::G_CaptureEntityRefs( entity, original, &saved ) );
	unsigned char bytes[4096];
	const size_t size = State_Write( game::entityRefsSchema, &saved, bytes, sizeof( bytes ) );
	assert( size );
	game::gEntityRefs_t decoded{};
	uint32_t version;
	assert( State_Read( game::entityRefsSchema, bytes, size, &decoded, &version ) && version == 1 );
	game::gentity_t restored{};
	restored.health = 19;
	assert( game::G_RestoreEntityRefs( decoded, replacement, &restored ) );
	assert( restored.health == 19 && restored.client == &newClients[1] );
	assert( restored.parent == &replacements[3] && restored.nextTrain == &replacements[4] && restored.prevTrain == &replacements[5] );
	assert( restored.target_ent == &replacements[6] && restored.chain == &replacements[7] && restored.enemy == &replacements[0] );
	assert( restored.activator == &replacements[1] && restored.teamchain == &replacements[2] && restored.teammaster == &replacements[3] );
	assert( restored.item == &newItems[0] ); // Item identity survives a reordered table.
	const auto before = restored;
	decoded.parent = 8;
	assert( !game::G_RestoreEntityRefs( decoded, replacement, &restored ) );
	assert( !memcmp( &before, &restored, sizeof( restored ) ) );
	decoded = saved;
	decoded.client = -2;
	assert( !game::G_RestoreEntityRefs( decoded, replacement, &restored ) );
	assert( !memcmp( &before, &restored, sizeof( restored ) ) );
	decoded = saved;
	strcpy( decoded.item, "missing" );
	assert( !game::G_RestoreEntityRefs( decoded, replacement, &restored ) );
	assert( !memcmp( &before, &restored, sizeof( restored ) ) );
	entity.enemy = &restored; // A valid pointer, but outside the entity pool.
	const auto previous = saved;
	assert( !game::G_CaptureEntityRefs( entity, original, &saved ) );
	assert( !memcmp( &previous, &saved, sizeof( saved ) ) );
	entity = {};
	assert( game::G_CaptureEntityRefs( entity, original, &saved ) );
	assert( game::G_RestoreEntityRefs( saved, replacement, &restored ) );
	assert( !restored.client && !restored.item && !restored.parent && !restored.enemy && !restored.teammaster );
	puts( "PASS: entity/client slots and named items restore into different pools without saving addresses" );
}

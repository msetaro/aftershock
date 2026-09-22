#include "g_local.h"

static bool ValidPools( const gStatePools_t &pools ) {
	return pools.entityCount >= 0 && pools.entityCount <= MAX_GENTITIES && ( !pools.entityCount || pools.entities ) &&
		   pools.clientCount >= 0 && pools.clientCount <= MAX_CLIENTS && ( !pools.clientCount || pools.clients ) &&
		   pools.itemCount >= 0 && pools.itemCount <= MAX_ITEMS && ( !pools.itemCount || pools.items );
}
template <typename T>
static int Slot( const T *pointer, const T *pool, int count ) {
	if ( !pointer )
		return -1;
	// ponytail: bounded pool scan at save time; index only if checkpoint latency needs it.
	for ( int i = 0; i < count; ++i )
		if ( pointer == &pool[i] )
			return i;
	return -2;
}
static const struct {
	gentity_t *gentity_t::*entity;
	int32_t gEntityRefs_t::*saved;
} entityReferences[] = {
	{ &gentity_t::parent, &gEntityRefs_t::parent },
	{ &gentity_t::nextTrain, &gEntityRefs_t::nextTrain },
	{ &gentity_t::prevTrain, &gEntityRefs_t::prevTrain },
	{ &gentity_t::target_ent, &gEntityRefs_t::target_ent },
	{ &gentity_t::chain, &gEntityRefs_t::chain },
	{ &gentity_t::enemy, &gEntityRefs_t::enemy },
	{ &gentity_t::activator, &gEntityRefs_t::activator },
	{ &gentity_t::teamchain, &gEntityRefs_t::teamchain },
	{ &gentity_t::teammaster, &gEntityRefs_t::teammaster },
};
bool G_CaptureEntityRefs( const gentity_t &entity, const gStatePools_t &pools, gEntityRefs_t *references ) {
	if ( !references || !ValidPools( pools ) )
		return false;
	gEntityRefs_t saved{};
	saved.client = Slot( entity.client, pools.clients, pools.clientCount );
	if ( saved.client == -2 )
		return false;
	for ( const auto &field : entityReferences ) {
		saved.*field.saved = Slot( entity.*field.entity, pools.entities, pools.entityCount );
		if ( saved.*field.saved == -2 )
			return false;
	}
	const int item = Slot( entity.item, pools.items, pools.itemCount );
	if ( item == -2 )
		return false;
	if ( item >= 0 ) {
		const char *name = pools.items[item].classname;
		if ( !name || !name[0] || strlen( name ) >= sizeof( saved.item ) )
			return false;
		strcpy( saved.item, name );
	}
	*references = saved;
	return true;
}
bool G_RestoreEntityRefs( const gEntityRefs_t &references, const gStatePools_t &pools, gentity_t *entity ) {
	if ( !entity || !ValidPools( pools ) || references.client < -1 || references.client >= pools.clientCount ||
		 !memchr( references.item, 0, sizeof( references.item ) ) )
		return false;
	auto restored = *entity;
	restored.client = references.client == -1 ? nullptr : &pools.clients[references.client];
	for ( const auto &field : entityReferences ) {
		const int slot = references.*field.saved;
		if ( slot < -1 || slot >= pools.entityCount )
			return false;
		restored.*field.entity = slot == -1 ? nullptr : &pools.entities[slot];
	}
	restored.item = nullptr;
	if ( references.item[0] ) {
		for ( int i = 0; i < pools.itemCount; ++i )
			if ( pools.items[i].classname && !strcmp( references.item, pools.items[i].classname ) ) {
				if ( restored.item )
					return false; // Ambiguous item identities are not portable saves.
				restored.item = &pools.items[i];
			}
		if ( !restored.item )
			return false;
	}
	*entity = restored;
	return true;
}
static constexpr stateField_t entityRefsFields[] = {
	{ "client", offsetof( gEntityRefs_t, client ), 1, stateType_t::Int32 },
	{ "parent", offsetof( gEntityRefs_t, parent ), 1, stateType_t::Int32 },
	{ "nextTrain", offsetof( gEntityRefs_t, nextTrain ), 1, stateType_t::Int32 },
	{ "prevTrain", offsetof( gEntityRefs_t, prevTrain ), 1, stateType_t::Int32 },
	{ "target_ent", offsetof( gEntityRefs_t, target_ent ), 1, stateType_t::Int32 },
	{ "chain", offsetof( gEntityRefs_t, chain ), 1, stateType_t::Int32 },
	{ "enemy", offsetof( gEntityRefs_t, enemy ), 1, stateType_t::Int32 },
	{ "activator", offsetof( gEntityRefs_t, activator ), 1, stateType_t::Int32 },
	{ "teamchain", offsetof( gEntityRefs_t, teamchain ), 1, stateType_t::Int32 },
	{ "teammaster", offsetof( gEntityRefs_t, teammaster ), 1, stateType_t::Int32 },
	{ "item", offsetof( gEntityRefs_t, item ), 64, stateType_t::String },
};
const stateSchema_t entityRefsSchema = { "game.entityRefs", 1, 1, sizeof( gEntityRefs_t ), entityRefsFields, 11 };

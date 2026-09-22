#include "g_local.h"
#include "../../engine/public/state_replication_public.h"

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

#define ENTITY_STRINGS( X ) \
	X( classname, 0 ) \
	X( model, 1 ) \
	X( model2, 2 ) \
	X( message, 3 ) \
	X( target, 4 ) \
	X( targetname, 5 ) \
	X( team, 6 ) \
	X( targetShaderName, 7 ) \
	X( targetShaderNewName, 8 ) \
	X( definitionName, 9 )
static bool CaptureString( const char *source, char ( &out )[MAX_SPAWN_VARS_CHARS], uint32_t bit, uint32_t *present ) {
	if ( !source )
		return true;
	size_t length = 0;
	while ( length < sizeof( out ) && source[length] )
		++length;
	if ( length == sizeof( out ) )
		return false;
	memcpy( out, source, length + 1 );
	*present |= 1u << bit;
	return true;
}
bool G_CaptureEntityStrings( const gentity_t &entity, gEntityStrings_t *strings ) {
	if ( !strings )
		return false;
	gEntityStrings_t saved{};
#define CAPTURE( field, bit ) if ( !CaptureString( entity.field, saved.field, bit, &saved.present ) ) return false;
	ENTITY_STRINGS( CAPTURE )
#undef CAPTURE
	*strings = saved;
	return true;
}
static size_t StringBytes( const char ( &text )[MAX_SPAWN_VARS_CHARS], bool present ) {
	const char *end = (const char *)memchr( text, 0, sizeof( text ) );
	if ( !end || ( !present && text[0] ) )
		return SIZE_MAX;
	return present ? size_t( end - text ) + 1 : 0;
}
size_t G_EntityStringBytes( const gEntityStrings_t &strings ) {
	if ( strings.present & ~1023u )
		return SIZE_MAX;
	size_t total = 0;
#define MEASURE( field, bit ) { const size_t bytes = StringBytes( strings.field, ( strings.present & ( 1u << bit ) ) != 0 ); if ( bytes == SIZE_MAX ) return SIZE_MAX; total += bytes; }
	ENTITY_STRINGS( MEASURE )
#undef MEASURE
	return total;
}
static char *RestoreString( const char *text, bool present, char **cursor ) {
	if ( !present )
		return nullptr;
	char *result = *cursor;
	const size_t bytes = strlen( text ) + 1;
	memcpy( result, text, bytes );
	*cursor += bytes;
	return result;
}
bool G_RestoreEntityStrings( const gEntityStrings_t &strings, char *storage, size_t capacity, gentity_t *entity ) {
	const size_t bytes = G_EntityStringBytes( strings );
	if ( !entity || bytes == SIZE_MAX || bytes > capacity || ( bytes && !storage ) )
		return false;
#define RESTORE( field, bit ) entity->field = RestoreString( strings.field, ( strings.present & ( 1u << bit ) ) != 0, &storage );
	ENTITY_STRINGS( RESTORE )
#undef RESTORE
	return true;
}
static constexpr stateField_t entityStringsFields[] = {
	{ "present", offsetof( gEntityStrings_t, present ), 1, stateType_t::UInt32 },
#define FIELD( field, bit ) { #field, offsetof( gEntityStrings_t, field ), MAX_SPAWN_VARS_CHARS, stateType_t::String },
	ENTITY_STRINGS( FIELD )
#undef FIELD
};
const stateSchema_t entityStringsSchema = { "game.entityStrings", 1, 1, sizeof( gEntityStrings_t ), entityStringsFields, 11 };
#undef ENTITY_STRINGS

static constexpr stateField_t gameEntityFields[] = {
	{ "inuse", offsetof( gentity_t, inuse ), 1, stateType_t::UInt32 },
	{ "rewindSpawn", offsetof( gentity_t, rewindSpawn ), 1, stateType_t::UInt32 },
	{ "spawnflags", offsetof( gentity_t, spawnflags ), 1, stateType_t::Int32 },
	{ "neverFree", offsetof( gentity_t, neverFree ), 1, stateType_t::UInt32 },
	{ "flags", offsetof( gentity_t, flags ), 1, stateType_t::Int32 },
	{ "freetime", offsetof( gentity_t, freetime ), 1, stateType_t::Int32 },
	{ "eventTime", offsetof( gentity_t, eventTime ), 1, stateType_t::Int32 },
	{ "freeAfterEvent", offsetof( gentity_t, freeAfterEvent ), 1, stateType_t::UInt32 },
	{ "unlinkAfterEvent", offsetof( gentity_t, unlinkAfterEvent ), 1, stateType_t::UInt32 },
	{ "physicsObject", offsetof( gentity_t, physicsObject ), 1, stateType_t::UInt32 },
	{ "physicsBounce", offsetof( gentity_t, physicsBounce ), 1, stateType_t::Float32 },
	{ "clipmask", offsetof( gentity_t, clipmask ), 1, stateType_t::Int32 },
	{ "moverState", offsetof( gentity_t, moverState ), 1, stateType_t::UInt32 },
	{ "soundPos1", offsetof( gentity_t, soundPos1 ), 1, stateType_t::Int32 },
	{ "sound1to2", offsetof( gentity_t, sound1to2 ), 1, stateType_t::Int32 },
	{ "sound2to1", offsetof( gentity_t, sound2to1 ), 1, stateType_t::Int32 },
	{ "soundPos2", offsetof( gentity_t, soundPos2 ), 1, stateType_t::Int32 },
	{ "soundLoop", offsetof( gentity_t, soundLoop ), 1, stateType_t::Int32 },
	{ "pos1", offsetof( gentity_t, pos1 ), 3, stateType_t::Float32 },
	{ "pos2", offsetof( gentity_t, pos2 ), 3, stateType_t::Float32 },
	{ "timestamp", offsetof( gentity_t, timestamp ), 1, stateType_t::Int32 },
	{ "angle", offsetof( gentity_t, angle ), 1, stateType_t::Float32 },
	{ "speed", offsetof( gentity_t, speed ), 1, stateType_t::Float32 },
	{ "movedir", offsetof( gentity_t, movedir ), 3, stateType_t::Float32 },
	{ "nextthink", offsetof( gentity_t, nextthink ), 1, stateType_t::Int32 },
	{ "pain_debounce_time", offsetof( gentity_t, pain_debounce_time ), 1, stateType_t::Int32 },
	{ "fly_sound_debounce_time", offsetof( gentity_t, fly_sound_debounce_time ), 1, stateType_t::Int32 },
	{ "last_move_time", offsetof( gentity_t, last_move_time ), 1, stateType_t::Int32 },
	{ "health", offsetof( gentity_t, health ), 1, stateType_t::Int32 },
	{ "takedamage", offsetof( gentity_t, takedamage ), 1, stateType_t::UInt32 },
	{ "damage", offsetof( gentity_t, damage ), 1, stateType_t::Int32 },
	{ "splashDamage", offsetof( gentity_t, splashDamage ), 1, stateType_t::Int32 },
	{ "splashRadius", offsetof( gentity_t, splashRadius ), 1, stateType_t::Int32 },
	{ "methodOfDeath", offsetof( gentity_t, methodOfDeath ), 1, stateType_t::Int32 },
	{ "splashMethodOfDeath", offsetof( gentity_t, splashMethodOfDeath ), 1, stateType_t::Int32 },
	{ "count", offsetof( gentity_t, count ), 1, stateType_t::Int32 },
#ifdef MISSIONPACK
	{ "kamikazeTime", offsetof( gentity_t, kamikazeTime ), 1, stateType_t::Int32 },
	{ "kamikazeShockTime", offsetof( gentity_t, kamikazeShockTime ), 1, stateType_t::Int32 },
#endif
	{ "watertype", offsetof( gentity_t, watertype ), 1, stateType_t::Int32 },
	{ "waterlevel", offsetof( gentity_t, waterlevel ), 1, stateType_t::Int32 },
	{ "noise_index", offsetof( gentity_t, noise_index ), 1, stateType_t::Int32 },
	{ "wait", offsetof( gentity_t, wait ), 1, stateType_t::Float32 },
	{ "random", offsetof( gentity_t, random ), 1, stateType_t::Float32 },
};
const stateSchema_t gameEntitySchema = { "game.entity", 1, 1, sizeof( gentity_t ), gameEntityFields, sizeof( gameEntityFields ) / sizeof( gameEntityFields[0] ) };
static constexpr stateField_t entitySharedFields[] = {
	{ "linked", offsetof( entityShared_t, linked ), 1, stateType_t::UInt32 },
	{ "linkcount", offsetof( entityShared_t, linkcount ), 1, stateType_t::Int32 },
	{ "svFlags", offsetof( entityShared_t, svFlags ), 1, stateType_t::Int32 },
	{ "singleClient", offsetof( entityShared_t, singleClient ), 1, stateType_t::Int32 },
	{ "bmodel", offsetof( entityShared_t, bmodel ), 1, stateType_t::UInt32 },
	{ "mins", offsetof( entityShared_t, mins ), 3, stateType_t::Float32 },
	{ "maxs", offsetof( entityShared_t, maxs ), 3, stateType_t::Float32 },
	{ "contents", offsetof( entityShared_t, contents ), 1, stateType_t::Int32 },
	{ "absmin", offsetof( entityShared_t, absmin ), 3, stateType_t::Float32 },
	{ "absmax", offsetof( entityShared_t, absmax ), 3, stateType_t::Float32 },
	{ "currentOrigin", offsetof( entityShared_t, currentOrigin ), 3, stateType_t::Float32 },
	{ "currentAngles", offsetof( entityShared_t, currentAngles ), 3, stateType_t::Float32 },
	{ "ownerNum", offsetof( entityShared_t, ownerNum ), 1, stateType_t::Int32 },
};
const stateSchema_t entitySharedSchema = { "game.shared", 1, 1, sizeof( entityShared_t ), entitySharedFields, sizeof( entitySharedFields ) / sizeof( entitySharedFields[0] ) };

bool G_WriteEntityState( stateWriter_t *writer, uint32_t slot, const gentity_t &entity, const gStatePools_t &pools, const gSaveCallback_t *const *callbacks ) {
	if ( !writer )
		return false;
	gEntityRefs_t refs;
	gCallbackNames_t names;
	gEntityStrings_t strings;
	if ( !ValidPools( pools ) || slot >= uint32_t( pools.entityCount ) ||
		 !G_CaptureEntityRefs( entity, pools, &refs ) || !G_CaptureCallbacks( entity, callbacks, &names ) || !G_CaptureEntityStrings( entity, &strings ) ) {
		writer->failed = true;
		return false;
	}
	return State_Append( writer, gameEntitySchema, slot, &entity ) &&
		   State_Append( writer, entitySharedSchema, slot, &entity.r ) &&
		   State_Append( writer, entityStateSchema, slot, &entity.s ) &&
		   State_Append( writer, entityStateSchema, slot + MAX_GENTITIES, &entity.r.s ) &&
		   State_Append( writer, entityRefsSchema, slot, &refs ) &&
		   State_Append( writer, callbackNamesSchema, slot, &names ) &&
		   State_Append( writer, entityStringsSchema, slot, &strings );
}
bool G_ReadEntityState( const stateReader_t &reader, uint32_t slot, const gStatePools_t &pools, const gSaveCallback_t *const *callbacks,
	gentity_t *entity, gEntityStrings_t *strings ) {
	if ( !entity || !strings || !ValidPools( pools ) || slot >= uint32_t( pools.entityCount ) )
		return false;
	gentity_t restored{};
	gEntityRefs_t refs;
	gCallbackNames_t names;
	gEntityStrings_t text;
	uint32_t version;
	if ( !State_Find( reader, gameEntitySchema, slot, &restored, &version ) ||
		 !State_Find( reader, entitySharedSchema, slot, &restored.r, &version ) ||
		 !State_Find( reader, entityStateSchema, slot, &restored.s, &version ) ||
		 !State_Find( reader, entityStateSchema, slot + MAX_GENTITIES, &restored.r.s, &version ) ||
		 !State_Find( reader, entityRefsSchema, slot, &refs, &version ) ||
		 !State_Find( reader, callbackNamesSchema, slot, &names, &version ) ||
		 !State_Find( reader, entityStringsSchema, slot, &text, &version ) ||
		 G_EntityStringBytes( text ) == SIZE_MAX ||
		 !G_RestoreEntityRefs( refs, pools, &restored ) || !G_RestoreCallbacks( names, callbacks, &restored ) )
		return false;
	*entity = restored;
	*strings = text;
	return true;
}

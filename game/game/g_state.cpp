#include "g_local.h"
#include "../../engine/public/state_replication_public.h"

bool G_StatePoolsValid( const gStatePools_t &pools ) {
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
int G_StateEntitySlot( const gentity_t *entity, const gStatePools_t &pools ) {
	return G_StatePoolsValid( pools ) ? Slot( entity, pools.entities, pools.entityCount ) : -2;
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
	if ( !references || !G_StatePoolsValid( pools ) )
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
	if ( !entity || !G_StatePoolsValid( pools ) || references.client < -1 || references.client >= pools.clientCount ||
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
	if ( !G_StatePoolsValid( pools ) || slot >= uint32_t( pools.entityCount ) ||
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
	if ( !entity || !strings || !G_StatePoolsValid( pools ) || slot >= uint32_t( pools.entityCount ) )
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

static constexpr stateField_t gameClientFields[] = {
	{ "pers.connected", offsetof( gclient_t, pers.connected ), 1, stateType_t::UInt32 },
	{ "pers.localClient", offsetof( gclient_t, pers.localClient ), 1, stateType_t::UInt32 },
	{ "pers.initialSpawn", offsetof( gclient_t, pers.initialSpawn ), 1, stateType_t::UInt32 },
	{ "pers.predictItemPickup", offsetof( gclient_t, pers.predictItemPickup ), 1, stateType_t::UInt32 },
	{ "pers.pmoveFixed", offsetof( gclient_t, pers.pmoveFixed ), 1, stateType_t::UInt32 },
	{ "pers.netname", offsetof( gclient_t, pers.netname ), MAX_NETNAME, stateType_t::String },
	{ "pers.maxHealth", offsetof( gclient_t, pers.maxHealth ), 1, stateType_t::Int32 },
	{ "pers.enterTime", offsetof( gclient_t, pers.enterTime ), 1, stateType_t::Int32 },
	{ "pers.teamState.state", offsetof( gclient_t, pers.teamState.state ), 1, stateType_t::UInt32 },
	{ "pers.teamState.location", offsetof( gclient_t, pers.teamState.location ), 1, stateType_t::Int32 },
	{ "pers.teamState.captures", offsetof( gclient_t, pers.teamState.captures ), 1, stateType_t::Int32 },
	{ "pers.teamState.basedefense", offsetof( gclient_t, pers.teamState.basedefense ), 1, stateType_t::Int32 },
	{ "pers.teamState.carrierdefense", offsetof( gclient_t, pers.teamState.carrierdefense ), 1, stateType_t::Int32 },
	{ "pers.teamState.flagrecovery", offsetof( gclient_t, pers.teamState.flagrecovery ), 1, stateType_t::Int32 },
	{ "pers.teamState.fragcarrier", offsetof( gclient_t, pers.teamState.fragcarrier ), 1, stateType_t::Int32 },
	{ "pers.teamState.assists", offsetof( gclient_t, pers.teamState.assists ), 1, stateType_t::Int32 },
	{ "pers.teamState.lasthurtcarrier", offsetof( gclient_t, pers.teamState.lasthurtcarrier ), 1, stateType_t::Float32 },
	{ "pers.teamState.lastreturnedflag", offsetof( gclient_t, pers.teamState.lastreturnedflag ), 1, stateType_t::Float32 },
	{ "pers.teamState.flagsince", offsetof( gclient_t, pers.teamState.flagsince ), 1, stateType_t::Float32 },
	{ "pers.teamState.lastfraggedcarrier", offsetof( gclient_t, pers.teamState.lastfraggedcarrier ), 1, stateType_t::Float32 },
	{ "pers.voteCount", offsetof( gclient_t, pers.voteCount ), 1, stateType_t::Int32 },
	{ "pers.teamVoteCount", offsetof( gclient_t, pers.teamVoteCount ), 1, stateType_t::Int32 },
	{ "pers.teamInfo", offsetof( gclient_t, pers.teamInfo ), 1, stateType_t::UInt32 },
	{ "sess.sessionTeam", offsetof( gclient_t, sess.sessionTeam ), 1, stateType_t::UInt32 },
	{ "sess.spectatorTime", offsetof( gclient_t, sess.spectatorTime ), 1, stateType_t::Int32 },
	{ "sess.spectatorState", offsetof( gclient_t, sess.spectatorState ), 1, stateType_t::UInt32 },
	{ "sess.spectatorClient", offsetof( gclient_t, sess.spectatorClient ), 1, stateType_t::Int32 },
	{ "sess.wins", offsetof( gclient_t, sess.wins ), 1, stateType_t::Int32 },
	{ "sess.losses", offsetof( gclient_t, sess.losses ), 1, stateType_t::Int32 },
	{ "sess.teamLeader", offsetof( gclient_t, sess.teamLeader ), 1, stateType_t::UInt32 },
	{ "readyToExit", offsetof( gclient_t, readyToExit ), 1, stateType_t::UInt32 },
	{ "noclip", offsetof( gclient_t, noclip ), 1, stateType_t::UInt32 },
	{ "lastCmdTime", offsetof( gclient_t, lastCmdTime ), 1, stateType_t::Int32 },
	{ "buttons", offsetof( gclient_t, buttons ), 1, stateType_t::Int32 },
	{ "oldbuttons", offsetof( gclient_t, oldbuttons ), 1, stateType_t::Int32 },
	{ "latched_buttons", offsetof( gclient_t, latched_buttons ), 1, stateType_t::Int32 },
	{ "oldOrigin", offsetof( gclient_t, oldOrigin ), 3, stateType_t::Float32 },
	{ "damage_armor", offsetof( gclient_t, damage_armor ), 1, stateType_t::Int32 },
	{ "damage_blood", offsetof( gclient_t, damage_blood ), 1, stateType_t::Int32 },
	{ "damage_knockback", offsetof( gclient_t, damage_knockback ), 1, stateType_t::Int32 },
	{ "damage_from", offsetof( gclient_t, damage_from ), 3, stateType_t::Float32 },
	{ "damage_fromWorld", offsetof( gclient_t, damage_fromWorld ), 1, stateType_t::UInt32 },
	{ "accurateCount", offsetof( gclient_t, accurateCount ), 1, stateType_t::Int32 },
	{ "accuracy_shots", offsetof( gclient_t, accuracy_shots ), 1, stateType_t::Int32 },
	{ "accuracy_hits", offsetof( gclient_t, accuracy_hits ), 1, stateType_t::Int32 },
	{ "lastkilled_client", offsetof( gclient_t, lastkilled_client ), 1, stateType_t::Int32 },
	{ "lasthurt_client", offsetof( gclient_t, lasthurt_client ), 1, stateType_t::Int32 },
	{ "lasthurt_mod", offsetof( gclient_t, lasthurt_mod ), 1, stateType_t::Int32 },
	{ "respawnTime", offsetof( gclient_t, respawnTime ), 1, stateType_t::Int32 },
	{ "inactivityTime", offsetof( gclient_t, inactivityTime ), 1, stateType_t::Int32 },
	{ "inactivityWarning", offsetof( gclient_t, inactivityWarning ), 1, stateType_t::UInt32 },
	{ "rewardTime", offsetof( gclient_t, rewardTime ), 1, stateType_t::Int32 },
	{ "airOutTime", offsetof( gclient_t, airOutTime ), 1, stateType_t::Int32 },
	{ "lastKillTime", offsetof( gclient_t, lastKillTime ), 1, stateType_t::Int32 },
	{ "fireHeld", offsetof( gclient_t, fireHeld ), 1, stateType_t::UInt32 },
	{ "switchTeamTime", offsetof( gclient_t, switchTeamTime ), 1, stateType_t::Int32 },
	{ "timeResidual", offsetof( gclient_t, timeResidual ), 1, stateType_t::Int32 },
#ifdef MISSIONPACK
	{ "portalID", offsetof( gclient_t, portalID ), 1, stateType_t::Int32 },
	{ "ammoTimes", offsetof( gclient_t, ammoTimes ), WP_NUM_WEAPONS, stateType_t::Int32 },
	{ "invulnerabilityTime", offsetof( gclient_t, invulnerabilityTime ), 1, stateType_t::Int32 },
#endif
};
const stateSchema_t gameClientSchema = { "game.client", 1, 1, sizeof( gclient_t ), gameClientFields, sizeof( gameClientFields ) / sizeof( gameClientFields[0] ) };
struct gClientRefs_t {
	int32_t hook;
#ifdef MISSIONPACK
	int32_t persistantPowerup;
#endif
};
static constexpr stateField_t clientRefsFields[] = {
	{ "hook", offsetof( gClientRefs_t, hook ), 1, stateType_t::Int32 },
#ifdef MISSIONPACK
	{ "persistantPowerup", offsetof( gClientRefs_t, persistantPowerup ), 1, stateType_t::Int32 },
#endif
};
static constexpr stateSchema_t clientRefsSchema = { "game.clientRefs", 1, 1, sizeof( gClientRefs_t ), clientRefsFields, sizeof( clientRefsFields ) / sizeof( clientRefsFields[0] ) };
bool G_WriteClientState( stateWriter_t *writer, uint32_t slot, const gclient_t &client, const gStatePools_t &pools ) {
	if ( !writer )
		return false;
	// areabits is unused by the native game. Reject unexpected ownership instead of discarding it.
	if ( !G_StatePoolsValid( pools ) || slot >= MAX_CLIENTS || client.areabits ) {
		writer->failed = true;
		return false;
	}
	gClientRefs_t refs{ Slot( client.hook, pools.entities, pools.entityCount )
#ifdef MISSIONPACK
							,
		Slot( client.persistantPowerup, pools.entities, pools.entityCount )
#endif
	};
	if ( refs.hook == -2
#ifdef MISSIONPACK
		 || refs.persistantPowerup == -2
#endif
	) {
		writer->failed = true;
		return false;
	}
	return State_Append( writer, gameClientSchema, slot, &client ) &&
		   State_Append( writer, playerStateSchema, slot, &client.ps ) &&
		   State_Append( writer, usercmdStateSchema, slot, &client.pers.cmd ) &&
		   State_Append( writer, clientRefsSchema, slot, &refs );
}
bool G_ReadClientState( const stateReader_t &reader, uint32_t slot, const gStatePools_t &pools, gclient_t *client ) {
	if ( !client || !G_StatePoolsValid( pools ) || slot >= MAX_CLIENTS )
		return false;
	gclient_t restored{};
	gClientRefs_t refs;
	uint32_t version;
	if ( !State_Find( reader, gameClientSchema, slot, &restored, &version ) ||
		 !State_Find( reader, playerStateSchema, slot, &restored.ps, &version ) ||
		 !State_Find( reader, usercmdStateSchema, slot, &restored.pers.cmd, &version ) ||
		 !State_Find( reader, clientRefsSchema, slot, &refs, &version ) ||
		 refs.hook < -1 || refs.hook >= pools.entityCount
#ifdef MISSIONPACK
		 || refs.persistantPowerup < -1 || refs.persistantPowerup >= pools.entityCount
#endif
	)
		return false;
	restored.hook = refs.hook == -1 ? nullptr : &pools.entities[refs.hook];
#ifdef MISSIONPACK
	restored.persistantPowerup = refs.persistantPowerup == -1 ? nullptr : &pools.entities[refs.persistantPowerup];
#endif
	*client = restored;
	return true;
}

static constexpr stateField_t gameLevelFields[] = {
	{ "num_entities", offsetof( level_locals_t, num_entities ), 1, stateType_t::Int32 },
	{ "warmupTime", offsetof( level_locals_t, warmupTime ), 1, stateType_t::Int32 },
	{ "maxclients", offsetof( level_locals_t, maxclients ), 1, stateType_t::Int32 },
	{ "framenum", offsetof( level_locals_t, framenum ), 1, stateType_t::Int32 },
	{ "time", offsetof( level_locals_t, time ), 1, stateType_t::Int32 },
	{ "previousTime", offsetof( level_locals_t, previousTime ), 1, stateType_t::Int32 },
	{ "startTime", offsetof( level_locals_t, startTime ), 1, stateType_t::Int32 },
	{ "teamScores", offsetof( level_locals_t, teamScores ), TEAM_NUM_TEAMS, stateType_t::Int32 },
	{ "lastTeamLocationTime", offsetof( level_locals_t, lastTeamLocationTime ), 1, stateType_t::Int32 },
	{ "newSession", offsetof( level_locals_t, newSession ), 1, stateType_t::UInt32 },
	{ "restarted", offsetof( level_locals_t, restarted ), 1, stateType_t::UInt32 },
	{ "numConnectedClients", offsetof( level_locals_t, numConnectedClients ), 1, stateType_t::Int32 },
	{ "numNonSpectatorClients", offsetof( level_locals_t, numNonSpectatorClients ), 1, stateType_t::Int32 },
	{ "numPlayingClients", offsetof( level_locals_t, numPlayingClients ), 1, stateType_t::Int32 },
	{ "sortedClients", offsetof( level_locals_t, sortedClients ), MAX_CLIENTS, stateType_t::Int32 },
	{ "follow1", offsetof( level_locals_t, follow1 ), 1, stateType_t::Int32 },
	{ "follow2", offsetof( level_locals_t, follow2 ), 1, stateType_t::Int32 },
	{ "snd_fry", offsetof( level_locals_t, snd_fry ), 1, stateType_t::Int32 },
	{ "warmupModificationCount", offsetof( level_locals_t, warmupModificationCount ), 1, stateType_t::Int32 },
	{ "voteString", offsetof( level_locals_t, voteString ), MAX_STRING_CHARS, stateType_t::String },
	{ "voteDisplayString", offsetof( level_locals_t, voteDisplayString ), MAX_STRING_CHARS, stateType_t::String },
	{ "voteTime", offsetof( level_locals_t, voteTime ), 1, stateType_t::Int32 },
	{ "voteExecuteTime", offsetof( level_locals_t, voteExecuteTime ), 1, stateType_t::Int32 },
	{ "voteYes", offsetof( level_locals_t, voteYes ), 1, stateType_t::Int32 },
	{ "voteNo", offsetof( level_locals_t, voteNo ), 1, stateType_t::Int32 },
	{ "numVotingClients", offsetof( level_locals_t, numVotingClients ), 1, stateType_t::Int32 },
	{ "teamVoteString[0]", offsetof( level_locals_t, teamVoteString[0] ), MAX_STRING_CHARS, stateType_t::String },
	{ "teamVoteString[1]", offsetof( level_locals_t, teamVoteString[1] ), MAX_STRING_CHARS, stateType_t::String },
	{ "teamVoteTime", offsetof( level_locals_t, teamVoteTime ), 2, stateType_t::Int32 },
	{ "teamVoteYes", offsetof( level_locals_t, teamVoteYes ), 2, stateType_t::Int32 },
	{ "teamVoteNo", offsetof( level_locals_t, teamVoteNo ), 2, stateType_t::Int32 },
	{ "numteamVotingClients", offsetof( level_locals_t, numteamVotingClients ), 2, stateType_t::Int32 },
	{ "intermissionQueued", offsetof( level_locals_t, intermissionQueued ), 1, stateType_t::Int32 },
	{ "intermissiontime", offsetof( level_locals_t, intermissiontime ), 1, stateType_t::Int32 },
	{ "readyToExit", offsetof( level_locals_t, readyToExit ), 1, stateType_t::UInt32 },
	{ "exitTime", offsetof( level_locals_t, exitTime ), 1, stateType_t::Int32 },
	{ "intermission_origin", offsetof( level_locals_t, intermission_origin ), 3, stateType_t::Float32 },
	{ "intermission_angle", offsetof( level_locals_t, intermission_angle ), 3, stateType_t::Float32 },
	{ "locationLinked", offsetof( level_locals_t, locationLinked ), 1, stateType_t::UInt32 },
	{ "bodyQueIndex", offsetof( level_locals_t, bodyQueIndex ), 1, stateType_t::Int32 },
#ifdef MISSIONPACK
	{ "portalSequence", offsetof( level_locals_t, portalSequence ), 1, stateType_t::Int32 },
#endif
};
const stateSchema_t gameLevelSchema = { "game.level", 1, 1, sizeof( level_locals_t ), gameLevelFields, sizeof( gameLevelFields ) / sizeof( gameLevelFields[0] ) };
struct gLevelRefs_t {
	int32_t locationHead, bodyQue[BODY_QUEUE_SIZE];
};
static_assert( sizeof( gLevelRefs_t ) == 36 );
static constexpr stateField_t levelRefsFields[] = {
	{ "locationHead", offsetof( gLevelRefs_t, locationHead ), 1, stateType_t::Int32 },
	{ "bodyQue", offsetof( gLevelRefs_t, bodyQue ), BODY_QUEUE_SIZE, stateType_t::Int32 },
};
static constexpr stateSchema_t levelRefsSchema = { "game.levelRefs", 1, 1, sizeof( gLevelRefs_t ), levelRefsFields, 2 };
static constexpr stateField_t levelStringsFields[] = {
	{ "present", offsetof( gLevelStrings_t, present ), 1, stateType_t::UInt32 },
	{ "changemap", offsetof( gLevelStrings_t, changemap ), MAX_SPAWN_VARS_CHARS, stateType_t::String },
};
static constexpr stateSchema_t levelStringsSchema = { "game.levelStrings", 1, 1, sizeof( gLevelStrings_t ), levelStringsFields, 2 };
size_t G_LevelStringBytes( const gLevelStrings_t &strings ) {
	return strings.present > 1 ? SIZE_MAX : StringBytes( strings.changemap, strings.present != 0 );
}
bool G_RestoreLevelStrings( const gLevelStrings_t &strings, char *storage, size_t capacity, level_locals_t *level ) {
	const size_t bytes = G_LevelStringBytes( strings );
	if ( !level || bytes == SIZE_MAX || bytes > capacity || ( bytes && !storage ) )
		return false;
	level->changemap = RestoreString( strings.changemap, strings.present != 0, &storage );
	return true;
}
bool G_WriteLevelState( stateWriter_t *writer, const level_locals_t &level, const gStatePools_t &pools ) {
	if ( !writer )
		return false;
	if ( !G_StatePoolsValid( pools ) || level.spawning || level.gentities != pools.entities || level.clients != pools.clients ||
		 ( level.gentitySize && level.gentitySize != sizeof( gentity_t ) ) || level.num_entities < 0 || level.num_entities > pools.entityCount ||
		 level.maxclients < 0 || level.maxclients > pools.clientCount ) {
		writer->failed = true;
		return false;
	}
	gLevelRefs_t refs{};
	gLevelStrings_t strings{};
	refs.locationHead = Slot( level.locationHead, pools.entities, pools.entityCount );
	if ( refs.locationHead == -2 || !CaptureString( level.changemap, strings.changemap, 0, &strings.present ) ) {
		writer->failed = true;
		return false;
	}
	for ( int i = 0; i < BODY_QUEUE_SIZE; ++i ) {
		refs.bodyQue[i] = Slot( level.bodyQue[i], pools.entities, pools.entityCount );
		if ( refs.bodyQue[i] == -2 ) {
			writer->failed = true;
			return false;
		}
	}
	return State_Append( writer, gameLevelSchema, 0, &level ) && State_Append( writer, levelRefsSchema, 0, &refs ) && State_Append( writer, levelStringsSchema, 0, &strings );
}
bool G_ReadLevelState( const stateReader_t &reader, const gStatePools_t &pools, level_locals_t *level, gLevelStrings_t *strings ) {
	if ( !level || !strings || !G_StatePoolsValid( pools ) )
		return false;
	level_locals_t restored{};
	gLevelRefs_t refs;
	gLevelStrings_t text;
	uint32_t version;
	if ( !State_Find( reader, gameLevelSchema, 0, &restored, &version ) || !State_Find( reader, levelRefsSchema, 0, &refs, &version ) ||
		 !State_Find( reader, levelStringsSchema, 0, &text, &version ) || G_LevelStringBytes( text ) == SIZE_MAX ||
		 restored.num_entities < 0 || restored.num_entities > pools.entityCount || restored.maxclients < 0 || restored.maxclients > pools.clientCount ||
		 refs.locationHead < -1 || refs.locationHead >= pools.entityCount )
		return false;
	for ( int i = 0; i < BODY_QUEUE_SIZE; ++i ) {
		const int slot = refs.bodyQue[i];
		if ( slot < -1 || slot >= pools.entityCount )
			return false;
		restored.bodyQue[i] = slot == -1 ? nullptr : &pools.entities[slot];
	}
	restored.locationHead = refs.locationHead == -1 ? nullptr : &pools.entities[refs.locationHead];
	restored.clients = pools.clients;
	restored.gentities = pools.entities;
	restored.gentitySize = level->gentitySize; // Unused by native gameplay; map setup leaves zero.
	restored.logFile = level->logFile;
	// Spawn parsing is frame-local scratch; file handles and pool addresses come from map setup.
	*level = restored;
	*strings = text;
	return true;
}

struct cachedCvarSave_t {
	char name[64];
	uint32_t count, present;
	vmCvar_t value;
};
static_assert( sizeof( cachedCvarSave_t ) == 344 && offsetof( cachedCvarSave_t, value ) == 72 );
static constexpr stateField_t cachedCvarFields[] = {
	{ "name", offsetof( cachedCvarSave_t, name ), 64, stateType_t::String },
	{ "count", offsetof( cachedCvarSave_t, count ), 1, stateType_t::UInt32 },
	{ "present", offsetof( cachedCvarSave_t, present ), 1, stateType_t::UInt32 },
	{ "modificationCount", offsetof( cachedCvarSave_t, value.modificationCount ), 1, stateType_t::Int32 },
	{ "value", offsetof( cachedCvarSave_t, value.value ), 1, stateType_t::Float32 },
	{ "integer", offsetof( cachedCvarSave_t, value.integer ), 1, stateType_t::Int32 },
	{ "string", offsetof( cachedCvarSave_t, value.string ), MAX_CVAR_VALUE_STRING, stateType_t::String }
};
static bool EngineCvarGroup( const char *group, char ( &output )[64] ) {
	if ( !group )
		return false;
	const int size = snprintf( output, sizeof( output ), "engine.%s", group );
	return size > 0 && size < int( sizeof( output ) );
}
bool G_WriteCachedCvars( stateWriter_t *writer, const char *group, const gCachedCvar_t *bindings, uint32_t count ) {
	if ( !writer )
		return false;
	char engineGroup[64];
	if ( !bindings || !count || count > 128 || !EngineCvarGroup( group, engineGroup ) ) {
		writer->failed = true;
		return false;
	}
	const stateSchema_t schema = { group, 1, 1, sizeof( cachedCvarSave_t ), cachedCvarFields, 7 };
	for ( uint32_t i = 0; i < count; ++i ) {
		const auto &binding = bindings[i];
		if ( !binding.name || !binding.name[0] || strlen( binding.name ) >= 64 || ( binding.value && !std::isfinite( binding.value->value ) ) ) {
			writer->failed = true;
			return false;
		}
		cachedCvarSave_t saved{};
		strcpy( saved.name, binding.name );
		saved.count = count;
		saved.present = binding.value != nullptr;
		if ( binding.value )
			saved.value = *binding.value;
		if ( !State_Append( writer, schema, i, &saved ) || !GameImport_WriteCvarState( writer, engineGroup, i, binding.name ) ) {
			writer->failed = true;
			return false;
		}
	}
	return true;
}
bool G_ReadCachedCvars( const stateReader_t &reader, const char *group, const gCachedCvar_t *bindings, uint32_t count, int apply ) {
	char engineGroup[64];
	if ( apply < 0 || apply > 2 || !bindings || !count || count > 128 || !EngineCvarGroup( group, engineGroup ) )
		return false;
	const stateSchema_t schema = { group, 1, 1, sizeof( cachedCvarSave_t ), cachedCvarFields, 7 };
	cachedCvarSave_t saved[128];
	uint32_t version;
	for ( uint32_t i = 0; i < count; ++i )
		if ( !bindings[i].name || !State_Find( reader, schema, i, &saved[i], &version ) || saved[i].count != count ||
			 saved[i].present != uint32_t( bindings[i].value != nullptr ) || strcmp( saved[i].name, bindings[i].name ) || !std::isfinite( saved[i].value.value ) ||
			 !GameImport_ReadCvarState( &reader, engineGroup, i, bindings[i].name, false, false ) )
			return false;
	if ( apply )
		for ( uint32_t i = 0; i < count; ++i ) {
			if ( !GameImport_ReadCvarState( &reader, engineGroup, i, bindings[i].name, apply, false ) )
				return false;
			if ( bindings[i].value ) {
				saved[i].value.handle = bindings[i].value->handle;
				*bindings[i].value = saved[i].value;
			}
		}
	return true;
}

static bool ExtraCvarRecord( stateWriter_t *writer, const stateReader_t *reader, uint32_t slot, const char *name, int apply, bool removable = false ) {
	return writer ? GameImport_WriteCvarState( writer, "engine.game.extraCvars", slot, name ) != 0 : GameImport_ReadCvarState( reader, "engine.game.extraCvars", slot, name, apply, removable ) != 0;
}
static bool ExtraCvarRecords( stateWriter_t *writer, const stateReader_t *reader, int apply ) {
	static const char *const names[] = {
		"nextmap", "bot_enable", "bot_testichat", "bot_visualizejumppads", "bot_forceclustering",
		"bot_forcereachability", "bot_forcewrite", "bot_aasoptimize", "bot_reloadcharacters",
		"max_aaslinks", "max_levelitems", "g_entityDefinitions", "g_animationBody", "g_animationRifle",
		"g_weapons", "g_arenasFile", "g_botsFile", "session"
	};
	static_assert( sizeof( names ) / sizeof( names[0] ) < 128 );
	for ( uint32_t i = 0; i < sizeof( names ) / sizeof( names[0] ); ++i )
		if ( !ExtraCvarRecord( writer, reader, i, names[i], apply, !strcmp( names[i], "session" ) ) )
			return false;
	for ( uint32_t i = 0; i < MAX_CLIENTS; ++i ) {
		char name[64];
		snprintf( name, sizeof( name ), "session%u", i );
		if ( !ExtraCvarRecord( writer, reader, 128 + i, name, apply, true ) )
			return false;
		snprintf( name, sizeof( name ), "botsession%u", i );
		if ( !ExtraCvarRecord( writer, reader, 256 + i, name, apply, true ) )
			return false;
	}
	if ( bg_numItems < 1 || bg_numItems > MAX_ITEMS )
		return false;
	for ( uint32_t i = 1; i < uint32_t( bg_numItems ); ++i ) {
		if ( !bg_itemlist[i].classname )
			return false;
		char name[64];
		const int length = snprintf( name, sizeof( name ), "disable_%s", bg_itemlist[i].classname );
		if ( length < 0 || length >= int( sizeof( name ) ) || !ExtraCvarRecord( writer, reader, 512 + i, name, apply ) )
			return false;
	}
	return true;
}
bool G_WriteExtraCvarState( stateWriter_t *writer ) {
	if ( !writer )
		return false;
	if ( !ExtraCvarRecords( writer, nullptr, 0 ) ) {
		writer->failed = true;
		return false;
	}
	return true;
}
bool G_ReadExtraCvarState( const stateReader_t &reader, int apply ) {
	return apply >= 0 && apply <= 2 && ExtraCvarRecords( nullptr, &reader, 0 ) && ( !apply || ExtraCvarRecords( nullptr, &reader, apply ) );
}

// Draft validation never evaluates an out-of-range C enum before checking its
// representation. Some imported game enums deliberately retain the legacy ABI.
template <typename T>
static uint32_t StateEnum( const T &value ) {
	static_assert( sizeof( T ) == sizeof( uint32_t ) );
	uint32_t bits;
	memcpy( &bits, &value, sizeof( bits ) );
	return bits;
}
static bool StateFloatsFinite( const stateSchema_t &schema, const void *object ) {
	for ( uint32_t i = 0; i < schema.fieldCount; ++i ) {
		const auto &field = schema.fields[i];
		if ( field.type != stateType_t::Float32 )
			continue;
		for ( uint32_t j = 0; j < field.count; ++j ) {
			float value;
			memcpy( &value, (const uint8_t *)object + field.offset + j * sizeof( value ), sizeof( value ) );
			if ( !std::isfinite( value ) )
				return false;
		}
	}
	return true;
}
static bool ValidStateTrajectory( const trajectory_t &trajectory ) {
	return StateEnum( trajectory.trType ) <= TR_GRAVITY &&
		   ( StateEnum( trajectory.trType ) != TR_SINE || trajectory.trDuration > 0 );
}
static bool ValidNetworkEntityState( const entityState_t &entity ) {
	// Predictable temporary events carry the two event-sequence bits in eType.
	const uint32_t event = uint32_t( entity.eType ) - uint32_t( ET_EVENTS );
	const bool kind = ( entity.eType >= ET_GENERAL && entity.eType < ET_EVENTS ) ||
					  ( event & ~uint32_t( EV_EVENT_BITS ) ) <= uint32_t( EV_WEAPON_NOTIFY ) ||
					  entity.eType == ET_ANIMATION || entity.eType == ET_WEAPON_STATE || entity.eType == ET_WEAPON_ANIMATION;
	return kind && entity.number >= 0 && entity.number < MAX_GENTITIES &&
		   ValidStateTrajectory( entity.pos ) && ValidStateTrajectory( entity.apos ) && StateFloatsFinite( entityStateSchema, &entity );
}
bool G_ValidateEntityState( uint32_t slot, const gentity_t &entity, const gStatePools_t &pools ) {
	if ( !G_StatePoolsValid( pools ) || slot >= uint32_t( pools.entityCount ) )
		return false;
	const uint32_t flags[] = { StateEnum( entity.inuse ), StateEnum( entity.neverFree ), StateEnum( entity.freeAfterEvent ),
		StateEnum( entity.unlinkAfterEvent ), StateEnum( entity.physicsObject ), StateEnum( entity.takedamage ), StateEnum( entity.r.linked ), StateEnum( entity.r.bmodel ) };
	for ( uint32_t flag : flags )
		if ( flag > 1 )
			return false;
	if ( StateEnum( entity.moverState ) > MOVER_2TO1 || !StateFloatsFinite( gameEntitySchema, &entity ) ||
		 !StateFloatsFinite( entitySharedSchema, &entity.r ) || !ValidNetworkEntityState( entity.s ) || !ValidNetworkEntityState( entity.r.s ) ||
		 ( entity.inuse && entity.s.number != int( slot ) ) || entity.r.ownerNum < -1 || entity.r.ownerNum >= MAX_GENTITIES ||
		 entity.waterlevel < 0 || entity.waterlevel > 3 || entity.methodOfDeath < MOD_UNKNOWN || entity.methodOfDeath > MOD_GRAPPLE ||
		 entity.splashMethodOfDeath < MOD_UNKNOWN || entity.splashMethodOfDeath > MOD_GRAPPLE )
		return false;
	if ( entity.client && ( slot >= uint32_t( pools.clientCount ) || entity.client != &pools.clients[slot] ) )
		return false;
	if ( ( entity.r.svFlags & ( SVF_SINGLECLIENT | SVF_NOTSINGLECLIENT ) ) && !( entity.r.svFlags & SVF_CLIENTMASK ) &&
		 ( entity.r.singleClient < 0 || entity.r.singleClient >= pools.clientCount ) )
		return false;
	for ( int i = 0; i < 3; ++i )
		if ( entity.r.mins[i] > entity.r.maxs[i] || ( entity.r.linked && entity.r.absmin[i] > entity.r.absmax[i] ) )
			return false;
	return true;
}
bool G_ValidateClientState( uint32_t slot, const gclient_t &client, const gStatePools_t &pools ) {
	if ( !G_StatePoolsValid( pools ) || slot >= uint32_t( pools.clientCount ) || StateEnum( client.pers.connected ) > CON_CONNECTED ||
		 StateEnum( client.pers.teamState.state ) > TEAM_ACTIVE || StateEnum( client.sess.sessionTeam ) >= TEAM_NUM_TEAMS ||
		 StateEnum( client.sess.spectatorState ) > SPECTATOR_SCOREBOARD || !StateFloatsFinite( gameClientSchema, &client ) || !StateFloatsFinite( playerStateSchema, &client.ps ) )
		return false;
	const uint32_t flags[] = { StateEnum( client.pers.localClient ), StateEnum( client.pers.initialSpawn ), StateEnum( client.pers.predictItemPickup ),
		StateEnum( client.pers.pmoveFixed ), StateEnum( client.pers.teamInfo ), StateEnum( client.sess.teamLeader ), StateEnum( client.readyToExit ),
		StateEnum( client.noclip ), StateEnum( client.damage_fromWorld ), StateEnum( client.inactivityWarning ), StateEnum( client.fireHeld ) };
	for ( uint32_t flag : flags )
		if ( flag > 1 )
			return false;
	return client.sess.spectatorClient >= -2 && client.sess.spectatorClient < MAX_CLIENTS &&
		   client.ps.pm_type >= PM_NORMAL && client.ps.pm_type <= PM_SPINTERMISSION &&
		   client.ps.weaponstate >= WEAPON_READY && client.ps.weaponstate <= WEAPON_FIRING &&
		   client.ps.weapon >= WP_NONE && client.ps.weapon < WP_NUM_WEAPONS &&
		   client.ps.groundEntityNum >= -1 && client.ps.groundEntityNum < MAX_GENTITIES &&
		   client.ps.jumppad_ent >= -1 && client.ps.jumppad_ent < MAX_GENTITIES &&
		   client.ps.clientNum >= 0 && client.ps.clientNum < MAX_CLIENTS &&
		   ( client.pers.connected != CON_CONNECTED || client.ps.clientNum == int( slot ) ) &&
		   client.ps.persistant[PERS_TEAM] >= TEAM_FREE && client.ps.persistant[PERS_TEAM] < TEAM_NUM_TEAMS;
}
bool G_ValidateLevelState( const level_locals_t &saved, const gStatePools_t &pools ) {
	if ( !G_StatePoolsValid( pools ) || saved.maxclients < 1 || saved.maxclients > pools.clientCount ||
		 saved.num_entities < MAX_CLIENTS || saved.num_entities > pools.entityCount || !StateFloatsFinite( gameLevelSchema, &saved ) ||
		 saved.numConnectedClients < 0 || saved.numConnectedClients > saved.maxclients || saved.numNonSpectatorClients < 0 ||
		 saved.numNonSpectatorClients > saved.numConnectedClients || saved.numPlayingClients < 0 || saved.numPlayingClients > saved.numNonSpectatorClients ||
		 saved.follow1 < -1 || saved.follow1 >= saved.maxclients || saved.follow2 < -1 || saved.follow2 >= saved.maxclients ||
		 saved.bodyQueIndex < 0 || saved.bodyQueIndex >= BODY_QUEUE_SIZE )
		return false;
	const uint32_t flags[] = { StateEnum( saved.newSession ), StateEnum( saved.restarted ), StateEnum( saved.readyToExit ), StateEnum( saved.locationLinked ) };
	for ( uint32_t flag : flags )
		if ( flag > 1 )
			return false;
	bool seen[MAX_CLIENTS] = {};
	for ( int i = 0; i < saved.numConnectedClients; ++i ) {
		const int client = saved.sortedClients[i];
		if ( client < 0 || client >= saved.maxclients || seen[client] )
			return false;
		seen[client] = true;
	}
	const int votes[] = { saved.voteYes, saved.voteNo, saved.numVotingClients, saved.teamVoteYes[0], saved.teamVoteYes[1],
		saved.teamVoteNo[0], saved.teamVoteNo[1], saved.numteamVotingClients[0], saved.numteamVotingClients[1] };
	for ( int count : votes )
		if ( count < 0 || count > MAX_CLIENTS )
			return false;
	return true;
}


static constexpr stateField_t nativeRandomField{ "seed", 0, 1, stateType_t::UInt32 };
static constexpr stateSchema_t nativeRandomSchema{ "game.random", 1, 1, sizeof( uint32_t ), &nativeRandomField, 1 };
static bool ReadCheckpointCvars( const stateReader_t &reader, int apply ) {
	return G_ReadMainCvarState( reader, apply ) && G_ReadBotCvarState( reader, apply ) && G_ReadBotNavigationCvarState( reader, apply ) &&
		   G_ReadBotQueueCvarState( reader, apply ) && G_ReadAnimationCvarState( reader, apply ) && G_ReadWeaponCvarState( reader, apply ) &&
		   G_ReadRewindCvarState( reader, apply ) && G_ReadExtraCvarState( reader, apply );
}
bool G_ReadCheckpointCvars( const stateReader_t &reader, int apply ) {
	return apply >= 0 && apply <= 2 && ReadCheckpointCvars( reader, 0 ) && ( !apply || ReadCheckpointCvars( reader, apply ) );
}
static bool CheckpointOwnerWrite( bool success, const char *name ) {
	if ( !success )
		G_Printf( "Checkpoint: %s record failed.\n", name );
	return success;
}
static bool WriteCheckpointOwners( stateWriter_t *writer, const gStatePools_t &pools ) {
	return CheckpointOwnerWrite( G_WriteComposedState( writer ), "G_WriteComposedState" ) && CheckpointOwnerWrite( G_WriteAnimationState( writer, pools ), "G_WriteAnimationState" ) && CheckpointOwnerWrite( G_WriteWeaponState( writer, pools ), "G_WriteWeaponState" ) &&
		   CheckpointOwnerWrite( G_WriteRewindState( writer ), "G_WriteRewindState" ) && CheckpointOwnerWrite( G_WriteUtilityState( writer ), "G_WriteUtilityState" ) && CheckpointOwnerWrite( G_WriteCombatState( writer ), "G_WriteCombatState" ) && CheckpointOwnerWrite( G_WritePodiumState( writer, pools ), "G_WritePodiumState" ) &&
		   CheckpointOwnerWrite( G_WriteTeamState( writer, pools ), "G_WriteTeamState" ) && CheckpointOwnerWrite( G_WriteDefinitionState( writer ), "G_WriteDefinitionState" ) && CheckpointOwnerWrite( G_WriteEditorState( writer ), "G_WriteEditorState" ) && CheckpointOwnerWrite( G_WriteBotQueueState( writer ), "G_WriteBotQueueState" ) &&
		   CheckpointOwnerWrite( G_WriteBotClockState( writer ), "G_WriteBotClockState" ) && CheckpointOwnerWrite( G_WriteBotTeamState( writer ), "G_WriteBotTeamState" ) && CheckpointOwnerWrite( G_WriteWaypointState( writer ), "G_WriteWaypointState" ) && CheckpointOwnerWrite( G_WriteBotPoolState( writer ), "G_WriteBotPoolState" ) &&
		   CheckpointOwnerWrite( G_WriteBotNavigationState( writer ), "G_WriteBotNavigationState" ) && CheckpointOwnerWrite( G_WriteMemoryState( writer ), "G_WriteMemoryState" ) && CheckpointOwnerWrite( G_WriteRegisteredItemState( writer ), "G_WriteRegisteredItemState" ) && CheckpointOwnerWrite( G_WriteIPFilterState( writer ), "G_WriteIPFilterState" ) &&
		   CheckpointOwnerWrite( G_WriteBotInfoState( writer ), "G_WriteBotInfoState" ) && CheckpointOwnerWrite( G_WriteMainCvarState( writer ), "G_WriteMainCvarState" ) && CheckpointOwnerWrite( G_WriteBotCvarState( writer ), "G_WriteBotCvarState" ) && CheckpointOwnerWrite( G_WriteBotNavigationCvarState( writer ), "G_WriteBotNavigationCvarState" ) &&
		   CheckpointOwnerWrite( G_WriteBotQueueCvarState( writer ), "G_WriteBotQueueCvarState" ) && CheckpointOwnerWrite( G_WriteAnimationCvarState( writer ), "G_WriteAnimationCvarState" ) && CheckpointOwnerWrite( G_WriteWeaponCvarState( writer ), "G_WriteWeaponCvarState" ) && CheckpointOwnerWrite( G_WriteRewindCvarState( writer ), "G_WriteRewindCvarState" ) &&
		   CheckpointOwnerWrite( G_WriteExtraCvarState( writer ), "G_WriteExtraCvarState" );
}
static bool ReadCheckpointOwners( const stateReader_t &reader, const gStatePools_t &pools, bool apply ) {
	return G_ReadComposedState( reader, apply ) && G_ReadAnimationState( reader, pools, apply ) && G_ReadWeaponState( reader, pools, apply ) &&
		   G_ReadRewindState( reader, apply ) && G_ReadUtilityState( reader, apply ) && G_ReadCombatState( reader, apply ) && G_ReadPodiumState( reader, pools, apply ) &&
		   G_ReadTeamState( reader, pools, apply ) && G_ReadDefinitionState( reader, apply ) && G_ReadEditorState( reader, apply ) && G_ReadBotQueueState( reader, apply ) &&
		   G_ReadBotClockState( reader, apply ) && G_ReadBotTeamState( reader, apply ) && G_ReadWaypointState( reader, apply ) && G_ReadBotPoolState( reader, apply ) &&
		   G_ReadBotNavigationState( reader, apply ) && G_ReadMemoryState( reader, apply ) && G_ReadRegisteredItemState( reader, apply ) && G_ReadIPFilterState( reader, apply ) &&
		   G_ReadBotInfoState( reader ) && ReadCheckpointCvars( reader, apply ? 1 : 0 );
}
bool G_WriteCheckpoint( stateWriter_t *writer ) {
	if ( !writer )
		return false;
	const gStatePools_t pools{ g_entities, MAX_GENTITIES, level.clients, MAX_CLIENTS, bg_itemlist, bg_numItems };
	const auto *callbacks = G_StateCallbackTables();
	const uint32_t seed = Q_GetRandomSeed();
	if ( !G_ValidateLevelState( level, pools ) || !G_WriteLevelState( writer, level, pools ) || !State_Append( writer, nativeRandomSchema, 0, &seed ) ) {
		writer->failed = true;
		G_Printf( "Checkpoint: level record failed.\n" );
		return false;
	}
	for ( uint32_t i = 0; i < MAX_GENTITIES; ++i )
		if ( !G_ValidateEntityState( i, g_entities[i], pools ) || !G_WriteEntityState( writer, i, g_entities[i], pools, callbacks ) ) {
			G_Printf( "Checkpoint: entity %u (%s) record failed; type %d, number %d, trajectory %u/%u, client %d, semantic %d.\n", i,
				g_entities[i].classname ? g_entities[i].classname : "empty", g_entities[i].s.eType, g_entities[i].s.number,
				StateEnum( g_entities[i].s.pos.trType ), StateEnum( g_entities[i].s.apos.trType ),
				g_entities[i].client != nullptr, G_ValidateEntityState( i, g_entities[i], pools ) );
			writer->failed = true;
			return false;
		}
	for ( uint32_t i = 0; i < MAX_CLIENTS; ++i )
		if ( !G_ValidateClientState( i, level.clients[i], pools ) || !G_WriteClientState( writer, i, level.clients[i], pools ) ) {
			G_Printf( "Checkpoint: client %u record failed.\n", i );
			writer->failed = true;
			return false;
		}
	if ( !WriteCheckpointOwners( writer, pools ) ) {
		writer->failed = true;
		return false;
	}
	return true;
}
bool G_ReadCheckpoint( const stateReader_t &reader, bool apply ) {
	const gStatePools_t pools{ g_entities, MAX_GENTITIES, level.clients, MAX_CLIENTS, bg_itemlist, bg_numItems };
	const auto *callbacks = G_StateCallbackTables();
	static gentity_t entities[MAX_GENTITIES];
	static gclient_t clients[MAX_CLIENTS];
	level_locals_t restoredLevel{};
	restoredLevel.logFile = level.logFile;
	gLevelStrings_t levelStrings;
	uint32_t seed, version;
	if ( !G_StatePoolsValid( pools ) || !G_ReadLevelState( reader, pools, &restoredLevel, &levelStrings ) ||
		 !G_ValidateLevelState( restoredLevel, pools ) || restoredLevel.maxclients != level.maxclients ||
		 !State_Find( reader, nativeRandomSchema, 0, &seed, &version ) )
		return false;
	size_t stringBytes = G_LevelStringBytes( levelStrings );
	gEntityStrings_t strings;
	for ( uint32_t i = 0; i < MAX_GENTITIES; ++i ) {
		if ( !G_ReadEntityState( reader, i, pools, callbacks, &entities[i], &strings ) || !G_ValidateEntityState( i, entities[i], pools ) )
			return false;
		const size_t bytes = G_EntityStringBytes( strings );
		if ( bytes == SIZE_MAX || bytes > SIZE_MAX - stringBytes )
			return false;
		stringBytes += bytes;
	}
	// Same bounded entity/string limits as the individual records, with one allocation.
	if ( stringBytes > uint64_t( MAX_GENTITIES + 1 ) * sizeof( gEntityStrings_t ) )
		return false;
	for ( uint32_t i = 0; i < MAX_CLIENTS; ++i )
		if ( !G_ReadClientState( reader, i, pools, &clients[i] ) || !G_ValidateClientState( i, clients[i], pools ) )
			return false;
	if ( !ReadCheckpointOwners( reader, pools, false ) || !G_ValidateBotReferences( reader, entities, clients ) )
		return false;
	if ( !apply )
		return true;
	// Called only in a freshly loaded, stopped world. The immutable reader was
	// completely validated above; failures here require discarding that world.
	char *storage = stringBytes ? (char *)GameImport_AllocLevelMemory( uint32_t( stringBytes ) ) : nullptr;
	size_t remaining = stringBytes;
	const size_t levelBytes = G_LevelStringBytes( levelStrings );
	if ( !G_RestoreLevelStrings( levelStrings, storage, remaining, &restoredLevel ) )
		return false;
	if ( levelBytes )
		storage += levelBytes;
	remaining -= levelBytes;
	for ( uint32_t i = 0; i < MAX_GENTITIES; ++i ) {
		// Decode text one entity at a time; do not keep a 40 MiB string draft.
		if ( !State_Find( reader, entityStringsSchema, i, &strings, &version ) || !G_RestoreEntityStrings( strings, storage, remaining, &entities[i] ) )
			return false;
		const size_t bytes = G_EntityStringBytes( strings );
		if ( bytes )
			storage += bytes;
		remaining -= bytes;
	}
	memcpy( g_entities, entities, sizeof( entities ) );
	memcpy( level.clients, clients, sizeof( clients ) );
	level = restoredLevel;
	if ( !ReadCheckpointOwners( reader, pools, true ) )
		return false;
	trap_LocateGameData( g_entities, level.num_entities, sizeof( gentity_t ), &level.clients[0].ps, sizeof( gclient_t ) );
	return true;
}
bool G_RestoreCheckpointRandom( const stateReader_t &reader ) {
	uint32_t seed, version;
	if ( !State_Find( reader, nativeRandomSchema, 0, &seed, &version ) )
		return false;
	srand( seed );
	return true;
}

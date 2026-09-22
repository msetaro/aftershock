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
		 level.gentitySize != sizeof( gentity_t ) || level.num_entities < 0 || level.num_entities > pools.entityCount ||
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
	restored.gentitySize = sizeof( gentity_t );
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
bool G_WriteCachedCvars( stateWriter_t *writer, const char *group, const gCachedCvar_t *bindings, uint32_t count ) {
	if ( !writer )
		return false;
	if ( !bindings || !count || count > 128 ) {
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
		if ( !State_Append( writer, schema, i, &saved ) )
			return false;
	}
	return true;
}
bool G_ReadCachedCvars( const stateReader_t &reader, const char *group, const gCachedCvar_t *bindings, uint32_t count, bool apply ) {
	if ( !bindings || !count || count > 128 )
		return false;
	const stateSchema_t schema = { group, 1, 1, sizeof( cachedCvarSave_t ), cachedCvarFields, 7 };
	cachedCvarSave_t saved[128];
	uint32_t version;
	for ( uint32_t i = 0; i < count; ++i )
		if ( !bindings[i].name || !State_Find( reader, schema, i, &saved[i], &version ) || saved[i].count != count ||
			 saved[i].present != uint32_t( bindings[i].value != nullptr ) || strcmp( saved[i].name, bindings[i].name ) || !std::isfinite( saved[i].value.value ) )
			return false;
	if ( apply )
		for ( uint32_t i = 0; i < count; ++i )
			if ( bindings[i].value ) {
				saved[i].value.handle = bindings[i].value->handle;
				*bindings[i].value = saved[i].value;
			}
	return true;
}

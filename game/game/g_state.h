#ifndef G_STATE_H
#define G_STATE_H
#include "../../engine/public/state_public.h"

// Addresses remain process-local. Checkpoints store typed callback names.
struct gSaveCallback_t {
	const char *name;
	decltype( gentity_t::think ) think = nullptr;
	decltype( gentity_t::reached ) reached = nullptr;
	decltype( gentity_t::blocked ) blocked = nullptr;
	decltype( gentity_t::touch ) touch = nullptr;
	decltype( gentity_t::use ) use = nullptr;
	decltype( gentity_t::pain ) pain = nullptr;
	decltype( gentity_t::die ) die = nullptr;
};
struct gCallbackNames_t {
	char think[64];
	char reached[64];
	char blocked[64];
	char touch[64];
	char use[64];
	char pain[64];
	char die[64];
};
static_assert( sizeof( gCallbackNames_t ) == 448 );
bool G_CaptureCallbacks( const gentity_t &entity, const gSaveCallback_t *const *tables, gCallbackNames_t *names );
bool G_RestoreCallbacks( const gCallbackNames_t &names, const gSaveCallback_t *const *tables, gentity_t *entity );
const gSaveCallback_t *const *G_StateCallbackTables();
extern const stateSchema_t callbackNamesSchema;

struct gStatePools_t {
	gentity_t *entities;
	int entityCount;
	gclient_t *clients;
	int clientCount;
	gitem_t *items;
	int itemCount;
};
bool G_StatePoolsValid( const gStatePools_t &pools );
int G_StateEntitySlot( const gentity_t *entity, const gStatePools_t &pools );
struct gEntityRefs_t {
	int32_t client, parent, nextTrain, prevTrain, target_ent, chain, enemy, activator, teamchain, teammaster;
	char item[64];
};
static_assert( sizeof( gEntityRefs_t ) == 104 && offsetof( gEntityRefs_t, item ) == 40 );
bool G_CaptureEntityRefs( const gentity_t &entity, const gStatePools_t &pools, gEntityRefs_t *references );
bool G_RestoreEntityRefs( const gEntityRefs_t &references, const gStatePools_t &pools, gentity_t *entity );
extern const stateSchema_t entityRefsSchema;

struct gEntityStrings_t {
	uint32_t present;
	char classname[MAX_SPAWN_VARS_CHARS];
	char model[MAX_SPAWN_VARS_CHARS];
	char model2[MAX_SPAWN_VARS_CHARS];
	char message[MAX_SPAWN_VARS_CHARS];
	char target[MAX_SPAWN_VARS_CHARS];
	char targetname[MAX_SPAWN_VARS_CHARS];
	char team[MAX_SPAWN_VARS_CHARS];
	char targetShaderName[MAX_SPAWN_VARS_CHARS];
	char targetShaderNewName[MAX_SPAWN_VARS_CHARS];
	char definitionName[MAX_SPAWN_VARS_CHARS];
};
static_assert( sizeof( gEntityStrings_t ) == 40964 );
bool G_CaptureEntityStrings( const gentity_t &entity, gEntityStrings_t *strings );
// SIZE_MAX means invalid; zero means every string is null.
size_t G_EntityStringBytes( const gEntityStrings_t &strings );
// Storage is caller-owned level-lifetime memory, disjoint from strings/entity.
// No output is changed unless the complete record and storage capacity validate.
bool G_RestoreEntityStrings( const gEntityStrings_t &strings, char *storage, size_t capacity, gentity_t *entity );
extern const stateSchema_t entityStringsSchema;
extern const stateSchema_t gameEntitySchema, entitySharedSchema;
bool G_WriteEntityState( stateWriter_t *writer, uint32_t slot, const gentity_t &entity, const gStatePools_t &pools, const gSaveCallback_t *const *callbacks );
// Returns a draft, not a linked/live entity. The checkpoint coordinator validates
// gameplay invariants, provides string lifetime storage and rebuilds spatial links.
bool G_ReadEntityState( const stateReader_t &reader, uint32_t slot, const gStatePools_t &pools, const gSaveCallback_t *const *callbacks,
	gentity_t *entity, gEntityStrings_t *strings );
extern const stateSchema_t gameClientSchema;
bool G_WriteClientState( stateWriter_t *writer, uint32_t slot, const gclient_t &client, const gStatePools_t &pools );
bool G_ReadClientState( const stateReader_t &reader, uint32_t slot, const gStatePools_t &pools, gclient_t *client );
struct gLevelStrings_t {
	uint32_t present;
	char changemap[MAX_SPAWN_VARS_CHARS];
};
static_assert( sizeof( gLevelStrings_t ) == 4100 );
extern const stateSchema_t gameLevelSchema;
size_t G_LevelStringBytes( const gLevelStrings_t &strings );
bool G_RestoreLevelStrings( const gLevelStrings_t &strings, char *storage, size_t capacity, level_locals_t *level );
bool G_WriteLevelState( stateWriter_t *writer, const level_locals_t &level, const gStatePools_t &pools );
bool G_ReadLevelState( const stateReader_t &reader, const gStatePools_t &pools, level_locals_t *level, gLevelStrings_t *strings );
bool G_WriteComposedState( stateWriter_t *writer );
bool G_ReadComposedState( const stateReader_t &reader, bool apply );
bool G_WriteAnimationState( stateWriter_t *writer, const gStatePools_t &pools );
bool G_ReadAnimationState( const stateReader_t &reader, const gStatePools_t &pools, bool apply );
bool G_WriteWeaponState( stateWriter_t *writer, const gStatePools_t &pools );
bool G_ReadWeaponState( const stateReader_t &reader, const gStatePools_t &pools, bool apply );
bool G_WriteRewindState( stateWriter_t *writer );
bool G_ReadRewindState( const stateReader_t &reader, bool apply );
bool G_WriteUtilityState( stateWriter_t *writer );
bool G_ReadUtilityState( const stateReader_t &reader, bool apply );
bool G_WriteCombatState( stateWriter_t *writer );
bool G_ReadCombatState( const stateReader_t &reader, bool apply );
bool G_WritePodiumState( stateWriter_t *writer, const gStatePools_t &pools );
bool G_ReadPodiumState( const stateReader_t &reader, const gStatePools_t &pools, bool apply );
bool G_WriteTeamState( stateWriter_t *writer, const gStatePools_t &pools );
bool G_ReadTeamState( const stateReader_t &reader, const gStatePools_t &pools, bool apply );
bool G_WriteDefinitionState( stateWriter_t *writer );
bool G_ReadDefinitionState( const stateReader_t &reader, bool apply );
bool G_WriteEditorState( stateWriter_t *writer );
bool G_ReadEditorState( const stateReader_t &reader, bool apply );
#endif

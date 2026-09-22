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
struct gEntityRefs_t {
	int32_t client, parent, nextTrain, prevTrain, target_ent, chain, enemy, activator, teamchain, teammaster;
	char item[64];
};
static_assert( sizeof( gEntityRefs_t ) == 104 && offsetof( gEntityRefs_t, item ) == 40 );
bool G_CaptureEntityRefs( const gentity_t &entity, const gStatePools_t &pools, gEntityRefs_t *references );
bool G_RestoreEntityRefs( const gEntityRefs_t &references, const gStatePools_t &pools, gentity_t *entity );
extern const stateSchema_t entityRefsSchema;
#endif

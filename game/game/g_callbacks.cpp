#include "g_local.h"

extern const gSaveCallback_t saveCallbacks_g_arenas[];
extern const gSaveCallback_t saveCallbacks_g_client[];
extern const gSaveCallback_t saveCallbacks_g_combat[];
extern const gSaveCallback_t saveCallbacks_g_composed[];
extern const gSaveCallback_t saveCallbacks_g_items[];
extern const gSaveCallback_t saveCallbacks_g_misc[];
extern const gSaveCallback_t saveCallbacks_g_missile[];
extern const gSaveCallback_t saveCallbacks_g_mover[];
extern const gSaveCallback_t saveCallbacks_g_rewind[];
extern const gSaveCallback_t saveCallbacks_g_target[];
extern const gSaveCallback_t saveCallbacks_g_team[];
extern const gSaveCallback_t saveCallbacks_g_trigger[];
extern const gSaveCallback_t saveCallbacks_g_utils[];
extern const gSaveCallback_t saveCallbacks_g_weapon[];
const gSaveCallback_t *const *G_StateCallbackTables() {
	static const gSaveCallback_t *const tables[] = {
		saveCallbacks_g_arenas,
		saveCallbacks_g_client,
		saveCallbacks_g_combat,
		saveCallbacks_g_composed,
		saveCallbacks_g_items,
		saveCallbacks_g_misc,
		saveCallbacks_g_missile,
		saveCallbacks_g_mover,
		saveCallbacks_g_rewind,
		saveCallbacks_g_target,
		saveCallbacks_g_team,
		saveCallbacks_g_trigger,
		saveCallbacks_g_utils,
		saveCallbacks_g_weapon,
		nullptr
	};
	return tables;
}

template <typename T>
static const char *CallbackName( T callback, T gSaveCallback_t::*member, const gSaveCallback_t *const *tables ) {
	if ( !callback )
		return "";
	for ( const auto *const *table = tables; *table; ++table )
		for ( const auto *entry = *table; entry->name; ++entry )
			if ( entry->*member == callback )
				return entry->name;
	return nullptr;
}
template <typename T>
static bool CallbackValue( const char ( &name )[64], T gSaveCallback_t::*member, const gSaveCallback_t *const *tables, T *callback ) {
	if ( !memchr( name, 0, sizeof( name ) ) )
		return false;
	if ( !name[0] ) {
		*callback = nullptr;
		return true;
	}
	for ( const auto *const *table = tables; *table; ++table )
		for ( const auto *entry = *table; entry->name; ++entry )
			if ( entry->*member && !strcmp( entry->name, name ) ) {
				*callback = entry->*member;
				return true;
			}
	return false;
}
bool G_CaptureCallbacks( const gentity_t &entity, const gSaveCallback_t *const *tables, gCallbackNames_t *names ) {
	if ( !tables || !names )
		return false;
	gCallbackNames_t captured{};
	const char *think = CallbackName( entity.think, &gSaveCallback_t::think, tables );
	if ( !think || strlen( think ) >= sizeof( captured.think ) )
		return false;
	strcpy( captured.think, think );
	const char *reached = CallbackName( entity.reached, &gSaveCallback_t::reached, tables );
	if ( !reached || strlen( reached ) >= sizeof( captured.reached ) )
		return false;
	strcpy( captured.reached, reached );
	const char *blocked = CallbackName( entity.blocked, &gSaveCallback_t::blocked, tables );
	if ( !blocked || strlen( blocked ) >= sizeof( captured.blocked ) )
		return false;
	strcpy( captured.blocked, blocked );
	const char *touch = CallbackName( entity.touch, &gSaveCallback_t::touch, tables );
	if ( !touch || strlen( touch ) >= sizeof( captured.touch ) )
		return false;
	strcpy( captured.touch, touch );
	const char *use = CallbackName( entity.use, &gSaveCallback_t::use, tables );
	if ( !use || strlen( use ) >= sizeof( captured.use ) )
		return false;
	strcpy( captured.use, use );
	const char *pain = CallbackName( entity.pain, &gSaveCallback_t::pain, tables );
	if ( !pain || strlen( pain ) >= sizeof( captured.pain ) )
		return false;
	strcpy( captured.pain, pain );
	const char *die = CallbackName( entity.die, &gSaveCallback_t::die, tables );
	if ( !die || strlen( die ) >= sizeof( captured.die ) )
		return false;
	strcpy( captured.die, die );
	*names = captured;
	return true;
}
bool G_RestoreCallbacks( const gCallbackNames_t &names, const gSaveCallback_t *const *tables, gentity_t *entity ) {
	if ( !tables || !entity )
		return false;
	auto restored = *entity;
	if ( !CallbackValue( names.think, &gSaveCallback_t::think, tables, &restored.think ) ||
		 !CallbackValue( names.reached, &gSaveCallback_t::reached, tables, &restored.reached ) ||
		 !CallbackValue( names.blocked, &gSaveCallback_t::blocked, tables, &restored.blocked ) ||
		 !CallbackValue( names.touch, &gSaveCallback_t::touch, tables, &restored.touch ) ||
		 !CallbackValue( names.use, &gSaveCallback_t::use, tables, &restored.use ) ||
		 !CallbackValue( names.pain, &gSaveCallback_t::pain, tables, &restored.pain ) ||
		 !CallbackValue( names.die, &gSaveCallback_t::die, tables, &restored.die ) )
		return false;
	*entity = restored;
	return true;
}
static constexpr stateField_t callbackNamesFields[] = {
	{ "think", offsetof( gCallbackNames_t, think ), 64, stateType_t::String },
	{ "reached", offsetof( gCallbackNames_t, reached ), 64, stateType_t::String },
	{ "blocked", offsetof( gCallbackNames_t, blocked ), 64, stateType_t::String },
	{ "touch", offsetof( gCallbackNames_t, touch ), 64, stateType_t::String },
	{ "use", offsetof( gCallbackNames_t, use ), 64, stateType_t::String },
	{ "pain", offsetof( gCallbackNames_t, pain ), 64, stateType_t::String },
	{ "die", offsetof( gCallbackNames_t, die ), 64, stateType_t::String },
};
const stateSchema_t callbackNamesSchema = { "game.callbacks", 1, 1, sizeof( gCallbackNames_t ), callbackNamesFields, 7 };

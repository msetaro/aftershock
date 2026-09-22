#define NATIVE_NAMESPACE game
#define NATIVE_SOURCE "game/g_callbacks.cpp"
#include "../../game/module.cpp"

static void Think( game::gentity_t * ) {
}
static void OtherThink( game::gentity_t * ) {
}
static void Touch( game::gentity_t *, game::gentity_t *, game::trace_t * ) {
}
static void Use( game::gentity_t *, game::gentity_t *, game::gentity_t * ) {
}
static void Pain( game::gentity_t *, game::gentity_t *, int ) {
}
static void Die( game::gentity_t *, game::gentity_t *, game::gentity_t *, int, int ) {
}
static void Blocked( game::gentity_t *, game::gentity_t * ) {
}

int main() {
	const game::gSaveCallback_t first[] = { { .name = "think", .think = Think }, { .name = "touch", .touch = Touch }, { nullptr } };
	const game::gSaveCallback_t second[] = {
		{ .name = "reached", .reached = Think }, { .name = "blocked", .blocked = Blocked }, { .name = "use", .use = Use },
		{ .name = "pain", .pain = Pain }, { .name = "die", .die = Die }, { nullptr }
	};
	const game::gSaveCallback_t *tables[] = { first, second, nullptr };
	game::gentity_t entity{};
	entity.think = Think;
	entity.reached = Think;
	entity.blocked = Blocked;
	entity.touch = Touch;
	entity.use = Use;
	entity.pain = Pain;
	entity.die = Die;
	game::gCallbackNames_t names{};
	assert( game::G_CaptureCallbacks( entity, tables, &names ) );
	assert( !strcmp( names.think, "think" ) && !strcmp( names.reached, "reached" ) );
	unsigned char bytes[4096];
	const size_t size = State_Write( game::callbackNamesSchema, &names, bytes, sizeof( bytes ) );
	assert( size );
	game::gCallbackNames_t decoded{};
	uint32_t version;
	assert( State_Read( game::callbackNamesSchema, bytes, size, &decoded, &version ) && version == 1 );
	game::gentity_t restored{};
	restored.health = 37;
	assert( game::G_RestoreCallbacks( decoded, tables, &restored ) );
	assert( restored.think == Think && restored.reached == Think && restored.touch == Touch && restored.blocked == Blocked );
	assert( restored.use == Use && restored.pain == Pain && restored.die == Die && restored.health == 37 );
	const auto original = restored;
	strcpy( decoded.die, "missing" );
	assert( !game::G_RestoreCallbacks( decoded, tables, &restored ) );
	assert( !memcmp( &original, &restored, sizeof( restored ) ) );
	strcpy( decoded.die, "touch" );
	assert( !game::G_RestoreCallbacks( decoded, tables, &restored ) );
	assert( !memcmp( &original, &restored, sizeof( restored ) ) );
	memset( decoded.think, 'x', sizeof( decoded.think ) );
	assert( !game::G_RestoreCallbacks( decoded, tables, &restored ) );
	entity.think = OtherThink;
	const auto previousNames = names;
	assert( !game::G_CaptureCallbacks( entity, tables, &names ) );
	assert( !memcmp( &previousNames, &names, sizeof( names ) ) );
	decoded = {};
	assert( game::G_RestoreCallbacks( decoded, tables, &restored ) );
	assert( !restored.think && !restored.reached && !restored.touch && !restored.blocked && !restored.use && !restored.pain && !restored.die );
	puts( "PASS: named callbacks preserve all signatures and reject unknown or mismatched identities before mutation" );
}

#include "../../engine/botlib/be_ai_chat.cpp"
#include <stdio.h>
#include <type_traits>

namespace game {
#include "../../game/game/be_ai_chat.h"
}

static_assert( sizeof( bot_matchvariable_t ) == 8 );
static_assert( sizeof( bot_match_t ) == 328 );
static_assert( sizeof( game::bot_match_t ) == sizeof( bot_match_t ) );
static_assert( offsetof( game::bot_matchvariable_t, length ) == offsetof( bot_matchvariable_t, length ) );
static_assert( std::is_trivially_copyable_v<bot_match_t> && std::is_standard_layout_v<bot_match_t> );

botlib_import_t botimport;
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}

int main( void ) {
	char text[] = "hello";
	bot_matchstring_t string = { text, NULL };
	bot_matchpiece_t piece = { MT_STRING, &string, 0, NULL };
	bot_matchtemplate_t pattern = { 1, 0, 0, &piece, NULL };
	bot_match_t match = {};
	matchtemplates = &pattern;
	match.string[127] = 'P';
	match.string[255] = 'Q';
	match.variables[0].length = 1;
	if ( !BotFindMatch( text, &match, 1 ) )
		return 1;

	int failures = 0;
	const int offsets[] = { -1, 0, 127 };
	for ( int offset : offsets ) {
		if ( offset >= 0 )
			match.variables[0].offset = offset;
		const char *expected = offset < 0 ? "" : offset == 0 ? "h"
															 : "P";
		char extracted[8] = {};
		char expanded[8] = {};
		BotMatchVariable( &match, 0, extracted, sizeof( extracted ) );
		BotExpandChatMessage( expanded, sizeof( expanded ), "\1v0\1", 0, &match, 0, qfalse );
		if ( strcmp( extracted, expected ) || strcmp( expanded, expected ) ) {
			fprintf( stderr, "offset %d: expected '%s', extracted '%s', expanded '%s'\n", offset, expected, extracted, expanded );
			++failures;
		}
	}
	game::bot_matchvariable_t mirrored = {};
	mirrored.offset = -1;
	if ( mirrored.offset >= 0 ) {
		fprintf( stderr, "game declaration lost the negative sentinel\n" );
		++failures;
	}
	return failures ? 1 : 0;
}

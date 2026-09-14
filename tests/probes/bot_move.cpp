#include "../../code/qcommon/q_shared.h"
#include "../../code/botlib/botlib.h"
#include "../../code/botlib/be_ai_goal.h"
#include "../../code/botlib/be_ai_move.h"

extern botlib_import_t botimport;
static int diagnostics;

static void QDECL print( int, const char *, ... ) {
	diagnostics++;
}

extern "C" int __wrap_main( void ) {
	bot_moveresult_t result;
	const bot_moveresult_t empty = {};
	const int poisons[] = { 0x55, 0xA5 };
	botimport.Print = print;
	for ( int poison : poisons ) {
		memset( &result, poison, sizeof( result ) );
		BotMoveToGoal( &result, 0, NULL, 0 );
		if ( memcmp( &result, &empty, sizeof( result ) ) ) {
			fprintf( stderr, "FAIL: BotMoveToGoal left stale output fields\n" );
			return 1;
		}
	}
	assert( diagnostics == 2 );
	puts( "PASS: BotMoveToGoal initializes the complete result" );
	return 0;
}

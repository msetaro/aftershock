#include "q_shared.h"

void Info_RemoveKey_Big( char *s, const char *key );

void QDECL Com_Error( int code, const char *message, ... ) {
	abort();
}

int main( int argc, char **argv ) {
	static const struct {
		const char *input;
		const char *key;
		const char *expected;
	} cases[] = {
		{ "\\a\\1\\b\\23456789\\c\\xyz", "a", "\\b\\23456789\\c\\xyz" },
		{ "\\a\\1\\b\\23456789\\c\\xyz", "b", "\\a\\1\\c\\xyz" },
		{ "\\a\\1\\b\\23456789\\c\\xyz", "c", "\\a\\1\\b\\23456789" },
		{ "\\a\\1", "a", "" },
		{ "\\a\\1\\b\\2", "missing", "\\a\\1\\b\\2" },
		{ "", "a", "" },
		{ "\\a\\1\\b\\2", "a\\b", "\\a\\1\\b\\2" }
	};
	void ( *removeKey )( char *, const char * );
	unsigned i;
	char info[128];

	assert( argc == 2 );
	removeKey = !strcmp( argv[1], "big" ) ? Info_RemoveKey_Big : Info_RemoveKey;
	for ( i = 0; i < sizeof( cases ) / sizeof( cases[0] ); i++ ) {
		strcpy( info, cases[i].input );
		removeKey( info, cases[i].key );
		assert( !strcmp( info, cases[i].expected ) );
	}
	return 0;
}

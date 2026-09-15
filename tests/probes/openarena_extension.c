#include "q_shared.h"
#include <assert.h>

void QDECL Com_Error( int level, const char *format, ... ) {
	abort();
}

int main( void ) {
	const char *names[] = {
		"models/weapons2/grenadel/grenadel.md3", "models/weapons2/machinegun/machinegun.md3",
		"directory.ext/model", "model", "model.one.two"
	};
	const char *expected[] = {
		"models/weapons2/grenadel/grenadel", "models/weapons2/machinegun/machinegun",
		"directory.ext/model", "model", "model.one"
	};
	char path[MAX_QPATH], separate[MAX_QPATH];
	int i;
	for ( i = 0; i < sizeof(names) / sizeof(names[0]); i++ ) {
		strcpy( path, names[i] );
		COM_StripExtension( path, path, sizeof(path) );
		COM_StripExtension( names[i], separate, sizeof(separate) );
		assert( !strcmp( path, expected[i] ) );
		assert( !strcmp( separate, expected[i] ) );
	}
	strcpy( path, "model.md3" );
	COM_StripExtension( path, path, 4 );
	assert( !strcmp( path, "mod" ) && path[4] == 'l' );
	return 0;
}

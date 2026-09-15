#include "q_shared.h"
#include <assert.h>

int main( void ) {
	const char *names[] = { "elimination_active", NULL, "other", "" };
	int i, j, index = 0;
	for ( i = 0; i < 4; i++ ) {
		for ( j = 0; j < 4; j++ ) {
			assert( strequals( names[i], names[j] ) == (i == j && i != 1) );
		}
	}
	assert( !strequals( "elimination_active", "ELIMINATION_ACTIVE" ) );
	assert( strequals( names[index++], "elimination_active" ) );
	assert( index == 1 );
	return 0;
}

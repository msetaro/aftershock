#include "../../engine/ui/ui_public.h"
#include "../../third_party/sha256/sha-256.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main( int argc, char **argv ) {
	assert( argc == 2 );
	FILE *file = fopen( argv[1], "rb" );
	assert( file );
	static unsigned char bytes[65536];
	const size_t size = fread( bytes, 1, sizeof( bytes ), file );
	assert( feof( file ) );
	fclose( file );
	static uiDocument_t document;
	assert( UI_ReadDocument( bytes, size, &document ) );
	assert( document.items[1].action == UI_ACTION_BACKEND && !strcmp( document.items[1].target, "login" ) );
	uiDocumentState_t state{};
	assert( UI_OpenDocument( document, &state, "main" ) && state.focus == 1 );
	UI_Navigate( document, &state, 1 );
	assert( !strcmp( document.items[state.focus].target, "queue" ) );
	assert( document.items[7].kind == UI_VALUE && !strcmp( document.items[7].target, "backend_status" ) );
	// The byte reader must validate targets too, independently of the cooker.
	const size_t item = 48 + sizeof( uiDocumentHeader_t ) + document.header.pageCount * sizeof( uiPage_t ) + sizeof( uiItem_t );
	strcpy( (char *)bytes + item + 44, "quit;inject" );
	calc_sha_256( bytes + 16, bytes + 48, size - 48 );
	assert( !UI_ReadDocument( bytes, size, &document ) );
	puts( "PASS: authored backend actions, profile values and rejected command targets" );
}

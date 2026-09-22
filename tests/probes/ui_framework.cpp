#include "../../engine/ui/ui_public.h"
#include <assert.h>
#include <cmath>
#include <stdio.h>
#include <string.h>

int main( int argc, char **argv ) {
	assert( argc == 2 );
	FILE *file = fopen( argv[1], "rb" );
	assert( file );
	static unsigned char bytes[65536];
	const size_t fileSize = fread( bytes, 1, sizeof( bytes ), file );
	assert( feof( file ) );
	fclose( file );
	static uiDocument_t document;
	assert( UI_ReadDocument( bytes, fileSize, &document ) );
	assert( document.header.pageCount == 3 && document.header.itemCount == 10 );
	assert( document.header.textCount == 8 && document.header.localeCount == 2 );
	assert( document.header.glyphCount == 95 );
	assert( !strcmp( document.locales[1].name, "ar" ) && document.locales[1].rtl );
	const auto &arabic = document.runs[document.header.textCount];
	assert( arabic.width > 0 && arabic.advance > 0 );
	assert( arabic.width != document.runs[0].width );
	uiDocumentState_t state = {};
	assert( UI_OpenDocument( document, &state, "main" ) );
	assert( state.focus == 1 ); // The title is not interactive.
	UI_Navigate( document, &state, 1 );
	assert( state.focus == 2 && document.items[state.focus].action == UI_ACTION_PAGE );
	UI_Navigate( document, &state, -1 );
	assert( state.focus == 1 );
	UI_Navigate( document, &state, -1 );
	assert( state.focus == 3 );
	UI_Navigate( document, &state, 1 );
	assert( state.focus == 1 );
	assert( UI_OpenDocument( document, &state, "options" ) && state.focus == 5 );
	assert( UI_Nudge( document.items[state.focus], .95f, 1 ) == 1.0f );
	assert( UI_Nudge( document.items[state.focus], .05f, -1 ) == 0.0f );
	assert( UI_OpenDocument( document, &state, "hud" ) && state.focus == UI_NO_FOCUS );
	UI_Navigate( document, &state, 1 );
	assert( state.focus == UI_NO_FOCUS );
	const int sizes[][2] = { { 1920, 1080 }, { 2560, 1440 }, { 3840, 2160 }, { 3440, 1440 }, { 1280, 1024 } };
	for ( const auto &size : sizes ) {
		for ( uint32_t i = 0; i < document.header.itemCount; ++i ) {
			uiRectangle_t rectangle;
			assert( UI_Layout( document, document.items[i], size[0], size[1], .05f, &rectangle ) );
			assert( rectangle.x >= float( size[0] ) * .05f - .01f && rectangle.y >= float( size[1] ) * .05f - .01f );
			assert( rectangle.x + rectangle.width <= float( size[0] ) * .95f + .01f );
			assert( rectangle.y + rectangle.height <= float( size[1] ) * .95f + .01f );
			assert( rectangle.scale > 0 );
		}
	}
	uiRectangle_t a, b;
	assert( UI_Layout( document, document.items[1], 1920, 1080, .05f, &a ) );
	assert( UI_Layout( document, document.items[1], 3840, 2160, .05f, &b ) );
	assert( std::fabs( b.x - 2 * a.x ) < .01f && std::fabs( b.y - 2 * a.y ) < .01f );
	assert( std::fabs( b.width - 2 * a.width ) < .01f && std::fabs( b.height - 2 * a.height ) < .01f );
	puts( "PASS: cooked localized UI, focus/navigation, slider bounds, safe-area layouts at five resolutions" );
}

#include "devtools_public.h"
#include <string.h>

static char consoleText[32768];
static size_t consoleLength;

void DevTools_Log( const char *text ) {
	size_t length = strlen( text );
	if ( length >= sizeof( consoleText ) ) {
		text += length - ( sizeof( consoleText ) - 1 );
		length = sizeof( consoleText ) - 1;
	}
	if ( consoleLength + length >= sizeof( consoleText ) ) {
		const size_t discard = consoleLength + length - ( sizeof( consoleText ) - 1 );
		memmove( consoleText, consoleText + discard, consoleLength - discard );
		consoleLength -= discard;
	}
	memcpy( consoleText + consoleLength, text, length + 1 );
	consoleLength += length;
}

const char *DevTools_Console( void ) {
	return consoleText;
}

#include "../devtools/devtools_public.h"
#include "../qcommon/qcommon_public.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <errno.h>
#endif

// Pipes are private to the parent process; no socket or listening port exists.
int Sys_AgentRead( char *line, uint32_t capacity ) {
	static char input[4096];
	static uint32_t cursor, available;
	uint32_t length = 0;
	bool overflow = false;
	for ( ;; ) {
		if ( cursor == available ) {
#ifdef _WIN32
			DWORD count = 0;
			if ( !ReadFile( GetStdHandle( STD_INPUT_HANDLE ), input, sizeof( input ), &count, nullptr ) || !count )
				return -1;
			available = count;
#else
			const ssize_t count = read( STDIN_FILENO, input, sizeof( input ) );
			if ( count < 0 && errno == EINTR )
				continue;
			if ( count <= 0 )
				return -1;
			available = (uint32_t)count;
#endif
			cursor = 0;
		}
		const char c = input[cursor++];
		if ( c == '\n' ) {
			line[length] = 0;
			return overflow ? 0 : (int)length;
		}
		if ( length + 1 < capacity )
			line[length++] = c;
		else
			overflow = true; // Drain the entire line before accepting another request.
	}
}

bool Sys_AgentWrite( const char *text, uint32_t length ) {
	while ( length ) {
#ifdef _WIN32
		DWORD count = 0;
		if ( !WriteFile( GetStdHandle( STD_OUTPUT_HANDLE ), text, length, &count, nullptr ) || !count )
			return false;
#else
		const ssize_t count = write( STDOUT_FILENO, text, length );
		if ( count < 0 && errno == EINTR )
			continue;
		if ( count <= 0 )
			return false;
#endif
		text += count;
		length -= (uint32_t)count;
	}
	return true;
}

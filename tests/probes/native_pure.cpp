/* Exercise the real filesystem pure list with statically linked game modules. */
#include "../../engine/qcommon/files.cpp"
#include <assert.h>

void QDECL Com_Printf( const char *, ... ) {}
void QDECL Com_DPrintf( const char *, ... ) {}
void QDECL Com_Error( errorParm_t, const char *, ... ) { abort(); }

int main() {
	fs_checksumFeed = 123;
	fs_searchpaths = NULL;
	assert( !strcmp( FS_ReferencedPakPureChecksums( 1024 ), "0 0 @ 123" ) );
	static pack_t content;
	static searchpath_t path;
	content.referenced = FS_GENERAL_REF;
	content.pure_checksum = 17;
	path.pack = &content;
	fs_searchpaths = &path;
	assert( !strcmp( FS_ReferencedPakPureChecksums( 1024 ), "0 0 @ 17 107" ) );
	return 0;
}

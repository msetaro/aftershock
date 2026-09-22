#include "../../engine/qcommon/package_public.h"
#include "../../engine/qcommon/qcommon_public.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int allocations, live;
void *Z_TagMalloc( size_t size, memtag_t ) {
	void *memory = malloc( size );
	assert(memory);
	++allocations;
	++live;
	return memory;
}
void *Z_Malloc( size_t size ) {
	void *memory = Z_TagMalloc( size, TAG_PACK );
	memset( memory, 0, size );
	return memory;
}
void Z_Free( void *memory ) {
	if ( memory ) {
		--live;
		free( memory );
	}
}
FILE *Sys_FOpen( const char *path, const char *mode ) {
	return fopen( path, mode );
}
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}

static void Compare( package_t *package, const char *name, const char *root ) {
	const int index = Package_Find( package, name );
	assert(index>=0);
	auto *stream = Package_OpenAsset( package, uint32_t( index ) );
	assert(stream);
	char path[1024];
	snprintf( path, sizeof( path ), "%s/%s", root, name );
	FILE *reference = fopen( path, "rb" );
	assert(reference);
	const int before = allocations;
	unsigned char expected[257], actual[257];
	uint64_t total = 0;
	for ( ;; ) {
		const size_t count = fread( expected, 1, sizeof( expected ), reference );
		assert(Package_ReadAsset(stream,actual,sizeof(actual))==int(count));
		assert(!memcmp(expected,actual,count));
		total += count;
		assert(Package_TellAsset(stream)==total);
		if ( count < sizeof( expected ) )
			break;
	}
	assert(Package_SeekAsset(stream,total/2));
	assert(!fseek(reference,int(total/2),SEEK_SET));
	const size_t count = fread( expected, 1, sizeof( expected ), reference );
	assert(Package_ReadAsset(stream,actual,sizeof(actual))==int(count));
	assert(!memcmp(expected,actual,count));
	assert(Package_SeekAsset(stream,0)&&Package_TellAsset(stream)==0);
	assert(Package_SeekAsset(stream,-1,FS_SEEK_END)&&Package_TellAsset(stream)==total-1);
	assert(Package_SeekAsset(stream,1,FS_SEEK_CUR)&&Package_TellAsset(stream)==total);
	assert(!Package_SeekAsset(stream,INT64_MAX,FS_SEEK_CUR)&&Package_TellAsset(stream)==total);
	assert(!Package_SeekAsset(stream,INT64_MIN,FS_SEEK_END)&&Package_TellAsset(stream)==total);
	assert(!Package_SeekAsset(stream,1,FS_SEEK_END)&&Package_TellAsset(stream)==total);
	assert(Package_SeekAsset(stream,-int64_t(total),FS_SEEK_CUR)&&Package_TellAsset(stream)==0);
	assert(allocations==before); // No per-read/seek zone allocation.
	fclose( reference );
	Package_CloseAsset( stream );
}

int main( int argc, char **argv ) {
	assert(argc==5);
	package_t *packages[3];
	for ( int i = 0; i < 3; ++i ) {
		packages[i] = Package_Load( argv[i + 1] );
		assert(packages[i]);
	}
	uint8_t identity[32];
	assert(Package_ValidateMounts(packages,3,identity));
	assert(!Package_ValidateMounts(packages+1,1,identity)); // Patch requires its base.
	Compare( packages[0], "models/rig.iqm", argv[4] );
	Compare( packages[0], "sounds/wav.wav", argv[4] );
	Compare( packages[1], "textures/bc7.ktx2", argv[4] );
	const int deleted = Package_Find( packages[2], "obsolete.cfg" );
	assert(deleted>=0&&!Package_OpenAsset(packages[2],uint32_t(deleted)));
	for ( auto *package : packages )
		Package_Free( package );
	assert(!live);
	puts( "PASS: native package hashes, patch identities, stored/compressed streams, seeks and complete release" );
}

#include "../../engine/platform/save_public.h"
#include <cassert>
#include <cstdio>
#include <cstring>

static int pcCalls, providerCalls, revisions;
static bool failWrite, invalidSize;
static const char payload[] = "saved state";
int FS_ReadSave( const char *, void *data, int capacity ) {
	++pcCalls;
	if ( data && capacity >= int( sizeof( payload ) ) )
		memcpy( data, payload, sizeof( payload ) );
	return sizeof( payload );
}
saveWriteResult_t FS_CreateSave( const char *, const void *, int ) {
	++pcCalls;
	return saveWriteResult_t::Written;
}
static int Read( const char *path, void *data, int capacity ) {
	++providerCalls;
	assert(!strcmp(path,"profiles/legacy.asstate"));
	if ( invalidSize )
		return capacity + 1;
	if ( data && capacity >= int( sizeof( payload ) ) )
		memcpy( data, payload, sizeof( payload ) );
	return sizeof( payload );
}
static saveWriteResult_t Create( const char *path, const void *data, int size ) {
	++providerCalls;
	assert(size==sizeof(payload) && !memcmp(data,payload,sizeof(payload)));
	if ( failWrite )
		return saveWriteResult_t::Failed;
	char expected[64];
	snprintf( expected, sizeof( expected ), "saves/test.%03d.asstate", revisions++ );
	assert(!strcmp(path,expected));
	return revisions == 3 ? saveWriteResult_t::Written : saveWriteResult_t::Exists;
}
int main( int argc, char ** ) {
	char path[64], bytes[64];
	const saveProvider_t provider{ Read, Create }, invalid{ Read, nullptr };
	assert(!Sys_InstallSaveProvider(&invalid));
	assert(Sys_InstallSaveProvider(nullptr));
	assert(!Sys_SaveRevision(saveKind_t::Game,"../escape",payload,sizeof(payload),path,sizeof(path)));
	assert(!Sys_SaveRevision(saveKind_t::Game,"valid",payload,sizeof(payload),path,4));
	assert(Sys_ReadSave("profiles/../x.asstate",bytes,sizeof(bytes))==-1);
	assert(Sys_ReadSave("profiles/sub/x.asstate",bytes,sizeof(bytes))==-1);
	assert(Sys_ReadSave("other/x.asstate",bytes,sizeof(bytes))==-1);
	assert(Sys_ReadSave("profiles/x.cfg",bytes,sizeof(bytes))==-1);
	assert(!pcCalls && !providerCalls);
	if ( argc == 1 ) {
		assert(Sys_SaveRevision(saveKind_t::Profile,"test",payload,sizeof(payload),path,sizeof(path)));
		assert(!strcmp(path,"profiles/test.000.asstate"));
		assert(Sys_ReadSave("profiles/test.000.asstate",nullptr,0)==sizeof(payload));
		assert(Sys_ReadSave("profiles/test.000.asstate",bytes,sizeof(bytes))==sizeof(payload));
		assert(!memcmp(bytes,payload,sizeof(payload)) && pcCalls==3 && !providerCalls);
	} else {
		assert(Sys_InstallSaveProvider(&provider));
		assert(Sys_SaveRevision(saveKind_t::Game,"test",payload,sizeof(payload),path,sizeof(path)));
		assert(!strcmp(path,"saves/test.002.asstate") && revisions==3);
		failWrite = true;
		const int calls = providerCalls;
		assert(!Sys_SaveRevision(saveKind_t::Game,"test",payload,sizeof(payload),path,sizeof(path)));
		assert(!path[0] && providerCalls==calls+1); // Storage failure does not retry or fall back to PC.
		assert(Sys_ReadSave("profiles/legacy.asstate",nullptr,0)==sizeof(payload));
		assert(Sys_ReadSave("profiles/legacy.asstate",bytes,sizeof(bytes))==sizeof(payload));
		assert(!memcmp(bytes,payload,sizeof(payload)));
		invalidSize = true;
		assert(Sys_ReadSave("profiles/legacy.asstate",bytes,sizeof(bytes))==-1);
		assert(!pcCalls);
	}
	assert(!Sys_InstallSaveProvider(nullptr) && !Sys_InstallSaveProvider(&provider));
	puts( "PASS: platform save routing, immutable revisions, bounded paths and provider failures" );
}

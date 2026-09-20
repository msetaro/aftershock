// Traverse real zone lists, including segment separators and free blocks.
#include "../../engine/qcommon/common.cpp"
#include <assert.h>

int main() {
	memzone_t zone = {};
	memblock_t blocks[4] = {};
	const memtag_t tags[] = { TAG_GENERAL, TAG_GENERAL, TAG_FREE, TAG_DEVTOOLS };
	const uint32_t sizes[] = { 64, 0, 128, 256 };
	zone.blocklist.next = blocks;
	for ( int i = 0; i < 4; ++i ) {
		blocks[i].tag = tags[i];
		blocks[i].size = sizes[i];
		blocks[i].next = i == 3 ? &zone.blocklist : &blocks[i + 1];
	}
	mainzone = &zone;
	s_hunkTotal = 1000;
	hunk_low.permanent = 100;
	hunk_low.temp = 200;
	hunk_high.permanent = 300;
	hunk_high.temp = 100;
	devMemory_t memory;
	Com_DeveloperMemory( &memory );
	assert( memory.bytes[TAG_GENERAL] == 64 && memory.blocks[TAG_GENERAL] == 1 );
	assert( memory.bytes[TAG_FREE] == 128 && memory.bytes[TAG_DEVTOOLS] == 256 );
	assert( !strcmp( memory.names[TAG_DEVTOOLS], "DEVTOOLS" ) );
	assert( memory.hunkTotal == 1000 && memory.hunkPermanent == 400 && memory.hunkTemporary == 100 && memory.hunkFree == 500 );
	mainzone = nullptr;
	Com_DeveloperMemory( &memory );
	assert( memory.bytes[TAG_GENERAL] == 0 && memory.blocks[TAG_GENERAL] == 0 );
}

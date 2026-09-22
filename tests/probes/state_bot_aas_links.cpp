#include "../../engine/botlib/be_aas_sample.cpp"
#include <assert.h>
aas_t aasworld;
botlib_import_t botimport;
int botDeveloper;
void *GetHunkMemory(size_t) {
	assert(false);
	return nullptr;
}
int LibVarInteger( const char *, const char *, int, int ) {
	assert(false);
	return 0;
}
static aas_link_t original[MAX_SAVED_AAS_LINKS], loaded[MAX_SAVED_AAS_LINKS];
static aas_entity_t entities[3], restored[3];
static aas_link_t *areaHeads[3], *loadedHeads[3];
int main() {
	aasworld.loaded = aasworld.initialized = 1;
	aasworld.linkheap = original;
	aasworld.linkheapsize = 8;
	aasworld.entities = entities;
	aasworld.maxentities = 3;
	aasworld.numareas = 3;
	aasworld.arealinkedentities = areaHeads;
	AAS_InitAASLinkHeap();
	aas_plane_t plane{};
	plane.normal[0] = 1;
	plane.type = 0;
	aasworld.planes = &plane;
	aas_node_t nodes[2]{};
	nodes[1].children[0] = -1;
	nodes[1].children[1] = -2;
	aasworld.nodes = nodes;
	vec3_t mins{ -1, -1, -1 }, maxs{ 1, 1, 1 };
	entities[1].areas = AAS_AASLinkEntity( mins, maxs, 1 );
	entities[2].areas = AAS_AASLinkEntity( mins, maxs, 2 );
	assert(numaaslinks==4);
	static unsigned char bytes[2 * 1024 * 1024];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(AAS_WriteLinkState(&writer));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	const int entityHead = AASLinkSlot( entities[1].areas ), areaHead = AASLinkSlot( areaHeads[1] );
	AAS_UnlinkFromAreas( entities[1].areas );
	entities[1].areas = nullptr;
	entities[1].areas = AAS_AASLinkEntity( mins, maxs, 1 );
	const int nextHead = AASLinkSlot( entities[1].areas ), nextAreaHead = AASLinkSlot( areaHeads[1] );
	aasworld.linkheap = loaded;
	aasworld.entities = restored;
	aasworld.arealinkedentities = loadedHeads;
	assert(AAS_ReadLinkState(reader,false) && !restored[1].areas);
	assert(AAS_ReadLinkState(reader,true));
	assert(AASLinkSlot(restored[1].areas)==entityHead && AASLinkSlot(loadedHeads[1])==areaHead && numaaslinks==4);
	AAS_UnlinkFromAreas( restored[1].areas );
	restored[1].areas = nullptr;
	restored[1].areas = AAS_AASLinkEntity( mins, maxs, 1 );
	assert(AASLinkSlot(restored[1].areas)==nextHead && AASLinkSlot(loadedHeads[1])==nextAreaHead && numaaslinks==4);
	aasLinksHeader_t header;
	uint32_t version;
	assert(State_Find(reader,aasLinksHeaderSchema,0,&header,&version));
	assert(AASLinkRecords(nullptr,&reader,header));
	savedAASLinks.next_area[entityHead] = entityHead;
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,aasLinksHeaderSchema,0,&header) && AASLinkRecords(&writer,nullptr,header));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	auto *before = restored[1].areas;
	auto *free = aasworld.freelinks;
	assert(!AAS_ReadLinkState(reader,true) && restored[1].areas==before && aasworld.freelinks==free && numaaslinks==4);
	// A link cannot appear in a live list and the free list simultaneously.
	aasworld.freelinks = restored[1].areas;
	writer = { bytes, sizeof( bytes ) };
	assert(!AAS_WriteLinkState(&writer) && !State_Finish(&writer));
	// Maximum-size all-free heap; discarded payload is deliberately invalid.
	memset( entities, 0, sizeof( entities ) );
	memset( areaHeads, 0, sizeof( areaHeads ) );
	aasworld.linkheap = original;
	aasworld.linkheapsize = MAX_SAVED_AAS_LINKS;
	aasworld.entities = entities;
	aasworld.arealinkedentities = areaHeads;
	AAS_InitAASLinkHeap();
	original[MAX_SAVED_AAS_LINKS - 1].entnum = -42;
	original[0].next_area = reinterpret_cast<aas_link_t *>( uintptr_t( 1 ) );
	writer = { bytes, sizeof( bytes ) };
	assert(AAS_WriteLinkState(&writer) && writer.records==2);
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	aasworld.linkheap = loaded;
	aasworld.entities = restored;
	aasworld.arealinkedentities = loadedHeads;
	assert(AAS_ReadLinkState(reader,true) && numaaslinks==MAX_SAVED_AAS_LINKS);
	assert(AAS_AllocAASLink()==&loaded[0] && numaaslinks==MAX_SAVED_AAS_LINKS-1);
	puts( "PASS: AAS spatial lists and maximum free heap preserve exact unlink/relink continuation after relocation" );
}

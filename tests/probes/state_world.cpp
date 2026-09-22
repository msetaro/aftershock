#include "../../engine/server/sv_world.cpp"
#include <cassert>
#include <cstdlib>

server_t sv;
static sharedEntity_t entities[4];
sharedEntity_t *SV_GentityNum( int slot ) {
	return &entities[slot];
}
sharedEntity_t *SV_GEntityForSvEntity( svEntity_t *entity ) {
	return &entities[entity - sv.svEntities];
}
int CM_NumClusters() {
	return 16;
}
int CM_NumAreas() {
	return 1;
}
void QDECL Com_Printf( const char *, ... ) {
}
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}

int main() {
	static unsigned char bytes[128 * 1024];
	sv.num_entities = 4;
	sv_numworldSectors = 1;
	sv_worldSectors[0].axis = -1;
	const int order[] = { 2, 0, 1 };
	for ( int i = 0; i < 3; ++i ) {
		const int slot = order[i];
		auto &entity = entities[slot];
		entity.r.linked = qtrue;
		entity.r.currentOrigin[0] = 0.25f;
		for ( int axis = 0; axis < 3; ++axis ) {
			entity.r.absmin[axis] = -1;
			entity.r.absmax[axis] = 1;
		}
		auto &link = sv.svEntities[slot];
		link.worldSector = &sv_worldSectors[0];
		link.nextEntityInWorldSector = i < 2 ? &sv.svEntities[order[i + 1]] : nullptr;
		link.numClusters = 1;
		link.clusternums[0] = slot;
		link.areanum = 0;
		link.areanum2 = -1;
	}
	sv_worldSectors[0].entities = &sv.svEntities[2];
	int before[4], after[4];
	const vec3_t mins = { -2, -2, -2 }, maxs = { 2, 2, 2 };
	assert(SV_AreaEntities(mins,maxs,before,4)==3 && !memcmp(before,order,sizeof(order)));
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(SV_WriteWorldState(&writer));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	memset( sv.svEntities, 0, sizeof( sv.svEntities ) );
	sv_worldSectors[0].entities = nullptr;
	assert(SV_ReadWorldState(reader,false) && !sv_worldSectors[0].entities);
	assert(SV_ReadWorldState(reader,true));
	assert(SV_AreaEntities(mins,maxs,after,4)==3 && !memcmp(before,after,sizeof(order)));
	assert(entities[0].r.currentOrigin[0]==0.25f && entities[0].r.absmin[0]==-1);
	assert(sv.svEntities[2].clusternums[0]==2);
	worldSave_t bad;
	uint32_t version;
	assert(State_Find(reader,worldSaveSchema,0,&bad,&version));
	bad.next[0] = 0;
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,worldSaveSchema,0,&bad));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(!SV_ReadWorldState(reader,true));
	assert(SV_AreaEntities(mins,maxs,after,4)==3 && !memcmp(before,after,sizeof(order)));
	puts( "PASS: exact spatial list order, saved snapped bounds, cluster membership and cycle rejection" );
}
